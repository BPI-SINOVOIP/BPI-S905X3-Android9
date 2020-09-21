# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.common_lib.cros import autoupdater
from autotest_lib.client.common_lib.cros import dev_server
from autotest_lib.server import afe_utils
from autotest_lib.server import site_utils
from autotest_lib.server import test
from autotest_lib.server.cros.dynamic_suite import frontend_wrappers


class servohost_Reboot(test.test):
    """Enable a safe reboot for a servo host."""
    version = 1

    def run_once(self, host, force_reboot=False):
        """
        Perfom a safe reboot for a servo host.

        A servo host could be used by multiple duts so we need to lock them down
        to ensure they're not running a test that requires the servo.

        @param host: Dut that was designated to kick off the reboot for the
                servo host.
        """
        s_host = host._servo_host
        reboot_needed = force_reboot

        # If we don't have to force reboot, check if we need to reboot at all.
        if not force_reboot:
          servo_host_build = afe_utils.get_stable_cros_image_name(
                  s_host.get_board())
          ds = dev_server.ImageServer.resolve(s_host.hostname)
          url = ds.get_update_url(servo_host_build)
          updater = autoupdater.ChromiumOSUpdater(update_url=url, host=s_host)
          reboot_needed = (updater.check_update_status() ==
                           autoupdater.UPDATER_NEED_REBOOT)
        if reboot_needed:
            # Get the list of duts to lock but take out the current host so we
            # don't wait forever.
            afe = frontend_wrappers.RetryingAFE(timeout_min=5, delay_sec=10)
            dut_list = s_host.get_attached_duts(afe)
            dut_list.remove(host.hostname)

            # Lock the duts and reboot the servo host.
            lock_msg = 'reboot for servo host %s' % s_host.hostname
            with site_utils.lock_duts_and_wait(
                    dut_list, afe, lock_msg=lock_msg) as lock_success:
                logging.info(
                        'status waiting for duts to go idle for '
                        'servo host[%s]: %s', s_host.hostname, lock_success)
                if lock_success:
                    s_host.reboot()
