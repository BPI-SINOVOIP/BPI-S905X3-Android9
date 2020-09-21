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
"""Tests acloud.public.acloud_kernel.kernel_swapper."""

import subprocess
import mock

import unittest
from acloud.internal.lib import android_compute_client
from acloud.internal.lib import auth
from acloud.internal.lib import driver_test_lib
from acloud.public.acloud_kernel import kernel_swapper


class KernelSwapperTest(driver_test_lib.BaseDriverTest):
    """Test kernel_swapper."""

    def setUp(self):
        """Set up the test."""
        super(KernelSwapperTest, self).setUp()
        self.cfg = mock.MagicMock()
        self.credentials = mock.MagicMock()
        self.Patch(auth, 'CreateCredentials', return_value=self.credentials)
        self.compute_client = mock.MagicMock()
        self.Patch(
            android_compute_client,
            'AndroidComputeClient',
            return_value=self.compute_client)
        self.subprocess_call = self.Patch(subprocess, 'check_call')

        self.fake_ip = '123.456.789.000'
        self.fake_instance = 'fake-instance'
        self.compute_client.GetInstanceIP.return_value = self.fake_ip

        self.kswapper = kernel_swapper.KernelSwapper(self.cfg,
                                                     self.fake_instance)
        self.ssh_cmd_prefix = 'ssh %s root@%s' % (
            ' '.join(kernel_swapper.SSH_FLAGS), self.fake_ip)
        self.scp_cmd_prefix = 'scp %s' % ' '.join(kernel_swapper.SSH_FLAGS)

    def testPushFile(self):
        """Test RebootTarget."""
        fake_src_path = 'fake-src'
        fake_dest_path = 'fake-dest'
        scp_cmd = ' '.join([self.scp_cmd_prefix, '%s root@%s:%s' %
                            (fake_src_path, self.fake_ip, fake_dest_path)])

        self.kswapper.PushFile(fake_src_path, fake_dest_path)
        self.subprocess_call.assert_called_once_with(scp_cmd, shell=True)

    def testRebootTarget(self):
        """Test RebootTarget."""
        self.kswapper.RebootTarget()
        reboot_cmd = ' '.join([
            self.ssh_cmd_prefix, '"%s"' % kernel_swapper.REBOOT_CMD
        ])

        self.subprocess_call.assert_called_once_with(reboot_cmd, shell=True)
        self.compute_client.WaitForBoot.assert_called_once_with(
            self.fake_instance)

    def testSwapKernel(self):
        """Test SwapKernel."""
        fake_local_kernel_image = 'fake-kernel'
        mount_cmd = ' '.join([
            self.ssh_cmd_prefix, '"%s"' % kernel_swapper.MOUNT_CMD
        ])
        scp_cmd = ' '.join([self.scp_cmd_prefix, '%s root@%s:%s' %
                            (fake_local_kernel_image, self.fake_ip, '/boot')])
        reboot_cmd = ' '.join([
            self.ssh_cmd_prefix, '"%s"' % kernel_swapper.REBOOT_CMD
        ])

        self.kswapper.SwapKernel(fake_local_kernel_image)
        self.subprocess_call.assert_has_calls([
            mock.call(
                mount_cmd, shell=True), mock.call(
                    scp_cmd, shell=True), mock.call(
                        reboot_cmd, shell=True)
        ])


if __name__ == '__main__':
    unittest.main()
