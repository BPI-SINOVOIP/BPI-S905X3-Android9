#!/usr/bin/env python
#
# Copyright 2018 - The Android Open Source Project
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
import sys

from build.vts_spec_parser import VtsSpecParser
from configure.test_case_creator import TestCaseCreator
"""Regenerate test configures for all hal adapter tests.

Usage:
  python update_hal_adapter_tests.py [-h] cts_hal_mapping_dir
"""


def main():
    build_top = os.getenv('ANDROID_BUILD_TOP')
    if not build_top:
        print('Error: Missing ANDROID_BUILD_TOP env variable. Please run '
              '\'. build/envsetup.sh; lunch <build target>\' Exiting...')
        sys.exit(1)

    parser = argparse.ArgumentParser(
        description='Update vts hal adapter tests.')
    parser.add_argument(
        'cts_hal_mapping_dir',
        help='Directory that stores cts_hal_mapping files.')
    args = parser.parse_args()

    vts_spec_parser = VtsSpecParser()
    hal_list = vts_spec_parser.HalNamesAndVersions()

    for hal_name, hal_version in hal_list:
        hal_package_name = 'android.hardware.' + hal_name + '@' + hal_version
        major_version, minor_version = hal_version.split('.')
        if int(minor_version) > 0:
            lower_version = major_version + '.' + str(int(minor_version) - 1)
            if (hal_name, lower_version) in hal_list:
                test_case_creater = TestCaseCreator(vts_spec_parser,
                                                    hal_package_name)
                test_case_creater.LaunchTestCase(
                    test_type='adapter',
                    mapping_dir_path=args.cts_hal_mapping_dir)


if __name__ == '__main__':
    main()
