#!/usr/bin/env python
#
# Copyright 2016 - The Android Open Source Project
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

# Initiate a test case directory.
# This script copy a template which contains Android.mk, __init__.py files,
# AndroidTest.xml and a test case python file into a given relative directory
# under testcases/ using the given test name.

import os
import sys
import datetime
import re
import shutil
import argparse

VTS_PATH = 'test/vts'
VTS_TEST_CASE_PATH = os.path.join(VTS_PATH, 'testcases')
PYTHON_INIT_FILE_NAME = '__init__.py'
ANDROID_MK_FILE_NAME = 'Android.mk'
ANDROID_TEST_XML_FILE_NAME = 'AndroidTest.xml'


class TestCaseCreator(object):
    '''Init a test case directory with helloworld test case.

    Attributes:
        test_name: string, test case name in UpperCamel
        build_top: string, equal to environment variable ANDROID_BUILD_TOP
        test_dir: string, test case absolute directory
        test_name: string, test case name in UpperCamel
        test_plan: string, the plan that the test belongs to
        test_type: test type, such as HidlHalTest, HostDrivenTest, etc
        current_year: current year
        vts_test_case_dir: absolute dir of vts testcases directory
    '''

    def __init__(self, test_name, test_plan, test_dir_under_testcases,
                 test_type):
        '''Initialize class attributes.

        Args:
            test_name: string, test case name in UpperCamel
            test_plan: string, the plan that the test belongs to
            test_dir_under_testcases: string, test case relative directory under
                                      test/vts/testcases.
        '''
        if not test_dir_under_testcases:
            print 'Error: Empty test directory entered. Exiting'
            sys.exit(3)
        test_dir_under_testcases = os.path.normpath(
            test_dir_under_testcases.strip())

        if not self.IsUpperCamel(test_name):
            print 'Error: Test name not in UpperCamel case. Exiting'
            sys.exit(4)
        self.test_name = test_name

        if not test_plan:
            self.test_plan = 'vts-misc'
        else:
            self.test_plan = test_plan

        if not test_type:
            self.test_type = 'HidlHalTest'
        else:
            self.test_type = test_type

        self.build_top = os.getenv('ANDROID_BUILD_TOP')
        if not self.build_top:
            print('Error: Missing ANDROID_BUILD_TOP env variable. Please run '
                  '\'. build/envsetup.sh; lunch <build target>\' Exiting...')
            sys.exit(1)

        self.vts_test_case_dir = os.path.abspath(
            os.path.join(self.build_top, VTS_TEST_CASE_PATH))

        self.test_dir = os.path.abspath(
            os.path.join(self.vts_test_case_dir, test_dir_under_testcases))

        self.current_year = datetime.datetime.now().year

    def InitTestCaseDir(self):
        '''Start init test case directory'''
        if os.path.exists(self.test_dir):
            print 'Error: Test directory already exists. Exiting...'
            sys.exit(2)
        try:
            os.makedirs(self.test_dir)
        except:
            print('Error: Failed to create test directory at %s. '
                  'Exiting...' % self.test_dir)
            sys.exit(2)

        self.CreatePythonInitFile()
        self.CreateAndroidMk()
        self.CreateAndroidTestXml()
        self.CreateTestCasePy()

    def UpperCamelToLowerUnderScore(self, name):
        '''Convert UpperCamel name to lower_under_score name.

        Args:
            name: string in UpperCamel.

        Returns:
            a lower_under_score version of the given name
        '''
        return re.sub('(?!^)([A-Z]+)', r'_\1', name).lower()

    def IsUpperCamel(self, name):
        '''Check whether a given name is UpperCamel case.

        Args:
            name: string.

        Returns:
            True if name is in UpperCamel case, False otherwise
        '''
        regex = re.compile('((?:[A-Z][a-z]+)[0-9]*)+')
        match = regex.match(name)
        return match and (match.end() - match.start() == len(name))

    def CreatePythonInitFile(self):
        '''Populate test case directory and parent directories with __init__.py.
        '''
        if not self.test_dir.startswith(self.vts_test_case_dir):
            print 'Error: Test case directory is not under VTS test case directory.'
            sys.exit(4)

        path = self.test_dir
        while not path == self.vts_test_case_dir:
            target = os.path.join(path, PYTHON_INIT_FILE_NAME)
            if not os.path.exists(target):
                print 'Creating %s' % target
                with open(target, 'w') as f:
                    pass
            path = os.path.dirname(path)

    def CreateAndroidMk(self):
        '''Populate test case directory and parent directories with Android.mk
        '''
        vts_dir = os.path.join(self.build_top, VTS_PATH)

        target = os.path.join(self.test_dir, ANDROID_MK_FILE_NAME)
        with open(target, 'w') as f:
            print 'Creating %s' % target
            f.write(LICENSE_STATEMENT_POUND.format(year=self.current_year))
            f.write('\n')
            f.write(
                ANDROID_MK_TEMPLATE.format(
                    test_name=self.test_name,
                    config_src_dir=self.test_dir[len(vts_dir) + 1:]))

        path = self.test_dir
        while not path == vts_dir:
            target = os.path.join(path, ANDROID_MK_FILE_NAME)
            if not os.path.exists(target):
                print 'Creating %s' % target
                with open(target, 'w') as f:
                    f.write(
                        LICENSE_STATEMENT_POUND.format(year=self.current_year))
                    f.write(ANDROID_MK_CALL_SUB)
            path = os.path.dirname(path)

    def CreateAndroidTestXml(self):
        '''Create AndroidTest.xml'''
        target = os.path.join(self.test_dir, ANDROID_TEST_XML_FILE_NAME)
        with open(target, 'w') as f:
            print 'Creating %s' % target
            f.write(XML_HEADER)
            f.write(LICENSE_STATEMENT_XML.format(year=self.current_year))
            f.write(
                ANDROID_TEST_XML_TEMPLATE.format(
                    test_name=self.test_name,
                    test_plan=self.test_plan,
                    test_type=self.test_type,
                    test_path_under_vts=self.test_dir[
                        len(os.path.join(self.build_top, VTS_PATH)) + 1:],
                    test_case_file_without_extension=self.test_name))

    def CreateTestCasePy(self):
        '''Create <test_case_name>.py'''
        target = os.path.join(self.test_dir, '%s.py' % self.test_name)
        with open(target, 'w') as f:
            print 'Creating %s' % target
            f.write(PY_HEADER)
            f.write(LICENSE_STATEMENT_POUND.format(year=self.current_year))
            f.write('\n')
            f.write(TEST_CASE_PY_TEMPLATE.format(test_name=self.test_name))


