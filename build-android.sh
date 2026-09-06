#!/usr/bin/env bash
set -eo pipefail
SELF=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
TREE=$(realpath "${1:-$SELF/../..}")
bash "$SELF/prepare.sh" "$TREE"
test -s "$TREE/device/huawei/gaokun3/prebuilt-boot/vmlinuz.efi"
test -s "$TREE/device/huawei/gaokun3/prebuilt-boot/dtb/gaokun3.dtb"
cd "$TREE"
# crDroid envsetup uses intentionally short-reading pipelines (for example
# /dev/urandom | head). Source it with the interactive shell's error policy.
set +e
set +o pipefail
source build/envsetup.sh
lunch lineage_gaokun3-bp4a-userdebug || exit $?
set -e -o pipefail
m -j"${JOBS:-16}"
m -j"${JOBS:-16}" superimage
