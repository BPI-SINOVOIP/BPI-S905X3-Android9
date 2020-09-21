"""Tests for rf_switch_ap_box."""

import common
import tempfile
import unittest

from autotest_lib.server.cros.network import rf_switch_ap_box
import mock

AP_CONF = '\n'.join([
          '[1a:2b:3c:4d:5e:6f]',
          'brand = Google',
          'wan_hostname = chromeos9-ap1',
          'ssid = rf_switch_router',
          'frequency = 2432',
          'bss = 1a:2b:3c:4d:5e:6f',
          'wan mac = 1a:2b:3c:4d:5e:6f',
          'model = dummy',
          'security = wpa2',
          'psk = chromeos',
          'class_name = StaticAPConfigurator'])


class RfSwitchApBoxTest(unittest.TestCase):
    """Tests for RFSwitchAPBox."""


    def setUp(self):
        """Initial set up for the tests."""
        self.ap_config_file = tempfile.NamedTemporaryFile()
        self.patcher1 = mock.patch('autotest_lib.server.frontend.Host')
        self.patcher2 = mock.patch('os.path.join')
        self.mock_host = self.patcher1.start()
        self.mock_host.hostname = 'chromeos9-apbox1'
        self.mock_os_path_join = self.patcher2.start()
        self.mock_os_path_join.return_value = self.ap_config_file.name


    def tearDown(self):
        """End the patchers and Close the file."""
        self.patcher1.stop()
        self.patcher2.stop()
        self.ap_config_file.close()


    def testGetApsList(self):
        """Test to get all APs from an AP Box."""
        self.mock_host.labels = ['rf_switch_1', 'ap_box_1', 'rf_switch_aps']
        self.ap_config_file.write(AP_CONF)
        self.ap_config_file.seek(0)
        ap_box = rf_switch_ap_box.APBox(self.mock_host)
        self.assertEqual(ap_box.ap_box_label, 'ap_box_1')
        self.assertEqual(ap_box.rf_switch_label, 'rf_switch_1')
        aps = ap_box.get_ap_list()
        self.assertEqual(len(aps), 1)
        ap = aps[0]
        self.assertEqual(ap.get_wan_host(), 'chromeos9-ap1')
        self.assertEqual(ap.get_bss(), '1a:2b:3c:4d:5e:6f')


    def testMissingApboxLabel(self):
        """Test when ap_box_label is missing."""
        self.mock_host.labels = ['rf_switch_1', 'rf_switch_aps']
        with self.assertRaises(Exception) as context:
            rf_switch_ap_box.APBox(self.mock_host)
        self.assertTrue(
                'AP Box chromeos9-apbox1 does not have ap_box and/or '
                'rf_switch labels' in context.exception)


    def testMissingRfSwitchLabel(self):
        """Test when rf_switch_lable is missing."""
        self.mock_host.labels = ['ap_box_1', 'rf_switch_aps']
        with self.assertRaises(Exception) as context:
            rf_switch_ap_box.APBox(self.mock_host)
        self.assertTrue(
                'AP Box chromeos9-apbox1 does not have ap_box and/or '
                'rf_switch labels' in context.exception)


    def testForEmptyApbox(self):
        """Test when no APs are in the APBox."""
        self.mock_host.labels = ['rf_switch_1', 'ap_box_1', 'rf_switch_aps']
        self.ap_config_file.write('')
        self.ap_config_file.seek(0)
        ap_box = rf_switch_ap_box.APBox(self.mock_host)
        aps = ap_box.get_ap_list()
        self.assertEqual(len(aps), 0)


if __name__ == '__main__':
    unittest.main()
