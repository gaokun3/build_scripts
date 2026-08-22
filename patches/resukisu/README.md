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

---

## 实际需要的两个补丁（主线 v7.2-rc2，2026-08-23 编译通过）

### 0001 —— `asm/patching.h` 改名成 `asm/text-patching.h`

`hook/patch_memory.h` 按 `LINUX_VERSION_CODE >= 5.14` 去 include
`asm/patching.h`，**只有下界没有上界**。而 arm64 那个头在 6.13 被并进跨架构的
`asm/text-patching.h`（`aarch64_insn_patch_text*` 一起搬走了），7.2 上
`asm/patching.h` 根本不存在。

修法用 `__has_include` 逐个探，而不是再加一段版本阶梯 —— 阶梯在下一次改名时
还会错一遍。

### 0002 —— 主线把 `strncpy()` 删了

7.2 的 `include/linux/string.h` 里 `strncpy` **只剩注释**（"replacement for
strncpy() uses …"），`lib/string.c` 里也没有定义，`fs/` 与 `kernel/` 全树零调用。
而 ReSukiSU 有 9 处在用它。

⚠️ 这个错误**只在链接期出现**（`undefined reference to 'strncpy'`）：
gcc 自带 `strncpy` 的 builtin 原型，所以编译阶段一声不吭。

修法是**按原样补一个 shim**，不是逐点换成 `strscpy`/`strscpy_pad`：
`dispatch.c` 拷的是要交给用户态的 uname 缓冲区，**依赖 strncpy 的补零**；
`throne_tracker.c` 则依赖"源串过长时不加终止符"。语义逐点分析容易出错，
而这不是我们的代码 —— 现代化那些调用点是上游的活。

⚠️★ **顺带踩到一个 make 的坑**：探测函数在不在用的是
`ifeq ($(shell grep -qE "…" …),0)`，而我第一版模式里写了 `\(`。
**make 在扫描 `$(shell …)` 时照样把 `\(` 当成一个左括号**，于是配平错位、
吞掉了收尾的 `)`，报的是
`kernel_compat.mk:309: *** invalid syntax in conditional`
—— 完全看不出跟正则有关。**`$(shell)` 里的模式不要出现不配对的括号。**

★ 探测方向也要挑对：写成"匹配到就用 shim"是错的方向。现在是"匹配到声明才
认为内核有"，**猜错的那一边只是多定义一个同语义的 static inline（无害）**，
而反过来会得到一个链接错误。
