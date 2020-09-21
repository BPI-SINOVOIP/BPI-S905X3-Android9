#!/usr/bin/env python
#
# Copyright 2017, The Android Open Source Project
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

"""
Command line utility for running Android tests through TradeFederation.

atest helps automate the flow of building test modules across the Android
code base and executing the tests via the TradeFederation test harness.

atest is designed to support any test types that can be ran by TradeFederation.
"""

import logging
import os
import subprocess
import sys
import tempfile
import time

import atest_error
import atest_utils
import cli_translator
# pylint: disable=import-error
import constants
import module_info
import test_runner_handler
from test_runners import regression_test_runner

EXPECTED_VARS = frozenset([
    constants.ANDROID_BUILD_TOP,
    'ANDROID_TARGET_OUT_TESTCASES',
    constants.ANDROID_OUT])
BUILD_STEP = 'build'
INSTALL_STEP = 'install'
TEST_STEP = 'test'
ALL_STEPS = [BUILD_STEP, INSTALL_STEP, TEST_STEP]
TEST_RUN_DIR_PREFIX = 'atest_run_%s_'
HELP_DESC = '''Build, install and run Android tests locally.'''
REBUILD_MODULE_INFO_FLAG = '--rebuild-module-info'
CUSTOM_ARG_FLAG = '--'

