# Gaokun3 build tooling

crDroid 16.0 / Huawei MateBook E Go. Sources and setup instructions live in
[`gaokun3/local_manifests`](https://github.com/gaokun3/local_manifests).

After syncing crDroid with the gaokun3 local manifests and supplying the device firmware:

```sh
bash tools/gaokun/build-kernel.sh "$PWD"
bash tools/gaokun/build-android.sh "$PWD"
```

`JOBS` defaults to 16. The kernel needs `aarch64-linux-gnu-gcc`, make, flex,
bison, libssl-dev and libelf-dev. It uses the committed
`gaokun3_android_defconfig`; no kernel patch script is required after sync.

Device-specific source changes are committed in the corresponding public
forks. Two preparation operations remain: relocate Mesa's generated
absolute build paths to the current checkout (the original generator bypasses
the Soong sandbox), and apply the small GApps integration patch. GApps follows its GitLab baklava branch because its history contains APKs larger than
GitHub's regular Git file limit. `prepare.sh` checks the patch before applying
it and is safe to rerun.

No separate device vendor repository is used. Firmware and sensor inputs
are supplied locally beneath the device tree; see its `firmware/README.md`
and `hexagonrpcd-root/README.md`. An Android vendor partition still exists.
Supply `device/huawei/gaokun3/adb_keys` from the development host's public
ADB key if using the original device configuration.

## Provenance

Imported from `vahiru/gaokun-android` at
`2f2b903` (2026-09-06 checkout). Original history and licensing are retained.
The working tree is reduced to build tooling; upstream deployment, flashing,
installer and forensic tools remain available in the original repository.
The archived `crdroid-tree-fixes.py` documents the source patches; do not run
it as a substitute for syncing the patched repositories.

Kernel base: `gregkh/linux`, branch `linux-rolling-stable`, with gaokun device
support migrated from the local Linux 7.2 tree and the Android configuration.
Updating to a new rolling-stable commit requires patch review and a new build;
a successful compile alone does not validate suspend, GPU, audio or boot on hardware.
