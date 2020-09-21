"""Tests for rf_switch_controller class."""

import unittest

import mock

from autotest_lib.server import frontend
from autotest_lib.server import site_utils
from autotest_lib.server.cros.network import rf_switch_controller


class RfSwitchControllerTest(unittest.TestCase):
    """Tests for RF Switch Controller class."""


    RF_SWITCH_CLIENT = 'rf_switch_client'
    RF_SWITCH_APS = 'rf_switch_aps'


    def setUp(self):
        """Initial setup required to test the methods.

        Create three hosts: RF Switch, AP Box and Client Box. Assign the
        hostnames and labels. Create an instance of RfSwitchController and
        add mock hosts.
        """
        self.rf_switch_host = frontend.Host('', '')
        self.ap_box_host = frontend.Host('', '')
        self.client_box_host = frontend.Host('', '')

        self.rf_switch_host.hostname = 'chromeos9-rfswitch'
        self.ap_box_host.hostname = 'chromeos9-apbox1'
        self.client_box_host.hostname = 'chromeos9-clientbox1'

        self.rf_switch_host.labels = ['rf_switch', 'rf_switch_1']
        self.ap_box_host.labels = ['rf_switch_1', 'ap_box_1', 'rf_switch_aps']
        self.client_box_host.labels = [
                'rf_switch_1', 'client_box_1', 'rf_switch_client']


    @mock.patch('autotest_lib.server.frontend.AFE')
    def testGetAPBoxes(self, mock_afe):
        """Test to get all AP Boxes connected to the RF Switch."""
        afe_instance = mock_afe()
        afe_instance.get_hosts.return_value = [
                self.ap_box_host, self.client_box_host]
        rf_switch_manager = rf_switch_controller.RFSwitchController(
                self.rf_switch_host)
        ap_boxes = rf_switch_manager.get_ap_boxes()
        self.assertEquals(len(ap_boxes), 1)
        ap_box = ap_boxes[0]
        self.assertEquals(ap_box.ap_box_host.hostname, 'chromeos9-apbox1')


    @mock.patch('autotest_lib.server.frontend.AFE')
    def testGetClientBoxes(self, mock_afe):
        """Test to get all Client Boxes connected to the RF Switch."""
        afe_instance = mock_afe()
        afe_instance.get_hosts.return_value = [
                self.ap_box_host, self.client_box_host]
        rf_switch_manager = rf_switch_controller.RFSwitchController(
                self.rf_switch_host)
        client_boxes = rf_switch_manager.get_client_boxes()
        self.assertEquals(len(client_boxes), 1)
        client_box = client_boxes[0]
        self.assertEquals(
                client_box.client_box_host.hostname, 'chromeos9-clientbox1')


    @mock.patch('autotest_lib.server.frontend.AFE')
    def testRfSwitchNotConnectedToAPBoxes(self, mock_afe):
        """Testing scenario when RF Switch is not connnected to AP Boxes."""
        afe_instance = mock_afe()
        afe_instance.get_hosts.return_value = [self.client_box_host]
        with mock.patch('logging.Logger.error') as mock_logger:
            rf_switch_manager = rf_switch_controller.RFSwitchController(
                    self.rf_switch_host)
            mock_logger.assert_called_with(
                    'No AP boxes available for the RF Switch.')
        ap_boxes = rf_switch_manager.get_ap_boxes()
        self.assertEquals(len(ap_boxes), 0)


    @mock.patch('autotest_lib.server.frontend.AFE')
    def testClientBoxWithInvalidLabels(self, mock_afe):
        """Test when RF Switch connected to Client Box with invalid labels."""
        afe_instance = mock_afe()
        self.client_box_host.labels = ['rf_switch_1', 'client_1', 'rf_client']
        afe_instance.get_hosts.return_value = [self.client_box_host]
        with mock.patch('logging.Logger.error') as mock_logger:
            rf_switch_manager = rf_switch_controller.RFSwitchController(
                    self.rf_switch_host)
            mock_logger.assert_called_with(
                'No Client boxes available for the RF Switch.')
        client_boxes = rf_switch_manager.get_client_boxes()
        self.assertEquals(len(client_boxes), 0)


if __name__ == '__main__':
  unittest.main()
