# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import cryptohome

class platform_CryptohomeMigrateKey(test.test):
    version = 1

    def good(self):
        user = utils.random_username()
        old_pass = 'old'
        new_pass = 'new'

        cryptohome.mount_vault(user, old_pass, create=True)
        cryptohome.unmount_vault(user)
        cryptohome.change_password(user, old_pass, new_pass)
        try:
            cryptohome.mount_vault(user, old_pass)
        except:
            pass
        else:
            raise error.TestFail('Old password still works.')
        cryptohome.mount_vault(user, new_pass)
        cryptohome.unmount_vault(user)
        cryptohome.remove_vault(user)


    def bad_password(self):
        user = utils.random_username()
        old_pass = 'old'
        new_pass = 'new'
        cryptohome.mount_vault(user, old_pass, create=True)
        cryptohome.unmount_vault(user)
        try:
            cryptohome.change_password(user, 'bad', new_pass)
        except:
            pass
        else:
            raise error.TestFail('Migrated with bad password.')
        cryptohome.remove_vault(user)


    def nonexistent_user(self):
        user = utils.random_username()
        old_pass = 'old'
        new_pass = 'new'
        try:
            cryptohome.change_password(user, old_pass, new_pass)
        except:
            pass
        else:
            raise error.TestFail('Migrated a nonexistent user.')

    def run_once(self):
        self.good()
        self.bad_password()
        self.nonexistent_user()
