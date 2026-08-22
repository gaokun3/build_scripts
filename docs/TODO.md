# 待办清单

最后更新：2026-08-23（v0.4.0-alpha 发布之后）

这份清单的排序原则是**用户能不能感觉到**，而不是有趣程度。每条都尽量写出
**具体的第一步** —— 没有第一步的条目只是愿望，不是待办。

状态表与公开招募项在 [`../README.md`](../README.md)；每条的证据在
[`stage4-findings.md`](stage4-findings.md) 等案卷里。

---

## A. 用户能感觉到的缺口

### A0. ⚠️ 侧滑返回手势失效（用户报告，**假说未证实**）
用户在装了 v0.4.0-alpha 之前的那一版（桌面模式开着）时报告边缘侧滑返回不工作。

**已知**：`navigation_mode = 2`（手势导航）本身是对的；当时 `dumpsys window`
显示所有应用都在 **freeform 窗口**里（`mWindowingMode=freeform`、`Task name=Desk`），
而桌面窗口模式下边缘返回的处理方式本来就不同。

⚠️ **这只是嫌疑，没有证实** —— 当时没有运行时开关能把桌面模式关掉再对照
（那个开发者选项开关只能强制打开，见 A′/#桌面模式）。v0.4.0-alpha 已把桌面
模式关闭，**所以第一步就是让用户在新版上再试一次**：
* 好了 ⇒ 就是桌面模式，本条关闭；
* 还是不行 ⇒ 与桌面模式无关，查 `back_gesture_inset_scale_left/right`
  （现在是 null）、`dumpsys window | grep -i gesture` 的排除区域，
  以及触摸驱动在屏幕边缘的上报（#26 那套 evdev 录制方法可复用）。

