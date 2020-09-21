"""Tests for base_sysinfo."""

import mock
import unittest

import common
from autotest_lib.client.common_lib import autotemp
from autotest_lib.client.bin import base_sysinfo


class LoggableTestException(Exception):
    """An exception thrown by the loggable used for testing."""


class BaseSysinfoTestCase(unittest.TestCase):
    """TestCase for free functions in the base_sysinfo module."""

    def setUp(self):
        self._output_dir = autotemp.tempdir()
        self.addCleanup(self._output_dir.clean)

    def test_run_loggables_with_no_exception(self):
        """Tests _run_loggables_ignoring_errors when no loggable throws"""
        loggables = {
                mock.create_autospec(base_sysinfo.loggable),
                mock.create_autospec(base_sysinfo.loggable),
        }
        base_sysinfo._run_loggables_ignoring_errors(loggables, self._output_dir)
        for log in loggables:
            log.run.assert_called_once_with(self._output_dir)

    def test_run_loggables_with_exception(self):
        """Tests _run_loggables_ignoring_errors when one loggable throws"""
        failing_loggable = mock.create_autospec(base_sysinfo.loggable)
        failing_loggable.run.side_effect = LoggableTestException
        loggables = {
                mock.create_autospec(base_sysinfo.loggable),
                failing_loggable,
                mock.create_autospec(base_sysinfo.loggable),
        }
        base_sysinfo._run_loggables_ignoring_errors(loggables, self._output_dir)
        for log in loggables:
            log.run.assert_called_once_with(self._output_dir)


if __name__ == '__main__':
    unittest.main()
