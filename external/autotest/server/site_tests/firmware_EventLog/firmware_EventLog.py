# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, re, time

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros import vboot_constants as vboot
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest
from autotest_lib.server.cros.watchdog_tester import WatchdogTester

POWER_DIR = '/var/lib/power_manager'
TMP_POWER_DIR = '/tmp/power_manager'

class firmware_EventLog(FirmwareTest):
    """
    Test to ensure eventlog is written on boot and suspend/resume.
    """
    version = 1

    _TIME_FORMAT = '%Y-%m-%d %H:%M:%S'

    def initialize(self, host, cmdline_args):
        super(firmware_EventLog, self).initialize(host, cmdline_args)
        self.host = host
        self.switcher.setup_mode('normal')
        self.setup_usbkey(usbkey=True, host=False)

    def _has_event(self, pattern):
        return bool(filter(re.compile(pattern).search, self._events))

    def _gather_events(self):
        entries = self.faft_client.system.run_shell_command_get_output(
                'mosys eventlog list')
        now = self._now()
        self._events = []
        for line in reversed(entries):
            _, time_string, event = line.split(' | ', 2)
            timestamp = time.strptime(time_string, self._TIME_FORMAT)
            if timestamp > now:
                logging.error('Found prophecy: "%s"', line)
                raise error.TestFail('Event timestamp lies in the future')
            if timestamp < self._cutoff_time:
                logging.debug('Event "%s" too early, stopping search', line)
                break
            logging.info('Found event: "%s"', line)
            self._events.append(event)

    # This assumes that Linux and the firmware use the same RTC. mosys converts
    # timestamps to localtime, and so do we (by calling date without --utc).
    def _now(self):
        time_string = self.faft_client.system.run_shell_command_get_output(
                'date +"%s"' % self._TIME_FORMAT)[0]
        logging.debug('Current local system time on DUT is "%s"', time_string)
        return time.strptime(time_string, self._TIME_FORMAT)

    def disable_suspend_to_idle(self):
        """Disable the powerd preference for suspend_to_idle."""
        logging.info('Disabling suspend_to_idle')
        # Make temporary directory to hold powerd preferences so we
        # do not leave behind any state if the test is aborted.
        self.host.run('mkdir -p %s' % TMP_POWER_DIR)
        self.host.run('echo 0 > %s/suspend_to_idle' % TMP_POWER_DIR)
        self.host.run('mount --bind %s %s' % (TMP_POWER_DIR, POWER_DIR))
        self.host.run('restart powerd')

    def teardown_powerd_prefs(self):
        """Clean up custom powerd preference changes."""
        self.host.run('umount %s' % POWER_DIR)
        self.host.run('restart powerd')

    def run_once(self):
        if not self.faft_config.has_eventlog:
            raise error.TestNAError('This board has no eventlog support.')

        logging.info('Verifying eventlog behavior on normal mode boot')
        self._cutoff_time = self._now()
        self.switcher.mode_aware_reboot()
        self.check_state((self.checkers.crossystem_checker, {
                              'devsw_boot': '0',
                              'mainfw_type': 'normal',
                              }))
        self._gather_events()
        if not self._has_event(r'System boot'):
            raise error.TestFail('No "System boot" event on normal boot.')
        # ' Wake' to match 'FW Wake' and 'ACPI Wake' but not 'Wake Source'
        if self._has_event(r'Developer Mode|Recovery Mode|Sleep| Wake'):
            raise error.TestFail('Incorrect event logged on normal boot.')

        logging.debug('Transitioning to dev mode for next test')
        self.switcher.reboot_to_mode(to_mode='dev')

        logging.info('Verifying eventlog behavior on developer mode boot')
        self._cutoff_time = self._now()
        self.switcher.mode_aware_reboot()
        self.check_state((self.checkers.crossystem_checker, {
                              'devsw_boot': '1',
                              'mainfw_type': 'developer',
                              }))
        self._gather_events()
        if (not self._has_event(r'System boot') or
            not self._has_event(r'Chrome OS Developer Mode')):
            raise error.TestFail('Missing required event on dev mode boot.')
        if self._has_event(r'Recovery Mode|Sleep| Wake'):
            raise error.TestFail('Incorrect event logged on dev mode boot.')

        logging.debug('Transitioning back to normal mode for final tests')
        self.switcher.reboot_to_mode(to_mode='normal')

        logging.info('Verifying eventlog behavior in recovery mode')
        self._cutoff_time = self._now()
        self.switcher.reboot_to_mode(to_mode='rec')
        logging.debug('Check we booted into recovery')
        self.check_state((self.checkers.crossystem_checker, {
                         'mainfw_type': 'recovery',
                         'recovery_reason' : vboot.RECOVERY_REASON['RO_MANUAL'],
                         }))
        self.switcher.mode_aware_reboot()
        self.check_state((self.checkers.crossystem_checker, {
                              'devsw_boot': '0',
                              'mainfw_type': 'normal',
                              }))
        self._gather_events()
        if (not self._has_event(r'System boot') or
            not self._has_event(r'Chrome OS Recovery Mode \| Recovery Button')):
            raise error.TestFail('Missing required event in recovery mode.')
        if self._has_event(r'Developer Mode|Sleep|FW Wake|ACPI Wake \| S3'):
            raise error.TestFail('Incorrect event logged in recovery mode.')

        logging.info('Verifying eventlog behavior on suspend/resume')
        self.disable_suspend_to_idle()
        self._cutoff_time = self._now()
        self.faft_client.system.run_shell_command(
                'powerd_dbus_suspend -wakeup_timeout=10')
        time.sleep(5)   # a little slack time for powerd to write the 'Wake'
        self.teardown_powerd_prefs()
        self._gather_events()
        if ((not self._has_event(r'^Wake') or not self._has_event(r'Sleep')) and
            (not self._has_event(r'ACPI Enter \| S3') or
             not self._has_event(r'ACPI Wake \| S3'))):
            raise error.TestFail('Missing required event on suspend/resume.')
        if self._has_event(r'System |Developer Mode|Recovery Mode'):
            raise error.TestFail('Incorrect event logged on suspend/resume.')

        watchdog = WatchdogTester(self.host)
        if not watchdog.is_supported():
            logging.info('No hardware watchdog on this platform, skipping')
        elif self.faft_client.system.run_shell_command_get_output(
                'crossystem arch')[0] != 'x86': # TODO: Implement event on x86
            logging.info('Verifying eventlog behavior with hardware watchdog')
            self._cutoff_time = self._now()
            with watchdog:
                watchdog.trigger_watchdog()
            self._gather_events()
            if (not self._has_event(r'System boot') or
                not self._has_event(r'Hardware watchdog reset')):
                raise error.TestFail('Did not log hardware watchdog event')
