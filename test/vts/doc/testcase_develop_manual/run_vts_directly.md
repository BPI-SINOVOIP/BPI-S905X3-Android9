# Run a VTS test directly

First of all, if you have not done [VTS setup](../setup/index.md), that is required here.

## Download required Python packages to local host

`$ . test/vts/script/pypi-packages-local.sh`

## Build Binaries

`$ cd test/vts/script`

`$ ./create-image.sh <build target>-userdebug`

or

`$ ./create-image.sh aosp_salifish-userdebug`

## Copy Binaries

`$ ./setup-local.sh <device name>`

or

`$ ./setup-local.sh sailfish`

## Run a test direclty

`$ PYTHONPATH=$PYTHONPATH:.. python -m vts.testcases.<test py package path> $ANDROID_BUILD_TOP/test/vts/testcases/<path to its config file>`

For example, for SampleShellTest, please run:

`PYTHONPATH=$PYTHONPATH:.. python -m vts.testcases.host.shell.SampleShellTest $ANDROID_BUILD_TOP/test/vts/testcases/host/shell/SampleShellTest.config`

More examples are in `test/vts/script/run-local.sh`.

## Additional Step for LTP and Linux-Kselftest

Add `'data_file_path' : '<your android build top>/out/host/linux-x86/vts/android-vts/testcases'`
to your config file (e.g., `VtsKernelLtpStaging.config`).

## Add a new test

In order to add a new test, the following two files needed to be extended.

`test/vts/create-image.sh`

`test/vts/setup-local.sh`

Optionally, the command used to add a new test can be also added to:

`test/vts/run-local.sh`
