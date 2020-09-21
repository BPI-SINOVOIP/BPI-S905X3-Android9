# VTS User Manual

Linux is officially supported for building and running VTS. Building on Windows is not supported, but it is possible to [run VTS on Windows](#run_vts) with Python, Java, and ADB installed.

The following instructions assume Linux environment.

## 1. Setup

### 1.1. Host setup

Please follow:

* [Setup Manual](setup/index.md)

(Optional)
During VTS test runs, required Python packages are downloaded from the [Python Package Index](https://pypi.python.org/simple/). There is an option to instead install these packages from a local directory during test runtime by predownloading the packages. First run the lunch command, then set an environment variable VTS_PYPI_PATH as a new local directory to host the Python packages. Then run the download-pypi-packages.sh script:

* `$ cd ${branch}`

* `$ . test/vts/script/download-pypi-packages.sh`

### 1.2. Checkout master git repository

[Download Android Source Code](https://source.android.com/source/downloading.html)

`$ export branch=master`

`$ mkdir ${branch}`

`$ cd ${branch}`

`$ repo init -b ${branch} -u persistent https://android.googlesource.com/platform/manifest`

`$ repo sync -j 8`

### 1.3. Build an Android image

`$ cd ${branch}`

`$ . build/make/envsetup.sh`

`$ lunch aosp_arm64-userdebug  # or <your device>-userdebug`

The below is an optional step:

`$ make -j 8`

If this fails, please do:

`$ repo sync -j 8`

`$ make -j 8`

Such can happen because tip of tree (ToT) may not always be buildable.

### 1.4. Build a VTS package

`$ cd ${branch}`

`$ make vts -j8`

Or use the exact command:

`$ make -j8 vts showcommands dist TARGET_PRODUCT=aosp_arm64 WITH_DEXPREOPT=false TARGET_BUILD_VARIANT=userdebug`

### 1.5. Connect to an Android device

Let's connect an Android device and a host computer using a USB cable.

* On an Android device, Setting -> About Phone -> Click repeatedly 'Build number' until developer mode is enabled.
* On an Android device, Setting -> Developer options -> Turn on 'USB debugging'
* On a host, run `adb devices` from a command line shell.
* On a Android device, confirm that the host is trusted.
* On a host, type `adb shell` and if that works, we're ready.

## 2. Run VTS Tests

### <a name="run_vts" /> 2.1. Run a VTS test plan

For Linux users,

`$ vts-tradefed`

`> run vts`

For Windows users, please build on Linux. Then copy the following zip file to Windows and extract it.

`out/host/linux-x86/vts/android-vts.zip`

Launch the batch file in the extracted folder.

`$ android-vts\tools\vts-tradefed_win.bat`

`> run vts`

Example stdout:

```
…
…
08-16 09:36:03 I/ResultReporter: Saved logs for device_logcat in .../out/host/linux-x86/vts/android-vts/logs/2016.08.16_09.17.13/device_logcat_7912321856562095748.zip
08-16 09:36:03 I/ResultReporter: Saved logs for host_log in .../out/host/linux-x86/vts/android-vts/logs/2016.08.16_09.17.13/host_log_2775945280523850018.zip
08-16 09:36:04 I/ResultReporter: Invocation finished in 18m 50s. PASSED: 18, FAILED: 0, NOT EXECUTED: 2, MODULES: 8 of 10
08-16 09:36:04 I/ResultReporter: Test Result: .../out/host/linux-x86/vts/android-vts/results/2016.08.16_09.17.13/test_result.xml
08-16 09:36:04 I/ResultReporter: Full Result: .../out/host/linux-x86/vts/android-vts/results/2016.08.16_09.17.13.zip
```

### 2.2. Test report for APFE (Android Partner Front-End)

The uploadable report xml file can be found at

`out/host/linux-x86/vts/android-vts/results/`

After Android O release, you will be able to upload that xml file to [AFPE](https://partner.android.com)
and obtain a certificate.

### 2.3. Check the test logs

`$ vi out/host/linux-x86/vts/android-vts/logs/`

Then select a directory which captures the time stamp of your test run (e.g., 2016.08.16_09.17.13).

Then select `host_log_<timestamp>.zip` and host_log.txt in that zip file for host log.

Then select `device_logcat_<timestamp>.zip` and device_logcat.txt in that zip file for device log.

## 3. Run Options for Advanced Users

### 3.1. List of VTS Plans

Documented at [here](../tools/vts-tradefed/res/config/plans.md).

### 3.2. Run kernel test cases

LTP (Linux Test Project) is part of vts-kernel.
[This doc](developer_testing/kernel/ltp.md) shows how to run each LTP test case.

## 4. Debugging

### 4.1. Run VTS tests directly for debugging

[Run directly from command line](testcase_develop_manual/run_vts_directly.md)
