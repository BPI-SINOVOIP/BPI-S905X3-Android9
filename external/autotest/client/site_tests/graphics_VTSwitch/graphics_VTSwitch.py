# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import time

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros.graphics import graphics_utils
from autotest_lib.client.cros.image_comparison import pdiff_image_comparer

def get_percent_difference(file1, file2):
    """
    Performs pixel comparison of two files, given by their paths |file1|
    and |file2| using terminal tool 'perceptualdiff' and returns percentage
    difference of the total file size.

    @param file1: path to image
    @param file2: path to secondary image
    @return: percentage difference of total file size.
    @raise ValueError: if image dimensions are not the same
    @raise OSError: if file does not exist or cannot be opened.

    """
    # Using pdiff image comparer to compare the two images. This class
    # invokes the terminal tool perceptualdiff.
    pdi = pdiff_image_comparer.PdiffImageComparer()
    diff_bytes = pdi.compare(file1, file2)[0]
    return round(100. * diff_bytes / os.path.getsize(file1))


class graphics_VTSwitch(graphics_utils.GraphicsTest):
    """
    Verify that VT switching works.
    """
    version = 2
    _WAIT = 5
    # TODO(crosbug.com/36417): Need to handle more than one display screen.

    @graphics_utils.GraphicsTest.failure_report_decorator('graphics_VTSwitch')
    def run_once(self,
                 num_iterations=2,
                 similarity_percent_threshold=95,
                 difference_percent_threshold=5):

        # Check for chromebook type devices
        if not utils.get_board_type() == 'CHROMEBOOK':
            raise error.TestNAError('DUT is not Chromebook. Test Skipped.')

        self._num_errors = 0
        keyvals = {}

        # Make sure we start in VT1.
        self.open_vt1()

        with chrome.Chrome():

            # wait for Chrome to start before taking screenshot
            time.sleep(10)

            # Take screenshot of browser.
            vt1_screenshot = self._take_current_vt_screenshot(1)

            keyvals['num_iterations'] = num_iterations

            # Go to VT2 and take a screenshot.
            self.open_vt2()
            vt2_screenshot = self._take_current_vt_screenshot(2)

            # Make sure VT1 and VT2 are sufficiently different.
            diff = get_percent_difference(vt1_screenshot, vt2_screenshot)
            keyvals['percent_initial_VT1_VT2_difference'] = diff
            if not diff >= difference_percent_threshold:
                self._num_errors += 1
                logging.error('VT1 and VT2 screenshots only differ by ' + \
                              '%d %%: %s vs %s' %
                              (diff, vt1_screenshot, vt2_screenshot))

            num_identical_vt1_screenshots = 0
            num_identical_vt2_screenshots = 0
            max_vt1_difference_percent = 0
            max_vt2_difference_percent = 0

            # Repeatedly switch between VT1 and VT2.
            for iteration in xrange(num_iterations):
                logging.info('Iteration #%d', iteration)

                # Go to VT1 and take a screenshot.
                self.open_vt1()
                current_vt1_screenshot = self._take_current_vt_screenshot(1)

                # Make sure the current VT1 screenshot is the same as (or similar
                # to) the original login screen screenshot.
                diff = get_percent_difference(vt1_screenshot,
                                              current_vt1_screenshot)
                if not diff < similarity_percent_threshold:
                    max_vt1_difference_percent = \
                        max(diff, max_vt1_difference_percent)
                    self._num_errors += 1
                    logging.error('VT1 screenshots differ by %d %%: %s vs %s',
                                  diff, vt1_screenshot,
                                  current_vt1_screenshot)
                else:
                    num_identical_vt1_screenshots += 1

                # Go to VT2 and take a screenshot.
                self.open_vt2()
                current_vt2_screenshot = self._take_current_vt_screenshot(2)

                # Make sure the current VT2 screenshot is the same as (or
                # similar to) the first VT2 screenshot.
                diff = get_percent_difference(vt2_screenshot,
                                              current_vt2_screenshot)
                if not diff <= similarity_percent_threshold:
                    max_vt2_difference_percent = \
                        max(diff, max_vt2_difference_percent)
                    self._num_errors += 1
                    logging.error(
                        'VT2 screenshots differ by %d %%: %s vs %s',
                        diff, vt2_screenshot, current_vt2_screenshot)
                else:
                    num_identical_vt2_screenshots += 1

        self.open_vt1()

        keyvals['percent_VT1_screenshot_max_difference'] = \
            max_vt1_difference_percent
        keyvals['percent_VT2_screenshot_max_difference'] = \
            max_vt2_difference_percent
        keyvals['num_identical_vt1_screenshots'] = num_identical_vt1_screenshots
        keyvals['num_identical_vt2_screenshots'] = num_identical_vt2_screenshots

        self.write_perf_keyval(keyvals)

        if self._num_errors > 0:
            raise error.TestFail('Failed: saw %d VT switching errors.' %
                                  self._num_errors)

    def _take_current_vt_screenshot(self, current_vt):
        """
        Captures a screenshot of the current VT screen in PNG format.

        @param current_vt: desired vt for screenshot.

        @returns the path of the screenshot file.

        """
        extension = 'png'

        return graphics_utils.take_screenshot(self.resultsdir,
                                              'graphics_VTSwitch_VT%d' % current_vt,
                                              extension)

    def cleanup(self):
        # Return to VT1 when done.  Ideally, the screen should already be in VT1
        # but the test might fail and terminate while in VT2.
        self.open_vt1()
        super(graphics_VTSwitch, self).cleanup()