EPILOG_TEXT = '''


- - - - - - - - -
IDENTIFYING TESTS
- - - - - - - - -

    The positional argument <tests> should be a reference to one or more
    of the tests you'd like to run. Multiple tests can be run in one command by
    separating test references with spaces.

    Usage template: atest <reference_to_test_1> <reference_to_test_2>

    A <reference_to_test> can be satisfied by the test's MODULE NAME,
    MODULE:CLASS, CLASS NAME, TF INTEGRATION TEST, FILE PATH or PACKAGE NAME.
    Explanations and examples of each follow.


    < MODULE NAME >

        Identifying a test by its module name will run the entire module. Input
        the name as it appears in the LOCAL_MODULE or LOCAL_PACKAGE_NAME
        variables in that test's Android.mk or Android.bp file.

        Note: Use < TF INTEGRATION TEST > to run non-module tests integrated
        directly into TradeFed.

        Examples:
            atest FrameworksServicesTests
            atest CtsJankDeviceTestCases


    < MODULE:CLASS >

        Identifying a test by its class name will run just the tests in that
        class and not the whole module. MODULE:CLASS is the preferred way to run
        a single class. MODULE is the same as described above. CLASS is the
        name of the test class in the .java file. It can either be the fully
        qualified class name or just the basic name.

        Examples:
            atest FrameworksServicesTests:ScreenDecorWindowTests
            atest FrameworksServicesTests:com.android.server.wm.ScreenDecorWindowTests
            atest CtsJankDeviceTestCases:CtsDeviceJankUi


    < CLASS NAME >

        A single class can also be run by referencing the class name without
        the module name.

        Examples:
            atest ScreenDecorWindowTests
            atest CtsDeviceJankUi

        However, this will take more time than the equivalent MODULE:CLASS
        reference, so we suggest using a MODULE:CLASS reference whenever
        possible. Examples below are ordered by performance from the fastest
        to the slowest:

        Examples:
            atest FrameworksServicesTests:com.android.server.wm.ScreenDecorWindowTests
            atest FrameworksServicesTests:ScreenDecorWindowTests
            atest ScreenDecorWindowTests

    < TF INTEGRATION TEST >

        To run tests that are integrated directly into TradeFed (non-modules),
        input the name as it appears in the output of the "tradefed.sh list
        configs" cmd.

        Examples:
           atest example/reboot
           atest native-benchmark


    < FILE PATH >

        Both module-based tests and integration-based tests can be run by
        inputting the path to their test file or dir as appropriate. A single
        class can also be run by inputting the path to the class's java file.
        Both relative and absolute paths are supported.

        Example - 2 ways to run the `CtsJankDeviceTestCases` module via path:
        1. run module from android <repo root>:
            atest cts/tests/jank/jank

        2. from <android root>/cts/tests/jank:
            atest .

        Example - run a specific class within CtsJankDeviceTestCases module
        from <android repo> root via path:
           atest cts/tests/jank/src/android/jank/cts/ui/CtsDeviceJankUi.java

        Example - run an integration test from <android repo> root via path:
           atest tools/tradefederation/contrib/res/config/example/reboot.xml


    < PACKAGE NAME >

        Atest supports searching tests from package name as well.

        Examples:
           atest com.android.server.wm
           atest android.jank.cts


- - - - - - - - - - - - - - - - - - - - - - - - - -
SPECIFYING INDIVIDUAL STEPS: BUILD, INSTALL OR RUN
- - - - - - - - - - - - - - - - - - - - - - - - - -

    The -b, -i and -t options allow you to specify which steps you want to run.
    If none of those options are given, then all steps are run. If any of these
    options are provided then only the listed steps are run.

    Note: -i alone is not currently support and can only be included with -t.
    Both -b and -t can be run alone.

    Examples:
        atest -b <test>    (just build targets)
        atest -t <test>    (run tests only)
        atest -it <test>   (install apk and run tests)
        atest -bt <test>   (build targets, run tests, but skip installing apk)


    Atest now has the ability to force a test to skip its cleanup/teardown step.
    Many tests, e.g. CTS, cleanup the device after the test is run, so trying to
    rerun your test with -t will fail without having the --disable-teardown
    parameter. Use -d before -t to skip the test clean up step and test iteratively.

        atest -d <test>    (disable installing apk and cleanning up device)
        atest -t <test>

    Note that -t disables both setup/install and teardown/cleanup of the
    device. So you can continue to rerun your test with just

        atest -t <test>

    as many times as you want.


- - - - - - - - - - - - -
RUNNING SPECIFIC METHODS
- - - - - - - - - - - - -

    It is possible to run only specific methods within a test class. To run
    only specific methods, identify the class in any of the ways supported for
    identifying a class (MODULE:CLASS, FILE PATH, etc) and then append the
    name of the method or method using the following template:

      <reference_to_class>#<method1>

    Multiple methods can be specified with commas:

      <reference_to_class>#<method1>,<method2>,<method3>...

    Examples:
      atest com.android.server.wm.ScreenDecorWindowTests#testMultipleDecors

      atest FrameworksServicesTests:ScreenDecorWindowTests#testFlagChange,testRemoval


- - - - - - - - - - - - -
RUNNING MULTIPLE CLASSES
- - - - - - - - - - - - -

    To run multiple classes, deliminate them with spaces just like you would
    when running multiple tests.  Atest will handle building and running
    classes in the most efficient way possible, so specifying a subset of
    classes in a module will improve performance over running the whole module.


    Examples:
    - two classes in same module:
      atest FrameworksServicesTests:ScreenDecorWindowTests FrameworksServicesTests:DimmerTests

    - two classes, different modules:
      atest FrameworksServicesTests:ScreenDecorWindowTests CtsJankDeviceTestCases:CtsDeviceJankUi


- - - - - - - - - - -
REGRESSION DETECTION
- - - - - - - - - - -

    Generate pre-patch or post-patch metrics without running regression detection:

    Example:
        atest <test> --generate-baseline <optional iter>
        atest <test> --generate-new-metrics <optional iter>

    Local regression detection can be run in three options:

    1) Provide a folder containing baseline (pre-patch) metrics (generated
       previously). Atest will run the tests n (default 5) iterations, generate
       a new set of post-patch metrics, and compare those against existing metrics.

    Example:
        atest <test> --detect-regression </path/to/baseline> --generate-new-metrics <optional iter>

    2) Provide a folder containing post-patch metrics (generated previously).
       Atest will run the tests n (default 5) iterations, generate a new set of
       pre-patch metrics, and compare those against those provided. Note: the
       developer needs to revert the device/tests to pre-patch state to generate
       baseline metrics.

    Example:
        atest <test> --detect-regression </path/to/new> --generate-baseline <optional iter>

    3) Provide 2 folders containing both pre-patch and post-patch metrics. Atest
       will run no tests but the regression detection algorithm.

    Example:
        atest --detect-regression </path/to/baseline> </path/to/new>


'''


