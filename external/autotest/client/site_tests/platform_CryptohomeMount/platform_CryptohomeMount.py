# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import cryptohome

class platform_CryptohomeMount(test.test):
    """Validates basic cryptohome creation and mounting."""
    version = 1


    def run_once(self):
        test_user = 'this_is_a_local_test_account@chromium.org';
        test_password = 'this_is_a_test_password';

        # Remove the test user account (if it exists), create it and
        # mount it
        cryptohome.ensure_clean_cryptohome_for(test_user, test_password)

        # Unmount the vault and ensure it's not there
        cryptohome.unmount_vault(test_user)

        # Make sure that an incorrect password fails
        incorrect_password = 'this_is_an_incorrect_password'
        try:
            cryptohome.mount_vault(test_user, incorrect_password)
        except:
            pass
        else:
            raise error.TestFail('Cryptohome mounted with a bad password')

        # Ensure that the user directory is not mounted
        if cryptohome.is_permanent_vault_mounted(test_user, allow_fail=True):
            raise error.TestFail('Cryptohome mounted even though mount failed')

        # Remove the test user account
        cryptohome.remove_vault(test_user)
