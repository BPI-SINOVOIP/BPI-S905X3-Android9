# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import re

from autotest_lib.client.bin import test
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros.video import helper_logger

EXTRA_BROWSER_ARGS = ['--use-fake-ui-for-media-stream']

# Variables from the getusermedia.html page.
IS_TEST_DONE = 'isTestDone'

# Polling timeout.
SEVERAL_MINUTES_IN_SECS = 240


class video_WebRtcCamera(test.test):
    """Local getUserMedia test with webcam at VGA (and 720p, if supported)."""
    version = 1

    def start_getusermedia(self, cr):
        """Opens the test page.

        @param cr: Autotest Chrome instance.
        """
        cr.browser.platform.SetHTTPServerDirectories(self.bindir)

        self.tab = cr.browser.tabs[0]
        self.tab.Navigate(cr.browser.platform.http_server.UrlOf(
                os.path.join(self.bindir, 'getusermedia.html')))
        self.tab.WaitForDocumentReadyStateToBeComplete()


    def webcam_supports_720p(self):
        """Checks if 720p capture supported.

        @returns: True if 720p supported, false if VGA is supported.
        @raises: TestError if neither 720p nor VGA are supported.
        """
        cmd = 'lsusb -v'
        # Get usb devices and make output a string with no newline marker.
        usb_devices = utils.system_output(cmd, ignore_status=True).splitlines()
        usb_devices = ''.join(usb_devices)

        # Check if 720p resolution supported.
        if re.search(r'\s+wWidth\s+1280\s+wHeight\s+720', usb_devices):
            return True
        # The device should support at least VGA.
        # Otherwise the cam must be broken.
        if re.search(r'\s+wWidth\s+640\s+wHeight\s+480', usb_devices):
            return False
        # This should not happen.
        raise error.TestFail(
                'Could not find any cameras reporting '
                'either VGA or 720p in lsusb output: %s' % usb_devices)


    def wait_test_completed(self, timeout_secs):
        """Waits until the test is done.

        @param timeout_secs Max time to wait in seconds.

        @raises TestError on timeout, or javascript eval fails.
        """
        def _test_done():
            is_test_done = self.tab.EvaluateJavaScript(IS_TEST_DONE)
            return is_test_done == 1

        utils.poll_for_condition(
            _test_done, timeout=timeout_secs, sleep_interval=1,
            desc=('getusermedia.html:reportTestDone did not run. Test did not '
                  'complete successfully.'))

    @helper_logger.video_log_wrapper
    def run_once(self):
        """Runs the test."""
        self.board = utils.get_current_board()
        with chrome.Chrome(extra_browser_args=EXTRA_BROWSER_ARGS +\
                           [helper_logger.chrome_vmodule_flag()],
                           init_network_controller=True) as cr:
            self.start_getusermedia(cr)
            self.print_perf_results()


    def print_perf_results(self):
        """Prints the perf results unless there was an error.

        @returns the empty string if perf results were printed, otherwise
                 a description of the error that occured.
        """
        self.wait_test_completed(SEVERAL_MINUTES_IN_SECS)
        try:
            results = self.tab.EvaluateJavaScript('getResults()')
        except Exception as e:
            raise error.TestFail('Failed to get getusermedia.html results', e)
        logging.info('Results: %s', results)

        errors = []
        for width_height, data in results.iteritems():
            resolution = re.sub(',', 'x', width_height)
            if data['cameraErrors']:
                if (resolution == '1280x720' and
                        not self.webcam_supports_720p()):
                    logging.info('Accepting 720p failure since device webcam '
                                 'does not support 720p')
                    continue

                # Else we had a VGA failure or a legit 720p failure.
                errors.append('Camera error: %s for resolution '
                            '%s.' % (data['cameraErrors'], resolution))
                continue
            if not data['frameStats']:
                errors.append('Frame Stats is empty '
                              'for resolution: %s' % resolution)
                continue
            self.output_perf_value(
                    description='black_frames_%s' % resolution,
                    value=data['frameStats']['numBlackFrames'],
                    units='frames', higher_is_better=False)
            self.output_perf_value(
                    description='frozen_frames_%s' % resolution,
                    value=data['frameStats']['numFrozenFrames'],
                    units='frames', higher_is_better=False)
            self.output_perf_value(
                    description='total_num_frames_%s' % resolution,
                    value=data['frameStats']['numFrames'],
                    units='frames', higher_is_better=True)
        if errors:
            raise error.TestFail('Found errors: %s' % ', '.join(errors))
