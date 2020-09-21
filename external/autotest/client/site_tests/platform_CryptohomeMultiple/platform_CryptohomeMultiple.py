# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.bin import test, utils
from autotest_lib.client.cros import cryptohome

class platform_CryptohomeMultiple(test.test):
    version = 1


    def test_mount_single(self):
        """
        Tests mounting a single not-already-existing cryptohome. Ensures that
        the infrastructure for multiple mounts is present and active.
        """
        user = utils.random_username()
        cryptohome.mount_vault(user, 'test', create=True)
        cryptohome.unmount_vault(user)


    def run_once(self):
        self.test_mount_single()
