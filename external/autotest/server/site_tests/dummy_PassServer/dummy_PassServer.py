# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import utils
from autotest_lib.server import test

class dummy_PassServer(test.test):
    """Tests that server tests can pass."""
    version = 1

    def run_once(self, force_ssp=False):
        """There is no body for this test.

        @param force_ssp: True to force the test to run using server-side
                packaging. Test shall fail if not running inside a container.
                Default is set to False.
        """
        if force_ssp and not utils.is_in_container():
            raise error.TestFail('The test is not running inside container.')
        return
