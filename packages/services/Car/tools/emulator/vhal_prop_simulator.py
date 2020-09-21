#!/usr/bin/env python
#
# Copyright (C) 2017 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import argparse
import time

from threading import Thread

import driving_info_generator
import user_action_generator
import vhal_consts_2_0 as c

from vhal_emulator import Vhal

DEFAULT_TIMEOUT_SEC = 60 * 60 # 1 hour

class VhalPropSimulator(object):
    """
        A class simulates vhal properties by calling each generator in a separate thread. It is
        itself a listener passed to each generator to handle vhal event
    """

    def __init__(self, device, gpxFile,):
        self.vhal = Vhal(c.vhal_types_2_0, device)
        self.gpxFile = gpxFile

    def handle(self, prop, area_id, value, desc=None):
        """
            handle generated VHAL property by injecting through vhal emulator.
        """
        print "Generated property %s with value: %s" % (desc, value)
        self.vhal.setProperty(prop, area_id, value)

    def _startGeneratorThread(self, generator):
        thread = Thread(target=generator.generate, args=(self,))
        thread.daemon = True
        thread.start()

    def run(self, timeout):
        userActionGenerator = user_action_generator.UserActionGenerator(self.vhal)
        drivingInfoGenerator = driving_info_generator.DrivingInfoGenerator(self.gpxFile, self.vhal)
        self._startGeneratorThread(userActionGenerator)
        self._startGeneratorThread(drivingInfoGenerator)
        time.sleep(float(timeout))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Vhal Property Simulator')
    parser.add_argument('-s', action='store', dest='deviceid', default=None)
    parser.add_argument('--timeout', action='store', dest='timeout', default=DEFAULT_TIMEOUT_SEC)
    parser.add_argument('--gpx', action='store', dest='gpxFile', default=None)
    args = parser.parse_args()

    simulator = VhalPropSimulator(device=args.deviceid, gpxFile=args.gpxFile)
    simulator.run(args.timeout)
