# NBU Connectivity Test Suite

This suite includes tests that exercises Android connectivity APIs and verify
device behaviors.

## Overview

These tests simulate connectivity use cases critical to NBU market like
peer-to-peer interactions.

We created these tests using tools from the open source project Mobly, if you
want to learn more, see its [Github page](https://github.com/google/mobly) and
[tutorial](https://github.com/google/mobly/blob/master/docs/tutorial.md).

The tests have two components, an agent apk running on the Android Device Under
Test (DUT) and Python test scripts running on a computer to which the DUTs are
connected via USB. The tests issue cmds to the agent on the device to trigger
actions and read status, thus coordinating actions across multiple devices.

The tests by default write output to `/tmp/logs`.

## Test Environment Setup

This section lists the components and steps required to create a setup to run
these tests.

### Test Zip File

This is a zip file that you have downloaded.

The zip file includes:

*   this README file
*   an apk called `android_snippet.apk`
*   several Python files.

### Two Android Devices (DUT)

The two devices should be of the same model and build (identical fingerprint).

On each device:

*   Flash the build you want to test. The build's API level has to be `>=26`.
*   Enable USB debugging.
*   Enable BT snoop log.
*   Enable Wi-Fi verbose log.
*   Enable location service.
*   Install the `android_snippet.apk` with opition -g.

#### Create a test config file

Based on the two devices' serial numbers, we need to create a config file.

Create a plain text file `config.yaml` in the following format, with the `<>`
blocks replaced with the information of your actual devices:

```
TestBeds:
  - Name: P2pTestBed
    Controllers:
        AndroidDevice:
          - serial: <DUT1 serial>
          - serial: <DUT2 serial>
```

### Linux Computer (Ubuntu 14.04+)

We need the following packages:

*   adb
*   Python 2.7+

To check your Python's version, use command `$ python --version`.

In addition to those, we also need to install a few other tools:

```
$ sudo apt-get install python-setuptools python-pip
$ pip install mobly
```

After all the tools are installed, connect the devices to the computer with USB
cables and make sure they show up as "device" in the output of `$ adb devices`

## Test Execution

First, you need to put the file `config.yaml` in the same directory as the
Python scripts. Then cd to that directory and run:

```
$ rm -rf /tmp/logs/ # Clear previous logs
$ python xxx_test.py -c config.yaml
```

Execute all the `*_test.py` files the same way. Once they all finish executing,
all the logs will be collected in `/tmp/logs/`. You can check the test results
and debug info there.

Zip up the content of `/tmp/logs/` and send it to your contact at Google.
