# patches/resukisu/

对 **ReSukiSU**（上游 GPL-3.0，`scripts/kernel-setup-resukisu.sh` 里钉了 commit）
的本地修复。由那个脚本以 `git apply` 打进 `<内核树>/KernelSU/`，
所以补丁里的路径**相对 ReSukiSU 仓库根**（`kernel/...`），不是内核树根。

为什么单独一个目录：ReSukiSU 有一万多行、不是我们的代码，不入库；
但**我们改了什么必须一眼可见**。

## 目前是空的 —— 这本身是个结论

⚠️★ 一开始我选错了上游（把 "resukisu" 读成了 SukiSU-Ultra）。那个错误反而
留下一组有用的对照数据：

| | SukiSU-Ultra | ReSukiSU |
|---|---|---|
| 主线 v7.2-rc2 首次编译 | **4 个错误**，全在 `selinux/sepolicy.c` | 见下 |
| 原因 | 直接引用 Android common kernel 私有的 `policydb.android_netlink_route` 等 | 同一处代码用 `KSU_COMPAT_HAS_*` 宏包着 |
| 对新内核的适配机制 | 手写 `LINUX_VERSION_CODE` 守卫（最新只到 6.18） | `tools/kernel_compat.mk` —— **grep 内核源码**探测每个 API 在不在 |

★ **值得记的一条**：那 4 个错误看起来像"7.2 太新"，其实一个都不是版本问题，
全是"这棵树不是 ACK"。**"新内核编不过"和"非 Android 内核编不过"是两回事**，
把前者当默认解释会一路查错方向。
