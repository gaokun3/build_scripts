#!/usr/bin/env bash
set -euo pipefail
SELF=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
TREE=$(realpath "${1:-$SELF/../..}")
KERNEL="$TREE/kernel/huawei/gaokun3"
KERNEL_OUT="${KERNEL_OUT:-$TREE/out/kernel-gaokun3}"
export CROSS_COMPILE="${CROSS_COMPILE:-aarch64-linux-gnu-}"
mkdir -p "$KERNEL_OUT"
make -C "$KERNEL" O="$KERNEL_OUT" ARCH=arm64 gaokun3_android_defconfig
make -C "$KERNEL" O="$KERNEL_OUT" ARCH=arm64 -j"${JOBS:-16}" vmlinuz.efi dtbs
DEST="$TREE/device/huawei/gaokun3/prebuilt-boot"
mkdir -p "$DEST/dtb"
if find "$DEST/dtb" -maxdepth 1 -name '*.dtb' ! -name gaokun3.dtb | read -r _; then
    echo 'Unexpected extra DTB in prebuilt-boot/dtb; move it out before building.' >&2
    exit 1
fi
install -m644 "$KERNEL_OUT/arch/arm64/boot/vmlinuz.efi" "$DEST/vmlinuz.efi"
install -m644 "$KERNEL_OUT/arch/arm64/boot/dts/qcom/sc8280xp-huawei-gaokun3.dtb" "$DEST/dtb/gaokun3.dtb"
