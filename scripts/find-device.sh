#!/usr/bin/env bash
# 按 MAC 在局域网里找回设备（DHCP 会漂，本仓已经因此浪费过多次时间）。
#
#   bash scripts/find-device.sh            # 找 Android（adb 5555）
#   bash scripts/find-device.sh --ssh      # 找救援系统（sshd 22）
#
# ⚠️ 必须绕开沙箱跑（sandbox 代理会把【所有】TCP 连接都答应下来，
#    于是端口扫描每个 IP 都"开着"，完全没有信息量 —— 本仓 2026-08-23 踩过）。
set -u
WLAN_MAC=${GAOKUN3_MAC:-}
PORT=5555; [ "${1:-}" = "--ssh" ] && PORT=22

# 1. 让 ARP 表填起来：广播 ping 整个网段
SELF=$(ipconfig 2>/dev/null | grep -oE "192\.168\.[0-9]+\.[0-9]+" | head -1)
NET=$(echo "$SELF" | cut -d. -f1-3)
[ -n "$NET" ] || { echo "!! 认不出本机网段" >&2; exit 1; }
echo "网段 $NET.0/24（本机 $SELF）"
for i in $(seq 1 254); do (ping -n 1 -w 200 "$NET.$i" >/dev/null 2>&1 &) ; done
sleep 4

# 2. 从 ARP 里取邻居；有 MAC 就直接命中
NEIGH=$(arp -a 2>/dev/null | grep -oE "$NET\.[0-9]+ +[0-9a-f-]{17}" | tr -s ' ')
if [ -n "$WLAN_MAC" ]; then
    HIT=$(printf '%s\n' "$NEIGH" | grep -i "$(echo "$WLAN_MAC" | tr ':' '-')" | cut -d' ' -f1)
    if [ -n "$HIT" ]; then echo "按 MAC 命中：$HIT"; echo "$HIT"; exit 0; fi
fi

# 3. 没有 MAC 就按端口筛
echo "ARP 邻居 $(printf '%s\n' "$NEIGH" | wc -l) 个，逐个探 $PORT"
printf '%s\n' "$NEIGH" | cut -d' ' -f1 | while read -r ip; do
    [ "$ip" = "$SELF" ] && continue
    timeout 1 bash -c "echo > /dev/tcp/$ip/$PORT" 2>/dev/null && echo "  ✓ $ip:$PORT"
done
