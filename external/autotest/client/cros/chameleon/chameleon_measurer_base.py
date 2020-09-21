# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time
from contextlib import contextmanager

from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.chameleon import chameleon_port_finder


class _BaseChameleonMeasurer(object):
    """Base class of performing measurement using Chameleon."""

    _TIME_WAIT_FADE_OUT = 10
    _WAIT_TIME_LID_TRANSITION = 5

    def __init__(self, cros_host, outputdir=None):
        """Initializes the object."""
        raise NotImplementedError('_BaseChameleonMeasurer.__init__')


    @contextmanager
    def start_mirrored_mode_measurement(self):
        """Starts the mirrored mode to measure.

        It iterates the connection ports between DUT and Chameleon and uses
        the first port. Sets DUT into the mirrored mode. Then yields the
        connected ports.

        It is used via a with statement, like the following:

            measurer = LocalChameleonMeasurer(cros_host, args, chrome)
            with measurer.start_mirrored_mode_measurement() as chameleon_port:
                # chameleon_port is automatically plugged before this line.
                do_some_test_on(chameleon_port)
                # chameleon_port is automatically unplugged after this line.

        @yields the first connected ChameleonVideoInput which is ensured plugged
                before yielding.

        @raises TestFail if no connected video port.
        """
        finder = chameleon_port_finder.ChameleonVideoInputFinder(
                self.chameleon, self.display_facade)
        with finder.use_first_port() as chameleon_port:
            logging.info('Used Chameleon port: %s',
                         chameleon_port.get_connector_type())

            logging.info('Setting to mirrored mode')
            self.display_facade.set_mirrored(True)

            # Hide the typing cursor.
            self.display_facade.hide_typing_cursor()

            # Sleep a while to wait the pop-up window faded-out.
            time.sleep(self._TIME_WAIT_FADE_OUT)

            # Get the resolution to make sure Chameleon in a good state.
            resolution = chameleon_port.get_resolution()
            logging.info('Detected the resolution: %dx%d', *resolution)

            yield chameleon_port

    @contextmanager
    def start_dock_mode_measurement(self):
        """Starts the dock mode to measure.

        It iterates the connection ports between DUT and Chameleon and uses
        the first port. Sets DUT into the dock mode. Then yields the
        connected ports.

        It is used via a with statement, like the following:

            measurer = LocalChameleonMeasurer(cros_host, args, chrome)
            with measurer.start_dock_mode_measurement() as chameleon_port:
                # chameleon_port is automatically plugged before this line
                # and lid is close to enter doc mode.
                do_some_test_on(chameleon_port)
                # chameleon_port is automatically unplugged after this line
                # and lid is open again.

        @yields the first connected ChameleonVideoInput which is ensured plugged
                before yielding.

        @raises TestFail if no connected video port or fail to enter dock mode.
        """
        finder = chameleon_port_finder.ChameleonVideoInputFinder(
                self.chameleon, self.display_facade)
        try:
            with finder.use_first_port() as chameleon_port:
                logging.info('Used Chameleon port: %s',
                             chameleon_port.get_connector_type())

                logging.info('Close lid to switch into dock mode...')
                self.host.servo.lid_close()
                time.sleep(self._WAIT_TIME_LID_TRANSITION)

                # Hide the typing cursor.
                self.display_facade.hide_typing_cursor()

                # Sleep a while to wait the pop-up window faded-out.
                time.sleep(self._TIME_WAIT_FADE_OUT)

                # Get the resolution to make sure Chameleon in a good state.
                resolution = chameleon_port.get_resolution()
                logging.info('Detected the resolution: %dx%d', *resolution)

                # Check if it is dock mode, no internal screen.
                if self.display_facade.get_internal_resolution() is not None:
                    raise error.TestError('Failed to enter dock mode: '
                                          'internal display still valid')

                yield chameleon_port

        finally:
            logging.info('Open lid again...')
            self.host.servo.lid_open()
