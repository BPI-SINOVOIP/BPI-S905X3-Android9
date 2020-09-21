# Android Kernel Config Fragments

The files in these directories are meant to be used as a base for an Android
kernel config. All devices must have the options in `android-base.cfg` configured
as specified.  If an `android-base-ARCH.cfg` file exists for the architecture of
your device, the options in that file must be configured as specified also.

While not mandatory, the options in `android-recommended.cfg` enable advanced
Android features.

Assuming you already have a minimalist defconfig for your device, a possible
way to enable these options would be to use the `merge_config.sh` script in the
kernel tree. From the root of the kernel tree:

```sh
     ARCH=<arch> scripts/kconfig/merge_config.sh <...>/<device>_defconfig <...>/android-base.cfg <...>/android-base-<arch>.cfg <...>/android-recommended.cfg
```

This will generate a `.config` that can then be used to save a new defconfig or
compile a new kernel with Android features enabled.

Because there is no tool to consistently generate these config fragments,
lets keep them alphabetically sorted instead of random.
