# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os

from autotest_lib.client.bin import test
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros.video import helper_logger

EXTRA_BROWSER_ARGS = ['--use-fake-ui-for-media-stream']

# Polling timeout.
TIMEOUT = 90


class video_WebRtcPeerConnectionWithCamera(test.test):
    """Tests a full WebRTC call with a real webcam."""
    version = 1

    def start_loopback(self, cr, video_codec):
        """Opens WebRTC loopback page.

        @param cr: Autotest Chrome instance.
        @param video_codec: video codec to use.
        """
        cr.browser.platform.SetHTTPServerDirectories(self.bindir)

        self.tab = cr.browser.tabs[0]
        self.tab.Navigate(cr.browser.platform.http_server.UrlOf(
                os.path.join(self.bindir, 'loopback.html')))
        self.tab.WaitForDocumentReadyStateToBeComplete()
        self.tab.EvaluateJavaScript(
                "testWebRtcLoopbackCall('%s')" % video_codec)

    def wait_test_completed(self, timeout_secs):
        """Waits until the test is done.

        @param timeout_secs Max time to wait in seconds.

        @raises TestError on timeout, or javascript eval fails.
        """
        def _test_done():
            status = self.tab.EvaluateJavaScript('getStatus()')
            logging.debug(status);
            return status != 'running'

        utils.poll_for_condition(
            _test_done, timeout=timeout_secs, sleep_interval=1,
            desc='loopback.html reports itself as finished')

    @helper_logger.video_log_wrapper
    def run_once(self, video_codec):
        """Runs the video_WebRtcPeerConnectionWithCamera test.

        @param video_codec: video codec to use.
        """
        with chrome.Chrome(extra_browser_args=EXTRA_BROWSER_ARGS +\
                           [helper_logger.chrome_vmodule_flag()],
                           init_network_controller=True) as cr:
            self.start_loopback(cr, video_codec)
            self.wait_test_completed(TIMEOUT)
            self.print_loopback_result(video_codec)

    def print_loopback_result(self, video_codec):
        """Prints loopback results (unless we failed to retrieve them).

        @param video_codec: video codec to use.
        @raises TestError if the test failed outright.
        """
        status = self.tab.EvaluateJavaScript('getStatus()')
        if status != 'ok-done':
            raise error.TestFail('Failed: %s' % status)

        results = self.tab.EvaluateJavaScript('getResults()')
        logging.info('Camera Type: %s', results['cameraType'])
        logging.info('PeerConnectionstats: %s', results['peerConnectionStats'])
        logging.info('FrameStats: %s', results['frameStats'])

        pc_stats = results.get('peerConnectionStats')
        if not pc_stats:
            raise error.TestFail('Peer Connection Stats is empty')
        self.output_perf_value(
                description='max_input_fps_%s' % video_codec,
                value=pc_stats[1], units='fps', higher_is_better=True)
        self.output_perf_value(
                description='max_sent_fps_%s' % video_codec,
                value=pc_stats[4], units='fps', higher_is_better=True)

        frame_stats = results.get('frameStats')
        if not frame_stats:
            raise error.TestFail('Frame Stats is empty')

        self.output_perf_value(
                description='black_frames_%s' % video_codec,
                value=frame_stats['numBlackFrames'],
                units='frames', higher_is_better=False)
        self.output_perf_value(
                description='frozen_frames_%s' % video_codec,
                value=frame_stats['numFrozenFrames'],
                units='frames', higher_is_better=False)
        self.output_perf_value(
                description='total_num_frames_%s' % video_codec,
                value=frame_stats['numFrames'],
                units='frames', higher_is_better=True)
