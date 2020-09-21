# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, time

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import utils
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest

# platform_S0ixCycle test timing constants
BEFORE_SUSPEND_WAIT_TIME_SECONDS = 10
BEFORE_RESUME_WAIT_TIME_SECONDS = 2
SUSPEND_WAIT_TIME_SECONDS = 5
LIDOPEN_WAIT_TIME_SECONDS = 2
POWER_STATE_RETRY_COUNT = 10

class platform_S0ixCycle(FirmwareTest):
    '''
    Servo based S0ix cycle test and wake source.
    '''
    version = 1

    def initialize(self, host, cmdline_args):
        dict_args = utils.args_to_dict(cmdline_args)
        self.faft_iterations = int(dict_args.get('faft_iterations', 1))
        super(platform_S0ixCycle, self).initialize(host, cmdline_args)
        self.switcher.setup_mode('normal')

    def perform_s0ix_cycle(self):
        """
        Perform S0ix suspend/resume cycle and check state transition.
        """
        resume_sources = ['powerbtn', 'lid', 'kbpress']
        for resume_source in resume_sources:
            time.sleep(BEFORE_SUSPEND_WAIT_TIME_SECONDS);
            self.perform_suspend()
            self.perform_resume(resume_source)

    def perform_suspend(self):
        """
        Perform suspend to idle and check state transition.
        """
        logging.info('== S0ix suspend and check the state transition ==')
        # check S0ix state transition
        if not self.wait_power_state('S0', POWER_STATE_RETRY_COUNT):
            raise error.TestFail('Platform failed to reach S0 state.')
        self.faft_client.system.run_shell_command('echo freeze > /sys/power/state &')
        time.sleep(SUSPEND_WAIT_TIME_SECONDS);
        # check S0ix state transition
        if not self.wait_power_state('S0ix', POWER_STATE_RETRY_COUNT):
            raise error.TestFail('Platform failed to reach S0ix state.')

    def perform_resume(self, resume_source):
        """
        Perform resume with selected resume source and check state transition.
        @param resume_source(string):resume source option.
        """
        logging.info('== S0ix resume and check the state transition ==')
        time.sleep(BEFORE_RESUME_WAIT_TIME_SECONDS);
        if resume_source == 'powerbtn':
            self.ec.send_command('powerbtn')
        elif resume_source == 'lid':
            self.ec.send_command('lidclose')
            time.sleep(LIDOPEN_WAIT_TIME_SECONDS);
            self.ec.send_command('lidopen')
        elif resume_source == 'kbpress':
            self.ec.key_press('<enter>')
        else:
            raise error.TestFail('Invalid resume source.')
        # check S0 state transition
        if not self.wait_power_state('S0', POWER_STATE_RETRY_COUNT):
            raise error.TestFail('Platform failed to reach S0 state.')

    def is_skl_board(self):
        """
        Check this device is a SKL based ChromeBook.
        """
        skl_boards = ('Kunimitsu', 'Lars', 'Glados', 'Chell', 'Sentry')
        output = self.faft_client.system.get_platform_name()
        return output in skl_boards

    def is_s0ix_supported(self):
        """
        Check this device supports suspend to idle.
        """
        cmd = 'cat /var/lib/power_manager/suspend_to_idle'
        output = self.faft_client.system.run_shell_command_get_output(cmd)
        if not output:
            return False
        else:
            return int(output[0]) == 1

    def run_once(self):
        if not self.faft_config.chrome_ec or not self.check_ec_capability():
            raise error.TestNAError('Chrome EC is not supported on this device.')

        if not (self.is_skl_board() and self.is_s0ix_supported()):
            raise error.TestNAError('Suspend to idle is not supported on this device.')

        for i in xrange(self.faft_iterations):
            logging.info('== Running FAFT ITERATION %d/%s ==',i+1, self.faft_iterations)
            logging.info('S0ix suspend/resume back and check state transition.')
            # wake the display by key press.
            self.ec.key_press('<enter>')
            self.switcher.mode_aware_reboot('custom', self.perform_s0ix_cycle)

    def cleanup(self):
        self.ec.set_uart_regexp('None')
        # Test may failed before resume, wake the system.
        self.ec.send_command('powerbtn')
        # Perform a warm reboot as part of the cleanup.
        self._client.reboot()
        super(platform_S0ixCycle, self).cleanup()
