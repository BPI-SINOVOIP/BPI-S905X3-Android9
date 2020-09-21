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
import acts
import json
import logging
import math
import os
import time
import acts.controllers.iperf_server as ipf
from acts import asserts
from acts import base_test
from acts import utils
from acts.controllers import monsoon
from acts.test_utils.wifi import wifi_test_utils as wutils
from acts.test_utils.wifi import wifi_power_test_utils as wputils

SETTINGS_PAGE = 'am start -n com.android.settings/.Settings'
SCROLL_BOTTOM = 'input swipe 0 2000 0 0'
UNLOCK_SCREEN = 'input keyevent 82'
SCREENON_USB_DISABLE = 'dumpsys battery unplug'
RESET_BATTERY_STATS = 'dumpsys batterystats --reset'
AOD_OFF = 'settings put secure doze_always_on 0'
MUSIC_IQ_OFF = 'pm disable-user com.google.intelligence.sense'
# Command to disable gestures
LIFT = 'settings put secure doze_pulse_on_pick_up 0'
DOUBLE_TAP = 'settings put secure doze_pulse_on_double_tap 0'
JUMP_TO_CAMERA = 'settings put secure camera_double_tap_power_gesture_disabled 1'
RAISE_TO_CAMERA = 'settings put secure camera_lift_trigger_enabled 0'
FLIP_CAMERA = 'settings put secure camera_double_twist_to_flip_enabled 0'
ASSIST_GESTURE = 'settings put secure assist_gesture_enabled 0'
ASSIST_GESTURE_ALERT = 'settings put secure assist_gesture_silence_alerts_enabled 0'
ASSIST_GESTURE_WAKE = 'settings put secure assist_gesture_wake_enabled 0'
SYSTEM_NAVI = 'settings put secure system_navigation_keys_enabled 0'
# End of command to disable gestures
AUTO_TIME_OFF = 'settings put global auto_time 0'
AUTO_TIMEZONE_OFF = 'settings put global auto_time_zone 0'
FORCE_YOUTUBE_STOP = 'am force-stop com.google.android.youtube'
FORCE_DIALER_STOP = 'am force-stop com.google.android.dialer'
IPERF_TIMEOUT = 180
THRESHOLD_TOLERANCE = 0.2
GET_FROM_PHONE = 'get_from_dut'
GET_FROM_AP = 'get_from_ap'
PHONE_BATTERY_VOLTAGE = 4.2
MONSOON_MAX_CURRENT = 8.0
MONSOON_RETRY_INTERVAL = 300
MEASUREMENT_RETRY_COUNT = 3
RECOVER_MONSOON_RETRY_COUNT = 3
MIN_PERCENT_SAMPLE = 95
ENABLED_MODULATED_DTIM = 'gEnableModulatedDTIM='
MAX_MODULATED_DTIM = 'gMaxLIModulatedDTIM='
TEMP_FILE = '/sdcard/Download/tmp.log'
IPERF_DURATION = 'iperf_duration'
INITIAL_ATTEN = [0, 0, 90, 90]


class ObjNew():
    """Create a random obj with unknown attributes and value.

    """

    def __init__(self, **kwargs):
        self.__dict__.update(kwargs)

    def __contains__(self, item):
        """Function to check if one attribute is contained in the object.

        Args:
            item: the item to check
        Return:
            True/False
        """
        return hasattr(self, item)


