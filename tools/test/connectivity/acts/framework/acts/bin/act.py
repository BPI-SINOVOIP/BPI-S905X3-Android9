#!/usr/bin/env python3.4
#
#   Copyright 2016 - The Android Open Source Project
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

from builtins import str

import argparse
import multiprocessing
import signal
import sys
import traceback

from acts import config_parser
from acts import keys
from acts import signals
from acts import test_runner


def _run_test(parsed_config, test_identifiers, repeat=1):
    """Instantiate and runs test_runner.TestRunner.

    This is the function to start separate processes with.

    Args:
        parsed_config: A dict that is a set of configs for one
                       test_runner.TestRunner.
        test_identifiers: A list of tuples, each identifies what test case to
                          run on what test class.
        repeat: Number of times to iterate the specified tests.

    Returns:
        True if all tests passed without any error, False otherwise.
    """
    runner = _create_test_runner(parsed_config, test_identifiers)
    try:
        for i in range(repeat):
            runner.run()
        return runner.results.is_all_pass
    except signals.TestAbortAll:
        return True
    except:
        print("Exception when executing %s, iteration %s." %
              (runner.testbed_name, i))
        print(traceback.format_exc())
    finally:
        runner.stop()


def _create_test_runner(parsed_config, test_identifiers):
    """Instantiates one test_runner.TestRunner object and register termination
    signal handlers that properly shut down the test_runner.TestRunner run.

    Args:
        parsed_config: A dict that is a set of configs for one
                       test_runner.TestRunner.
        test_identifiers: A list of tuples, each identifies what test case to
                          run on what test class.

    Returns:
        A test_runner.TestRunner object.
    """
    try:
        t = test_runner.TestRunner(parsed_config, test_identifiers)
    except:
        print("Failed to instantiate test runner, abort.")
        print(traceback.format_exc())
        sys.exit(1)
    # Register handler for termination signals.
    handler = config_parser.gen_term_signal_handler([t])
    signal.signal(signal.SIGTERM, handler)
    signal.signal(signal.SIGINT, handler)
    return t


def _run_tests_parallel(parsed_configs, test_identifiers, repeat):
    """Executes requested tests in parallel.

    Each test run will be in its own process.

    Args:
        parsed_configs: A list of dicts, each is a set of configs for one
                        test_runner.TestRunner.
        test_identifiers: A list of tuples, each identifies what test case to
                          run on what test class.
        repeat: Number of times to iterate the specified tests.

    Returns:
        True if all test runs executed successfully, False otherwise.
    """
    print("Executing {} concurrent test runs.".format(len(parsed_configs)))
    arg_list = [(c, test_identifiers, repeat) for c in parsed_configs]
    results = []
    with multiprocessing.Pool(processes=len(parsed_configs)) as pool:
        # Can't use starmap for py2 compatibility. One day, one day......
        for args in arg_list:
            results.append(pool.apply_async(_run_test, args))
        pool.close()
        pool.join()
    for r in results:
        if r.get() is False or isinstance(r, Exception):
            return False


def _run_tests_sequential(parsed_configs, test_identifiers, repeat):
    """Executes requested tests sequentially.

    Requested test runs will commence one after another according to the order
    of their corresponding configs.

    Args:
        parsed_configs: A list of dicts, each is a set of configs for one
                        test_runner.TestRunner.
        test_identifiers: A list of tuples, each identifies what test case to
                          run on what test class.
        repeat: Number of times to iterate the specified tests.

    Returns:
        True if all test runs executed successfully, False otherwise.
    """
    ok = True
    for c in parsed_configs:
        try:
            ret = _run_test(c, test_identifiers, repeat)
            ok = ok and ret
        except Exception as e:
            print("Exception occurred when executing test bed %s. %s" %
                  (c[keys.Config.key_testbed.value], e))
    return ok


def main(argv):
    """This is a sample implementation of a cli entry point for ACTS test
    execution.

    Or you could implement your own cli entry point using acts.config_parser
    functions and acts.test_runner.execute_one_test_class.
    """
    parser = argparse.ArgumentParser(
        description=("Specify tests to run. If nothing specified, "
                     "run all test cases found."))
    parser.add_argument(
        '-c',
        '--config',
        nargs=1,
        type=str,
        required=True,
        metavar="<PATH>",
        help="Path to the test configuration file.")
    parser.add_argument(
        '--test_args',
        nargs='+',
        type=str,
        metavar="Arg1 Arg2 ...",
        help=("Command-line arguments to be passed to every test case in a "
              "test run. Use with caution."))
    parser.add_argument(
        '-p',
        '--parallel',
        action="store_true",
        help=("If set, tests will be executed on all testbeds in parallel. "
              "Otherwise, tests are executed iteratively testbed by testbed."))
    parser.add_argument(
        '-ci',
        '--campaign_iterations',
        metavar="<CAMPAIGN_ITERATIONS>",
        nargs='?',
        type=int,
        const=1,
        default=1,
        help="Number of times to run the campaign or a group of test cases.")
    parser.add_argument(
        '-tb',
        '--testbed',
        nargs='+',
        type=str,
        metavar="[<TEST BED NAME1> <TEST BED NAME2> ...]",
        help="Specify which test beds to run tests on.")
    parser.add_argument(
        '-lp',
        '--logpath',
        type=str,
        metavar="<PATH>",
        help="Root path under which all logs will be placed.")
    parser.add_argument(
        '-tp',
        '--testpaths',
        nargs='*',
        type=str,
        metavar="<PATH> <PATH>",
        help="One or more non-recursive test class search paths.")

    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument(
        '-tc',
        '--testclass',
        nargs='+',
        type=str,
        metavar="[TestClass1 TestClass2:test_xxx ...]",
        help="A list of test classes/cases to run.")
    group.add_argument(
        '-tf',
        '--testfile',
        nargs=1,
        type=str,
        metavar="<PATH>",
        help=("Path to a file containing a comma delimited list of test "
              "classes to run."))
    parser.add_argument(
        '-r',
        '--random',
        action="store_true",
        help="If set, tests will be executed in random order.")
    parser.add_argument(
        '-ti',
        '--test_case_iterations',
        metavar="<TEST_CASE_ITERATIONS>",
        nargs='?',
        type=int,
        help="Number of times to run every test case.")

    args = parser.parse_args(argv)
    test_list = None
    if args.testfile:
        test_list = config_parser.parse_test_file(args.testfile[0])
    elif args.testclass:
        test_list = args.testclass
    parsed_configs = config_parser.load_test_config_file(
        args.config[0], args.testbed, args.testpaths, args.logpath,
        args.test_args, args.random, args.test_case_iterations)
    # Prepare args for test runs
    test_identifiers = config_parser.parse_test_list(test_list)

    # Execute test runners.
    if args.parallel and len(parsed_configs) > 1:
        print('Running tests in parallel.')
        exec_result = _run_tests_parallel(parsed_configs, test_identifiers,
                                          args.campaign_iterations)
    else:
        print('Running tests sequentially.')
        exec_result = _run_tests_sequential(parsed_configs, test_identifiers,
                                            args.campaign_iterations)
    if exec_result is False:
        # return 1 upon test failure.
        sys.exit(1)
    sys.exit(0)


if __name__ == "__main__":
    main(sys.argv[1:])
