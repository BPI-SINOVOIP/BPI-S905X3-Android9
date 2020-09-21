# Android Comms Test Suite
The Android Comms Test Suite, is a lightweight Python-based automation tool set
that is used to perform automated testing of current and upcoming Android
devices. It provides a simple execution interface; a set of pluggable libraries
for accessing commercially avilable devices, Android devices, and a collection
of utility functions to further ease test development. It is an ideal desktop
tool for a wireless stack developer or integrator whether exercising a new code
path, performing sanity testing, or running extended regression test suites.

Included in the tests/google directory are a bundle of tests, many of which can
be run with as little as one or two Android devices with wifi, cellular, or
bluetooth connectivity, including:
1. Wifi tests for access point interopability, enterprise server integration,
WiFi scanning, WiFi auto join, and round trip time.
2. Bluetooth tests for low energy, GATT, SPP, and bonding.
3. Cellular tests for circuit switch and IMS calling, data connectivity,
messaging, network switching, and WiFi hotspot.

ACTS follows the Google Open-source
[Python Style Guide](https://google.github.io/styleguide/pyguide.html), and
it is recommended for all new test cases.

System dependencies:
  - adb
  - python3.4+
  - python3.4-setuptools

Python dependencies (installed automatically by setup.py):
  - future
  - pyserial

To run unit tests:
$ python3 setup.py test
$ python setup.py test

## ACTS Execution Flow Overview
Below is a high level view of the ACTS flow:

1. Read configuration files
2. Create controllers
3. Sequentially execute test classes

```
FooTest.setup_class()
FooTest.setup_test()
FooTest.test_A()
FooTest.teardown_test()
FooTest.setup_test()
FooTest.test_B()
FooTest.teardown_test()
....
FooTest.teardown_class()
BarTest.setup_class()
....
```

4. Destroy controllers

## Preparing an Android Device
### Allow USB Debugging
USB debugging must be enabled before a device can take commands from adb.
To enable USB debugging, first enable developer mode.
1. Go to Settings->About phone
2. Tap Build number repeatedly until "You're a developer now" is displayed.

In developer mode:
1. Plug the device into a computer (host)
2. Run `$adb devices`
- A pop-up asking to allow the host to access the android device may be
displayed. Check the "Always" box and click "Yes".

## ACTS Setup

1. Install the system dependencies.
     On Ubuntu, sudo apt-get install python3.4 python3-setuptools
2. Run "python3.4 setup.py install" with elevated permissions
3. To verify ACTS is ready to go, at the location for README, and run:
     cd framework/tests/ \
     && act.py -c acts_sanity_test_config.json -tc IntegrationTest

After installation, `act.py` will be in usr/bin and can be called as command
line utilities. Components in ACTS are importable under the package "acts."
in Python, for example:

```
$ python
>>> from acts.controllers import android_device
>>> device_list = android_device.get_all_instances()
```

## Breaking Down a Sample Command

Above, the command `act.py -c acts_sanity_test_config.json -tc IntegrationTest`
was run to verify ACTS was properly set up.
Below are the components of that command:
- `act.py`: is the script that runs the test
-  -c acts_sanity_test_config: is the flag and name of the configuration file
to be used in the test
-  -tc IntegrationTest: is the name of the test case

### Configuration Files
To run tests, required information must be provided via a json-formatted
text file. The required information includes a list of ***testbed*** configs.
Each specifies the hardware, services, the path to the logs directory, and
a list of paths where the python test case files are located. Below are the
contents of a sample configuration file:

```
{   "_description": "This is an example skeleton test configuration file.",
    "testbed":
    [
        {
            "_description": "Sample testbed with no devices",
            "name": "SampleTestBed"
        }
    ],
    "logpath": "/tmp/logs",
    "testpaths": ["../tests/sample"],
    "custom_param1": {"favorite_food": "Icecream!"}
}
```
The ***testpaths*** and ***logpath*** keys may alternately be supplied via the
execution environment though the ACTS_TESTPATHS and ACTS_LOGPATH keys
respectively. To specify multiple test paths, the key should follow
standard a ':'-delimited format. Explicit keys in a configuration file will
override any defaults provided by the environment.

### Test Class
Test classes are instantiated with a dictionary of “controllers”. The
controllers dictionary contains all resources provided to the test class
and are created based on the provided configuration file.

Test classes must also contain an iterable member self.tests that lists the
test case names within the class.  More on this is discussed after the
following code snippet.

```
from acts.base_test import BaseTestClass

class SampleTest(BaseTestClass):

    def __init__(self, controllers):
        BaseTestClass.__init__(self, controllers)
        self.tests = (
            "test_make_toast",
        )

    """Tests"""
    def test_make_toast(self):
        for ad in self.android_devices:
            ad.droid.makeToast("Hello World.")
        return True
```
By default all test cases listed in a Test Class\'s self.tests will be run.
Using the syntax below will override the default behavior by executing only
specific tests within a test class.

The following will run a single test, test_make_toast:

`$ act.py -c sample_config.txt -tb SampleTestBed -tc SampleTest:test_make_toast`

Multiple tests may be specified with a comma-delimited list. The following
will execute test_make_toast and test_make_bagel:

- `$ act.py -c sample_config.txt -tb SampleTestBed -tc
SampleTest:test_make_toast,test_make_bagel`
