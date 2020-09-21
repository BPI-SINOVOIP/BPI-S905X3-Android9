# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import utils


class detachablebase_TriggerHammerd(test.test):
    """Hammerd smoke test.

    Hammerd upstart job should be invoked on boot. The job should exit normally.
    """
    version = 1

    # The upstart job's name.
    PROCESS_NAME = 'hammerd'

    # Path of the system log.
    MESSAGE_PATH = '/var/log/messages'

    # Message to find the start line of each boot. The boot timing is usually
    # [    0.000000], but in case the boot process has delay, we accept any
    # timing within 1 second.
    BOOT_START_LINE_MSG = 'kernel: \[    0\.[0-9]\{6\}\] Linux version'

    # Message that is printed when hammerd is triggered on boot.
    # It is defined at `src/platform2/hammerd/init/hammerd-at-boot.sh`.
    TRIGGER_ON_BOOT_MSG = 'Force trigger hammerd at boot.'

    # Message that is printed when the hammerd job failed to terminated
    # normally.
    PROCESS_FAILED_MSG = '%s main process ([0-9]\+) terminated' % PROCESS_NAME

    def run_once(self):
        # Get the start line of message belonging to this current boot.
        start_line = utils.run(
                'grep -ni "%s" "%s" | tail -n 1 | cut -d ":" -f 1' %
                (self.BOOT_START_LINE_MSG, self.MESSAGE_PATH),
                ignore_status=True).stdout.strip()
        logging.info('Start line: %s', start_line)
        if not start_line:
            raise error.TestError('Start line of boot is not found.')

        def _grep_messages(pattern):
            return utils.run('tail -n +%s %s | grep "%s"' %
                             (start_line, self.MESSAGE_PATH, pattern),
                             ignore_status=True).stdout

        # Check hammerd is triggered on boot.
        if not _grep_messages(self.TRIGGER_ON_BOOT_MSG):
            raise error.TestFail('hammerd is not triggered on boot.')
        # Check hammerd is terminated normally.
        if _grep_messages(self.PROCESS_FAILED_MSG):
            hammerd_log = _grep_messages(self.PROCESS_NAME)
            logging.error('Hammerd log: %s', hammerd_log)
            raise error.TestFail('hammerd terminated with non-zero value')
