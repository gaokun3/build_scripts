#!/usr/bin/env bash
# ReSukiSU（root）上机验收。在【宿主机】跑，通过 adb 检查设备。
#
#   bash scripts/verify-root.sh
#
# ⚠️ 需要 adb root（本机要先 `adb shell setprop service.adb.root 1` 再 `adb root`，
#    而且每次重启都要重做）。
set -uo pipefail
PASS=0; FAIL=0
ok()   { echo "  [OK]   $*"; PASS=$((PASS+1)); }
bad()  { echo "  [FAIL] $*"; FAIL=$((FAIL+1)); }
info() { echo "         $*"; }

A() { adb shell "$@" 2>/dev/null; }

echo "═══ 0. 设备与内核 ═══"
UNAME=$(A 'uname -r -v')
info "$UNAME"
[ -n "$UNAME" ] || { echo "adb 不通，停"; exit 2; }

echo "═══ 1. 内核配置（从 /proc/config.gz 读实值，不看构建机上的 .config）═══"
CFG=$(adb exec-out 'zcat /proc/config.gz' 2>/dev/null)
if [ -z "$CFG" ]; then
    bad "读不到 /proc/config.gz（要 root；CONFIG_IKCONFIG_PROC 也得开）"
else
    for k in CONFIG_KSU=y CONFIG_KSU_TRACEPOINT_HOOK=y CONFIG_FTRACE_SYSCALLS=y CONFIG_KALLSYMS_ALL=y; do
        if printf '%s' "$CFG" | grep -qx "$k"; then ok "$k"; else
            bad "$k —— 实际：$(printf '%s' "$CFG" | grep "^${k%%=*}=" || echo '未设置')"
        fi
    done
fi

echo "═══ 2. 驱动初始化（dmesg）═══"
# 字符串出处：kernel/include/klog.h 把 pr_fmt 设成 "KernelSU: "；
#            kernel/core/init.c:177 是内建模式那一行。
INIT=$(A 'dmesg' | grep -m1 'KernelSU: Initialized with driver version')
if [ -n "$INIT" ]; then
    ok "驱动已初始化"
    info "$(printf '%s' "$INIT" | sed 's/.*KernelSU: //')"
    printf '%s' "$INIT" | grep -q 'Work mode: Built-in' \
        && ok "Work mode = Built-in（不是 LKM 后加载）" \
        || bad "Work mode 不是 Built-in —— 后加载模式会尝试把 SELinux 切成 enforcing"
else
    bad "dmesg 里没有 KernelSU 初始化行"
fi

echo "═══ 3. tracepoint 钩子真的注册上了 ═══"
# 出处：kernel/hook/syscall_hook_manager.c:149
if A 'dmesg' | grep -q 'sys_enter tracepoint registered'; then
    ok "sys_enter tracepoint 已注册"
else
    bad "没看到 'sys_enter tracepoint registered' —— 钩子没挂上，root 不会工作"
    A 'dmesg' | grep -i 'hook_manager' | head -5 | sed 's/^/         /'
fi

echo "═══ 4. 回归：SELinux 还是 permissive ═══"
# ⚠️ init.c:268 在【后加载】分支里会 setenforce(true)。我们是内建，不该走到那儿，
#    但这条值得每次都验 —— 本机没写 sepolicy，被切成 enforcing 会大面积失效。
SE=$(A 'getenforce')
[ "$SE" = "Permissive" ] && ok "getenforce = Permissive" || bad "getenforce = $SE（被改了？）"

echo "═══ 5. 管理器 App ═══"
if A 'pm list packages' | grep -q 'com.resukisu.resukisu'; then
    ok "管理器已安装（com.resukisu.resukisu）"
else
    info "[跳过] 管理器没装。装法："
    info "  下载 ReSukiSU_<ver>-arm64-v8a-release.apk 后 adb install"
    info "  （ksud 就在 APK 的 lib/arm64-v8a/libksud.so 里，装 App 即到位）"
fi

echo "═══ 6. ksud 是否就位 ═══"
if A 'ls /data/adb/ksud' | grep -q ksud; then
    ok "/data/adb/ksud 存在"
else
    info "[跳过] /data/adb/ksud 还没有 —— 装了管理器并打开一次才会生成"
fi

echo
echo "═══ 小结：通过 $PASS · 失败 $FAIL ═══"
[ "$FAIL" -eq 0 ]
