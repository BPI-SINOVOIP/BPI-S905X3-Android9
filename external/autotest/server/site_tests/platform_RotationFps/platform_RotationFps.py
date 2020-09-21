# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This is a server side screen rotation test using the Chameleon board."""

import logging

from autotest_lib.client.common_lib import error
from autotest_lib.server import test
from autotest_lib.server.cros.chameleon import chameleon_measurer


class platform_RotationFps(test.test):
    """Server side screen rotation test.

    This test talks to a Chameleon board and a DUT to set up dock mode,
    change rotation, and measure the fps.
    """
    version = 1

    DELAY_BEFORE_ROTATION = 5
    DELAY_AFTER_ROTATION = 5

    def run_once(self, host):
        # Check the servo object
        if host.servo is None:
            raise error.TestError('Invalid servo object found on the host.')
        if host.get_board_type() != 'CHROMEBOOK':
            raise error.TestNAError('DUT is not Chromebook. Test Skipped')

        measurer = chameleon_measurer.RemoteChameleonMeasurer(
                host, self.outputdir)
        display_facade = measurer.display_facade

        chameleon_board = host.chameleon
        chameleon_board.setup_and_reset(self.outputdir)

        with measurer.start_dock_mode_measurement() as chameleon_port:
            chameleon_port_name = chameleon_port.get_connector_type()
            logging.info('Detected %s chameleon port.', chameleon_port_name)
            display_id = display_facade.get_first_external_display_id()

            # Ask Chameleon to capture the video during rotation
            chameleon_port.start_capturing_video()
            # Rotate the screen to 90 degree.
            # Adding delays before and after rotation such that we can easily
            # know the rotation fps, not other animation like opening a tab.
            display_facade.set_display_rotation(
                    display_id, 90, self.DELAY_BEFORE_ROTATION,
                    self.DELAY_AFTER_ROTATION)
            chameleon_port.stop_capturing_video()
            # Restore back to 0 degree.
            display_facade.set_display_rotation(display_id, 0)

            # Retrieve the FPS info
            fps_list = chameleon_port.get_captured_fps_list()
            # Cut the fps numbers before and after rotation
            fps_list = fps_list[-(self.DELAY_AFTER_ROTATION + 1):
                                -(self.DELAY_AFTER_ROTATION - 1)]
            # The fps during rotation may cross the second-boundary. Sum them.
            fps = sum(fps_list)
            logging.info('***RESULT*** Rotation FPS is %d (max 15)', fps)

            # Output the perf value
            self.output_perf_value(description='Rotation FPS',
                                   value=fps,
                                   higher_is_better=True,
                                   units='fps')
