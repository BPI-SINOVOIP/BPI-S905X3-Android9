# Longevity Library

This library serves as a test runner for longevity suites, which exercise test cases repeatedly in
order to exercise or stress the device under test. Annotate a collection of JUnit test classes as a
`LongevitySuite`, and voila...you now have longevity tests!

## Examples

There is both host-side (core) and device-side (platform) for longevity tests with only
minor differences. All of the examples below use the sample suites bundled as part of the
`LongevityHostLibSamples` and `LongevityPlatformLibSamples` modules under the `samples/` directory.
Follow the samples directory to see how longevity suites are constructed.

### Host

**Template command with stubbed out options.**

`java <options> -cp <jar> org.junit.runner.JUnitCore <suite>`

**Run simple test suite 50 times, and quit when an error is encountered.**

`java -Diterations=5 -Dquitter=true out/host/linux-x86/framework/LongevityHostSamples.jar
org.junit.runner.JUnitCore android.longevity.core.samples.SimpleSuite`

**Run simple test suite 10 times, shuffle all tests, and quit after 30 minutes.**

`java -Diterations=10 -Dsuite-timeout_msec=1800000 -Dshuffle=true
out/host/linux-x86/framework/LongevityHostSamples.jar
org.junit.runner.JUnitCore android.longevity.core.samples.SimpleSuite`

### Device

**Template command with stubbed out options.**

`adb shell am instrument -w -r -e <option> <value> -e class <suite>
android.platform.longevity.samples/android.support.test.runner.AndroidJUnitRunner`

**Run simple test suite 50 times, and quit when an error is encountered.**

`adb shell am instrument -w -r -e iterations 50 -e quitter true
-e class android.platform.longevity.samples.SimpleSuite
android.platform.longevity.samples/android.support.test.runner.AndroidJUnitRunner`

**Run simple test suite 10 times, shuffle all tests, and quit after 30 minutes.**

`adb shell am instrument -w -r -e iterations 10 -e shuffle true -e suite-timeout_msec 1800000
-e class android.platform.longevity.samples.SimpleSuite
android.platform.longevity.samples/android.support.test.runner.AndroidJUnitRunner`

**Run simple test suite 100 times, and quit when battery drops below 5%.**

`adb shell am instrument -w -r -e iterations 100 -e min-battery 0.05
-e class android.platform.longevity.samples.SimpleSuite
android.platform.longevity.samples/android.support.test.runner.AndroidJUnitRunner`

## Options

*   `iterations <int>` - the number of times to repeat the suite.
*   `min-battery <double>` - quit if battery falls below this threshold.
*   `shuffle <bool>` - shuffles all test cases in the repeated suite.
*   `suite-timeout_msec <long>` - an overall timeout for the suite.
*   `timeout_msec <long>` - a timeout for individual test methods.
*   `quitter <bool>` - quit the suite if any test errors are encountered.

## Tests

Host-side tests for the library can be built with the `LongevityHostLibTests` module under the
`tests/` directory. Run them using `java out/host/linux-x86/framework/LongevityHostLibTests.jar
org.junit.runner.JUnitCore <test class>`.

Device-side tests for the library can be built with the `LongevityLibTests` module under the
`tests/` directory. Run them using `adb shell am instrument -w -r
android.platform.longevity.tests/android.support.test.runner.AndroidJUnitRunner`.

## Issues

If any issues are encountered, please send patches to recent contributors.
