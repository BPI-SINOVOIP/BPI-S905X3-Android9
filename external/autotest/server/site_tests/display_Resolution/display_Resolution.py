# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This is a server side resolution display test using the Chameleon board."""

import logging
import os
import time

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.chameleon import chameleon_port_finder
from autotest_lib.client.cros.chameleon import chameleon_screen_test
from autotest_lib.client.cros.chameleon import edid
from autotest_lib.server import test
from autotest_lib.server.cros.multimedia import remote_facade_factory


class display_Resolution(test.test):
    """Server side external display test.

    This test talks to a Chameleon board and a DUT to set up, run, and verify
    external display function of the DUT.
    """
    version = 1

    # Allowed timeout for reboot.
    REBOOT_TIMEOUT = 30
    # Time to allow lid transition to take effect
    WAIT_TIME_LID_TRANSITION = 5

    DEFAULT_RESOLUTION_LIST = [
            ('EDIDv1', 1280, 800),
            ('EDIDv1', 1440, 900),
            ('EDIDv1', 1600, 900),
            ('EDIDv1', 1680, 1050),
            ('EDIDv2', 1280, 720),
            ('EDIDv2', 1920, 1080),
    ]
    # These boards are unable to work with servo - crosbug.com/p/27591.
    INCOMPATIBLE_SERVO_BOARDS = ['daisy', 'falco']

    def run_once(self, host, test_mirrored=False, test_suspend_resume=False,
                 test_reboot=False, test_lid_close_open=False,
                 resolution_list=None):

        # Check the servo object.
        if test_lid_close_open and host.servo is None:
            raise error.TestError('Invalid servo object found on the host.')
        if test_lid_close_open and not host.get_board_type() == 'CHROMEBOOK':
            raise error.TestNAError('DUT is not Chromebook. Test Skipped')
        if test_mirrored and not host.get_board_type() == 'CHROMEBOOK':
            raise error.TestNAError('DUT is not Chromebook. Test Skipped')

        # Check for incompatible with servo chromebooks.
        board_name = host.get_board().split(':')[1]
        if board_name in self.INCOMPATIBLE_SERVO_BOARDS:
            raise error.TestNAError(
                    'DUT is incompatible with servo. Skipping test.')

        factory = remote_facade_factory.RemoteFacadeFactory(host)
        display_facade = factory.create_display_facade()
        chameleon_board = host.chameleon

        chameleon_board.setup_and_reset(self.outputdir)
        finder = chameleon_port_finder.ChameleonVideoInputFinder(
                chameleon_board, display_facade)

        errors = []
        if resolution_list is None:
            resolution_list = self.DEFAULT_RESOLUTION_LIST
        chameleon_supported = True
        for chameleon_port in finder.iterate_all_ports():
            screen_test = chameleon_screen_test.ChameleonScreenTest(
                    host, chameleon_port, display_facade, self.outputdir)
            chameleon_port_name = chameleon_port.get_connector_type()
            logging.info('Detected %s chameleon port.', chameleon_port_name)
            for label, width, height in resolution_list:
                test_resolution = (width, height)
                test_name = "%s_%dx%d" % ((label,) + test_resolution)

                # The chameleon DP RX doesn't support 4K resolution.
                # The max supported resolution is 2560x1600.
                # See crbug/585900
                if (chameleon_port_name.startswith('DP') and
                    test_resolution > (2560,1600)):
                    chameleon_supported = False

                if not edid.is_edid_supported(host, width, height):
                    logging.info('Skip unsupported EDID: %s', test_name)
                    continue

                if test_lid_close_open:
                    logging.info('Close lid...')
                    host.servo.lid_close()
                    time.sleep(self.WAIT_TIME_LID_TRANSITION)

                if test_reboot:
                    # Unplug the monitor explicitly. Otherwise, the following
                    # use_edid_file() call would expect a valid video signal,
                    # which is not true during reboot.
                    chameleon_port.unplug()
                    logging.info('Reboot...')
                    boot_id = host.get_boot_id()
                    host.reboot(wait=False)
                    host.test_wait_for_shutdown(self.REBOOT_TIMEOUT)

                path = os.path.join(self.bindir, 'test_data', 'edids',
                                    test_name)
                logging.info('Use EDID: %s', test_name)
                with chameleon_port.use_edid_file(path):
                    if test_lid_close_open:
                        logging.info('Open lid...')
                        host.servo.lid_open()
                        time.sleep(self.WAIT_TIME_LID_TRANSITION)

                    if test_reboot:
                        host.test_wait_for_boot(boot_id)
                        chameleon_port.plug()

                    utils.wait_for_value_changed(
                            display_facade.get_external_connector_name,
                            old_value=False)

                    logging.info('Set mirrored: %s', test_mirrored)
                    display_facade.set_mirrored(test_mirrored)
                    if test_suspend_resume:
                        if test_mirrored:
                            # magic sleep to wake up nyan_big in mirrored mode
                            # TODO: find root cause
                            time.sleep(6)
                        logging.info('Going to suspend...')
                        display_facade.suspend_resume()
                        logging.info('Resumed back')

                    screen_test.test_screen_with_image(test_resolution,
                            test_mirrored, errors, chameleon_supported)

        if errors:
            raise error.TestFail('; '.join(set(errors)))
