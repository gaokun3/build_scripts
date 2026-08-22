#!/usr/bin/env bash
# 造 gaokun3 救援/LiveCD 的根文件系统（Alpine aarch64）→ squashfs。
#
#   sudo bash scripts/live/build-rootfs.sh --profile rescue --out /tmp/gk3live \
#        --ssh-key ~/.ssh/ed25519.pub
#
# profile：
#   rescue  无图形，只有 ssh + 分区/文件系统工具。装在内置盘上。
#   live    rescue + 图形安装器。做成 U 盘。
#
# ⚠️ 必须 root（要 chroot、要建设备节点）。
# ⚠️ 交叉构建（在 x86_64 上造 aarch64）需要 qemu-user-static + binfmt；
#    脚本会先检查，缺了就明确报错，而不是让 apk 在半路吐一堆看不懂的东西。
#
# 设计取舍见 docs/stage7-live-installer.md。
set -euo pipefail

# ---- 钉死的上游 ----------------------------------------------------------
ALPINE_BRANCH=v3.24
ALPINE_VER=3.24.1
ALPINE_ARCH=aarch64
ROOTFS_SHA256=f55a90f69052c5bd6f92cb09a8f47065970830b194c917a006fb94028e721259
MIRROR=https://dl-cdn.alpinelinux.org/alpine

REPO=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
LIVE=$REPO/scripts/live

PROFILE=rescue
OUT=
SSH_KEY=
SDBOOT=
WIFI_CONF=
KEEP=

die() { echo "!! $*" >&2; exit 1; }
say() { echo; echo "══ $*"; }
ok()  { echo "   ✓ $*"; }

while [ $# -gt 0 ]; do
    case "$1" in
        --profile) PROFILE=$2; shift 2 ;;
        --out)     OUT=$2; shift 2 ;;
        --ssh-key) SSH_KEY=$2; shift 2 ;;
        --sdboot)  SDBOOT=$2; shift 2 ;;
        --wifi-conf) WIFI_CONF=$2; shift 2 ;;
        --keep)    KEEP=1; shift ;;
        *) die "不认识的参数：$1" ;;
    esac
done

[ -n "$OUT" ] || die "要 --out <目录>"
case "$PROFILE" in rescue|live) ;; *) die "--profile 只能是 rescue 或 live" ;; esac
[ "$(id -u)" = 0 ] || die "要 root（chroot + mknod）"

for t in curl tar chroot mksquashfs cpio sha256sum; do
    command -v "$t" >/dev/null || die "缺工具：$t（Debian/Ubuntu: apt install squashfs-tools cpio curl）"
done

WORK=$OUT/work-$PROFILE
ROOTFS=$WORK/rootfs
mkdir -p "$OUT" "$WORK"

# ---- 1. 取 minirootfs -----------------------------------------------------
say "1. Alpine $ALPINE_VER $ALPINE_ARCH minirootfs"
TARBALL=$OUT/alpine-minirootfs-$ALPINE_VER-$ALPINE_ARCH.tar.gz
if [ ! -f "$TARBALL" ]; then
    curl -fSL -o "$TARBALL" \
        "$MIRROR/$ALPINE_BRANCH/releases/$ALPINE_ARCH/alpine-minirootfs-$ALPINE_VER-$ALPINE_ARCH.tar.gz"
fi
# ★ 钉的是【我们查过的那个 hash】，不是从镜像现拉一个 .sha256 再比 ——
#   后者只能证明"文件和这个镜像说的一致"，证明不了它没被换过。
GOT=$(sha256sum "$TARBALL" | cut -d' ' -f1)
[ "$GOT" = "$ROOTFS_SHA256" ] || die "minirootfs 校验和不对：$GOT"
ok "校验和一致"

rm -rf "$ROOTFS"; mkdir -p "$ROOTFS"
tar -xzf "$TARBALL" -C "$ROOTFS"
ok "解开到 $ROOTFS"

# ---- 2. 交叉构建准备 ------------------------------------------------------
HOST_ARCH=$(uname -m)
NEED_QEMU=0
if [ "$HOST_ARCH" != "aarch64" ]; then
    NEED_QEMU=1
    QEMU=$(command -v qemu-aarch64-static || true)
    [ -n "$QEMU" ] || die "在 $HOST_ARCH 上造 aarch64 需要 qemu-aarch64-static（apt install qemu-user-static）"
    grep -q 'aarch64' /proc/sys/fs/binfmt_misc/status 2>/dev/null || true
    [ -e /proc/sys/fs/binfmt_misc/qemu-aarch64 ] \
        || die "binfmt 里没有 qemu-aarch64（apt install binfmt-support 或 systemctl restart systemd-binfmt）"
    cp "$QEMU" "$ROOTFS/usr/bin/"
    ok "qemu-aarch64-static 已放进 chroot"
