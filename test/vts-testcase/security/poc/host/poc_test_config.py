#!/usr/bin/env python3.4
#
# Copyright (C) 2016 The Android Open Source Project
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


class ConfigKeys(object):
    RUN_STAGING = "run_staging"

class ExitCode(object):
    """Exit codes for test binaries."""
    POC_TEST_PASS = 0
    POC_TEST_FAIL = 1
    POC_TEST_SKIP = 2

# Directory on the target where the tests are copied.
POC_TEST_DIR = "/data/local/tmp/security/poc"

POC_TEST_CASES_STABLE = [
]

POC_TEST_CASES_STAGING = [
    "kernel_sound/28838221",
    "kernel_bluetooth/30149612",
    "kernel_wifi/32219453",
    "kernel_wifi/31707909",
    "kernel_wifi/32402310",
]

POC_TEST_CASES_DISABLED = [
]
