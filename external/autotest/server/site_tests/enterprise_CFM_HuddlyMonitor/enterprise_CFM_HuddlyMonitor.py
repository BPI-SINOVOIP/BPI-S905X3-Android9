# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import time

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import dbus_send
from autotest_lib.server import test
from autotest_lib.server.cros.cfm import cfm_base_test
from autotest_lib.server.cros.multimedia import remote_facade_factory

LONG_TIMEOUT = 20
SHORT_TIMEOUT = 1
OBJ_INTERFACE = "org.chromium.huddlymonitor"

class enterprise_CFM_HuddlyMonitor(cfm_base_test.CfmBaseTest):
    """ Autotests for huddly-monitor, within cfm-device-monitor.

    All autotests involve being in a cfm meeting, without loss of generality.

    All test scenarios are in a function of their own, with the explanation
    as docstring.
    """
    version = 1

    def is_monitor_alive(self):
        """Check if huddly-monitor is alive and registered on Dbus."""
        result = dbus_send.dbus_send("org.freedesktop.DBus",
                  "org.freedesktop.DBus",
                  "/org/freedesktop/DBus",
                  "ListNames",
                  None,
                  self._host,
                  2,
                  False,
                  "cfm-monitor")
        return OBJ_INTERFACE in result.response

    def fake_error_should_remediate(self):
        """ Enter an error message in kernel log buffer. Wait to see if
        monitor detects it and remediates the camera accordingly """
        err_msg = "sudo echo \"<3>uvcvideo: Failed AUTOTEST\" >> /dev/kmsg"

        self._host.run(err_msg)

        # Wait till camera reboots
        time.sleep(LONG_TIMEOUT)
        # Make sure camera is turned on
        self.cfm_facade.unmute_camera()

        # Check if camera operational
        if(self.cfm_facade.is_camera_muted()):
            raise error.TestFail("Camera still not functional.")

    def fake_error_monitor_sleeping_no_action(self):
        """Enter an error message in kernel log buffer when monitor is
        sleeping. Make sure it does not detect it."""
        err_msg = "sudo echo \"<3>uvcvideo: Failed AUTOTEST\" >> /dev/kmsg"

        # Force-sleep monitor
        self._host.run("/usr/bin/huddlymonitor_update false")

        # Fake error
        self._host.run(err_msg)

        # Check camera is not hotplugged, since monitor asleep
        if(self.cfm_facade.is_camera_muted()):
            raise error.TestFail("Should not have hotplug.")


    def monitor_woke_detect_earlier_error(self):
        """Wake up monitor. Check to see if it detects an error message
        that was entered earlier. This test assumes there already was an
        error message in the kernel log buffer. Typically used after
        fake_error_monitor_sleeping_no_action."""
        # Wake up monitor
        self._host.run("/usr/bin/huddlymonitor_update true")

        # Wait till camera reboots, takes some time for monitor to wake up.
        time.sleep(LONG_TIMEOUT)

        self.cfm_facade.unmute_camera()

        # Check if camera operational
        if(self.cfm_facade.is_camera_muted()):
            raise error.TestFail("Camera still not functional.")

    def monitor_skip_unrelated_error(self):
        """Enter a bogus kernel message. Check to see if the monitor detects
        it. """
        # Send error message not intended for monitor
        self._host.run("sudo echo \"<4>uvcvideo: Failed \" >> /dev/kmsg")

        # Make sure no action was taken
        if(self.cfm_facade.is_camera_muted()):
            raise error.Testfail("Should not have hotplug")

    def run_once(self):
        """Run the autotest suite once."""

        self.cfm_facade.wait_for_meetings_telemetry_commands()
        self.cfm_facade.start_meeting_session()
        # Quit early if monitor is not alive.
        result = self.is_monitor_alive()
        if(not result):
            raise error.TestFail("Monitor not alive")

        # Enough time to enter meeting
        time.sleep(SHORT_TIMEOUT)

        self.fake_error_should_remediate()
        self.fake_error_monitor_sleeping_no_action()
        self.monitor_woke_detect_earlier_error()
        self.monitor_skip_unrelated_error()
