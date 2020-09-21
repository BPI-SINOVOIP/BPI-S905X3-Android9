# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2015, ARM Limited and contributors.
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may
# not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import re
import os
import logging

from time import sleep

from target_script import TargetScript
from android import Screen, System
from android.workload import Workload


class DisplayImage(Workload):
    """
    Android DisplayImage workload
    """

    package = 'com.google.android.apps.photos'
    action = 'android.intent.action.VIEW'

    def __init__(self, test_env):
        super(DisplayImage, self).__init__(test_env)
        self._log = logging.getLogger('DisplayImage')
        self._log.debug('Workload created')

        # Set of output data reported by DisplayImage
        self.db_file = None

    def run(self, out_dir, duration_s, brightness, filepath, collect=''):
        """
        Run single display image workload.

        :param out_dir: Path to experiment directory where to store results.
        :type out_dir: str

        :param duration_s: Duration of test
        :type duration_s: int

        :param brightness: Brightness of the screen 0-100
        :type brightness: int

        :param out_dir: Path to the image to display
        :type out_dir: str

        :param collect: Specifies what to collect. Possible values:
            - 'energy'
            - 'time_in_state'
            - 'systrace'
            - 'ftrace'
            - any combination of the above, except energy and display-energy
              cannot be collected simultaneously.
        :type collect: list(str)
        """

        # Keep track of mandatory parameters
        self.out_dir = out_dir
        self.collect = collect

        # Set brightness
        Screen.set_brightness(self._target, auto=False, percent=brightness)

        # Set timeout to be sufficiently longer than the duration of the test
        Screen.set_timeout(self._target, seconds=(duration_s+60))

        # Turn on airplane mode
        System.set_airplane_mode(self._target, on=True)

        # Unlock device screen (assume no password required)
        Screen.unlock(self._target)

        # Push the image to the device
        device_filepath = '/data/local/tmp/image'
        self._target.push(filepath, device_filepath)
        sleep(1)

        # Put image on the screen
        System.start_action(self._target, self.action,
                '-t image/* -d file://{}'.format(device_filepath))
        sleep(1)

        # Dimiss the navigation bar by tapping the center of the screen
        System.tap(self._target, 50, 50)

        self.tracingStart(screen_always_on=False)

        self._log.info('Waiting for duration {}'.format(duration_s))
        sleep(duration_s)

        self.tracingStop(screen_always_on=False)

        # Close app and dismiss image
        System.force_stop(self._target, self.package, clear=True)

        # Remove image
        self._target.execute('rm /data/local/tmp/image')

        Screen.set_defaults(self._target)
        System.set_airplane_mode(self._target, on=False)

        Screen.set_screen(self._target, on=False)

# vim :set tabstop=4 shiftwidth=4 expandtab
