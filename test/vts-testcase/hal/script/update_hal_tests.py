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

from configure.test_case_creator import TestCaseCreator
from build.vts_spec_parser import VtsSpecParser
"""Regenerate test configures for all existing tests.

Usage:
  python update_hal_tests.py
"""


def GetTimeOut(configure_path):
    """Get the timeout settings from the original configure.

    Args:
      configure_path: path of the original configure file.

    Returns:
      timeout values.
    """
    time_out = "1m"
    configure_file = open(configure_path, "r")
    for line in configure_file.readlines():
        if "test-timeout" in line:
            temp = line[(line.find("value") + 7):]
            time_out = temp[0:temp.find('"')]
            break
    return time_out


def GetDisableRunTime(configure_path):
    """Get the stop runtime settings from the original configure.

    Args:
      configure_path: path of the original configure file.

    Returns:
      Settings about whether to stop runtime before test.
    """
    disable_runtime = False
    configure_file = open(configure_path, "r")
    for line in configure_file.readlines():
        if "binary-test-disable-framework" in line:
            disable_runtime = True
            break
    return disable_runtime


test_categories = {
    'target': ('target/AndroidTest.xml', 'target', False),
    'target_profiling': ('target_profiling/AndroidTest.xml', 'target', True),
    'host': ('host/AndroidTest.xml', 'host', False),
    'host_profiling': ('host_profiling/AndroidTest.xml', 'host', True),
}


def main():
    build_top = os.getenv('ANDROID_BUILD_TOP')
    if not build_top:
        print('Error: Missing ANDROID_BUILD_TOP env variable. Please run '
              '\'. build/envsetup.sh; lunch <build target>\' Exiting...')
        sys.exit(1)

    vts_spec_parser = VtsSpecParser()
    hal_list = vts_spec_parser.HalNamesAndVersions()

    for hal_name, hal_version in hal_list:
        hal_package_name = 'android.hardware.' + hal_name + '@' + hal_version
        test_case_creater = TestCaseCreator(vts_spec_parser, hal_package_name)
        hal_path = hal_name.replace(".", "/")
        hal_version_str = 'V' + hal_version.replace('.', '_')
        hal_test_path = os.path.join(build_top, 'test/vts-testcase/hal',
                                     hal_path, hal_version_str)

        for test_categry, configure in test_categories.iteritems():
            print test_categry
            print configure
            test_configure_path = os.path.join(hal_test_path, configure[0])
            if os.path.exists(test_configure_path):
                time_out = GetTimeOut(test_configure_path)
                stop_runtime = GetDisableRunTime(test_configure_path)
                test_case_creater.LaunchTestCase(
                    configure[1],
                    time_out=time_out,
                    is_profiling=configure[2],
                    stop_runtime=stop_runtime,
                    update_only=True)


if __name__ == '__main__':
    main()
