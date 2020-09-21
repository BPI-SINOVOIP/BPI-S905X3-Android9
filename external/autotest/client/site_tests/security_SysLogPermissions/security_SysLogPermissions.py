# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import errno
import grp
import logging
import os
import pwd
import stat

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error

class security_SysLogPermissions(test.test):
    version = 1

    def run_once(self, baseline='suid'):
        syslog_uid = pwd.getpwnam('syslog').pw_uid
        syslog_gid = grp.getgrnam('syslog').gr_gid
        st = os.stat('/var/log')
        if not (st.st_mode & stat.S_ISVTX):
            raise error.TestFail('/var/log is not sticky')
        if st.st_gid != syslog_gid:
            raise error.TestFail('/var/log is not group syslog')

        # The /var/log/messages file might be rotated while this test runs.
        # Be a bit forgiving when it comes to slightly-off settings.
        try:
            st = os.stat('/var/log/messages')
        except OSError as e:
            # Ignore missing (middle of rotation) files.
            if e.errno == errno.ENOENT:
                return
            raise
        if st.st_uid == 0 and st.st_size == 0:
            # Ignore freshly created files.
            pass
        elif st.st_uid != syslog_uid:
            raise error.TestFail('/var/log/messages is not user syslog')
