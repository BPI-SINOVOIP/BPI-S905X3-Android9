# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import common
from autotest_lib.client.common_lib import error
from autotest_lib.server import test


class brillo_Keymaster(test.test):
    """Test Keymaster HAL functionality."""
    version = 1

    TEST_EXECUTABLE = 'su shell keystore_cli_v2 brillo-platform-test'

    def run_once(self, host=None):
        """Runs the test.

        @param host: A host object representing the DUT.

        @raise TestFail: The test executable returned an error.
        """
        try:
            host.run(self.TEST_EXECUTABLE)
        except error.GenericHostRunError as run_error:
            raise error.TestFail(run_error)
