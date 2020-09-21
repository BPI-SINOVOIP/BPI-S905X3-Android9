# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, time

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import utils
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest

# platform_S3Cycle test timing constants
BEFORE_SUSPEND_WAIT_TIME_SECONDS = 10
BEFORE_RESUME_WAIT_TIME_SECONDS = 2
SUSPEND_WAIT_TIME_SECONDS = 5
LIDOPEN_WAIT_TIME_SECONDS = 2
POWER_STATE_RETRY_COUNT = 10

class platform_S3Cycle(FirmwareTest):
    '''
    Servo based S3 cycle test.
    '''
    version = 1

    def initialize(self, host, cmdline_args):
        dict_args = utils.args_to_dict(cmdline_args)
        self.faft_iterations = int(dict_args.get('faft_iterations', 1))
        super(platform_S3Cycle, self).initialize(host, cmdline_args)
        self.switcher.setup_mode('normal')

    def perform_s3_cycle(self):
        """
        Perform S3 suspend/resume cycle and check state transition.
        """
        resume_sources = ['powerbtn', 'lid', 'kbpress']
        for resume_source in resume_sources:
            time.sleep(BEFORE_SUSPEND_WAIT_TIME_SECONDS);
            self.perform_suspend()
            self.perform_resume(resume_source)

    def perform_suspend(self):
        """
        Perform suspend to mem and check state transition.
        """
        logging.info('== S3 suspend and check the state transition ==')
        # check S0 state transition
        if not self.wait_power_state('S0', POWER_STATE_RETRY_COUNT):
            raise error.TestFail('Platform failed to reach S0 state.')
        self.faft_client.system.run_shell_command('echo mem > /sys/power/state &')
        time.sleep(SUSPEND_WAIT_TIME_SECONDS);
        # check S3 state transition
        if not self.wait_power_state('S3', POWER_STATE_RETRY_COUNT):
            raise error.TestFail('Platform failed to reach S3 state.')

    def perform_resume(self, resume_source):
        """
        Perform resume with selected resume source and check state transition.
        @param resume_source(string):resume source option.
        """
        logging.info('== S3 resume and check the state transition ==')
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

    def run_once(self):
        if not self.faft_config.chrome_ec or not self.check_ec_capability():
            raise error.TestNAError('Chrome EC is not supported on this device.')

        for i in xrange(self.faft_iterations):
            logging.info('== Running FAFT ITERATION %d/%s ==',i+1, self.faft_iterations)
            logging.info('S3 suspend/resume back and check state transition.')
            self.switcher.mode_aware_reboot('custom', self.perform_s3_cycle)

    def cleanup(self):
        self.ec.set_uart_regexp('None')
        # Test may failed before resume, wake the system.
        self.ec.send_command('powerbtn')
        # Perform a warm reboot as part of the cleanup.
        self._client.reboot()
        super(platform_S3Cycle, self).cleanup()
