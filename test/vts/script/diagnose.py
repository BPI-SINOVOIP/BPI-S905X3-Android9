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

import datetime
import platform
import subprocess
import sys


def runCommand(cmd, description):
    """Runs given command, then prints output and error.

    Args:
        cmd: string, command to execute
        description: string, noun that describes the output of the command
    """
    print '=====', description, '====='

    returnCode = subprocess.call(cmd.split(' '))
    if returnCode != 0:
        print 'Error occured when executing following command:', cmd
        print 'Error code:', returnCode

def main():
    # Get current date and time
    now = datetime.datetime.now()
    print '===== Current date and time =====\n', str(now)
    print '===== OS version =====\n', platform.platform()
    print '===== Python version =====\n', sys.version

    # Get pip version
    runCommand('pip --version', 'Pip version')

    # Get virtualenv version
    runCommand('virtualenv --version', 'Virtualenv version')

    # Get target device info with adb
    runCommand('adb shell getprop ro.build.fingerprint', 'Target device info [ro.build.fingerprint]')

    runCommand('adb shell lshal --init-vintf', 'Read HAL Info')
    runCommand('adb shell cat /vendor/manifest.xml', 'Read vendor/manifest.xml')


if __name__ == "__main__":
    main()
