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


class ExitCode(object):
    """Exit codes for test binaries."""
    FUZZER_TEST_PASS = 0
    # Default failure error code in LLVM libfuzzer.
    FUZZER_TEST_FAIL = 77

# Directory on the target where the tests are copied.
FUZZER_TEST_DIR = "/data/local/tmp/libfuzzer_test"

# Directory on the target where VTS specs are copied.
FUZZER_SPEC_DIR = "/data/local/tmp/spec"

# Directory under data_file_path on the host where fuzzer executables are.
FUZZER_BIN_DIR = "DATA/bin"

# File used to save crash-causing fuzzer input.
FUZZER_TEST_CRASH_REPORT = FUZZER_TEST_DIR + "/crash_report"

# Default parameters that will be passed to fuzzer executable.
FUZZER_DEFAULT_PARAMS = {
    "max_len": 100,
    "max_total_time": 60,
    "exact_artifact_path": FUZZER_TEST_CRASH_REPORT,
    "timeout": 120,
}
