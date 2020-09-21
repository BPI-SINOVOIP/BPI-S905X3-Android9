#!/usr/bin/python2.7
#
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

import common
from autotest_lib.utils import gslib


class EscapeTestCase(unittest.TestCase):
    """Tests for basic KeyvalLabel functions."""

    def test_escape_printable(self):
        """Test escaping printable characters."""
        got = gslib.escape('foo[]*?#')
        self.assertEqual(got, 'foo%5b%5d%2a%3f%23')

    def test_escape_control(self):
        """Test escaping control characters by hex."""
        got = gslib.escape('foo\x88')
        self.assertEqual(got, 'foo%88')


if __name__ == '__main__':
    unittest.main()
