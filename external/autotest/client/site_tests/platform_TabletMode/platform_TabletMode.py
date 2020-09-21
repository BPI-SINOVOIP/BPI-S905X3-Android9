# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import time

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
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


class platform_TabletMode(test.test):
    """
    Verify that tablet mode toggles appropriately.
    """
    version = 1
    _WAIT = 5
    _SHORT_WAIT = 1
    SPOOF_CMD = 'ectool motionsense spoof '
    # Disable spoof mode and return into laptop state.
    RESET_SENSOR_0 = '-- 0 0'
    RESET_SENSOR_1 = '-- 1 0'
    # Spoof sensor 1 to force laptop into landscape tablet mode.
    LANDSCAPE_SENSOR_1 = '-- 1 1 32 -16256 -224'
    # Spoof sensor 0 and sensor 1 to force laptop into portrait tablet mode.
    PORTRAIT_SENSOR_0 = '-- 0 1 -7760 -864 -14112'
    PORTRAIT_SENSOR_1 = '-- 1 1 -7936 848 14480'
    ERRORS = []

    def _revert_laptop(self):
        """Resets sensors to revert back to laptop mode."""
        utils.system(self.SPOOF_CMD + self.RESET_SENSOR_0)
        time.sleep(self._SHORT_WAIT)
        utils.system(self.SPOOF_CMD + self.RESET_SENSOR_1)
        time.sleep(self._WAIT)

    def _spoof_tablet_landscape(self):
        """Spoofs sensors to change into tablet landscape mode."""
        utils.system(self.SPOOF_CMD + self.LANDSCAPE_SENSOR_1)
        time.sleep(self._WAIT)

    def _spoof_tablet_portrait(self):
        """Spoofs sensors to change into tablet portrait mode."""
        utils.system(self.SPOOF_CMD + self.PORTRAIT_SENSOR_0)
        time.sleep(self._SHORT_WAIT)
        utils.system(self.SPOOF_CMD + self.PORTRAIT_SENSOR_1)
        time.sleep(self._WAIT)

    def _take_screenshot(self, suffix):
        """
        Captures a screenshot of the current VT screen in BMP format.

        @param suffixcurrent_vt: desired vt for screenshot.

        @returns the path of the screenshot file.

        """
        extension = 'bmp'
        return graphics_utils.take_screenshot(self.resultsdir,
                                              suffix + '_tablet_mode',
                                              extension)

    def _verify_difference(self, screenshot1, screenshot2,
                           difference_percent_threshold=5):
        """
        Make sure screenshots are sufficiently different.

        @param screenshot1: path to screenshot.
        @param screenshot2: path to screenshot.
        @param difference_percent_threshold: threshold for difference.

        @returns number of errors found (0 or 1).

        """
        filename1 = screenshot1.split('/')[-1]
        filename2 = screenshot2.split('/')[-1]
        diff = get_percent_difference(screenshot1, screenshot2)
        logging.info("Screenshot 1 and 2 diff: %s" % diff)
        if not diff >= difference_percent_threshold:
            error = ('Screenshots differ by %d %%: %s vs %s'
                     % (diff, filename1, filename2))
            self.ERRORS.append(error)

    def _verify_similarity(self, screenshot1, screenshot2,
                           similarity_percent_threshold=5):
        """
        Make sure screenshots are the same or similar.

        @param screenshot1: path to screenshot.
        @param screenshot2: path to screenshot.
        @param difference_percent_threshold: threshold for similarity.

        @returns number of errors found (0 or 1).

        """
        filename1 = screenshot1.split('/')[-1]
        filename2 = screenshot2.split('/')[-1]
        diff = get_percent_difference(screenshot1, screenshot2)
        logging.info("Screenshot 1 and 2 similarity diff: %s" % diff)
        if not diff <= similarity_percent_threshold:
            error = ('Screenshots differ by %d %%: %s vs %s'
                     % (diff, filename1, filename2))
            self.ERRORS.append(error)

    def run_once(self):
        """
        Run tablet mode test to spoof various tablet modes and ensure
        device changes accordingly.
        """

        # Ensure we start in laptop mode.
        self._revert_laptop()

        logging.info("Take screenshot for initial laptop mode.")
        laptop_start = self._take_screenshot('laptop_start')

        logging.info("Entering landscape mode.")
        self._spoof_tablet_landscape()
        landscape = self._take_screenshot('landscape')

        self._revert_laptop()

        logging.info("Entering portrait mode.")
        self._spoof_tablet_portrait()
        portrait = self._take_screenshot('portrait')

        self._revert_laptop()
        laptop_end = self._take_screenshot('laptop_end')

        # Compare screenshots and determine the number of errors.
        self._verify_similarity(laptop_start, laptop_end)
        self._verify_difference(laptop_start, landscape)
        self._verify_difference(landscape, portrait)
        self._verify_difference(portrait, laptop_end)

        if self.ERRORS:
            raise error.TestFail('; '.join(set(self.ERRORS)))

    def cleanup(self):
        self._revert_laptop()