def main():
    parser = argparse.ArgumentParser(description='Initiate a test case.')
    parser.add_argument(
        '--name',
        dest='test_name',
        required=True,
        help='Test case name in UpperCamel. Example: VtsKernelLtp')
    parser.add_argument(
        '--plan',
        dest='test_plan',
        required=False,
        help='The plan that the test belongs to. Example: vts-kernel')
    parser.add_argument(
        '--dir',
        dest='test_dir',
        required=True,
        help='Test case relative directory under test/vts/testcses.')
    parser.add_argument(
        '--type',
        dest='test_type',
        required=False,
        help='Test type, such as HidlHalTest, HostDrivenTest, etc.')

    args = parser.parse_args()
    test_case_creater = TestCaseCreator(args.test_name, args.test_plan,
                                        args.test_dir, args.test_type)
    test_case_creater.InitTestCaseDir()


LICENSE_STATEMENT_POUND = '''#
# Copyright (C) {year} The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
'''

LICENSE_STATEMENT_XML = '''<!-- Copyright (C) {year} The Android Open Source Project

     Licensed under the Apache License, Version 2.0 (the "License");
     you may not use this file except in compliance with the License.
     You may obtain a copy of the License at

          http://www.apache.org/licenses/LICENSE-2.0

     Unless required by applicable law or agreed to in writing, software
     distributed under the License is distributed on an "AS IS" BASIS,
     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
     See the License for the specific language governing permissions and
     limitations under the License.
-->
'''

ANDROID_MK_TEMPLATE = '''LOCAL_PATH := $(call my-dir)

include $(call all-subdir-makefiles)

include $(CLEAR_VARS)

LOCAL_MODULE := {test_name}
VTS_CONFIG_SRC_DIR := {config_src_dir}
include test/vts/tools/build/Android.host_config.mk
'''

ANDROID_MK_CALL_SUB = '''LOCAL_PATH := $(call my-dir)

include $(call all-subdir-makefiles)
'''

XML_HEADER = '''<?xml version="1.0" encoding="utf-8"?>
'''

ANDROID_TEST_XML_TEMPLATE = '''<configuration description="Config for VTS {test_name} test cases">
    <option name="config-descriptor:metadata" key="plan" value="{test_plan}" />
    <target_preparer class="com.android.compatibility.common.tradefed.targetprep.VtsFilePusher">
        <option name="push-group" value="{test_type}.push" />
    </target_preparer>
    <test class="com.android.tradefed.testtype.VtsMultiDeviceTest">
        <option name="test-module-name" value="{test_name}" />
        <option name="test-case-path" value="vts/{test_path_under_vts}/{test_case_file_without_extension}" />
    </test>
</configuration>
'''

PY_HEADER = '''#!/usr/bin/env python
'''

TEST_CASE_PY_TEMPLATE = '''import logging

from vts.runners.host import asserts
from vts.runners.host import base_test
from vts.runners.host import const
from vts.runners.host import test_runner



class {test_name}(base_test.BaseTestClass):
    """Two hello world test cases which use the shell driver."""

    def setUpClass(self):
        self.dut = self.android_devices[0]
        self.shell = self.dut.shell

    def testEcho1(self):
        """A simple testcase which sends a command."""
        results = self.shell.Execute("echo hello_world")  # runs a shell command.
        logging.info(str(results[const.STDOUT]))  # prints the stdout
        asserts.assertEqual(results[const.STDOUT][0].strip(), "hello_world")  # checks the stdout
        asserts.assertEqual(results[const.EXIT_CODE][0], 0)  # checks the exit code

    def testEcho2(self):
        """A simple testcase which sends two commands."""
        results = self.shell.Execute(["echo hello", "echo world"])
        logging.info(str(results[const.STDOUT]))
        asserts.assertEqual(len(results[const.STDOUT]), 2)  # check the number of processed commands
        asserts.assertEqual(results[const.STDOUT][0].strip(), "hello")
        asserts.assertEqual(results[const.STDOUT][1].strip(), "world")
        asserts.assertEqual(results[const.EXIT_CODE][0], 0)
        asserts.assertEqual(results[const.EXIT_CODE][1], 0)


if __name__ == "__main__":
    test_runner.main()
'''

if __name__ == '__main__':
    main()