else
    ok "宿主就是 aarch64，不需要 qemu"
fi

# ---- 3. apk ---------------------------------------------------------------
say "2. 装包"
cat > "$ROOTFS/etc/apk/repositories" <<EOF
$MIRROR/$ALPINE_BRANCH/main
$MIRROR/$ALPINE_BRANCH/community
EOF
cp /etc/resolv.conf "$ROOTFS/etc/resolv.conf"

mount -t proc  none "$ROOTFS/proc"
mount --rbind /sys  "$ROOTFS/sys"
mount --rbind /dev  "$ROOTFS/dev"
cleanup() {
    umount -lR "$ROOTFS/dev"  2>/dev/null || true
    umount -lR "$ROOTFS/sys"  2>/dev/null || true
    umount -l  "$ROOTFS/proc" 2>/dev/null || true
}
trap cleanup EXIT

pkglist() { grep -vE '^\s*(#|$)' "$1" | tr '\n' ' '; }
PKGS=$(pkglist "$LIVE/pkgs-common.txt")
[ "$PROFILE" = live ] && PKGS="$PKGS $(pkglist "$LIVE/pkgs-live.txt")"

chroot "$ROOTFS" /sbin/apk update
# shellcheck disable=SC2086
chroot "$ROOTFS" /sbin/apk add $PKGS
ok "装了 $(echo $PKGS | wc -w) 个包"

# ---- 4. 铺 overlay --------------------------------------------------------
say "3. 配置"
copy_overlay() {
    [ -d "$1" ] || return 0
    cp -a "$1"/. "$ROOTFS"/
    ok "铺了 $(basename "$1")"
}
copy_overlay "$LIVE/overlay-common"
[ "$PROFILE" = rescue ] && copy_overlay "$LIVE/overlay-rescue"
[ "$PROFILE" = live ]   && copy_overlay "$LIVE/overlay-live"

echo "gaokun3-rescue" > "$ROOTFS/etc/hostname"

# ★ 开机自启：不用 rc-update（要跑目标架构的脚本），直接建符号链接。
#   OpenRC 的 runlevel 就是一堆指向 /etc/init.d/* 的符号链接，没有别的状态。
for svc in devfs dmesg sysfs hwdrivers; do
    ln -sf "/etc/init.d/$svc" "$ROOTFS/etc/runlevels/sysinit/$svc" 2>/dev/null || true
done
mkdir -p "$ROOTFS/etc/runlevels/default" "$ROOTFS/etc/runlevels/boot"
for svc in hostname bootmisc syslog; do
    [ -f "$ROOTFS/etc/init.d/$svc" ] && ln -sf "/etc/init.d/$svc" "$ROOTFS/etc/runlevels/boot/$svc"
done
for svc in networking gk3-wifi sshd dbus avahi-daemon haveged; do
    [ -f "$ROOTFS/etc/init.d/$svc" ] && ln -sf "/etc/init.d/$svc" "$ROOTFS/etc/runlevels/default/$svc"
done
ok "OpenRC 服务已挂到 runlevel"

# ---- 5. ssh 公钥 ----------------------------------------------------------
if [ -n "$SSH_KEY" ]; then
    [ -f "$SSH_KEY" ] || die "--ssh-key 指的文件不在：$SSH_KEY"
    case "$(head -c 4 "$SSH_KEY")" in
        ssh-|ecds) ;;
        *) die "$SSH_KEY 看着不像公钥 —— ⚠️ 别把私钥装进镜像" ;;
    esac
    mkdir -p "$ROOTFS/root/.ssh"
    cp "$SSH_KEY" "$ROOTFS/root/.ssh/authorized_keys"
    chmod 700 "$ROOTFS/root/.ssh"; chmod 600 "$ROOTFS/root/.ssh/authorized_keys"
    ok "已装入 ssh 公钥：$(cut -d' ' -f3 "$SSH_KEY" 2>/dev/null || echo '(无注释)')"
else
    # ⚠️ 公开发布的 live 镜像【不能】带任何人的公钥。
    [ "$PROFILE" = rescue ] && echo "   ⚠️ 没给 --ssh-key —— 这个救援镜像将【无法 ssh 登录】"
fi

