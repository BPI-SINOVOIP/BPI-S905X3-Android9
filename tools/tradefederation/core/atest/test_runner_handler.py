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
Aggregates test runners, groups tests by test runners and kicks off tests.
"""

import itertools

import atest_error
from test_runners import atest_tf_test_runner
from test_runners import robolectric_test_runner
from test_runners import vts_tf_test_runner

# pylint: disable=line-too-long
_TEST_RUNNERS = {
    atest_tf_test_runner.AtestTradefedTestRunner.NAME: atest_tf_test_runner.AtestTradefedTestRunner,
    robolectric_test_runner.RobolectricTestRunner.NAME: robolectric_test_runner.RobolectricTestRunner,
    vts_tf_test_runner.VtsTradefedTestRunner.NAME: vts_tf_test_runner.VtsTradefedTestRunner,
}


def _get_test_runners():
    """Returns the test runners.

    If external test runners are defined outside atest, they can be try-except
    imported into here.

    Returns:
        Dict of test runner name to test runner class.
    """
    test_runners_dict = _TEST_RUNNERS
    # Example import of example test runner:
    try:
        # pylint: disable=line-too-long
        from test_runners import example_test_runner
        test_runners_dict[example_test_runner.ExampleTestRunner.NAME] = example_test_runner.ExampleTestRunner
    except ImportError:
        pass
    return test_runners_dict


def _group_tests_by_test_runners(test_infos):
    """Group the test_infos by test runners

    Args:
        test_infos: List of TestInfo.

    Returns:
        List of tuples (test runner, tests).
    """
    tests_by_test_runner = []
    test_runner_dict = _get_test_runners()
    key = lambda x: x.test_runner
    sorted_test_infos = sorted(list(test_infos), key=key)
    for test_runner, tests in itertools.groupby(sorted_test_infos, key):
        # groupby returns a grouper object, we want to operate on a list.
        tests = list(tests)
        test_runner_class = test_runner_dict.get(test_runner)
        if test_runner_class is None:
            raise atest_error.UnknownTestRunnerError('Unknown Test Runner %s' %
                                                     test_runner)
        tests_by_test_runner.append((test_runner_class, tests))
    return tests_by_test_runner


def get_test_runner_reqs(module_info, test_infos):
    """Returns the requirements for all test runners specified in the tests.

    Args:
        module_info: ModuleInfo object.
        test_infos: List of TestInfo.

    Returns:
        Set of build targets required by the test runners.
    """
    dummy_result_dir = ''
    test_runner_build_req = set()
    for test_runner, _ in _group_tests_by_test_runners(test_infos):
        test_runner_build_req |= test_runner(
            dummy_result_dir,
            module_info=module_info).get_test_runner_build_reqs()
    return test_runner_build_req


def run_all_tests(results_dir, test_infos, extra_args):
    """Run the given tests.

    Args:
        test_infos: List of TestInfo.
        extra_args: Dict of extra args for test runners to use.
    """
    for test_runner, tests in _group_tests_by_test_runners(test_infos):
        test_runner(results_dir).run_tests(tests, extra_args)
