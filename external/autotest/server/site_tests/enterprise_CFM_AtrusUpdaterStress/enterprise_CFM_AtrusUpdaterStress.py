# Copyright (c) 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Auto test for Atrus firmware updater functionality."""

import logging

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.atrus import atrus_base_test
from autotest_lib.server.cros.atrus import atrus_utils


class enterprise_CFM_AtrusUpdaterStress(atrus_base_test.AtrusBaseTest):
    """
    Atrus firmware updater functionality test in Chrome Box.

    The procedure of the test is:
    1. Trigger forced upgrade of the atrus via atrusctl with dbus.
    2. Wait for the updater to finish, and check status of upgrade.
            The upgrade will be successfull if the transfer of the binary was
            successfull, and if the writing of the binary to flash was
            successfull.
    3. Repeat
    """

    version = 1

    def run_once(self, repeat):
        """Main test procedure."""
        successfull_upgrades = 0

        # Check if Atrusctl is running and have dbus enabled
        if not atrus_utils.check_dbus_available(self._host):
            raise error.TestError('No DBus support in atrusd.')

        for cycle in xrange(repeat):

            atrus_utils.wait_for_atrus_enumeration(self._host)

            if atrus_utils.force_upgrade_atrus(self._host):
                successfull_upgrades += 1

            logging.info('Successful attempts: {}/{}'
                    .format(successfull_upgrades,cycle+1))

        if successfull_upgrades < repeat:
            raise error.TestFail('Upgrade failed in {}/{} of tries.'
                    .format(repeat - successfull_upgrades, repeat))

