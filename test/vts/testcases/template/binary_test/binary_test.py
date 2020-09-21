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
import os.path
import posixpath as targetpath
import time

from vts.runners.host import asserts
from vts.runners.host import base_test
from vts.runners.host import const
from vts.runners.host import errors
from vts.runners.host import keys
from vts.runners.host import test_runner
from vts.utils.python.common import list_utils
from vts.utils.python.coverage import coverage_utils
from vts.utils.python.os import path_utils
from vts.utils.python.precondition import precondition_utils
from vts.utils.python.web import feature_utils

from vts.testcases.template.binary_test import binary_test_case

DATA_NATIVETEST = 'data/nativetest'
DATA_NATIVETEST64 = '%s64' % DATA_NATIVETEST


class BinaryTest(base_test.BaseTestClass):
    '''Base class to run binary tests on target.

    Attributes:
        _dut: AndroidDevice, the device under test as config
        shell: ShellMirrorObject, shell mirror
        testcases: list of BinaryTestCase objects, list of test cases to run
        tags: all the tags that appeared in binary list
        DEVICE_TMP_DIR: string, temp location for storing binary
        TAG_DELIMITER: string, separator used to separate tag and path
        SYSPROP_VTS_NATIVE_SERVER: string, the name of a system property which
                                   tells whether to stop properly configured
                                   native servers where properly configured
                                   means a server's init.rc is configured to
                                   stop when that property's value is 1.
    '''
    SYSPROP_VTS_NATIVE_SERVER = "vts.native_server.on"

    DEVICE_TMP_DIR = '/data/local/tmp'
    TAG_DELIMITER = '::'
    PUSH_DELIMITER = '->'
    DEFAULT_TAG_32 = '_%s' % const.SUFFIX_32BIT
    DEFAULT_TAG_64 = '_%s' % const.SUFFIX_64BIT
    DEFAULT_LD_LIBRARY_PATH_32 = '/data/local/tmp/32/'
    DEFAULT_LD_LIBRARY_PATH_64 = '/data/local/tmp/64/'
    DEFAULT_PROFILING_LIBRARY_PATH_32 = '/data/local/tmp/32/'
    DEFAULT_PROFILING_LIBRARY_PATH_64 = '/data/local/tmp/64/'

    def setUpClass(self):
        '''Prepare class, push binaries, set permission, create test cases.'''
        required_params = [
            keys.ConfigKeys.IKEY_DATA_FILE_PATH,
        ]
        opt_params = [
            keys.ConfigKeys.IKEY_BINARY_TEST_SOURCE,
            keys.ConfigKeys.IKEY_BINARY_TEST_WORKING_DIRECTORY,
            keys.ConfigKeys.IKEY_BINARY_TEST_ENVP,
            keys.ConfigKeys.IKEY_BINARY_TEST_ARGS,
            keys.ConfigKeys.IKEY_BINARY_TEST_LD_LIBRARY_PATH,
            keys.ConfigKeys.IKEY_BINARY_TEST_PROFILING_LIBRARY_PATH,
            keys.ConfigKeys.IKEY_BINARY_TEST_DISABLE_FRAMEWORK,
            keys.ConfigKeys.IKEY_BINARY_TEST_STOP_NATIVE_SERVERS,
            keys.ConfigKeys.IKEY_NATIVE_SERVER_PROCESS_NAME,
            keys.ConfigKeys.IKEY_PRECONDITION_FILE_PATH_PREFIX,
            keys.ConfigKeys.IKEY_PRECONDITION_SYSPROP,
        ]
        self.getUserParams(
            req_param_names=required_params, opt_param_names=opt_params)

        # test-module-name is required in binary tests.
        self.getUserParam(
            keys.ConfigKeys.KEY_TESTBED_NAME, error_if_not_found=True)

        logging.info("%s: %s", keys.ConfigKeys.IKEY_DATA_FILE_PATH,
                     self.data_file_path)

        self.binary_test_source = self.getUserParam(
            keys.ConfigKeys.IKEY_BINARY_TEST_SOURCE, default_value=[])

        self.working_directory = {}
        if hasattr(self, keys.ConfigKeys.IKEY_BINARY_TEST_WORKING_DIRECTORY):
            self.binary_test_working_directory = map(
                str, self.binary_test_working_directory)
            for token in self.binary_test_working_directory:
                tag = ''
                path = token
                if self.TAG_DELIMITER in token:
                    tag, path = token.split(self.TAG_DELIMITER)
                self.working_directory[tag] = path

        self.envp = {}
        if hasattr(self, keys.ConfigKeys.IKEY_BINARY_TEST_ENVP):
            self.binary_test_envp = map(str, self.binary_test_envp)
            for token in self.binary_test_envp:
                tag = ''
                path = token
                split = token.find(self.TAG_DELIMITER)
                if split >= 0:
                    tag, arg = token[:split], token[
                        split + len(self.TAG_DELIMITER):]
                if tag in self.envp:
                    self.envp[tag] += ' %s' % path
                else:
                    self.envp[tag] = path

        self.args = {}
        if hasattr(self, keys.ConfigKeys.IKEY_BINARY_TEST_ARGS):
            self.binary_test_args = map(str, self.binary_test_args)
            for token in self.binary_test_args:
                tag = ''
                arg = token
                split = token.find(self.TAG_DELIMITER)
                if split >= 0:
                    tag, arg = token[:split], token[
                        split + len(self.TAG_DELIMITER):]
                if tag in self.args:
                    self.args[tag] += ' %s' % arg
                else:
                    self.args[tag] = arg

        if hasattr(self, keys.ConfigKeys.IKEY_PRECONDITION_FILE_PATH_PREFIX):
            self.file_path_prefix = {
                self.DEFAULT_TAG_32: [],
                self.DEFAULT_TAG_64: [],
            }
            self.precondition_file_path_prefix = map(
                str, self.precondition_file_path_prefix)
            for token in self.precondition_file_path_prefix:
                tag = ''
                path = token
                if self.TAG_DELIMITER in token:
                    tag, path = token.split(self.TAG_DELIMITER)
                if tag == '':
                    self.file_path_prefix[self.DEFAULT_TAG_32].append(path)
                    self.file_path_prefix[self.DEFAULT_TAG_64].append(path)
                elif tag in self.file_path_prefix:
                    self.file_path_prefix[tag].append(path)
                else:
                    logging.warn(
                        "Incorrect tag %s in precondition-file-path-prefix",
                        tag)

        self.ld_library_path = {
            self.DEFAULT_TAG_32: self.DEFAULT_LD_LIBRARY_PATH_32,
            self.DEFAULT_TAG_64: self.DEFAULT_LD_LIBRARY_PATH_64,
        }
        if hasattr(self, keys.ConfigKeys.IKEY_BINARY_TEST_LD_LIBRARY_PATH):
            self.binary_test_ld_library_path = map(
                str, self.binary_test_ld_library_path)
            for token in self.binary_test_ld_library_path:
                tag = ''
                path = token
                if self.TAG_DELIMITER in token:
                    tag, path = token.split(self.TAG_DELIMITER)
                if tag in self.ld_library_path:
                    self.ld_library_path[tag] = '{}:{}'.format(
                        path, self.ld_library_path[tag])
                else:
                    self.ld_library_path[tag] = path

        self.profiling_library_path = {
            self.DEFAULT_TAG_32: self.DEFAULT_PROFILING_LIBRARY_PATH_32,
            self.DEFAULT_TAG_64: self.DEFAULT_PROFILING_LIBRARY_PATH_64,
        }
        if hasattr(self,
                   keys.ConfigKeys.IKEY_BINARY_TEST_PROFILING_LIBRARY_PATH):
            self.binary_test_profiling_library_path = map(
                str, self.binary_test_profiling_library_path)
            for token in self.binary_test_profiling_library_path:
                tag = ''
                path = token
                if self.TAG_DELIMITER in token:
                    tag, path = token.split(self.TAG_DELIMITER)
                self.profiling_library_path[tag] = path

        self._dut = self.android_devices[0]
        self.shell = self._dut.shell

        if self.coverage.enabled and self.coverage.global_coverage:
            self.coverage.InitializeDeviceCoverage(self._dut)
            for tag in [self.DEFAULT_TAG_32, self.DEFAULT_TAG_64]:
                if tag in self.envp:
                    self.envp[tag] = '%s %s'.format(
                        self.envp[tag], coverage_utils.COVERAGE_TEST_ENV)
                else:
                    self.envp[tag] = coverage_utils.COVERAGE_TEST_ENV

        self.testcases = []
        if not precondition_utils.CheckSysPropPrecondition(
                self, self._dut, self.shell):
            logging.info('Precondition sysprop not met; '
                         'all tests skipped.')
            self.skipAllTests('precondition sysprop not met')

        self.tags = set()
        self.CreateTestCases()
        cmd = list(
            set('chmod 755 %s' % test_case.path
                for test_case in self.testcases))
        cmd_results = self.shell.Execute(cmd)
        if any(cmd_results[const.EXIT_CODE]):
            logging.error('Failed to set permission to some of the binaries:\n'
                          '%s\n%s', cmd, cmd_results)

        if getattr(self, keys.ConfigKeys.IKEY_BINARY_TEST_DISABLE_FRAMEWORK,
                   False):
            # Disable the framework if requested.
            self._dut.stop()
        else:
            # Enable the framework if requested.
            self._dut.start()

        if getattr(self, keys.ConfigKeys.IKEY_BINARY_TEST_STOP_NATIVE_SERVERS,
                   False):
            logging.debug("Stops all properly configured native servers.")
            results = self._dut.setProp(self.SYSPROP_VTS_NATIVE_SERVER, "1")
            native_server_process_names = getattr(
                self, keys.ConfigKeys.IKEY_NATIVE_SERVER_PROCESS_NAME, [])
            if native_server_process_names:
                for native_server_process_name in native_server_process_names:
                    while True:
                        cmd_result = self.shell.Execute("ps -A")
                        if cmd_result[const.EXIT_CODE][0] != 0:
                            logging.error("ps command failed (exit code: %s",
                                          cmd_result[const.EXIT_CODE][0])
                            break
                        if (native_server_process_name not in cmd_result[
                                const.STDOUT][0]):
                            logging.info("Process %s not running",
                                         native_server_process_name)
                            break
                        logging.info("Checking process %s",
                                     native_server_process_name)
                        time.sleep(1)

    def CreateTestCases(self):
        '''Push files to device and create test case objects.'''
        source_list = list(map(self.ParseTestSource, self.binary_test_source))

        def isValidSource(source):
            '''Checks that the truth value and bitness of source is valid.

            Args:
                source: a tuple of (string, string, string or None),
                representing (host side absolute path, device side absolute
                path, tag), is the return value of self.ParseTestSource

            Returns:
                False if source has a false truth value or its bitness does
                not match the abi_bitness of the test run.
            '''
            if not source:
                return False

            tag = source[2]
            if tag is None:
                return True

            tag = str(tag)
            if (tag.endswith(const.SUFFIX_32BIT) and self.abi_bitness == '64'
                ) or (tag.endswith(const.SUFFIX_64BIT) and
                      self.abi_bitness == '32'):
                logging.info('Bitness of test source, %s, does not match the '
                             'abi_bitness, %s, of test run.', str(source[0]),
                             self.abi_bitness)
                return False

            return True

        source_list = filter(isValidSource, source_list)
        logging.info('Parsed test sources: %s', source_list)

        # Push source files first
        for src, dst, tag in source_list:
            if src:
                if os.path.isdir(src):
                    src = os.path.join(src, '.')
                logging.info('Pushing from %s to %s.', src, dst)
                self._dut.adb.push('{src} {dst}'.format(src=src, dst=dst))
                self.shell.Execute('ls %s' % dst)

        if not hasattr(self, 'testcases'):
            self.testcases = []

        # Then create test cases
        for src, dst, tag in source_list:
            if tag is not None:
                # tag not being None means to create a test case
                self.tags.add(tag)
                logging.info('Creating test case from %s with tag %s', dst,
                             tag)
                testcase = self.CreateTestCase(dst, tag)
                if not testcase:
                    continue

                if type(testcase) is list:
                    self.testcases.extend(testcase)
                else:
                    self.testcases.append(testcase)

        if not self.testcases:
            logging.warn("No test case is found or generated.")

    def PutTag(self, name, tag):
        '''Put tag on name and return the resulting string.

        Args:
            name: string, a test name
            tag: string

        Returns:
            String, the result string after putting tag on the name
        '''
        return '{}{}'.format(name, tag)

    def ExpandListItemTags(self, input_list):
        '''Expand list items with tags.

        Since binary test allows a tag to be added in front of the binary
        path, test names are generated with tags attached. This function is
        used to expand the filters correspondingly. If a filter contains
        a tag, only test name with that tag will be included in output.
        Otherwise, all known tags will be paired to the test name in output
        list.

        Args:
            input_list: list of string, the list to expand

        Returns:
            A list of string
        '''
        result = []
        for item in input_list:
            if self.TAG_DELIMITER in item:
                tag, name = item.split(self.TAG_DELIMITER)
                result.append(self.PutTag(name, tag))
            for tag in self.tags:
                result.append(self.PutTag(item, tag))
        return result

    def tearDownClass(self):
        '''Perform clean-up tasks'''
        if getattr(self, keys.ConfigKeys.IKEY_BINARY_TEST_STOP_NATIVE_SERVERS,
                   False):
            logging.debug("Restarts all properly configured native servers.")
            results = self._dut.setProp(self.SYSPROP_VTS_NATIVE_SERVER, "0")

        # Retrieve coverage if applicable
        if self.coverage.enabled and self.coverage.global_coverage:
            if not self.isSkipAllTests():
                self.coverage.SetCoverageData(dut=self._dut, isGlobal=True)

        # Clean up the pushed binaries
        logging.info('Start class cleaning up jobs.')
        # Delete pushed files

        sources = [
            self.ParseTestSource(src) for src in self.binary_test_source
        ]
        sources = set(filter(bool, sources))
        paths = [dst for src, dst, tag in sources if src and dst]
        cmd = ['rm -rf %s' % dst for dst in paths]
        cmd_results = self.shell.Execute(cmd, no_except=True)
        if not cmd_results or any(cmd_results[const.EXIT_CODE]):
            logging.warning('Failed to clean up test class: %s', cmd_results)

        # Delete empty directories in working directories
        dir_set = set(path_utils.TargetDirName(dst) for dst in paths)
        dir_set.add(self.ParseTestSource('')[1])
        dirs = list(dir_set)
        dirs.sort(lambda x, y: cmp(len(y), len(x)))
        cmd = ['rmdir %s' % d for d in dirs]
        cmd_results = self.shell.Execute(cmd, no_except=True)
        if not cmd_results or any(cmd_results[const.EXIT_CODE]):
            logging.warning('Failed to remove: %s', cmd_results)

        if not self.isSkipAllTests() and self.profiling.enabled:
            self.profiling.ProcessAndUploadTraceData()

        logging.info('Finished class cleaning up jobs.')

    def ParseTestSource(self, source):
        '''Convert host side binary path to device side path.

        Args:
            source: string, binary test source string

        Returns:
            A tuple of (string, string, string), representing (host side
            absolute path, device side absolute path, tag). Returned tag
            will be None if the test source is for pushing file to working
            directory only. If source file is specified for adb push but does not
            exist on host, None will be returned.
        '''
        tag = ''
        path = source
        if self.TAG_DELIMITER in source:
            tag, path = source.split(self.TAG_DELIMITER)

        src = path
        dst = None
        if self.PUSH_DELIMITER in path:
            src, dst = path.split(self.PUSH_DELIMITER)

        if src:
            src = os.path.join(self.data_file_path, src)
            if not os.path.exists(src):
                logging.warning('binary source file is specified '
                                'but does not exist on host: %s', src)
                return None

        push_only = dst is not None and dst == ''

        if not dst:
            parent = self.working_directory[
                tag] if tag in self.working_directory else self._GetDefaultBinaryPushDstPath(
                    src, tag)
            dst = path_utils.JoinTargetPath(parent, os.path.basename(src))

        if push_only:
            tag = None

        return str(src), str(dst), tag

    def _GetDefaultBinaryPushDstPath(self, src, tag):
        '''Get default binary push destination path.

        This method is called to get default push destination path when
        it is not specified.

        If binary source path contains 'data/nativetest[64]', then the binary
        will be pushed to /data/nativetest[64] instead of /data/local/tmp

        Args:
            src: string, source path of binary
            tag: string, tag of binary source

        Returns:
            string, default push path
        '''
        src_lower = src.lower()
        if DATA_NATIVETEST64 in src_lower:
            parent_path = targetpath.sep + DATA_NATIVETEST64
        elif DATA_NATIVETEST in src_lower:
            parent_path = targetpath.sep + DATA_NATIVETEST
        else:
            parent_path = self.DEVICE_TMP_DIR

        return targetpath.join(
            parent_path, 'vts_binary_test_%s' % self.__class__.__name__, tag)

    def CreateTestCase(self, path, tag=''):
        '''Create a list of TestCase objects from a binary path.

        Args:
            path: string, absolute path of a binary on device
            tag: string, a tag that will be appended to the end of test name

        Returns:
            A list of BinaryTestCase objects
        '''
        working_directory = self.working_directory[
            tag] if tag in self.working_directory else None
        envp = self.envp[tag] if tag in self.envp else ''
        args = self.args[tag] if tag in self.args else ''
        ld_library_path = self.ld_library_path[
            tag] if tag in self.ld_library_path else None
        profiling_library_path = self.profiling_library_path[
            tag] if tag in self.profiling_library_path else None

        return binary_test_case.BinaryTestCase(
            '',
            path_utils.TargetBaseName(path),
            path,
            tag,
            self.PutTag,
            working_directory,
            ld_library_path,
            profiling_library_path,
            envp=envp,
            args=args)

    def VerifyTestResult(self, test_case, command_results):
        '''Parse test case command result.

        Args:
            test_case: BinaryTestCase object, the test case whose command
            command_results: dict of lists, shell command result
        '''
        asserts.assertTrue(command_results, 'Empty command response.')
        asserts.assertFalse(
            any(command_results[const.EXIT_CODE]),
            'Test {} failed with the following results: {}'.format(
                test_case, command_results))

    def RunTestCase(self, test_case):
        '''Runs a test_case.

        Args:
            test_case: BinaryTestCase object
        '''
        if self.profiling.enabled:
            self.profiling.EnableVTSProfiling(self.shell,
                                              test_case.profiling_library_path)

        cmd = test_case.GetRunCommand()
        logging.info("Executing binary test command: %s", cmd)
        command_results = self.shell.Execute(cmd)

        self.VerifyTestResult(test_case, command_results)

        if self.profiling.enabled:
            self.profiling.ProcessTraceDataForTestCase(self._dut)
            self.profiling.DisableVTSProfiling(self.shell)

    def generateAllTests(self):
        '''Runs all binary tests.'''
        self.runGeneratedTests(
            test_func=self.RunTestCase, settings=self.testcases, name_func=str)


if __name__ == "__main__":
    test_runner.main()
