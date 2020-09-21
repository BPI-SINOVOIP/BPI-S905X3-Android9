#!/usr/bin/env python
#
# Copyright (C) 2017 The Android Open Source Project
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

import logging

from vts.runners.host import asserts
from vts.runners.host import base_test
from vts.runners.host import const
from vts.runners.host import test_runner
from vts.testcases.kernel.api.selinux import SelinuxCheckReqProtTest
from vts.testcases.kernel.api.selinux import SelinuxPolicyTest
from vts.testcases.kernel.api.selinux import SelinuxNullTest
from vts.utils.python.controllers import android_device
from vts.utils.python.file import target_file_utils

TEST_OBJECTS = {
    SelinuxCheckReqProtTest.SelinuxCheckReqProt(),
    SelinuxPolicyTest.SelinuxPolicy(), SelinuxNullTest.SelinuxNull()
}


class VtsKernelSelinuxFileApiTest(base_test.BaseTestClass):
    """Test cases which check content of selinuxfs files.
    """

    def setUpClass(self):
        self.dut = self.android_devices[0]
        self.shell = self.dut.shell

    def runSelinuxFileTest(self, test_object):
        """Reads the file and checks that its content and permissions are valid.

        Args:
            test_object: inherits KernelSelinuxFileTestBase, contains the test functions
        """
        logging.info("Testing existence of %s" % (test_object.get_path()))

        asserts.assertTrue(
            target_file_utils.Exists(test_object.get_path(), self.shell),
            "%s: File does not exist." % test_object.get_path())

        logging.info("Testing permissions of %s" % (test_object.get_path()))
        try:
            permissions = target_file_utils.GetPermission(
                test_object.get_path(), self.shell)
            asserts.assertTrue(
                test_object.get_permission_checker()(permissions),
                "%s: File has invalid permissions (%s)" %
                (test_object.get_path(), permissions))
        except (ValueError, IOError) as e:
            asserts.fail("Failed to assert permissions: %s" % str(e))

        logging.info("Testing format of %s" % (test_object.get_path()))
        file_content = target_file_utils.ReadFileContent(
            test_object.get_path(), self.shell)
        asserts.assertTrue(
            test_object.result_correct(file_content), "Results not valid!")

    def generateProcFileTests(self):
        """Run all selinux file tests."""
        self.runGeneratedTests(
            test_func=self.runSelinuxFileTest,
            settings=TEST_OBJECTS,
            name_func=lambda test_obj: "test" + test_obj.__class__.__name__)


if __name__ == "__main__":
    test_runner.main()
