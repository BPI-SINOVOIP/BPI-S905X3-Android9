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

import logging
import os

from vts.runners.host import asserts
from vts.runners.host import base_test
from vts.runners.host import const
from vts.runners.host import keys
from vts.runners.host import test_runner
from vts.utils.python.controllers import adb

from vts.utils.python.common import list_utils
from vts.utils.python.os import path_utils

from vts.testcases.template.llvmfuzzer_test import llvmfuzzer_test_config as config


class LLVMFuzzerTest(base_test.BaseTestClass):
    """Runs fuzzer tests on target.

    Attributes:
        _dut: AndroidDevice, the device under test as config
        _testcases: string list, list of testcases to run
    """

    def setUpClass(self):
        """Creates a remote shell instance, and copies data files."""
        required_params = [
            keys.ConfigKeys.IKEY_DATA_FILE_PATH,
            config.ConfigKeys.FUZZER_CONFIGS
        ]
        self.getUserParams(required_params)

        self._testcases = map(lambda x: str(x), self.fuzzer_configs.keys())

        logging.info("Testcases: %s", self._testcases)
        logging.info("%s: %s", keys.ConfigKeys.IKEY_DATA_FILE_PATH,
                     self.data_file_path)
        logging.info("%s: %s", config.ConfigKeys.FUZZER_CONFIGS,
                     self.fuzzer_configs)

        self._dut = self.registerController(android_device, False)[0]
        self._dut.adb.shell("mkdir %s -p" % config.FUZZER_TEST_DIR)

    def tearDownClass(self):
        """Deletes all copied data."""
        self._dut.adb.shell("rm -rf %s" % config.FUZZER_TEST_DIR)

    def PushFiles(self, testcase):
        """adb pushes testcase file to target.

        Args:
            testcase: string, path to executable fuzzer.
        """
        push_src = os.path.join(self.data_file_path, config.FUZZER_SRC_DIR,
                                testcase)
        self._dut.adb.push("%s %s" % (push_src, config.FUZZER_TEST_DIR))
        logging.info("Adb pushed: %s", testcase)

    def CreateFuzzerFlags(self, fuzzer_config):
        """Creates flags for the fuzzer executable.

        Args:
            fuzzer_config: dict, contains configuration for the fuzzer.

        Returns:
            string, command line flags for fuzzer executable.
        """

        def _SerializeVTSFuzzerParams(params):
            """Creates VTS command line flags for fuzzer executable.

            Args:
                params: dict, contains flags and their values.

            Returns:
                string, of form "--<flag0>=<val0> --<flag1>=<val1> ... "
            """
            VTS_SPEC_FILES = "vts_spec_files"
            VTS_EXEC_SIZE = "vts_exec_size"
            DELIMITER = ":"

            # vts_spec_files is a string list, will be serialized like this:
            # [a, b, c] -> "a:b:c"
            vts_spec_files = params.get(VTS_SPEC_FILES, {})
            target_vts_spec_files = DELIMITER.join(map(
                lambda x: path_utils.JoinTargetPath(config.FUZZER_SPEC_DIR, x),
                vts_spec_files))
            flags = "--%s=\"%s\" " % (VTS_SPEC_FILES, target_vts_spec_files)

            vts_exec_size = params.get(VTS_EXEC_SIZE, {})
            flags += "--%s=%s" % (VTS_EXEC_SIZE, vts_exec_size)
            return flags

        def _SerializeLLVMFuzzerParams(params):
            """Creates LLVM libfuzzer command line flags for fuzzer executable.

            Args:
                params: dict, contains flags and their values.

            Returns:
                string, of form "--<flag0>=<val0> --<flag1>=<val1> ... "
            """
            return " ".join(["-%s=%s" % (k, v) for k, v in params.items()])

        vts_fuzzer_params = fuzzer_config.get("vts_fuzzer_params", {})

        llvmfuzzer_params = config.FUZZER_PARAMS.copy()
        llvmfuzzer_params.update(fuzzer_config.get("llvmfuzzer_params", {}))

        vts_fuzzer_flags = _SerializeVTSFuzzerParams(vts_fuzzer_params)
        llvmfuzzer_flags = _SerializeLLVMFuzzerParams(llvmfuzzer_params)

        return vts_fuzzer_flags + " -- " + llvmfuzzer_flags

    def CreateCorpus(self, fuzzer, fuzzer_config):
        """Creates a corpus directory on target.

        Args:
            fuzzer: string, name of the fuzzer executable.
            fuzzer_config: dict, contains configuration for the fuzzer.

        Returns:
            string, path to corpus directory on the target.
        """
        corpus = fuzzer_config.get("corpus", [])
        corpus_dir = path_utils.JoinTargetPath(config.FUZZER_TEST_DIR,
                                               "%s_corpus" % fuzzer)

        self._dut.adb.shell("mkdir %s -p" % corpus_dir)
        for idx, corpus_entry in enumerate(corpus):
            corpus_entry = corpus_entry.replace("x", "\\x")
            corpus_entry_file = path_utils.JoinTargetPath(
                corpus_dir, "input%s" % idx)
            cmd = "echo -ne '%s' > %s" % (str(corpus_entry), corpus_entry_file)
            # Vts shell drive doesn't play nicely with escape characters,
            # so we use adb shell.
            self._dut.adb.shell("\"%s\"" % cmd)

        return corpus_dir

    def RunTestcase(self, fuzzer):
        """Runs the given testcase and asserts the result.

        Args:
            fuzzer: string, name of fuzzer executable.
        """
        self.PushFiles(fuzzer)

        fuzzer_config = self.fuzzer_configs.get(fuzzer, {})
        test_flags = self.CreateFuzzerFlags(fuzzer_config)
        corpus_dir = self.CreateCorpus(fuzzer, fuzzer_config)

        chmod_cmd = "chmod -R 755 %s" % path_utils.JoinTargetPath(
            config.FUZZER_TEST_DIR, fuzzer)
        self._dut.adb.shell(chmod_cmd)

        cd_cmd = "cd %s" % config.FUZZER_TEST_DIR
        ld_path = "LD_LIBRARY_PATH=/data/local/tmp/64:/data/local/tmp/32:$LD_LIBRARY_PATH"
        test_cmd = "./%s" % fuzzer

        fuzz_cmd = "%s && %s %s %s %s > /dev/null" % (cd_cmd, ld_path,
                                                      test_cmd, corpus_dir,
                                                      test_flags)
        logging.info("Executing: %s", fuzz_cmd)
        # TODO(trong): vts shell doesn't handle timeouts properly, change this after it does.
        try:
            stdout = self._dut.adb.shell("'%s'" % fuzz_cmd)
            result = {
                const.STDOUT: stdout,
                const.STDERR: "",
                const.EXIT_CODE: 0
            }
        except adb.AdbError as e:
            result = {
                const.STDOUT: e.stdout,
                const.STDERR: e.stderr,
                const.EXIT_CODE: e.ret_code
            }
        self.AssertTestResult(fuzzer, result)

    def LogCrashReport(self, fuzzer):
        """Logs crash-causing fuzzer input.

        Reads the crash report file and logs the contents in format:
        "\x01\x23\x45\x67\x89\xab\xcd\xef"

        Args:
            fuzzer: string, name of fuzzer executable.
        """
        cmd = "xxd -p %s" % config.FUZZER_TEST_CRASH_REPORT

        # output is string of a hexdump from crash report file.
        # From the example above, output would be "0123456789abcdef".
        output = self._dut.adb.shell(cmd)
        remove_chars = ["\r", "\t", "\n", " "]
        for char in remove_chars:
            output = output.replace(char, "")

        crash_report = ""
        # output is guaranteed to be even in length since its a hexdump.
        for offset in xrange(0, len(output), 2):
            crash_report += "\\x%s" % output[offset:offset + 2]

        logging.info('FUZZER_TEST_CRASH_REPORT for %s: "%s"', fuzzer,
                     crash_report)

    # TODO(trong): differentiate between crashes and sanitizer rule violations.
    def AssertTestResult(self, fuzzer, result):
        """Asserts that testcase finished as expected.

        Checks that device is in responsive state. If not, waits for boot
        then reports test as failure. If it is, asserts that all test commands
        returned exit code 0.

        Args:
            fuzzer: string, name of fuzzer executable.
            result: dict(str, str, int), command results from shell.
        """
        logging.info("Test result: %s" % result)
        if not self._dut.hasBooted():
            self._dut.waitForBootCompletion()
            asserts.fail("%s left the device in unresponsive state." % fuzzer)

        exit_code = result[const.EXIT_CODE]
        if exit_code == config.ExitCode.FUZZER_TEST_FAIL:
            self.LogCrashReport(fuzzer)
            asserts.fail("%s failed normally." % fuzzer)
        elif exit_code != config.ExitCode.FUZZER_TEST_PASS:
            asserts.fail("%s failed abnormally." % fuzzer)

    def generateFuzzerTests(self):
        """Runs fuzzer tests."""
        self.runGeneratedTests(
            test_func=self.RunTestcase,
            settings=self._testcases,
            name_func=lambda x: x.split("/")[-1])


if __name__ == "__main__":
    test_runner.main()
