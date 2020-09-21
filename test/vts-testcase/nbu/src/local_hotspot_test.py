"""Local hotspot tests."""

import time



from mobly import asserts
from mobly import test_runner
from utils import android_base_test

SSID = 'SSID'
# Number of seconds for the receiver to restore wifi state.
RESTORE_TIME = 20
# The time period that SSID needs to be found.
SCAN_TIMEOUT = 30


class LocalHotspotTest(android_base_test.AndroidBaseTest):
  """Local hotspot tests."""

  def setup_class(self):
    super(LocalHotspotTest, self).setup_class()
    self.station = self.dut_a
    self.ap = self.dut_b
    # The device used to discover Bluetooth devices and send messages.
    # Sets the tag that represents this device in logs.
    self.station.debug_tag = 'station'
    # The device that is expected to be discovered and receive messages.
    self.ap.debug_tag = 'ap'

  def setup_test(self):
    # Make sure wifi is on.
    self.station.android.wifiEnable()
    self.ap.android.wifiEnable()

  def test_local_hotspot_process(self):
    """Test for basic local hotspot process flow.

    Steps:
      1. Ap sets up a local hotspot and retrieves the credentials.
      2. Station connects to the hotspot.

    Verifies:
      Station can connect to the local hotspot created by ap.
    """
    wifi_on_before = self.ap.android.wifiIsEnabled()
    start_localhotspot_callback = self.ap.android.startLocalHotspot()
    start_result = start_localhotspot_callback.waitAndGet('onStarted', 30)
    local_hotspot_info = start_result.data
    self.ap.log.info('Local hotspot started')
    network_found = self.station.android.wifiScanAndFindSsid(
        local_hotspot_info[SSID], SCAN_TIMEOUT)
    asserts.assert_true(network_found, 'Network is not found within 30 seconds')
    self.station.android.wifiConnectByUpdate(local_hotspot_info[SSID],
                                             local_hotspot_info['Password'])
    self.station.log.info('Connected to the network %s.'
                          % local_hotspot_info[SSID])
    self.ap.android.stopLocalHotspot()
    time.sleep(RESTORE_TIME)
    wifi_on_after = self.ap.android.wifiIsEnabled()
    asserts.assert_equal(wifi_on_before, wifi_on_after)
    self.ap.log.info('Wifi state restored')

  def teardown_test(self):
    # Turn wifi off on both devices after test finishes.
    self.station.android.wifiDisable()
    self.ap.android.wifiDisable()


if __name__ == '__main__':
  test_runner.main()
