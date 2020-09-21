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

"""A test module to check system-as-root is enabled for new devices in P.

The test logic is:
    if precondition-first-api-level >= 28 (in AndroidTest.xml):
        assert (
            ro.build.system_root_image is 'true' AND
            /system mount point doesn't exist)
    else:
        assert True
"""


import logging

from vts.runners.host import asserts
from vts.runners.host import base_test
from vts.runners.host import const
from vts.runners.host import test_runner


# The property to indicate the system image is also root image.
_SYSTEM_ROOT_IMAGE_PROP = "ro.build.system_root_image"


class VtsKernelSystemAsRootTest(base_test.BaseTestClass):
    """A test class to verify system-as-root is enabled."""

    def setUpClass(self):
        """Initializes device and shell."""
        self._dut = self.android_devices[0]
        self._shell = self._dut.shell

    def testSystemRootImageProperty(self):
        """Checks ro.build.system_root_image is 'true'."""
        asserts.assertEqual("true",
                            self._dut.getProp(_SYSTEM_ROOT_IMAGE_PROP),
                            "%s is not true" % _SYSTEM_ROOT_IMAGE_PROP)

    def testNoSystemMountPoint(self):
        """Checks there is no /system mount point."""
        # The format of /proc/mounts is:
        # <partition> <mount point> <file system> <mount options> ...
        results = self._shell.Execute(
            "cat /proc/mounts | cut -d\" \" -f2")
        mount_points = results[const.STDOUT][0].split()
        logging.info('Mount points on the device: %s', mount_points)
        asserts.assertFalse("/system" in mount_points,
                            "/system mount point shouldn't exist")


if __name__ == "__main__":
    test_runner.main()
