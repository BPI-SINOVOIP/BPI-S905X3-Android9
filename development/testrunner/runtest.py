#!/usr/bin/env python
#
# Copyright 2008, The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Command line utility for running Android tests

runtest helps automate the instructions for building and running tests
- It builds the corresponding test package for the code you want to test
- It pushes the test package to your device or emulator
- It launches InstrumentationTestRunner (or similar) to run the tests you
specify.

runtest supports running tests whose attributes have been pre-defined in
_TEST_FILE_NAME files, (runtest <testname>), or by specifying the file
system path to the test to run (runtest --path <path>).

Do runtest --help to see full list of options.
"""

# Python imports
import glob
import optparse
import os
import re
from sets import Set
import sys
import time

# local imports
import adb_interface
import android_build
from coverage import coverage
import errors
import logger
import make_tree
import run_command
from test_defs import test_defs
from test_defs import test_walker


class TestRunner(object):
  """Command line utility class for running pre-defined Android test(s)."""

  _TEST_FILE_NAME = "test_defs.xml"

  # file path to android core platform tests, relative to android build root
  # TODO move these test data files to another directory
  _CORE_TEST_PATH = os.path.join("development", "testrunner",
                                 _TEST_FILE_NAME)

  # vendor glob file path patterns to tests, relative to android
  # build root
  _VENDOR_TEST_PATH = os.path.join("vendor", "*", "tests", "testinfo",
                                   _TEST_FILE_NAME)

  _RUNTEST_USAGE = (
      "usage: runtest.py [options] short-test-name[s]\n\n"
      "The runtest script works in two ways.  You can query it "
      "for a list of tests, or you can launch one or more tests.")

  # default value for make -jX
  _DEFAULT_JOBS = 16

  _DALVIK_VERIFIER_PROP = "dalvik.vm.dexopt-flags"
  _DALVIK_VERIFIER_OFF_VALUE = "v=n"
  _DALVIK_VERIFIER_OFF_PROP = "%s = %s" %(_DALVIK_VERIFIER_PROP, _DALVIK_VERIFIER_OFF_VALUE)

  # regular expression to match path to artifacts to install in make output
  _RE_MAKE_INSTALL = re.compile(r'INSTALL-PATH:\s([^\s]+)\s(.*)$')

  def __init__(self):
    # disable logging of timestamp
    self._root_path = android_build.GetTop()
    out_base_name = os.path.basename(android_build.GetOutDir())
    # regular expression to find remote device path from a file path relative
    # to build root
    pattern = r'' + out_base_name + r'\/target\/product\/\w+\/(.+)$'
    self._re_make_install_path = re.compile(pattern)
    logger.SetTimestampLogging(False)
    self._adb = None
    self._known_tests = None
    self._options = None
    self._test_args = None
    self._tests_to_run = None

  def _ProcessOptions(self):
    """Processes command-line options."""
    # TODO error messages on once-only or mutually-exclusive options.
    user_test_default = os.path.join(os.environ.get("HOME"), ".android",
                                     self._TEST_FILE_NAME)

    parser = optparse.OptionParser(usage=self._RUNTEST_USAGE)

    parser.add_option("-l", "--list-tests", dest="only_list_tests",
                      default=False, action="store_true",
                      help="To view the list of tests")
    parser.add_option("-b", "--skip-build", dest="skip_build", default=False,
                      action="store_true", help="Skip build - just launch")
    parser.add_option("-j", "--jobs", dest="make_jobs",
                      metavar="X", default=self._DEFAULT_JOBS,
                      help="Number of make jobs to use when building")
    parser.add_option("-n", "--skip_execute", dest="preview", default=False,
                      action="store_true",
                      help="Do not execute, just preview commands")
    parser.add_option("-i", "--build-install-only", dest="build_install_only", default=False,
                      action="store_true",
                      help="Do not execute, build tests and install to device only")
    parser.add_option("-r", "--raw-mode", dest="raw_mode", default=False,
                      action="store_true",
                      help="Raw mode (for output to other tools)")
    parser.add_option("-a", "--suite-assign", dest="suite_assign_mode",
                      default=False, action="store_true",
                      help="Suite assignment (for details & usage see "
                      "InstrumentationTestRunner)")
    parser.add_option("-v", "--verbose", dest="verbose", default=False,
                      action="store_true",
                      help="Increase verbosity of %s" % sys.argv[0])
    parser.add_option("-w", "--wait-for-debugger", dest="wait_for_debugger",
                      default=False, action="store_true",
                      help="Wait for debugger before launching tests")
    parser.add_option("-c", "--test-class", dest="test_class",
                      help="Restrict test to a specific class")
    parser.add_option("-m", "--test-method", dest="test_method",
                      help="Restrict test to a specific method")
    parser.add_option("-p", "--test-package", dest="test_package",
                      help="Restrict test to a specific java package")
    parser.add_option("-z", "--size", dest="test_size",
                      help="Restrict test to a specific test size")
    parser.add_option("--annotation", dest="test_annotation",
                      help="Include only those tests tagged with a specific"
                      " annotation")
    parser.add_option("--not-annotation", dest="test_not_annotation",
                      help="Exclude any tests tagged with a specific"
                      " annotation")
    parser.add_option("-u", "--user-tests-file", dest="user_tests_file",
                      metavar="FILE", default=user_test_default,
                      help="Alternate source of user test definitions")
    parser.add_option("-o", "--coverage", dest="coverage",
                      default=False, action="store_true",
                      help="Generate code coverage metrics for test(s)")
    parser.add_option("--coverage-target", dest="coverage_target_path",
                      default=None,
                      help="Path to app to collect code coverage target data for.")
    parser.add_option("-k", "--skip-permissions", dest="skip_permissions",
                      default=False, action="store_true",
                      help="Do not grant runtime permissions during test package"
                      " installation.")
    parser.add_option("-x", "--path", dest="test_path",
                      help="Run test(s) at given file system path")
    parser.add_option("-t", "--all-tests", dest="all_tests",
                      default=False, action="store_true",
                      help="Run all defined tests")
    parser.add_option("--continuous", dest="continuous_tests",
                      default=False, action="store_true",
                      help="Run all tests defined as part of the continuous "
                      "test set")
    parser.add_option("--timeout", dest="timeout",
                      default=300, help="Set a timeout limit (in sec) for "
                      "running native tests on a device (default: 300 secs)")
    parser.add_option("--suite", dest="suite",
                      help="Run all tests defined as part of the "
                      "the given test suite")
    parser.add_option("--user", dest="user",
                      help="The user that test apks are installing to."
                      " This is the integer user id, e.g. 0 or 10."
                      " If no user is specified, apk will be installed with"
                      " adb's default behavior, which is currently all users.")
    parser.add_option("--install-filter", dest="filter_re",
                      help="Regular expression which generated apks have to"
                      " match to be installed to target device. Default is None"
                      " and will install all packages built.  This is"
                      " useful when the test path has a lot of apks but you"
                      " only care about one.")
    group = optparse.OptionGroup(
        parser, "Targets", "Use these options to direct tests to a specific "
        "Android target")
    group.add_option("-e", "--emulator", dest="emulator", default=False,
                     action="store_true", help="use emulator")
    group.add_option("-d", "--device", dest="device", default=False,
                     action="store_true", help="use device")
    group.add_option("-s", "--serial", dest="serial",
                     help="use specific serial")
    parser.add_option_group(group)
    self._options, self._test_args = parser.parse_args()

    if (not self._options.only_list_tests
        and not self._options.all_tests
        and not self._options.continuous_tests
        and not self._options.suite
        and not self._options.test_path
        and len(self._test_args) < 1):
      parser.print_help()
      logger.SilentLog("at least one test name must be specified")
      raise errors.AbortError

    self._adb = adb_interface.AdbInterface()
    if self._options.emulator:
      self._adb.SetEmulatorTarget()
    elif self._options.device:
      self._adb.SetDeviceTarget()
    elif self._options.serial is not None:
      self._adb.SetTargetSerial(self._options.serial)
    if self._options.verbose:
      logger.SetVerbose(True)

    if self._options.coverage_target_path:
      self._options.coverage = True

    self._known_tests = self._ReadTests()

    self._options.host_lib_path = android_build.GetHostLibraryPath()
    self._options.test_data_path = android_build.GetTestAppPath()

  def _ReadTests(self):
    """Parses the set of test definition data.

    Returns:
      A TestDefinitions object that contains the set of parsed tests.
    Raises:
      AbortError: If a fatal error occurred when parsing the tests.
    """
    try:
      known_tests = test_defs.TestDefinitions()
      # only read tests when not in path mode
      if not self._options.test_path:
        core_test_path = os.path.join(self._root_path, self._CORE_TEST_PATH)
        if os.path.isfile(core_test_path):
          known_tests.Parse(core_test_path)
        # read all <android root>/vendor/*/tests/testinfo/test_defs.xml paths
        vendor_tests_pattern = os.path.join(self._root_path,
                                            self._VENDOR_TEST_PATH)
        test_file_paths = glob.glob(vendor_tests_pattern)
        for test_file_path in test_file_paths:
          known_tests.Parse(test_file_path)
        if os.path.isfile(self._options.user_tests_file):
          known_tests.Parse(self._options.user_tests_file)
      return known_tests
    except errors.ParseError:
      raise errors.AbortError

  def _DumpTests(self):
    """Prints out set of defined tests."""
    print "The following tests are currently defined:\n"
    print "%-25s %-40s %s" % ("name", "build path", "description")
    print "-" * 80
    for test in self._known_tests:
      print "%-25s %-40s %s" % (test.GetName(), test.GetBuildPath(),
                                test.GetDescription())
    print "\nSee %s for more information" % self._TEST_FILE_NAME

  def _DoBuild(self):
    logger.SilentLog("Building tests...")
    tests = self._GetTestsToRun()

    # Build and install tests that do not get granted permissions
    self._DoPermissionAwareBuild(tests, False)

    # Build and install tests that require granted permissions
    self._DoPermissionAwareBuild(tests, True)

  def _DoPermissionAwareBuild(self, tests, test_requires_permissions):
    # turn off dalvik verifier if necessary
    # TODO: skip turning off verifier for now, since it puts device in bad
    # state b/14088982
    #self._TurnOffVerifier(tests)
    self._DoFullBuild(tests, test_requires_permissions)

    target_tree = make_tree.MakeTree()

    extra_args_set = []
    for test_suite in tests:
      if test_suite.IsGrantedPermissions() == test_requires_permissions:
        self._AddBuildTarget(test_suite, target_tree, extra_args_set)

    if not self._options.preview:
      self._adb.EnableAdbRoot()
    else:
      logger.Log("adb root")

    if not target_tree.IsEmpty():
      if self._options.coverage:
        coverage.EnableCoverageBuild()
        target_tree.AddPath("external/emma")

      target_list = target_tree.GetPrunedMakeList()
      target_dir_list = [re.sub(r'Android[.]mk$', r'', i) for i in target_list]
      target_build_string = " ".join(target_list)
      target_dir_build_string = " ".join(target_dir_list)
      extra_args_string = " ".join(extra_args_set)

      install_path_goals = []
      mmma_goals = []
      for d in target_dir_list:
        if d.startswith("./"):
          d = d[2:]
        if d.endswith("/"):
          d = d[:-1]
        install_path_goals.append("GET-INSTALL-PATH-IN-" + d.replace("/","-"))
        mmma_goals.append("MODULES-IN-" + d.replace("/","-"))
      # mmm cannot be used from python, so perform a similar operation using
      # ONE_SHOT_MAKEFILE
      cmd = 'ONE_SHOT_MAKEFILE="%s" make -j%s -C "%s" %s %s %s' % (
          target_build_string, self._options.make_jobs, self._root_path,
          " ".join(install_path_goals), " ".join(mmma_goals), extra_args_string)
      # mmma cannot be used from python, so perform a similar operation
      alt_cmd = 'make -j%s -C "%s" -f build/core/main.mk %s %s' % (
              self._options.make_jobs, self._root_path, extra_args_string, " ".join(mmma_goals))

      logger.Log(cmd)
      if not self._options.preview:
        run_command.SetAbortOnError()
        try:
          output = run_command.RunCommand(cmd, return_output=True, timeout_time=600)
          ## Chances are this failed because it didn't build the dependencies
        except errors.AbortError:
          logger.Log("make failed. Trying to rebuild all dependencies.")
          logger.Log("mmma -j%s %s" %(self._options.make_jobs, target_dir_build_string))
          # Try again with mma equivalent, which will build the dependencies
          run_command.RunCommand(alt_cmd, return_output=False, timeout_time=600)
          # Run mmm again to get the install paths only
          output = run_command.RunCommand(cmd, return_output=True, timeout_time=600)
        run_command.SetAbortOnError(False)
        logger.SilentLog(output)
        filter_re = re.compile(self._options.filter_re) if self._options.filter_re else None

        self._DoInstall(output, test_requires_permissions, filter_re=filter_re)

  def _DoInstall(self, make_output, test_requires_permissions, filter_re=None):
    """Install artifacts from build onto device.

    Looks for 'install:' text from make output to find artifacts to install.

    Files with the .apk extension get 'adb install'ed, all other files
    get 'adb push'ed onto the device.

    Args:
      make_output: stdout from make command
    """
    for line in make_output.split("\n"):
      m = self._RE_MAKE_INSTALL.match(line)
      if m:
        # strip the 'INSTALL: <name>' from the left hand side
        # the remaining string is a space-separated list of build-generated files
        install_paths = m.group(2)
        for install_path in re.split(r'\s+', install_paths):
          if filter_re and not filter_re.match(install_path):
            continue
          if install_path.endswith(".apk"):
            abs_install_path = os.path.join(self._root_path, install_path)
            extra_flags = ""
            if test_requires_permissions and not self._options.skip_permissions:
              extra_flags = "-g"
            if self._options.user:
              extra_flags += " --user " + self._options.user
            logger.Log("adb install -r %s %s" % (extra_flags, abs_install_path))
            logger.Log(self._adb.Install(abs_install_path, extra_flags))
          else:
            self._PushInstallFileToDevice(install_path)

  def _PushInstallFileToDevice(self, install_path):
    m = self._re_make_install_path.match(install_path)
    if m:
      remote_path = m.group(1)
      remote_dir = os.path.dirname(remote_path)
      logger.Log("adb shell mkdir -p %s" % remote_dir)
      self._adb.SendShellCommand("mkdir -p %s" % remote_dir)
      abs_install_path = os.path.join(self._root_path, install_path)
      logger.Log("adb push %s %s" % (abs_install_path, remote_path))
      self._adb.Push(abs_install_path, remote_path)
    else:
      logger.Log("Error: Failed to recognize path of file to install %s" % install_path)

  def _DoFullBuild(self, tests, test_requires_permissions):
    """If necessary, run a full 'make' command for the tests that need it."""
    extra_args_set = Set()

    for test in tests:
      if test.IsFullMake() and test.IsGrantedPermissions() == test_requires_permissions:
        if test.GetExtraBuildArgs():
          # extra args contains the args to pass to 'make'
          extra_args_set.add(test.GetExtraBuildArgs())
        else:
          logger.Log("Warning: test %s needs a full build but does not specify"
                     " extra_build_args" % test.GetName())

    # check if there is actually any tests that required a full build
    if extra_args_set:
      cmd = ('make -j%s %s' % (self._options.make_jobs,
                               ' '.join(list(extra_args_set))))
      logger.Log(cmd)
      if not self._options.preview:
        old_dir = os.getcwd()
        os.chdir(self._root_path)
        output = run_command.RunCommand(cmd, return_output=True)
        logger.SilentLog(output)
        os.chdir(old_dir)
        self._DoInstall(output, test_requires_permissions)

  def _AddBuildTarget(self, test_suite, target_tree, extra_args_set):
    if not test_suite.IsFullMake():
      build_dir = test_suite.GetBuildPath()
      if self._AddBuildTargetPath(build_dir, target_tree):
        extra_args_set.append(test_suite.GetExtraBuildArgs())
      for path in test_suite.GetBuildDependencies(self._options):
        self._AddBuildTargetPath(path, target_tree)

  def _AddBuildTargetPath(self, build_dir, target_tree):
    if build_dir is not None:
      target_tree.AddPath(build_dir)
      return True
    return False

  def _GetTestsToRun(self):
    """Get a list of TestSuite objects to run, based on command line args."""
    if self._tests_to_run:
      return self._tests_to_run

    self._tests_to_run = []
    if self._options.all_tests:
      self._tests_to_run = self._known_tests.GetTests()
    elif self._options.continuous_tests:
      self._tests_to_run = self._known_tests.GetContinuousTests()
    elif self._options.suite:
      self._tests_to_run = \
          self._known_tests.GetTestsInSuite(self._options.suite)
    elif self._options.test_path:
      walker = test_walker.TestWalker()
      self._tests_to_run = walker.FindTests(self._options.test_path)

    for name in self._test_args:
      test = self._known_tests.GetTest(name)
      if test is None:
        logger.Log("Error: Could not find test %s" % name)
        self._DumpTests()
        raise errors.AbortError
      self._tests_to_run.append(test)
    return self._tests_to_run

  def _TurnOffVerifier(self, test_list):
    """Turn off the dalvik verifier if needed by given tests.

    If one or more tests needs dalvik verifier off, and it is not already off,
    turns off verifier and reboots device to allow change to take effect.
    """
    # hack to check if these are frameworks/base tests. If so, turn off verifier
    # to allow framework tests to access private/protected/package-private framework api
    framework_test = False
    for test in test_list:
      if os.path.commonprefix([test.GetBuildPath(), "frameworks/base"]):
        framework_test = True
    if framework_test:
      # check if verifier is off already - to avoid the reboot if not
      # necessary
      output = self._adb.SendShellCommand("cat /data/local.prop")
      if not self._DALVIK_VERIFIER_OFF_PROP in output:

        # Read the existing dalvik verifier flags.
        old_prop_value = self._adb.SendShellCommand("getprop %s" \
            %(self._DALVIK_VERIFIER_PROP))
        old_prop_value = old_prop_value.strip() if old_prop_value else ""

        # Append our verifier flags to existing flags
        new_prop_value = "%s %s" %(self._DALVIK_VERIFIER_OFF_VALUE, old_prop_value)

        # Update property now, as /data/local.prop is not read until reboot
        logger.Log("adb shell setprop %s '%s'" \
            %(self._DALVIK_VERIFIER_PROP, new_prop_value))
        if not self._options.preview:
          self._adb.SendShellCommand("setprop %s '%s'" \
            %(self._DALVIK_VERIFIER_PROP, new_prop_value))

        # Write prop to /data/local.prop
        # Every time device is booted, it will pick up this value
        new_prop_assignment = "%s = %s" %(self._DALVIK_VERIFIER_PROP, new_prop_value)
        if self._options.preview:
          logger.Log("adb shell \"echo %s >> /data/local.prop\""
                     % new_prop_assignment)
          logger.Log("adb shell chmod 644 /data/local.prop")
        else:
          logger.Log("Turning off dalvik verifier and rebooting")
          self._adb.SendShellCommand("\"echo %s >> /data/local.prop\""
                                     % new_prop_assignment)

        # Reset runtime so that dalvik picks up new verifier flags from prop
        self._ChmodRuntimeReset()
      elif not self._options.preview:
        # check the permissions on the file
        permout = self._adb.SendShellCommand("ls -l /data/local.prop")
        if not "-rw-r--r--" in permout:
          logger.Log("Fixing permissions on /data/local.prop and rebooting")
          self._ChmodRuntimeReset()

  def _ChmodRuntimeReset(self):
    """Perform a chmod of /data/local.prop and reset the runtime.
    """
    logger.Log("adb shell chmod 644 /data/local.prop ## u+w,a+r")
    if not self._options.preview:
      self._adb.SendShellCommand("chmod 644 /data/local.prop")

    self._adb.RuntimeReset(preview_only=self._options.preview)

    if not self._options.preview:
      self._adb.EnableAdbRoot()


  def RunTests(self):
    """Main entry method - executes the tests according to command line args."""
    try:
      run_command.SetAbortOnError()
      self._ProcessOptions()
      if self._options.only_list_tests:
        self._DumpTests()
        return

      if not self._options.skip_build:
        self._DoBuild()

      if self._options.build_install_only:
        logger.Log("Skipping test execution (due to --build-install-only flag)")
        return

      for test_suite in self._GetTestsToRun():
        try:
          test_suite.Run(self._options, self._adb)
        except errors.WaitForResponseTimedOutError:
          logger.Log("Timed out waiting for response")

    except KeyboardInterrupt:
      logger.Log("Exiting...")
    except errors.AbortError, error:
      logger.Log(error.msg)
      logger.SilentLog("Exiting due to AbortError...")
    except errors.WaitForResponseTimedOutError:
      logger.Log("Timed out waiting for response")


def RunTests():
  runner = TestRunner()
  runner.RunTests()

if __name__ == "__main__":
  RunTests()
