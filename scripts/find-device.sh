#!/usr/bin/env bash
# 按 MAC 在局域网里找回设备（DHCP 会漂，本仓已经因此浪费过多次时间）。
#
#   GAOKUN3_MAC=00:03:7f:12:4b:19 bash scripts/find-device.sh          # adb 5555
#   GAOKUN3_MAC=00:03:7f:12:4b:19 bash scripts/find-device.sh --ssh    # sshd 22
#
# ⚠️★ 这个脚本的第一版有一个会造成【完全错误的调查】的 bug：
#   MAC 在 ARP 里找不到时，它退化成"扫谁的端口开着就返回谁"，
#   于是把局域网里另一台开着 sshd 的机器当成了我们的设备 ——
#   我照着那台陌生机器的 sshd 行为查了两轮"公钥为什么被拒"。
#   ★ 现在的规矩：**MAC 对不上就什么都不返回**。
#     宁可报"没找到"，也不能返回一个看起来很像答案的错误答案。
#
# ⚠️ 必须绕开沙箱跑（sandbox 代理会把【所有】TCP 连接都答应下来，
#    于是端口扫描每个 IP 都"开着"，完全没有信息量）。
set -u
MAC=${GAOKUN3_MAC:-}
PORT=5555; [ "${1:-}" = "--ssh" ] && PORT=22

[ -n "$MAC" ] || { echo "!! 要 GAOKUN3_MAC=<设备 MAC>；没有 MAC 就无法确认找到的是不是它" >&2; exit 2; }
MACD=$(echo "$MAC" | tr ':' '-' | tr 'A-Z' 'a-z')

SELF=$(ipconfig 2>/dev/null | grep -oE "192\.168\.[0-9]+\.[0-9]+" | head -1)
NET=$(echo "$SELF" | cut -d. -f1-3)
[ -n "$NET" ] || { echo "!! 认不出本机网段" >&2; exit 1; }

# 广播 ping 把 ARP 表填起来（DHCP 刚换过 IP 的设备也会因此出现）
for i in $(seq 1 254); do (ping -n 1 -w 150 "$NET.$i" >/dev/null 2>&1 &); done
sleep 5

IP=$(arp -a 2>/dev/null | tr -s ' ' | grep -i " $MACD " | grep -oE "$NET\.[0-9]+" | head -1)
if [ -z "$IP" ]; then
    echo "!! ARP 里没有 $MAC —— 设备不在这个网段上（没开机 / 没连上 WiFi / 在别的网段）" >&2
    exit 1
fi

# 找到了 MAC 才谈端口。端口不通也照样把 IP 打出来 —— 那是有用的信息
# （"在网上但服务没起来" 和 "根本不在网上" 是两回事）。
if timeout 2 bash -c "echo > /dev/tcp/$IP/$PORT" 2>/dev/null; then
    echo "$IP"
else
    echo "!! $IP 是本机（MAC 对得上），但 $PORT 端口不通" >&2
    echo "$IP"
    exit 3
fi
