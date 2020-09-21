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

import os

from vts.utils.python.os import path_utils

from vts.testcases.fuzz.template.libfuzzer_test import libfuzzer_test_config as config


class LibFuzzerTestCase(object):
    """Represents libfuzzer test case.

    Attributes:
        _bin_host_path: string, path to binary on host.
        _bin_name: string, name of the binary.
        _test_name: string, name of the test case.
        _libfuzzer_params: dict, libfuzzer-specific parameters.
        _additional_params: dict, additional parameters.
    """

    def __init__(self, bin_host_path, libfuzzer_params, additional_params):
        self._bin_host_path = bin_host_path
        self._libfuzzer_params = libfuzzer_params
        self._additional_params = additional_params
        self._binary_name = os.path.basename(bin_host_path)
        self._test_name = self._binary_name

    def GetCorpusName(self):
        """Returns corpus directory name on target."""
        corpus_dir = path_utils.JoinTargetPath(
            config.FUZZER_TEST_DIR, '%s_corpus' % self._test_name)
        return corpus_dir

    def CreateFuzzerFlags(self):
        """Creates flags for the fuzzer executable.

        Returns:
            string, of form '-<flag0>=<val0> -<flag1>=<val1> ... '
        """
        # Used to separate additional and libfuzzer flags.
        DELIMITER = '--'
        additional_flags = ' '.join(
            ['--%s=%s' % (k, v) for k, v in self._additional_params.items()])
        libfuzzer_flags = ' '.join(
            ['-%s=%s' % (k, v) for k, v in self._libfuzzer_params.items()])
        if not additional_flags:
            flags = libfuzzer_flags
        else:
            flags = '%s %s %s' % (additional_flags, DELIMITER, libfuzzer_flags)
        return flags

    def GetRunCommand(self, debug_mode=False):
        """Returns target shell command to run the fuzzer binary."""
        test_flags = self.CreateFuzzerFlags()
        corpus_dir = '' if debug_mode else self.GetCorpusName()

        cd_cmd = 'cd %s' % config.FUZZER_TEST_DIR
        chmod_cmd = 'chmod 777 %s' % self._binary_name
        ld_path = 'LD_LIBRARY_PATH=/data/local/tmp/64:/data/local/tmp/32:$LD_LIBRARY_PATH'
        test_cmd = '%s ./%s %s %s' % (ld_path, self._binary_name, corpus_dir,
                                      test_flags)
        if not debug_mode:
          test_cmd += ' > /dev/null'
        return ' && '.join([cd_cmd, chmod_cmd, test_cmd])

    @property
    def test_name(self):
        """Name of this test case."""
        return str(self._test_name)

    @test_name.setter
    def test_name(self, name):
        """Set name of this test case."""
        self._test_name = name

    @property
    def bin_host_path(self):
        """Host path to binary for this test case."""
        return self._bin_host_path
