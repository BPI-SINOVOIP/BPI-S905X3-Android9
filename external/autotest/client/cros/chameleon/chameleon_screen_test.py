# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error as error_lib
from autotest_lib.client.cros.chameleon import screen_utility_factory


class ChameleonScreenTest(object):
    """Utility to test the screen between Chameleon and CrOS.

    This class contains the screen-related testing operations.

    """
    # Time in seconds to wait for notation bubbles, including bubbles for
    # external detection, mirror mode and fullscreen, to disappear.
    _TEST_IMAGE_STABILIZE_TIME = 10

    def __init__(self, host, chameleon_port, display_facade, output_dir):
        """Initializes the ScreenUtilityFactory objects."""
        self._host = host
        self._display_facade = display_facade
        factory = screen_utility_factory.ScreenUtilityFactory(
                chameleon_port, display_facade)
        self._resolution_comparer = factory.create_resolution_comparer()
        self._screen_comparer = factory.create_screen_comparer(output_dir)
        self._mirror_comparer = factory.create_mirror_comparer(output_dir)
        self._calib_comparer = factory.create_calibration_comparer(output_dir)
        self._calibration_image_tab_descriptor = None


    def test_resolution(self, expected_resolution):
        """Tests if the resolution of Chameleon matches with the one of CrOS.

        @param expected_resolution: A tuple (width, height) for the expected
                                    resolution.
        @return: None if the check passes; otherwise, a string of error message.
        """
        return self._resolution_comparer.compare(expected_resolution)


    def test_screen(self, expected_resolution, test_mirrored=None,
                    error_list=None):
        """Tests if the screen of Chameleon matches with the one of CrOS.

        @param expected_resolution: A tuple (width, height) for the expected
                                    resolution.
        @param test_mirrored: True to test mirrored mode. False not to. None
                              to test mirrored mode iff the current mode is
                              mirrored.
        @param error_list: A list to append the error message to or None.
        @return: None if the check passes; otherwise, a string of error message.
        """
        if test_mirrored is None:
            test_mirrored = self._display_facade.is_mirrored_enabled()

        error = self._resolution_comparer.compare(expected_resolution)
        if not error:
            # Do two screen comparisons with and without hiding cursor, to
            # work-around some devices still showing cursor on CrOS FB.
            # TODO: Remove this work-around once crosbug/p/34524 got fixed.
            error = self._screen_comparer.compare()
            if error:
                logging.info('Hide cursor and do screen comparison again...')
                self._display_facade.hide_cursor()
                error = self._screen_comparer.compare()

            if error:
                # On some ARM device, the internal FB has some issue and it
                # won't get fixed in the near future. As this issue don't
                # affect users, just the tests. So compare it with the
                # calibration image directly as an workaround.
                board = self._host.get_board().replace('board:', '')
                if board in ['kevin']:
                    # TODO(waihong): In mirrored mode, using calibration image
                    # to compare is not feasible due to the size difference.
                    # Skip the error first and think a better way to compare.
                    if test_mirrored:
                        raise error_lib.TestNAError('Test this item manually! '
                                'Missing CrOS feature, not able to automate.')
                    logging.info('Compare the calibration image directly...')
                    error = self._calib_comparer.compare()

        if not error and test_mirrored:
            error = self._mirror_comparer.compare()
        if error and error_list is not None:
            error_list.append(error)
        return error


    def load_test_image(self, image_size, test_mirrored=None):
        """Loads calibration image on the CrOS with logging

        @param image_size: A tuple (width, height) conforms the resolution.
        @param test_mirrored: True to test mirrored mode. False not to. None
                              to test mirrored mode iff the current mode is
                              mirrored.
        """
        if test_mirrored is None:
            test_mirrored = self._display_facade.is_mirrored_enabled()
        self._calibration_image_tab_descriptor = \
            self._display_facade.load_calibration_image(image_size)
        if not test_mirrored:
            self._display_facade.move_to_display(
                    self._display_facade.get_first_external_display_id())
        self._display_facade.set_fullscreen(True)
        logging.info('Waiting for calibration image to stabilize...')
        time.sleep(self._TEST_IMAGE_STABILIZE_TIME)


    def unload_test_image(self):
        """Closes the tab in browser to unload the fullscreen test image."""
        self._display_facade.close_tab(self._calibration_image_tab_descriptor)


    def test_screen_with_image(self, expected_resolution, test_mirrored=None,
                               error_list=None, chameleon_supported=True,
                               retry_count=2):
        """Tests the screen with image loaded.

        @param expected_resolution: A tuple (width, height) for the expected
                                    resolution.
        @param test_mirrored: True to test mirrored mode. False not to. None
                              to test mirrored mode iff the current mode is
                              mirrored.
        @param error_list: A list to append the error message to or None.
        @param retry_count: A count to retry the screen test.
        @param chameleon_supported: Whether resolution is supported by
                                    chameleon. The DP RX doesn't support
                                    4K resolution. The max supported resolution
                                    is 2560x1600. See crbug/585900.
        @return: None if the check passes; otherwise, a string of error message.
        """
        if test_mirrored is None:
            test_mirrored = self._display_facade.is_mirrored_enabled()

        if test_mirrored:
            test_image_size = self._display_facade.get_internal_resolution()
        else:
            # DUT needs time to respond to the plug event
            test_image_size = utils.wait_for_value_changed(
                    self._display_facade.get_external_resolution,
                    old_value=None)
        error = None
        if test_image_size != expected_resolution:
            error = ('Screen size %s is not as expected %s!'
                     % (str(test_image_size), str(expected_resolution)))
            if test_mirrored:
                # For the case of mirroring, depending on hardware vs
                # software mirroring, screen size can be different.
                logging.info('Warning: %s', error)
                error = None
            else:
                error_list.append(error)

        if chameleon_supported:
            error = self._resolution_comparer.compare(expected_resolution)
            if not error:
                while retry_count:
                    retry_count = retry_count - 1
                    try:
                        self.load_test_image(test_image_size)
                        error = self.test_screen(expected_resolution, test_mirrored)
                        if error is None:
                            return error
                        elif retry_count > 0:
                            logging.info('Retry screen comparison again...')
                    finally:
                        self.unload_test_image()

            if error and error_list is not None:
                error_list.append(error)
        return error


    def check_external_display_connected(self, expected_display,
                                         error_list=None):
        """Checks the given external display connected.

        @param expected_display: Name of the expected display or False
                if no external display is expected.
        @param error_list: A list to append the error message to or None.
        @return: None if the check passes; otherwise, a string of error message.
        """
        error = None
        if not self._display_facade.wait_external_display_connected(
                expected_display):
            error = 'Waited for display %s but timed out' % expected_display

        if error and error_list is not None:
            logging.error(error)
            error_list.append(error)
        return error