class PowerBaseTest(base_test.BaseTestClass):
    """Base class for all wireless power related tests.

    """

    def __init__(self, controllers):

        base_test.BaseTestClass.__init__(self, controllers)

    def setup_class(self):

        self.log = logging.getLogger()
        self.tests = self._get_all_test_names()

        # Setup the must have controllers, phone and monsoon
        self.dut = self.android_devices[0]
        self.mon_data_path = os.path.join(self.log_path, 'Monsoon')
        self.mon = self.monsoons[0]
        self.mon.set_max_current(8.0)
        self.mon.set_voltage(4.2)
        self.mon.attach_device(self.dut)

        # Unpack the test/device specific parameters
        TEST_PARAMS = self.TAG + '_params'
        req_params = [TEST_PARAMS, 'custom_files']
        self.unpack_userparams(req_params)
        # Unpack the custom files based on the test configs
        for file in self.custom_files:
            if 'pass_fail_threshold_' + self.dut.model in file:
                self.threshold_file = file
            elif 'attenuator_setting' in file:
                self.attenuation_file = file
            elif 'network_config' in file:
                self.network_file = file

        # Unpack test specific configs
        self.unpack_testparams(getattr(self, TEST_PARAMS))
        if hasattr(self, 'attenuators'):
            self.num_atten = self.attenuators[0].instrument.num_atten
            self.atten_level = self.unpack_custom_file(self.attenuation_file)
        self.set_attenuation(INITIAL_ATTEN)
        self.threshold = self.unpack_custom_file(self.threshold_file)
        self.mon_info = self.create_monsoon_info()

        # Onetime task for each test class
        # Temporary fix for b/77873679
        self.adb_disable_verity()
        self.dut.adb.shell('mv /vendor/bin/chre /vendor/bin/chre_renamed')
        self.dut.adb.shell('pkill chre')

    def setup_test(self):
        """Set up test specific parameters or configs.

        """
        # Set the device into rockbottom state
        self.dut_rockbottom()
        # Wait for extra time if needed for the first test
        if hasattr(self, 'extra_wait'):
            self.more_wait_first_test()

    def teardown_test(self):
        """Tear down necessary objects after test case is finished.

        """
        self.log.info('Tearing down the test case')
        self.mon.usb('on')

    def teardown_class(self):
        """Clean up the test class after tests finish running

        """
        self.log.info('Tearing down the test class')
        self.mon.usb('on')

    def unpack_testparams(self, bulk_params):
        """Unpack all the test specific parameters.

        Args:
            bulk_params: dict with all test specific params in the config file
        """
        for key in bulk_params.keys():
            setattr(self, key, bulk_params[key])

    def unpack_custom_file(self, file, test_specific=True):
        """Unpack the pass_fail_thresholds from a common file.

        Args:
            file: the common file containing pass fail threshold.
        """
        with open(file, 'r') as f:
            params = json.load(f)
        if test_specific:
            try:
                return params[self.TAG]
            except KeyError:
                pass
        else:
            return params

    def decode_test_configs(self, attrs, indices):
        """Decode the test config/params from test name.

        Remove redundant function calls when tests are similar.
        Args:
            attrs: a list of the attrs of the test config obj
            indices: a list of the location indices of keyword in the test name.
        """
        # Decode test parameters for the current test
        test_params = self.current_test_name.split('_')
        values = [test_params[x] for x in indices]
        config_dict = dict(zip(attrs, values))
        self.test_configs = ObjNew(**config_dict)

    def more_wait_first_test(self):
        # For the first test, increase the offset for longer wait time
        if self.current_test_name == self.tests[0]:
            self.mon_info.offset = self.mon_offset + self.extra_wait
        else:
            self.mon_info.offset = self.mon_offset

    def set_attenuation(self, atten_list):
        """Function to set the attenuator to desired attenuations.

        Args:
            atten_list: list containing the attenuation for each attenuator.
        """
        if len(atten_list) != self.num_atten:
            raise Exception('List given does not have the correct length')
        for i in range(self.num_atten):
            self.attenuators[i].set_atten(atten_list[i])

    def dut_rockbottom(self):
        """Set the phone into Rock-bottom state.

        """
        self.dut.log.info('Now set the device to Rockbottom State')
        utils.require_sl4a((self.dut, ))
        self.dut.droid.connectivityToggleAirplaneMode(False)
        time.sleep(2)
        self.dut.droid.connectivityToggleAirplaneMode(True)
        time.sleep(2)
        utils.set_ambient_display(self.dut, False)
        utils.set_auto_rotate(self.dut, False)
        utils.set_adaptive_brightness(self.dut, False)
        utils.sync_device_time(self.dut)
        utils.set_location_service(self.dut, False)
        utils.set_mobile_data_always_on(self.dut, False)
        utils.disable_doze_light(self.dut)
        utils.disable_doze(self.dut)
        wutils.reset_wifi(self.dut)
        wutils.wifi_toggle_state(self.dut, False)
        try:
            self.dut.droid.nfcDisable()
        except acts.controllers.sl4a_lib.rpc_client.Sl4aApiError:
            self.dut.log.info('NFC is not available')
        self.dut.droid.setScreenBrightness(0)
        self.dut.adb.shell(AOD_OFF)
        self.dut.droid.setScreenTimeout(2200)
        self.dut.droid.wakeUpNow()
        self.dut.adb.shell(LIFT)
        self.dut.adb.shell(DOUBLE_TAP)
        self.dut.adb.shell(JUMP_TO_CAMERA)
        self.dut.adb.shell(RAISE_TO_CAMERA)
        self.dut.adb.shell(FLIP_CAMERA)
        self.dut.adb.shell(ASSIST_GESTURE)
        self.dut.adb.shell(ASSIST_GESTURE_ALERT)
        self.dut.adb.shell(ASSIST_GESTURE_WAKE)
        self.dut.adb.shell(SCREENON_USB_DISABLE)
        self.dut.adb.shell(UNLOCK_SCREEN)
        self.dut.adb.shell(SETTINGS_PAGE)
        self.dut.adb.shell(SCROLL_BOTTOM)
        self.dut.adb.shell(MUSIC_IQ_OFF)
        self.dut.adb.shell(AUTO_TIME_OFF)
        self.dut.adb.shell(AUTO_TIMEZONE_OFF)
        self.dut.adb.shell(FORCE_YOUTUBE_STOP)
        self.dut.adb.shell(FORCE_DIALER_STOP)
        self.dut.droid.wifiSetCountryCode('US')
        self.dut.droid.wakeUpNow()
        self.dut.log.info('Device has been set to Rockbottom state')
        self.dut.log.info('Screen is ON')

    def measure_power_and_validate(self):
        """The actual test flow and result processing and validate.

        """
        self.collect_power_data()
        self.pass_fail_check()

    def collect_power_data(self):
        """Measure power, plot and take log if needed.

        """
        tag = ''
        # Collecting current measurement data and plot
        begin_time = utils.get_current_epoch_time()
        self.file_path, self.test_result = self.monsoon_data_collect_save()
        wputils.monsoon_data_plot(self.mon_info, self.file_path, tag=tag)
        # Take Bugreport
        if self.bug_report:
            self.dut.take_bug_report(self.test_name, begin_time)

    def pass_fail_check(self):
        """Check the test result and decide if it passed or failed.

        The threshold is provided in the config file. In this class, result is
        current in mA.
        """
        current_threshold = self.threshold[self.test_name]
        if self.test_result:
            asserts.assert_true(
                abs(self.test_result - current_threshold) / current_threshold <
                THRESHOLD_TOLERANCE,
                ('Measured average current in [{}]: {}, which is '
                 'more than {} percent off than acceptable threshold {:.2f}mA'
                 ).format(self.test_name, self.test_result,
                          self.pass_fail_tolerance * 100, current_threshold))
            asserts.explicit_pass('Measurement finished for {}.'.format(
                self.test_name))
        else:
            asserts.fail(
                'Something happened, measurement is not complete, test failed')

    def create_monsoon_info(self):
        """Creates the config dictionary for monsoon

        Returns:
            mon_info: Dictionary with the monsoon packet config
        """
        if hasattr(self, IPERF_DURATION):
            self.mon_duration = self.iperf_duration - 10
        mon_info = ObjNew(
            dut=self.mon,
            freq=self.mon_freq,
            duration=self.mon_duration,
            offset=self.mon_offset,
            data_path=self.mon_data_path)
        return mon_info

    def monsoon_recover(self):
        """Test loop to wait for monsoon recover from unexpected error.

        Wait for a certain time duration, then quit.0
        Args:
            mon: monsoon object
        Returns:
            True/False
        """
        try:
            self.mon.reconnect_monsoon()
            time.sleep(2)
            self.mon.usb('on')
            logging.info('Monsoon recovered from unexpected error')
            time.sleep(2)
            return True
        except monsoon.MonsoonError:
            logging.info(self.mon.mon.ser.in_waiting)
            logging.warning('Unable to recover monsoon from unexpected error')
            return False

    def monsoon_data_collect_save(self):
        """Current measurement and save the log file.

        Collect current data using Monsoon box and return the path of the
        log file. Take bug report if requested.

        Returns:
            data_path: the absolute path to the log file of monsoon current
                       measurement
            avg_current: the average current of the test
        """

        tag = '{}_{}_{}'.format(self.test_name, self.dut.model,
                                self.dut.build_info['build_id'])
        data_path = os.path.join(self.mon_info.data_path, '{}.txt'.format(tag))
        total_expected_samples = self.mon_info.freq * (
            self.mon_info.duration + self.mon_info.offset)
        min_required_samples = total_expected_samples * MIN_PERCENT_SAMPLE / 100
        # Retry counter for monsoon data aquisition
        retry_measure = 1
        # Indicator that need to re-collect data
        need_collect_data = 1
        result = None
        while retry_measure <= MEASUREMENT_RETRY_COUNT:
            try:
                # If need to retake data
                if need_collect_data == 1:
                    #Resets the battery status right before the test started
                    self.dut.adb.shell(RESET_BATTERY_STATS)
                    self.log.info(
                        'Starting power measurement with monsoon box, try #{}'.
                        format(retry_measure))
                    #Start the power measurement using monsoon
                    self.mon_info.dut.monsoon_usb_auto()
                    result = self.mon_info.dut.measure_power(
                        self.mon_info.freq,
                        self.mon_info.duration,
                        tag=tag,
                        offset=self.mon_info.offset)
                    self.mon_info.dut.reconnect_dut()
                # Reconnect to dut
                else:
                    self.mon_info.dut.reconnect_dut()
                # Reconnect and return measurement results if no error happens
                avg_current = result.average_current
                monsoon.MonsoonData.save_to_text_file([result], data_path)
                self.log.info('Power measurement done within {} try'.format(
                    retry_measure))
                return data_path, avg_current
            # Catch monsoon errors during measurement
            except monsoon.MonsoonError:
                self.log.info(self.mon_info.dut.mon.ser.in_waiting)
                # Break early if it's one count away from limit
                if retry_measure == MEASUREMENT_RETRY_COUNT:
                    self.log.error(
                        'Test failed after maximum measurement retry')
                    break

                self.log.warning('Monsoon error happened, now try to recover')
                # Retry loop to recover monsoon from error
                retry_monsoon = 1
                while retry_monsoon <= RECOVER_MONSOON_RETRY_COUNT:
                    mon_status = self.monsoon_recover(self.mon_info.dut)
                    if mon_status:
                        break
                    else:
                        retry_monsoon += 1
                        self.log.warning(
                            'Wait for {} second then try again'.format(
                                MONSOON_RETRY_INTERVAL))
                        time.sleep(MONSOON_RETRY_INTERVAL)

                # Break the loop to end test if failed to recover monsoon
                if not mon_status:
                    self.log.error(
                        'Tried our best, still failed to recover monsoon')
                    break
                else:
                    # If there is no data, or captured samples are less than min
                    # required, re-take
                    if not result:
                        self.log.warning('No data taken, need to remeasure')
                    elif len(result._data_points) <= min_required_samples:
                        self.log.warning(
                            'More than {} percent of samples are missing due to monsoon error. Need to remeasure'.
                            format(100 - MIN_PERCENT_SAMPLE))
                    else:
                        need_collect_data = 0
                        self.log.warning(
                            'Data collected is valid, try reconnect to DUT to finish test'
                        )
                    retry_measure += 1

        if retry_measure > MEASUREMENT_RETRY_COUNT:
            self.log.error('Test failed after maximum measurement retry')

    def setup_ap_connection(self, network, bandwidth=80, connect=True):
        """Setup AP and connect DUT to it.

        Args:
            network: the network config for the AP to be setup
            bandwidth: bandwidth of the WiFi network to be setup
            connect: indicator of if connect dut to the network after setup
        Returns:
            self.brconfigs: dict for bridge interface configs
        """
        wutils.wifi_toggle_state(self.dut, True)
        self.brconfigs = wputils.ap_setup(
            self.access_point, network, bandwidth=bandwidth)
        if connect:
            wutils.wifi_connect(self.dut, network)
        return self.brconfigs

    def process_iperf_results(self):
        """Get the iperf results and process.

        Returns:
             throughput: the average throughput during tests.
        """
        # Get IPERF results and add this to the plot title
        RESULTS_DESTINATION = os.path.join(self.iperf_server.log_path,
                                           'iperf_client_output_{}.log'.format(
                                               self.current_test_name))
        PULL_FILE = '{} {}'.format(TEMP_FILE, RESULTS_DESTINATION)
        self.dut.adb.pull(PULL_FILE)
        # Calculate the average throughput
        if self.use_client_output:
            iperf_file = RESULTS_DESTINATION
        else:
            iperf_file = self.iperf_server.log_files[-1]
        try:
            iperf_result = ipf.IPerfResult(iperf_file)
            throughput = (math.fsum(iperf_result.instantaneous_rates[:-1]) /
                          len(iperf_result.instantaneous_rates[:-1])) * 8
            self.log.info('The average throughput is {}'.format(throughput))
        except ValueError:
            self.log.warning('Cannot get iperf result. Setting to 0')
            throughput = 0
        return throughput

    # TODO(@qijiang)Merge with tel_test_utils.py
    def adb_disable_verity(self):
        """Disable verity on the device.

        """
        if self.dut.adb.getprop("ro.boot.veritymode") == "enforcing":
            self.dut.adb.disable_verity()
            self.dut.reboot()
            self.dut.adb.root()
            self.dut.adb.remount()
