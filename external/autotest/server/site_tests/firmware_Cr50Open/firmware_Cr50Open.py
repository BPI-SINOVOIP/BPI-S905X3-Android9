# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest


class firmware_Cr50Open(FirmwareTest):
    """Verify cr50 unlock.

    Enable the lock on cr50, run 'lock disable', and then press the power
    button until it is unlocked.
    """
    version = 1

    def initialize(self, host, cmdline_args):
        """Initialize the test"""
        super(firmware_Cr50Open, self).initialize(host, cmdline_args)

        if not hasattr(self, 'cr50'):
            raise error.TestNAError('Test can only be run on devices with '
                                    'access to the Cr50 console')
        if self.cr50.using_ccd():
            raise error.TestNAError('Use a flex cable instead of CCD cable.')

        if not self.cr50.has_command('ccdstate'):
            raise error.TestNAError('Cannot test on Cr50 with old CCD version')


    def run_once(self, ccd_lockout):
        """Lock CCD and then Open it."""
        self.cr50.set_ccd_level('lock')
        try:
            self.cr50.set_ccd_level('open')
            success = True
        except error.TestFail, e:
            logging.debug(e)
            if 'Access Denied' in e.message:
                success = False
            else:
                raise

        ccd_status_str =  'locked out' if ccd_lockout else 'accessible'
        # Make sure we only got 'Access Denied' when ccd is locked out and open
        # was successful only when ccd is accessible.
        if success == ccd_lockout:
            raise error.TestFail('ccd open %sed with ccd %s' % ('succeed'
                    if success else 'fail', ccd_status_str))
        logging.info('ccd open is %s', ccd_status_str)

