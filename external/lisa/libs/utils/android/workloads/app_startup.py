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

from subprocess import Popen, PIPE
from time import sleep

from android import Screen, System, Workload

class AppStartup(Workload):
    """
    Android AppStartup workload
    """

    # Package required by this workload
    package = 'optional'

    def __init__(self, test_env):
        super(AppStartup, self).__init__(test_env)
        self._log = logging.getLogger('AppStartup')
        self._log.debug('Workload created')

        # Set of output data reported by AppStartup
        self.db_file = None

    def run(self, out_dir, package, permissions, duration_s, collect=''):
        """
        Run single AppStartup workload.

        :param out_dir: Path to experiment directory where to store results.
        :type out_dir: str

        :param package: Name of the apk package
        :type package: str

        :param permissions: List of permissions the app requires
        :type permissions: list of str

        :param duration_s: Run benchmak for this required number of seconds
        :type duration_s: int

        :param collect: Specifies what to collect. Possible values:
            - 'energy'
            - 'time_in_state'
            - 'systrace'
            - 'ftrace'
            - any combination of the above
        :type collect: list(str)
        """

        # Keep track of mandatory parameters
        self.out_dir = out_dir
        self.collect = collect

        # Check if the package exists on the device
        if not System.contains_package(self._target, package):
            raise RuntimeError('System does not contain the requried package')

        # Unlock device screen (assume no password required)
        Screen.unlock(self._target)

        # Close and clear application
        System.force_stop(self._target, package, clear=True)
        sleep(3)

        # Grant permissions
        for permission in permissions:
            System.grant_permission(self._target, package, permission)
        sleep(3)

        # Set min brightness
        Screen.set_brightness(self._target, auto=False, percent=100)

        # Set screen timeout
        Screen.set_timeout(self._target, seconds=duration_s+60)

        # Force screen in PORTRAIT mode
        Screen.set_orientation(self._target, portrait=True)

        # Start the main view of the app
        System.monkey(self._target, package)

        # Start tracing
        self.tracingStart()

        # Sleep for duration
        sleep(duration_s)

        # Stop tracing
        self.tracingStop()

        # Reset permissions
        System.reset_permissions(self._target, package)

        # Close and clear application
        System.force_stop(self._target, package, clear=True)

        # Go back to home screen
        System.home(self._target)

        # Switch back to original settings
        Screen.set_defaults(self._target)

# vim :set tabstop=4 shiftwidth=4 expandtab
