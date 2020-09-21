# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error, utils
from autotest_lib.client.cros import cryptohome


def run_cmd(cmd):
    return utils.system_output(cmd + ' 2>&1', retain_output=True,
                               ignore_status=True)

class platform_CryptohomeChangePassword(test.test):
    version = 1


    def run_once(self):
        test_user = 'this_is_a_local_test_account@chromium.org'
        test_password = 'this_is_a_test_password'

        # Remove any old test user account
        cryptohome.remove_vault(test_user)

        # Create a fresh test user account
        cryptohome.mount_vault(test_user, test_password, create=True)
        cryptohome.unmount_vault(test_user)

        # Try to migrate the password
        new_password = 'this_is_a_new_password'
        cryptohome.change_password(test_user, test_password, new_password)

        # Mount the test user account with the new password
        cryptohome.mount_vault(test_user, new_password)
        cryptohome.unmount_vault(test_user)

        # Ensure the old password doesn't work
        try:
            cryptohome.mount_vault(test_user, test_password)
        except cryptohome.ChromiumOSError:
            pass
        else:
            raise error.TestFail("Mount with old password worked")

        # Remove the test user account
        cryptohome.remove_vault(test_user)
