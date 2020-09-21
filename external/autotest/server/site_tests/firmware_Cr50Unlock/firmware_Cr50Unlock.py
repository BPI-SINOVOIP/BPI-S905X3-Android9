# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import tpm_utils
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest


class firmware_Cr50Unlock(FirmwareTest):
    """Verify cr50 unlock using the console and gsctool.

    Enable the lock on cr50, run the different forms of unlock, making sure
    cr50 can or cannot be unlocked.

    This does not verify unlock with password.
    """
    version = 1

    def initialize(self, host, cmdline_args):
        """Initialize servo and check that it has access to cr50 with ccd"""
        super(firmware_Cr50Unlock, self).initialize(host, cmdline_args)

        if not hasattr(self, 'cr50'):
            raise error.TestNAError('Test can only be run on devices with '
                                    'access to the Cr50 console')
        if self.cr50.using_ccd():
            raise error.TestNAError('Use a flex cable instead of CCD cable.')

        self.host = host


    def gsctool_unlock(self, unlock_allowed):
        """Unlock cr50 using the gsctool command"""
        result = self.host.run('gsctool -a -U',
                ignore_status=not unlock_allowed)
        if not unlock_allowed and (result.exit_status != 3 or
            'Error: rv 7, response 7' not in result.stderr):
            raise error.TestFail('unexpected lockout result %r', result)
        self.check_unlock(unlock_allowed)


    def console_unlock(self, unlock_allowed):
        """Unlock cr50 using the console command"""
        try:
            self.cr50.set_ccd_level('unlock')
        except error.TestFail, e:
            # The console cannot be used to unlock cr50 unless a password is
            # is set.
            if 'Unlock only allowed after password is set' in e.message:
                logging.info('Unlock unsupported without password')
            else:
                raise
        self.check_unlock(unlock_allowed)


    def check_unlock(self, unlock_allowed):
        """Check that cr50 is unlocked or locked

        Args:
            unlock_allowed: True or False on whether Cr50 should be able to
                            be unlocked.
        Raises:
            TestFail if the cr50 ccd state does not match unlock_allowed
        """
        unlocked = self.cr50.get_ccd_level() == 'unlock'
        if unlocked and not unlock_allowed:
            raise error.TestFail("Cr50 was unlocked when it shouldn't have "
                    "been")
        elif not unlocked and unlock_allowed:
            raise error.TestFail('Cr50 was not unlocked when it should have '
                    'been')


    def unlock_test(self, unlock_func, unlock_allowed):
        """Verify cr50 can or cannot be unlocked with the given unlock_func"""
        self.cr50.set_ccd_level('lock')

        # Clear the TPM owner. The login state can affect unlock abilities
        tpm_utils.ClearTPMOwnerRequest(self.host)

        unlock_func(unlock_allowed)

        # TODO: set password and try to unlock again. Make sure it succeeds.
        # no matter how it is being set.


    def run_once(self, ccd_lockout):
        """Verify cr50 lock behavior on v1 images and v0 images"""
        logging.info('ccd should %sbe locked out',
                '' if ccd_lockout else 'not ')
        if self.cr50.has_command('ccdstate'):
            self.unlock_test(self.gsctool_unlock, not ccd_lockout)
            self.unlock_test(self.console_unlock, False)
            logging.info('ccd unlock is %s', 'locked out' if ccd_lockout else
                    'accessible')
        else:
            # pre-v1, cr50 cannot be unlocked. Make sure that's true
            logging.info(self.cr50.send_command_get_output('lock disable',
                    ['Access Denied\s+Usage: lock']))
            logging.info('Cr50 cannot be unlocked with ccd v0')
