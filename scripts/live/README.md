# scripts/live —— 救援系统 / LiveCD 构建

设计与取舍见 [`docs/stage7-live-installer.md`](../../docs/stage7-live-installer.md)。
这里只讲怎么用和有哪些坑。

## 一句话

**救援系统和 LiveCD 是同一套镜像的两个 profile。** 底座 Alpine aarch64，
整个系统跑在内存里（只读 squashfs + tmpfs overlay），
所以救援系统**不再需要一个 24.6 GiB 的分区**。

| profile | 内容 | 落在哪 |
|---|---|---|
| `rescue` | 无图形：ssh + 分区/文件系统工具 | 内置盘的一个小分区 |
| `live`   | `rescue` + 图形安装器 | U 盘 |

## 用法

```sh
# 1. 根文件系统 → squashfs（要 root；x86 上还要 qemu-user-static + binfmt）
sudo bash scripts/live/build-rootfs.sh \
     --profile rescue --out /tmp/gk3 \
     --ssh-key ~/.ssh/ed25519.pub \
     --sdboot /path/to/systemd-bootaa64.efi

# 2. initramfs（不要 root）
bash scripts/live/build-initramfs.sh --busybox /tmp/gk3/busybox.static --out /tmp/gk3
```

产物：`gaokun3-rescue.squashfs` + `initramfs.img`。
**内核直接用 Android 那一个**，不单独编（见下）。

## 几条不显然的设计

### 与 Android 共用同一个内核

ESP 上原来有两个内核（Android 一个、救援 Ubuntu 一个），白占 14 MiB
且要各自维护。现在救援 = **同一个 `vmlinuz.efi` + 另一个 initramfs + 另一条 cmdline**。
为此在 `kernel-config-android.sh` 里补了三项，对 Android 是惰性的：

* `SQUASHFS=y` —— ⚠️ 它**默认是 `=m`**，而 initramfs 里没有模块。
  这是本仓第 14 个「=m 坑」。
* `NTFS3_FS=y` —— 双系统安装要缩 Windows 分区。
  ⚠️ 它的 Kconfig 是 `depends on !NTFS_FS || m`，旧的 `NTFS_FS` 兼容壳开着就
  把它**钉死在 =m**，得先 `--disable NTFS_FS`。
* `NLS_UTF8=y` —— FAT 上的非 ASCII 文件名。

### initramfs 不认标签，挨个找

`initramfs-init` 不靠分区标签/UUID，而是把每个分区挂上去找
`/gaokun3/rescue.squashfs`。一份 init 同时服务 U 盘和内置盘，
**配置越少越不会因为换台机器而失效**。
先扫可移动介质再扫内置盘 —— 插着 LiveCD 启动时，用户要的是 U 盘上那一份。

### ⚠️ 失败时重启，不停在 shell

这台机器没有串口。initramfs 停在 shell 就等于要人到机器旁按电源键。
所以出错默认打印诊断 → 60 秒 → `reboot -f`，回到 systemd-boot 菜单 →
15 秒 → `default`（Android），也就是回到一个能远程接入的系统。
要停下来调试就给 `gk3.debug`。

### ⚠️ 这台机器只有 WiFi

没有网口（USB-C 扩展坞卡在 UCSI 缺陷，TODO A6）。所以
`wpa_supplicant` + `dhcpcd` + `linux-firmware-ath11k` 是**必需项**，
不是可选项 —— 少了它救援系统就是一台连不上的机器。

**凭据不进镜像**：`/etc/init.d/gk3-wifi` 优先读**启动介质上**的
`/media/gk3/gaokun3/wpa_supplicant.conf`。这样公开发布的 LiveCD 不带任何人的
WiFi 密码，而救援镜像换了 WiFi 也不用重造。
构建时注入是备选（`--wifi-conf`），本仓不收这个文件。

### 断言在打包【之前】

`build-rootfs.sh` 在 `mksquashfs` 之前逐个检查关键文件
（`sgdisk` / `resize2fs` / `ntfsresize` / **`simg2img`** / `wpa_supplicant` /
ath11k 固件 / OpenRC 的 runlevel 链接）。
理由是本仓反复吃过的亏：**包名写错时 `apk add` 的失败很容易被吞掉**，
而错误要等到镜像装到机器上、开机连不上网才暴露。

## 现状

* ✅ 脚本写完，包名逐个核对过（`pkgs.alpinelinux.org` 全部 200）
* ⬜ 还没在构建机上真跑过
* ⬜ 图形安装器（`live` profile）还没写
* ⬜ `install-gaokun3.sh` 还是"清空整盘"一条路，未拆成可调用的库
