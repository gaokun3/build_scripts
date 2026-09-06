#!/usr/bin/env bash
set -euo pipefail
SELF=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
TREE=$(realpath "${1:-$SELF/../..}")
test -f "$TREE/build/envsetup.sh"
if [[ -d "$TREE/vendor/gapps" ]]; then
    if git -C "$TREE/vendor/gapps" apply --reverse --check "$SELF/patches/vendor_gapps.patch" 2>/dev/null; then
        echo 'GApps integration patch is already applied.'
    else
        git -C "$TREE/vendor/gapps" apply --check "$SELF/patches/vendor_gapps.patch"
        git -C "$TREE/vendor/gapps" apply "$SELF/patches/vendor_gapps.patch"
    fi
fi
python3 "$SELF/scripts/mesa-relocate-abs-paths.py" "$TREE"
# Minimal checkouts may omit the date/tar prebuilts. Remove only dangling
# wrappers, so the build falls back to its supported host tools.
for arch in linux-x86 linux-arm64 darwin-x86; do
    for tool in date tar; do
        link="$TREE/prebuilts/build-tools/path/$arch/$tool"
        if [[ -L "$link" && ! -e "$link" ]]; then rm -- "$link"; fi
    done
done
echo 'Source preparation complete.'
