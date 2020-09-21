# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import logging
import time

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros.a11y import a11y_test_base
from autotest_lib.client.cros.audio import cras_utils
from autotest_lib.client.cros.audio import sox_utils


class accessibility_ChromeVoxSound(a11y_test_base.a11y_test_base):
    """Check whether ChromeVox makes noise on real hardware."""
    version = 1

    _audio_chunk_size = 1 # Length of chunk size in seconds.
    _detect_time = 40 # Max length of time to spend detecting audio in seconds.


    def _detect_audio(self, name, min_time):
        """Detects whether audio was heard and returns the approximate time.

        Runs for at most self._detect_time, checking each chunk for sound.
        After first detecting a chunk that has audio, counts the subsequent
        chunks that also do.

        Finally, check whether the found audio matches the expected length.

        @param name: a string representing which sound is expected.
        @param min_time: the minimum allowed sound length in seconds.

        @raises: error.TestFail if the observed behavior doesn't match
                 expected: either no sound or sound of bad length.

        """
        count = 0
        counting = False
        saw_sound_end = False

        for i in xrange(self._detect_time / self._audio_chunk_size):
            rms = self._rms_of_next_audio_chunk()
            if rms > 0:
                logging.info('Found passing chunk: %d.', i)
                if not counting:
                    start_time = time.time()
                    counting = True
                count += 1
            elif counting:
                audio_length = time.time() - start_time
                saw_sound_end = True
                break
        if not counting:
            raise error.TestFail('No audio for %s was found!' % name)
        if not saw_sound_end:
            raise error.TestFail('Audio for %s was more than % seconds!' % (
                    name, self._detect_time))

        logging.info('Time taken - %s: %f', name, audio_length)
        if audio_length < min_time:
            raise error.TestFail(
                    '%s audio was only %f seconds long!' % (name, audio_length))
        return


    def _rms_of_next_audio_chunk(self):
        """Finds the sox_stats values of the next chunk of audio."""
        cras_utils.loopback(self._loopback_file, channels=1,
                            duration=self._audio_chunk_size)
        stat_output = sox_utils.get_stat(self._loopback_file)
        logging.info(stat_output)
        return vars(stat_output)['rms']


    def _check_chromevox_sound(self, cr):
        """Test contents.

        Enable ChromeVox, navigate to a new page, and open a new tab.  Check
        the audio output at each point.

        @param cr: the chrome.Chrome() object

        """
        chromevox_start_time = time.time()
        self._toggle_chromevox()
        self._confirm_chromevox_state(True)

        # TODO: this sound doesn't play for Telemetry user.  crbug.com/590403
        # Welcome ding
        # self._detect_audio('enable ChromeVox ding', 1)

        # "ChromeVox Spoken Feedback is ready!"
        self._detect_audio('welcome message', 2)
        chromevox_open_time = time.time() - chromevox_start_time
        logging.info('ChromeVox took %f seconds to start.')

        # New tab sound.
        tab = cr.browser.tabs.New()
        self._detect_audio('new tab ding', 2)

        # Page navigation sound.
        tab.Navigate('chrome://version')
        self._detect_audio('page navigation sound', 2)


    def run_once(self):
        """Entry point of this test."""
        self._loopback_file = os.path.join(self.bindir, 'cras_loopback.wav')
        extension_path = self._get_extension_path()

        with chrome.Chrome(extension_paths=[extension_path]) as cr:
            self._extension = cr.get_extension(extension_path)
            cr.browser.tabs[0].WaitForDocumentReadyStateToBeComplete()
            self._confirm_chromevox_state(False)
            self._check_chromevox_sound(cr)


    def _child_test_cleanup(self):
        try:
            os.remove(self._loopback_file)
        except OSError:
            pass
