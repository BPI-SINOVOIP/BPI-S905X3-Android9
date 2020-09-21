#!/usr/bin/env python
#
# Copyright 2017 - The Android Open Source Project
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
#

import argparse
import os
import re
import sys

from build.build_rule_gen import BuildRuleGen
from build.vts_spec_parser import VtsSpecParser
from configure.test_case_creator import TestCaseCreator
from utils.const import Constant

TEST_TIME_OUT_PATTERN = '(([0-9]+)(m|s|h))+'

"""Generate Android.mk, Android.bp, and AndroidTest.xml files for given hal

Usage:
  python launch_hal_test.py [--options] hal_package name

  where options are:
  --test_type: Test type, currently support two types: target and host.
  --time_out: Timeout for the test, default is 1m.
  --package_root: Prefix of the HAL package. default is android.hardware
  --path_root: Root path that stores the HAL definition. default is hardware/interfaces
  --test_binary_file: Test binary file for target-side HAL test.
  --test_script_file: Test script file for host-side HAL test.
  --test_config_dir: Directory path to store the test configure files.
  --enable_profiling: Whether this is a profiling test.
  --replay: Whether this is a replay test.
  --disable_stop_runtime: Whether to stop framework before the test.
Example:
  python launch_hal_test.py android.hardware.nfc@1.0
  python launch_hal_test.py --enable_profiling android.hardware.nfc@1.0
  python launch_hal_test.py --test_type=host --time_out=5m android.hardware.nfc@1.0
  python launch_hal_test.py --package_root com.qualcomm.qti
  --path_root vendor/qcom/proprietary/interfaces/com/qualcomm/qti/
  --test_configure_dir temp/ com.qualcomm.qti.qcril.qcrilhook@1.0
"""


def main():
    parser = argparse.ArgumentParser(description='Initiate a test case.')
    parser.add_argument(
        '--test_type',
        dest='test_type',
        required=False,
        default='target',
        help='Test type, currently support two types: target and host.')
    parser.add_argument(
        '--time_out',
        dest='time_out',
        required=False,
        default='1m',
        help='Timeout for the test, default is 1m.')
    parser.add_argument(
        '--enable_profiling',
        dest='enable_profiling',
        action='store_true',
        required=False,
        help='Whether to create profiling test.')
    parser.add_argument(
        '--replay',
        dest='is_replay',
        action='store_true',
        required=False,
        help='Whether this is a replay test.')
    parser.add_argument(
        '--disable_stop_runtime',
        dest='disable_stop_runtime',
        action='store_true',
        required=False,
        help='Whether to stop framework before the test.')
    parser.add_argument(
        '--package_root',
        dest='package_root',
        required=False,
        default=Constant.HAL_PACKAGE_PREFIX,
        help='Prefix of the HAL package. e.g. android.hardware')
    parser.add_argument(
        '--path_root',
        dest='path_root',
        required=False,
        default=Constant.HAL_INTERFACE_PATH,
        help=
        'Root path that stores the HAL definition. e.g. hardware/interfaces')
    parser.add_argument(
        '--test_binary_file',
        dest='test_binary_file',
        required=False,
        help='Test binary file for target-side HAL test.')
    parser.add_argument(
        '--test_script_file',
        dest='test_script_file',
        required=False,
        help='Test script file for host-side HAL test.')
    parser.add_argument(
        '--test_config_dir',
        dest='test_config_dir',
        required=False,
        help='Directory path to store the test configure files.')
    parser.add_argument(
        'hal_package_name',
        help='hal package name (e.g. android.hardware.nfc@1.0).')
    args = parser.parse_args()

    regex = re.compile(Constant.HAL_PACKAGE_NAME_PATTERN)
    result = re.match(regex, args.hal_package_name)
    if not result:
        print 'Invalid hal package name. Exiting..'
        sys.exit(1)

    if not args.hal_package_name.startswith(args.package_root + '.'):
        print 'hal_package_name does not start with package_root. Exiting...'
        sys.exit(1)

    if args.test_type != 'target' and args.test_type != 'host':
        print 'Unsupported test type. Exiting...'
        sys.exit(1)
    elif args.test_type == 'host' and args.is_replay:
        print 'Host side replay test is not supported yet. Exiting...'
        sys.exit(1)

    regex = re.compile(TEST_TIME_OUT_PATTERN)
    result = re.match(regex, args.time_out)
    if not result:
        print 'Invalid test time out format. Exiting...'
        sys.exit(1)

    if not args.test_config_dir:
        if args.package_root == Constant.HAL_PACKAGE_PREFIX:
            args.test_config_dir = Constant.VTS_HAL_TEST_CASE_PATH
        else:
            args.test_config_dir = args.path_root

    stop_runtime = False
    if args.test_type == 'target' and not args.disable_stop_runtime:
        stop_runtime = True

    vts_spec_parser = VtsSpecParser(
        package_root=args.package_root, path_root=args.path_root)
    test_case_creater = TestCaseCreator(vts_spec_parser, args.hal_package_name)
    if not test_case_creater.LaunchTestCase(
            args.test_type,
            args.time_out,
            is_replay=args.is_replay,
            is_profiling=args.enable_profiling,
            stop_runtime=stop_runtime,
            test_binary_file=args.test_binary_file,
            test_script_file=args.test_script_file,
            test_config_dir=args.test_config_dir,
            package_root=args.package_root,
            path_root=args.path_root):
        print('Error: Failed to launch test for %s. Exiting...' %
              args.hal_package_name)
        sys.exit(1)

    if args.test_type == "host" or args.enable_profiling:
        build_rule_gen = BuildRuleGen(
            Constant.BP_WARNING_HEADER, args.package_root, args.path_root)
        name_version = args.hal_package_name[len(args.package_root) + 1:]
        build_rule_gen.UpdateHalDirBuildRule(
            [name_version.split('@')], args.test_config_dir)

if __name__ == '__main__':
    main()
