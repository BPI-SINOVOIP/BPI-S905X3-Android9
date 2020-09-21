#!/usr/bin/env python3.4
#
#   Copyright 2017 - The Android Open Source Project
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

from acts import base_test
from acts.libs.ota import ota_updater


class OtaSampleTest(base_test.BaseTestClass):
    """Demonstrates an example OTA Update test."""

    def setup_class(self):
        ota_updater.initialize(self.user_params, self.android_devices)
        self.dut = self.android_devices[0]

    def test_my_test(self):
        self.pre_ota()
        ota_updater.update(self.dut)
        self.post_ota()

    def pre_ota(self):
        pass

    def post_ota(self):
        pass
