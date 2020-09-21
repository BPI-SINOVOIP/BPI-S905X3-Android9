#/usr/bin/env python3.4
#
#   Copyright 2016 - The Android Open Source Project
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
"""
Power tests for cellular connectivity.
"""

import time
import json
import logging
import os
import scapy.all as scapy

import acts.controllers.iperf_server as ipf

from acts import base_test
from acts.test_decorators import test_tracker_info
from acts.controllers.anritsu_lib._anritsu_utils import AnritsuError
from acts.controllers.anritsu_lib.md8475a import MD8475A
from acts.controllers.anritsu_lib.md8475a import BtsBandwidth
from acts.controllers.anritsu_lib.md8475a import BtsPacketRate
from acts.controllers.anritsu_lib.md8475a import VirtualPhoneStatus
from acts.test_utils.tel.anritsu_utils import cb_serial_number
from acts.test_utils.tel.anritsu_utils import set_system_model_1x
from acts.test_utils.tel.anritsu_utils import set_system_model_gsm
from acts.test_utils.tel.anritsu_utils import load_system_model_from_config_files
from acts.test_utils.tel.anritsu_utils import load_system_model_from_config_files_ca
from acts.test_utils.tel.anritsu_utils import set_system_model_lte
from acts.test_utils.tel.anritsu_utils import set_system_model_lte_wcdma
from acts.test_utils.tel.anritsu_utils import set_system_model_wcdma
from acts.test_utils.tel.anritsu_utils import sms_mo_send
from acts.test_utils.tel.anritsu_utils import sms_mt_receive_verify
from acts.test_utils.tel.anritsu_utils import set_usim_parameters
from acts.test_utils.tel.anritsu_utils import set_post_sim_params
from acts.test_utils.tel.tel_defines import DIRECTION_MOBILE_ORIGINATED
from acts.test_utils.tel.tel_defines import DIRECTION_MOBILE_TERMINATED
from acts.test_utils.tel.tel_defines import NETWORK_MODE_CDMA
from acts.test_utils.tel.tel_defines import NETWORK_MODE_GSM_ONLY
from acts.test_utils.tel.tel_defines import NETWORK_MODE_GSM_UMTS
from acts.test_utils.tel.tel_defines import NETWORK_MODE_LTE_GSM_WCDMA
from acts.test_utils.tel.tel_defines import NETWORK_MODE_LTE_CDMA_EVDO
from acts.test_utils.tel.tel_defines import RAT_1XRTT
from acts.test_utils.tel.tel_defines import RAT_GSM
from acts.test_utils.tel.tel_defines import RAT_LTE
from acts.test_utils.tel.tel_defines import RAT_WCDMA
from acts.test_utils.tel.tel_defines import RAT_FAMILY_CDMA2000
from acts.test_utils.tel.tel_defines import RAT_FAMILY_GSM
from acts.test_utils.tel.tel_defines import RAT_FAMILY_LTE
from acts.test_utils.tel.tel_defines import RAT_FAMILY_UMTS
from acts.test_utils.tel.tel_defines import NETWORK_SERVICE_DATA
from acts.test_utils.tel.tel_defines import GEN_4G
from acts.test_utils.tel.tel_test_utils import ensure_network_rat
from acts.test_utils.tel.tel_test_utils import ensure_phones_idle
from acts.test_utils.tel.tel_test_utils import ensure_network_generation
from acts.test_utils.tel.tel_test_utils import toggle_airplane_mode
from acts.test_utils.tel.tel_test_utils import iperf_test_by_adb
from acts.test_utils.wifi import wifi_power_test_utils as wputils

from acts.utils import adb_shell_ping
from acts.utils import rand_ascii_str
from acts.controllers import iperf_server
from acts.utils import exe_cmd

DEFAULT_PING_DURATION = 30


SCHEDULING_DYNAMIC = 0
SCHEDULING_MIN_MCS = 1
SCHEDULING_MAX_MCS = 2

