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

"""Kernel Swapper.

This class manages swapping kernel images for a Cloud Android instance.
"""
import os
import subprocess

from acloud.public import errors
from acloud.public import report
from acloud.internal.lib import android_build_client
from acloud.internal.lib import android_compute_client
from acloud.internal.lib import auth
from acloud.internal.lib import gstorage_client
from acloud.internal.lib import utils

ALL_SCOPES = ' '.join([android_build_client.AndroidBuildClient.SCOPE,
                       gstorage_client.StorageClient.SCOPE,
                       android_compute_client.AndroidComputeClient.SCOPE])

# ssh flags used to communicate with the Cloud Android instance.
SSH_FLAGS = [
    '-q', '-o UserKnownHostsFile=/dev/null', '-o "StrictHostKeyChecking no"',
    '-o ServerAliveInterval=10'
]

# Shell commands run on target.
MOUNT_CMD = ('if mountpoint -q /boot ; then umount /boot ; fi ; '
             'mount -t ext4 /dev/block/sda1 /boot')
REBOOT_CMD = 'nohup reboot > /dev/null 2>&1 &'


class KernelSwapper(object):
    """A class that manages swapping a kernel image on a Cloud Android instance.

    Attributes:
        _compute_client: AndroidCopmuteClient object, manages AVD.
        _instance_name: string, name of Cloud Android Instance.
        _target_ip: string, IP address of Cloud Android instance.
        _ssh_flags: string list, flags to be used with ssh and scp.
    """

    def __init__(self, cfg, instance_name):
        """Initialize.

        Args:
            cfg: AcloudConfig object, used to create credentials.
            instance_name: string, instance name.
        """
        credentials = auth.CreateCredentials(cfg, ALL_SCOPES)
        self._compute_client = android_compute_client.AndroidComputeClient(
            cfg, credentials)
        # Name of the Cloud Android instance.
        self._instance_name = instance_name
        # IP of the Cloud Android instance.
        self._target_ip = self._compute_client.GetInstanceIP(instance_name)

    def SwapKernel(self, local_kernel_image):
        """Swaps the kernel image on target AVD with given kernel.

        Mounts boot image containing the kernel image to the filesystem, then
        overwrites that kernel image with a new kernel image, then reboots the
        Cloud Android instance.

        Args:
            local_kernel_image: string, local path to a kernel image.

        Returns:
            A Report instance.
        """
        r = report.Report(command='swap_kernel')
        try:
            self._ShellCmdOnTarget(MOUNT_CMD)
            self.PushFile(local_kernel_image, '/boot')
            self.RebootTarget()
        except subprocess.CalledProcessError as e:
            r.AddError(str(e))
            r.SetStatus(report.Status.FAIL)
            return r
        except errors.DeviceBootTimeoutError as e:
            r.AddError(str(e))
            r.SetStatus(report.Status.BOOT_FAIL)
            return r

        r.SetStatus(report.Status.SUCCESS)
        return r

    def PushFile(self, src_path, dest_path):
        """Pushes local file to target Cloud Android instance.

        Args:
            src_path: string, local path to file to be pushed.
            dest_path: string, path on target where to push the file to.

        Raises:
            subprocess.CalledProcessError: see _ShellCmd.
        """
        cmd = 'scp %s %s root@%s:%s' % (' '.join(SSH_FLAGS), src_path,
                                        self._target_ip, dest_path)
        self._ShellCmd(cmd)

    def RebootTarget(self):
        """Reboots the target Cloud Android instance and waits for boot.

        Raises:
            subprocess.CalledProcessError: see _ShellCmd.
            errors.DeviceBootTimeoutError: if booting times out.
        """
        self._ShellCmdOnTarget(REBOOT_CMD)
        self._compute_client.WaitForBoot(self._instance_name)

    def _ShellCmdOnTarget(self, target_cmd):
        """Runs a shell command on target Cloud Android instance.

        Args:
            target_cmd: string, shell command to be run on target.

        Raises:
            subprocess.CalledProcessError: see _ShellCmd.
        """
        ssh_cmd = 'ssh %s root@%s' % (' '.join(SSH_FLAGS), self._target_ip)
        host_cmd = ' '.join([ssh_cmd, '"%s"' % target_cmd])
        self._ShellCmd(host_cmd)

    def _ShellCmd(self, host_cmd):
        """Runs a shell command on host device.

        Args:
            host_cmd: string, shell command to be run on host.

        Raises:
            subprocess.CalledProcessError: For any non-zero return code of
                                           host_cmd.
        """
        utils.Retry(
            retry_checker=lambda e: isinstance(e, subprocess.CalledProcessError),
            max_retries=2,
            functor=lambda cmd: subprocess.check_call(cmd, shell=True),
            cmd=host_cmd)
