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
Sanity tests for voice tests in telephony
"""
import time, os

from acts.test_utils.tel.anritsu_utils import make_ims_call
from acts.test_utils.tel.anritsu_utils import tear_down_call
from acts.test_utils.tel.tel_test_utils import iperf_test_by_adb
from acts.test_utils.tel.TelephonyBaseTest import TelephonyBaseTest
from acts.test_utils.tel.TelephonyLabPowerTest import TelephonyLabPowerTest
from acts.utils import adb_shell_ping
from acts.controllers import iperf_server
from acts.utils import exe_cmd
import json

DEFAULT_PING_DURATION = 10
IPERF_DURATION = 30
IPERF_LOG_FILE_PATH = "/sdcard/iperf.txt"

DEFAULT_CALL_NUMBER = "+11234567891"
WAIT_TIME_VOLTE = 5


class TelLabPowerDataTest(TelephonyLabPowerTest):
    # TODO Keep if we want to add more in here for this class.
    def __init__(self, controllers):
        TelephonyLabPowerTest.__init__(self, controllers)
        self.ip_server = self.iperf_servers[0]
        self.port_num = self.ip_server.port
        self.log.info("Iperf Port is %s", self.port_num)
        self.log.info("End of __init__ class of TelLabPowerDataTest")

    # May not need
    def teardown_class(self):
        # Always take down the simulation
        TelephonyLabPowerTest.teardown_class(self)

    def iperf_setup(self):
        # Fetch IP address of the host machine
        cmd = "|".join(("ifconfig", "grep eth0 -A1", "grep inet",
                        "cut -d ':' -f2", "cut -d ' ' -f 1"))
        destination_ip = exe_cmd(cmd)
        destination_ip = (destination_ip.decode("utf-8")).split("\n")[0]
        self.log.info("Dest IP is %s", destination_ip)
        time.sleep(1)
        if not adb_shell_ping(
                self.ad, DEFAULT_PING_DURATION, destination_ip,
                loss_tolerance=95):
            self.log.error("Pings failed to Destination.")
            return False

        return destination_ip

    def _iperf_task(self, destination_ip, duration):
        self.log.info("Starting iPerf task")
        self.ip_server.start()
        tput_dict = {"Uplink": 0, "Downlink": 0}
        if iperf_test_by_adb(
                self.log,
                self.ad,
                destination_ip,
                self.port_num,
                True,  # reverse
                duration,
                rate_dict=tput_dict,
                blocking=False,
                log_file_path=IPERF_LOG_FILE_PATH):
            return True
        else:
            self.log.error("iperf failed to Destination.")
            self.ip_server.stop()
            return False

    def power_iperf_test(self, olvl, rflvl, sch_mode="DYNAMIC", volte=False):
        if volte:
            # make a VoLTE MO call
            self.log.info("DEFAULT_CALL_NUMBER = " + DEFAULT_CALL_NUMBER)
            if not make_ims_call(self.log, self.ad, self.anritsu,
                                 DEFAULT_CALL_NUMBER):
                self.log.error("Phone {} Failed to make volte call to {}"
                               .format(self.ad.serial, DEFAULT_CALL_NUMBER))
                return False
            self.log.info("wait for %d seconds" % WAIT_TIME_VOLTE)
            time.sleep(WAIT_TIME_VOLTE)

        server_ip = self.iperf_setup()
        if not server_ip:
            self.log.error("iperf server can not be reached by ping")
            return False

        self._iperf_task(server_ip, IPERF_DURATION)
        self.log.info("Wait for 10 secconds before power measurement")
        time.sleep(10)
        self.power_test(olvl, rflvl, sch_mode)

        result = self.ad.adb.shell("cat {}".format(IPERF_LOG_FILE_PATH))
        if result is not None:
            data_json = json.loads(''.join(result))
            rx_rate = data_json['end']['sum_received']['bits_per_second']
            xfer_time = data_json['end']['sum_received']['seconds']
            self.ad.log.info('iPerf3 transfer time was %ssecs', xfer_time)
            self.ad.log.info('iPerf3 download speed is %sbps', rx_rate)

        if volte:
            # check if the phone is still in call, then tear it down
            if not self.ad.droid.telecomIsInCall():
                self.log.error("Call is already ended in the phone.")
                return False
            if not tear_down_call(self.log, self.ad, self.anritsu):
                self.log.error("Phone {} Failed to tear down VoLTE call"
                               .format(self.ad.serial))
                return False

        return True

    """ Tests Begin """

    @TelephonyBaseTest.tel_test_wrap
    def test_data_power_n30_n30(self):
        """ Test power consumption for iPerf data @ DL/UL -30/-30dBm
        Steps:
        1. Assume UE already in Communication mode.
        2. Initiate iPerf data transfer.
        3. Set DL/UL power and Dynamic scheduling.
        4. Measure power consumption.

        Expected Results:
        1. power consumption measurement is successful
        2. measurement results is saved accordingly

        Returns:
            True if pass; False if fail
        """
        return self.power_iperf_test(-30, -30)

    @TelephonyBaseTest.tel_test_wrap
    def test_data_power_n50_n10(self):
        """ Test power consumption for iPerf data @ DL/UL -50/-10dBm
        Steps:
        1. Assume UE already in Communication mode.
        2. Initiate iPerf data transfer.
        3. Set DL/UL power and Dynamic scheduling.
        4. Measure power consumption.

        Expected Results:
        1. power consumption measurement is successful
        2. measurement results is saved accordingly

        Returns:
            True if pass; False if fail
        """
        return self.power_iperf_test(-50, -10)

    @TelephonyBaseTest.tel_test_wrap
    def test_data_power_n70_10(self):
        """ Test power consumption for iPerf data @ DL/UL -70/+10dBm
        Steps:
        1. Assume UE already in Communication mode.
        2. Initiate iPerf data transfer.
        3. Set DL/UL power and Dynamic scheduling.
        4. Measure power consumption.

        Expected Results:
        1. power consumption measurement is successful
        2. measurement results is saved accordingly

        Returns:
            True if pass; False if fail
        """
        return self.power_iperf_test(-70, 10)

    @TelephonyBaseTest.tel_test_wrap
    def test_data_volte_power_n30_n30(self):
        """ Test power consumption for iPerf data and volte @ DL/UL -30/-30dBm
        Steps:
        1. Assume UE already in Communication mode.
        2. Make MO VoLTE call.
        3. Initiate iPerf data transfer.
        4. Set DL/UL power and Dynamic scheduling.
        5. Measure power consumption.

        Expected Results:
        1. power consumption measurement is successful
        2. measurement results is saved accordingly

        Returns:
            True if pass; False if fail
        """
        return self.power_iperf_test(-30, -30, volte=True)

    @TelephonyBaseTest.tel_test_wrap
    def test_data_volte_power_n50_n10(self):
        """ Test power consumption for iPerf data and volte @ DL/UL -50/-10dBm
        Steps:
        1. Assume UE already in Communication mode.
        2. Make MO VoLTE call.
        3. Initiate iPerf data transfer.
        4. Set DL/UL power and Dynamic scheduling.
        5. Measure power consumption.

        Expected Results:
        1. power consumption measurement is successful
        2. measurement results is saved accordingly

        Returns:
            True if pass; False if fail
        """
        return self.power_iperf_test(-50, -10, volte=True)

    @TelephonyBaseTest.tel_test_wrap
    def test_data_volte_power_n70_10(self):
        """ Test power consumption for iPerf data and volte @ DL/UL -70/+10dBm
        Steps:
        1. Assume UE already in Communication mode.
        2. Make MO VoLTE call.
        3. Initiate iPerf data transfer.
        4. Set DL/UL power and Dynamic scheduling.
        5. Measure power consumption.

        Expected Results:
        1. power consumption measurement is successful
        2. measurement results is saved accordingly

        Returns:
            True if pass; False if fail
        """
        return self.power_iperf_test(-70, 10, volte=True)

    """ Tests End """
