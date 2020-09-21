#!/usr/bin/env python3.4
#
#   Copyright 2016 - The Android Open Source Project
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

import logging

from acts import base_test
from acts.controllers.ap_lib import hostapd_config


class ConfigurableAccessPointTest(base_test.BaseTestClass):
    """ Demonstrates example usage of a configurable access point."""

    def setup_class(self):
        self.ap = self.access_points[0]

    def setup_test(self):
        self.ap.stop_all_aps()

    def teardown_test(self):
        self.ap.stop_all_aps()

    def test_setup_access_point(self):
        config = hostapd_config.HostapdConfig(
            channel=6, ssid='ImagineYourNetworkHere')
        self.ap.start_ap(config)

        logging.info('Your test logic should go here!')
