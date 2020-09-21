# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
from autotest_lib.client.common_lib.cros import tpm_utils
from autotest_lib.server import test, autotest

class platform_CryptohomeTPMReOwnServer(test.test):
    """
    The server-side controller for verifying that cryptohome can re-create a
    user's vault if the TPM is cleared and re-owned.
    """
    version = 1
    n_client_reboots = 0
    client_at = None

    # Run the client subtest named [subtest].
    def tpm_run(self, subtest, ignore_status=False):
        self.client_at.run_test(self.client_test,
                                subtest=subtest,
                                check_client_result=(not ignore_status))


    def reboot_client(self):
        # Reboot the client
        logging.info('CryptohomeTPMReOwnServer: rebooting %s number %d',
                     self.client.hostname, self.n_client_reboots)
        self.client.reboot()
        self.n_client_reboots += 1


    def run_once(self, host=None):
        self.client = host
        self.client_at = autotest.Autotest(self.client)
        self.client_test = 'platform_CryptohomeTPMReOwn'

        # Set up the client in the unowned state and init the TPM again.
        tpm_utils.ClearTPMOwnerRequest(self.client)
        self.tpm_run("take_tpm_ownership", ignore_status=True)

        self.tpm_run("mount_cryptohome")

        self.reboot_client()
        self.tpm_run("mount_cryptohome_after_reboot")

        # Clear and re-own the TPM on the next boot.
        tpm_utils.ClearTPMOwnerRequest(self.client)
        self.tpm_run("take_tpm_ownership", ignore_status=True)

        self.tpm_run("mount_cryptohome_check_recreate")
