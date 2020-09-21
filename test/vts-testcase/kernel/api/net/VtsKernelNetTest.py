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
import os

from vts.runners.host import asserts
from vts.runners.host import base_test
from vts.runners.host import const
from vts.runners.host import keys
from vts.runners.host import test_runner
from vts.utils.python.os import path_utils


class VtsKernelNetTest(base_test.BaseTestClass):
    """Host test class to run android kernel unit test.

    Attributes:
        dut: AndroidDevice, the device under test.
        shell: AdbProxy, instance of adb shell.
        host_bin_path: string, path to test binary on the host.
        target_bin_path: string, path to test binary on the target.
    """

    def setUpClass(self):
        required_params = [
            keys.ConfigKeys.IKEY_DATA_FILE_PATH,
        ]
        self.getUserParams(required_params)
        logging.info('%s: %s', keys.ConfigKeys.IKEY_DATA_FILE_PATH,
                     self.data_file_path)

        self.dut = self.android_devices[0]
        self.shell = self.dut.adb.shell

        # 32-bit version of the test should only run against 32-bit kernel;
        # same for 64 bit.
        bin_path = ('nativetest64' if self.dut.is64Bit else 'nativetest',
                    'kernel_net_tests', 'kernel_net_tests')

        self.host_bin_path = os.path.join(self.data_file_path, 'DATA', *bin_path)
        self.target_bin_path = path_utils.JoinTargetPath('data', *bin_path)

    def tearDownClass(self):
        self.shell('rm -rf %s' % path_utils.TargetDirName(self.target_bin_path))

    def testKernelNetworking(self):
        """Android kernel unit test."""
        # Push the test binary to target device.
        self.shell('mkdir -p %s' % path_utils.TargetDirName(self.target_bin_path))
        self.dut.adb.push('%s %s' % (self.host_bin_path, self.target_bin_path))
        self.shell('chmod 777 %s' % self.target_bin_path)

        # Execute the test binary.
        result = self.shell(self.target_bin_path, no_except=True)

        logging.info('stdout: %s', result[const.STDOUT])
        logging.error('stderr: %s', result[const.STDERR])
        logging.info('exit code: %s', result[const.EXIT_CODE])
        asserts.assertFalse(
            result[const.EXIT_CODE],
            'kernel_net_tests binary returned non-zero exit code.')

if __name__ == '__main__':
    test_runner.main()
