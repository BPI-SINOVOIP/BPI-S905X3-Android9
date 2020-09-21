"""Wifi Direct tests."""


from mobly import asserts
from mobly import test_runner
from utils import android_base_test
from utils import assert_utils

SSID = 'SSID'
# The time period that SSID needs to be found.
SCAN_TIMEOUT = 30
# The listen channel and operation channel for wifi direct.
CHANNEL = 1
# The frequency of wifi direct to be expected.
FREQUENCY = 2412


class WifiDirectTest(android_base_test.AndroidBaseTest):
  """Wifi Direct tests."""

  def setup_class(self):
    super(WifiDirectTest, self).setup_class()
    self.station = self.dut_a
    self.group_owner = self.dut_b
    # Sets the tag that represents this device in logs.
    self.station.debug_tag = 'station'
    self.group_owner.debug_tag = 'group_owner'

  def setup_test(self):
    # Make sure wifi of group owner is off to disable wifi direct.
    self.group_owner.android.wifiDisable()
    # Make sure wifi is on.
    self.station.android.wifiEnable()
    self.group_owner.android.wifiEnable()

  def test_wifi_direct_legacy_connection(self):
    """Test for basic Wifi Direct process flow.

    Steps:
      1. Group owner sets up a wifi p2p group.
      2. Station connects to the group as a regular hotspot.

    Verifies:
      Station can connect to the wifi p2p group created by group owner.
    """
    callback = self.group_owner.android.wifiP2pSetChannel(CHANNEL, CHANNEL)
    assert_utils.AssertAsyncSuccess(callback)
    self.group_owner.log.info('Wifi direct channel set.')
    callback = self.group_owner.android.wifiP2pStartGroup()
    group_info = assert_utils.AssertAsyncSuccess(callback)
    self.group_owner.log.info('Wifi direct group started as a temporary group.')
    network_found = self.station.android.wifiScanAndFindSsid(
        group_info.data[SSID], SCAN_TIMEOUT)
    asserts.assert_true(network_found, 'Network is not found within 30 seconds')
    asserts.assert_equal(network_found['frequency'], FREQUENCY)
    self.station.log.info('Network is found, connecting...')
    connect_result = self.station.android.wifiConnectByUpdate(
        group_info.data[SSID], group_info.data['Password'])
    asserts.assert_true(connect_result, 'Failed to connect to the network')
    self.station.log.info('Connected to the network')
    callback = self.group_owner.android.wifiP2pRemoveGroup()
    assert_utils.AssertAsyncSuccess(callback)
    self.group_owner.log.info('Wifi direct group removed')

  def teardown_test(self):
    # Turn wifi off on both devices after test finishes.
    self.station.android.wifiDisable()
    self.group_owner.android.wifiDisable()


if __name__ == '__main__':
  test_runner.main()
