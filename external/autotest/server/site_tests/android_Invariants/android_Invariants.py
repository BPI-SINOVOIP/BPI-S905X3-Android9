# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import common
from autotest_lib.client.common_lib import error
from autotest_lib.server import test


class android_Invariants(test.test):
    """Verify basic characteristics common to all Android devices."""
    version = 1


    def assert_path_test(self, path, test, negative=False):
        """Performs a test against a path.

        See the man page for test(1) for valid tests (e.g. -e, -b, -d).

        @param path: the path to check.
        @param test: the test to perform, without leading dash.
        @param negative: if True, test for the negative.
        """
        self.host.run('test %s -%s %s' % ('!' if negative else '', test, path))


    def assert_selinux_context(self, path, ctx):
        """Checks the selinux context of a path.

        @param path: the path to check.
        @param ctx: the selinux context to check for.

        @raises error.TestFail
        """
        # Example output of 'ls -LZ /dev/block/by-name/misc' is:
        # u:object_r:misc_block_device:s0 /dev/block/by-name/misc
        tokens = self.host.run_output('ls -LZ %s' % path).split()
        path_ctx = tokens[0]
        if not ctx in path_ctx:
            raise error.TestFail('Context "%s" for path "%s" does not '
                                 'contain "%s"' % (path_ctx, path, ctx))


    def check_fstab_name(self):
        """Checks that the fstab file has the name /fstab.<ro.hardware>.
        """
        hardware = self.host.run_output('getprop ro.hardware')
        self.assert_path_test('/fstab.%s' % hardware, 'e')


    def run_once(self, host=None):
        """Verify basic characteristics common to all Android devices.

        @param host: host object representing the device under test.
        """
        self.host = host
        self.check_fstab_name()