DIRECTION_UPLINK = 0
DIRECTION_DOWNLINK = 1

TM1 = 1
TM4 = 4

class PowerTelTest(base_test.BaseTestClass):

    SETTLING_TIME = 10


    def __init__(self, controllers):
        base_test.BaseTestClass.__init__(self, controllers)

        self.ad = self.android_devices[0]
        self.iperf_server = self.iperf_servers[0]
        self.port_num = self.iperf_server.port
        self.log.info("Iperf Port is %s", self.port_num)
        self.ad.sim_card = getattr(self.ad, "sim_card", None)
        self.log.info("SIM Card is %s", self.ad.sim_card)
        self.md8475a_ip_address = self.user_params[
            "anritsu_md8475a_ip_address"]
        self.wlan_option = self.user_params.get("anritsu_wlan_option", False)

        # Load power level values
        self.big_step = self.user_params.get("big_step", 10)
        self.small_step_range = self.user_params.get("small_step_range", [])
        self.small_step = self.user_params.get("small_step", 3)

        # Load power levels
        self.uplink_power_levels = self.get_power_levels(
            small_step_range = self.user_params.get("uplink_small_step_range", []),
            big_step_range = self.user_params.get("uplink_big_step_range", [])
        )

        self.downlink_power_levels = self.get_power_levels(
            small_step_range = self.user_params.get("downlink_small_step_range", []),
            big_step_range = self.user_params.get("downlink_big_step_range", [])
        )

        # Setup sampling durations
        self.mon_offset = self.user_params.get("monsoon_offset", 15)
        self.mon_duration = self.user_params.get("monsoon_sampling_time", 10) 
        self.iperf_offset = self.user_params.get("iperf_offset", 5)
        self.iperf_duration = self.mon_duration + self.iperf_offset + self.mon_offset 

        # Setup monsoon
        self.mon_freq = 5000
        self.mon_data_path = os.path.join(self.log_path, 'Monsoon')
        self.mon = self.monsoons[0]
        self.mon.set_max_current(8.0)
        self.mon.set_voltage(4.2)
        self.mon.attach_device(self.ad)
        self.mon_info = wputils.create_monsoon_info(self)

        # Fetch IP address of the host machine
        self.ip = scapy.get_if_addr(self.user_params.get("interface", "eno1"))
        self.log.info("Dest IP is %s", self.ip)

    def setup_class(self):
        try:
            self.anritsu = MD8475A(self.md8475a_ip_address, self.log,
                                   self.wlan_option)
        except AnritsuError:
            self.log.error("Error in connecting to Anritsu Simulator")
            return False
        return True

    def setup_test(self):
        ensure_phones_idle(self.log, self.android_devices)
        wputils.dut_rockbottom(self.ad)
        return True

    def teardown_test(self):
        self.log.info("Stopping Simulation")
        self.anritsu.stop_simulation()
        toggle_airplane_mode(self.log, self.ad, True)
        return True

    def teardown_class(self):
        self.anritsu.disconnect()
        return True        
    
    def get_power_levels(self, small_step_range, big_step_range):
        
        power_levels = []
        if len(big_step_range) != 0:
            if len(big_step_range) != 2:
                self.log.error("big_step_range should contain an array with a min and max value for that part of the sweeping range.")
            else:
                if big_step_range[0] > big_step_range[1]:
                    aux = big_step_range[0]
                    big_step_range[0] = big_step_range[1]
                    big_step_range[1] = aux
                power_levels.extend(range(big_step_range[1], big_step_range[0], -self.big_step))
                power_levels.append(big_step_range[0])
        if len(small_step_range) != 0:
            if len(small_step_range) != 2:
                self.log.error("small_step_range should contain an array with a min and max value for that part of the sweeping range.")
            else:
                if small_step_range[0] > small_step_range[1]:
                    aux = small_step_range[0]
                    small_step_range[0] = small_step_range[1]
                    small_step_range[1] = aux
                power_levels.extend(range(small_step_range[1], small_step_range[0], -self.small_step))
                power_levels.append(small_step_range[0])
        print(str(power_levels))
        return power_levels

    def start_sitmulation(self, set_simulation_func, band, scheduling, bandwidth, transmission_mode):
        
        [self.bts1] = set_simulation_func(self.anritsu, self.user_params,
                              self.ad.sim_card)

        self.bts1.band = band
    
        if bandwidth == 20:
            self.bts1.bandwidth = BtsBandwidth.LTE_BANDWIDTH_20MHz
        elif bandwidth == 15:
            self.bts1.bandwidth = BtsBandwidth.LTE_BANDWIDTH_15MHz
        elif bandwidth == 10:
            self.bts1.bandwidth = BtsBandwidth.LTE_BANDWIDTH_10MHz
        elif bandwidth == 5:
            self.bts1.bandwidth = BtsBandwidth.LTE_BANDWIDTH_5MHz
        elif bandwidth == 3:
            self.bts1.bandwidth = BtsBandwidth.LTE_BANDWIDTH_3MHz
        elif bandwidth == 1.4:
            self.bts1.bandwidth = BtsBandwidth.LTE_BANDWIDTH_1dot4MHz

        if scheduling == SCHEDULING_DYNAMIC:
            self.bts1.lte_scheduling_mode = "DYNAMIC"
        else:
            
            self.bts1.lte_scheduling_mode = "STATIC"
            self.bts1.packet_rate = BtsPacketRate.LTE_MANUAL
            self.anritsu.send_command("TBSPATTERN OFF, BTS1")
            self.bts1.lte_mcs_dl = 0
            self.bts1.nrb_dl = 5 * bandwidth
            self.bts1.nrb_ul = 5 * bandwidth
            
            if scheduling == SCHEDULING_MIN_MCS:
                self.bts1.lte_mcs_ul = 0
            else:
                self.bts1.lte_mcs_ul = 23
    
        if transmission_mode == TM1:
            self.bts1.dl_antenna = 1
            self.bts1.transmode = "TM1"
        elif transmission_mode == TM4:
            self.bts1.dl_antenna = 2
            self.bts1.transmode = "TM4"
            
        self.anritsu.start_simulation()
       
    def start_sitmulation_ca(self, set_simulation_func):
        
        [self.bts1] = set_simulation_func(self.anritsu, self.user_params,
                              self.ad.sim_card)
            
        self.anritsu.start_simulation()
    
    def measure_throughput_and_power(self, direction):
            
        # Start iperf locally
        self.log.info("Starting iperf server.")
        self.iperf_server.start()

        self.log.info("Starting iperf client on the phone.")
        iperf_args = '-i 1 -t %d' % self.iperf_duration
        if direction == DIRECTION_DOWNLINK:
            iperf_args = iperf_args + ' -R'
        iperf_args = iperf_args + ' > /dev/null' 
        
        wputils.run_iperf_client_nonblocking(
            self.ad, self.ip, iperf_args)

        # Collect power data
        self.log.info("Starting sampling with monsoon.")
        file_path, current = wputils.monsoon_data_collect_save(
            self.ad, self.mon_info, self.current_test_name, bug_report=0)

        # Collect iperf data

        # Give some time for iperf to finish
        time.sleep(self.iperf_offset)

        self.iperf_server.stop()

        throughput = 0
        try:
            iperf_result = ipf.IPerfResult(self.iperf_server.log_files[-1])
            
            if direction == DIRECTION_DOWNLINK:
                if iperf_result.avg_send_rate is not None:
                    throughput = iperf_result.avg_send_rate * 8
            elif direction == DIRECTION_UPLINK:
                if iperf_result.avg_receive_rate is not None:
                    throughput = iperf_result.avg_receive_rate * 8
        except:
            pass
            
        self.log.info("Average receive rate: %sMbps", throughput)

        return [throughput, current]
    
    def sweep(self, power_levels, direction):
        
        if direction == DIRECTION_DOWNLINK:
            self.bts1.input_level = -40

        
        results_throughput = []
        results_power = []
        
        for power in power_levels:
            
            self.log.info("------- Measuring with power level %d dBm -------", power)
            
            if direction == DIRECTION_DOWNLINK:
                self.bts1.output_level = power
            elif direction == DIRECTION_UPLINK:
                self.bts1.input_level = power
            
            self.log.info("Current Power Level is %s dBm", power)

            
            throughput, current = self.measure_throughput_and_power(direction)
            results_throughput.append(throughput)
            results_power.append(current)


        return [results_throughput, results_power]

    def set_to_rockbottom_and_attach(self):
        
        self.bts1.input_power = -10
        self.bts1.output_power = -30
        
        # Set device to rockbottom
        self.ad.droid.goToSleepNow()
        
        # Turn of airplane mode and wait until the phone attaches
        toggle_airplane_mode(self.log, self.ad, False)
        time.sleep(2)
        self.anritsu.wait_for_registration_state()
        time.sleep(self.SETTLING_TIME)
        self.log.info("UE attached to the callbox.")

    def save_results(self, results_throughput, results_power, power_levels, file_name = ""):

        if file_name == "":
            file_name = self.current_test_name
        
        self.logpath = os.path.join(logging.log_path, self.current_test_name + ".csv")
        with open(self.logpath, "a") as tput_file:
            tput_file.write("# rf_power, current, throughput")
            tput_file.write("\n")
            for i in range(0, len(results_power)):
                tput_file.write(str(power_levels[i]) + ", " + str(results_power[i]) + ", " + str(results_throughput[i]))
                tput_file.write("\n")

    def do_test(self, direction, band, scheduling, bandwidth, transmission_mode, ca_band2 = 0):
        
        if direction == DIRECTION_DOWNLINK:
            power_levels = self.downlink_power_levels
        elif direction == DIRECTION_UPLINK:
            power_levels = self.uplink_power_levels
        
        if ca_band2 == 0:
            self.start_sitmulation(load_system_model_from_config_files, band, scheduling, bandwidth, transmission_mode)
        else:
            self.start_sitmulation_ca(load_system_model_from_config_files_ca, band, ca_band2, scheduling, bandwidth)
            
        self.set_to_rockbottom_and_attach()
        results_throughput, results_power = self.sweep(power_levels, direction)
        self.save_results(results_throughput, results_power, power_levels)

    def do_test_ca(self, direction):
        
        if direction == DIRECTION_DOWNLINK:
            power_levels = self.downlink_power_levels
        elif direction == DIRECTION_UPLINK:
            power_levels = self.uplink_power_levels
        
        self.start_sitmulation_ca(load_system_model_from_config_files_ca)
            
        self.set_to_rockbottom_and_attach()
        results_throughput, results_power = self.sweep(power_levels, direction)
        self.save_results(results_throughput, results_power, power_levels)



    """ Tests Begin """
	
    def test_downlink_tm1_band4_14MHz_dynamic(self):

        self.do_test(direction = DIRECTION_DOWNLINK, band = 4, scheduling = SCHEDULING_DYNAMIC, bandwidth = 1.4, transmission_mode = TM1, ca_band2 = 0)

    def test_uplink_tm1_band4_14MHz_min_mcs(self):

        self.do_test(direction = DIRECTION_UPLINK, band = 4, scheduling = SCHEDULING_MIN_MCS, bandwidth = 1.4, transmission_mode = TM1, ca_band2 = 0)

    def test_uplink_tm1_band4_14MHz_max_mcs(self):

        self.do_test(direction = DIRECTION_UPLINK, band = 4, scheduling = SCHEDULING_MAX_MCS, bandwidth = 1.4, transmission_mode = TM1, ca_band2 = 0)

    def test_downlink_tm4_band4_14MHz_dynamic(self):

        self.do_test(direction = DIRECTION_DOWNLINK, band = 4, scheduling = SCHEDULING_DYNAMIC, bandwidth = 1.4, transmission_mode = TM4, ca_band2 = 0)

    def test_uplink_tm4_band4_14MHz_min_mcs(self):

        self.do_test(direction = DIRECTION_UPLINK, band = 4, scheduling = SCHEDULING_MIN_MCS, bandwidth = 1.4, transmission_mode = TM4, ca_band2 = 0)

    def test_uplink_tm4_band4_14MHz_max_mcs(self):

        self.do_test(direction = DIRECTION_UPLINK, band = 4, scheduling = SCHEDULING_MAX_MCS, bandwidth = 1.4, transmission_mode = TM4, ca_band2 = 0)

    def test_downlink_tm1_band4_3MHz_dynamic(self):

        self.do_test(direction = DIRECTION_DOWNLINK, band = 4, scheduling = SCHEDULING_DYNAMIC, bandwidth = 3, transmission_mode = TM1, ca_band2 = 0)

    def test_uplink_tm1_band4_3MHz_min_mcs(self):

        self.do_test(direction = DIRECTION_UPLINK, band = 4, scheduling = SCHEDULING_MIN_MCS, bandwidth = 3, transmission_mode = TM1, ca_band2 = 0)

    def test_uplink_tm1_band4_3MHz_max_mcs(self):

        self.do_test(direction = DIRECTION_UPLINK, band = 4, scheduling = SCHEDULING_MAX_MCS, bandwidth = 3, transmission_mode = TM1, ca_band2 = 0)

    def test_downlink_tm4_band4_3MHz_dynamic(self):

        self.do_test(direction = DIRECTION_DOWNLINK, band = 4, scheduling = SCHEDULING_DYNAMIC, bandwidth = 3, transmission_mode = TM4, ca_band2 = 0)

    def test_uplink_tm4_band4_3MHz_min_mcs(self):

        self.do_test(direction = DIRECTION_UPLINK, band = 4, scheduling = SCHEDULING_MIN_MCS, bandwidth = 3, transmission_mode = TM4, ca_band2 = 0)

    def test_uplink_tm4_band4_3MHz_max_mcs(self):

        self.do_test(direction = DIRECTION_UPLINK, band = 4, scheduling = SCHEDULING_MAX_MCS, bandwidth = 3, transmission_mode = TM4, ca_band2 = 0)

    def test_downlink_tm1_band4_5MHz_dynamic(self):

        self.do_test(direction = DIRECTION_DOWNLINK, band = 4, scheduling = SCHEDULING_DYNAMIC, bandwidth = 5, transmission_mode = TM1, ca_band2 = 0)

    def test_uplink_tm1_band4_5MHz_min_mcs(self):

        self.do_test(direction = DIRECTION_UPLINK, band = 4, scheduling = SCHEDULING_MIN_MCS, bandwidth = 5, transmission_mode = TM1, ca_band2 = 0)

    def test_uplink_tm1_band4_5MHz_max_mcs(self):

        self.do_test(direction = DIRECTION_UPLINK, band = 4, scheduling = SCHEDULING_MAX_MCS, bandwidth = 5, transmission_mode = TM1, ca_band2 = 0)

    def test_downlink_tm4_band4_5MHz_dynamic(self):

        self.do_test(direction = DIRECTION_DOWNLINK, band = 4, scheduling = SCHEDULING_DYNAMIC, bandwidth = 5, transmission_mode = TM4, ca_band2 = 0)

    def test_uplink_tm4_band4_5MHz_min_mcs(self):

        self.do_test(direction = DIRECTION_UPLINK, band = 4, scheduling = SCHEDULING_MIN_MCS, bandwidth = 5, transmission_mode = TM4, ca_band2 = 0)

    def test_uplink_tm4_band4_5MHz_max_mcs(self):

        self.do_test(direction = DIRECTION_UPLINK, band = 4, scheduling = SCHEDULING_MAX_MCS, bandwidth = 5, transmission_mode = TM4, ca_band2 = 0)

    def test_downlink_tm1_band4_10MHz_dynamic(self):

        self.do_test(direction = DIRECTION_DOWNLINK, band = 4, scheduling = SCHEDULING_DYNAMIC, bandwidth = 10, transmission_mode = TM1, ca_band2 = 0)

    def test_uplink_tm1_band4_10MHz_min_mcs(self):

        self.do_test(direction = DIRECTION_UPLINK, band = 4, scheduling = SCHEDULING_MIN_MCS, bandwidth = 10, transmission_mode = TM1, ca_band2 = 0)

    def test_uplink_tm1_band4_10MHz_max_mcs(self):

        self.do_test(direction = DIRECTION_UPLINK, band = 4, scheduling = SCHEDULING_MAX_MCS, bandwidth = 10, transmission_mode = TM1, ca_band2 = 0)

    def test_downlink_tm4_band4_10MHz_dynamic(self):

        self.do_test(direction = DIRECTION_DOWNLINK, band = 4, scheduling = SCHEDULING_DYNAMIC, bandwidth = 10, transmission_mode = TM4, ca_band2 = 0)

    def test_uplink_tm4_band4_10MHz_min_mcs(self):

        self.do_test(direction = DIRECTION_UPLINK, band = 4, scheduling = SCHEDULING_MIN_MCS, bandwidth = 10, transmission_mode = TM4, ca_band2 = 0)

    def test_uplink_tm4_band4_10MHz_max_mcs(self):

        self.do_test(direction = DIRECTION_UPLINK, band = 4, scheduling = SCHEDULING_MAX_MCS, bandwidth = 10, transmission_mode = TM4, ca_band2 = 0)

    def test_downlink_tm1_band4_20MHz_dynamic(self):

        self.do_test(direction = DIRECTION_DOWNLINK, band = 4, scheduling = SCHEDULING_DYNAMIC, bandwidth = 20, transmission_mode = TM1, ca_band2 = 0)

    def test_uplink_tm1_band4_20MHz_min_mcs(self):

        self.do_test(direction = DIRECTION_UPLINK, band = 4, scheduling = SCHEDULING_MIN_MCS, bandwidth = 20, transmission_mode = TM1, ca_band2 = 0)

    def test_uplink_tm1_band4_20MHz_max_mcs(self):

        self.do_test(direction = DIRECTION_UPLINK, band = 4, scheduling = SCHEDULING_MAX_MCS, bandwidth = 20, transmission_mode = TM1, ca_band2 = 0)

    def test_downlink_tm4_band4_20MHz_dynamic(self):

        self.do_test(direction = DIRECTION_DOWNLINK, band = 4, scheduling = SCHEDULING_DYNAMIC, bandwidth = 20, transmission_mode = TM4, ca_band2 = 0)

    def test_uplink_tm4_band4_20MHz_min_mcs(self):

        self.do_test(direction = DIRECTION_UPLINK, band = 4, scheduling = SCHEDULING_MIN_MCS, bandwidth = 20, transmission_mode = TM4, ca_band2 = 0)

    def test_uplink_tm4_band4_20MHz_max_mcs(self):

        self.do_test(direction = DIRECTION_UPLINK, band = 4, scheduling = SCHEDULING_MAX_MCS, bandwidth = 20, transmission_mode = TM4, ca_band2 = 0)

    def test_downlink_tm1_band7_5MHz_dynamic(self):

        self.do_test(direction = DIRECTION_DOWNLINK, band = 7, scheduling = SCHEDULING_DYNAMIC, bandwidth = 5, transmission_mode = TM1, ca_band2 = 0)

    def test_uplink_tm1_band7_5MHz_min_mcs(self):

        self.do_test(direction = DIRECTION_UPLINK, band = 7, scheduling = SCHEDULING_MIN_MCS, bandwidth = 5, transmission_mode = TM1, ca_band2 = 0)

    def test_uplink_tm1_band7_5MHz_max_mcs(self):

        self.do_test(direction = DIRECTION_UPLINK, band = 7, scheduling = SCHEDULING_MAX_MCS, bandwidth = 5, transmission_mode = TM1, ca_band2 = 0)

    def test_downlink_tm4_band7_5MHz_dynamic(self):

        self.do_test(direction = DIRECTION_DOWNLINK, band = 7, scheduling = SCHEDULING_DYNAMIC, bandwidth = 5, transmission_mode = TM4, ca_band2 = 0)

    def test_uplink_tm4_band7_5MHz_min_mcs(self):

        self.do_test(direction = DIRECTION_UPLINK, band = 7, scheduling = SCHEDULING_MIN_MCS, bandwidth = 5, transmission_mode = TM4, ca_band2 = 0)

    def test_uplink_tm4_band7_5MHz_max_mcs(self):

        self.do_test(direction = DIRECTION_UPLINK, band = 7, scheduling = SCHEDULING_MAX_MCS, bandwidth = 5, transmission_mode = TM4, ca_band2 = 0)

    def test_downlink_tm1_band7_10MHz_dynamic(self):

        self.do_test(direction = DIRECTION_DOWNLINK, band = 7, scheduling = SCHEDULING_DYNAMIC, bandwidth = 10, transmission_mode = TM1, ca_band2 = 0)

    def test_uplink_tm1_band7_10MHz_min_mcs(self):

        self.do_test(direction = DIRECTION_UPLINK, band = 7, scheduling = SCHEDULING_MIN_MCS, bandwidth = 10, transmission_mode = TM1, ca_band2 = 0)

    def test_uplink_tm1_band7_10MHz_max_mcs(self):

        self.do_test(direction = DIRECTION_UPLINK, band = 7, scheduling = SCHEDULING_MAX_MCS, bandwidth = 10, transmission_mode = TM1, ca_band2 = 0)

    def test_downlink_tm4_band7_10MHz_dynamic(self):

        self.do_test(direction = DIRECTION_DOWNLINK, band = 7, scheduling = SCHEDULING_DYNAMIC, bandwidth = 10, transmission_mode = TM4, ca_band2 = 0)

    def test_uplink_tm4_band7_10MHz_min_mcs(self):

        self.do_test(direction = DIRECTION_UPLINK, band = 7, scheduling = SCHEDULING_MIN_MCS, bandwidth = 10, transmission_mode = TM4, ca_band2 = 0)

    def test_uplink_tm4_band7_10MHz_max_mcs(self):

        self.do_test(direction = DIRECTION_UPLINK, band = 7, scheduling = SCHEDULING_MAX_MCS, bandwidth = 10, transmission_mode = TM4, ca_band2 = 0)

    def test_downlink_tm1_band7_20MHz_dynamic(self):

        self.do_test(direction = DIRECTION_DOWNLINK, band = 7, scheduling = SCHEDULING_DYNAMIC, bandwidth = 20, transmission_mode = TM1, ca_band2 = 0)

    def test_uplink_tm1_band7_20MHz_min_mcs(self):

        self.do_test(direction = DIRECTION_UPLINK, band = 7, scheduling = SCHEDULING_MIN_MCS, bandwidth = 20, transmission_mode = TM1, ca_band2 = 0)

    def test_uplink_tm1_band7_20MHz_max_mcs(self):

        self.do_test(direction = DIRECTION_UPLINK, band = 7, scheduling = SCHEDULING_MAX_MCS, bandwidth = 20, transmission_mode = TM1, ca_band2 = 0)

    def test_downlink_tm4_band7_20MHz_dynamic(self):

        self.do_test(direction = DIRECTION_DOWNLINK, band = 7, scheduling = SCHEDULING_DYNAMIC, bandwidth = 20, transmission_mode = TM4, ca_band2 = 0)

    def test_uplink_tm4_band7_20MHz_min_mcs(self):

        self.do_test(direction = DIRECTION_UPLINK, band = 7, scheduling = SCHEDULING_MIN_MCS, bandwidth = 20, transmission_mode = TM4, ca_band2 = 0)

    def test_uplink_tm4_band7_20MHz_max_mcs(self):

        self.do_test(direction = DIRECTION_UPLINK, band = 7, scheduling = SCHEDULING_MAX_MCS, bandwidth = 20, transmission_mode = TM4, ca_band2 = 0)

    def test_downlink_tm1_band13_5MHz_dynamic(self):

        self.do_test(direction = DIRECTION_DOWNLINK, band = 13, scheduling = SCHEDULING_DYNAMIC, bandwidth = 5, transmission_mode = TM1, ca_band2 = 0)

    def test_uplink_tm1_band13_5MHz_min_mcs(self):

        self.do_test(direction = DIRECTION_UPLINK, band = 13, scheduling = SCHEDULING_MIN_MCS, bandwidth = 5, transmission_mode = TM1, ca_band2 = 0)

    def test_uplink_tm1_band13_5MHz_max_mcs(self):

        self.do_test(direction = DIRECTION_UPLINK, band = 13, scheduling = SCHEDULING_MAX_MCS, bandwidth = 5, transmission_mode = TM1, ca_band2 = 0)

    def test_downlink_tm4_band13_5MHz_dynamic(self):

        self.do_test(direction = DIRECTION_DOWNLINK, band = 13, scheduling = SCHEDULING_DYNAMIC, bandwidth = 5, transmission_mode = TM4, ca_band2 = 0)

    def test_uplink_tm4_band13_5MHz_min_mcs(self):

        self.do_test(direction = DIRECTION_UPLINK, band = 13, scheduling = SCHEDULING_MIN_MCS, bandwidth = 5, transmission_mode = TM4, ca_band2 = 0)

    def test_uplink_tm4_band13_5MHz_max_mcs(self):

        self.do_test(direction = DIRECTION_UPLINK, band = 13, scheduling = SCHEDULING_MAX_MCS, bandwidth = 5, transmission_mode = TM4, ca_band2 = 0)

    def test_downlink_tm1_band13_10MHz_dynamic(self):

        self.do_test(direction = DIRECTION_DOWNLINK, band = 13, scheduling = SCHEDULING_DYNAMIC, bandwidth = 10, transmission_mode = TM1, ca_band2 = 0)

    def test_uplink_tm1_band13_10MHz_min_mcs(self):

        self.do_test(direction = DIRECTION_UPLINK, band = 13, scheduling = SCHEDULING_MIN_MCS, bandwidth = 10, transmission_mode = TM1, ca_band2 = 0)

    def test_uplink_tm1_band13_10MHz_max_mcs(self):

        self.do_test(direction = DIRECTION_UPLINK, band = 13, scheduling = SCHEDULING_MAX_MCS, bandwidth = 10, transmission_mode = TM1, ca_band2 = 0)

    def test_downlink_tm4_band13_10MHz_dynamic(self):

        self.do_test(direction = DIRECTION_DOWNLINK, band = 13, scheduling = SCHEDULING_DYNAMIC, bandwidth = 10, transmission_mode = TM4, ca_band2 = 0)

    def test_uplink_tm4_band13_10MHz_min_mcs(self):

        self.do_test(direction = DIRECTION_UPLINK, band = 13, scheduling = SCHEDULING_MIN_MCS, bandwidth = 10, transmission_mode = TM4, ca_band2 = 0)

    def test_uplink_tm4_band13_10MHz_max_mcs(self):

        self.do_test(direction = DIRECTION_UPLINK, band = 13, scheduling = SCHEDULING_MAX_MCS, bandwidth = 10, transmission_mode = TM4, ca_band2 = 0)

    def test_downlink_ca_20MHz(self):
        self.do_test_ca(DIRECTION_DOWNLINK)

    def test_uplink_ca_20MHz(self):
        self.do_test_ca(DIRECTION_UPLINK)

    """ Tests End """
