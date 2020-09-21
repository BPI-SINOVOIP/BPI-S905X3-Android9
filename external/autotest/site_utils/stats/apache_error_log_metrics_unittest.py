"""Tests for apache_error_log_metrics."""

import os
import re
import subprocess
import tempfile
import unittest

import common

import apache_error_log_metrics


SCRIPT_PATH = os.path.abspath(
    os.path.join(os.path.dirname(__file__),
                 'apache_error_log_metrics.py'))


class ApacheErrorTest(unittest.TestCase):
    """Unittest for the apache error log regexp."""

    def testNonMatchingLine(self):
        """Test for log lines which don't match the expected format.."""
        lines = [
          '[] [] [] blank components',
          '[] [:error] [] no "pid" section',
          '[] [:error] [pid 1234] no timestamp',
          '[hello world] [:] [pid 1234] no log level',
          '[hello] [:error] [pid 42]     too far indented.'
        ]
        for line in lines:
          self.assertEqual(
              None, apache_error_log_metrics.ERROR_LOG_MATCHER.match(line))

    def testMatchingLines(self):
        """Test for lines that are expected to match the format."""
        match = apache_error_log_metrics.ERROR_LOG_MATCHER.match(
            "[foo] [:bar] [pid 123] WARNING")
        self.assertEqual('bar', match.group('log_level'))
        self.assertEqual(None, match.group('mod_wsgi'))

        match = apache_error_log_metrics.ERROR_LOG_MATCHER.match(
            "[foo] [mpm_event:bar] [pid 123] WARNING")
        self.assertEqual('bar', match.group('log_level'))
        self.assertEqual(None, match.group('mod_wsgi'))

        match = apache_error_log_metrics.ERROR_LOG_MATCHER.match(
            "[foo] [:bar] [pid 123] mod_wsgi (pid=123)")
        self.assertEqual('bar', match.group('log_level'))
        self.assertEqual('od_wsgi', match.group('mod_wsgi'))

    def testExampleLog(self):
        """Try on some example lines from a real apache error log."""
        with open(os.path.join(os.path.dirname(__file__),
                               'apache_error_log_example.txt')) as fh:
          example_log = fh.readlines()
        matcher_output = [apache_error_log_metrics.ERROR_LOG_MATCHER.match(line)
                          for line in example_log]
        matched = filter(bool, matcher_output)
        self.assertEqual(5, len(matched))

        self.assertEqual('error', matched[0].group('log_level'))
        self.assertEqual(None, matched[0].group('mod_wsgi'))

        self.assertEqual('warn', matched[1].group('log_level'))
        self.assertEqual('od_wsgi', matched[1].group('mod_wsgi'))

        self.assertEqual('error', matched[2].group('log_level'))
        self.assertEqual(None, matched[2].group('mod_wsgi'))

        self.assertEqual('error', matched[3].group('log_level'))
        self.assertEqual(None, matched[3].group('mod_wsgi'))

        self.assertEqual('error', matched[4].group('log_level'))
        self.assertEqual(None, matched[4].group('mod_wsgi'))


    def _ShellOutToScript(self, lines):
        """Shells out to the script.

        @param lines: Lines to feed to stdin."""
        with tempfile.NamedTemporaryFile() as temp_file:
            p = subprocess.Popen([SCRIPT_PATH,
                                  '--debug-metrics-file', temp_file.name],
                                 stdin=subprocess.PIPE, stdout=subprocess.PIPE)
            p.communicate('\n'.join(lines))

            with open(temp_file.name) as fh:
                return fh.read()


    def testApacheErrorLogScriptWithNonMatchingLine(self):
        """Try shelling out the the script with --debug-file.

        Sending it a non-matching line should result in no output from
        ERROR_LOG_METRIC.
        """
        contents = self._ShellOutToScript(['an example log line'])

        # We have to use re.search here with a word border character ('\b')
        # because the ERROR_LOG_LINE_METRIC contains ERROR_LOG_METRIC as a
        # substring.
        self.assertTrue(re.search(
            apache_error_log_metrics.ERROR_LOG_LINE_METRIC[1:] + r'\b',
            contents))
        self.assertFalse(re.search(
            apache_error_log_metrics.ERROR_LOG_METRIC[1:] + r'\b',
            contents))

    def testApachErrorLogScriptWithMatchingLine(self):
        """Try shelling out the the script with a matching line.

        Sending it a line which matches the first-line regex should result in
        output from ERROR_LOG_METRIC.
        """
        contents = self._ShellOutToScript(['[foo] [:bar] [pid 123] WARNING'])

        self.assertTrue(re.search(
            apache_error_log_metrics.ERROR_LOG_LINE_METRIC[1:] + r'\b',
            contents))
        self.assertTrue(re.search(
            apache_error_log_metrics.ERROR_LOG_METRIC[1:] + r'\b',
            contents))

    def testApachErrorLogScriptWithSpecialLines(self):
        """Sending lines with specific messages."""
        contents = self._ShellOutToScript(['[foo] [:bar] [pid 123] WARNING Segmentation fault'])

        self.assertTrue(re.search(
            apache_error_log_metrics.SEGFAULT_METRIC[1:] + r'\b',
            contents))


if __name__ == '__main__':
    unittest.main()
