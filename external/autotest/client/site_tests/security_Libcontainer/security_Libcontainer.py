# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.bin import test, utils

import os

class security_Libcontainer(test.test):
    """Runs libcontainer unit tests in the device.

    This is useful since some features (like user namespacing) can only really
    be tested outside of a chroot environment.
    """
    version = 1
    executable = 'libcontainer_target_test'


    def setup(self):
        """Builds the binary for the device."""
        os.chdir(self.srcdir)
        utils.make()


    def run_once(self):
        """Runs the test on the device."""
        binpath = os.path.join(self.srcdir, self.executable)
        utils.system_output(binpath, retain_output=True)
