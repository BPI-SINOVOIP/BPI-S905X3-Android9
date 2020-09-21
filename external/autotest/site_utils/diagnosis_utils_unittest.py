#!/usr/bin/env python
# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest
import mock

import common
from autotest_lib.site_utils import diagnosis_utils
from autotest_lib.server import frontend


class DiagnosisUtilsTest(unittest.TestCase):
    """Tests for diagnosis_utils."""

    def setUp(self):
        """Set up test."""
        self.afe_mock = mock.MagicMock()

    def _constructRPCHelper(self):
        """Method to construct RPCHelper"""
        return diagnosis_utils.RPCHelper(self.afe_mock)

    def _mockZeroHost(self):
        """Mock method to return zero host"""
        return ()

    def _mockTwoAvailableHosts(self):
        """Mock method to return two available hosts"""
        host_a = _StubHost(status='Ready', locked=False)
        host_b = _StubHost(status='Ready', locked=False)
        return (host_a, host_b)

    def _mockTwoFailedHosts(self):
        """Mock method to return two unavailable hosts"""
        host_a = _StubHost(status='Repair Failed', locked=False)
        host_b = _StubHost(status='Repairing', locked=False)
        return (host_a, host_b)

    def testCheckDutAvailable(self):
        """Test check_dut_availability with different scenarios"""
        rpc_helper = self._constructRPCHelper()
        board = 'test_board'
        pool = 'test_pool'

        # Mock aef.get_hosts to return 0 host
        self.afe_mock.get_hosts.return_value = self._mockZeroHost()
        skip_duts_check = False

        # When minimum_duts is 0, do not force available DUTs
        minimum_duts = 0
        rpc_helper.check_dut_availability(board, pool,
                                          minimum_duts=minimum_duts,
                                          skip_duts_check=skip_duts_check)

        # When skip_duts_check = False and minimum_duts > 0 and there's no host,
        # should raise BoardNotAvailableError
        minimum_duts = 1
        with self.assertRaises(diagnosis_utils.BoardNotAvailableError):
            rpc_helper.check_dut_availability(board, pool,
                                              minimum_duts=minimum_duts,
                                              skip_duts_check=skip_duts_check)

        # Mock aef.get_hosts to return 2 available hosts
        self.afe_mock.get_hosts.return_value = self._mockTwoAvailableHosts()

        # Set skip_duts_check to True, should not force checking avialble DUTs
        # although available DUTs are less then minimum_duts
        minimum_duts = 4
        skip_duts_check = True
        rpc_helper.check_dut_availability(board, pool,
                                          minimum_duts=minimum_duts,
                                          skip_duts_check=skip_duts_check)

        # Set skip_duts_check to False and current available DUTs are less
        # then minimum_duts, should raise NotEnoughDutsError
        skip_duts_check = False
        with self.assertRaises(diagnosis_utils.NotEnoughDutsError):
            rpc_helper.check_dut_availability(board, pool,
                                              minimum_duts=minimum_duts,
                                              skip_duts_check=skip_duts_check)

        # When skip_duts_check is False and current available DUTs
        # satisfy minimum_duts, no errors
        minimum_duts = 2
        rpc_helper.check_dut_availability(board, pool,
                                          minimum_duts=minimum_duts,
                                          skip_duts_check=skip_duts_check)

        # Mock aef.get_hosts to return 2 failed hosts
        self.afe_mock.get_hosts.return_value = self._mockTwoFailedHosts()

        # When skip_duts_check is False and the two hosts are not available,
        # should raise NotEnoughDutsError
        with self.assertRaises(diagnosis_utils.NotEnoughDutsError):
            rpc_helper.check_dut_availability(board, pool,
                                              minimum_duts=minimum_duts,
                                              skip_duts_check=skip_duts_check)


class _StubHost(object):

    def __init__(self, status, locked):
        self.status = status
        self.locked = locked

    is_available = frontend.Host.is_available.im_func


if __name__ == '__main__':
    unittest.main()
