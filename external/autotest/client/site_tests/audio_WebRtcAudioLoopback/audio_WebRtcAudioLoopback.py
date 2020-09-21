# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os

from autotest_lib.client.bin import test
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros.video import helper_logger
from autotest_lib.client.cros.audio import audio_helper
from autotest_lib.client.cros.audio import cras_utils

# Suppress the media Permission Dialog.
EXTRA_BROWSER_ARGS = [
    '--use-fake-ui-for-media-stream',  # Suppress the Permission Dialog
    '--use-fake-device-for-media-stream'  # Use fake audio & video
]

AUDIO_LOOPBACK_PAGE = 'audio_loopback.html'

# The test's runtime.
TEST_RUNTIME_SECONDS = 10

# Number of peer connections to use.
NUM_PEER_CONNECTIONS = 1

# Polling timeout.
TIMEOUT = TEST_RUNTIME_SECONDS + 10


class audio_WebRtcAudioLoopback(test.test):
    """Tests a WebRTC call with a fake audio."""
    version = 1

    def start_test(self, cr, recorded_file):
        """Opens the WebRTC audio loopback page and records audio output.

        @param cr: Autotest Chrome instance.
        @param recorded_file: File to recorder the audio output to.
        """
        cr.browser.platform.SetHTTPServerDirectories(self.bindir)

        self.tab = cr.browser.tabs[0]
        self.tab.Navigate(cr.browser.platform.http_server.UrlOf(
            os.path.join(self.bindir, AUDIO_LOOPBACK_PAGE)))
        self.tab.WaitForDocumentReadyStateToBeComplete()
        self.tab.EvaluateJavaScript(
            "run(%d, %d)" % (TEST_RUNTIME_SECONDS, NUM_PEER_CONNECTIONS))
        self.wait_for_active_stream_count(1)
        cras_utils.capture(recorded_file, duration=TEST_RUNTIME_SECONDS)

    def wait_test_completed(self, timeout_secs):
        """Waits until the test is done.

        @param timeout_secs Max time to wait in seconds.

        @raises TestError on timeout, or javascript eval fails.
        """
        def _test_done():
            status = self.tab.EvaluateJavaScript('testRunner.getStatus()')
            logging.info(status)
            return status == 'ok-done'

        utils.poll_for_condition(
                _test_done, timeout=timeout_secs, sleep_interval=1,
                desc='audio.html reports itself as finished')

    @staticmethod
    def wait_for_active_stream_count(expected_count):
        """Waits for the expected number of active streams.

        @param expected_count: expected count of active streams.
        """
        utils.poll_for_condition(
            lambda: cras_utils.get_active_stream_count() == expected_count,
            exception=error.TestError(
                'Timeout waiting active stream count to become %d' %
                 expected_count))

    @helper_logger.video_log_wrapper
    def run_once(self):
        """Runs the audio_WebRtcAudioLoopback test."""
        # Record a sample of "silence" to use as a noise profile.
        noise_file = os.path.join(self.resultsdir, 'cras_noise.wav')
        cras_utils.capture(noise_file, duration=1)

        # Create a file for the audio recording.
        recorded_file = os.path.join(self.resultsdir, 'cras_recorded.wav')

        self.wait_for_active_stream_count(0)
        with chrome.Chrome(extra_browser_args=EXTRA_BROWSER_ARGS +\
                            [helper_logger.chrome_vmodule_flag()],
                           init_network_controller=True) as cr:
            self.start_test(cr, recorded_file)
            self.wait_test_completed(TIMEOUT)
            self.print_result(recorded_file, noise_file)

    def print_result(self, recorded_file, noise_file):
        """Prints results unless status is different from ok-done.

        @raises TestError if the test failed outright.
        @param recorded_file: File to recorder the audio output to.
        @param noise_file: Noise recording, used for comparison.
        """
        status = self.tab.EvaluateJavaScript('testRunner.getStatus()')
        if status != 'ok-done':
            raise error.TestFail('Failed: %s' % status)

        results = self.tab.EvaluateJavaScript('testRunner.getResults()')
        logging.info('runTimeSeconds: %.2f', results['runTimeSeconds'])

        rms_value = audio_helper.reduce_noise_and_get_rms(
                recorded_file, noise_file)[0]
        logging.info('rms_value: %f', rms_value)
        self.output_perf_value(
                description='rms_value',
                value=rms_value,
                units='', higher_is_better=True)