def _parse_args(argv):
    """Parse command line arguments.

    Args:
        argv: A list of arguments.

    Returns:
        An argspace.Namespace class instance holding parsed args.
    """
    import argparse
    parser = argparse.ArgumentParser(
        description=HELP_DESC,
        epilog=EPILOG_TEXT,
        formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('tests', nargs='*', help='Tests to build and/or run.')
    parser.add_argument('-b', '--build', action='append_const', dest='steps',
                        const=BUILD_STEP, help='Run a build.')
    parser.add_argument('-i', '--install', action='append_const', dest='steps',
                        const=INSTALL_STEP, help='Install an APK.')
    parser.add_argument('-t', '--test', action='append_const', dest='steps',
                        const=TEST_STEP,
                        help='Run the tests. WARNING: Many test configs force cleanup '
                        'of device after test run. In this case, -d must be used in previous '
                        'test run to disable cleanup, for -t to work. Otherwise, '
                        'device will need to be setup again with -i.')
    parser.add_argument('-s', '--serial',
                        help='The device to run the test on.')
    parser.add_argument('-d', '--disable-teardown', action='store_true',
                        help='Disables test teardown and cleanup.')
    parser.add_argument('-m', REBUILD_MODULE_INFO_FLAG, action='store_true',
                        help='Forces a rebuild of the module-info.json file. '
                             'This may be necessary following a repo sync or '
                             'when writing a new test.')
    parser.add_argument('-w', '--wait-for-debugger', action='store_true',
                        help='Only for instrumentation tests. Waits for '
                             'debugger prior to execution.')
    parser.add_argument('-v', '--verbose', action='store_true',
                        help='Display DEBUG level logging.')
    parser.add_argument('--generate-baseline', nargs='?', type=int, const=5, default=0,
                        help='Generate baseline metrics, run 5 iterations by default. '
                             'Provide an int argument to specify # iterations.')
    parser.add_argument('--generate-new-metrics', nargs='?', type=int, const=5, default=0,
                        help='Generate new metrics, run 5 iterations by default. '
                             'Provide an int argument to specify # iterations.')
    parser.add_argument('--detect-regression', nargs='*',
                        help='Run regression detection algorithm. Supply '
                             'path to baseline and/or new metrics folders.')
    # This arg actually doesn't consume anything, it's primarily used for the
    # help description and creating custom_args in the NameSpace object.
    parser.add_argument('--', dest='custom_args', nargs='*',
                        help='Specify custom args for the test runners. '
                             'Everything after -- will be consumed as custom '
                             'args.')
    # Store everything after '--' in custom_args.
    pruned_argv = argv
    custom_args_index = None
    if CUSTOM_ARG_FLAG in argv:
        custom_args_index = argv.index(CUSTOM_ARG_FLAG)
        pruned_argv = argv[:custom_args_index]
    args = parser.parse_args(pruned_argv)
    args.custom_args = []
    if custom_args_index is not None:
        args.custom_args = argv[custom_args_index+1:]
    return args


def _configure_logging(verbose):
    """Configure the logger.

    Args:
        verbose: A boolean. If true display DEBUG level logs.
    """
    if verbose:
        logging.basicConfig(level=logging.DEBUG)
    else:
        logging.basicConfig(level=logging.INFO)


def _missing_environment_variables():
    """Verify the local environment has been set up to run atest.

    Returns:
        List of strings of any missing environment variables.
    """
    missing = filter(None, [x for x in EXPECTED_VARS if not os.environ.get(x)])
    if missing:
        logging.error('Local environment doesn\'t appear to have been '
                      'initialized. Did you remember to run lunch? Expected '
                      'Environment Variables: %s.', missing)
    return missing


def make_test_run_dir():
    """Make the test run dir in tmp.

    Returns:
        A string of the dir path.
    """
    utc_epoch_time = int(time.time())
    prefix = TEST_RUN_DIR_PREFIX % utc_epoch_time
    return tempfile.mkdtemp(prefix=prefix)


def run_tests(run_commands):
    """Shell out and execute tradefed run commands.

    Args:
        run_commands: A list of strings of Tradefed run commands.
    """
    logging.info('Running tests')
    # TODO: Build result parser for run command. Until then display raw stdout.
    for run_command in run_commands:
        logging.debug('Executing command: %s', run_command)
        subprocess.check_call(run_command, shell=True, stderr=subprocess.STDOUT)


def get_extra_args(args):
    """Get extra args for test runners.

    Args:
        args: arg parsed object.

    Returns:
        Dict of extra args for test runners to utilize.
    """
    extra_args = {}
    if args.wait_for_debugger:
        extra_args[constants.WAIT_FOR_DEBUGGER] = None
    steps = args.steps or ALL_STEPS
    if INSTALL_STEP not in steps:
        extra_args[constants.DISABLE_INSTALL] = None
    if args.disable_teardown:
        extra_args[constants.DISABLE_TEARDOWN] = args.disable_teardown
    if args.generate_baseline:
        extra_args[constants.PRE_PATCH_ITERATIONS] = args.generate_baseline
    if args.serial:
        extra_args[constants.SERIAL] = args.serial
    if args.generate_new_metrics:
        extra_args[constants.POST_PATCH_ITERATIONS] = args.generate_new_metrics
    if args.custom_args:
        extra_args[constants.CUSTOM_ARGS] = args.custom_args
    return extra_args


def _get_regression_detection_args(args, results_dir):
    """Get args for regression detection test runners.

    Args:
        args: parsed args object.
        results_dir: string directory to store atest results.

    Returns:
        Dict of args for regression detection test runner to utilize.
    """
    regression_args = {}
    pre_patch_folder = (os.path.join(results_dir, 'baseline-metrics') if args.generate_baseline
                        else args.detect_regression.pop(0))
    post_patch_folder = (os.path.join(results_dir, 'new-metrics') if args.generate_new_metrics
                         else args.detect_regression.pop(0))
    regression_args[constants.PRE_PATCH_FOLDER] = pre_patch_folder
    regression_args[constants.POST_PATCH_FOLDER] = post_patch_folder
    return regression_args


def _will_run_tests(args):
    """Determine if there are tests to run.

    Currently only used by detect_regression to skip the test if just running regression detection.

    Args:
        args: parsed args object.

    Returns:
        True if there are tests to run, false otherwise.
    """
    return not (args.detect_regression and len(args.detect_regression) == 2)


def _has_valid_regression_detection_args(args):
    """Validate regression detection args.

    Args:
        args: parsed args object.

    Returns:
        True if args are valid
    """
    if args.generate_baseline and args.generate_new_metrics:
        logging.error('Cannot collect both baseline and new metrics at the same time.')
        return False
    if args.detect_regression is not None:
        if not args.detect_regression:
            logging.error('Need to specify at least 1 arg for regression detection.')
            return False
        elif len(args.detect_regression) == 1:
            if args.generate_baseline or args.generate_new_metrics:
                return True
            logging.error('Need to specify --generate-baseline or --generate-new-metrics.')
            return False
        elif len(args.detect_regression) == 2:
            if args.generate_baseline:
                logging.error('Specified 2 metric paths and --generate-baseline, '
                              'either drop --generate-baseline or drop a path')
                return False
            if args.generate_new_metrics:
                logging.error('Specified 2 metric paths and --generate-new-metrics, '
                              'either drop --generate-new-metrics or drop a path')
                return False
            return True
        else:
            logging.error('Specified more than 2 metric paths.')
            return False
    return True


def main(argv):
    """Entry point of atest script.

    Args:
        argv: A list of arguments.

    Returns:
        Exit code.
    """
    args = _parse_args(argv)
    _configure_logging(args.verbose)
    if _missing_environment_variables():
        return constants.EXIT_CODE_ENV_NOT_SETUP
    if args.generate_baseline and args.generate_new_metrics:
        logging.error('Cannot collect both baseline and new metrics at the same time.')
        return constants.EXIT_CODE_ERROR
    if not _has_valid_regression_detection_args(args):
        return constants.EXIT_CODE_ERROR
    results_dir = make_test_run_dir()
    mod_info = module_info.ModuleInfo(force_build=args.rebuild_module_info)
    translator = cli_translator.CLITranslator(module_info=mod_info)
    build_targets = set()
    test_infos = set()
    if _will_run_tests(args):
        try:
            build_targets, test_infos = translator.translate(args.tests)
        except atest_error.TestDiscoveryException:
            logging.exception('Error occured in test discovery:')
            logging.info('This can happen after a repo sync or if the test is '
                         'new. Running: with "%s"  may resolve the issue.',
                         REBUILD_MODULE_INFO_FLAG)
            return constants.EXIT_CODE_TEST_NOT_FOUND
    build_targets |= test_runner_handler.get_test_runner_reqs(mod_info,
                                                              test_infos)
    extra_args = get_extra_args(args)
    if args.detect_regression:
        build_targets |= (regression_test_runner.RegressionTestRunner('')
                          .get_test_runner_build_reqs())
    # args.steps will be None if none of -bit set, else list of params set.
    steps = args.steps if args.steps else ALL_STEPS
    if build_targets and BUILD_STEP in steps:
        # Add module-info.json target to the list of build targets to keep the
        # file up to date.
        build_targets.add(mod_info.module_info_target)
        success = atest_utils.build(build_targets, args.verbose)
        if not success:
            return constants.EXIT_CODE_BUILD_FAILURE
    elif TEST_STEP not in steps:
        logging.warn('Install step without test step currently not '
                     'supported, installing AND testing instead.')
        steps.append(TEST_STEP)
    if TEST_STEP in steps:
        test_runner_handler.run_all_tests(results_dir, test_infos, extra_args)
    if args.detect_regression:
        regression_args = _get_regression_detection_args(args, results_dir)
        regression_test_runner.RegressionTestRunner('').run_tests(None, regression_args)
    return constants.EXIT_CODE_SUCCESS

if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
