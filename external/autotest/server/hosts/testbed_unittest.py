#!/usr/bin/python
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import mox
import unittest

import common
from autotest_lib.client.common_lib import error
from autotest_lib.server.hosts import testbed


BOARD_1 = 'board1'
BOARD_1_BUILD_1 = 'branch1/board1-userdebug/1'
BOARD_1_BUILD_2 = 'branch1/board1-userdebug/2'
BOARD_2 = 'board2'
BOARD_2_BUILD_1 = 'branch1/board2-userdebug/1'


class TestBedUnittests(mox.MoxTestBase):
    """Tests for TestBed."""

    def testLocateDeviceSuccess_SingleBuild(self):
        """Test locate_device call can allocate devices by given builds.
        """
        serials = ['s1', 's2', 's3']
        testbed_1 = testbed.TestBed(adb_serials=serials)
        hosts = [self.mox.CreateMockAnything(),
                 self.mox.CreateMockAnything(),
                 self.mox.CreateMockAnything()]
        for host in hosts:
            self.mox.StubOutWithMock(host, 'get_device_aliases')
            host.get_device_aliases().MultipleTimes().AndReturn([BOARD_1])
        self.mox.StubOutWithMock(testbed_1, 'get_adb_devices')
        testbed_1.get_adb_devices().AndReturn(dict(zip(serials, hosts)))
        images = [(BOARD_1_BUILD_1, None)]*3
        self.mox.ReplayAll()

        devices = testbed_1.locate_devices(images)
        self.assertEqual(devices, dict(zip(serials, [BOARD_1_BUILD_1]*3)))


    def testLocateDeviceFail_MixedBuild(self):
        """Test locate_device call cannot allocate devices by given builds.

        If the given builds are not the same and the number of duts required is
        less than the number of devices the testbed has, it should fail to
        locate devices for the test.
        """
        serials = ['s1', 's2', 's3']
        testbed_1 = testbed.TestBed(adb_serials=serials)
        hosts = [self.mox.CreateMockAnything(),
                 self.mox.CreateMockAnything(),
                 self.mox.CreateMockAnything()]
        for host in hosts:
            self.mox.StubOutWithMock(host, 'get_device_aliases')
            host.get_device_aliases().MultipleTimes().AndReturn([BOARD_1])
        self.mox.StubOutWithMock(testbed_1, 'get_adb_devices')
        testbed_1.get_adb_devices().AndReturn(dict(zip(serials, hosts)))
        images = [(BOARD_1_BUILD_1, None), (BOARD_1_BUILD_2, None)]
        self.mox.ReplayAll()

        self.assertRaises(error.InstallError, testbed_1.locate_devices, images)


    def testLocateDeviceFail_TooManyBuilds(self):
        """Test locate_device call cannot allocate devices by given builds.

        If the given builds are more than the number of devices the testbed has,
        it should fail to locate devices for the test.
        """
        serials = ['s1', 's2', 's3']
        testbed_1 = testbed.TestBed(adb_serials=serials)
        hosts = [self.mox.CreateMockAnything(),
                 self.mox.CreateMockAnything(),
                 self.mox.CreateMockAnything()]
        for host in hosts:
            self.mox.StubOutWithMock(host, 'get_device_aliases')
            host.get_device_aliases().MultipleTimes().AndReturn([BOARD_1])
        self.mox.StubOutWithMock(testbed_1, 'get_adb_devices')
        testbed_1.get_adb_devices().AndReturn(dict(zip(serials, hosts)))
        # Request 4 images but the testbed has only 3 duts.
        images = [(BOARD_1_BUILD_1, None)]*4
        self.mox.ReplayAll()

        self.assertRaises(error.InstallError, testbed_1.locate_devices, images)


    def testLocateDeviceSuccess_MixedBuildsSingleBoard(self):
        """Test locate_device call can allocate devices by given builds.

        If the given builds are the same and the number of duts required is
        less than the number of devices the testbed has, it should return all
        devices with the same build.
        """
        serials = ['s1', 's2', 's3']
        testbed_1 = testbed.TestBed(adb_serials=serials)
        hosts = [self.mox.CreateMockAnything(),
                 self.mox.CreateMockAnything(),
                 self.mox.CreateMockAnything()]
        for host in hosts:
            self.mox.StubOutWithMock(host, 'get_device_aliases')
            host.get_device_aliases().MultipleTimes().AndReturn([BOARD_1])
        self.mox.StubOutWithMock(testbed_1, 'get_adb_devices')
        testbed_1.get_adb_devices().AndReturn(dict(zip(serials, hosts)))
        images = [(BOARD_1_BUILD_1, None), (BOARD_1_BUILD_1, None)]
        self.mox.ReplayAll()

        devices = testbed_1.locate_devices(images)
        self.assertEqual(devices, dict(zip(serials, [BOARD_1_BUILD_1]*3)))


    def testLocateDeviceSuccess_MixedBuildsMultiBoards(self):
        """Test locate_device call can allocate devices by given builds for
        multiple boards.
        """
        serials = ['s1', 's2', 's3', 's4']
        testbed_1 = testbed.TestBed(adb_serials=serials)
        hosts = [self.mox.CreateMockAnything(),
                 self.mox.CreateMockAnything(),
                 self.mox.CreateMockAnything(),
                 self.mox.CreateMockAnything()]
        for i in [0, 1]:
            self.mox.StubOutWithMock(hosts[i], 'get_device_aliases')
            hosts[i].get_device_aliases().MultipleTimes().AndReturn([BOARD_1])
        for i in [2, 3]:
            self.mox.StubOutWithMock(hosts[i], 'get_device_aliases')
            hosts[i].get_device_aliases().MultipleTimes().AndReturn([BOARD_2])
        self.mox.StubOutWithMock(testbed_1, 'get_adb_devices')
        testbed_1.get_adb_devices().AndReturn(dict(zip(serials, hosts)))
        images = [(BOARD_1_BUILD_1, None), (BOARD_1_BUILD_1, None),
                  (BOARD_2_BUILD_1, None), (BOARD_2_BUILD_1, None)]
        self.mox.ReplayAll()

        devices = testbed_1.locate_devices(images)
        expected = dict(zip(serials[0:2], [BOARD_1_BUILD_1]*2))
        expected.update(dict(zip(serials[2:], [BOARD_2_BUILD_1]*2)))
        self.assertEqual(devices, expected)


if __name__ == "__main__":
    unittest.main()
