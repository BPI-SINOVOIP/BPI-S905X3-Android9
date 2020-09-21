# Atest

Atest is a command line tool that allows users to build, install and run Android tests locally.
This markdown will explain how to use atest on the commandline to run android tests.<br>

**For instructions on writing tests [go here](https://android.googlesource.com/platform/platform_testing/+/master/docs/index.md).**
Importantly, when writing your test's build script file (Android.mk), make sure to include
the variable `LOCAL_COMPATIBILITY_SUITE`.  A good default to use for it is `device-test`.

Curious about how atest works? Want to add a feature but not sure where to begin? [Go here.](./docs/developer_workflow.md)
Just want to learn about the overall structure? [Go here.](./docs/atest_structure.md)

##### Table of Contents
1. [Environment Setup](#environment-setup)
2. [Basic Usage](#basic-usage)
3. [Identifying Tests](#identifying-tests)
4. [Specifying Steps: Build, Install or Run](#specifying-steps:-build,-install-or-run)
5. [Running Specific Methods](#running-specific-methods)
6. [Running Multiple Classes](#running-multiple-classes)
7. [Detecting Metrics Regression](#detecting-metrics-regression)


## <a name="environment-setup">Environment Setup</a>

Before you can run atest, you first have to setup your environment.

##### 1. Run envsetup.sh
From the root of the android source checkout, run the following command:

`$ source build/envsetup.sh`

##### 2. Run lunch

Run the `$ lunch` command to bring up a "menu" of supported devices.  Find the device that matches
the device you have connected locally and run that command.

For instance, if you have an ARM device connected, you would run the following command:

`$ lunch aosp_arm64-eng`

This will set various environment variables necessary for running atest. It will also add the
atest command to your $PATH.

## <a name="basic-usage">Basic Usage</a>

Atest commands take the following form:

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;**atest \<optional arguments> \<tests to run>**

#### \<optional arguments>

<table>
<tr><td>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;-b</td><td>--build</td>
    <td>Build test targets.</td></tr>
<tr><td>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;-i</td><td>--install</td>
    <td>Install test artifacts (APKs) on device.</td></tr>
<tr><td>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;-t</td><td>--test</td>
    <td>Run the tests.</td></tr>
<tr><td>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;-s</td><td>--serial</td>
    <td>Run the tests on the specified device. Currently, one device can be tested at a time.</td></tr>
<tr><td>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;-d</td><td>--disable-teardown</td>
    <td>Disables test teardown and cleanup.</td></tr>
<tr><td>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;-m</td><td>--rebuild-module-info</td>
    <td>Forces a rebuild of the module-info.json file.</td></tr>
<tr><td>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;-w</td><td>--wait-for-debugger</td>
    <td>Only for instrumentation tests. Waits for debugger prior to execution.</td></tr>
<tr><td>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;-v</td><td>--verbose</td>
    <td>Display DEBUG level logging.</td></tr>
<tr><td>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; </td><td>--generate-baseline</td>
    <td>Generate baseline metrics, run 5 iterations by default.</td></tr>
<tr><td>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; </td><td>--generate-new-metrics</td>
    <td>Generate new metrics, run 5 iterations by default.</td></tr>
<tr><td>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; </td><td>--detect-regression</td>
    <td>Run regression detection algorithm.</td></tr>
<tr><td>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</td><td>--[CUSTOM_ARGS]</td>
    <td>Specify custom args for the test runners.</td></tr>
<tr><td>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;-h</td><td>--help</td>
    <td>Show this help message and exit</td></tr>
</table>

More information about **-b**, **-i** and **-t** can be found below under [Specifying Steps: Build, Install or Run.](#specifying-steps:-build,-install-or-run)

#### \<tests to run>

   The positional argument **\<tests to run>** should be a reference to one or more
   of the tests you'd like to run. Multiple tests can be run by separating test
   references with spaces.<br>

   Usage template: &nbsp; `atest <reference_to_test_1> <reference_to_test_2>`

   Here are some simple examples:

   &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`atest FrameworksServicesTests`<br>
   &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`atest example/reboot`<br>
   &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`atest FrameworksServicesTests CtsJankDeviceTestCases`<br>
   &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`atest FrameworksServicesTests:ScreenDecorWindowTests`<br>

   More information on how to reference a test can be found under [Identifying Tests.](#identifying-tests)


## <a name="identifying-tests">Identifying Tests</a>

  A \<reference_to_test> can be satisfied by the test's  **Module Name**,
**Module:Class**, **Class Name**, **TF Integration Test**, **File Path**
or **Package Name**.

  #### Module Name

  Identifying a test by its module name will run the entire module. Input
  the name as it appears in the `LOCAL_MODULE` or `LOCAL_PACKAGE_NAME`
  variables in that test's **Android.mk** or **Android.bp** file.

  Note: Use **TF Integration Test** to run non-module tests integrated directly
into TradeFed.

  Examples:<br>
      &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`atest FrameworksServicesTests`<br>
      &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`atest CtsJankDeviceTestCases`<br>


  #### Module:Class

  Identifying a test by its class name will run just the tests in that
  class and not the whole module. **Module:Class** is the preferred way to run
  a single class. **Module** is the same as described above. **Class** is the
  name of the test class in the .java file. It can either be the fully
  qualified class name or just the basic name.

  Examples:<br>
       &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
       `atest FrameworksServicesTests:ScreenDecorWindowTests`<br>
       &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
       `atest FrameworksServicesTests:com.android.server.wm.ScreenDecorWindowTests`<br>
       &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
       `atest CtsJankDeviceTestCases:CtsDeviceJankUi`<br>


  #### Class Name

  A single class can also be run by referencing the class name without
  the module name.

  Examples:<br>
      &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`atest ScreenDecorWindowTests`<br>
      &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`atest CtsDeviceJankUi`<br>

  However, this will take more time than the equivalent **Module:Class**
  reference, so we suggest using a **Module:Class** reference whenever possible.
  Examples below are ordered by performance from the fastest to the slowest:

  Examples:<br>
      &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
      `atest FrameworksServicesTests:com.android.server.wm.ScreenDecorWindowTests`<br>
      &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
      `atest FrameworksServicesTests:ScreenDecorWindowTests`<br>
      &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
      `atest ScreenDecorWindowTests`<br>

  #### TF Integration Test

  To run tests that are integrated directly into TradeFed (non-modules),
  input the name as it appears in the output of the "tradefed.sh list
  configs" cmd.

  Example:<br>
     &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`atest example/reboot` &nbsp;(runs [this test](https://android.googlesource.com/platform/tools/tradefederation/contrib/+/master/res/config/example/reboot.xml))<br>
     &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`atest native-benchmark` &nbsp;(runs [this test](https://android.googlesource.com/platform/tools/tradefederation/+/master/res/config/native-benchmark.xml))<br>


  #### File Path

  Both module-based tests and integration-based tests can be run by
  inputting the path to their test file or dir as appropriate. A single
  class can also be run by inputting the path to the class's java file.
  Both relative and absolute paths are supported.

  Example - 2 ways to run the `CtsJankDeviceTestCases` module via path:<br>
  1. run module from android \<repo root>: `atest cts/tests/jank`
  2. from android \<repo root>/cts/tests/jank: `atest .`


  Example - run a specific class within `CtsJankDeviceTestCases` module via path:<br>
  &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;From android \<repo root>:
  `atest cts/tests/jank/src/android/jank/cts/ui/CtsDeviceJankUi.java`

  Example - run an integration test via path:<br>
  &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;From android \<repo root>:
  `atest tools/tradefederation/contrib/res/config/example/reboot.xml`


  #### Package Name

  Atest supports searching tests from package name as well.

  Examples:<br>
     &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`atest com.android.server.wm`<br>
     &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`atest android.jank.cts`<br>

## <a name="specifying-steps:-build,-install-or-run">Specifying Steps: Build, Install or Run</a>

The **-b**, **-i** and **-t** options allow you to specify which steps you want
to run. If none of those options are given, then all steps are run. If any of
these options are provided then only the listed steps are run.

Note: **-i** alone is not currently support and can only be included with **-t**.
Both **-b** and **-t** can be run alone.


- Build targets only:  `atest -b <test>`
- Run tests only: `atest -t <test> `
- Install apk and run tests: `atest -it <test> `
- Build and run, but don't install: `atest -bt <test> `


Atest now has the ability to force a test to skip its cleanup/teardown step.
Many tests, e.g. CTS, cleanup the device after the test is run, so trying
to rerun your test with -t will fail without having the **--disable-teardown**
parameter. Use **-d** before -t to skip the test clean up step and test
iteratively.

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`atest -d <test>`<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`atest -t <test>`

Note that -t disables both **setup/install** and **teardown/cleanup** of the device.
So you can continue to rerun your test with just `atest -t <test>` as many times as you want.


## <a name="running-specific-methods">Running Specific Methods</a>

It is possible to run only specific methods within a test class. While the whole module will
still need to be built, this will greatly speed up the time needed to run the tests. To run only
specific methods, identify the class in any of the ways supported for identifying a class
(MODULE:CLASS, FILE PATH, etc) and then append the name of the method or method using the following
template:

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`<reference_to_class>#<method1>`

Multiple methods can be specified with commas:

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`<reference_to_class>#<method1>,<method2>,<method3>`

Examples:<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
`atest com.android.server.wm.ScreenDecorWindowTests#testMultipleDecors`<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
`atest FrameworksServicesTests:ScreenDecorWindowTests#testFlagChange,testRemoval`

Here are the two preferred ways to run a single method, we're specifying the `testFlagChange` method.
These are preferred over just the class name, because specifying the module or the java file location
allows atest to find the test much faster:

1. `atest FrameworksServicesTests:ScreenDecorWindowTests#testFlagChange`
2. From android \<repo root>:<br/>
`atest frameworks/base/services/tests/servicestests/src/com/android/server/wm/ScreenDecorWindowTests.java#testFlagChange`

Multiple methods can be ran from different classes and modules:<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
`atest FrameworksServicesTests:ScreenDecorWindowTests#testFlagChange,testRemoval ScreenDecorWindowTests#testMultipleDecors`

## <a name="running-multiple-classes">Running Multiple Classes</a>

To run multiple classes, deliminate them with spaces just like you would when running multiple tests.
Atest will handle building and running classes in the most efficient way possible, so specifying
a subset of classes in a module will improve performance over running the whole module.

Examples:<br>
- two classes in same module:<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
`atest FrameworksServicesTests:ScreenDecorWindowTests FrameworksServicesTests:DimmerTests`

- two classes, different modules:<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
`atest FrameworksServicesTests:ScreenDecorWindowTests CtsJankDeviceTestCases:CtsDeviceJankUi`


## <a name="detecting-metrics-regression">Detecting Metrics Regression</a>

Generate pre-patch or post-patch metrics without running regression detection:

Examples:<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`atest <test> --generate-baseline <optional iter>` <br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`atest <test> --generate-new-metrics <optional iter>`

Local regression detection can be run in three options:

1) Provide a folder containing baseline (pre-patch) metrics (generated previously).
   Atest will run the tests n (default 5) iterations, generate a new set of post-patch metrics, and compare those against existing metrics.

   Example:<br>
   &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
   `atest <test> --detect-regression </path/to/baseline> --generate-new-metrics <optional iter>`

2) Provide a folder containing post-patch metrics (generated previously).
   Atest will run the tests n (default 5) iterations, generate a new set of pre-patch
   metrics, and compare those against those provided. Note: the developer needs to
   revert the device/tests to pre-patch state to generate baseline metrics.

   Example:<br>
   &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
   `atest <test> --detect-regression </path/to/new> --generate-baseline <optional iter>`

3) Provide 2 folders containing both pre-patch and post-patch metrics. Atest will run no tests but
   the regression detection algorithm.

   Example:<br>
   &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
   `atest --detect-regression </path/to/baseline> </path/to/new>`


