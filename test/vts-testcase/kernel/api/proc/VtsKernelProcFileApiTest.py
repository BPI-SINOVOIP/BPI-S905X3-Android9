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
from vts.runners.host import test_runner

from vts.testcases.kernel.api.proc import ProcAsoundTests
from vts.testcases.kernel.api.proc import ProcCmdlineTest
from vts.testcases.kernel.api.proc import ProcCpuFileTests
from vts.testcases.kernel.api.proc import ProcFsFileTests
from vts.testcases.kernel.api.proc import ProcKmsgTest
from vts.testcases.kernel.api.proc import ProcMapsTest
from vts.testcases.kernel.api.proc import ProcMiscTest
from vts.testcases.kernel.api.proc import ProcMemInfoTest
from vts.testcases.kernel.api.proc import ProcModulesTest
from vts.testcases.kernel.api.proc import ProcQtaguidCtrlTest
from vts.testcases.kernel.api.proc import ProcRemoveUidRangeTest
from vts.testcases.kernel.api.proc import ProcSimpleFileTests
from vts.testcases.kernel.api.proc import ProcShowUidStatTest
from vts.testcases.kernel.api.proc import ProcStatTest
from vts.testcases.kernel.api.proc import ProcUidIoStatsTest
from vts.testcases.kernel.api.proc import ProcUidTimeInStateTest
from vts.testcases.kernel.api.proc import ProcUidCpuPowerTests
from vts.testcases.kernel.api.proc import ProcVersionTest
from vts.testcases.kernel.api.proc import ProcVmallocInfoTest
from vts.testcases.kernel.api.proc import ProcVmstatTest
from vts.testcases.kernel.api.proc import ProcZoneInfoTest

from vts.utils.python.controllers import android_device
from vts.utils.python.file import target_file_utils

TEST_OBJECTS = {
    ProcAsoundTests.ProcAsoundCardsTest(),
    ProcCmdlineTest.ProcCmdlineTest(),
    ProcCpuFileTests.ProcCpuInfoTest(),
    ProcCpuFileTests.ProcLoadavgTest(),
    ProcFsFileTests.ProcDiskstatsTest(),
    ProcFsFileTests.ProcFilesystemsTest(),
    ProcFsFileTests.ProcMountsTest(),
    ProcFsFileTests.ProcSwapsTest(),
    ProcKmsgTest.ProcKmsgTest(),
    ProcMapsTest.ProcMapsTest(),
    ProcMiscTest.ProcMisc(),
    ProcMemInfoTest.ProcMemInfoTest(),
    ProcModulesTest.ProcModulesTest(),
    ProcQtaguidCtrlTest.ProcQtaguidCtrlTest(),
    ProcRemoveUidRangeTest.ProcRemoveUidRangeTest(),
    ProcSimpleFileTests.ProcCorePattern(),
    ProcSimpleFileTests.ProcCorePipeLimit(),
    ProcSimpleFileTests.ProcDirtyBackgroundBytes(),
    ProcSimpleFileTests.ProcDirtyBackgroundRatio(),
    ProcSimpleFileTests.ProcDmesgRestrict(),
    ProcSimpleFileTests.ProcDomainname(),
    ProcSimpleFileTests.ProcDropCaches(),
    ProcSimpleFileTests.ProcExtraFreeKbytes(),
    ProcSimpleFileTests.ProcHostname(),
    ProcSimpleFileTests.ProcHungTaskTimeoutSecs(),
    ProcSimpleFileTests.ProcKptrRestrictTest(),
    ProcSimpleFileTests.ProcMaxMapCount(),
    ProcSimpleFileTests.ProcMmapMinAddrTest(),
    ProcSimpleFileTests.ProcMmapRndBitsTest(),
    ProcSimpleFileTests.ProcModulesDisabled(),
    ProcSimpleFileTests.ProcOverCommitMemoryTest(),
    ProcSimpleFileTests.ProcPageCluster(),
    ProcSimpleFileTests.ProcPanicOnOops(),
    ProcSimpleFileTests.ProcPerfEventMaxSampleRate(),
    ProcSimpleFileTests.ProcPerfEventParanoid(),
    ProcSimpleFileTests.ProcPidMax(),
    ProcSimpleFileTests.ProcPipeMaxSize(),
    ProcSimpleFileTests.ProcProtectedHardlinks(),
    ProcSimpleFileTests.ProcProtectedSymlinks(),
    ProcSimpleFileTests.ProcRandomizeVaSpaceTest(),
    ProcSimpleFileTests.ProcSchedChildRunsFirst(),
    ProcSimpleFileTests.ProcSchedLatencyNS(),
    ProcSimpleFileTests.ProcSchedRTPeriodUS(),
    ProcSimpleFileTests.ProcSchedRTRuntimeUS(),
    ProcSimpleFileTests.ProcSchedTunableScaling(),
    ProcSimpleFileTests.ProcSchedWakeupGranularityNS(),
    ProcShowUidStatTest.ProcShowUidStatTest(),
    ProcSimpleFileTests.ProcSuidDumpable(),
    ProcSimpleFileTests.ProcSysKernelRandomBootId(),
    ProcSimpleFileTests.ProcSysRqTest(),
    ProcSimpleFileTests.ProcUptime(),
    ProcStatTest.ProcStatTest(),
    ProcUidIoStatsTest.ProcUidIoStatsTest(),
    ProcUidTimeInStateTest.ProcUidTimeInStateTest(),
    ProcUidCpuPowerTests.ProcUidCpuPowerTimeInStateTest(),
    ProcUidCpuPowerTests.ProcUidCpuPowerConcurrentActiveTimeTest(),
    ProcUidCpuPowerTests.ProcUidCpuPowerConcurrentPolicyTimeTest(),
    ProcVersionTest.ProcVersionTest(),
    ProcVmallocInfoTest.ProcVmallocInfoTest(),
    ProcVmstatTest.ProcVmstat(),
    ProcZoneInfoTest.ProcZoneInfoTest(),
}

