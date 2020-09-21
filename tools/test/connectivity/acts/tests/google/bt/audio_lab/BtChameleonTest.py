# /usr/bin/env python3.4
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
Test script to automate the Bluetooth audio testing and analysis.

Quick way to generate necessary audio files:
sudo apt-get install sox
sox -b 16 -r 48000 -c 2 -n audio_file_2k1k_10_sec.wav synth 10 sine 2000 sine 3000
sox -b 16 -r 48000 -c 2 -n audio_file_2k1k_300_sec.wav synth 300 sine 2000 sine 3000

"""
import os
import subprocess
import time

from acts.test_decorators import test_tracker_info
from acts.test_utils.audio_analysis_lib.check_quality import quality_analysis
from acts.test_utils.bt.BtFunhausBaseTest import BtFunhausBaseTest
from acts.test_utils.bt.bt_constants import audio_bits_per_sample_32
from acts.test_utils.bt.bt_constants import audio_channel_mode_8
from acts.test_utils.bt.bt_constants import audio_sample_rate_48000
from acts.test_utils.bt.bt_constants import delay_after_binding_seconds
from acts.test_utils.bt.bt_constants import delay_before_record_seconds
from acts.test_utils.bt.bt_constants import fpga_linein_bus_endpoint
from acts.test_utils.bt.bt_constants import headphone_bus_endpoint
from acts.test_utils.bt.bt_constants import silence_wait_seconds

from acts import utils


class BtChameleonTest(BtFunhausBaseTest):

    audio_file_2k1k_10_sec = "audio_file_2k1k_10_sec.wav"
    audio_file_2k1k_300_sec = "audio_file_2k1k_300_sec.wav"
    android_sdcard_music_path = "/sdcard/Music"

    def __init__(self, controllers):
        BtFunhausBaseTest.__init__(self, controllers)
        self.chameleon = self.chameleon_devices[0]
        self.dut = self.android_devices[0]
        self.raw_audio_dest = "{}/{}".format(self.android_devices[0].log_path,
                                             "Chameleon_audio")
        utils.create_dir(self.raw_audio_dest)
        self.chameleon.audio_board_connect(1, headphone_bus_endpoint)
        self.chameleon.audio_board_connect(1, fpga_linein_bus_endpoint)
        time.sleep(delay_after_binding_seconds)

    def _orchestrate_audio_quality_test(self, output_file_prefix_name,
                                        bits_per_sample, rate, record_seconds,
                                        channel, audio_to_play):
        audio_analysis_filename = "{}_audio_analysis.txt".format(
            output_file_prefix_name)
        bluetooth_bind_time_seconds = 5
        port_id = 6
        has_file = True
        # Additional sleep to allow full connection of Bluetooth device
        # from test setup.
        time.sleep(bluetooth_bind_time_seconds)
        self.chameleon.start_capturing_audio(port_id, has_file)
        time.sleep(delay_before_record_seconds)
        self.dut.droid.mediaPlayOpen("file://{}".format(audio_to_play))
        time.sleep(record_seconds + silence_wait_seconds)
        raw_audio_info = self.chameleon_devices[0].stop_capturing_audio(
            port_id)
        self.ad.droid.mediaPlayStopAll()
        raw_audio_path = raw_audio_info[0]
        dest_file_path = "{}/{}_recording.raw".format(self.raw_audio_dest,
                                                      output_file_prefix_name)
        self.chameleon.scp(raw_audio_path, dest_file_path)
        self._collect_bluetooth_manager_dumpsys_logs(self.android_devices)
        self.collect_bluetooth_manager_metrics_logs(self.android_devices)
        analysis_path = "{}/{}".format(self.raw_audio_dest,
                                       audio_analysis_filename)
        try:
            quality_analysis(
                filename=dest_file_path,
                output_file=analysis_path,
                bit_width=bits_per_sample,
                rate=rate,
                channel=channel,
                spectral_only=False)
        except Exception as err:
            self.log.exception("Failed to analyze raw audio: {}".format(err))
            return False
        # TODO: Log results to proto
        return True

    @test_tracker_info(uuid='b808fed6-5cb0-4e40-9522-c0f410cd77e8')
    def test_run_bt_audio_quality_2k1k_10_sec_sine_wave(self):
        """Measure audio quality over Bluetooth by playing a 1k2k sine wave.

        Play a sine wave and measure the analysis of 1kHz and 2kHz on two
            different channels for 10 seconds:
        1. Delays during playback.
        2. Noise before playback.
        3. Noise after playback.
        4. Bursts during playback.
        5. Volume changes.

        Steps:
        1. Connect Chameleon headphone audio bus endpoint.
        2. Connect FPGA line-in bus endpoint.
        3. Clear audio routes on the Chameleon device.
        4. Start capturing audio on the Chameleon device.
        5. Start playing the sine wave on the Android device.
        6. Record for record_seconds + silence_wait_seconds.
        7. Stop recording audio on the Chameleon device.
        8. Stop playing audio on the Android Device.
        9. Pull raw recorded audio from the Chameleon device.
        10. Analyze raw audio and log results.


        Expected Result:
        Audio is recorded and processed successfully.

        Returns:
          True if Pass
          False if Fail

        TAGS: Classic, A2DP, Chameleon
        Priority: 2
        """
        sox_call = "{}{}".format("sox -b 16 -r 48000 -c 2 -n {}".format(
            self.audio_file_2k1k_10_sec), " synth 10 sine 2000 sine 3000")
        subprocess.call(sox_call, shell=True)
        sox_audio_path = "{}/{}".format(
            os.path.dirname(os.path.realpath(self.audio_file_2k1k_10_sec)),
            self.audio_file_2k1k_10_sec)
        sox_audio_path = os.path.join(
            os.path.dirname(os.path.realpath(self.audio_file_2k1k_10_sec)),
            self.audio_file_2k1k_10_sec)
        self.dut.adb.push("{} {}".format(sox_audio_path,
                                         self.android_sdcard_music_path))
        output_file_prefix_name = "{}_{}".format("test_2k1k_10_sec",
                                                 time.time())
        bits_per_sample = audio_bits_per_sample_32
        rate = audio_sample_rate_48000
        record_seconds = 10  # The length in seconds for how long to record
        channel = audio_channel_mode_8
        audio_to_play = "{}/{}".format(self.android_sdcard_music_path,
                                       self.audio_file_2k1k_10_sec)
        audio_to_play = os.path.join(self.android_sdcard_music_path,
                                     self.audio_file_2k1k_10_sec)
        return self._orchestrate_audio_quality_test(
            output_file_prefix_name=output_file_prefix_name,
            bits_per_sample=bits_per_sample,
            rate=rate,
            record_seconds=record_seconds,
            channel=channel,
            audio_to_play=audio_to_play)

    @test_tracker_info(uuid='7e971cef-6637-4198-929a-7ecc712121d7')
    def test_run_bt_audio_quality_2k1k_300_sec_sine_wave(self):
        """Measure audio quality over Bluetooth by playing a 1k2k sine wave.

        Play a sine wave and measure the analysis of 1kHz and 2kHz on two
            different channels for 300 seconds:
        1. Delays during playback.
        2. Noise before playback.
        3. Noise after playback.
        4. Bursts during playback.
        5. Volume changes.

        Steps:
        1. Connect Chameleon headphone audio bus endpoint.
        2. Connect FPGA line-in bus endpoint.
        3. Clear audio routes on the Chameleon device.
        4. Start capturing audio on the Chameleon device.
        5. Start playing the sine wave on the Android device.
        6. Record for record_seconds + silence_wait_seconds.
        7. Stop recording audio on the Chameleon device.
        8. Stop playing audio on the Android Device.
        9. Pull raw recorded audio from the Chameleon device.
        10. Analyze raw audio and log results.


        Expected Result:
        Audio is recorded and processed successfully.

        Returns:
          True if Pass
          False if Fail

        TAGS: Classic, A2DP, Chameleon
        Priority: 2
        """
        sox_call = "{}{}".format("sox -b 16 -r 48000 -c 2 -n {}".format(
            self.audio_file_2k1k_300_sec), " synth 300 sine 2000 sine 3000")
        subprocess.call(sox_call, shell=True)
        sox_audio_path = os.path.join(
            os.path.dirname(os.path.realpath(self.audio_file_2k1k_300_sec)),
            self.audio_file_2k1k_300_sec)
        self.dut.adb.push("{} {}".format(sox_audio_path,
                                         self.android_sdcard_music_path))
        output_file_prefix_name = "{}_{}".format("test_2k1k_300_sec.wav",
                                                 time.time())
        bits_per_sample = audio_bits_per_sample_32
        rate = audio_sample_rate_48000
        record_seconds = 300  # The length in seconds for how long to record
        channel = audio_channel_mode_8
        audio_to_play = os.path.join(self.android_sdcard_music_path,
                                     self.audio_file_2k1k_300_sec)

        return self._orchestrate_audio_quality_test(
            output_file_prefix_name=output_file_prefix_name,
            bits_per_sample=bits_per_sample,
            rate=rate,
            record_seconds=record_seconds,
            channel=channel,
            audio_to_play=audio_to_play)
