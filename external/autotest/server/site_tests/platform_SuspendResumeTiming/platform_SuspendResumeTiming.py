# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import datetime
import logging
import time

from autotest_lib.client.common_lib import error
from autotest_lib.server import autotest
from autotest_lib.server import test

_POWERD_LOG_PATH = '/var/log/power_manager/powerd.LATEST'
_RESUME_END_LOG = '\"daemon.* Chrome is using normal display mode$\"'
_RESUME_START_LOG = '\"suspender.* Finishing request [0-9]+ successfully$\"'
_SERVO_USB_NUM = 2
_SHORT_WAIT_ = 5
_SUSPEND_END_LOG = '\"suspender.* Starting suspend$\"'
_SUSPEND_START_LOG = '\"suspender.* Starting request [0-9]+$\"'
_SUSPEND_TIME = 15
_TIME_TO_RESUME_BAR = 3
_TIME_TO_SUSPEND_BAR = 3


class platform_SuspendResumeTiming(test.test):
    """Checks suspend and resume happen in reasonable timelines."""
    version = 1


    def cleanup(self):
        """ Disconnect servo hub."""
        self.host.servo.set('dut_hub1_rst1', 'on')
        self.host.servo.set('usb_mux_sel3', 'servo_sees_usbkey')


    def get_suspender_log_stamp(self, pwrd_log):
        """ Reads powerd log and takes suspend and resume logs timestamps.

        @param pwrd_log: log string to search for.

        @raises TestError: if suspender log is met more than once.

        @returns log timestamp as datetime.
        """
        out_log = self.host.run('tac %s | grep -E %s'
                % (_POWERD_LOG_PATH, pwrd_log),
                ignore_status=True).stdout.strip()
        log_count = len(out_log.split('\n'))
        if log_count != 1:
            raise error.TestError('Log \"%s\" is found %d times!'
                                  % (pwrd_log, log_count))
        return datetime.datetime.strptime(out_log[1:12], "%m%d/%H%M%S")


    def get_display_mode_timestamp(self):
        """ Takes the first _RESUME_END_LOG line after _RESUME_START_LOG line
        and returns its timestamp.

        @returns log timestamp as datetime.
        """

        cmd = ('sed -nr \'/%s/,$p\' %s | grep -E %s | head -n 1'
            % (_RESUME_START_LOG.replace("\"",""),
               _POWERD_LOG_PATH, _RESUME_END_LOG))
        out_log = self.host.run(cmd, ignore_status=True).stdout.strip()
        return datetime.datetime.strptime(out_log[1:12], "%m%d/%H%M%S")


    def get_suspend_resume_time(self):
        """ Reads powerd log and takes suspend and resume timestamps.

        @returns times took to suspend and resume as a tuple.

        @raises error.TestError: if timestamps are not sequential.
        """

        suspend_start = self.get_suspender_log_stamp(_SUSPEND_START_LOG)
        suspend_end = self.get_suspender_log_stamp(_SUSPEND_END_LOG)
        resume_start = self.get_suspender_log_stamp(_RESUME_START_LOG)
        resume_end = self.get_display_mode_timestamp()


        logging.info([suspend_start, suspend_end,
                      resume_start, resume_end])

        if not all([resume_end >= resume_start,
                    resume_start > suspend_end,
                    suspend_end >= suspend_start]):
            raise error.TestError('Log timestamps are not sequental!')

        time_to_susp = (suspend_end - suspend_start).total_seconds()
        time_to_res = (resume_end - resume_start).total_seconds()

        return (time_to_susp, time_to_res)


    def get_lsusb_lines(self):
        """ Executes lsusb and returns list of the output lines."""
        output =self.host.run('lsusb', ignore_status=True).stdout
        return output.strip().split('\n')


    def run_once(self, host, plug_usb=False):
        """ Running the suspend-resume timing test.

        @param host: device under test host.
        @param plug_usb: whether to plug extetrnal USB through servo.

        @raises TestFail: if time to suspend to resume exceeds the bar
        and if no peripherals are connected to servo.
        """
        self.host = host
        self.host.servo.set('dut_hub1_rst1', 'on')

        # Reboot to create new powerd.Latest log file.
        self.host.reboot()

        # Test user login.
        autotest_client = autotest.Autotest(self.host)
        autotest_client.run_test("desktopui_SimpleLogin",
                                 exit_without_logout=True)

        # Plug USB hub with peripherals.
        if plug_usb:
            lsusb_unplugged_len = len(self.get_lsusb_lines())
            self.host.servo.switch_usbkey('dut')
            self.host.servo.set('usb_mux_sel3', 'dut_sees_usbkey')
            self.host.servo.set('dut_hub1_rst1', 'off')
            time.sleep(_SHORT_WAIT_)
            lsusb_plugged_len = len(self.get_lsusb_lines())
            if lsusb_plugged_len - lsusb_unplugged_len <  _SERVO_USB_NUM + 1:
                raise error.TestFail('No peripherals are connected to servo!')

        try:
            self.host.suspend(suspend_time=_SUSPEND_TIME)
        except error.AutoservSuspendError:
            pass
        self.host.run('sync')


        errors = []
        time_to_suspend, time_to_resume = self.get_suspend_resume_time()
        if time_to_suspend > _TIME_TO_SUSPEND_BAR:
            errors.append('Suspend time is too long: %d' % time_to_suspend)
        if time_to_resume > _TIME_TO_RESUME_BAR:
            errors.append('Resume time is too long: %d' % time_to_resume)
        if errors:
            raise error.TestFail('; '.join(set(errors)))


