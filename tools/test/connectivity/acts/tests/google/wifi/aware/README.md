# Wi-Fi Aware Integrated (ACTS/sl4a) Test Suite

This directory contains ACTS/sl4a test scripts to verify and characterize
the Wi-Fi Aware implementation in Android.

There are 4 groups of tests (in 4 sub-directories):

* functional: Functional tests that each implementation must pass. These
are pass/fail tests.
* performance: Tests which measure performance of an implementation - e.g.
latency or throughput. Some of the tests may not have pass/fail results -
they just record the measured performance. Even when tests do have a pass/
fail criteria - that criteria may not apply to all implementations.
* stress: Tests which run through a large number of iterations to stress
test the implementation. Considering that some failures are expected,
especially in an over-the-air situation, pass/fail criteria are either
not provided or may not apply to all implementations or test environments.
* ota (over-the-air): A small number of tests which configure the device
in a particular mode and expect the tester to capture an over-the-air
sniffer trace and analyze it for validity. These tests are **not** automated.

The tests can be executed in several ways:

1. Individual test(s): `act.py -c <config> -tc {<test_class>|<test_class>:<test_name>}`

Where a test file is any of the `.py` files in any of the test sub-directories.
If a test class is specified, then all tests within that test class are executed.

2. All tests in a test group: `act.py -c <config> -tf <test_file>`

Where `<test_file>` is a file containing a list of tests. Each of the test
group sub-directories contains a file with the same name as that of the
directory which lists all tests in the directory. E.g. to execute all functional
tests execute:

`act.py -c <config> -tf ./tools/test/connectivity/acts/tests/google/wifi/aware/functional/functional`

## Test Configurations
The test configurations, the `<config>` in the commands above, are stored in
the *config* sub-directory. The configurations simply use all connected
devices without listing specific serial numbers. Note that some tests use a
single device while others use 2 devices. In addition, the configurations
define the following key to configure the test:

* **aware_default_power_mode**: The power mode in which to run all tests. Options
are `INTERACTIVE` and `NON_INTERACTIVE`.

The following configurations are provided:
* wifi_aware.json: Normal (high power/interactive) test mode.
* wifi_aware_non_interactive.json: Low power (non-interactive) test mode.
