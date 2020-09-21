# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, os
from shutil import copyfile
from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error

UMOUNT_FAIL_BASENAME = 'shutdown_stateful_umount_failure'
SHUTDOWN_STATEFUL_UMOUNT_FAIL = ('/mnt/stateful_partition/' +
                                 UMOUNT_FAIL_BASENAME)

class platform_CleanShutdown(test.test):
    """Checks for the presence of an unclean shutdown file."""
    version = 1

    def run_once(self):
        if os.path.exists(SHUTDOWN_STATEFUL_UMOUNT_FAIL):
            with open(SHUTDOWN_STATEFUL_UMOUNT_FAIL) as f:
                logging.debug('Stateful unmount failure log:\n%s', f.read())

            copyfile(SHUTDOWN_STATEFUL_UMOUNT_FAIL,
                     os.path.join(self.resultsdir, UMOUNT_FAIL_BASENAME))

            # Delete the file between each test run to see if the last reboot
            # failed.
            os.remove(SHUTDOWN_STATEFUL_UMOUNT_FAIL)
            raise error.TestFail(
                '{} exists!'.format(SHUTDOWN_STATEFUL_UMOUNT_FAIL))
