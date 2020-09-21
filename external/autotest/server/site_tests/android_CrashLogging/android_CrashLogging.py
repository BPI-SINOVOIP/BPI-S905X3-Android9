# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import time
import common
from autotest_lib.client.common_lib import error
from autotest_lib.server import test

# Process to kill for log-generation purposes.
TARGET_PROCESS = '/system/bin/mediaserver'
# Number of seconds to wait for host.run commands to timeout.
COMMAND_TIMEOUT_SECONDS = 10
# Number of times to try to kill the process.
KILL_RETRIES = 10
# Number of times to retry the command the find command to find logs.
LOG_FIND_RETRIES = 5

class android_CrashLogging(test.test):
    """Confirm that crash logs are generated for native crashes."""
    version = 1


    def run_once(self, host=None):
        """Confirm that crash logs are generated for crashes.

        @param host: host object representing the device under test.
        """
        if host is None:
            raise error.TestFail('android_Crashlogging test executed without '
                                 'a host')
        self.host = host

        # Remove any pre-existing tombstones.
        self.host.run('rm /data/tombstones/tombstone_*',
                      timeout=COMMAND_TIMEOUT_SECONDS, ignore_status=True)

        # Find and kill a process.
        result = self.host.run('pgrep %s' % TARGET_PROCESS,
                               timeout=COMMAND_TIMEOUT_SECONDS,
                               ignore_status=True)
        pid = result.stdout.strip()
        if result.exit_status != 0 or not len(pid):
            raise error.TestFail('No %s process found to kill' % TARGET_PROCESS)
        for _ in xrange(KILL_RETRIES):
            status = self.host.run('kill -SIGSEGV %s' % pid,
                                   timeout=COMMAND_TIMEOUT_SECONDS,
                                   ignore_status=True).exit_status
            if status != 0:
                break

        logs = None
        for _ in xrange(LOG_FIND_RETRIES):
            try:
                logs = self.host.run_output(
                        'find /data/tombstones -maxdepth 1 -type f',
                        timeout=COMMAND_TIMEOUT_SECONDS).split()
            except (error.GenericHostRunError, error.AutoservSSHTimeout,
                    error.CmdTimeoutError):
                raise error.TestFail('No crash logs were created because of a '
                                     'host error or because the directory '
                                     'where crash logs are written to does not '
                                     'exist on the DUT.')
            if logs:
                break
            time.sleep(1)

        if not logs:
            raise error.TestFail('No crash logs were created.')