# ---- 5b. WiFi 凭据（可选）------------------------------------------------
# ⚠️ 本仓的规矩：WiFi 凭据不入库。这里也只从【命令行给的文件】读，
#    而且首选是让凭据留在启动介质上（见 /etc/init.d/gk3-wifi 的注释），
#    镜像本身可以一个字都不带。
if [ -n "$WIFI_CONF" ]; then
    [ -f "$WIFI_CONF" ] || die "--wifi-conf 指的文件不在：$WIFI_CONF"
    grep -q 'network=' "$WIFI_CONF" || die "$WIFI_CONF 里没有 network={...}，不像 wpa_supplicant 配置"
    install -Dm600 "$WIFI_CONF" "$ROOTFS/etc/wpa_supplicant/wpa_supplicant.conf"
    ok "已装入 WiFi 配置（$(grep -c 'network=' "$WIFI_CONF") 个网络）"
else
    echo "   ⓘ 没给 --wifi-conf —— 开机时会去启动介质上找"
    echo "     /media/gk3/gaokun3/wpa_supplicant.conf"
fi

# ---- 6. systemd-boot（安装器要往目标机 ESP 上写它）-----------------------
if [ -n "$SDBOOT" ]; then
    [ -f "$SDBOOT" ] || die "--sdboot 指的文件不在：$SDBOOT"
    head -c 2 "$SDBOOT" | grep -q MZ || die "$SDBOOT 不是 PE 文件"
    install -Dm644 "$SDBOOT" "$ROOTFS/usr/share/gaokun3/systemd-bootaa64.efi"
    ok "带上了 systemd-bootaa64.efi（$(stat -c %s "$SDBOOT") 字节）"
else
    echo "   ⚠️ 没给 --sdboot —— 安装器将无法给目标机装引导链"
fi

# ---- 7. 断言（在做成 squashfs 之前，别把坏镜像做出来）--------------------
say "4. 体检"
BAD=0
need_file() { [ -e "$ROOTFS$1" ] && ok "$1" || { echo "   ✗ 缺 $1"; BAD=1; }; }
need_file /sbin/init
need_file /usr/sbin/sshd
need_file /sbin/sgdisk
need_file /usr/sbin/resize2fs
need_file /sbin/mkfs.ext4
need_file /usr/sbin/mkfs.vfat
need_file /usr/sbin/ntfsresize
need_file /usr/bin/simg2img          # ★ super.img 是 sparse，没它装不了 Android
need_file /etc/runlevels/default/sshd
need_file /bin/busybox.static        # initramfs 要用它
need_file /sbin/wpa_supplicant
need_file /sbin/dhcpcd
need_file /etc/init.d/gk3-wifi
need_file /etc/runlevels/default/gk3-wifi
# ★ ath11k 固件：没有它 wlan0 根本不出现，而"没网"在这台机器上等于"救援失效"。
#   路径按设备实测（/vendor/firmware/ath11k/WCN6855/hw2.x/）对齐。
for hw in hw2.0 hw2.1; do
    need_file "/lib/firmware/ath11k/WCN6855/$hw/amss.bin"
    need_file "/lib/firmware/ath11k/WCN6855/$hw/board-2.bin"
done
if [ "$PROFILE" = live ]; then
    need_file /usr/lib/libcairo.so.2
    need_file /usr/lib/libinput.so.10
    need_file /usr/share/fonts/wenquanyi
fi
[ $BAD -eq 0 ] || die "体检没过 —— 不出镜像。上面缺的东西要么包名错了，要么 apk 装失败被吞了。"

# ---- 8. 出 squashfs -------------------------------------------------------
say "5. 打包"
rm -f "$ROOTFS/etc/resolv.conf"
[ "$NEED_QEMU" = 1 ] && rm -f "$ROOTFS/usr/bin/qemu-aarch64-static"
chroot_clean() { rm -rf "$ROOTFS/var/cache/apk"/* "$ROOTFS/tmp"/*; }
cleanup; trap - EXIT; chroot_clean

# initramfs 要的静态 busybox：在删 rootfs 之前先抠出来
cp "$ROOTFS/bin/busybox.static" "$OUT/busybox.static"
ok "留下 busybox.static 给 initramfs 用"

SQUASH=$OUT/gaokun3-$PROFILE.squashfs
rm -f "$SQUASH"
mksquashfs "$ROOTFS" "$SQUASH" -comp zstd -Xcompression-level 19 -noappend -no-progress
ok "$SQUASH  $(du -h "$SQUASH" | cut -f1)"

[ -n "$KEEP" ] || rm -rf "$ROOTFS"
echo
echo "下一步：bash scripts/live/build-initramfs.sh --rootfs-src $OUT --out $OUT"
