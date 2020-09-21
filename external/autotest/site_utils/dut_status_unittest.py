#!/usr/bin/env python
# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import mock
import time
import unittest

import common

from autotest_lib.client.common_lib import time_utils
from autotest_lib.site_utils import dut_status


class TimeOptionTests(unittest.TestCase):
    """Test the --since, --until, and --destination options.

    The options are allowed in these seven combinations:
      * No options - use the default end time and duration.
      * --since - use the given start time and the default end time.
      * --until - use the given end time and the default duration.
      * --duration - use the given duration and the default end time.
      * --since --until - use the given start and end times.
      * --since --duration - use the given start time and duration.
      * --until --duration - use the given end time and duration.

    It's an error to use all three options together.

    """

    def setUp(self):
        self.test_time = time.time()

    def _try_parse(self, options):
        with mock.patch('time.time', return_value=self.test_time):
            arguments = dut_status._parse_command(
                    ['mumble.py'] + options + ['hostname'])
            dut_status._validate_time_range(arguments)
        return arguments

    def _check_duration(self, arguments, duration):
        start_time = (arguments.until - duration * 3600)
        self.assertEqual(arguments.since, start_time)

    def test_default_time_bounds(self):
        """Test time bounds when no options are supplied."""
        end_time = int(self.test_time)
        arguments = self._try_parse([])
        self.assertEqual(arguments.until, end_time)
        self._check_duration(arguments, dut_status._DEFAULT_DURATION)

    def test_start_only(self):
        """Test time bounds with --since only.

        Also tests that --since and -s are equivalent.
        """
        end_time = int(self.test_time)
        start_time = end_time - 3600
        start_time_string = time_utils.epoch_time_to_date_string(start_time)
        for option in ['--since', '-s']:
            arguments = self._try_parse([option, start_time_string])
            self.assertEqual(arguments.until, end_time)
            self.assertEqual(arguments.since, start_time)

    def test_end_only(self):
        """Test time bounds with --until only.

        Also tests that --until and -u are equivalent.
        """
        end_time = int(self.test_time) - 3600
        end_time_string = time_utils.epoch_time_to_date_string(end_time)
        for option in ['--until', '-u']:
            arguments = self._try_parse([option, end_time_string])
            self.assertEqual(arguments.until, end_time)
            self._check_duration(arguments, dut_status._DEFAULT_DURATION)

    def test_duration_only(self):
        """Test time bounds with --duration only.

        Also tests that --duration and -d are equivalent.
        """
        for option in ['--duration', '-d']:
            duration = 4
            duration_string = '%d' % duration
            end_time = int(self.test_time)
            arguments = self._try_parse([option, duration_string])
            self.assertEqual(arguments.until, end_time)
            self._check_duration(arguments, duration)

    def test_start_and_end(self):
        """Test time bounds with --since and --until."""
        start_time = int(self.test_time) - 5 * 3600
        start_time_string = time_utils.epoch_time_to_date_string(start_time)
        end_time = start_time + 4 * 3600
        end_time_string = time_utils.epoch_time_to_date_string(end_time)
        arguments = self._try_parse(['-s', start_time_string,
                                     '-u', end_time_string])
        self.assertEqual(arguments.since, start_time)
        self.assertEqual(arguments.until, end_time)

    def test_start_and_duration(self):
        """Test time bounds with --since and --duration."""
        start_time = int(self.test_time) - 5 * 3600
        start_time_string = time_utils.epoch_time_to_date_string(start_time)
        duration = 4
        duration_string = '%d' % duration
        arguments = self._try_parse(['-s', start_time_string,
                                     '-d', duration_string])
        self.assertEqual(arguments.since, start_time)
        self._check_duration(arguments, duration)

    def test_end_and_duration(self):
        """Test time bounds with --until and --duration."""
        end_time = int(self.test_time) - 5 * 3600
        end_time_string = time_utils.epoch_time_to_date_string(end_time)
        duration = 4
        duration_string = '%d' % duration
        arguments = self._try_parse(['-u', end_time_string,
                                     '-d', duration_string])
        self.assertEqual(arguments.until, end_time)
        self._check_duration(arguments, duration)

    def test_all_options(self):
        """Test that all three options are a fatal error."""
        start_time = int(self.test_time) - 5 * 3600
        start_time_string = time_utils.epoch_time_to_date_string(start_time)
        duration = 4
        duration_string = '%d' % duration
        end_time = start_time + duration * 3600
        end_time_string = time_utils.epoch_time_to_date_string(end_time)
        with self.assertRaises(SystemExit):
            self._try_parse(['-s', start_time_string,
                             '-u', end_time_string,
                             '-d', duration_string])


if __name__ == '__main__':
    unittest.main()
