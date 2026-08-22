#!/usr/bin/env bash
# 造 gaokun3 救援/LiveCD 的 initramfs。
#
#   bash scripts/live/build-initramfs.sh --busybox <busybox.static> --out <目录>
#
# 里面只有：静态 busybox + 我们的 /init。没有模块 —— 本机内核该有的都是 =y
# （NVMe / USB_STORAGE / SQUASHFS / OVERLAY_FS / loop / vfat / ext4）。
# 不需要 root。
set -euo pipefail

REPO=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
LIVE=$REPO/scripts/live
BB=; OUT=

die() { echo "!! $*" >&2; exit 1; }
ok()  { echo "   ✓ $*"; }

while [ $# -gt 0 ]; do
    case "$1" in
        --busybox) BB=$2; shift 2 ;;
        --out)     OUT=$2; shift 2 ;;
        *) die "不认识的参数：$1" ;;
    esac
done
[ -n "$BB" ] && [ -f "$BB" ] || die "要 --busybox <静态 busybox 二进制>（build-rootfs.sh 会在 --out 里留一个 busybox.static）"
[ -n "$OUT" ] || die "要 --out <目录>"
command -v cpio >/dev/null || die "缺 cpio"

# ★ 断言它真是静态的 aarch64 —— 动态链接的 busybox 在 initramfs 里会直接
#   "No such file or directory"（找不到 ld-musl），而那个报错完全指不到根因。
if command -v file >/dev/null; then
    finfo=$(file -b "$BB")
    case "$finfo" in
        *"ARM aarch64"*) ;;
        *) die "$BB 不是 aarch64：$finfo" ;;
    esac
    case "$finfo" in
        *statically*) ok "静态 aarch64 busybox" ;;
        *) die "$BB 不是静态链接的：$finfo" ;;
    esac
fi

D=$OUT/initramfs
rm -rf "$D"
mkdir -p "$D"/{bin,sbin,proc,sys,dev,mnt,lower,upper,newroot}

install -m755 "$BB" "$D/bin/busybox"
# /init 需要的 applet。busybox 靠 argv[0] 分派，所以要有对应的符号链接。
for a in sh ash mount umount mkdir rmdir ls cat echo sleep basename dirname \
         readlink switch_root reboot mdev losetup tr cut grep sed find \
         mknod chmod ln rm dmesg; do
    ln -sf busybox "$D/bin/$a"
done
ln -sf ../bin/busybox "$D/sbin/switch_root"

install -m755 "$LIVE/initramfs-init" "$D/init"
ok "init + $(ls "$D/bin" | wc -l) 个 applet"

IMG=$OUT/initramfs.img
( cd "$D" && find . -print0 | cpio --null -o -H newc --quiet ) | gzip -9 > "$IMG"
ok "$IMG  $(du -h "$IMG" | cut -f1)"

# 体检：解回来看看 /init 在不在、是不是可执行
T=$(mktemp -d)
( cd "$T" && gzip -dc "$IMG" | cpio -idm --quiet )
[ -x "$T/init" ] || die "打出来的 initramfs 里 /init 不可执行"
[ -x "$T/bin/busybox" ] || die "打出来的 initramfs 里没有 busybox"
rm -rf "$T"
ok "回解体检通过"
