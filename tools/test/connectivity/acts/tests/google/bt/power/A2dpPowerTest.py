#/usr/bin/env python3.4
#
# Copyright (C) 2017 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.
"""
This test script exercises power test scenarios for A2DP streaming with
   5 codec types, 4 sample rate values, 3 bits per sample values,
   2 music file types and 3 LDAC playback quality values.

This test script was designed with this setup in mind:
Shield box one: Android Device, headset and Monsoon tool box
"""

import json
import os
import time
import sys

from acts import logger
from acts.controllers import monsoon
from acts.keys import Config
from acts.test_decorators import test_tracker_info
from acts.test_utils.bt.bt_constants import ble_scan_settings_modes
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt.PowerBaseTest import PowerBaseTest
from acts.test_utils.bt.bt_test_utils import bluetooth_enabled_check
from acts.test_utils.bt.bt_test_utils import disable_bluetooth
from acts.test_utils.bt.bt_test_utils import reset_bluetooth
from acts.controllers.relay_lib.sony_xb2_speaker import SonyXB2Speaker


def push_file_to_device(ad, file_path, device_path, config_path):
    """Utility functions to push a file to android device

    Args:
        ad: Device for file push
        file_path: File path for the file to be pushed to the device
        device_path: File path on the device as destination
        config_path: File path to the config file.  This is only used when
                     a relative path is passed via the ACTS config.

    Returns:
        True if successful, False if unsuccessful.
    """

    if not os.path.isfile(file_path):
        file_path = os.path.join(config_path, file_path)
        if not os.path.isfile(file_path):
            return False
    ad.adb.push("{} {}".format(file_path, device_path))
    return True


