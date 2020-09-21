# Create and run a suite

A suite allows one to run a group of tests for a given build. Adding or removing
individual test from a suite can be as simple as a test config file change. One
use case is that we can easily maintain a collection of stable tests for
presubmit check. If a test becomes unstable, we can make a single test config
change to pull it out of presubmit check.

A test can also be easily added to multiple suites by specifying the suite name
in the test config file. So one doesn't need to maintain a collection of test in
complicated GCL files, for a test to run in multiple suites.

## Create a suite

1.  Create a suite configuration file

Suite configuration file is similar to a TradeFed test configuration file. It
should always use "TfSuiteRunner" as runner and the only option needed is
"run-suite-tag", which defines the name of the suite. Any test configuration
file has option "test-suite-tag" set to that value will be included in that
suite.

A sample suite configuration file looks like this:


```
<configuration description="Android framework-base-presubmit test suite config.">
    <test class="com.android.tradefed.testtype.suite.TfSuiteRunner">
        <option name="run-suite-tag" value="framework-base-presubmit" />
    </test>
</configuration>
```

1.  Add test to suite

To include a test in the suite, it must be built to device-tests or
general-tests artifact. That is, include following in the mk file:

```
LOCAL_COMPATIBILITY_SUITE := device-tests
```

The test configuration file should also have option "test-suite-tag" set to the
value of the suite name, for example:

```
<option name="test-suite-tag" value="framework-base-presubmit" />
```

## Run a suite

### Run a suite in local build

After you build target tradefed-all (to include the new test suite
config), and the test targets (SystemUI and FrameworksNotificationTests in the
above example), you can run the suite locally. Running a suite job is similar to
running a test. The TradeFed command line looks like.

```
ANDROID_TARGET_OUT_TESTCASES=$ANDROID_TARGET_OUT_TESTCASES \
  tradefed.sh run template/local --template:map \
  test=suite/framework-base-presubmit --log-level-display VERBOSE
```

Before running that command line, you should have ran lunch command to set up
the environment properly.

### Run a suite with remote build

To run a suite with remote build, you need to provide build arguments, e.g.,
--branch, --build-id etc. A sample command is as follows:

```
tradefed.sh run google/template/lab-base \
  --template:map test=suite/framework-base-presubmit --test-tag SUITE \
  --branch git_master --build-flavor angler-userdebug \
  --build-os fastbuild3b_linux --build-id $BUILD_ID \
  --test-zip-file-filter .*device-tests.zip
```

"test-zip-file-filter" option tells TradeFed to download and extract all device
tests into a local build provider, so test configs can be located for the given
suite.
