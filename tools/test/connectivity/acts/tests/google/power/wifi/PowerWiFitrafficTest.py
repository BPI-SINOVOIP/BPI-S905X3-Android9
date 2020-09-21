#!/usr/bin/env python3.4
#
#   Copyright 2018 - The Android Open Source Project
#
#   Licensed under the Apache License, Version 2.0 (the 'License');
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an 'AS IS' BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

import time
from acts.test_decorators import test_tracker_info
from acts.test_utils.power import PowerWiFiBaseTest as PWBT
from acts.test_utils.wifi import wifi_power_test_utils as wputils

TEMP_FILE = '/sdcard/Download/tmp.log'


class PowerWiFitrafficTest(PWBT.PowerWiFiBaseTest):
    def iperf_power_test_func(self):
        """Test function for iperf power measurement at different RSSI level.

        """
        # Decode the test configs from test name
        attrs = [
            'screen_status', 'band', 'traffic_direction', 'traffic_type',
            'signal_level', 'oper_mode', 'bandwidth'
        ]
        indices = [3, 4, 5, 6, 7, 9, 10]
        self.decode_test_configs(attrs, indices)

        # Set attenuator to desired level
        self.log.info('Set attenuation to desired RSSI level')
        atten_setting = self.test_configs.band + '_' + self.test_configs.signal_level
        self.set_attenuation(self.atten_level[atten_setting])

        # Setup AP and connect dut to the network
        self.setup_ap_connection(
            network=self.main_network[self.test_configs.band],
            bandwidth=int(self.test_configs.bandwidth))
        # Wait for DHCP on the ethernet port and get IP as Iperf server address
        # Time out in 60 seconds if not getting DHCP address
        self.iperf_server_address = wputils.wait_for_dhcp(
            self.pkt_sender.interface)

        time.sleep(5)
        if self.test_configs.screen_status == 'OFF':
            self.dut.droid.goToSleepNow()
        self.RSSI = wputils.get_wifi_rssi(self.dut)

        # Run IPERF
        self.setup_iperf()
        self.iperf_server.start()
        wputils.run_iperf_client_nonblocking(
            self.dut, self.iperf_server_address, self.iperf_args)

        self.measure_power_and_validate()

    def setup_iperf(self):
        """Setup iperf command and arguments.

        """
        # Construct the iperf command based on the test params
        iperf_args = '-i 1 -t {} -p {} -J'.format(self.iperf_duration,
                                                  self.iperf_server.port)
        if self.test_configs.traffic_type == "UDP":
            iperf_args = iperf_args + "-u -b 2g"
        if self.test_configs.traffic_direction == "DL":
            iperf_args = iperf_args + ' -R'
            self.use_client_output = True
        else:
            self.use_client_output = False
        # Parse the client side data to a file saved on the phone
        self.iperf_args = iperf_args + ' > %s' % TEMP_FILE

    # Screen off TCP test cases
    @test_tracker_info(uuid='93f79f74-88d9-4781-bff0-8899bed1c336')
    def test_traffic_screen_OFF_2g_DL_TCP_high_RSSI_HT_20(self):

        self.iperf_power_test_func()

    @test_tracker_info(uuid='147eff45-97d7-47c0-b306-f84d9adecd9b')
    def test_traffic_screen_OFF_2g_DL_TCP_medium_RSSI_HT_20(self):

        self.iperf_power_test_func()

    @test_tracker_info(uuid='5982268b-57e4-40bf-848e-fee80fabf9d7')
    def test_traffic_screen_OFF_2g_DL_TCP_low_RSSI_HT_20(self):

        self.iperf_power_test_func()

    @test_tracker_info(uuid='c71a8c77-d355-4a82-b9f1-7cc8b888abd8')
    def test_traffic_screen_OFF_5g_DL_TCP_high_RSSI_VHT_20(self):

        self.iperf_power_test_func()

    @test_tracker_info(uuid='307945a6-32b7-42d0-a26c-d439f1599963')
    def test_traffic_screen_OFF_5g_DL_TCP_medium_RSSI_VHT_20(self):

        self.iperf_power_test_func()

    @test_tracker_info(uuid='e9a900a1-e263-45ad-bdf3-9c463f761d3c')
    def test_traffic_screen_OFF_5g_DL_TCP_low_RSSI_VHT_20(self):

        self.iperf_power_test_func()

    @test_tracker_info(uuid='1d1d9a06-98e1-486e-a1db-2102708161ec')
    def test_traffic_screen_OFF_5g_DL_TCP_high_RSSI_VHT_40(self):

        self.iperf_power_test_func()

    @test_tracker_info(uuid='feeaad15-6893-4d49-aaf6-bf9802780f5d')
    def test_traffic_screen_OFF_5g_DL_TCP_medium_RSSI_VHT_40(self):

        self.iperf_power_test_func()

    @test_tracker_info(uuid='f378679a-1c20-43a1-bff6-a6a5482a8e3d')
    def test_traffic_screen_OFF_5g_DL_TCP_low_RSSI_VHT_40(self):

        self.iperf_power_test_func()

    @test_tracker_info(uuid='6a05f133-49e5-4436-ba84-0746f04021ef')
    def test_traffic_screen_OFF_5g_DL_TCP_high_RSSI_VHT_80(self):

        self.iperf_power_test_func()

    @test_tracker_info(uuid='750bf1c3-2099-4b89-97dd-18f8e72df462')
    def test_traffic_screen_OFF_5g_DL_TCP_medium_RSSI_VHT_80(self):

        self.iperf_power_test_func()

    @test_tracker_info(uuid='1ea458af-1ae0-40ee-853d-ac57b51d3eda')
    def test_traffic_screen_OFF_5g_DL_TCP_low_RSSI_VHT_80(self):

        self.iperf_power_test_func()

    @test_tracker_info(uuid='43d9b146-3547-4a27-9d79-c9341c32ccda')
    def test_traffic_screen_OFF_2g_UL_TCP_high_RSSI_HT_20(self):

        self.iperf_power_test_func()

    @test_tracker_info(uuid='f00a868b-c8b1-4b36-8136-b39b5c2396a7')
    def test_traffic_screen_OFF_2g_UL_TCP_medium_RSSI_HT_20(self):

        self.iperf_power_test_func()

    @test_tracker_info(uuid='cd0c37ac-23fe-4dd1-9130-ccb2dfa71020')
    def test_traffic_screen_OFF_2g_UL_TCP_low_RSSI_HT_20(self):

        self.iperf_power_test_func()

    @test_tracker_info(uuid='f9173d39-b46d-4d80-a5a5-7966f5eed9de')
    def test_traffic_screen_OFF_5g_UL_TCP_high_RSSI_VHT_20(self):

        self.iperf_power_test_func()

    @test_tracker_info(uuid='cf77e1dc-30bc-4df9-88be-408f1fddc24f')
    def test_traffic_screen_OFF_5g_UL_TCP_medium_RSSI_VHT_20(self):

        self.iperf_power_test_func()

    @test_tracker_info(uuid='48f91745-22dc-47c9-ace6-c2719df651d6')
    def test_traffic_screen_OFF_5g_UL_TCP_low_RSSI_VHT_20(self):

        self.iperf_power_test_func()

    @test_tracker_info(uuid='18456aa7-62f0-4560-a7dc-4d7e01f6aca5')
    def test_traffic_screen_OFF_5g_UL_TCP_high_RSSI_VHT_40(self):

        self.iperf_power_test_func()

    @test_tracker_info(uuid='8ad237d7-f5e1-45e1-a4a2-a010628a4db9')
    def test_traffic_screen_OFF_5g_UL_TCP_medium_RSSI_VHT_40(self):

        self.iperf_power_test_func()

    @test_tracker_info(uuid='3e29173f-b950-4a41-a7f6-6cc0731bf477')
    def test_traffic_screen_OFF_5g_UL_TCP_low_RSSI_VHT_40(self):

        self.iperf_power_test_func()

    @test_tracker_info(uuid='3d4cdb21-a1b0-4011-9956-ca0b7a9f3bec')
    def test_traffic_screen_OFF_5g_UL_TCP_high_RSSI_VHT_80(self):

        self.iperf_power_test_func()

    @test_tracker_info(uuid='8427d3f0-9418-4b5c-aea9-7509e5959ce6')
    def test_traffic_screen_OFF_5g_UL_TCP_medium_RSSI_VHT_80(self):

        self.iperf_power_test_func()

    @test_tracker_info(uuid='5ac91734-0323-464b-b04a-c7d3d7ff8cdf')
    def test_traffic_screen_OFF_5g_UL_TCP_low_RSSI_VHT_80(self):

        self.iperf_power_test_func()

    # Screen off UDP tests - only check 5g VHT 80
    @test_tracker_info(uuid='1ab4a4e2-bce2-4ff8-be9d-f8ed2bb617cd')
    def test_traffic_screen_OFF_5g_DL_UDP_high_RSSI_VHT_80(self):

        self.iperf_power_test_func()

    @test_tracker_info(uuid='a2c66d63-e93f-42aa-a021-0c6cdfdc87b8')
    def test_traffic_screen_OFF_5g_DL_UDP_medium_RSSI_VHT_80(self):

        self.iperf_power_test_func()

    @test_tracker_info(uuid='68e6f92a-ae15-4e76-81e7-a7b491e181fe')
    def test_traffic_screen_OFF_5g_DL_UDP_low_RSSI_VHT_80(self):

        self.iperf_power_test_func()

    @test_tracker_info(uuid='258500f4-f177-43df-82a7-a64d66e90720')
    def test_traffic_screen_OFF_5g_UL_UDP_high_RSSI_VHT_80(self):

        self.iperf_power_test_func()

    @test_tracker_info(uuid='3d2d3d45-575d-4080-86f9-b32a96963032')
    def test_traffic_screen_OFF_5g_UL_UDP_medium_RSSI_VHT_80(self):

        self.iperf_power_test_func()

    @test_tracker_info(uuid='a17c7d0b-58ca-47b5-9f32-0b7a3d7d3d9d')
    def test_traffic_screen_OFF_5g_UL_UDP_low_RSSI_VHT_80(self):

        self.iperf_power_test_func()

    # Screen on point check
    @test_tracker_info(uuid='c1c71639-4463-4999-8f5d-7d9153402c79')
    def test_traffic_screen_ON_2g_DL_TCP_high_RSSI_HT_20(self):

        self.iperf_power_test_func()

    @test_tracker_info(uuid='40daebc4-45a2-4299-b724-e8cb917b86e8')
    def test_traffic_screen_ON_5g_DL_TCP_high_RSSI_VHT_80(self):

        self.iperf_power_test_func()

    @test_tracker_info(uuid='2e286f36-1a47-4895-a0e8-a161d6a9fd9f')
    def test_traffic_screen_ON_2g_UL_TCP_high_RSSI_HT_20(self):

        self.iperf_power_test_func()

    @test_tracker_info(uuid='9f6b52cb-b48a-4382-8061-3d3a511a261a')
    def test_traffic_screen_ON_5g_UL_TCP_high_RSSI_VHT_80(self):

        self.iperf_power_test_func()

    @test_tracker_info(uuid='59d79274-15cf-446b-a567-655c07f8a778')
    def test_traffic_screen_ON_2g_DL_UDP_high_RSSI_HT_20(self):

        self.iperf_power_test_func()

    @test_tracker_info(uuid='02891671-48cc-4186-9a95-3e02671477d0')
    def test_traffic_screen_ON_5g_DL_UDP_high_RSSI_VHT_80(self):

        self.iperf_power_test_func()

    @test_tracker_info(uuid='02821540-7b08-4e4f-a1f1-b455fd4cec6e')
    def test_traffic_screen_ON_2g_UL_UDP_high_RSSI_HT_20(self):

        self.iperf_power_test_func()

    @test_tracker_info(uuid='59ea06ac-3ac8-4ecc-abb1-bcde34f47358')
    def test_traffic_screen_ON_2g_UL_UDP_medium_RSSI_HT_20(self):

        self.iperf_power_test_func()

    @test_tracker_info(uuid='0cbbd849-7b59-4143-95e7-92cf1fd955dc')
    def test_traffic_screen_ON_2g_UL_UDP_low_RSSI_HT_20(self):

        self.iperf_power_test_func()

    @test_tracker_info(uuid='d84f11d8-41a9-4ce8-a351-ebb0379d56c1')
    def test_traffic_screen_ON_5g_UL_UDP_high_RSSI_VHT_80(self):

        self.iperf_power_test_func()

    @test_tracker_info(uuid='01b6087c-b39a-441d-90e9-da659aa0db7f')
    def test_traffic_screen_ON_5g_UL_UDP_medium_RSSI_VHT_80(self):

        self.iperf_power_test_func()

    @test_tracker_info(uuid='7e16dcaa-128f-4874-ab52-2f43e25e6da8')
    def test_traffic_screen_ON_5g_UL_UDP_low_RSSI_VHT_80(self):

        self.iperf_power_test_func()
