# Wi-Fi RTT (IEEE 802.11mc) Integrated (ACTS/sl4a) Test Suite

This directory contains ACTS/sl4a test scripts to verify and characterize
the Wi-Fi RTT (IEEE 802.11mc) implementation in Android.

There are 2 groups of tests (in 2 sub-directories):

* functional: Functional tests that each implementation must pass. These
are pass/fail tests.
* stress: Tests which run through a large number of iterations to stress
test the implementation. Considering that some failures are expected,
especially in an over-the-air situation, pass/fail criteria are either
not provided or may not apply to all implementations or test environments.

The tests can be executed using:

`act.py -c <config> -tc {<test_class>|<test_class>:<test_name>}`

Where a test file is any of the `.py` files in any of the test sub-directories.
If a test class is specified, then all tests within that test class are executed.

## Test Beds
The Wi-Fi RTT tests support several different test scenarios which require different test bed
configuration. The test beds and their corresponding test files are:

* Device Under Test + AP which supports IEEE 802.11mc
  * functional/RangeApSupporting11McTest.py
  * functional/RttRequestManagementTest.py
  * functional/RttDisableTest.py
  * stress/StressRangeApTest.py
* Device Under Test + AP which does **not** support IEEE 802.11mc
  * functional/RangeApNonSupporting11McTest.py
* 2 Devices Under Test
  * functional/RangeAwareTest.py
  * functional/AwareDiscoveryWithRangingTest.py
  * functional/RangeSoftApTest.py
  * stress/StressRangeAwareTest.py

## Test Configurations
The test configuration, the `<config>` in the commands above, is stored in
the *config* sub-directory. The configuration simply uses all connected
devices without listing specific serial numbers. Note that some tests use a
single device while others use 2 devices.

The only provided configuration is *wifi_rtt.json*.

The configuration defines the following keys to configure the test:

* **lci_reference**, **lcr_reference**: Arrays of bytes used to validate that the *correct* LCI and
LCR were received from the AP. These are empty by default and should be configured to match the
configuration of the AP used in the test.
* **rtt_reference_distance_mm**: The reference distance, in mm, between the test device and the test
AP or between the two test devices (for Aware ranging tests).
* **stress_test_min_iteration_count**, **stress_test_target_run_time_sec**: Parameters used to
control the length and duration of the stress tests. The stress test runs for the specified number
of iterations or for the specified duration - whichever is longer.
