# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import contextlib
import os

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import cryptohome

@contextlib.contextmanager
def scoped_dir(path):
    os.mkdir(path)
    yield
    os.rmdir(path)


class platform_CryptohomeBadPerms(test.test):
    """Tests Cryptohome's ability to detect directories with bad permissions or
       ownership in the mount path of a home directory.
    """
    version = 1

    def require_mount_fail(self, user):
        """
        Raise an error if the mount succeeded.
        @param user: A random user created in run_once.
        """
        try:
            cryptohome.mount_vault(user, 'test', create=True)
        except:
            pass
        else:
            raise error.TestFail('Mount unexpectedly succeeded for %s' % user)

    def run_once(self):
        # Leaf element of user path not owned by user.
        user = utils.random_username()
        path = cryptohome.user_path(user)
        with scoped_dir(path):
            os.chown(path, 0, 0)
            self.require_mount_fail(user)

        # Leaf element of system path not owned by root.
        user = utils.random_username()
        path = cryptohome.system_path(user)
        with scoped_dir(path):
            os.chown(path, 1, 1)
            self.require_mount_fail(user)

        # Leaf element of path too permissive.
        user = utils.random_username()
        path = cryptohome.user_path(user)
        with scoped_dir(path):
            os.chmod(path, 0777)
            self.require_mount_fail(user)

        # Non-leaf element of path not owned by root.
        user = utils.random_username()
        path = cryptohome.user_path(user)
        parent_path = os.path.dirname(path)
        os.chown(parent_path, 1, 1)
        try:
            self.require_mount_fail(user)
        finally:
            os.chown(parent_path, 0, 0)

        # Non-leaf element of path too permissive.
        user = utils.random_username()
        path = cryptohome.user_path(user)
        parent_path = os.path.dirname(path)
        old_perm = os.stat(parent_path).st_mode & 0777
        os.chmod(parent_path, 0777)
        try:
            self.require_mount_fail(user)
        finally:
            os.chmod(parent_path, old_perm)
            os.chown(parent_path, 0, 0)