class A2dpPowerTest(PowerBaseTest):
    # Time for measuring the power when playing music in seconds
    MEASURE_TIME = 400
    # Delay time in seconds between starting playing and measuring power
    DELAY_MEASURE_TIME = 300
    # Extra time to stop Music in PMC after power measurement is stopped
    DELAY_STOP_TIME = 10
    # Log file name
    LOG_FILE = "A2DPPOWER.log"
    # Wait time in seconds to check if Bluetooth device is connected
    WAIT_TIME = 10
    # Base command for PMC
    PMC_BASE_CMD = ("am broadcast -a com.android.pmc.A2DP")
    # Codec Types
    CODEC_SBC = 0
    CODEC_AAC = 1
    CODEC_APTX = 2
    CODEC_APTX_HD = 3
    CODEC_LDAC = 4

    # Music sample rates
    SAMPLE_RATE_44100 = 0x1 << 0
    SAMPLE_RATE_48000 = 0x1 << 1
    SAMPLE_RATE_88200 = 0x1 << 2
    SAMPLE_RATE_96000 = 0x1 << 3

    # Bits per sample
    BITS_PER_SAMPLE_16 = 0x1 << 0
    BITS_PER_SAMPLE_24 = 0x1 << 1
    BITS_PER_SAMPLE_32 = 0x1 << 2

    # LDAC playback quality:
    # For High Quality
    LDACBT_EQMID_HQ = 1000
    # For Standard Quality
    LDACBT_EQMID_SQ = 1001
    # For Mobile Use Quality
    LDACBT_EQMID_MQ = 1002
    # LDAC playback quality constant for the codecs other than LDAC
    LDACBT_NONE = 0

    def _pair_by_config(self):
        bt_device_address = self.user_params["bt_device_address"]
        # Push the bt_config.conf file to Android device
        # if it is specified in config file
        # so it can pair and connect automatically.
        # Because of bug 34933072 AK XB10 and Sony XB2 may not be able to
        # reconnect after flashing a new build and pushing bt_config
        # so just manually pair/connect before running the tests
        bt_conf_path_dut = "/data/misc/bluedroid/bt_config.conf"
        bt_config_path = None
        try:
            bt_config_path = self.user_params["bt_config"]
        except KeyError:
            self.log.info("No bt_config is specified in config file")

        if bt_config_path != None:
            self.log.info(
                "Push bt_config file so it will connect automatically")
            if not push_file_to_device(
                    self.ad, bt_config_path, bt_conf_path_dut,
                    self.user_params[Config.key_config_path]):
                self.log.error(
                    "Unable to push file {} to DUT.".format(bt_config_path))

            self.log.info("Reboot")
            self.ad.reboot()

        if not bluetooth_enabled_check(self.ad):
            self.log.error("Failed to enable Bluetooth on DUT")
            return False

        # Verify Bluetooth device is connected
        self.log.info("Waiting up to {} seconds for device to reconnect.".
                      format(self.WAIT_TIME))
        start_time = time.time()
        result = False
        while time.time() < start_time + self.WAIT_TIME:
            connected_devices = self.ad.droid.bluetoothGetConnectedDevices()
            if len(connected_devices) > 0:
                result = True
                break

            try:
                self.ad.droid.bluetoothConnectBonded(
                    self.user_params["bt_device_address"])
            except Exception as err:
                self.log.error(
                    "Failed to connect bonded device. Err: {}".format(err))

        if result is False:
            self.log.error("No headset is connected")
            return False
        return True

    def _discover_and_pair(self):
        self.ad.droid.bluetoothStartDiscovery()
        time.sleep(5)
        self.ad.droid.bluetoothCancelDiscovery()
        for device in self.ad.droid.bluetoothGetDiscoveredDevices():
            if device['address'] == self.a2dp_speaker.mac_address:
                self.ad.droid.bluetoothDiscoverAndBond(
                    self.a2dp_speaker.mac_address)
                end_time = time.time() + 20
                self.log.info("Verifying devices are bonded")
                while (time.time() < end_time):
                    bonded_devices = self.ad.droid.bluetoothGetBondedDevices()
                    for d in bonded_devices:
                        if d['address'] == self.a2dp_speaker.mac_address:
                            self.log.info("Successfully bonded to device.")
                            self.log.info(
                                "Bonded devices:\n{}".format(bonded_devices))
                            return True
        return False

    def setup_class(self):
        self.ad = self.android_devices[0]
        self.ad.droid.bluetoothFactoryReset()
        # Factory reset requires a short delay to take effect
        time.sleep(3)

        self.ad.log.info("Making sure BT phone is enabled here during setup")
        if not bluetooth_enabled_check(self.ad):
            self.log.error("Failed to turn Bluetooth on DUT")
        # Give a breathing time of short delay to take effect
        time.sleep(3)

        reset_bluetooth([self.ad])

        # Determine if we have a relay-based device
        self.a2dp_speaker = None
        if self.relay_devices[0]:
            self.a2dp_speaker = self.relay_devices[0]
            self.a2dp_speaker.setup()
            # The device may be on, enter pairing mode.
            self.a2dp_speaker.enter_pairing_mode()
            if self._discover_and_pair() is False:
                # The device is probably off, turn it on
                self.a2dp_speaker.power_on()
                time.sleep(0.25)
                self.a2dp_speaker.enter_pairing_mode()
                if self._discover_and_pair() is False:
                    self.log.info("Unable to pair to relay based device.")
                    return False
            if len(self.ad.droid.bluetoothGetConnectedDevices()) == 0:
                self.log.info("Not connected to relay based device.")
                return False
        else:
            # No relay-based device used config
            if self._pair_by_config() is False:
                return False

        # Add music files to the Android device
        music_path_dut = "/sdcard/Music/"
        self.cd_quality_music_file = self.user_params["cd_quality_music_file"][
            0]
        self.log.info(
            "Push CD quality music file {}".format(self.cd_quality_music_file))
        if not push_file_to_device(self.ad, self.cd_quality_music_file,
                                   music_path_dut,
                                   self.user_params[Config.key_config_path]):
            self.log.error("Unable to push file {} to DUT.".format(
                self.cd_quality_music_file))

        self.hi_res_music_file = self.user_params["hi_res_music_file"][0]
        self.log.info(
            "Push Hi Res quality music file {}".format(self.hi_res_music_file))
        if not push_file_to_device(self.ad, self.hi_res_music_file,
                                   music_path_dut,
                                   self.user_params[Config.key_config_path]):
            self.log.error(
                "Unable to find file {}.".format(self.hi_res_music_file))

        # Then do other power testing setup in the base class
        # including start PMC etc
        super(A2dpPowerTest, self).setup_class()
        return True

    def teardown_class(self):
        if self.a2dp_speaker is not None:
            self.a2dp_speaker.power_off()
            self.a2dp_speaker.clean_up()

    def _main_power_test_function_for_codec(self,
                                            codec_type,
                                            sample_rate,
                                            bits_per_sample,
                                            music_file,
                                            test_case,
                                            ldac_playback_quality=LDACBT_NONE,
                                            bt_on_not_play=False,
                                            bt_off_mute=False):
        """Main util function for power test of A2dp with different codec.

        Steps:
        1. Prepare adb shell command
        2. Send the adb shell command to PMC
        3. PMC will create Media Player and set the codec info
        4. PMC start first alarm to start music
        5. After first alarm triggered it start the second alarm to stop music
        6. Save the power usage data into log file

        Args:
            codec_type: Codec Type - SBC, ACC, AptX, AptX-HD or LDAC
            sample_rate: Sample Rate - 44.1, 48.0, 88.2, 96.0 Khz
            bits_per_sample: Bits per Sample - 16, 24, 32 bits
            music_file: a music file with CD quality (44.1KHz/16bits) or
                        Hi Res quality (96KHz/24bits)
            ldac_playback_quality: LDACBT_EQMID_HQ, LDACBT_EQMID_SQ,
                                   LDACBT_EQMID_MQ
            bt_on_not_play: for baseline when MediaPlayer isn't play
                            but Bluetooth is on
            bt_off_mute: for baseline case when Bluetooth is off and
                            on board speakers muted

        Returns:
            True or False value per check_test_pass result
        """
        if bt_on_not_play == True:
            msg = "%s --es BT_ON_NotPlay %d" % (self.PMC_BASE_CMD, 1)
        else:
            # Get the base name of music file
            music_name = os.path.basename(music_file)
            self.music_url = "file:///sdcard/Music/{}".format(music_name)
            play_time = (self.MEASURE_TIME + self.DELAY_MEASURE_TIME +
                         self.DELAY_STOP_TIME)
            play_msg = "%s --es MusicURL %s --es PlayTime %d" % (
                self.PMC_BASE_CMD, self.music_url, play_time)

            if bt_off_mute == True:
                msg = "%s --es BT_OFF_Mute %d" % (play_msg, 1)
            else:
                codec1_msg = "%s --es CodecType %d --es SampleRate %d" % (
                    play_msg, codec_type, sample_rate)
                codec_msg = "%s --es BitsPerSample %d" % (codec1_msg,
                                                          bits_per_sample)
                if codec_type == self.CODEC_LDAC:
                    msg = "%s --es LdacPlaybackQuality %d" % (
                        codec_msg, ldac_playback_quality)
                else:
                    msg = codec_msg

        self.ad.log.info("Send broadcast message: %s", msg)
        self.ad.adb.shell(msg)
        # Check if PMC is ready
        status_msg = "READY"
        if bt_on_not_play == True:
            status_msg = "SUCCEED"
        if not self.check_pmc_status(self.LOG_FILE, status_msg,
                                     "PMC is not ready"):
            return

        # Start the power measurement
        result = self.mon.measure_power(
            self.POWER_SAMPLING_RATE, self.MEASURE_TIME,
            self.current_test_name, self.DELAY_MEASURE_TIME)

        # Sleep to wait for PMC to finish
        time.sleep(self.DELAY_STOP_TIME)
        # Check if PMC was successful
        self.check_pmc_status(self.LOG_FILE, "SUCCEED",
                              "PMC was not successful")

        (current_avg, stdev) = self.save_logs_for_power_test(
            result, self.MEASURE_TIME, 0)

        # perform watermark comparison numbers
        self.log.info("==> CURRENT AVG from PMC Monsoon app: %s" % current_avg)
        self.log.info(
            "==> WATERMARK from config file: %s" % self.user_params[test_case])
        return self.check_test_pass(current_avg, self.user_params[test_case])

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='6dc78cf4-7cae-4b03-8a31-0d23f41d1baa')
    def test_power_baseline_not_play_music(self):
        """Test power usage baseline without playing music.

        Test power usage baseline without playing music.

        Steps:
        The same steps described in _main_power_test_function_for_codec()
           except telling PMC not to play music

        Expected Result:
        Power consumption results

        TAGS: Bluetooth, A2DP, Power, Codec
        Priority: 3

        """
        current_test_case = func_name = sys._getframe().f_code.co_name
        return self._main_power_test_function_for_codec(
            self.CODEC_SBC,
            self.SAMPLE_RATE_44100,
            self.BITS_PER_SAMPLE_16,
            self.cd_quality_music_file,
            current_test_case,
            ldac_playback_quality=self.LDACBT_NONE,
            bt_on_not_play=True)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='d96080e3-1944-48b8-9655-4a77664a463b')
    def test_power_baseline_play_music_but_disable_bluetooth(self):
        """Test power usage baseline of playing music but Bluetooth is off.

        Test power usage baseline of playing music but Bluetooth is off
           speaker volume is set to 0.

        Steps:
        1. Disable Bluetooth
        2. The same steps described in _main_power_test_function_for_codec()
        3. Enable Bluetooth

        Expected Result:
        Power consumption results

        TAGS: Bluetooth, A2DP, Power, Codec
        Priority: 3

        """
        self.ad.log.info("Disable BT")
        if not disable_bluetooth(self.ad.droid):
            self.log.error("Failed to disable Bluetooth on DUT")
            return False

        current_test_case = func_name = sys._getframe().f_code.co_name
        return self._main_power_test_function_for_codec(
            self.CODEC_SBC,
            self.SAMPLE_RATE_44100,
            self.BITS_PER_SAMPLE_16,
            self.cd_quality_music_file,
            current_test_case,
            ldac_playback_quality=self.LDACBT_NONE,
            bt_on_not_play=False,
            bt_off_mute=True)

        self.ad.log.info("Enable BT")
        if not bluetooth_enabled_check(self.ad):
            self.log.error("Failed to turn Bluetooth on DUT")

        # Because of bug 34933072 Bluetooth device may not be able to reconnect
        #  after running this test case
        # We need manually to turn on Bluetooth devices to get it reconnect
        # for the next test case
        # The workaround is to run this test case in the end of test suite

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='f62238cf-35df-4e42-a160-16944f76fb86')
    def test_power_for_sbc_with_cd_quality_music(self):
        """Test power usage for SBC codec with CD quality music file.

        Test power usage for SBC codec with CD quality music file.
        SBC only supports 44.1 Khz and 16 bits

        Steps:
        The same steps described in _main_power_test_function_for_codec()

        Expected Result:
        Power consumption results

        TAGS: Bluetooth, A2DP, Power, Codec
        Priority: 3

        """
        current_test_case = func_name = sys._getframe().f_code.co_name
        return self._main_power_test_function_for_codec(
            self.CODEC_SBC, self.SAMPLE_RATE_44100, self.BITS_PER_SAMPLE_16,
            self.cd_quality_music_file, current_test_case)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='d9421c37-c2db-4dc5-841b-f837c0b2ea48')
    def test_power_for_sbc_with_hi_res_music(self):
        """Test power usage for SBC codec with Hi Resolution music file.

        Test power usage for SBC codec with Hi Resolution music file.
        SBC only supports 44.1 Khz and 16 bits

        Steps:
        The same steps described in _main_power_test_function_for_codec()

        Expected Result:
        Power consumption results

        TAGS: Bluetooth, A2DP, Power, Codec
        Priority: 3

        """
        current_test_case = func_name = sys._getframe().f_code.co_name
        return self._main_power_test_function_for_codec(
            self.CODEC_SBC, self.SAMPLE_RATE_44100, self.BITS_PER_SAMPLE_16,
            self.hi_res_music_file, current_test_case)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='f746c038-9a00-43d0-b6b1-b9a36fae5a5a')
    def test_power_for_aac_44100_16_with_cd_quality_music(self):
        """Test power usage for AAC codec with CD quality music file.

        Test power usage for AAC codec using 44.1KHz/16bits with
             CD quality music file.

        Steps:
        The same steps described in _main_power_test_function_for_codec()

        Expected Result:
        Power consumption results

        TAGS: Bluetooth, A2DP, Power, Codec
        Priority: 3

        """
        current_test_case = func_name = sys._getframe().f_code.co_name
        return self._main_power_test_function_for_codec(
            self.CODEC_AAC, self.SAMPLE_RATE_44100, self.BITS_PER_SAMPLE_16,
            self.cd_quality_music_file, current_test_case)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='510448e1-f4fb-4048-adad-67f8f16f96c4')
    def test_power_for_aac_44100_16_with_hi_res_music(self):
        """Test power usage for AAC codec with Hi Resolution music file.

        Test power usage for AAC codec using 44.1KHz/16bits with
             Hi Resolution music file.

        Steps:
        The same steps described in _main_power_test_function_for_codec()

        Expected Result:
        Power consumption results

        TAGS: Bluetooth, A2DP, Power, Codec
        Priority: 3

        """
        current_test_case = func_name = sys._getframe().f_code.co_name
        return self._main_power_test_function_for_codec(
            self.CODEC_AAC, self.SAMPLE_RATE_44100, self.BITS_PER_SAMPLE_16,
            self.hi_res_music_file, current_test_case)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='9df971d1-91b6-4fad-86ff-aa91d14aa895')
    def test_power_for_aac_48000_16_with_cd_quality_music(self):
        """Test power usage for AAC codec with CD quality music file.

        Test power usage for AAC codec using 48KHz/16bits with
             CD quality music file.

        Steps:
        The same steps described in _main_power_test_function_for_codec()

        Expected Result:
        Power consumption results

        TAGS: Bluetooth, A2DP, Power, Codec
        Priority: 3

        """
        current_test_case = func_name = sys._getframe().f_code.co_name
        return self._main_power_test_function_for_codec(
            self.CODEC_AAC, self.SAMPLE_RATE_48000, self.BITS_PER_SAMPLE_16,
            self.cd_quality_music_file, current_test_case)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='b020518e-027b-4716-8abb-cd6d83551869')
    def test_power_for_aac_48000_16_with_hi_res_music(self):
        """Test power usage for AAC codec with Hi Resolution music file.

        Test power usage for AAC codec using 48KHz/16bits with
             Hi Resolution music file.

        Steps:
        The same steps described in _main_power_test_function_for_codec()

        Expected Result:
        Power consumption results

        TAGS: Bluetooth, A2DP, Power, Codec
        Priority: 3

        """
        current_test_case = func_name = sys._getframe().f_code.co_name
        return self._main_power_test_function_for_codec(
            self.CODEC_AAC, self.SAMPLE_RATE_48000, self.BITS_PER_SAMPLE_16,
            self.hi_res_music_file, current_test_case)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='2006dffb-b47b-4986-b11d-de151d5f4794')
    def test_power_for_aptx_44100_16_with_cd_quality_music(self):
        """Test power usage for APTX codec with CD quality music file.

        Test power usage for APTX codec using 44.1KHz/16bits with
             CD quality music file.

        Steps:
        The same steps described in _main_power_test_function_for_codec()

        Expected Result:
        Power consumption results

        TAGS: Bluetooth, A2DP, Power, Codec
        Priority: 3

        """
        current_test_case = func_name = sys._getframe().f_code.co_name
        return self._main_power_test_function_for_codec(
            self.CODEC_APTX, self.SAMPLE_RATE_44100, self.BITS_PER_SAMPLE_16,
            self.cd_quality_music_file, current_test_case)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='8844356b-7756-4da6-89fe-96161e715cab')
    def test_power_for_aptx_44100_16_with_hi_res_music(self):
        """Test power usage for APTX codec with Hi Resolution music file.

        Test power usage for APTX codec using 44.1KHz/16bits with
             Hi Resolution music file.

        Steps:
        The same steps described in _main_power_test_function_for_codec()

        Expected Result:
        Power consumption results

        TAGS: Bluetooth, A2DP, Power, Codec
        Priority: 3

        """
        current_test_case = func_name = sys._getframe().f_code.co_name
        return self._main_power_test_function_for_codec(
            self.CODEC_APTX, self.SAMPLE_RATE_44100, self.BITS_PER_SAMPLE_16,
            self.hi_res_music_file, current_test_case)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='d037ae2e-c5e8-4f84-9908-88335803a3d9')
    def test_power_for_aptx_48000_16_with_cd_quality_music(self):
        """Test power usage for APTX codec with CD quality music file.

        Test power usage for APTX codec using 48KHz/16bits with
             CD quality music file.

        Steps:
        The same steps described in _main_power_test_function_for_codec()

        Expected Result:
        Power consumption results

        TAGS: Bluetooth, A2DP, Power, Codec
        Priority: 3

        """
        current_test_case = func_name = sys._getframe().f_code.co_name
        return self._main_power_test_function_for_codec(
            self.CODEC_APTX, self.SAMPLE_RATE_48000, self.BITS_PER_SAMPLE_16,
            self.cd_quality_music_file, current_test_case)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='4741b8cc-b038-4b38-8326-6a98de3f5ac6')
    def test_power_for_aptx_48000_16_with_hi_res_music(self):
        """Test power usage for APTX codec with Hi Resolution music file.

        Test power usage for APTX codec using 48KHz/16bits with
             Hi Resolution music file.

        Steps:
        The same steps described in _main_power_test_function_for_codec()

        Expected Result:
        Power consumption results

        TAGS: Bluetooth, A2DP, Power, Codec
        Priority: 3

        """
        current_test_case = func_name = sys._getframe().f_code.co_name
        return self._main_power_test_function_for_codec(
            self.CODEC_APTX, self.SAMPLE_RATE_48000, self.BITS_PER_SAMPLE_16,
            self.hi_res_music_file, current_test_case)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='8dff8f63-bbdb-4a2d-afca-ff1aabf7b5f2')
    def test_power_for_aptx_hd_44100_24_with_cd_quality_music(self):
        """Test power usage for APTX-HD codec with CD quality music file.

        Test power usage for APTX-HD codec using 44.1KHz/24bits with
             CD quality music file.

        Steps:
        The same steps described in _main_power_test_function_for_codec()

        Expected Result:
        Power consumption results

        TAGS: Bluetooth, A2DP, Power, Codec
        Priority: 3

        """
        current_test_case = func_name = sys._getframe().f_code.co_name
        return self._main_power_test_function_for_codec(
            self.CODEC_APTX_HD, self.SAMPLE_RATE_44100,
            self.BITS_PER_SAMPLE_24, self.cd_quality_music_file,
            current_test_case)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='ab364fdd-04dd-42b7-af0d-fe1d7d7b809b')
    def test_power_for_aptx_hd_44100_24_with_hi_res_music(self):
        """Test power usage for APTX-HD codec with Hi Resolution music file.

        Test power usage for APTX-HD codec using 44.1KHz/24bits with
             Hi Resolution music file.

        Steps:
        The same steps described in _main_power_test_function_for_codec()

        Expected Result:
        Power consumption results

        TAGS: Bluetooth, A2DP, Power, Codec
        Priority: 3

        """
        current_test_case = func_name = sys._getframe().f_code.co_name
        return self._main_power_test_function_for_codec(
            self.CODEC_APTX_HD, self.SAMPLE_RATE_44100,
            self.BITS_PER_SAMPLE_24, self.hi_res_music_file, current_test_case)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='dd838989-9440-4833-91f6-6fca6e219796')
    def test_power_for_aptx_hd_48000_24_with_cd_quality_music(self):
        """Test power usage for APTX-HD codec with CD quality music file.

        Test power usage for APTX-HD codec using 48KHz/24bits with
             CD quality music file.

        Steps:
        The same steps described in _main_power_test_function_for_codec()

        Expected Result:
        Power consumption results

        TAGS: Bluetooth, A2DP, Power, Codec
        Priority: 3

        """
        current_test_case = func_name = sys._getframe().f_code.co_name
        return self._main_power_test_function_for_codec(
            self.CODEC_APTX_HD, self.SAMPLE_RATE_48000,
            self.BITS_PER_SAMPLE_24, self.cd_quality_music_file,
            current_test_case)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='814122eb-7068-470b-b04c-d64883258b0c')
    def test_power_for_aptx_hd_48000_24_with_hi_res_music(self):
        """Test power usage for APTX-HD codec with Hi Resolution music file.

        Test power usage for APTX-HD codec using 48KHz/24bits with
             Hi Resolution music file.

        Steps:
        The same steps described in _main_power_test_function_for_codec()

        Expected Result:
        Power consumption results

        TAGS: Bluetooth, A2DP, Power, Codec
        Priority: 3

        """
        current_test_case = func_name = sys._getframe().f_code.co_name
        return self._main_power_test_function_for_codec(
            self.CODEC_APTX_HD, self.SAMPLE_RATE_48000,
            self.BITS_PER_SAMPLE_24, self.hi_res_music_file, current_test_case)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='682c055f-4883-4559-828a-67956e110475')
    def test_power_for_ldac_44100_16_with_cd_quality_music(self):
        """Test power usage for LDAC codec with CD quality music file.

        Test power usage for LDAC codec using 44.1KHz/16bits with
             CD quality music file.

        Steps:
        The same steps described in _main_power_test_function_for_codec()

        Expected Result:
        Power consumption results

        TAGS: Bluetooth, A2DP, Power, Codec
        Priority: 3

        """
        current_test_case = func_name = sys._getframe().f_code.co_name
        return self._main_power_test_function_for_codec(
            self.CODEC_LDAC,
            self.SAMPLE_RATE_44100,
            self.BITS_PER_SAMPLE_16,
            self.cd_quality_music_file,
            current_test_case,
            ldac_playback_quality=self.LDACBT_EQMID_HQ)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='21a2478c-9b66-49ae-a8ec-d2393142ed6c')
    def test_power_for_ldac_44100_16_with_hi_res_music(self):
        """Test power usage for LDAC codec with Hi Resolution music file.

        Test power usage for LDAC codec using 44.1KHz/16bits with
             Hi Resolution music file.

        Steps:
        The same steps described in _main_power_test_function_for_codec()

        Expected Result:
        Power consumption results

        TAGS: Bluetooth, A2DP, Power, Codec
        Priority: 3

        """
        current_test_case = func_name = sys._getframe().f_code.co_name
        return self._main_power_test_function_for_codec(
            self.CODEC_LDAC,
            self.SAMPLE_RATE_44100,
            self.BITS_PER_SAMPLE_16,
            self.hi_res_music_file,
            current_test_case,
            ldac_playback_quality=self.LDACBT_EQMID_HQ)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='f081cb13-ec02-4814-ba00-3ed33630e7c0')
    def test_power_for_ldac_48000_16_with_cd_quality_music(self):
        """Test power usage for LDAC codec with CD quality music file.

        Test power usage for LDAC codec using 48KHz/16bits with
             CD quality music file.

        Steps:
        The same steps described in _main_power_test_function_for_codec()

        Expected Result:
        Power consumption results

        TAGS: Bluetooth, A2DP, Power, Codec
        Priority: 3

        """
        current_test_case = func_name = sys._getframe().f_code.co_name
        return self._main_power_test_function_for_codec(
            self.CODEC_LDAC,
            self.SAMPLE_RATE_48000,
            self.BITS_PER_SAMPLE_16,
            self.cd_quality_music_file,
            current_test_case,
            ldac_playback_quality=self.LDACBT_EQMID_HQ)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='1c6b3a1b-59b8-479f-8247-e8cbbef6d82f')
    def test_power_for_ldac_48000_16_with_hi_res_music(self):
        """Test power usage for LDAC codec with Hi Resolution music file.

        Test power usage for LDAC codec using 48KHz/16bits with
             Hi Resolution music file.

        Steps:
        The same steps described in _main_power_test_function_for_codec()

        Expected Result:
        Power consumption results

        TAGS: Bluetooth, A2DP, Power, Codec
        Priority: 3

        """
        current_test_case = func_name = sys._getframe().f_code.co_name
        return self._main_power_test_function_for_codec(
            self.CODEC_LDAC,
            self.SAMPLE_RATE_48000,
            self.BITS_PER_SAMPLE_16,
            self.hi_res_music_file,
            current_test_case,
            ldac_playback_quality=self.LDACBT_EQMID_HQ)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='e6806e10-0f1e-4c44-8f70-66e0267ebf95')
    def test_power_for_ldac_88200_16_with_cd_quality_music(self):
        """Test power usage for LDAC codec with CD quality music file.

        Test power usage for LDAC codec using 88.2KHz/16bits with
             CD quality music file.

        Steps:
        The same steps described in _main_power_test_function_for_codec()

        Expected Result:
        Power consumption results

        TAGS: Bluetooth, A2DP, Power, Codec
        Priority: 3

        """
        current_test_case = func_name = sys._getframe().f_code.co_name
        return self._main_power_test_function_for_codec(
            self.CODEC_LDAC,
            self.SAMPLE_RATE_88200,
            self.BITS_PER_SAMPLE_16,
            self.cd_quality_music_file,
            current_test_case,
            ldac_playback_quality=self.LDACBT_EQMID_HQ)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='2458880d-c662-4313-9c3a-b14ad04dddfa')
    def test_power_for_ldac_88200_16_with_hi_res_music(self):
        """Test power usage for LDAC codec with Hi Resolution music file.

        Test power usage for LDAC codec using 88.2KHz/16bits with
             Hi Resolution music file.

        Steps:
        The same steps described in _main_power_test_function_for_codec()

        Expected Result:
        Power consumption results

        TAGS: Bluetooth, A2DP, Power, Codec
        Priority: 3

        """
        current_test_case = func_name = sys._getframe().f_code.co_name
        return self._main_power_test_function_for_codec(
            self.CODEC_LDAC,
            self.SAMPLE_RATE_88200,
            self.BITS_PER_SAMPLE_16,
            self.hi_res_music_file,
            current_test_case,
            ldac_playback_quality=self.LDACBT_EQMID_HQ)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='e492f173-8a5b-4a92-b3de-c792e8db32fb')
    def test_power_for_ldac_96000_16_with_cd_quality_music(self):
        """Test power usage for LDAC codec with CD quality music file.

        Test power usage for LDAC codec using 96KHz/16bits with
             CD quality music file.

        Steps:
        The same steps described in _main_power_test_function_for_codec()

        Expected Result:
        Power consumption results

        TAGS: Bluetooth, A2DP, Power, Codec
        Priority: 3

        """
        current_test_case = func_name = sys._getframe().f_code.co_name
        return self._main_power_test_function_for_codec(
            self.CODEC_LDAC,
            self.SAMPLE_RATE_96000,
            self.BITS_PER_SAMPLE_16,
            self.cd_quality_music_file,
            current_test_case,
            ldac_playback_quality=self.LDACBT_EQMID_HQ)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='65f78a14-5a1f-443e-9a23-8ae8a206bd6f')
    def test_power_for_ldac_96000_16_with_hi_res_music(self):
        """Test power usage for LDAC codec with Hi Resolution music file.

        Test power usage for LDAC codec using 96KHz/16bits with
             Hi Resolution music file.

        Steps:
        The same steps described in _main_power_test_function_for_codec()

        Expected Result:
        Power consumption results

        TAGS: Bluetooth, A2DP, Power, Codec
        Priority: 3

        """
        current_test_case = func_name = sys._getframe().f_code.co_name
        return self._main_power_test_function_for_codec(
            self.CODEC_LDAC,
            self.SAMPLE_RATE_96000,
            self.BITS_PER_SAMPLE_16,
            self.hi_res_music_file,
            current_test_case,
            ldac_playback_quality=self.LDACBT_EQMID_HQ)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='3f026ae2-e34e-4a0f-aac2-1684d22a3796')
    def test_power_for_ldac_44100_24_with_cd_quality_music(self):
        """Test power usage for LDAC codec with CD quality music file.

        Test power usage for LDAC codec using 44.1KHz/24bits with
             CD quality music file.

        Steps:
        The same steps described in _main_power_test_function_for_codec()

        Expected Result:
        Power consumption results

        TAGS: Bluetooth, A2DP, Power, Codec
        Priority: 3

        """
        current_test_case = func_name = sys._getframe().f_code.co_name
        return self._main_power_test_function_for_codec(
            self.CODEC_LDAC,
            self.SAMPLE_RATE_44100,
            self.BITS_PER_SAMPLE_24,
            self.cd_quality_music_file,
            current_test_case,
            ldac_playback_quality=self.LDACBT_EQMID_HQ)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='6d94bd0c-039e-47a7-8fc1-b87abcf9b27d')
    def test_power_for_ldac_44100_24_with_hi_res_music(self):
        """Test power usage for LDAC codec with Hi Resolution music file.

        Test power usage for LDAC codec using 44.1KHz/24bits with
             Hi Resolution music file.

        Steps:
        The same steps described in _main_power_test_function_for_codec()

        Expected Result:
        Power consumption results

        TAGS: Bluetooth, A2DP, Power, Codec
        Priority: 3

        """
        current_test_case = func_name = sys._getframe().f_code.co_name
        return self._main_power_test_function_for_codec(
            self.CODEC_LDAC,
            self.SAMPLE_RATE_44100,
            self.BITS_PER_SAMPLE_24,
            self.hi_res_music_file,
            current_test_case,
            ldac_playback_quality=self.LDACBT_EQMID_HQ)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='2d5cbad5-5293-434d-b996-850ef32792a0')
    def test_power_for_ldac_48000_24_with_cd_quality_music(self):
        """Test power usage for LDAC codec with CD quality music file.

        Test power usage for LDAC codec using 48KHz/24bits with
             CD quality music file.

        Steps:
        The same steps described in _main_power_test_function_for_codec()

        Expected Result:
        Power consumption results

        TAGS: Bluetooth, A2DP, Power, Codec
        Priority: 3

        """
        current_test_case = func_name = sys._getframe().f_code.co_name
        return self._main_power_test_function_for_codec(
            self.CODEC_LDAC,
            self.SAMPLE_RATE_48000,
            self.BITS_PER_SAMPLE_24,
            self.cd_quality_music_file,
            current_test_case,
            ldac_playback_quality=self.LDACBT_EQMID_HQ)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='fab004d1-c67e-4b9b-af33-e4469ce9f44a')
    def test_power_for_ldac_48000_24_with_hi_res_music(self):
        """Test power usage for LDAC codec with Hi Resolution music file.

        Test power usage for LDAC codec using 48KHz/24bits with
             Hi Resolution music file.

        Steps:
        The same steps described in _main_power_test_function_for_codec()

        Expected Result:
        Power consumption results

        TAGS: Bluetooth, A2DP, Power, Codec
        Priority: 3

        """
        current_test_case = func_name = sys._getframe().f_code.co_name
        return self._main_power_test_function_for_codec(
            self.CODEC_LDAC,
            self.SAMPLE_RATE_48000,
            self.BITS_PER_SAMPLE_24,
            self.hi_res_music_file,
            current_test_case,
            ldac_playback_quality=self.LDACBT_EQMID_HQ)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='9527f997-61c6-4e88-90f9-6791cbe00883')
    def test_power_for_ldac_88200_24_with_cd_quality_music(self):
        """Test power usage for LDAC codec with CD quality music file.

        Test power usage for LDAC codec using 88.2KHz/24bits with
             CD quality music file.

        Steps:
        The same steps described in _main_power_test_function_for_codec()

        Expected Result:
        Power consumption results

        TAGS: Bluetooth, A2DP, Power, Codec
        Priority: 3

        """
        current_test_case = func_name = sys._getframe().f_code.co_name
        return self._main_power_test_function_for_codec(
            self.CODEC_LDAC,
            self.SAMPLE_RATE_88200,
            self.BITS_PER_SAMPLE_24,
            self.cd_quality_music_file,
            current_test_case,
            ldac_playback_quality=self.LDACBT_EQMID_HQ)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='4a14a499-8b62-43d9-923e-f0c46e15121e')
    def test_power_for_ldac_88200_24_with_hi_res_music(self):
        """Test power usage for LDAC codec with Hi Resolution music file.

        Test power usage for LDAC codec using 88.2KHz/24bits with
             Hi Resolution music file.

        Steps:
        The same steps described in _main_power_test_function_for_codec()

        Expected Result:
        Power consumption results

        TAGS: Bluetooth, A2DP, Power, Codec
        Priority: 3

        """
        current_test_case = func_name = sys._getframe().f_code.co_name
        return self._main_power_test_function_for_codec(
            self.CODEC_LDAC,
            self.SAMPLE_RATE_88200,
            self.BITS_PER_SAMPLE_24,
            self.hi_res_music_file,
            current_test_case,
            ldac_playback_quality=self.LDACBT_EQMID_HQ)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='d6254318-7a9a-4c19-800b-03686642e846')
    def test_power_for_ldac_96000_24_with_cd_quality_music(self):
        """Test power usage for LDAC codec with CD quality music file.

        Test power usage for LDAC codec using 96KHz/24bits with
             CD quality music file.

        Steps:
        The same steps described in _main_power_test_function_for_codec()

        Expected Result:
        Power consumption results

        TAGS: Bluetooth, A2DP, Power, Codec
        Priority: 3

        """
        current_test_case = func_name = sys._getframe().f_code.co_name
        return self._main_power_test_function_for_codec(
            self.CODEC_LDAC,
            self.SAMPLE_RATE_96000,
            self.BITS_PER_SAMPLE_24,
            self.cd_quality_music_file,
            current_test_case,
            ldac_playback_quality=self.LDACBT_EQMID_HQ)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='1eb26676-19ec-43af-ab20-bfb7b055114f')
    def test_power_for_ldac_96000_24_with_hi_res_music(self):
        """Test power usage for LDAC codec with Hi Resolution music file.

        Test power usage for LDAC codec using 96KHz/24bits with
             Hi Resolution music file.

        Steps:
        The same steps described in _main_power_test_function_for_codec()

        Expected Result:
        Power consumption results

        TAGS: Bluetooth, A2DP, Power, Codec
        Priority: 3

        """
        current_test_case = func_name = sys._getframe().f_code.co_name
        return self._main_power_test_function_for_codec(
            self.CODEC_LDAC,
            self.SAMPLE_RATE_96000,
            self.BITS_PER_SAMPLE_24,
            self.hi_res_music_file,
            current_test_case,
            ldac_playback_quality=self.LDACBT_EQMID_HQ)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='efb75158-ff90-4a95-8bd0-0189d719e647')
    def test_power_for_ldac_44100_32_with_cd_quality_music(self):
        """Test power usage for LDAC codec with CD quality music file.

        Test power usage for LDAC codec using 44.1KHz/32bits with
             CD quality music file.

        Steps:
        The same steps described in _main_power_test_function_for_codec()

        Expected Result:
        Power consumption results

        TAGS: Bluetooth, A2DP, Power, Codec
        Priority: 3

        """
        current_test_case = func_name = sys._getframe().f_code.co_name
        return self._main_power_test_function_for_codec(
            self.CODEC_LDAC,
            self.SAMPLE_RATE_44100,
            self.BITS_PER_SAMPLE_32,
            self.cd_quality_music_file,
            current_test_case,
            ldac_playback_quality=self.LDACBT_EQMID_HQ)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='7d5ee1a0-b903-4cf4-8bcc-8db653f04e3b')
    def test_power_for_ldac_44100_32_with_hi_res_music(self):
        """Test power usage for LDAC codec with Hi Resolution music file.

        Test power usage for LDAC codec using 44.1KHz/32bits with
             Hi Resolution music file.

        Steps:
        The same steps described in _main_power_test_function_for_codec()

        Expected Result:
        Power consumption results

        TAGS: Bluetooth, A2DP, Power, Codec
        Priority: 3

        """
        current_test_case = func_name = sys._getframe().f_code.co_name
        return self._main_power_test_function_for_codec(
            self.CODEC_LDAC,
            self.SAMPLE_RATE_44100,
            self.BITS_PER_SAMPLE_32,
            self.hi_res_music_file,
            current_test_case,
            ldac_playback_quality=self.LDACBT_EQMID_HQ)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='2981e30a-9f5a-4d35-9387-96dd2ab3421a')
    def test_power_for_ldac_48000_32_with_cd_quality_music(self):
        """Test power usage for LDAC codec with CD quality music file.

        Test power usage for LDAC codec using 48KHz/32bits with
             CD quality music file.

        Steps:
        The same steps described in _main_power_test_function_for_codec()

        Expected Result:
        Power consumption results

        TAGS: Bluetooth, A2DP, Power, Codec
        Priority: 3

        """
        current_test_case = func_name = sys._getframe().f_code.co_name
        return self._main_power_test_function_for_codec(
            self.CODEC_LDAC,
            self.SAMPLE_RATE_48000,
            self.BITS_PER_SAMPLE_32,
            self.cd_quality_music_file,
            current_test_case,
            ldac_playback_quality=self.LDACBT_EQMID_HQ)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='297f0ab3-be6b-4367-9750-48f3ba12bb4b')
    def test_power_for_ldac_48000_32_with_hi_res_music(self):
        """Test power usage for LDAC codec with Hi Resolution music file.

        Test power usage for LDAC codec using 48KHz/32bits with
             Hi Resolution music file.

        Steps:
        The same steps described in _main_power_test_function_for_codec()

        Expected Result:
        Power consumption results

        TAGS: Bluetooth, A2DP, Power, Codec
        Priority: 3

        """
        current_test_case = func_name = sys._getframe().f_code.co_name
        return self._main_power_test_function_for_codec(
            self.CODEC_LDAC,
            self.SAMPLE_RATE_48000,
            self.BITS_PER_SAMPLE_32,
            self.hi_res_music_file,
            current_test_case,
            ldac_playback_quality=self.LDACBT_EQMID_HQ)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='71a23350-03bf-4690-a692-eb944f7d4782')
    def test_power_for_ldac_88200_32_with_cd_quality_music(self):
        """Test power usage for LDAC codec with CD quality music file.

        Test power usage for LDAC codec using 88.2KHz/32bits with
             CD quality music file.

        Steps:
        The same steps described in _main_power_test_function_for_codec()

        Expected Result:
        Power consumption results

        TAGS: Bluetooth, A2DP, Power, Codec
        Priority: 3

        """
        current_test_case = func_name = sys._getframe().f_code.co_name
        return self._main_power_test_function_for_codec(
            self.CODEC_LDAC,
            self.SAMPLE_RATE_88200,
            self.BITS_PER_SAMPLE_32,
            self.cd_quality_music_file,
            current_test_case,
            ldac_playback_quality=self.LDACBT_EQMID_HQ)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='7452e2dd-cbdd-4f50-a482-99d038ba0ee0')
    def test_power_for_ldac_88200_32_with_hi_res_music(self):
        """Test power usage for LDAC codec with Hi Resolution music file.

        Test power usage for LDAC codec using 88.2KHz/32bits with
             Hi Resolution music file.

        Steps:
        The same steps described in _main_power_test_function_for_codec()

        Expected Result:
        Power consumption results

        TAGS: Bluetooth, A2DP, Power, Codec
        Priority: 3

        """
        current_test_case = func_name = sys._getframe().f_code.co_name
        return self._main_power_test_function_for_codec(
            self.CODEC_LDAC,
            self.SAMPLE_RATE_88200,
            self.BITS_PER_SAMPLE_32,
            self.hi_res_music_file,
            current_test_case,
            ldac_playback_quality=self.LDACBT_EQMID_HQ)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='042493c1-00d9-46d8-b2e9-844c9ac849f8')
    def test_power_for_ldac_96000_32_with_cd_quality_music(self):
        """Test power usage for LDAC codec with CD quality music file.

        Test power usage for LDAC codec using 96KHz/32bits with
             CD quality music file.

        Steps:
        The same steps described in _main_power_test_function_for_codec()

        Expected Result:
        Power consumption results

        TAGS: Bluetooth, A2DP, Power, Codec
        Priority: 3

        """
        current_test_case = func_name = sys._getframe().f_code.co_name
        return self._main_power_test_function_for_codec(
            self.CODEC_LDAC,
            self.SAMPLE_RATE_96000,
            self.BITS_PER_SAMPLE_32,
            self.cd_quality_music_file,
            current_test_case,
            ldac_playback_quality=self.LDACBT_EQMID_HQ)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='7d90b5ee-c32b-4ef8-b27f-587f3550aed9')
    def test_power_for_ldac_96000_32_with_hi_res_music(self):
        """Test power usage for LDAC codec with Hi Resolution music file.

        Test power usage for LDAC codec using 96KHz/32bits with
             Hi Resolution music file.

        Steps:
        The same steps described in _main_power_test_function_for_codec()

        Expected Result:
        Power consumption results

        TAGS: Bluetooth, A2DP, Power, Codec
        Priority: 3

        """
        current_test_case = func_name = sys._getframe().f_code.co_name
        return self._main_power_test_function_for_codec(
            self.CODEC_LDAC,
            self.SAMPLE_RATE_96000,
            self.BITS_PER_SAMPLE_32,
            self.hi_res_music_file,
            current_test_case,
            ldac_playback_quality=self.LDACBT_EQMID_HQ)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='d3da605f-acd4-49a6-ae0f-d1ef216ac5b4')
    def test_power_for_ldac_96000_24_sq_with_hi_res_music(self):
        """Test power usage for LDAC codec with Standard Quality(SQ)
           for Hi Resolution music file.

        Test power usage for LDAC codec using 96KHz/24bits with
        Standard Quality(SQ) for Hi Resolution music file.

        Steps:
        The same steps described in _main_power_test_function_for_codec()

        Expected Result:
        Power consumption results

        TAGS: Bluetooth, A2DP, Power, Codec
        Priority: 3

        """
        current_test_case = func_name = sys._getframe().f_code.co_name
        return self._main_power_test_function_for_codec(
            self.CODEC_LDAC,
            self.SAMPLE_RATE_96000,
            self.BITS_PER_SAMPLE_24,
            self.hi_res_music_file,
            current_test_case,
            ldac_playback_quality=self.LDACBT_EQMID_SQ)

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='5d6494a3-ab00-48f3-9e68-50600708c176')
    def test_power_for_ldac_96000_24_mq_with_hi_res_music(self):
        """Test power usage for LDAC codec with Mobile Quality(SQ)
           for Hi Resolution music file.

        Test power usage for LDAC codec using 96KHz/24bits with
        Mobile Quality(SQ) for Hi Resolution music file.

        Steps:
        The same steps described in _main_power_test_function_for_codec()

        Expected Result:
        Power consumption results

        TAGS: Bluetooth, A2DP, Power, Codec
        Priority: 3

        """
        current_test_case = func_name = sys._getframe().f_code.co_name
        return self._main_power_test_function_for_codec(
            self.CODEC_LDAC,
            self.SAMPLE_RATE_96000,
            self.BITS_PER_SAMPLE_24,
            self.hi_res_music_file,
            current_test_case,
            ldac_playback_quality=self.LDACBT_EQMID_MQ)
