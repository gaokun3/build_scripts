#!/usr/bin/env bash
# gk3_shrink 的端到端测试：真的建 NTFS / ext4，真的缩，然后验数据。
#
# ★ 这个测试的重点【不是】"缩完大小对不对"，而是三件会毁数据的事：
#     1. 缩完文件还在不在（逐文件 md5）
#     2. PARTUUID 有没有变（变了 Windows 就起不来）
#     3. 空间是不是真的释放出来了（不然缩了也白缩）
#
# 要 root（loop + mount）。在构建机上跑，不需要目标硬件。
set -u
cd "$(dirname "$0")/../.."
. scripts/live/installer-lib.sh

PASS=0; FAIL=0
ok()  { echo "  ✓ $*"; PASS=$((PASS+1)); }
bad() { echo "  ✗ $*"; FAIL=$((FAIL+1)); }

[ "$(id -u)" = 0 ] || { echo "要 root"; exit 2; }
for t in sgdisk mkfs.ntfs ntfsresize mkfs.ext4 resize2fs losetup blockdev; do
    command -v "$t" >/dev/null || { echo "缺 $t"; exit 2; }
done

IMG=/tmp/shrink-test.img
rm -f "$IMG"; truncate -s 20G "$IMG"
LOOP=$(losetup -fP --show "$IMG")
trap 'umount /tmp/sm 2>/dev/null; losetup -d "$LOOP" 2>/dev/null; rm -f "$IMG"' EXIT
echo "假盘 $LOOP"

sgdisk --zap-all "$LOOP" >/dev/null 2>&1
sgdisk -n 1:2048:+10G -t 1:0700 -c 1:Windows "$LOOP" >/dev/null 2>&1
sgdisk -n 2:0:+5G     -t 2:8300 -c 2:Data    "$LOOP" >/dev/null 2>&1
partprobe "$LOOP" 2>/dev/null; sleep 1

P1=${LOOP}p1; P2=${LOOP}p2
mkfs.ntfs -f -L WINTEST "$P1" >/dev/null 2>&1 || { echo "mkfs.ntfs 失败"; exit 1; }
mkfs.ext4 -q -F -L DATATEST "$P2" >/dev/null 2>&1 || { echo "mkfs.ext4 失败"; exit 1; }

# 写进去一些可校验的东西
mkdir -p /tmp/sm
seed_files() {
    local mnt=$1 i
    for i in 1 2 3 4 5; do
        head -c $((i * 7 * 1024 * 1024)) /dev/urandom > "$mnt/file$i.bin"
    done
    (cd "$mnt" && md5sum file*.bin > MD5SUMS)
    sync
}
mount "$P1" /tmp/sm && seed_files /tmp/sm && umount /tmp/sm
mount "$P2" /tmp/sm && seed_files /tmp/sm && umount /tmp/sm
echo "两个分区都写好了测试数据"

check_files() {
    local part=$1 name=$2
    mount "$part" /tmp/sm 2>/dev/null || { bad "$name 缩完挂不上了"; return 1; }
    if (cd /tmp/sm && md5sum -c MD5SUMS >/dev/null 2>&1); then
        ok "$name 缩完文件逐个 md5 一致（$(ls /tmp/sm/file*.bin | wc -l) 个）"
    else
        bad "$name 缩完文件校验不过 —— 数据坏了"
    fi
    umount /tmp/sm
}

uuid_of() { sgdisk -i "$1" "$LOOP" 2>/dev/null | grep '^Partition unique GUID:' | awk '{print $4}'; }

echo "═══ 1. 问最小能缩到多少 ═══"
gk3_shrink_info "$P1" | sed 's/^/  /'
gk3_shrink_info "$P2" | sed 's/^/  /'

echo "═══ 2. 缩 NTFS：10 GiB -> 6 GiB ═══"
U1=$(uuid_of 1)
if gk3_shrink "$P1" 6144 >/dev/null 2>&1; then
    NEW=$(( $(blockdev --getsize64 "$P1") / 1048576 ))
    [ "$NEW" -le 6200 ] && ok "分区变成 ${NEW} MiB" || bad "分区还是 ${NEW} MiB"
    [ "$(uuid_of 1)" = "$U1" ] && ok "PARTUUID 未变" || bad "PARTUUID 变了！"
    check_files "$P1" NTFS
else
    bad "缩 NTFS 失败"
fi

echo "═══ 3. 缩 ext4：5 GiB -> 3 GiB ═══"
U2=$(uuid_of 2)
if gk3_shrink "$P2" 3072 >/dev/null 2>&1; then
    NEW=$(( $(blockdev --getsize64 "$P2") / 1048576 ))
    [ "$NEW" -le 3100 ] && ok "分区变成 ${NEW} MiB" || bad "分区还是 ${NEW} MiB"
    [ "$(uuid_of 2)" = "$U2" ] && ok "PARTUUID 未变" || bad "PARTUUID 变了！"
    check_files "$P2" ext4
else
    bad "缩 ext4 失败"
fi

echo "═══ 4. 空间真的释放出来了吗 ═══"
FREE=$(gk3_probe 2>/dev/null | grep "^FREE disk=$LOOP" | awk '{for(i=1;i<=NF;i++){split($i,a,"=");if(a[1]=="size_mib")s+=a[2]}}END{print s+0}')
[ "${FREE:-0}" -ge 5000 ] && ok "空闲空间 ${FREE} MiB（期望 ≥5000）" || bad "只释放出 ${FREE:-0} MiB"

echo "═══ 5. 拒绝缩到太小 ═══"
gk3_shrink "$P2" 100 >/dev/null 2>&1 && bad "缩到 100 MiB 居然通过了" || ok "缩到 100 MiB 被拒"

echo
echo "═══ 通过 $PASS · 失败 $FAIL ═══"
[ "$FAIL" -eq 0 ]
