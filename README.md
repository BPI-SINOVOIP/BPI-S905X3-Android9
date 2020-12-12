# BPI-S905X3-Android9

----------

**Prepare**

Get the docker image from [Sinovoip Docker Hub](https://hub.docker.com/r/sinovoip/bpi-build-android-7/) , Build the android source with this docker environment.

Other environment requirements, please refer to google Android's official build environment [requirements](https://source.android.com/setup/build/requirements) and [set up guide](https://source.android.com/setup/build/initializing) 

Download the [toolchains](https://download.banana-pi.dev/d/3ebbfa04265d4dddb81b/?p=/Tools/toolchains/bpi-m5&mode=list) and extract each tarball to /opt/ and set the PATH before build

    $ export PATH=$PATH:/opt/gcc-linaro-aarch64-none-elf-4.9-2014.09_linux/bin
    $ export PATH=$PATH:/opt/gcc-linaro-arm-none-eabi-4.8-2014.04_linux/bin
    $ export PATH=$PATH:/opt/gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu/bin
    $ export PATH=$PATH:/opt/gcc-linaro-6.3.1-2017.02-x86_64_arm-linux-gnueabihf/bin

----------

Get source code

    $ git clone https://github.com/BPI-SINOVOIP/BPI-S905X3-Android9

Because github limit 100MB size for single file, please download the [oversize files](https://download.banana-pi.dev/d/3ebbfa04265d4dddb81b/files/?p=/Source_Code/m5/android_github_oversize_files.zip) and merge them to correct directory before build.

----------

**Build**

    $ source env.sh
    $ ./device/amlogic/common/quick_compile.sh
    
    [NUM]   [        PROJECT]  [       SOC TYPE]  [  HARDWARE TYPE]
    ---------------------------------------------------------------
    ...
    [ 28]   [          AC214]  [         S905X3]  [          AC214]
    [ 29]   [    bananapi_m5]  [         S905X3]  [    BANANAPI_M5]
    ---------------------------------------------------------------
    please select platform type (default 1(Ampere)):29
    Select compile Android verion type lists:
    [NUM]   [Android Version]
    [  1]   [AOSP]
    ---------------------------------------------------------------
    Please select Android Version (default 1 (AOSP)):1
    including device/amlogic/ac214/vendorsetup.sh
    including device/amlogic/ampere/vendorsetup.sh
    including device/amlogic/anning/vendorsetup.sh
    ...

----------
**Flash**

The target usb download image is out/target/product/bananapi_m5/aml_upgrade_package.img, flash it to your device by AML Usb_Burning_Tool or Burn_Card_Maker. Please refer to [Bananapi M5 wiki](http://wiki.banana-pi.org/Getting_Started_with_BPI-M5) for how to flash the target image.