### A1. 音频与蓝牙长期运行后死锁 ⚠️ 次高优先
用户实机报告，我未复现、未定位（[#38](stage4-findings.md)）。
两者共用同一条到 DSP 的 QRTR/FastRPC 通路，而这条通路上**已经实测到过**
会话级卡死（使能光感会污染整个 SSC 会话）。

★ **取证看门狗已随 v0.2.0 起的镜像发布**（`bin/gaokun3-hangdump.sh`）：
现实是死锁时用户只会重启、证据就没了，所以证据必须自动留下。它 60 秒采一次
`/proc` 线程状态（刻意不跑 dumpsys，很便宜），判据是**同一个 tid 连续三次都在 D**
（≥2 分钟），命中后把 stack/wchan/QRTR 服务表/PCM 状态/binder 日志/logcat
写到 `/data/vendor/gaokun3/hangdump-<uptime>/`。

**第一步**：下次死锁后把那个目录整个要过来 —— 不必再追问"多久、什么负载"。
若目录是空的，说明它没判定成死锁（比如卡的不是 D 状态），那本身就是线索。
手工对照仍可用 `gaokun3-qrtr-lookup` 比服务表：少了哪个服务就指向哪个 DSP。
⚠️ 别把 `Handover signaled` 当崩溃证据，那是良性噪声（#37 已用对照实验证明）。

### A2. 硬件视频【编码】（Venus）— 解码已通，编码未验证
解码 ✅ 已随 v0.4.0-alpha 发布（`c2.v4l2.avc.decoder` 实测解出 30 帧，
案卷 [#41](stage4-findings.md)）。编码这一侧**从没成功过**：`screenrecord`
仍然失败，没有任何 `c2.v4l2.*.encoder` 被证明产出过一帧。

**第一步**：先读 `scripts/crdroid-tree-fixes.py` 里那两条解码修复 ——
它们都是"v4l2_codec2 照搬 ChromeOS 行为、而 venus 守规范"这一类，
编码器多半有它自己的同型问题。然后用 `screenrecord` 复现并抓
`logcat | grep V4L2Encoder`，看它卡在哪个 ioctl。

⚠️ 别只看"组件注册了"就以为能用 —— `c2.v4l2.avc.encoder` 一直在
MediaCodecList 里，而解码器也一直在，两者都不能工作。

### A3. 自动亮度（环境光）— 芯片在总线上不应答，四个软件维度已扫空
详见 [#72](stage4-findings.md)（并已作废 #43/#68/#70 的历次归因）。

**当前定性**：光感驱动**在 SLPI 固件里**（`strings` 见 tcs3701 120 次），
`default_sensors.json` **声明了** `.ambient_light`，SEE 也确实在 probe ——
**但芯片不应答**。阴性对照给出了判据：把能用的加速度计地址改错，
得到的失败签名与光感**一模一样**（都是「SSC 说没有传感器提供 data_type=...」）。

**已扫空的维度**（每格实测，不是推理）：
`bus_instance` **0–7 全扫** ／ `bus_type` 0–3 ／ `rail_on_state` 1 与 2 ／
Linux 没占那条总线（只有 4 条 i2c 适配器、无 0x39/0x46）／
Linux 没占中断脚（TLMM 32 与 127 都 `UNCLAIMED`）。
另：`is_dri = 0` ⇒ 光感**本来就走轮询**，「DRI 中断没到」这条假说**不成立**。

**新掌握的结构**：配置是**二供料**方案 —— IMU（sh3001 ✅／t1000）在 bus 1，
光感（tcs3701／sy3133cs）都在 bus 5。t1000 与 sh3001 同参数而只有后者注册出来，
⇒ SEE 的「探测择优」机制本身是正常的。

★ **一条一直没人注意的事实**：喂给 DSP 的 `hw_platform = QRD` 是**我们自己编的**
（主线不导出它）。所以这整套是**高通参考设计的配置**，板级差异原本在
Windows DriverData 注册表里，**已随抹除 Windows 丢失**。

**剩下的可能性**（按可行性）：
① 芯片没上电／被复位钉住（与触摸屏 gpio174 同一类；那些脚在 SSC 域，AP 看不到）；
② 真实总线/地址只存在于已丢失的注册表里；
③ 抄一台光感在主线上能用的 sc8280xp（ThinkPad X13s）。

★ **可复用工具已入库**：`scripts/ssc/`（60–90 秒一次的实验循环 +
带回读验证的改字段工具 + 必跑的阳性/阴性对照）。**下次动这块先读它的 README。**

⇒ **自动亮度暂时做不了。自动旋转与游戏体感不受影响。**

### A5. 恢复出厂设置不起作用
设置里那条路走 misc 的 BCB + recovery，而本机没有可用 recovery
（[#39](stage4-findings.md)）。实机证据：misc 里躺着一条没人消费的 `boot-recovery`。

**现在的替代**：从救援 Linux `mkfs.ext4 -F /dev/disk/by-partlabel/userdata`。
**真正的修法**：见 B3（EFI 加载器）或让 recovery 能启动（已搁置）。

### A6. USB-C 外接显示（UCSI）★ 现在还欠着待机那笔账
`PPM init failed -ETIMEDOUT`，本机主线已知缺陷，`/sys/class/typec/` 是空的。
代价还包括 USB 只有 high-speed（SuperSpeed 需要 UCSI 切 orientation）。

★ **它现在是三个问题的共同根因**：没有 role 源正是 `a600000.usb` 停在
半初始化 `device` 态的原因（[#52](stage4-findings.md)），而我们为此付的代价
就是**息屏时 USB adb 断开**。UCSI 修好 → role 被正确指派 → 那个取舍自动消失。
⚠️ 顺带说明这条为什么值得排在摄像头前面：**USB adb 掉线是本项目迄今最大的
效率税**（#27），每次都要靠扫网段 + TCP adb 找回来。

### A8. 设备的 WAN 吞吐只有 PC 的 1/20（原因未定）
详见 [#44](stage4-findings.md)。⚠️ **不是 WiFi、不是 ath11k** ——
那条归因我下错过并已推翻：ping 网关 **0% 丢包**（1400 字节大包也是），
从本机拉 200 MB 跑到 **61.7 MB/s**，全程 `msdu_done` 新增 **0**。

真正剩下的问题：设备从 R2 拉只有 **1–2 MB/s**，而同一网络里的 PC 拉同一个
URL 是 **36.9 MB/s**。对"用户走系统内 OTA 升级"有实际影响（1 GB 要二十多分钟）。

**第一步**：在设备上对同一个 URL 抓一次 `ss -ti` 看拥塞窗口与重传，
再换一个不同 CDN 的大文件对照 —— 先分清是"到 Cloudflare 这条路"还是
"设备的 TCP 行为"。

### A7. 摄像头
完全没碰。

---

## A′. 已关闭（只留索引，细节在案卷里）

* **s2idle 待机** ✅ v0.3.0-alpha。真凶是我们自己加的 `dr_mode="otg"`。
  [#52](stage4-findings.md)–[#57](stage4-findings.md)
* **耳机口 + 内置麦克风** ✅ 用户确认出声。[#40](stage4-findings.md)
* **普通应用能 panic 内核** ✅ v0.4.0-alpha，`patches/0013` 删掉 `drm_crtc`
  里那行竞态 `BUG_ON`。[#58](stage4-findings.md) / [#62](stage4-findings.md)
* **硬件视频解码** ✅ v0.4.0-alpha。[#41](stage4-findings.md)
* **亮度调节** ✅ 真 lights HAL 取代了只接受数值不干活的 stub。
* **扬声器音量偏小** ✅ WSA 数字音量上限 81→90，实测 +5.7 dB。
* **插着键盘时屏幕键盘不弹** ✅ `show_ime_with_hard_keyboard` 默认置 1。
  ⚠️ 键盘开关**不是**这条的解药（Android 看键盘设备存不存在，不看 inhibited）。

---

## B. 工程债与正确性

### B1. SELinux 转 enforcing
现在是 `permissive`。影响 Play Integrity 与部分带反作弊的游戏。

★ **已做过一次普查**（[#60](stage4-findings.md)）：988 行 → **237 种**去重元组。
而普查**推翻了这一条原先的第一步**（"把现有 denial 收集成 `.te`"）——
我们的服务**根本没有域**：init 起的 root 进程没有 `file_contexts` 条目就留在
`u:r:init:s0` 里，所以 `hexagonrpcd`、sensors HAL 的 denial 全挂在 `init` 名下。
照着这样的清单写 `.te`，主体从一开始就是错的。

✅ **已就地清掉 363 条（约全系统 37%）**：`bpf-relabel.sh` 在带 `patches/0007`
的内核上完全多余，改成"标签已对就 `exit 0`"。⚠️ 顺带记住：**permissive 下的
denial 不是无害的** —— [#59](stage4-findings.md) 证明日志洪水会把 panic 栈从
pstore 里挤掉。

**第一步（改过的）**：给 `hexagonrpcd` / sensors HAL / `gaokun3-usbrole` /
`audioroute` / `smmustall` / `hangdump` / `bpfrelabel` 各定一个域 +
`file_contexts` 条目。然后是那批 `device : chr_file` —— 那是**设备节点没打类型**
（`device` 是兜底标签），给节点定类型就一起消失，不必逐条写 allow。

### B2. 真温控 HAL
现在是 AOSP mock（温度恒定 30.1/30.2），框架完全没有真实温控感知。
⚠️★ **换成读 `/sys/class/thermal` 的真 HAL 时必须同时改阈值** ——
mock 报的 skin/battery SHUTDOWN 阈值只有 **36 °C**，而
`ThermalManagerService.shutdownIfNeeded()` 到 SHUTDOWN 会直接
`powerManager.shutdown()`。现在因为 mock 值恒定打不到，**换真 HAL 会开机
几分钟就自动关机**。

### B3. 自研 EFI 加载器（规范化的最后一段）
读 `misc` 的 `bootloader_control` 选槽 + 解析 Android boot 镜像 +
装 initrd/DTB 协议。做完之后：
* postinstall 钩子与 ESP 上的派生文件**全部可以退役**
* BCB 能被消费 → `adb reboot recovery` 与恢复出厂设置才有可能工作
* 是 AVB/verified boot 的前提

**安全阀已实测可用**（[#73](stage4-findings.md)，2026-08-23）：systemd-boot 的
`efi` 指令在这台机器上确实能 LoadImage + StartImage 另一个 EFI 应用，
三轮实验全部自动回到 Android，**没有人碰过机器**。所以"固件不支持 chainload"
这个顾虑不成立，开发这个部件不需要有人守在机器旁。
⚠️ 但 `efi` 条目**拿不到 initrd**（`boot.c:2428` 直接按类型返回），
`devicetree` 倒是照装 —— 拿它引内核必须自带 `panic=10`。

⚠️ **真正的物理约束是 ESP 只剩 28 MB**（296M 用了 268M）。
里面有 70 MB 的 `Persisted_Capsules.bin` 和 31 MB 的 `EFI/`
（含已抹除的 Windows 整棵树）。要往 ESP 加东西，先腾地方。

### B4. LiveCD 图形安装器（★ 已有设计：[stage7-live-installer.md](stage7-live-installer.md)）
用户 2026-08-23 定的范围：图形化安装流程 + **支持机器上已有别的系统时装 Android**
+ 让用户选装不装救援系统。

`scripts/install-gaokun3.sh` **从未端到端跑过**，而且只有"清空整盘"一条路。
要做双系统安装，它得先从"一条道跑到黑的脚本"改成**可被调用的库**
（探测现有系统 / 算空间 / 缩 NTFS 或 ext4 / 复用对方的 ESP）。

GUI 选型已定：**直接画 KMS + cairo/pango + libinput，不上合成器**
（理由和被否掉的两个方案见设计文档）。
⚠️ 必须同时能用键盘操作 —— 万一 himax 触摸没起来，安装器不能变砖。

**第一步**：在一台可牺牲的机器（或本机，数据已备份）上真跑一次现有脚本。
在那之前，"别人能装"这件事是未经验证的。

### B6. GPU SMMU 中断根治
实际 DT 是全局 672/673、context bank 从 678 起；而硬件拉的是 675/680，
其中 680 被分给 CB2、675 整张表里根本没有。很像 CB 起始偏移就错了。
⚠️ 但只凭"675/680 挂起"推不出正确映射，而且**改错了没有任何征兆**
（只是继续收不到 fault）。做成之后可以丢掉常驻的 `smmu-nostall.sh` 轮询。

### B7. 用轻量系统替掉救援 Ubuntu（★ 与 B4 是同一件事）
**M0 已完成**：构建链跑通，产物 squashfs **55 MiB** + initramfs **648 KiB**
（`scripts/live/`）。⬜ **还没在硬件上启动过** —— 下一步是从救援 Ubuntu 里
用未分配的 64 GiB 建一个 1 GiB 分区、并列加一个启动项，ssh 验过再谈删 p3。

现在 24.6 GiB 一整套 Ubuntu。设计见 [stage7-live-installer.md](stage7-live-installer.md)：
**救援系统不再是一个分区** —— 内核 + initramfs + 一个 ≤120 MiB 的 squashfs，
和 LiveCD 用同一套镜像（两个 profile）。

★ 顺带把 ESP 上那份**独立的救援内核 + initrd（59 MiB）** 也省掉：
救援与 Android **共用同一个内核**，只是换 initramfs 和 cmdline。
为此已经在 `kernel-config-android.sh` 里补了 `SQUASHFS=y`（原本是 `=m`）、
`NTFS3_FS=y`、`NLS_UTF8=y`。

⚠️ 迁移顺序：**先并列装上、ssh 验过真活儿，才删 p3。**
别把唯一的救援通路换成没验过的东西。

### B8. `invalid volume index range in the curve` ×12（既有，非回归）
每次 audioserver 启动都吐 12 条 `E APM_AudioPolicyManager: invalid volume index
range in the curve:`（后面是空的，连哪条曲线都没说）。
**确认与耳机改动无关**：干净 A/B，旧策略 12 条、新策略 12 条。
来源应在 `audio_policy_volumes.xml` / `default_volume_tables.xml`（两份都是从
`frameworks/av/services/audiopolicy/config/` 原样拷的）与本机 `devicePorts`
的交集上。目前没有可观测的功能损害，故只记不修。

**第一步**：给那条日志找出打印点（`EngineBase`/`VolumeCurve`），看它校验的是
哪个字段，再对照我们装进去的两份 XML。

---

### B9. SLPI 每 200 ms 一条 handover 噪声（根因未查）
详见 [#59](stage4-findings.md)。**它无害但有代价** —— 正是它把 [#58](stage4-findings.md)
那两次 panic 的调用栈从 pstore 里挤掉了（45 条记录里有用的不到 10 条）。
`patches/0014` 已把打印改成 ratelimited（治症状，正确且值得上游），
但**远端为什么每 200 ms 翻一次 smp2p 位仍然不知道**。

硬证据：SLPI 的 `q6v5 ready` 与 `q6v5 handover` 两条中断计数**完全相同、
同步增长**（5 秒 5475→5502），ADSP/CDSP 各只有 2。

**第一步**：5 Hz 很像一个采样节拍 —— 停掉 sensors HAL / `hexagonrpcd`
看频率变不变。⚠️ **别在没人看着时做**：M12 记过停/重启 HAL 会污染 SSC 会话，
自动旋转当场失效、要重启 `hexagonrpcd` 并等约 20 秒才恢复。

### B11. 把 root（ReSukiSU）装进 ROM
内核这一半已经跑通并实测（[#74](stage4-findings.md)，`scripts/verify-root.sh` 8/8）：
`CONFIG_KSU=y` + tracepoint 钩子 + 两个补丁，管理器拿到 root、`/data/adb/ksud` 自动就位。

**但现在只活在 ESP 的一个实验条目里**（`ksu-full.conf`，oneshot），
下次重启就回到不带 root 的 `android-b`。要常驻需要：
* 用带 KSU 的内核重建 boot.img + OTA（构建流程不用改，`kernel-setup-resukisu.sh`
  已经能把驱动接进内核树）
* 决定要不要预装管理器 APK。★ **ROM 侧其实什么都不用加** ——
  `ksud` 就在 APK 的 `lib/arm64-v8a/libksud.so` 里，装 App 即到位。
* ⚠️ 决定要不要在 cmdline 里给 `kernelsu.allow_shell=1`。**我的建议是不给**：
  那等于任何能连 adb 的人直接拿 root，没有任何确认。

⚠️ 顺带记一条产品层面的取舍：**root 会影响 Play Integrity 和部分带反作弊的游戏**，
而本项目的目标之一正是跑手游。这是用户的选择，不是技术障碍，但值得写在发版说明里。

### B10. `release.sh` 应该自己清理 staging 残留
⚠️ **这是我造成的流程缺陷，不是意外**：历次调试往 R2 传了 `staging/m14c`、
`m17`…`m21` 各一个约 1 GB 的 payload，**每次都没清**，加上两份完整的
`staging/<ver>/`，一共堆了 **9 GiB**（桶总量一度 15.6 GiB，而免费额度是 10 GB）。
2026-08-22 已手工清空，但下次照样会堆。

**第一步**：在 `release.sh` 上传成功之后删掉同版本的 `staging/<ver>/`；
`scripts/r2-upload.py` 现在有 `--list` / `--du` / `--delete`，够用了。
⚠️ 删之前必须确认没人引用 —— 这次是逐个核对了 GitHub 发布页正文里的链接
和 `docs/` 里的 R2 路径才敢删的。

---

## C. 上游或硬件层面（本地做不了）

* **磁力计** —— 本机**没有这个硬件**（SSC 亲口回答），所以没有指南针、
  没有 9 轴融合。不是缺驱动。
* **指纹（FocalTech FTE7001）、TPM** —— 没有任何驱动存在。
* **出厂传感器校准** —— 存在本机 Windows 的 DriverData 里、不在任何驱动包中，
  而 Windows 已抹除 → **永久丢失**。实测无害（单位矩阵恰好与面板方向一致），
  只影响 bias 精度。⚠️ 给还留着 Windows 的人：先把那个 registry 目录拷出来。

---

## D. 运维与安全（需要你动手）

1. ⚠️★ **轮换 R2 的 S3 密钥** —— 它们在聊天记录里出现过多次。
2. ⚠️ **把 Azure NSG 的 22 端口锁到你的出口 IP** —— 构建机是静态公网 IP，
   而它曾进过 git 历史（已 filter-branch 抹掉并强推，但 GitHub 仍保留旧对象）。
3. 构建机用完立刻 `az vm deallocate` 并**取真实退出码**（`| tail` 会吞掉失败）。
   ★ 大文件传输**走 R2 中转**，不要让按分钟计费的构建机干等：
   本轮直连 1 MB/s（2.7 GB 要 45 分钟）vs 上传 R2 43 MB/s（27 秒）。

### D5. PR #3 待回复（已审完，等你定措辞）
线上那个 PR 动的正是内核预编译这一块。我把要问的整理好了，**没有发到 GitHub**
—— 对外发言等你。三个问题：`dr_mode=host` 是不是有意为之（那正是我们 #52 的
取舍另一半）；他们刷完之后 USB adb 还通不通；能不能公开那份内核 `.config`
（我们这边的断言是 52 条 MUST_Y + `VIDEO_QCOM_IRIS` MUST_N，可以对一遍）。

### D6. 把 `drm_crtc` 那个 `BUG_ON` 报到 dri-devel（对外动作，等你定）
`patches/0013` 修的是**上游 mainline master 现存**的缺陷：普通应用查一次
present fence 的名字就能把整机 panic 掉。稿子照那个补丁的 commit message
改一改就能发。我不代发对外邮件。

### D7. v0.2.0-alpha 的 R2 产物要不要删（2.1 GiB，等你定）
`install/v0.2.0-alpha/` + `builds/…20260820….zip`。**它们正被 v0.2.0-alpha 的
GitHub 发布页链接着**，删了那个页面的下载链接会 404。桶现在 6.6 GiB，
免费额度 10 GB，不删也还撑得住。

---

## E. 明确搁置（记录理由，不是忘了）

* **recovery** —— 启动即复位循环（[#39](stage4-findings.md)）。
  现阶段意义不大：sideload 被系统内 OTA 覆盖，而调试它需要人反复到机器旁
  （本机没有串口、recovery 没有网络栈、pstore 对这类失败无效）。
  真要做，**第一步是把 USB adb 在 recovery 里弄通**，那是唯一能看见内部的通道。
* **fastboot** —— bootloader 级**不可能**（固件是 UEFI，不是 fastboot 设备）。
  用户态的 `fastbootd` 住在 recovery 的 ramdisk 里，所以随 recovery 一起搁置。
* **GMS / Play 商店** —— 用户未提出需求。
* **突破原神 1080×1728 的渲染上限** —— 那是游戏按**设备白名单**给的档位，
  不是本机的技术限制。要突破只能伪装机型，**有账号风险**，留给用户决定。
