#!/usr/bin/env python
#
# Copyright (C) 2018 The Android Open Source Project
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
import re

from vts.runners.host import asserts
from vts.runners.host import base_test
from vts.runners.host import const
from vts.runners.host import keys
from vts.runners.host import test_runner
from vts.utils.python.controllers import android_device
from vts.utils.python.file import target_file_utils

from vts.testcases.kernel.lib import version


class VtsKernelVersionTest(base_test.BaseTestClass):
    """Test case which verifies the version of the kernel on the DUT is
    supported.
    """

    def setUpClass(self):
        required_params = [keys.ConfigKeys.IKEY_DATA_FILE_PATH]
        self.getUserParams(required_params)
        self.dut = self.android_devices[0]
        self.shell = self.dut.shell
        self.supported_kernel_versions = version.getSupportedKernels(self.dut)

    def testKernelVersion(self):
        """Validate the kernel version of DUT is a valid kernel version.

        Returns:
            string, kernel version of device
        """
        cmd = "uname -a"
        results = self.shell.Execute(cmd)
        logging.info("Shell command '%s' results: %s", cmd, results)

        match = re.search(r"(\d+)\.(\d+)\.(\d+)", results[const.STDOUT][0])
        if match is None:
            asserts.fail("Failed to detect kernel version of device.")
        else:
            kernel_version = int(match.group(1))
            kernel_patchlevel = int(match.group(2))
            kernel_sublevel = int(match.group(3))
        logging.info("Detected kernel version: %s.%s.%s" % (kernel_version,
            kernel_patchlevel, kernel_sublevel))

        for v in self.supported_kernel_versions:
            if (kernel_version == v[0] and kernel_patchlevel == v[1] and
                kernel_sublevel >= v[2]):
                logging.info("Compliant kernel version %s.%s.%s found." %
                        (kernel_version, kernel_patchlevel, kernel_sublevel))
                return

        asserts.fail("Device is running an unsupported kernel version (%s.%s.%s)" %
                (kernel_version, kernel_patchlevel, kernel_sublevel))


if __name__ == "__main__":
    test_runner.main()
