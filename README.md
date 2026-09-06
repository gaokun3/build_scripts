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
forks. Mesa build rules use declared inputs inside the Soong sandbox and
need no checkout-path rewriting. `prepare.sh` applies the small GApps
integration patch and removes dangling date/tar wrappers in minimal checkouts.
GApps follows its GitLab baklava branch because its history contains APKs larger than
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
The source fixes are committed in the corresponding forks. The kernel uses
its committed `gaokun3_android_defconfig`; legacy patch-application, configuration
mutation and device-side OTA verification scripts are not needed in this build
repository. Mesa path handling is maintained in the Mesa fork itself.

Kernel base: `gregkh/linux`, branch `linux-rolling-stable`, with gaokun device
support migrated from the local Linux 7.2 tree and the Android configuration.
Updating to a new rolling-stable commit requires patch review and a new build;
a successful compile alone does not validate suspend, GPU, audio or boot on hardware.
