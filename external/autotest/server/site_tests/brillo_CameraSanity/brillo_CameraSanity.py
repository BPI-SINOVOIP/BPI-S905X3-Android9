# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.brillo import hal_utils
from autotest_lib.server import test


class brillo_CameraSanity(test.test):
    """Test the most basic camera functionality on a Brillo DUT."""
    version = 1


    def run_once(self, host):
        """Check for things that should look the same on all Brillo devices.

        @param host: host object representing the device under test.

        """
        if not hal_utils.has_hal('camera', host=host):
            raise error.TestNAError(
                    'This device does not appear to support camera at all')

        host.run('/system/bin/brillo_camera_client')