TEST_OBJECTS_64 = {
    ProcSimpleFileTests.ProcMmapRndCompatBitsTest(),
}


class VtsKernelProcFileApiTest(base_test.BaseTestClass):
    """Test cases which check content of proc files.

    Attributes:
        _PROC_SYS_ABI_SWP_FILE_PATH: the path of a file which decides behaviour of SWP instruction.
    """

    _PROC_SYS_ABI_SWP_FILE_PATH = "/proc/sys/abi/swp"

    def setUpClass(self):
        self.dut = self.android_devices[0]
        self.shell = self.dut.shell

    def runProcFileTest(self, test_object):
        """Reads from the file and checks that it parses and the content is valid.

        Args:
            test_object: inherits KernelProcFileTestBase, contains the test functions
        """
        asserts.skipIf(test_object in TEST_OBJECTS_64 and not self.dut.is64Bit,
                       "Skip test for 64-bit kernel.")
        filepath = test_object.get_path()
        asserts.skipIf(not target_file_utils.Exists(filepath, self.shell) and
                       test_object.file_optional(),
                       "%s does not exist and is optional." % filepath)
        target_file_utils.assertPermissionsAndExistence(
            self.shell, filepath, test_object.get_permission_checker())

        logging.info("Testing format of %s", filepath)

        asserts.assertTrue(
            test_object.prepare_test(self.shell, self.dut), "Setup failed!")

        if not test_object.test_format():
            return

        file_content = self.ReadFileContent(filepath)
        try:
            parse_result = test_object.parse_contents(file_content)
        except (SyntaxError, ValueError, IndexError) as e:
            asserts.fail("Failed to parse! " + str(e))
        asserts.assertTrue(
            test_object.result_correct(parse_result), "Results not valid!")

    def generateProcFileTests(self):
        """Run all proc file tests."""
        self.runGeneratedTests(
            test_func=self.runProcFileTest,
            settings=TEST_OBJECTS.union(TEST_OBJECTS_64),
            name_func=lambda test_obj: "test" + test_obj.__class__.__name__)

    def ReadFileContent(self, filepath):
        """Read the content of a file and perform assertions.

        Args:
            filepath: string, path to file

        Returns:
            string, content of file
        """
        cmd = "cat %s" % filepath
        results = self.shell.Execute(cmd)

        # checks the exit code
        asserts.assertEqual(
            results[const.EXIT_CODE][0], 0,
            "%s: Error happened while reading the file." % filepath)

        return results[const.STDOUT][0]

    def testProcPagetypeinfo(self):
        filepath = "/proc/pagetypeinfo"
        # Check that incident_helper can parse /proc/pagetypeinfo.
        result = self.shell.Execute("cat %s | incident_helper -s 2001" % filepath)
        asserts.assertEqual(
            result[const.EXIT_CODE][0], 0,
            "Failed to parse %s." % filepath)

    def testProcSysrqTrigger(self):
        filepath = "/proc/sysrq-trigger"

        # This command only performs a best effort attempt to remount all
        # filesystems. Check that it doesn't throw an error.
        self.dut.adb.shell("\"echo u > %s\"" % filepath)

        # Reboot the device.
        self.dut.adb.shell("\"echo b > %s\"" % filepath)
        asserts.assertFalse(self.dut.hasBooted(), "Device is still alive.")
        self.dut.waitForBootCompletion()
        self.dut.rootAdb()

        # Crash the system.
        self.dut.adb.shell("\"echo c > %s\"" % filepath)
        asserts.assertFalse(self.dut.hasBooted(), "Device is still alive.")
        self.dut.waitForBootCompletion()
        self.dut.rootAdb()

    def testProcUidProcstatSet(self):
        def UidIOStats(uid):
            """Returns I/O stats for a given uid.

            Args:
                uid, uid number.

            Returns:
                list of I/O numbers.
            """
            stats_path = "/proc/uid_io/stats"
            result = self.dut.adb.shell(
                    "\"cat %s | grep '^%d'\"" % (stats_path, uid),
                    no_except=True)
            return result[const.STDOUT].split()

        def CheckStatsInState(state):
            """Sets VTS (root uid) into a given state and checks the stats.

            Args:
                state, boolean. Use False for foreground,
                and True for background.
            """
            state = 1 if state else 0
            filepath = "/proc/uid_procstat/set"
            root_uid = 0

            # fg write chars are at index 2, and bg write chars are at 6.
            wchar_index = 6 if state else 2
            old_wchar = UidIOStats(root_uid)[wchar_index]
            self.dut.adb.shell("\"echo %d %s > %s\"" % (root_uid, state, filepath))
            # This should increase the number of write syscalls.
            self.dut.adb.shell("\"echo foo\"")
            asserts.assertLess(
                old_wchar,
                UidIOStats(root_uid)[wchar_index],
                "Number of write syscalls has not increased.")

        CheckStatsInState(False)
        CheckStatsInState(True)

    def testProcPerUidTimes(self):
        # TODO: make these files mandatory once they're in AOSP
        try:
            filepaths = self.dut.adb.shell("find /proc/uid -name time_in_state")
        except:
            asserts.skip("/proc/uid/ directory does not exist and is optional")

        asserts.skipIf(not filepaths,
                       "per-UID time_in_state files do not exist and are optional")

        filepaths = filepaths.splitlines()
        for filepath in filepaths:
            target_file_utils.assertPermissionsAndExistence(
                self.shell, filepath, target_file_utils.IsReadOnly
            )
            file_content = self.ReadFileContent(filepath)

    def testProcSysAbiSwpInstruction(self):
        """Tests /proc/sys/abi/swp.

        /proc/sys/abi/swp sets the execution behaviour for the obsoleted ARM instruction
        SWP. As per the setting in /proc/sys/abi/swp, the usage of SWP{B}
        can either generate an undefined instruction abort or use software emulation
        or hardware execution.
        """

        asserts.skipIf(not ("arm" in self.dut.cpu_abi and self.dut.is64Bit),
                       "file not present on non-ARM64 device")
        target_file_utils.assertPermissionsAndExistence(
            self.shell, self._PROC_SYS_ABI_SWP_FILE_PATH, target_file_utils.IsReadWrite)
        file_content = self.ReadFileContent(self._PROC_SYS_ABI_SWP_FILE_PATH)
        try:
            swp_state = int(file_content)
        except ValueError as e:
            asserts.fail("Failed to parse %s" % self._PROC_SYS_ABI_SWP_FILE_PATH)
        asserts.assertTrue(swp_state >= 0 and swp_state <= 2,
                           "%s contains incorrect value: %d" % (self._PROC_SYS_ABI_SWP_FILE_PATH,
                                                                swp_state))

if __name__ == "__main__":
    test_runner.main()
