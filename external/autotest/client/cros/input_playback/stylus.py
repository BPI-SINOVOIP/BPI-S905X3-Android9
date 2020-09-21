# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os

from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.graphics import graphics_utils
from autotest_lib.client.cros.input_playback import input_playback

_CLICK_EVENTS = '/tmp/click_events'
_CLICK_TEMPLATE = 'click_events.template'
_PREFIX_RESOLUTION = 'RESOLUTION'
_PREFIX_POSITION = 'POSITION'
_STYLUS_DEVICE = 'stylus'
_STYLUS_PROPERTY = '/tmp/stylus.prop'
_STYLUS_TEMPLATE = 'stylus.prop.template'


class Stylus(object):
    """An emulated stylus device used for UI automation."""

    def __init__(self):
        """Prepare an emulated stylus device based on the internal display."""
        self.dirname = os.path.dirname(__file__)
        width, height = graphics_utils.get_internal_resolution()
        logging.info('internal display W = %d H = %d ', width, height)
        # Skip the test if there is no internal display
        if width == -1:
            raise error.TestNAError('No internal display')

        # Enlarge resolution of the emulated stylus.
        self.width = width * 10
        self.height = height * 10
        stylus_template = os.path.join(self.dirname, _STYLUS_TEMPLATE)
        self.replace_with_prefix(stylus_template, _STYLUS_PROPERTY,
                                 _PREFIX_RESOLUTION, self.width, self.height)
        # Create an emulated stylus device.
        self.stylus = input_playback.InputPlayback()
        self.stylus.emulate(input_type=_STYLUS_DEVICE,
                            property_file=_STYLUS_PROPERTY)
        self.stylus.find_connected_inputs()

    def replace_with_prefix(self, in_file, out_file, prefix, x_value, y_value):
        """Substitute with the real positions and write to an output file.

        Replace the keywords in template file with the real values and save
        the results into a file.

        @param in_file: the template file containing keywords for substitution.
        @param out_file: the generated file after substitution.
        @param prefix: the prefix of the keywords for substituion.
        @param x_value: the target value of X.
        @param y_value: the target value of Y.

        """
        with open(in_file) as infile:
            content = infile.readlines()

        with open(out_file, 'w') as outfile:
            for line in content:
                if line.find(prefix + '_X') > 0:
                    line = line.replace(prefix + '_X', str(x_value))
                    x_value += 1
                else:
                    if line.find(prefix + '_Y') > 0:
                        line = line.replace(prefix + '_Y', str(y_value))
                        y_value += 1
                outfile.write(line)

    def click(self, position_x, position_y):
        """Click the point(x,y) on the emulated stylus panel.

        @param position_x: the X position of the click point.
        @param position_y: the Y position of the click point.

        """
        click_template = os.path.join(self.dirname, _CLICK_TEMPLATE)
        self.replace_with_prefix(click_template,
                                 _CLICK_EVENTS,
                                 _PREFIX_POSITION,
                                 position_x * 10,
                                 position_y * 10)
        self.stylus.blocking_playback(_CLICK_EVENTS, input_type=_STYLUS_DEVICE)

    def click_with_percentage(self, percent_x, percent_y):
        """Click a point based on the percentage of the display.

        @param percent_x: the percentage of X position over display width.
        @param percent_y: the percentage of Y position over display height.

        """
        position_x = int(percent_x * self.width / 10)
        position_y = int(percent_y * self.height / 10)
        self.click(position_x, position_y)

    def close(self):
        """Clean up the files/handles created in the class."""
        if self.stylus:
            self.stylus.close()
        if os.path.exists(_STYLUS_PROPERTY):
            os.remove(_STYLUS_PROPERTY)
        if os.path.exists(_CLICK_EVENTS):
            os.remove(_CLICK_EVENTS)
