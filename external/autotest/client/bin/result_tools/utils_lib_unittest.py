#!/usr/bin/python
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""unittest for utils_lib.py
"""

import unittest

import common
from autotest_lib.client.bin.result_tools import utils_lib


class TestUtilsLib(unittest.TestCase):
    """Test class for utils_lib module."""

    def testGetSizeString(self):
        """Test method get_size_string."""
        compares = {1: '1.0 B',
                    1999: '2.0 KB',
                    1100: '1.1 KB',
                    10 * 1024 * 1024: '10 MB',
                    10 * 1024 * 1024 * 1024: '10 GB'}
        for size, string in compares.items():
            self.assertEqual(utils_lib.get_size_string(size), string)


# this is so the test can be run in standalone mode
if __name__ == '__main__':
    """Main"""
    unittest.main()
