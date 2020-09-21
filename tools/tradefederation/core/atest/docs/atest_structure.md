# Atest Developer Guide

You're here because you'd like to contribute to atest. To start off, we'll
explain how atest is structured and where the major pieces live and what they
do. If you're more interested in how to use atest, go to the [README](../README.md).

##### Table of Contents
1. [Overall Structure](#overall-structure)
2. [Major Files and Dirs](#major-files-and-dirs)
3. [Test Finders](#test-finders)
4. [Test Runners](#test-runners)
5. [Constants Override](#constants-override)

## <a name="overall-structure">Overall Structure</a>

Atest is primarily composed of 2 components: [test finders](#test-finders) and
[test runners](#test-runners). At a high level, atest does the following:
1. Parse args and verify environment
2. Find test(s) based on user input
3. Build test dependencies
4. Run test(s)

Let's walk through an example run and highlight what happens under the covers.

> ```# atest hello_world_test```

Atest will first check the environment is setup and then load up the
module-info.json file (and build it if it's not detected or we want to rebuild
it). That is a critical piece that atest depends on. Module-info.json contains a
list of all modules in the android repo and some relevant info (e.g.
compatibility_suite, auto_gen_config, etc) that is used during the test finding
process. We create the results dir for our test runners to dump results in and
proceed to the first juicy part of atest, finding tests.

The tests specified by the user are passed into the ```CLITranslator``` to
transform the user input into a ```TestInfo``` object that contains all of the
required and optional bits used to run the test as how the user intended.
Required info would be the test name, test dependencies, and the test runner
used to run the test. Optional bits would be additional args for the test and
method/class filters.

Once ```TestInfo``` objects have been constructed for all the tests passed in
by the user, all of the test dependencies are built. This step can by bypassed
if the user specifies only _-t_ or _-i_.

The final step is to run the tests which is where the test runners do their job.
All of the ```TestInfo``` objects get passed into the ```test_runner_handler```
which invokes each ```TestInfo``` specified test runner. In this specific case,
the ```AtestTradefedTestRunner``` is used to kick off ```hello_world_test```.

Read on to learn more about the classes mentioned.

## <a name="major-files-and-dirs">Major Files and Dirs</a>

Here is a list of major files and dirs that are important to point out:
* ```atest.py``` - Main entry point.
* ```cli_translator.py``` - Home of ```CLITranslator``` class. Translates the
  user input into something the test runners can understand.
* ```test_finder_handler.py``` - Module that collects all test finders,
  determines which test finder methods to use and returns them for
  ```CLITranslator``` to utilize.
* ```test_finders/``` - Location of test finder classes. More details on test
  finders [below](#test-finders).
* ```test_finders/test_info.py``` - Module that defines ```TestInfo``` class.
* ```test_runner_handler.py``` - Module that collects all test runners and
  contains logic to determine what test runner to use for a particular
  ```TestInfo```.
* ```test_runners/``` - Location of test runner classes. More details on test
  runners [below](#test-runners).
* ```constants_default.py``` - Location of constant defaults. Need to override
  some of these constants for your private repo? [Instructions below](#constants-override).

## <a name="test-finders">Test Finders</a>

Test finders are classes that host find methods. The find methods are called by
atest to find tests in the android repo based on the user's input (path,
filename, class, etc).  Find methods will also find the corresponding test
dependencies for the supplied test, translating it into a form that a test
runner can understand, and specifying the test runner.

For more details and instructions on how to create new test finders,
[go here](./develop_test_finders.md)

## <a name="test-runners">Test Runners</a>

Test Runners are classes that execute the tests. They consume a ```TestInfo```
and execute the test as specified.

For more details and instructions on how to create new test runners, [go here](./develop_test_runners.md)

## <a name="constants-override">Constants Override</a>

You'd like to override some constants but not sure how?  Override them with your
own constants_override.py that lives in your own private repo.

1. Create new ```constants_override.py``` (or whatever you'd like to name it) in
  your own repo. It can live anywhere but just for example purposes, let's
  specify the path to be ```<private repo>/path/to/constants_override/constants_override.py```.
2. Add a ```vendorsetup.sh``` script in ```//vendor/<somewhere>``` to export the
  path of ```constants_override.py``` base path into ```PYTHONPATH```.
```bash
# This file is executed by build/envsetup.sh
_path_to_constants_override="$(gettop)/path/to/constants_override"
if [[ ! $PYTHONPATH == *${_path_to_constants_override}* ]]; then
  export PYTHONPATH=${_path_to_constants_override}:$PYTHONPATH
fi
```
3. Try-except import ```constants_override``` in ```constants.py```.
```python
try:
    from constants_override import *
except ImportError:
    pass
```
4. You're done! To pick up the override, rerun build/envsetup.sh to kick off the
  vendorsetup.sh script.
