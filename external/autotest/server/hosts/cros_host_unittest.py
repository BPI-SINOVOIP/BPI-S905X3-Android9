#!/usr/bin/python
# pylint: disable=missing-docstring

import unittest

import common

from autotest_lib.server.hosts import cros_host
from autotest_lib.server.hosts import servo_host

CROSSYSTEM_RESULT = '''
fwb_tries              = 0                              # Fake comment
fw_vboot2              = 1                              # Fake comment
fwid                   = Google_Reef.9933.0.0           # Fake comment
fwupdate_tries         = 0                              #
fw_tried               = B                              #
fw_try_count           = 0                              #
'''

class MockCmd(object):
    """Simple mock command with base command and results"""

    def __init__(self, cmd, exit_status, stdout):
        self.cmd = cmd
        self.stdout = stdout
        self.exit_status = exit_status


class MockHost(cros_host.CrosHost):
    """Simple host for running mock'd host commands"""

    def __init__(self, *args):
        self._mock_cmds = {c.cmd: c for c in args}

    def run(self, command, **kwargs):
        """Finds the matching result by command value"""
        mock_cmd = self._mock_cmds[command]
        file_out = kwargs.get('stdout_tee', None)
        if file_out:
            file_out.write(mock_cmd.stdout)
        return mock_cmd


class GetPlatformModelTests(unittest.TestCase):
    """Unit tests for CrosHost.get_platform_model"""

    def test_mosys_succeeds(self):
        host = MockHost(MockCmd('mosys platform model', 0, 'coral\n'))
        self.assertEqual(host.get_platform(), 'coral')

    def test_mosys_fails(self):
        host = MockHost(
            MockCmd('mosys platform model', 1, ''),
            MockCmd('crossystem', 0, CROSSYSTEM_RESULT))
        self.assertEqual(host.get_platform(), 'reef')


class DictFilteringTestCase(unittest.TestCase):

    """Tests for dict filtering methods on CrosHost."""

    def test_get_chameleon_arguments(self):
        got = cros_host.CrosHost.get_chameleon_arguments({
            'chameleon_host': 'host',
            'spam': 'eggs',
        })
        self.assertEqual(got, {'chameleon_host': 'host'})

    def test_get_plankton_arguments(self):
        got = cros_host.CrosHost.get_plankton_arguments({
            'plankton_host': 'host',
            'spam': 'eggs',
        })
        self.assertEqual(got, {'plankton_host': 'host'})

    def test_get_servo_arguments(self):
        got = cros_host.CrosHost.get_servo_arguments({
            servo_host.SERVO_HOST_ATTR: 'host',
            'spam': 'eggs',
        })
        self.assertEqual(got, {servo_host.SERVO_HOST_ATTR: 'host'})


if __name__ == "__main__":
    unittest.main()
