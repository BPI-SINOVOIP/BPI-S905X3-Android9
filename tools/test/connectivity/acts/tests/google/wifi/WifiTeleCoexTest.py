#!/usr/bin/env python3.4

import queue
import time

import acts.base_test
import acts.test_utils.wifi.wifi_test_utils as wifi_utils
import acts.test_utils.tel.tel_test_utils as tele_utils
import acts.utils

from acts import asserts
from acts import signals
from acts.test_decorators import test_tracker_info
from acts.test_utils.tel.TelephonyBaseTest import TelephonyBaseTest
from acts.test_utils.tel.tel_voice_utils import phone_setup_voice_general
from acts.test_utils.tel.tel_voice_utils import two_phone_call_short_seq

WifiEnums = wifi_utils.WifiEnums

ATTENUATORS = "attenuators"
WIFI_SSID = "wifi_network_ssid"
WIFI_PWD = "wifi_network_pass"
STRESS_COUNT = "stress_iteration"

class WifiTeleCoexTest(TelephonyBaseTest):
    """Tests for WiFi, Celular Co-existance."""


    def __init__(self, controllers):
        TelephonyBaseTest.__init__(self, controllers)


    def setup_class(self):
        TelephonyBaseTest.setup_class(self)
        self.dut = self.android_devices[0]
        wifi_utils.wifi_test_device_init(self.dut)
        # Set attenuation to 0 on all channels.
        if getattr(self, ATTENUATORS, []):
            for a in self.attenuators:
                a.set_atten(0)
        self.ads = self.android_devices
        self.dut = self.android_devices[0]
        self.wifi_network_ssid = self.user_params.get(WIFI_SSID)
        self.wifi_network_pass = self.user_params.get(WIFI_PWD)
        self.network = { WifiEnums.SSID_KEY : self.wifi_network_ssid,
                         WifiEnums.PWD_KEY : self.wifi_network_pass
                       }
        self.stress_count = self.user_params.get(STRESS_COUNT)


    def setup_test(self):
        wifi_utils.wifi_toggle_state(self.dut, True)


    def teardown_test(self):
        wifi_utils.reset_wifi(self.dut)


    """Helper Functions"""


    def connect_to_wifi(self, ad, network):
        """Connection logic for open and psk wifi networks.

        Args:
            ad: Android device object.
            network: A JSON dict of the WiFi network configuration.

        """
        ad.ed.clear_all_events()
        wifi_utils.start_wifi_connection_scan(ad)
        scan_results = ad.droid.wifiGetScanResults()
        wifi_utils.assert_network_in_list({WifiEnums.SSID_KEY:
                self.wifi_network_ssid}, scan_results)
        wifi_utils.wifi_connect(ad, network)
        self.log.debug("Connected to %s network on %s device" % (
                network[WifiEnums.SSID_KEY], ad.serial))


    def stress_toggle_wifi(self, stress_count):
        """Toggle WiFi in a loop.

        Args:
            stress_count: Number of times to toggle WiFi OFF and ON.

        """
        for count in range(stress_count):
            self.log.debug("stress_toggle_wifi: Iteration %d" % count)
            wifi_utils.toggle_wifi_off_and_on(self.dut)

        if not self.dut.droid.wifiGetisWifiEnabled():
            raise signals.TestFailure("WiFi did not turn on after toggling it"
                                      " %d times" % self.stress_count)


    def stress_toggle_airplane(self, stress_count):
        """Toggle Airplane mode in a loop.

        Args:
            stress_count: Number of times to toggle Airplane mode OFF and ON.

        """
        for count in range(stress_count):
            self.log.debug("stress_toggle_airplane: Iteration %d" % count)
            wifi_utils.toggle_airplane_mode_on_and_off(self.dut)

        if not self.dut.droid.wifiGetisWifiEnabled():
            raise signals.TestFailure("WiFi did not turn on after toggling it"
                                      " %d times" % self.stress_count)


    def stress_toggle_airplane_and_wifi(self, stress_count):
        """Toggle Airplane and WiFi modes in a loop.

        Args:
            stress_count: Number of times to perform Airplane mode ON, WiFi ON,
                          Airplane mode OFF, in a sequence.

        """
        self.log.debug("Toggling Airplane mode ON")

        asserts.assert_true(
            acts.utils.force_airplane_mode(self.dut, True),
            "Can not turn on airplane mode on: %s" % self.dut.serial)
        # Sleep for atleast 500ms so that, call to enable wifi is not deferred.
        time.sleep(1)

        self.log.debug("Toggling wifi ON")
        wifi_utils.wifi_toggle_state(self.dut, True)
        # Sleep for 1s before getting new WiFi state.
        time.sleep(1)
        if not self.dut.droid.wifiGetisWifiEnabled():
            raise signals.TestFailure("WiFi did not turn on after turning ON"
                                      " Airplane mode")
        asserts.assert_true(
            acts.utils.force_airplane_mode(self.dut, False),
            "Can not turn on airplane mode on: %s" % self.dut.serial)

        if not self.dut.droid.wifiGetisWifiEnabled():
            raise signals.TestFailure("WiFi did not turn on after toggling it"
                                      " %d times" % self.stress_count)


    def setup_cellular_voice_calling(self):
        """Setup phone for voice general calling and make sure phone is
           attached to voice."""
        # Make sure Phone A and B are attached to voice network.
        for ad in self.ads:
            if not phone_setup_voice_general(self.log, ad):
                raise signals.TestFailure("Phone failed to setup for voice"
                                          " calling serial:%s" % ad.serial)
        self.log.debug("Finished setting up phones for voice calling")


    def validate_cellular_and_wifi(self):
        """Validate WiFi, make some cellular calls.

        Steps:
            1. Check if device is still connected to the WiFi network.
            2. If WiFi looks good, check if deivce is attached to voice.
            3. Make a short sequence voice call between Phone A and B.

        """
        # Sleep for 5s before getting new WiFi state.
        time.sleep(5)
        wifi_info = self.dut.droid.wifiGetConnectionInfo()
        if wifi_info[WifiEnums.SSID_KEY] != self.wifi_network_ssid:
            raise signals.TestFailure("Phone failed to connect to %s network on"
                                      " %s" % (self.wifi_network_ssid,
                                      self.dut.serial))

        # Make short call sequence between Phone A and Phone B.
        two_phone_call_short_seq(self.log, self.ads[0], None, None, self.ads[1],
                                 None, None)

    """Tests"""


    @test_tracker_info(uuid="8b9b6fb9-964b-43e7-b75f-675774ee346f")
    @TelephonyBaseTest.tel_test_wrap
    def test_toggle_wifi_call(self):
        """Test to toggle WiFi and then perform WiFi connection and
           cellular calls.

        Steps:
            1. Attach device to voice subscription network.
            2. Connect to a WiFi network.
            3. Toggle WiFi OFF and ON.
            4. Verify device auto-connects to the WiFi network.
            5. Verify device is attached to voice network.
            6. Make short sequence voice calls.

        """
        self.setup_cellular_voice_calling()
        self.connect_to_wifi(self.dut, self.network)
        wifi_utils.toggle_wifi_off_and_on(self.dut)
        self.validate_cellular_and_wifi()
        return True


    @test_tracker_info(uuid="caf22447-6354-4a2e-99e5-0ff235fc8f20")
    @TelephonyBaseTest.tel_test_wrap
    def test_toggle_airplane_call(self):
        """Test to toggle Airplane mode and perform WiFi connection and
           cellular calls.

        Steps:
            1. Attach device to voice subscription network.
            2. Connect to a WiFi network.
            3. Toggle Airplane mode OFF and ON.
            4. Verify device auto-connects to the WiFi network.
            5. Verify device is attached to voice network.
            6. Make short sequence voice calls.

        """
        self.setup_cellular_voice_calling()
        self.connect_to_wifi(self.dut, self.network)
        wifi_utils.toggle_airplane_mode_on_and_off(self.dut)
        self.validate_cellular_and_wifi()
        return True


    @test_tracker_info(uuid="dd888b35-f820-409a-89af-4b0f6551e4d6")
    @TelephonyBaseTest.tel_test_wrap
    def test_toggle_airplane_and_wifi_call(self):
        """Test to toggle WiFi in a loop and perform WiFi connection and
           cellular calls.

        Steps:
            1. Attach device to voice subscription network.
            2. Connect to a WiFi network.
            3. Toggle Airplane mode ON.
            4. Turn WiFi ON.
            5. Toggle Airplane mode OFF.
            3. Verify device auto-connects to the WiFi network.
            4. Verify device is attached to voice network.
            5. Make short sequence voice calls.

        """
        self.setup_cellular_voice_calling()
        self.connect_to_wifi(self.dut, self.network)
        self.stress_toggle_airplane_and_wifi(1)
        self.validate_cellular_and_wifi()
        return True


    @test_tracker_info(uuid="15db5b7e-827e-4bc8-8e77-7fcce343a323")
    @TelephonyBaseTest.tel_test_wrap
    def test_stress_toggle_wifi_call(self):
        """Stress test to toggle WiFi in a loop, then perform WiFi connection
           and cellular calls.

        Steps:
            1. Attach device to voice subscription network.
            2. Connect to a WiFi network.
            3. Toggle WiFi OFF and ON in a loop.
            4. Verify device auto-connects to the WiFi network.
            5. Verify device is attached to voice network.
            6. Make short sequence voice calls.

        """
        self.setup_cellular_voice_calling()
        self.connect_to_wifi(self.dut, self.network)
        self.stress_toggle_wifi(self.stress_count)
        self.validate_cellular_and_wifi()
        return True


    @test_tracker_info(uuid="80a2f1bf-5e41-453a-9b8e-be3b41d4d313")
    @TelephonyBaseTest.tel_test_wrap
    def test_stress_toggle_airplane_call(self):
        """Stress test to toggle Airplane mode in a loop, then perform WiFi and
           cellular calls.

        Steps:
            1. Attach device to voice subscription network.
            2. Connect to a WiFi network.
            3. Toggle Airplane mode OFF and ON in a loop.
            4. Verify device auto-connects to the WiFi network.
            5. Verify device is attached to voice network.
            6. Make short sequence voice calls.

        """
        self.setup_cellular_voice_calling()
        self.connect_to_wifi(self.dut, self.network)
        self.stress_toggle_airplane(self.stress_count)
        self.validate_cellular_and_wifi()
        return True


    @test_tracker_info(uuid="b88ad3e7-6462-4280-ad57-22d0ac91fdd8")
    @TelephonyBaseTest.tel_test_wrap
    def test_stress_toggle_airplane_and_wifi_call(self):
        """Stress test to toggle Airplane and WiFi mode in a loop, then perform
           WiFi connection and cellular calls.

        Steps:
            1. Attach device to voice subscription network.
            2. Connect to a WiFi network.
            3. Toggle Airplane mode ON.
            4. Turn WiFi ON.
            5. Toggle Airplane mode OFF.
            6. Repeat 3, 4 & 5, in a loop.
            7. Verify device auto-connects to the WiFi network.
            8. Verify device is attached to voice network.
            9. Make short sequence voice calls.

        """
        self.setup_cellular_voice_calling()
        self.connect_to_wifi(self.dut, self.network)
        self.stress_toggle_airplane_and_wifi(self.stress_count)
        self.validate_cellular_and_wifi()
        return True
