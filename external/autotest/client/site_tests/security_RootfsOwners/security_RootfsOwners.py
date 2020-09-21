# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error

class security_RootfsOwners(test.test):
    """Ensures there are no files owned by chronos/chronos-access on the rootfs.
    """
    version = 1

    def run_once(self):
        """
        Do a find on the system for rootfs files owned by chronos
        or chronos-access. Fail if there are any.
        """
        cmd = 'find / -xdev -user chronos -print -o -user chronos-access -print'
        cmd_output = utils.system_output(cmd, ignore_status=True)

        if (cmd_output != ''):
            logging.error('chronos-/chronos-access-owned files:')
            logging.error(cmd_output)
            raise error.TestFail(
                'Rootfs contains files owned by chronos or chronos-access, '
                'see error log')
