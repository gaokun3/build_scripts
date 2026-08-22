# Stage 7 设计：LiveCD 图形安装器 + 轻量救援系统

> 状态：**设计**。这份文档里凡是标"实测"的都有出处，其余是设计选择，还没跑过。
> 目标由用户在 2026-08-23 定下：①用更轻的系统替掉 24.6 GiB 的 Ubuntu 救援；
> ②给 LiveCD 做图形化安装流程；③支持"机器上已有别的系统"时装 Android；
> ④让用户选装不装救援系统。

## 0. 一句话结论：这是**一件事**，不是两件

救援系统和 LiveCD 安装器需要的东西 95% 重合（分区工具、文件系统工具、
网络、ssh、写镜像）。差别只有一个 GUI。所以做**一套镜像、两个 profile**：

| profile | 内容 | 落在哪 | 大小目标 |
|---|---|---|---|
| `rescue` | 无 GUI，ssh + 分区/文件系统工具 | 内置盘 | ≤ 120 MiB |
| `live` | `rescue` + 图形安装器 | U 盘 | ≤ 400 MiB |

## 1. 现实约束（实测，2026-08-23）

| 事实 | 值 | 出处 |
|---|---|---|
| ESP | 300 MiB，**已用 268 MiB，只剩 28 MiB** | `df /mnt/esp` |
| ESP 里最大的一坨 | `Persisted_Capsules.bin` **70 MiB** | `du /mnt/esp/*` |
| ESP 里第二坨 | `EFI/` 31 MiB（含**已抹除的 Windows** 整棵树） | 同上 |
| 每个 Android 槽在 ESP 上 | Image 14 + ramdisk 13 + recovery-ramdisk 15 + dtb 0.17 ≈ 42 MiB | `ls` |
| 其中 `recovery-ramdisk.img` | **没有任何启动项引用它**，×2 = 30 MiB 死重 | grep entries |
| 磁盘 | 476.9 GiB，**约 64 GiB 未分配** | `/proc/partitions` 求和 |
| 现役救援 Ubuntu | p3，24.6 GiB | 同上 |
| 内存 | 15.7 GiB | 前期 |
| 固件 | UEFI 2.70，Qualcomm 8483.513；**chainload 实测可用** | [#73](stage4-findings.md) |

★ **ESP 只剩 28 MiB 是这份设计里最硬的约束**，它直接否决了"把救援 rootfs 也塞进 ESP"。

## 2. 结构决定

### 2.1 救援系统不再是一个分区

现在：24.6 GiB 的 ext4 装了一整套 Ubuntu。
改成：**内核 + initramfs + 一个 squashfs**。

* ESP 上放 `gaokun3/rescue/{vmlinuz.efi, initramfs.img, gaokun3.dtb}` ≈ 25–30 MiB
* squashfs 放**自己那个 1 GiB 分区**（partlabel `gaokun3rescue`），不放 ESP
* initramfs 的活：按 partlabel 找 squashfs → mount → 上面盖一层 tmpfs
  （overlay）→ `switch_root`。找不到就落回一个能 ssh 的 initramfs shell。

为什么 squashfs 不放 ESP：ESP 是 FAT、没有权限位、而且**别人机器上的 ESP
经常是 Windows 建的 100 MiB**。独立分区让"双系统安装"这条路不受对方 ESP 大小摆布。

⚠️ **U 盘上例外**：LiveCD 只有一个 FAT 分区，squashfs 就放那儿，
initramfs 先按 partlabel 找、找不到再按**文件名**扫所有可读分区。
两种介质走同一份 initramfs。

### 2.2 底座选 Alpine（aarch64）

* musl + apk，base 8 MiB 量级；Debian minbase 光 rootfs 就 120 MiB
* 我们要的东西 apk 里都有：`sgdisk` `parted` `e2fsprogs` `dosfstools`
  `f2fs-tools` `ntfs-3g-progs` `nvme-cli` `zstd` `openssh` `chrony`
* ⚠️ 在 x86 构建机上做 aarch64 rootfs 需要 `qemu-user-static` + binfmt
  （apk 的 post-install 脚本要在目标架构上跑）。构建机上装得了。
  另一条路是**在设备的救援 Ubuntu 上原生构建**，不需要 qemu —— 但那要占着机器。

### 2.3 内核：复用我们自己的

不要另找内核。用**本仓这一棵**（v7.2-rc2 + `patches/`），因为：
面板、触摸（gpio174 补丁）、EC、ath11k、NVMe 全靠它。
差别只在 config：救援内核**不需要** Android 那一套（SELinux/DM/binder…），
但**必须**有 `SQUASHFS`、`OVERLAY_FS`、`NTFS3_FS`（缩 Windows 分区要读）、
`USB_STORAGE`、`BLK_DEV_INITRD`、`DRM` + 面板 + `HID`。
⚠️ 现在的 Android config 里 `USB_STORAGE` 是什么状态未查 —— 装机要从 U 盘读文件，
这条必须先确认。

### 2.4 固件：不需要任何华为专有 blob

救援/安装只需要 ath11k（`qca/*`）与 GPU（`a660_*`）—— **两者都在
linux-firmware 里，可再分发**。ADSP/CDSP/SLPI 那三个 `.mbn` 是音频与传感器用的，
救援用不上。所以 **LiveCD 可以公开发布**，不触碰本仓"专有固件不入库"的红线。

## 3. GUI：直接画 KMS，不上合成器

选型（决定 = **A**）：

| 方案 | 体积 | 工作量 | 风险 |
|---|---|---|---|
| **A. DRM/KMS dumb buffer + cairo/pango + libinput** | +30 MiB | 中（要自己写按钮/列表/进度条） | 低：没有合成器可坏，GPU 栈我们最熟 |
| B. weston + GTK4 应用 | +300 MiB | 小 | 多一层 Wayland/seat/udev 要调；体积翻 3 倍 |
| C. TUI（dialog/fbterm） | +2 MiB | 小 | ❌ **不合要求**：用户要的是图形化，而平板可能根本没接键盘 |

A 的理由不是"更酷"，是**这台机器上合成器能出的问题比我们要画的界面还多**，
而界面本身只有 6 屏、全是按钮。

* 文字用 pango + 一个中文字体（`wqy-microhei` 约 5 MiB；Noto CJK 全量 20 MiB 太大）
* **必须同时支持键盘操作**（Tab/方向键/回车）：万一 himax 触摸没起来，
  安装器不能变砖 —— 键盘和触控板走 EC，是另一条独立通路
* 双语（中/英），默认跟随第一屏的选择

### 界面流程

1. 语言
2. 目标磁盘（列出型号/容量/现有分区与识别出的系统）
3. 安装方式：**清空整盘** / **保留现有系统，装在空闲空间**
4. 选项：装不装救援系统（默认**装**）；救援系统的 ssh 公钥/密码
5. **确认页**：把将要执行的分区操作逐条列出来（"删除 p3"、"把 p2 从 300G 缩到 200G"…）
   —— 这一屏是唯一能防止误删的东西，必须显示**将被销毁的数据**
6. 进度 → 完成/重启

## 4. 双系统安装

需要三样现在没有的东西：

1. **识别现有系统**：ESP 里的 `EFI/Microsoft` → Windows；
   ext4 里的 `/etc/os-release` → Linux 发行版名。
2. **腾空间**：优先用未分配空间；不够就缩分区。
   * NTFS → `ntfsresize`。⚠️ **必须先查 dirty bit**（`ntfsfix -n` / 引导扇区标志）：
     Windows 快速启动/休眠会留下脏卷，缩了就是数据损坏。脏就**拒绝**并告诉用户
     去 Windows 里关快速启动 + 完整关机。
   * ext4 → 先 `e2fsck -f` 再 `resize2fs`。
3. **共用 ESP**：不新建第二个 ESP（UEFI 一盘一个）。需要在对方 ESP 里腾出
   **≥ 90 MiB**（两个槽的 Image+ramdisk+dtb，不含 recovery-ramdisk）。
   ⚠️ 本机就是活样本：300 MiB 的 ESP 已经 91% 满。空间不够时安装器要**明说差多少**，
   而不是写一半失败。

**最小可用空间**（安装器要硬校验）：
super 12 GiB + boot_a/b 128 MiB + metadata 32 MiB + misc 4 MiB
+ userdata 至少 16 GiB + ESP 90 MiB ≈ **29 GiB**；救援再加 1 GiB。建议 ≥ 64 GiB。

## 5. 迁移路径（本机怎么从 Ubuntu 换过去）

**不要先删 Ubuntu。** 顺序：

1. 造出 `rescue` profile，写进 ESP + 新建 1 GiB `gaokun3rescue` 分区（用未分配的 64 GiB）
2. 加一个**并列的**启动项，Ubuntu 那个一个字不动
3. 从新救援系统里 ssh 进去，跑一遍真实活儿（`sgdisk -p`、挂 super、改 ESP）
4. 只有第 3 步过了，才删 p3，把 24.6 GiB 还给未分配
5. `docs/INSTALL.md` 与 `scripts/install-gaokun3.sh` 同步改

## 6. 待查（动手前必须先有答案）

* [x] **内核 config**（2026-08-23 查完，从设备的 `/proc/config.gz` 读的实值）：
  `USB_STORAGE=y` ✅、`BLK_DEV_LOOP=y` ✅、`VFAT_FS=y` ✅、`NVME_CORE=y` ✅、
  面板与触摸都 `=y` ✅；缺的三个已加进 `kernel-config-android.sh`：
  **`SQUASHFS` 原本是 `=m`**（第 14 个「=m 坑」，救援 initramfs 里没有模块）、
  `NTFS3_FS` 原本没有、`NLS_UTF8` 原本没有。
  ⚠️★ `NTFS3_FS` 的 Kconfig 是 `depends on !NTFS_FS || m` —— 旧的 `NTFS_FS`
  兼容壳开着就把它**钉死在 `=m`**，`--enable` 也改不动，得先 `--disable NTFS_FS`。
* [x] **`Persisted_Capsules.bin`（70 MiB）：不动它。** 它是 UEFI 的
  capsule-on-disk 暂存文件（固件更新用）。Windows 虽然抹了，但这台机器的
  BIOS 无法用常规手段恢复，**用 70 MiB 去赌固件更新通路不出事，赔率不对**。
  ESP 的空间从别处找（见下），不从这里找。
* [x] **`recovery-ramdisk.img` ×2（30 MiB）**：grep 过全部启动项，**无人引用**，
  可删。加上 `EFI/Boot/bootaa64.efi.bak-windows`（3.1 MiB）与
  已抹除 Windows 的 `EFI/Microsoft`，一共约 45 MiB。
  ★ 而真正的大头是**统一内核之后**省下的那份救援 Ubuntu 内核 + initrd（59 MiB）。
  两笔加起来 ≈ 104 MiB —— 够放 live/rescue 的内核与 initramfs 了。
* [ ] 构建机上 `qemu-user-static` + binfmt 能不能装（apk 的目标架构脚本要用）
* [ ] 现有 `scripts/install-gaokun3.sh` 只有"清空整盘"一条路，且**从未端到端跑过**
  （TODO B4）。图形安装器要复用它的分区/写盘逻辑，那就必须先让它可被库调用，
  而不是一个从头跑到尾的脚本。
