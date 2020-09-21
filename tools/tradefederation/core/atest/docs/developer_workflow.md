# Atest Developer Workflow

This document explains the practical steps for contributing code to atest.

##### Table of Contents
1. [Identify the code you should work on](#identify-the-code-you-should-work-on)
2. [Working on the Python Code](#working-on-the-python-code)
3. [Working on the TradeFed Code](#working-on-the-tradefed-code)
4. [Working on the VTS-TradeFed Code](#working-on-the-vts-tradefed-code)
5. [Working on the Robolectric Code](#working-on-the-robolectric-code)


## <a name="what-code">Identify the code you should work on</a>

Atest is essentially a wrapper around various test runners. Because of
this division, your first step should be to identify the code
involved with your change. This will help determine what tests you write
and run.  Note that the wrapper code is written in python, so we'll be
referring to it as the "Python Code".

##### The Python Code

This code defines atest's command line interface.
Its job is to translate user inputs into (1) build targets and (2)
information needed for the test runner to run the test. It then invokes
the appropriate test runner code to run the tests. As the tests
are run it also parses the test runner's output into the output seen by
the user. It uses Test Finder and Test Runner classes to do this work.
If your contribution involves any of this functionality, this is the
code you'll want to work on.

<p>For more details on how this code works, checkout the following docs:

 - [General Structure](./atest_structure.md)
 - [Test Finders](./develop_test_finders.md)
 - [Test Runners](./develop_test_runners.md)

##### The Test Runner Code

This is the code that actually runs the test. If your change
involves how the test is actually run, you'll need to work with this
code.

Each test runner will have a different workflow. Atest currently
supports the following test runners:
- TradeFed
- VTS-TradeFed
- Robolectric


## <a name="working-on-the-python-code">Working on the Python Code</a>

##### Where does the Python code live?

The python code lives here: `tools/tradefederation/core/atest/`
(path relative to android repo root)

##### Writing tests

Test files go in the same directory as the file being tested. The test
file should have the same name as the file it's testing, except it
should have "_unittests" appended to the name. For example, tests
for the logic in `cli_translator.py` go in the file
`cli_translator_unittests.py` in the same directory.


##### Running tests

Python tests are just python files executable by the Python interpreter.
You can run ALL the python tests by executing this bash script in the
atest root dir: `./run_atest_unittests.sh`. Alternatively, you can
directly execute any individual unittest file. However, you'll need to
first add atest to your PYTHONPATH via entering in your terminal:
`PYTHONPATH=<atest_dir>:$PYTHONPATH`.

All tests should be passing before you submit your change.

## <a name="working-on-the-tradefed-code">Working on the TradeFed Code</a>

##### Where does the TradeFed code live?

The TradeFed code lives here:
`tools/tradefederation/core/src/com/android/tradefed/` (path relative
to android repo root).

The `testtype/suite/AtestRunner.java` is the most important file in
the TradeFed Code. It defines the TradeFed API used
by the Python Code, specifically by
`test_runners/atest_tf_test_runner.py`. This is the file you'll want
to edit if you need to make changes to the TradeFed code.


##### Writing tests

Tradefed test files live in a parallel `/tests/` file tree here:
`tools/tradefederation/core/tests/src/com/android/tradefed/`.
A test file should have the same name as the file it's testing,
except with the word "Test" appended to the end. <p>
For example, the tests for `tools/tradefederation/core/src/com/android/tradefed/testtype/suite/AtestRunner.java`
can be found in `tools/tradefederation/core/tests/src/com/android/tradefed/testtype/suite/AtestRunnerTest.java`.

##### Running tests

TradeFed itself is used to run the TradeFed unittests so you'll need
to build TradeFed first. See the
[TradeFed README](../../README.md) for information about setting up
TradeFed. <p>
There are so many TradeFed tests that you'll probably want to
first run the test file your code change affected individually. The
command to run an individual test file is:<br>

`tradefed.sh run host -n --class <fully.qualified.ClassName>`

Thus, to run all the tests in AtestRunnerTest.java, you'd enter:

`tradefed.sh run host -n --class com.android.tradefed.testtype.suite.AtestRunnerTest`

To run ALL the TradeFed unittests, enter:
`./tools/tradefederation/core/tests/run_tradefed_tests.sh`
(from android repo root)

Before submitting code you should run all the TradeFed tests.

## <a name="working-on-the-vts-tradefed-code">Working on the VTS-TradeFed Code</a>

##### Where does the VTS-TradeFed code live?

The VTS-Tradefed code lives here: `test/vts/tools/vts-tradefed/`
(path relative to android repo root)

##### Writing tests

You shouldn't need to edit vts-tradefed code, so there is no
need to write vts-tradefed tests. Reach out to the vts team
if you need information on their unittests.

##### Running tests

Again, you shouldn't need to change vts-tradefed code.

## <a name="working-on-the-robolectric-code">Working on the Robolectric Code</a>

##### Where does the Robolectric code live?

The Robolectric code lives here: `prebuilts/misc/common/robolectric/3.6.1/`
(path relative to android repo root)

##### Writing tests

You shouldn't need to edit this code, so no need to write tests.

##### Running tests

Again, you shouldn't need to edit this code, so no need to run test.
