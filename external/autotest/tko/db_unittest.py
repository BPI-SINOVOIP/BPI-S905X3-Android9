#!/usr/bin/python

import sys
import unittest

from cStringIO import StringIO

import common
from autotest_lib.tko import db


class LogErrorTestCase(unittest.TestCase):
    """Tests for _log_error()."""

    def setUp(self):
        self._old_stderr = sys.stderr
        sys.stderr = self.stderr = StringIO()


    def tearDown(self):
        sys.stderr = self._old_stderr


    def test_log_error(self):
        """Test _log_error()."""
        db._log_error('error message')
        self.assertEqual(self.stderr.getvalue(), 'error message\n')


class FormatOperationalErrorTestCase(unittest.TestCase):
    """Tests for _format_operational_error()."""

    def test_format_operational_error(self):
        """Test _format_operational_error()."""
        got = db._format_operational_error(Exception())
        self.assertIn('An operational error occurred', got)


if __name__ == "__main__":
    unittest.main()
