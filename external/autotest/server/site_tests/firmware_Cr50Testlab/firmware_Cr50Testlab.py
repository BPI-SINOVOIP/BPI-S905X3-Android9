# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest


class firmware_Cr50Testlab(FirmwareTest):
    """Verify cr50 testlab enable/disable."""
    version = 1
    ACCESS_DENIED = 'Access Denied'
    INVALID_PARAM = 'Parameter 1 invalid'
    BASIC_ERROR = 'Usage: ccd '

    def initialize(self, host, cmdline_args):
        """Initialize servo. Check that it can access cr50"""
        super(firmware_Cr50Testlab, self).initialize(host, cmdline_args)

        if not hasattr(self, 'cr50'):
            raise error.TestNAError('Test can only be run on devices with '
                                    'access to the Cr50 console')
        if self.cr50.using_ccd():
            raise error.TestNAError('Use a flex cable instead of CCD cable.')

        if not self.cr50.has_command('ccdstate'):
            raise error.TestNAError('Cannot test on Cr50 with old CCD version')

        # Get the current reset count, so we can check that there haven't been
        # any cr50 resets at any point during the test.
        self.start_reset_count = self.servo.get('cr50_reset_count')


    def try_testlab(self, mode, err=''):
        """Try to modify ccd testlab mode.

        Args:
            mode: The testlab command: 'on', 'off', or 'open'
            err: An empty string if the command should succeed or the error
                 message.

        Raises:
            TestFail if setting the ccd testlab mode doesn't match err
        """
        logging.info('Setting ccd testlab %s', mode)
        rv = self.cr50.send_command_get_output('ccd testlab %s' % mode,
                ['.*>'])[0]
        logging.info(rv)
        if err not in rv or (not err and self.BASIC_ERROR in rv):
            raise error.TestFail('Unexpected result setting "%s": %r' % (mode,
                    rv))
        if err:
            return

        if mode == 'open':
            if mode != self.cr50.get_ccd_level():
                raise error.TestFail('ccd testlab open did not open the device')
        else:
            self.cr50.run_pp(self.cr50.PP_SHORT)
            if (mode == 'on') != self.cr50.testlab_is_on():
                raise error.TestFail('Testlab mode could not be turned %s' %
                        mode)
            logging.info('Set ccd testlab %s', mode)


    def check_reset_count(self):
        """Verify there haven't been any cr50 reboots"""
        reset_count = self.servo.get('cr50_reset_count')
        if self.start_reset_count != reset_count:
            raise error.TestFail('Unexpected cr50 reboot')


    def reset_ccd(self):
        """Enable ccd testlab mode and set the privilege level to open"""
        logging.info('Resetting CCD state')
        # If testlab mode is enabled, use that to open ccd. It is a lot faster.
        if self.cr50.testlab_is_on():
            self.try_testlab('open')
        else:
            self.cr50.set_ccd_level('open')
        self.try_testlab('on')
        self.check_reset_count()


    def run_once(self, ccd_lockout):
        """Try to set testlab mode from different privilege levels."""
        # ccd testlab can only be enabled after ccd is opened. This test wont
        # do much if we can't open the device. firmware_Cr50Open should be
        # enough to test ccd open capabilities. Do a basic test to make sure
        # testlab mode can't be enabled while the device is locked, then raise
        # test NA error.
        if ccd_lockout:
            self.cr50.set_ccd_level('lock')
            self.try_testlab('on', err=self.ACCESS_DENIED)
            raise error.TestNAError('Skipping firmware_Cr50Testlab when ccd is '
                    'locked out.')

        # Dummy isn't a valid mode. Make sure it fails
        self.reset_ccd()
        self.try_testlab('dummy', err=self.INVALID_PARAM)

        # If ccd is locked, ccd testlab dummy should fail with access denied not
        # invalid param.
        self.reset_ccd()
        self.cr50.set_ccd_level('lock')
        self.try_testlab('dummy', err=self.ACCESS_DENIED)

        # CCD can be opened without physical presence if testlab mode is enabled
        self.reset_ccd()
        self.try_testlab('on')
        self.try_testlab('open')
        self.check_reset_count()

        # You shouldn't be able to use testlab open if it is disabled
        self.reset_ccd()
        self.try_testlab('off')
        self.try_testlab('open', err=self.ACCESS_DENIED)
        self.check_reset_count()

        # You can't turn on testlab mode while ccd is locked
        self.reset_ccd()
        self.cr50.set_ccd_level('lock')
        self.try_testlab('on', err=self.ACCESS_DENIED)
        self.check_reset_count()

        # You can't turn off testlab mode while ccd is locked
        self.reset_ccd()
        self.cr50.set_ccd_level('lock')
        self.try_testlab('off', err=self.ACCESS_DENIED)
        self.check_reset_count()

        # If testlab mode is enabled, you can open the device without physical
        # presence by using 'ccd testlab open'.
        self.reset_ccd()
        self.try_testlab('on')
        self.cr50.set_ccd_level('lock')
        self.try_testlab('open')
        self.check_reset_count()

        # If testlab mode is disabled, testlab open should fail with access
        # denied.
        self.reset_ccd()
        self.try_testlab('off')
        self.cr50.set_ccd_level('lock')
        self.try_testlab('open', err=self.ACCESS_DENIED)
        self.check_reset_count()
        logging.info('ccd testlab is accessbile')
