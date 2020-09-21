# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.server.cros.cfm import cfm_base_test


class enterprise_CFM_RebootStress(cfm_base_test.CfmBaseTest):
    """
    Stress tests the CFM enrolled device by rebooting it multiple times using
    Chrome runtime restart() API and ensuring the packaged app launches as
    expected after every reboot.
    """
    version = 1


    def run_once(self, reboot_cycles, is_meeting):
        """
        Runs the test.

        @param reboot_cycles: The amount of times to reboot the DUT.
        @is_meeting: True for Hangouts Meet, False for classic Hangouts.
        """
        logging.info("Performing in total %d reboot cycles...", reboot_cycles)
        for cycle in range(reboot_cycles):
            logging.info("Started reboot cycle %d.", cycle)
            boot_id = self._host.get_boot_id()
            if is_meeting:
                self.cfm_facade.wait_for_meetings_landing_page()
            else:
                self.cfm_facade.wait_for_hangouts_telemetry_commands()
            self.cfm_facade.reboot_device_with_chrome_api()
            self._host.wait_for_restart(old_boot_id=boot_id)
            self.cfm_facade.restart_chrome_for_cfm()

