#!/usr/bin/python2

# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for apache_access_log_metrics.py"""

from __future__ import print_function

import os
import mock
import re
import subprocess
import tempfile
import unittest

import apache_access_log_metrics


SCRIPT_PATH = os.path.abspath(
    os.path.join(os.path.dirname(__file__),
                 'apache_access_log_metrics.py'))


EXAMPLE_REQUEST_LINE = (
    r'chromeos-server2.mtv.corp.google.com:80 100.108.96.5 - - '
    r'[19/May/2017:11:47:03 -0700] '
    r'"POST /afe/server/noauth/rpc/?method=foo HTTP/1.1\"" '
    r'200 354 "-" "Python-urllib/2.7" 5'
)


class TestParsers(unittest.TestCase):
    """Tests the parsing functions in apache_access_log_metrics."""

    def testParseResponse(self):
        """Tests that the regex matches the example log line."""
        match = apache_access_log_metrics.ACCESS_MATCHER.match(
            EXAMPLE_REQUEST_LINE)
        self.assertTrue(match)

        self.assertEqual(match.group('bytes_sent'), '354')
        self.assertEqual(match.group('response_seconds'), '5')


class TestEmitters(unittest.TestCase):
    """Tests the emitter functions in apache_access_log_metrics."""

    def testEmitResponse(self):
        """Tests that the matcher function doesn't throw an Exception."""
        match = apache_access_log_metrics.ACCESS_MATCHER.match(
            EXAMPLE_REQUEST_LINE)
        # Calling the emitter should not raise any exceptions (for example, by
        # referencing regex match groups that don't exist.
        with mock.patch.object(apache_access_log_metrics, 'metrics'):
            apache_access_log_metrics.EmitRequestMetrics(match)


class TestScript(unittest.TestCase):
    """Tests the script end-to-end."""
    def testApachAccessLogScriptWithMatchingLine(self):
        """Try shelling out the the script with --debug-file.

        Sending it a line which matches the first-line regex should result in
        output from ACCESS_TIME_METRIC.
        """
        with tempfile.NamedTemporaryFile() as temp_file:
            p = subprocess.Popen([SCRIPT_PATH,
                                  '--debug-metrics-file', temp_file.name],
                                 stdin=subprocess.PIPE, stdout=subprocess.PIPE)
            p.communicate(EXAMPLE_REQUEST_LINE)

            with open(temp_file.name) as fh:
                contents = fh.read()

            self.assertTrue(re.search(
                apache_access_log_metrics.ACCESS_TIME_METRIC[1:] + r'\b',
                contents))
            self.assertTrue(re.search(
                apache_access_log_metrics.ACCESS_BYTES_METRIC[1:] + r'\b',

                contents))



if __name__ == '__main__':
  unittest.main()
