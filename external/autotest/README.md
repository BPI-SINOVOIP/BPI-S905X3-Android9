# Autotest: Automated integration testing for Android and Chrome OS Devices

Autotest is a framework for fully automated testing. It was originally designed
to test the Linux kernel, and expanded by the Chrome OS team to validate
complete system images of Chrome OS and Android.

Autotest is composed of a number of modules that will help you to do stand alone
tests or setup a fully automated test grid, depending on what you are up to.
A non extensive list of functionality is:

* A body of code to run tests on the device under test.  In this setup, test
  logic executes on the machine being tested, and results are written to files
  for later collection from a development machine or lab infrastructure.

* A body of code to run tests against a remote device under test.  In this
  setup, test logic executes on a development machine or piece of lab
  infrastructure, and the device under test is controlled remotely via
  SSH/adb/some combination of the above.

* Developer tools to execute one or more tests.  `test_that` for Chrome OS and
  `test_droid` for Android allow developers to run tests against a device
  connected to their development machine on their desk.  These tools are written
  so that the same test logic that runs in the lab will run at their desk,
  reducing the number of configurations under which tests are run.

* Lab infrastructure to automate the running of tests.  This infrastructure is
  capable of managing and running tests against thousands of devices in various
  lab environments. This includes code for both synchronous and asynchronous
  scheduling of tests.  Tests are run against this hardware daily to validate
  every build of Chrome OS.

* Infrastructure to set up miniature replicas of a full lab.  A full lab does
  entail a certain amount of administrative work which isn't appropriate for
  a work group interested in automated tests against a small set of devices.
  Since this scale is common during device bringup, a special setup, called
  Moblab, allows a natural progressing from desk -> mini lab -> full lab.

## Run some autotests

See the guides to `test_that` and `test_droid`:

[test\_droid Basic Usage](docs/test-droid.md)

[test\_that Basic Usage](docs/test-that.md)

## Write some autotests

See the best practices guide, existing tests, and comments in the code.

[Autotest Best Practices](docs/best-practices.md)


## Grabbing the latest source

`git clone https://chromium.googlesource.com/chromiumos/third_party/autotest`


## Hacking and submitting patches

See the coding style guide for guidance on submitting patches.

[Coding Style](docs/coding-style.md)

## Pre-upload hook dependencies

You need to run `utils/build_externals.py` to set up the dependencies
for pre-upload hook tests.
