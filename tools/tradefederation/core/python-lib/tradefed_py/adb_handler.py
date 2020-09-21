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

import subprocess

class AdbHandler(object):
    """Adb wrapper to execute shell and adb command to the device."""

    def __init__(self, serial=None):
        self.serial = serial
        self.adb_cmd = 'adb -s {}'.format(serial)

    def exec_adb_command(self, cmd):
        """Method to execute an adb command against the device."""
        cmd = "{} {}".format(self.adb_cmd, cmd)
        proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
        (out, err) = proc.communicate()
        ret = proc.returncode
        if ret == 0:
            return out
        raise AdbError(cmd=cmd, stdout=out, stderr=err, ret_code=ret)

    def exec_shell_command(self, cmd):
        """Method to execute a shell command against the device."""
        cmd = '{} shell {}'.format(self.adb_cmd, cmd)
        proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
        (out, err) = proc.communicate()
        ret = proc.returncode
        if ret == 0:
            return out
        raise AdbError(cmd=cmd, stdout=out, stderr=err, ret_code=ret)

class AdbError(Exception):
    """Raised when there is an error in adb operations."""

    def __init__(self, cmd, stdout, stderr, ret_code):
        self.cmd = cmd
        self.stdout = stdout
        self.stderr = stderr
        self.ret_code = ret_code

    def __str__(self):
        return ('Error executing adb cmd "%s". ret: %d, stdout: %s, stderr: %s'
                ) % (self.cmd, self.ret_code, self.stdout, self.stderr)
