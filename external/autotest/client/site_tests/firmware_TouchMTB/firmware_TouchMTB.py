# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A semi-automated test suite for touch device firmware MTB events."""

from autotest_lib.client.bin import test
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error


class firmware_TouchMTB(test.test):
    """A dummpy class for installing the test suite."""
    version = 1

    def run_once(self):
        raise error.TestFail('Test is obsolete.')
