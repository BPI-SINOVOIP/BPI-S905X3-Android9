# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time
import os

from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.crash.crash_test import CrashTest
from autotest_lib.server import test


class logging_GenerateCrashFiles(test.test):
    """Tests if crash files are generated when crash is invoked."""
    version = 1
    SHORT_WAIT = 10
    REBOOT_TIMEOUT = 60
    CRASH_DIR = CrashTest._SYSTEM_CRASH_DIR

    def check_missing_crash_files(self, expected_extensions, existing_files,
                                  prefix):
        """Find if the crash dumps with appropriate extensions are created.
        @param expected_extensions: matching crash file extensions.
        @param existing files: state of crash dir before induced crash.
        @param prefix: matching crash file prefix.
        @raises TestFail error if crash files are not generated.
        """
        crash_extensions = list()

        out = self.host.run('ls %s' % self.CRASH_DIR, ignore_status=True)
        current_files = out.stdout.strip().split('\n')

        file_diff = set(current_files) - set(existing_files)
        logging.info("Crash files diff: %s" % file_diff)

        # Check empty files, prefix, and extension of crash files.
        for crash_file in file_diff:
            if crash_file.split('.')[0] != prefix:
                continue
            file_path = self.CRASH_DIR + '/' + crash_file
            if '0' == self.host.run("du -h %s" % file_path).stdout[:1]:
                raise error.TestFail('Crash file is empty: %s' % crash_file)
            crash_extensions.append(crash_file.split('.')[-1])

        # Check for presence of all crash file extensions.
        extension_diff = set(expected_extensions) - set(crash_extensions)
        if len(extension_diff) > 0:
            raise error.TestFail('%s files not generated.' % extension_diff)

        # Remove existing file crash files, if any.
        for crash_file in file_diff:
            self.host.run('rm %s' % self.CRASH_DIR + '/' + crash_file)

    def run_once(self, host, crash_cmd, crash_files, prefix):
        self.host = host

        # Sync the file system.
        self.host.run('sync', ignore_status=True)
        time.sleep(self.SHORT_WAIT)
        file_list = self.host.run('ls %s' % self.CRASH_DIR, ignore_status=True)
        existing_files = file_list.stdout.strip().split('\n')

        # Execute crash command.
        self.host.run(crash_cmd, ignore_status=True,
                      timeout=30, ignore_timeout=True)
        logging.debug('Crash invoked!')

        # In case of kernel crash the reboot will take some time.
        host.ping_wait_up(self.REBOOT_TIMEOUT)

        # Sync the file system.
        self.host.run('sync', ignore_status=True)
        time.sleep(self.SHORT_WAIT)

        self.check_missing_crash_files(crash_files, existing_files, prefix)
