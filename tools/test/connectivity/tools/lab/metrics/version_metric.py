#!/usr/bin/env python
#
#   Copyright 2017 - The Android Open Source Project
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

from metrics.metric import Metric


class FastbootVersionMetric(Metric):

    FASTBOOT_COMMAND = 'fastboot --version'
    FASTBOOT_VERSION = 'fastboot_version'

    # String to return if fastboot version is too old
    FASTBOOT_ERROR_MESSAGE = ('this version is older than versions that'
                              'return versions properly')

    def gather_metric(self):
        """Tells which versions of fastboot

        Returns:
            A dict with the following fields:
              fastboot_version: string representing version of fastboot
                  Older versions of fastboot do not have a version flag. On an
                  older version, this metric will print 'this version is older
                  than versions that return versions properly'

        """
        result = self._shell.run(self.FASTBOOT_COMMAND, ignore_status=True)
        # If '--version' flag isn't recognized, will print to stderr
        if result.stderr:
            version = self.FASTBOOT_ERROR_MESSAGE
        else:
            # The version is the last token on the first line
            version = result.stdout.splitlines()[0].split()[-1]

        response = {self.FASTBOOT_VERSION: version}
        return response


class AdbVersionMetric(Metric):

    ADB_COMMAND = 'adb version'
    ADB_VERSION = 'adb_version'
    ADB_REVISION = 'adb_revision'

    def gather_metric(self):
        """Tells which versions of adb

        Returns:
            A dict with the following fields:
              adb_version: string representing version of adb
              adb_revision: string representing revision of adb

        """
        result = self._shell.run(self.ADB_COMMAND)
        stdout = result.stdout.splitlines()
        adb_version = stdout[0].split()[-1]
        adb_revision = ""
        # Old versions of ADB do not have revision numbers.
        if len(stdout) > 1:
            adb_revision = stdout[1].split()[1]

        response = {
            self.ADB_VERSION: adb_version,
            self.ADB_REVISION: adb_revision
        }
        return response


class PythonVersionMetric(Metric):

    PYTHON_COMMAND = 'python -V 2>&1'
    PYTHON_VERSION = 'python_version'

    def gather_metric(self):
        """Tells which versions of python

        Returns:
            A dict with the following fields:
              python_version: string representing version of python

        """
        result = self._shell.run(self.PYTHON_COMMAND)
        # Python prints this to stderr
        version = result.stdout.split()[-1]

        response = {self.PYTHON_VERSION: version}
        return response


class KernelVersionMetric(Metric):

    KERNEL_COMMAND = 'uname -r'
    KERNEL_RELEASE = 'kernel_release'

    def gather_metric(self):
        """Tells which versions of kernel

        Returns:
            A dict with the following fields:
              kernel_release: string representing kernel release

        """
        result = self._shell.run(self.KERNEL_COMMAND).stdout
        response = {self.KERNEL_RELEASE: result}
        return response
