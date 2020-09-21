# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.server import test


class AtrusBaseTest(test.test):
    """
    Base class for Atrus enterprise tests.

    AtrusBaseTest provides common setup and cleanup methods.
    """

    def initialize(self, host):
        """
        Initializes common test properties.

        @param host: a host object representing the DUT.
        """
        super(AtrusBaseTest, self).initialize()
        self._host = host

