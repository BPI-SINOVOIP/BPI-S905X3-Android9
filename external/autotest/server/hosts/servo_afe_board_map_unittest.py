#!/usr/bin/python
#
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

import common
from autotest_lib.server.hosts import servo_host


class ServoAfeBoardMapTest(unittest.TestCase):
    """Tests for how we map AFE boards to servo boards."""

    def test_afe_board_mapping(self):
        """Tests mappings."""
        afe_map = servo_host._map_afe_board_to_servo_board
        self.assertEqual(afe_map('kip'), 'kip')
        self.assertEqual(afe_map('gizmo'), 'panther')
        self.assertEqual(afe_map('link_freon'), 'link')
        self.assertEqual(afe_map('nyan_blaze-freon'), 'nyan_blaze')
        self.assertEqual(afe_map('stumpy_moblab'), 'stumpy')
        self.assertEqual(afe_map('veyron_minnie-cheets'), 'veyron_minnie')


if __name__ == '__main__':
    unittest.main()
