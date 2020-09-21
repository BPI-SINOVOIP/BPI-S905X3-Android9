# Copyright (c) 2010 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import tempfile

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.audio import audio_spec
from autotest_lib.client.cros.audio import alsa_utils
from autotest_lib.client.cros.audio import cras_utils

DURATION = 3
TOLERANT_RATIO = 0.1

class audio_Microphone(test.test):
    version = 1


    def check_recorded_filesize(
            self, filesize, duration, channels, rate, bits=16):
        expected = duration * channels * (bits / 8) * rate
        if abs(float(filesize) / expected - 1) > TOLERANT_RATIO:
            raise error.TestFail('File size not correct: %d' % filesize)


    def verify_alsa_capture(self, channels, rate, device, bits=16):
        recorded_file = tempfile.NamedTemporaryFile()
        alsa_utils.record(
                recorded_file.name, duration=DURATION, channels=channels,
                bits=bits, rate=rate, device=device)
        self.check_recorded_filesize(
                os.path.getsize(recorded_file.name),
                DURATION, channels, rate, bits)


    def verify_cras_capture(self, channels, rate):
        recorded_file = tempfile.NamedTemporaryFile()
        cras_utils.capture(
                recorded_file.name, duration=DURATION, channels=channels,
                rate=rate)
        self.check_recorded_filesize(
                os.path.getsize(recorded_file.name),
                DURATION, channels, rate)


    def run_once(self):
        cras_device_name = cras_utils.get_selected_input_device_name()
        logging.debug("Selected input device name=%s", cras_device_name)

        if cras_device_name is None:
            board_type = utils.get_board_type()
            if not audio_spec.has_internal_microphone(board_type):
                logging.debug("No internal mic. Skipping the test.")
                return
            raise error.TestFail("Fail to get selected input device.")

        # Mono and stereo capturing should work fine @ 44.1KHz and 48KHz.

        # Verify recording using ALSA utils.
        alsa_device_name = alsa_utils.convert_device_name(cras_device_name)
        channels = alsa_utils.get_record_device_supported_channels(
                alsa_device_name)
        if channels is None:
            raise error.TestFail("Fail to get supported channels for %s",
                                alsa_device_name)

        for c in channels:
            self.verify_alsa_capture(c, 44100, alsa_device_name)
            self.verify_alsa_capture(c, 48000, alsa_device_name)

        # Verify recording of CRAS.
        self.verify_cras_capture(1, 44100)
        self.verify_cras_capture(1, 48000)
        self.verify_cras_capture(2, 48000)
        self.verify_cras_capture(2, 44100)
