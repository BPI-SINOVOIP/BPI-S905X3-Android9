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

import logging

from vts.runners.host import asserts
from vts.runners.host import base_test
from vts.runners.host import const
from vts.runners.host import keys
from vts.runners.host import test_runner
from vts.testcases.vndk.golden import vndk_data
from vts.utils.python.vndk import vndk_utils


class VtsVndkOpenLibrariesTest(base_test.BaseTestClass):
    """A test module to verify libraries opened by running processes.

    Attributes:
        data_file_path: The path to VTS data directory.
        _dut: The AndroidDevice under test.
        _shell: The ShellMirrorObject to execute commands
    """

    def setUpClass(self):
        """Initializes the data file path and shell."""
        required_params = [keys.ConfigKeys.IKEY_DATA_FILE_PATH]
        self.getUserParams(required_params)
        self._dut = self.android_devices[0]
        self._shell = self._dut.shell

    def _ListProcessCommands(self, cmd_filter):
        """Finds current processes whose commands match the filter.

        Args:
            cmd_filter: A function that takes a binary file path as argument and
                        returns whether the path matches the condition.

        Returns:
            A dict of {pid: command} where pid and command are strings.
        """
        result = self._shell.Execute("ps -Aw -o PID,COMMAND")
        asserts.assertEqual(result[const.EXIT_CODE][0], 0)
        lines = result[const.STDOUT][0].split("\n")
        pid_end = lines[0].index("PID") + len("PID")
        cmd_begin = lines[0].index("COMMAND", pid_end)
        cmds = {}
        for line in lines[1:]:
            cmd = line[cmd_begin:]
            if not cmd_filter(cmd):
                continue
            pid = line[:pid_end].lstrip()
            cmds[pid] = cmd
        return cmds

    def _ListOpenFiles(self, pids, file_filter):
        """Finds open files whose names match the filter.

        Args:
            pids: A collection of strings, the PIDs to list open files.
            file_filter: A function that takes a file path as argument and
                         returns whether the path matches the condition.

        Returns:
            A dict of {pid: [file, ...]} where pid and file are strings.
        """
        lsof_cmd = "lsof -p " + ",".join(pids)
        result = self._shell.Execute(lsof_cmd)
        asserts.assertEqual(result[const.EXIT_CODE][0], 0)
        lines = result[const.STDOUT][0].split("\n")
        pid_end = lines[0].index("PID") + len("PID")
        name_begin = lines[0].index("NAME")
        files = {}
        for line in lines[1:]:
            name = line[name_begin:]
            if not file_filter(name):
                continue
            pid_begin = line.rindex(" ", 0, pid_end) + 1
            pid = line[pid_begin:pid_end]
            if pid in files:
                files[pid].append(name)
            else:
                files[pid] = [name]
        return files

    def testVendorProcessOpenLibraries(self):
        """Checks if vendor processes load shared libraries on system."""
        asserts.skipIf(not vndk_utils.IsVndkRuntimeEnforced(self._dut),
                       "VNDK runtime is not enforced on the device.")
        vndk_lists = vndk_data.LoadVndkLibraryLists(
            self.data_file_path,
            self._dut.vndk_version,
            vndk_data.LL_NDK,
            vndk_data.LL_NDK_PRIVATE,
            vndk_data.VNDK,
            vndk_data.VNDK_PRIVATE,
            vndk_data.VNDK_SP,
            vndk_data.VNDK_SP_PRIVATE)
        asserts.assertTrue(vndk_lists, "Cannot load VNDK library lists.")
        allowed_libs = set()
        for vndk_list in vndk_lists:
            allowed_libs.update(vndk_list)
        logging.debug("Allowed system libraries: %s", allowed_libs)

        asserts.assertTrue(self._dut.isAdbRoot,
                           "Must be root to find all libraries in use.")
        cmds = self._ListProcessCommands(lambda x: (x.startswith("/odm/") or
                                                    x.startswith("/vendor/")))
        deps = self._ListOpenFiles(cmds.keys(),
                                   lambda x: (x.startswith("/system/") and
                                              x.endswith(".so") and
                                              x not in allowed_libs))
        if deps:
            error_lines = ["%s %s %s" % (pid, cmds[pid], libs)
                           for pid, libs in deps.iteritems()]
            logging.error("pid command libraries\n%s", "\n".join(error_lines))
            asserts.fail("Number of vendor processes using system libraries: " +
                         str(len(deps)))


if __name__ == "__main__":
    test_runner.main()
