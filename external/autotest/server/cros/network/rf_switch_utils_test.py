"""Tests for rf_switch_utils."""

import common
import unittest

import mock

from autotest_lib.server.cros.network import rf_switch_utils


class RfSwitchUtilsTest(unittest.TestCase):
    """Tests for RF Switch Utils."""


    def setUp(self):
        """Initial set up for the tests."""
        self.patcher1 = mock.patch('autotest_lib.server.frontend.AFE')
        self.patcher2 = mock.patch('autotest_lib.server.frontend.Host')
        self.mock_afe = self.patcher1.start()
        self.mock_host = self.patcher2.start()


    def tearDown(self):
        """End mock patcher."""
        self.patcher1.stop()
        self.patcher2.stop()


    def testAllocateRfSwitch(self):
        """Test to allocate a RF Switch."""
        afe_instance = self.mock_afe()
        mock_host_instance = self.mock_host()
        mock_host_instance.hostname = 'chromeos9-rfswitch1'
        # AFE returns list of RF Switch Hosts.
        afe_instance.get_hosts.return_value = [mock_host_instance]
        # Locking the first available RF Switch Host.
        afe_instance.lock_host.return_value = True
        rf_switch = rf_switch_utils.allocate_rf_switch()
        self.assertEquals(rf_switch, mock_host_instance)
        self.assertEquals(rf_switch.hostname, 'chromeos9-rfswitch1')


    def testRfSwitchCannotBeLocked(self):
        """Test logs when RF Switch cannot be locked."""
        afe_instance = self.mock_afe()
        mock_host_instance = self.mock_host()
        afe_instance.get_hosts.return_value = [mock_host_instance]
        mock_host_instance.hostname = 'chromeos9-rfswitch1'
        afe_instance.lock_host.return_value = False
        with mock.patch('logging.Logger.error') as mock_logger:
            rf_switch_utils.allocate_rf_switch()
            mock_logger.assert_called_with(
                    'RF Switch chromeos9-rfswitch1 could not be locked')


    def testRfSwitchesNotAvailable(self):
        """Test logs when no RF Switches are available to lock."""
        afe_instance = self.mock_afe()
        afe_instance.get_hosts.return_value = []
        with mock.patch('logging.Logger.debug') as mock_logger:
            rf_switch_utils.allocate_rf_switch()
            mock_logger.assert_called_with(
                    'No RF Switches are available for tests.')


    def testDeallocateRfSwitch(self):
        """Test to deallocate RF Switch."""
        afe_instance = self.mock_afe()
        mock_host_instance = self.mock_host()
        mock_host_instance.locked = False
        afe_instance.get_hosts.return_value = [mock_host_instance]
        self.assertEquals(
                rf_switch_utils.deallocate_rf_switch(mock_host_instance), True)


    def testRfSwitchCannotBeDeallocated(self):
        """Test when RF Switch cannot be deallocated."""
        afe_instance = self.mock_afe()
        mock_host_instance = self.mock_host()
        mock_host_instance.locked = True
        afe_instance.get_hosts.return_value = [mock_host_instance]
        self.assertEquals(
                rf_switch_utils.deallocate_rf_switch(mock_host_instance), False)


if __name__ == '__main__':
  unittest.main()
