# Copyright (c) 2011 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, os, time

from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.audio import audio_helper
from autotest_lib.client.cros.audio import audio_test_data
from autotest_lib.client.cros.audio import alsa_utils
from autotest_lib.client.cros.audio import cmd_utils
from autotest_lib.client.cros.audio import cras_utils


TEST_DURATION = 1

class audio_AlsaLoopback(audio_helper.alsa_rms_test):
    """Verifies audio playback and capture function."""
    version = 1

    def run_once(self):
        """Entry point of this test."""

        # Sine wav file lasts 5 seconds
        wav_path = os.path.join(self.bindir, '5SEC.wav')
        data_format = dict(file_type='wav', sample_format='S16_LE',
                channel=2, rate=48000)
        wav_file = audio_test_data.GenerateAudioTestData(
            path=wav_path,
            data_format=data_format,
            duration_secs=5,
            frequencies=[440, 440],
            volume_scale=0.9)

        recorded_file = os.path.join(self.resultsdir, 'hw_recorded.wav')

        # Get selected input and output devices.
        cras_input = cras_utils.get_selected_input_device_name()
        cras_output = cras_utils.get_selected_output_device_name()
        logging.debug("Selected input=%s, output=%s", cras_input, cras_output)
        if cras_input is None:
            raise error.TestFail("Fail to get selected input device.")
        if cras_output is None:
            raise error.TestFail("Fail to get selected output device.")
        alsa_input = alsa_utils.convert_device_name(cras_input)
        alsa_output = alsa_utils.convert_device_name(cras_output)

        (output_type, input_type) = cras_utils.get_selected_node_types()
        if 'MIC' not in input_type:
            raise error.TestFail("Wrong input type=%s", input_type)
        if 'HEADPHONE' not in output_type:
            raise error.TestFail("Wrong output type=%s", output_type)

        p = cmd_utils.popen(alsa_utils.playback_cmd(wav_file.path, device=alsa_output))
        try:
            # Wait one second to make sure the playback has been started.
            time.sleep(1)
            alsa_utils.record(recorded_file, duration=TEST_DURATION,
                              device=alsa_input)

            # Make sure the audio is still playing.
            if p.poll() != None:
                raise error.TestError('playback stopped')
        finally:
            cmd_utils.kill_or_log_returncode(p)
            wav_file.delete()

        rms_value = audio_helper.get_rms(recorded_file)[0]

        self.write_perf_keyval({'rms_value': rms_value})

