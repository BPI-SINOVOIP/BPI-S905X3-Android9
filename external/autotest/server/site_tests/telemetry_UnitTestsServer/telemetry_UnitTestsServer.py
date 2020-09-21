# Copyright (c) 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import os.path

from autotest_lib.server import autotest
from autotest_lib.server import test


class telemetry_UnitTestsServer(test.test):
    """Runs the telemetry_UnitTests client tests with some extra setup."""
    version = 1

    # The .boto file needs to be copied into the client (VM) so that the tests
    # can run. The host boto file
    LOCAL_BOTO_FILE = os.path.join(os.getenv('HOME'), '.boto')
    CLIENT_BOTO_FILE = '/home/chromeos-test/.boto'

    def initialize(self, host, copy_boto_file=False):
        if copy_boto_file:
            # Copy ~/.boto from the local file system to the client. This is
            # needed for the telemetry tests to run there.
            logging.info('Creating client directory %s',
                         os.path.dirname(self.CLIENT_BOTO_FILE))
            host.run('mkdir -p %s' % os.path.dirname(self.CLIENT_BOTO_FILE))

            logging.info('Copying local %s to client %s', self.LOCAL_BOTO_FILE,
                         self.CLIENT_BOTO_FILE)
            assert(os.path.exists(self.LOCAL_BOTO_FILE))
            host.send_file(self.LOCAL_BOTO_FILE, self.CLIENT_BOTO_FILE)

    def cleanup(self, host, copy_boto_file=False):
        if copy_boto_file:
            # Clear the copied .boto file from the client, since it should no
            # longer be useful.
            logging.info('Clearing client %s', self.CLIENT_BOTO_FILE)
            host.run('rm %s' % self.CLIENT_BOTO_FILE)

    def run_once(self, host, use_packaging, browser_type, unit_tests,
                 perf_tests):
        # Otherwise we do nothing but run the client side tests.
        client_at = autotest.Autotest(host)
        client_at.run_test(
            'telemetry_UnitTests', host=host, use_packaging=use_packaging,
            browser_type=browser_type, unit_tests=unit_tests,
            perf_tests=perf_tests)
