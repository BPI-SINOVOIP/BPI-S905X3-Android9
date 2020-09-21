"""Tests for rf_switch_client_box."""

import common
import mock
import unittest

from autotest_lib.server.cros.network import rf_switch_client_box


class RfSwitchClientBoxTest(unittest.TestCase):
    """Tests for the RfSwitchClientBox."""


    def setUp(self):
        """Initial set up for the tests."""
        rf_switch_client_box.frontend = mock.MagicMock()
        self.client_box_host = rf_switch_client_box.frontend.Host('', '')
        self.client_box_host.hostname = 'chromeos9-clientbox1'
        self.client_box_host.labels = [
                'rf_switch_1', 'client_box_1', 'rf_switch_client']


    def testGetDevices(self):
        """Test to get all devices from a Client Box."""
        rf_switch_client_box.frontend = mock.MagicMock()
        dut_host = rf_switch_client_box.frontend.Host('', '')
        dut_host.hostname = 'chromeos9-device1'
        dut_host.labels = ['rf_switch_1', 'client_box_1', 'rf_switch_dut']
        # Add a device to the Client Box and verify.
        afe_instance = rf_switch_client_box.frontend.AFE()
        afe_instance.get_hosts.return_value = [self.client_box_host, dut_host]
        client_box = rf_switch_client_box.ClientBox(self.client_box_host)
        devices = client_box.get_devices()
        self.assertEqual(len(devices), 1)
        device = devices[0]
        self.assertEqual(device, 'chromeos9-device1')


    def testNoDevicesInClientbox(self):
        """Test for no devices in the Client Box."""
        rf_switch_client_box.frontend = mock.MagicMock()
        afe_instance =rf_switch_client_box.frontend.AFE()
        afe_instance.get_hosts.return_value = [self.client_box_host]
        client_box = rf_switch_client_box.ClientBox(self.client_box_host)
        devices = client_box.get_devices()
        self.assertEqual(len(devices), 0)


    def testGetOtherDevices(self):
        """Test to get stumpy from ClientBox if installed."""
        rf_switch_client_box.frontend = mock.MagicMock()
        stumpy_host = rf_switch_client_box.frontend.Host('', '')
        stumpy_host.hostname = 'chromeos9-stumpy1'
        stumpy_host.labels = ['rf_switch_1', 'client_box_1', 'stumpy']
        afe_instance = rf_switch_client_box.frontend.AFE()
        afe_instance.get_hosts.side_effect = [[stumpy_host]]
        client_box = rf_switch_client_box.ClientBox(self.client_box_host)
        self.assertEqual(
                client_box.get_devices_using_labels(stumpy_host.labels),
                ['chromeos9-stumpy1'])


    def testGetOtherDeviceWithWrongLabels(self):
        """Test to get Devices when using wrong list of labels."""
        rf_switch_client_box.frontend = mock.MagicMock()
        stumpy_host = rf_switch_client_box.frontend.Host('', '')
        stumpy_host.hostname = 'chromeos9-stumpy1'
        stumpy_host.labels = ['rf_switch_1', 'client_box_1', 'stumpy']
        list_of_labels = ['rf_switch_1', 'client_box_1', 'packet_capture']
        afe_instance = rf_switch_client_box.frontend.AFE()
        afe_instance.get_hosts.side_effect = [[self.client_box_host],
                                              [stumpy_host]]
        client_box = rf_switch_client_box.ClientBox(self.client_box_host)
        self.assertEqual(
                len(client_box.get_devices_using_labels(list_of_labels)), 0)


if __name__ == '__main__':
    unittest.main()

