# Test Runner Developer Guide

Learn about test runners and how to create a new test runner class.

##### Table of Contents
1. [Test Runner Details](#test-runner-details)
2. [Creating a Test Runner](#creating-a-test-runner)

## <a name="test-runner-details">Test Runner Details</a>

The test runner class is responsible for test execution. Its primary logic
involve construction of the commandline given a ```TestInfo``` and
```extra_args``` passed into the ```run_tests``` method. The extra_args are
top-level args consumed by atest passed onto the test runner. It is up to the
test runner to translate those args into the specific args the test runner
accepts. In this way, you can think of the test runner as a translator between
the atest CLI and your test runner's CLI. The reason for this is so that atest
can have a consistent CLI for args instead of requiring the users to remember
the differing CLIs of various test runners.  The test runner should also
determine its specific dependencies that need to be built prior to any test
execution.

## <a name="creating-a-test-runner">Creating a Test Runner</a>

First thing to choose is where to put the test runner. This will primarily
depend on if the test runner will be public or private. If public,
```test_runners/``` is the default location.

> If it will be private, then you can
> follow the same instructions for ```vendorsetup.sh``` in
> [constants override](atest_structure.md#constants-override) where you will
> add the path of where the test runner lives into ```$PYTHONPATH```. Same
> rules apply, rerun ```build/envsetup.sh``` to update ```$PYTHONPATH```.

To create a new test runner, create a new class that inherits
```TestRunnerBase```. Take a look at ```test_runners/example_test_runner.py```
to see what a simple test runner will look like.

**Important Notes**
You'll need to override the following parent methods:
* ```host_env_check()```: Check if host environment is properly setup for the
  test runner. Raise an expception if not.
* ```get_test_runner_build_reqs()```: Return a set of build targets that need
  to be built prior to test execution.
* ```run_tests()```: Execute the test(s).

And define the following class vars:
* ```NAME```: Unique name of the test runner.
* ```EXECUTABLE```: Test runner command, should be an absolute path if the
  command can not be found in ```$PATH```.

There is a parent helper method (```run```) that should be used to execute the
actual test command.

Once the test runner class is created, you'll need to add it in
```test_runner_handler``` so that atest is aware of it. Try-except import the
test runner in ```_get_test_runners``` like how ```ExampleTestRunner``` is.
```python
try:
    from test_runners import new_test_runner
    test_runners_dict[new_test_runner.NewTestRunner.NAME] = new_test_runner.NewTestRunner
except ImportError:
    pass
```
