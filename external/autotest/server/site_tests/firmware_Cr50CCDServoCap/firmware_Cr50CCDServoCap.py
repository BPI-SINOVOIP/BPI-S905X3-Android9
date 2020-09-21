# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import pprint
import time

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest


class firmware_Cr50CCDServoCap(FirmwareTest):
    """Verify Cr50 CCD output enable/disable when servo is connected.

    Verify Cr50 will enable/disable the CCD servo output capabilities when servo
    is attached/detached.
    """
    version = 1

    # Time used to wait for Cr50 to detect the servo state
    SLEEP = 2

    # A list of the actions we should verify
    TEST_CASES = [
        'fake_servo on, cr50_run reboot',
        'fake_servo on, rdd attach, cr50_run reboot',

        'rdd attach, fake_servo on, cr50_run reboot, fake_servo off',
        'rdd attach, fake_servo on, rdd detach',
        'rdd attach, fake_servo off, rdd detach',
    ]
    # Used to reset the servo and ccd state after the test.
    CLEANUP = 'fake_servo on, rdd attach'

    ON = 'on'
    OFF = 'off'
    UNDETECTABLE = 'undetectable'
    # There are many valid CCD state strings. These lists define which strings
    # translate to off, on and unknown.
    STATE_VALUES = {
        OFF : ['off', 'disconnected', 'disabled', 'UARTAP UARTEC',
                'UARTAP UARTEC I2C'],
        ON : ['on', 'connected', 'enabled', 'UARTAP+TX UARTEC+TX I2C SPI'],
        UNDETECTABLE : ['undetectable'],
    }
    # RESULT_ORDER is a list of the CCD state strings. The order corresponds
    # with the order of the key states in EXPECTED_RESULTS.
    RESULT_ORDER = ['State flags', 'CCD EXT', 'Servo']
    # A dictionary containing an order of steps to verify and the expected ccd
    # states as the value.
    #
    # The keys are a list of strings with the order of steps to run.
    #
    # The values are the expected state of [state flags, ccd ext, servo]. The
    # ccdstate strings are in RESULT_ORDER. The order of the EXPECTED_RESULTS
    # key states must match the order in RESULT_ORDER.
    #
    # There are three valid states: UNDETECTABLE, ON, or OFF. Undetectable only
    # describes the servo state when EC uart is enabled. If the ec uart is
    # enabled, cr50 cannot detect servo and the state becomes undetectable. All
    # other ccdstates can only be off or on. Cr50 has a lot of different words
    # for off and on. These other descriptors are in STATE_VALUES.
    EXPECTED_RESULTS = {
        # The state all tests will start with. Servo and the ccd cable are
        # disconnected.
        'reset_ccd state' : [OFF, OFF, OFF],

        # If rdd is attached all ccd functionality will be enabled, and servo
        # will be undetectable.
        'rdd attach' : [ON, ON, UNDETECTABLE],

        # Cr50 cannot detect servo if ccd has been enabled first
        'rdd attach, fake_servo off' : [ON, ON, UNDETECTABLE],
        'rdd attach, fake_servo off, rdd detach' : [OFF, OFF, OFF],
        'rdd attach, fake_servo on' : [ON, ON, UNDETECTABLE],
        'rdd attach, fake_servo on, rdd detach' : [OFF, OFF, ON],
        # Cr50 can detect servo after a reboot even if rdd was attached before
        # servo.
        'rdd attach, fake_servo on, cr50_run reboot' : [OFF, ON, ON],
        # Once servo is detached, Cr50 will immediately reenable the EC uart.
        'rdd attach, fake_servo on, cr50_run reboot, fake_servo off' :
            [ON, ON, UNDETECTABLE],

        # Cr50 can detect a servo attach
        'fake_servo on' : [OFF, OFF, ON],
        # Cr50 knows servo is attached when ccd is enabled, so it wont enable
        # uart.
        'fake_servo on, rdd attach' : [OFF, ON, ON],
        'fake_servo on, rdd attach, cr50_run reboot' : [OFF, ON, ON],
        'fake_servo on, cr50_run reboot' : [OFF, OFF, ON],
    }


    def initialize(self, host, cmdline_args):
        super(firmware_Cr50CCDServoCap, self).initialize(host, cmdline_args)
        if not hasattr(self, 'cr50'):
            raise error.TestNAError('Test can only be run on devices with '
                                    'access to the Cr50 console')

        if self.servo.get_servo_version() != 'servo_v4_with_servo_micro':
            raise error.TestNAError('Must use servo v4 with servo micro')

        if not self.cr50.has_command('ccdstate'):
            raise error.TestNAError('Cannot test on Cr50 with old CCD version')

        self._original_testlab_state = self.servo.get('cr50_testlab')
        self.cr50.set_ccd_testlab('on')
        if not self.cr50.testlab_is_on():
            raise error.TestNAError('Cr50 testlab mode needs to be enabled')

        self._orignal_ccdstate = self.get_ccdstate()

        if not self.cr50.servo_v4_supports_dts_mode():
            raise error.TestNAError('Need working servo v4 DTS control')


    def cleanup(self):
        """Disable CCD and reenable the EC uart"""
        if hasattr(self, '_orignal_testlab_state'):
            self.cr50.set_ccd_testlab(self._original_testlab_state)
        if (hasattr(self, '_orignal_ccdstate') and
            self._orignal_ccdstate != self.get_ccdstate()):
            self.reset_ccd()
            self.run_steps(self.CLEANUP)

        super(firmware_Cr50CCDServoCap, self).cleanup()


    def get_ccdstate(self):
        """Get the current Cr50 CCD states"""
        self.cr50.send_command('ccd testlab open')
        rv = self.cr50.send_command_get_output('ccdstate',
            ['ccdstate(.*)>'])[0][1].split('\n')
        ccdstate = {}
        for line in rv:
            line = line.strip()
            if line:
                k, v = line.split(':', 1)
                ccdstate[k.strip()] = v.strip()
        logging.info('Current CCD state:\n%s', pprint.pformat(ccdstate))
        return ccdstate


    def verify_ccdstate(self, run):
        """Verify the current state matches the expected result from the run.

        Args:
            run: the string representing the actions that have been run.

        Raises:
            TestError if any of the states are not correct
        """
        if run not in self.EXPECTED_RESULTS:
            raise error.TestError('Add results for %s to EXPECTED_RESULTS', run)
        expected_states = self.EXPECTED_RESULTS[run]
        mismatch = []
        ccdstate = self.get_ccdstate()
        for i, expected_state in enumerate(expected_states):
            name = self.RESULT_ORDER[i]
            actual_state = ccdstate[name]
            valid_values = self.STATE_VALUES[expected_state]
            # Check that the current state is one of the valid states.
            if actual_state not in valid_values:
                mismatch.append('%s: "%s" not in "%s"' % (name, actual_state,
                    ', '.join(valid_values)))
        if mismatch:
            raise error.TestFail('Unexpected states after %s: %s' % (run,
                mismatch))


    def cr50_run(self, action):
        """Reboot cr50

        @param action: string 'reboot'
        """
        if action == 'reboot':
            self.cr50.reboot()
            self.cr50.send_command('ccd testlab open')
            time.sleep(self.SLEEP)


    def reset_ccd(self, state=None):
        """detach the ccd cable and disconnect servo.

        State is ignored. It just exists to be consistent with the other action
        functions.

        @param state: a var that is ignored
        """
        self.rdd('detach')
        self.fake_servo('off')


    def rdd(self, state):
        """Attach or detach the ccd cable.

        @param state: string 'attach' or 'detach'
        """
        self.servo.set_nocheck('servo_v4_dts_mode',
            'on' if state == 'attach' else 'off')
        time.sleep(self.SLEEP)


    def fake_servo(self, state):
        """Mimic servo on/off

        Cr50 monitors the servo EC uart tx signal to detect servo. If the signal
        is pulled up, then Cr50 will think servo is connnected. Enable the ec
        uart to enable the pullup. Disable the it to remove the pullup.

        It takes some time for Cr50 to detect the servo state so wait 2 seconds
        before returning.
        """
        self.servo.set('ec_uart_en', state)

        # Cr50 needs time to detect the servo state
        time.sleep(self.SLEEP)


    def run_steps(self, steps):
        """Do each step in steps and then verify the uart state.

        The uart state is order dependent, so we need to know all of the
        previous steps to verify the state. This will do all of the steps in
        the string and verify the Cr50 CCD uart state after each step.

        @param steps: a comma separated string with the steps to run
        """
        # The order of steps is separated by ', '. Remove the last step and
        # run all of the steps before it.
        separated_steps = steps.rsplit(', ', 1)
        if len(separated_steps) > 1:
            self.run_steps(separated_steps[0])

        step = separated_steps[-1]
        # The func and state are separated by ' '
        func, state = step.split(' ')
        logging.info('running %s', step)
        getattr(self, func)(state)

        # Verify the ccd state is correct
        self.verify_ccdstate(steps)


    def run_once(self):
        """Run through TEST_CASES and verify that Cr50 enables/disables uart"""
        for steps in self.TEST_CASES:
            self.run_steps('reset_ccd state')
            logging.info('TESTING: %s', steps)
            self.run_steps(steps)
            logging.info('VERIFIED: %s', steps)
