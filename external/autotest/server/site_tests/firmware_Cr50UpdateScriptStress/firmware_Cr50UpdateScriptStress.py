# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.common_lib.cros import cr50_utils
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest


class firmware_Cr50UpdateScriptStress(FirmwareTest):
    """
    Stress the Cr50 Update Script.

    Clear the update state to force the device to retry the cr50 update. This
    will not update cr50. It will just force the cr50-update script to get
    the version and verify an update is not needed. If there are any reboots
    caused by the update verification, then we know something failed.

    This test is intended to be run with many iterations to ensure that the
    update process is not flaky.
    """
    version = 1

    def run_once(self, host, cmdline_args):
        """Run the cr50 update script and make sure there aren't errors"""
        # Find the last cr50 update message already in /var/log/messages
        last_message = cr50_utils.CheckForFailures(host, '')

        # Clears the state and reboots the system to get the cr50-update to run
        cr50_utils.ClearUpdateStateAndReboot(host)

        cr50_utils.CheckForFailures(host, last_message)
