# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import cryptohome


class firmware_SetFWMP(test.test):
    """Set the FWMP flags and dev_key_hash."""
    version = 1

    def own_tpm(self):
        """Own the TPM"""
        cryptohome.take_tpm_ownership()
        for i in range(4):
            status = cryptohome.get_tpm_status()
            if status['Owned']:
                return status
            time.sleep(2)
        raise error.TestFail('Failed to own the TPM %s' % status)

    def run_once(self, fwmp_cleared=True, flags=None, dev_key_hash=None):
        # make sure the FMWP is in the expected state
        cryptohome.get_fwmp(fwmp_cleared)
        status = cryptohome.get_tpm_status()
        # Own the TPM
        if not status['Owned']:
            status = self.own_tpm()

        # Verify we have access to the password
        if not status['Password']:
            logging.warning('No access to the password')

        logging.info(status)

        # Set the FWMP flags using a dev key hash
        cryptohome.set_fwmp(flags, dev_key_hash)

        # Check that the flags are set
        fwmp = cryptohome.get_fwmp()
        if flags and fwmp['flags'] != str(int(flags, 16)):
            raise error.TestFail('Unexpected FWMP status: %s', fwmp)
