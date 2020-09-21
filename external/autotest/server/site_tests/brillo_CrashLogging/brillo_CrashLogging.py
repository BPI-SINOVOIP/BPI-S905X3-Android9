# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import time
import common
from autotest_lib.client.common_lib import error
from autotest_lib.server import test

# Process to kill for log-generation purposes.
TARGET_PROCESS = 'weaved'
# Number of seconds to wait for host.run commands to timeout.
COMMAND_TIMEOUT_SECONDS = 10
# Number of times to retry the command the ls command to find logs.
LOG_LIST_RETRIES = 5

class brillo_CrashLogging(test.test):
    """Confirm that crash logs are generated for native crashes."""
    version = 1


    def run_once(self, host=None):
        """Confirm that crash logs are generated for native crashes.

        @param host: host object representing the device under test.
        """
        if host is None:
            raise error.TestFail('brillo_Crashlogging test executed without '
                                 'a host')
        self.host = host

        # Remove any existing crash logs.
        self.host.run('rm /data/misc/crash_reporter/crash/*',
                      timeout=COMMAND_TIMEOUT_SECONDS, ignore_status=True)

        # Find and kill a process.
        result = self.host.run('pgrep %s' % TARGET_PROCESS,
                               timeout=COMMAND_TIMEOUT_SECONDS)
        pid = result.stdout.strip()
        if not len(pid):
            raise error.TestFail('No %s process found to kill' % TARGET_PROCESS)
        self.host.run('kill -SIGSEGV %s' % pid, timeout=COMMAND_TIMEOUT_SECONDS)
        # If the process links against bionic, then the first kill will be
        # caught by bionic, so issue the command twice.
        self.host.run('kill -SIGSEGV %s' % pid, timeout=COMMAND_TIMEOUT_SECONDS,
                      ignore_status=True)
        logs = None
        for _ in xrange(LOG_LIST_RETRIES):
            try:
                logs = self.host.run_output(
                        'ls /data/misc/crash_reporter/crash',
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
