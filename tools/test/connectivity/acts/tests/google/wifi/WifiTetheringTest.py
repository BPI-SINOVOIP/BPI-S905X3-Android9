#
#   Copyright 2017 - The Android Open Source Project
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

import logging
import random
import socket
import time

from acts import asserts
from acts import base_test
from acts import test_runner
from acts.controllers import adb
from acts.test_decorators import test_tracker_info
from acts.test_utils.tel import tel_defines
from acts.test_utils.tel.tel_data_utils import wait_for_cell_data_connection
from acts.test_utils.tel.tel_test_utils import get_operator_name
from acts.test_utils.tel.tel_test_utils import verify_http_connection
from acts.test_utils.tel.tel_test_utils import WIFI_CONFIG_APBAND_2G
from acts.test_utils.tel.tel_test_utils import WIFI_CONFIG_APBAND_5G
from acts.test_utils.net import socket_test_utils as sutils
from acts.test_utils.net import arduino_test_utils as dutils
from acts.test_utils.wifi import wifi_test_utils as wutils

WAIT_TIME = 2

class WifiTetheringTest(base_test.BaseTestClass):
    """ Tests for Wifi Tethering """

    def setup_class(self):
        """ Setup devices for tethering and unpack params """

        self.hotspot_device = self.android_devices[0]
        self.tethered_devices = self.android_devices[1:]
        req_params = ("network", "url", "open_network")
        self.unpack_userparams(req_params)
        self.new_ssid = "wifi_tethering_test2"

        wutils.wifi_toggle_state(self.hotspot_device, False)
        self.hotspot_device.droid.telephonyToggleDataConnection(True)
        wait_for_cell_data_connection(self.log, self.hotspot_device, True)
        asserts.assert_true(
            verify_http_connection(self.log, self.hotspot_device),
            "HTTP verification failed on cell data connection")
        asserts.assert_true(
            self.hotspot_device.droid.connectivityIsTetheringSupported(),
            "Tethering is not supported for the provider")
        for ad in self.tethered_devices:
            ad.droid.telephonyToggleDataConnection(False)
            wutils.wifi_test_device_init(ad)

    def teardown_class(self):
        """ Reset devices """
        wutils.wifi_toggle_state(self.hotspot_device, True)
        for ad in self.tethered_devices:
            ad.droid.telephonyToggleDataConnection(True)

    def on_fail(self, test_name, begin_time):
        """ Collect bug report on failure """
        self.hotspot_device.take_bug_report(test_name, begin_time)
        for ad in self.tethered_devices:
            ad.take_bug_report(test_name, begin_time)

    """ Helper functions """

    def _is_ipaddress_ipv6(self, ip_address):
        """ Verify if the given string is a valid IPv6 address

        Args:
            1. string which contains the IP address

        Returns:
            True: if valid ipv6 address
            False: if not
        """
        try:
            socket.inet_pton(socket.AF_INET6, ip_address)
            return True
        except socket.error:
            return False

    def _supports_ipv6_tethering(self, dut):
        """ Check if provider supports IPv6 tethering.
            Currently, only Verizon supports IPv6 tethering

        Returns:
            True: if provider supports IPv6 tethering
            False: if not
        """
        # Currently only Verizon support IPv6 tethering
        carrier_supports_tethering = ["vzw"]
        operator = get_operator_name(self.log, dut)
        return operator in carrier_supports_tethering

    def _carrier_supports_ipv6(self,dut):
        """ Verify if carrier supports ipv6
            Currently, only verizon and t-mobile supports IPv6

        Returns:
            True: if carrier supports ipv6
            False: if not
        """
        carrier_supports_ipv6 = ["vzw", "tmo"]
        operator = get_operator_name(self.log, dut)
        self.log.info("Carrier is %s" % operator)
        return operator in carrier_supports_ipv6

    def _verify_ipv6_tethering(self, dut):
        """ Verify IPv6 tethering """
        http_response = dut.droid.httpRequestString(self.url)
        self.log.info("IP address %s " % http_response)
        active_link_addrs = dut.droid.connectivityGetAllAddressesOfActiveLink()
        if dut==self.hotspot_device and self._carrier_supports_ipv6(dut)\
            or self._supports_ipv6_tethering(self.hotspot_device):
            asserts.assert_true(self._is_ipaddress_ipv6(http_response),
                                "The http response did not return IPv6 address")
            asserts.assert_true(
                active_link_addrs and http_response in str(active_link_addrs),
                "Could not find IPv6 address in link properties")
            asserts.assert_true(
                dut.droid.connectivityHasIPv6DefaultRoute(),
                "Could not find IPv6 default route in link properties")
        else:
            asserts.assert_true(
                not dut.droid.connectivityHasIPv6DefaultRoute(),
                "Found IPv6 default route in link properties")

    def _start_wifi_tethering(self, wifi_band=WIFI_CONFIG_APBAND_2G):
        """ Start wifi tethering on hotspot device

        Args:
            1. wifi_band: specifies the wifi band to start the hotspot
               on. The current options are 2G and 5G
        """
        wutils.start_wifi_tethering(self.hotspot_device,
                                    self.network[wutils.WifiEnums.SSID_KEY],
                                    self.network[wutils.WifiEnums.PWD_KEY],
                                    wifi_band)

    def _connect_disconnect_devices(self):
        """ Randomly connect and disconnect devices from the
            self.tethered_devices list to hotspot device
        """
        device_connected = [ False ] * len(self.tethered_devices)
        for _ in range(50):
            dut_id = random.randint(0, len(self.tethered_devices)-1)
            dut = self.tethered_devices[dut_id]
            # wait for 1 sec between connect & disconnect stress test
            time.sleep(1)
            if device_connected[dut_id]:
                wutils.wifi_forget_network(dut, self.network["SSID"])
            else:
                wutils.wifi_connect(dut, self.network)
            device_connected[dut_id] = not device_connected[dut_id]

    def _connect_disconnect_android_device(self, dut_id, wifi_state):
        """ Connect or disconnect wifi on android device depending on the
            current wifi state

        Args:
            1. dut_id: tethered device to change the wifi state
            2. wifi_state: current wifi state
        """
        ad = self.tethered_devices[dut_id]
        if wifi_state:
            self.log.info("Disconnecting wifi on android device")
            wutils.wifi_forget_network(ad, self.network["SSID"])
        else:
            self.log.info("Connecting to wifi on android device")
            wutils.wifi_connect(ad, self.network)

    def _connect_disconnect_wifi_dongle(self, dut_id, wifi_state):
        """ Connect or disconnect wifi on wifi dongle depending on the
            current wifi state

        Args:
            1. dut_id: wifi dongle to change the wifi state
            2. wifi_state: current wifi state
        """
        wd = self.arduino_wifi_dongles[dut_id]
        if wifi_state:
            self.log.info("Disconnecting wifi on dongle")
            dutils.disconnect_wifi(wd)
        else:
            self.log.info("Connecting to wifi on dongle")
            dutils.connect_wifi(wd, self.network)

    def _connect_disconnect_tethered_devices(self):
        """ Connect disconnect tethered devices to wifi hotspot """
        num_android_devices = len(self.tethered_devices)
        num_wifi_dongles = 0
        if self.arduino_wifi_dongles:
            num_wifi_dongles = len(self.arduino_wifi_dongles)
        total_devices = num_android_devices + num_wifi_dongles
        device_connected = [False] * total_devices
        for _ in range(50):
            dut_id = random.randint(0, total_devices-1)
            wifi_state = device_connected[dut_id]
            if dut_id < num_android_devices:
              self._connect_disconnect_android_device(dut_id, wifi_state)
            else:
              self._connect_disconnect_wifi_dongle(dut_id-num_android_devices,
                                                   wifi_state)
            device_connected[dut_id] = not device_connected[dut_id]

    def _verify_ping(self, dut, ip, isIPv6=False):
        """ Verify ping works from the dut to IP/hostname

        Args:
            1. dut - ad object to check ping from
            2. ip - ip/hostname to ping (IPv4 and IPv6)

        Returns:
            True - if ping is successful
            False - if not
        """
        self.log.info("Pinging %s from dut %s" % (ip, dut.serial))
        if isIPv6 or self._is_ipaddress_ipv6(ip):
            return dut.droid.pingHost(ip, 5, "ping6")
        return dut.droid.pingHost(ip)

    def _return_ip_for_interface(self, dut, iface_name):
        """ Return list of IP addresses for an interface

        Args:
            1. dut - ad object
            2. iface_name - interface name

        Returns:
            List of IPv4 and IPv6 addresses
        """
        return dut.droid.connectivityGetIPv4Addresses(iface_name) + \
            dut.droid.connectivityGetIPv6Addresses(iface_name)

    def _test_traffic_between_two_tethered_devices(self, ad, wd):
        """ Verify pinging interfaces of one DUT from another

        Args:
            1. ad - android device
            2. wd - wifi dongle
        """
        wutils.wifi_connect(ad, self.network)
        dutils.connect_wifi(wd, self.network)
        local_ip = ad.droid.connectivityGetIPv4Addresses('wlan0')[0]
        remote_ip = wd.ip_address()
        port = 8888

        time.sleep(6) # wait until UDP packets method is invoked
        socket = sutils.open_datagram_socket(ad, local_ip, port)
        sutils.send_recv_data_datagram_sockets(
            ad, ad, socket, socket, remote_ip, port)
        sutils.close_datagram_socket(ad, socket)

    def _ping_hotspot_interfaces_from_tethered_device(self, dut):
        """ Ping hotspot interfaces from tethered device

        Args:
            1. dut - tethered device

        Returns:
            True - if all IP addresses are pingable
            False - if not
        """
        ifaces = self.hotspot_device.droid.connectivityGetNetworkInterfaces()
        return_result = True
        for interface in ifaces:
            iface_name = interface.split()[0].split(':')[1]
            if iface_name == "lo":
                continue
            ip_list = self._return_ip_for_interface(
                self.hotspot_device, iface_name)
            for ip in ip_list:
                ping_result = self._verify_ping(dut, ip)
                self.log.info("Ping result: %s %s %s" %
                              (iface_name, ip, ping_result))
                return_result = return_result and ping_result

        return return_result

    def _save_wifi_softap_configuration(self, ad, config):
        """ Save soft AP configuration

        Args:
            1. dut - device to save configuration on
            2. config - soft ap configuration
        """
        asserts.assert_true(ad.droid.wifiSetWifiApConfiguration(config),
                            "Failed to set WifiAp Configuration")
        wifi_ap = ad.droid.wifiGetApConfiguration()
        asserts.assert_true(wifi_ap[wutils.WifiEnums.SSID_KEY] == config[wutils.WifiEnums.SSID_KEY],
                            "Configured wifi hotspot SSID does not match with the expected SSID")

    def _turn_on_wifi_hotspot(self, ad):
        """ Turn on wifi hotspot with a config that is already saved

        Args:
            1. dut - device to turn wifi hotspot on
        """
        ad.droid.wifiStartTrackingTetherStateChange()
        ad.droid.connectivityStartTethering(tel_defines.TETHERING_WIFI, False)
        try:
            ad.ed.pop_event("ConnectivityManagerOnTetheringStarted")
            ad.ed.wait_for_event("TetherStateChanged",
                                  lambda x: x["data"]["ACTIVE_TETHER"], 30)
        except:
            asserts.fail("Didn't receive wifi tethering starting confirmation")
        ad.droid.wifiStopTrackingTetherStateChange()

    """ Test Cases """

    @test_tracker_info(uuid="36d03295-bea3-446e-8342-b9f8f1962a32")
    def test_ipv6_tethering(self):
        """ IPv6 tethering test

        Steps:
            1. Start wifi tethering on hotspot device
            2. Verify IPv6 address on hotspot device (VZW & TMO only)
            3. Connect tethered device to hotspot device
            4. Verify IPv6 address on the client's link properties (VZW only)
            5. Verify ping on client using ping6 which should pass (VZW only)
            6. Disable mobile data on provider and verify that link properties
               does not have IPv6 address and default route (VZW only)
        """
        # Start wifi tethering on the hotspot device
        wutils.toggle_wifi_off_and_on(self.hotspot_device)
        self._start_wifi_tethering()

        # Verify link properties on hotspot device
        self.log.info("Check IPv6 properties on the hotspot device. "
                      "Verizon & T-mobile should have IPv6 in link properties")
        self._verify_ipv6_tethering(self.hotspot_device)

        # Connect the client to the SSID
        wutils.wifi_connect(self.tethered_devices[0], self.network)

        # Need to wait atleast 2 seconds for IPv6 address to
        # show up in the link properties
        time.sleep(WAIT_TIME)

        # Verify link properties on tethered device
        self.log.info("Check IPv6 properties on the tethered device. "
                      "Device should have IPv6 if carrier is Verizon")
        self._verify_ipv6_tethering(self.tethered_devices[0])

        # Verify ping6 on tethered device
        ping_result = self._verify_ping(self.tethered_devices[0],
                                        wutils.DEFAULT_PING_ADDR, True)
        if self._supports_ipv6_tethering(self.hotspot_device):
            asserts.assert_true(ping_result, "Ping6 failed on the client")
        else:
            asserts.assert_true(not ping_result, "Ping6 failed as expected")

        # Disable mobile data on hotspot device
        # and verify the link properties on tethered device
        self.log.info("Disabling mobile data to verify ipv6 default route")
        self.hotspot_device.droid.telephonyToggleDataConnection(False)
        asserts.assert_equal(
            self.hotspot_device.droid.telephonyGetDataConnectionState(),
            tel_defines.DATA_STATE_CONNECTED,
            "Could not disable cell data")

        time.sleep(WAIT_TIME) # wait until the IPv6 is removed from link properties

        result = self.tethered_devices[0].droid.connectivityHasIPv6DefaultRoute()
        self.hotspot_device.droid.telephonyToggleDataConnection(True)
        if result:
            asserts.fail("Found IPv6 default route in link properties:Data off")
        self.log.info("Did not find IPv6 address in link properties")

        # Disable wifi tethering
        wutils.stop_wifi_tethering(self.hotspot_device)

    @test_tracker_info(uuid="110b61d1-8af2-4589-8413-11beac7a3025")
    def test_wifi_tethering_2ghz_traffic_between_2tethered_devices(self):
        """ Steps:

            1. Start wifi hotspot with 2G band
            2. Connect 2 tethered devices to the hotspot device
            3. Ping interfaces between the tethered devices
        """
        wutils.toggle_wifi_off_and_on(self.hotspot_device)
        self._start_wifi_tethering(WIFI_CONFIG_APBAND_2G)
        self._test_traffic_between_two_tethered_devices(self.tethered_devices[0],
                                                        self.arduino_wifi_dongles[0])
        wutils.stop_wifi_tethering(self.hotspot_device)

    @test_tracker_info(uuid="953f6e2e-27bd-4b73-85a6-d2eaa4e755d5")
    def wifi_tethering_5ghz_traffic_between_2tethered_devices(self):
        """ Steps:

            1. Start wifi hotspot with 5ghz band
            2. Connect 2 tethered devices to the hotspot device
            3. Send traffic between the tethered devices
        """
        wutils.toggle_wifi_off_and_on(self.hotspot_device)
        self._start_wifi_tethering(WIFI_CONFIG_APBAND_5G)
        self._test_traffic_between_two_tethered_devices(self.tethered_devices[0],
                                                        self.arduino_wifi_dongles[0])
        wutils.stop_wifi_tethering(self.hotspot_device)

    @test_tracker_info(uuid="d7d5aa51-682d-4882-a334-61966d93b68c")
    def test_wifi_tethering_2ghz_connect_disconnect_devices(self):
        """ Steps:

            1. Start wifi hotspot with 2ghz band
            2. Connect and disconnect multiple devices randomly
            3. Verify the correct functionality
        """
        wutils.toggle_wifi_off_and_on(self.hotspot_device)
        self._start_wifi_tethering(WIFI_CONFIG_APBAND_2G)
        self._connect_disconnect_tethered_devices()
        wutils.stop_wifi_tethering(self.hotspot_device)

    @test_tracker_info(uuid="34abd6c9-c7f1-4d89-aa2b-a66aeabed9aa")
    def test_wifi_tethering_5ghz_connect_disconnect_devices(self):
        """ Steps:

            1. Start wifi hotspot with 5ghz band
            2. Connect and disconnect multiple devices randomly
            3. Verify the correct functionality
        """
        wutils.toggle_wifi_off_and_on(self.hotspot_device)
        self._start_wifi_tethering(WIFI_CONFIG_APBAND_5G)
        self._connect_disconnect_devices()
        wutils.stop_wifi_tethering(self.hotspot_device)

    @test_tracker_info(uuid="7edfb220-37f8-42ea-8d7c-39712fbe9be5")
    def test_wifi_tethering_2ghz_ping_hotspot_interfaces(self):
        """ Steps:

            1. Start wifi hotspot with 2ghz band
            2. Connect tethered device to hotspot device
            3. Ping 'wlan0' and 'rmnet_data' interface's IPv4
               and IPv6 interfaces on hotspot device from tethered device
        """
        wutils.toggle_wifi_off_and_on(self.hotspot_device)
        self._start_wifi_tethering(WIFI_CONFIG_APBAND_2G)
        wutils.wifi_connect(self.tethered_devices[0], self.network)
        result = self._ping_hotspot_interfaces_from_tethered_device(
            self.tethered_devices[0])
        wutils.stop_wifi_tethering(self.hotspot_device)
        return result

    @test_tracker_info(uuid="17e450f4-795f-4e67-adab-984940dffedc")
    def test_wifi_tethering_5ghz_ping_hotspot_interfaces(self):
        """ Steps:

            1. Start wifi hotspot with 5ghz band
            2. Connect tethered device to hotspot device
            3. Ping 'wlan0' and 'rmnet_data' interface's IPv4
               and IPv6 interfaces on hotspot device from tethered device
        """
        wutils.toggle_wifi_off_and_on(self.hotspot_device)
        self._start_wifi_tethering(WIFI_CONFIG_APBAND_5G)
        wutils.wifi_connect(self.tethered_devices[0], self.network)
        result = self._ping_hotspot_interfaces_from_tethered_device(
            self.tethered_devices[0])
        wutils.stop_wifi_tethering(self.hotspot_device)
        return result

    @test_tracker_info(uuid="2bc344cb-0277-4f06-b6cc-65b3972086ed")
    def test_change_wifi_hotspot_ssid_when_hotspot_enabled(self):
        """ Steps:

            1. Start wifi tethering
            2. Verify wifi Ap configuration
            3. Change the SSID of the wifi hotspot while hotspot is on
            4. Verify the new SSID in wifi ap configuration
            5. Restart tethering and verify that the tethered device is able
               to connect to the new SSID
        """
        wutils.toggle_wifi_off_and_on(self.hotspot_device)
        dut = self.hotspot_device

        # start tethering and verify the wifi ap configuration settings
        self._start_wifi_tethering()
        wifi_ap = dut.droid.wifiGetApConfiguration()
        asserts.assert_true(
            wifi_ap[wutils.WifiEnums.SSID_KEY] == \
                self.network[wutils.WifiEnums.SSID_KEY],
            "Configured wifi hotspot SSID did not match with the expected SSID")
        wutils.wifi_connect(self.tethered_devices[0], self.network)

        # update the wifi ap configuration with new ssid
        config = {wutils.WifiEnums.SSID_KEY: self.new_ssid}
        config[wutils.WifiEnums.PWD_KEY] = self.network[wutils.WifiEnums.PWD_KEY]
        config[wutils.WifiEnums.APBAND_KEY] = WIFI_CONFIG_APBAND_2G
        self._save_wifi_softap_configuration(dut, config)

        # start wifi tethering with new wifi ap configuration
        wutils.stop_wifi_tethering(dut)
        self._turn_on_wifi_hotspot(dut)

        # verify dut can connect to new wifi ap configuration
        new_network = {wutils.WifiEnums.SSID_KEY: self.new_ssid,
                       wutils.WifiEnums.PWD_KEY: \
                       self.network[wutils.WifiEnums.PWD_KEY]}
        wutils.wifi_connect(self.tethered_devices[0], new_network)
        wutils.stop_wifi_tethering(self.hotspot_device)

    @test_tracker_info(uuid="4cf7ab26-ca2d-46f6-9d3f-a935c7e04c97")
    def test_wifi_tethering_open_network_2g(self):
        """ Steps:

            1. Start wifi tethering with open network 2G band
               (Not allowed manually. b/72412729)
            2. Connect tethered device to the SSID
            3. Verify internet connectivity
        """
        wutils.start_wifi_tethering(
            self.hotspot_device, self.open_network[wutils.WifiEnums.SSID_KEY],
            None, WIFI_CONFIG_APBAND_2G)
        wutils.wifi_connect(self.tethered_devices[0], self.open_network)
        wutils.stop_wifi_tethering(self.hotspot_device)

    @test_tracker_info(uuid="f407049b-1324-40ea-a8d1-f90587933310")
    def test_wifi_tethering_open_network_5g(self):
        """ Steps:

            1. Start wifi tethering with open network 5G band
               (Not allowed manually. b/72412729)
            2. Connect tethered device to the SSID
            3. Verify internet connectivity
        """
        wutils.start_wifi_tethering(
            self.hotspot_device, self.open_network[wutils.WifiEnums.SSID_KEY],
            None, WIFI_CONFIG_APBAND_5G)
        wutils.wifi_connect(self.tethered_devices[0], self.open_network)
        wutils.stop_wifi_tethering(self.hotspot_device)

    @test_tracker_info(uuid="d964f2e6-0acb-417c-ada9-eb9fc5a470e4")
    def test_wifi_tethering_open_network_2g_stress(self):
        """ Steps:

            1. Save wifi hotspot configuration with open network 2G band
               (Not allowed manually. b/72412729)
            2. Turn on wifi hotspot
            3. Connect tethered device and verify internet connectivity
            4. Turn off wifi hotspot
            5. Repeat steps 2 to 4
        """
        # save open network wifi ap configuration with 2G band
        config = {wutils.WifiEnums.SSID_KEY:
                  self.open_network[wutils.WifiEnums.SSID_KEY]}
        config[wutils.WifiEnums.APBAND_KEY] = WIFI_CONFIG_APBAND_2G
        self._save_wifi_softap_configuration(self.hotspot_device, config)

        # turn on/off wifi hotspot, connect device
        for _ in range(9):
            self._turn_on_wifi_hotspot(self.hotspot_device)
            wutils.wifi_connect(self.tethered_devices[0], self.open_network)
            wutils.stop_wifi_tethering(self.hotspot_device)
            time.sleep(1) # wait for some time before turning on hotspot

    @test_tracker_info(uuid="c7ef840c-4003-41fc-80e3-755f9057b542")
    def test_wifi_tethering_open_network_5g_stress(self):
        """ Steps:

            1. Save wifi hotspot configuration with open network 5G band
               (Not allowed manually. b/72412729)
            2. Turn on wifi hotspot
            3. Connect tethered device and verify internet connectivity
            4. Turn off wifi hotspot
            5. Repeat steps 2 to 4
        """
        # save open network wifi ap configuration with 5G band
        config = {wutils.WifiEnums.SSID_KEY:
                  self.open_network[wutils.WifiEnums.SSID_KEY]}
        config[wutils.WifiEnums.APBAND_KEY] = WIFI_CONFIG_APBAND_5G
        self._save_wifi_softap_configuration(self.hotspot_device, config)

        # turn on/off wifi hotspot, connect device
        for _ in range(9):
            self._turn_on_wifi_hotspot(self.hotspot_device)
            wutils.wifi_connect(self.tethered_devices[0], self.open_network)
            wutils.stop_wifi_tethering(self.hotspot_device)
            time.sleep(1) # wait for some time before turning on hotspot
