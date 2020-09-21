# Copyright 2018 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Test the install module."""

import mock
import unittest

import common
from autotest_lib.site_utils.deployment import install
from autotest_lib.site_utils.stable_images import assign_stable_images


class AFEMock(object):
    """Mock frontend.AFE."""
    CROS_IMAGE_TYPE = 'cros'
    FIRMWARE_IMAGE_TYPE = 'firmware'

    def __init__(self, cros_version=None, fw_version=None):
        self.cros_version_map = mock.Mock()
        self.cros_version_map.get_version.return_value = cros_version
        self.fw_version_map = mock.Mock()
        self.fw_version_map.get_version.return_value = fw_version

    def get_stable_version_map(self, image_type):
        if image_type == self.CROS_IMAGE_TYPE:
            return self.cros_version_map
        elif image_type == self.FIRMWARE_IMAGE_TYPE:
             return self.fw_version_map


class UpdateBuildTests(unittest.TestCase):
    """Tests for _update_build."""

    OMAHA_VERSION = 'R64-10176.65.0'
    CROS_VERSION = 'R64-10175.65.0'

    WOLF_BOARD = 'wolf'
    WOLF_FW_VERSION = 'Google_Wolf.4389.24.62'
    WOLF_NEW_FW_VERSION = 'Google_Wolf.4390.24.62'
    WOLF_FW_VERSION_MAP = {WOLF_BOARD: WOLF_NEW_FW_VERSION}

    CORAL_BOARD = 'coral'
    CORAL_FW_VERSION = 'Google_Coral.10068.37.0'
    CORAL_FW_VERSION_MAP = {
            'blue': 'Google_Coral.10068.39.0',
            'robo360': 'Google_Coral.10068.34.0',
            'porbeagle': None,
    }

    def setUp(self):
        self.report_log_mock = mock.Mock()
        self.patchers = []

    def _set_patchers(self,
                      omaha_version=OMAHA_VERSION,
                      firmware_versions=WOLF_FW_VERSION_MAP):
        patcher1 = mock.patch.object(
                install, '_get_omaha_build',
                return_value=omaha_version)
        patcher2 = mock.patch.object(
                assign_stable_images, 'get_firmware_versions',
                return_value=firmware_versions)

        self.patchers.extend([patcher1, patcher2])

        for p in self.patchers:
            p.start()

    def tearDown(self):
        for p in self.patchers:
            p.stop()

    def test_update_build_cros_and_fw_version_on_non_unibuild(self):
        """Update non-unibuild with old cros_version and fw_version in AFE."""
        afe_mock = AFEMock(
                cros_version=self.CROS_VERSION, fw_version=self.WOLF_FW_VERSION)

        self._set_patchers()
        arguments = mock.Mock(board=self.WOLF_BOARD, nostable=False, build=None)
        cros_version = install._update_build(
                afe_mock, self.report_log_mock, arguments)

        afe_mock.cros_version_map.set_version.assert_called_once_with(
                self.WOLF_BOARD, self.OMAHA_VERSION)
        afe_mock.fw_version_map.set_version.assert_called_once_with(
                self.WOLF_BOARD, self.WOLF_NEW_FW_VERSION)
        self.assertEqual(cros_version, self.OMAHA_VERSION)

    def test_update_build_without_omaha_version_on_non_unibuild(self):
        """Do not update non-unibuild as no OMAHA_VERSION found."""
        afe_mock = AFEMock(
                cros_version=self.CROS_VERSION, fw_version=self.WOLF_FW_VERSION)

        self._set_patchers(omaha_version=None)
        arguments = mock.Mock(board=self.WOLF_BOARD, nostable=False, build=None)
        cros_version = install._update_build(
                afe_mock, self.report_log_mock, arguments)

        afe_mock.cros_version_map.set_version.assert_not_called()
        afe_mock.cros_version_map.delete_version.assert_not_called()
        afe_mock.fw_version_map.set_version.assert_not_called()
        afe_mock.fw_version_map.delete_version.assert_not_called()
        self.assertEqual(cros_version, self.CROS_VERSION)

    def test_update_build_cros_on_non_unibuild(self):
        """Update non-unibuild with old cros_version in AFE."""
        afe_mock = AFEMock(
                cros_version=self.CROS_VERSION, fw_version=self.WOLF_FW_VERSION)
        self._set_patchers(
                firmware_versions={self.WOLF_BOARD: self.WOLF_FW_VERSION})
        arguments = mock.Mock(board=self.WOLF_BOARD, nostable=False, build=None)
        cros_version = install._update_build(
                afe_mock, self.report_log_mock, arguments)

        afe_mock.cros_version_map.set_version.assert_called_once_with(
                self.WOLF_BOARD, self.OMAHA_VERSION)
        afe_mock.fw_version_map.set_version.assert_not_called()
        afe_mock.fw_version_map.delete_version.assert_not_called()
        self.assertEqual(cros_version, self.OMAHA_VERSION)

    def test_update_build_none_cros_and_fw_version_on_non_unibuild(self):
        """Update Non-unibuild with None cros_version & fw_version in AFE."""
        afe_mock = AFEMock(cros_version=None, fw_version=None)
        self._set_patchers()
        arguments = mock.Mock(board=self.WOLF_BOARD, nostable=False, build=None)
        cros_version = install._update_build(
                afe_mock, self.report_log_mock, arguments)

        afe_mock.cros_version_map.set_version.assert_called_once_with(
                self.WOLF_BOARD, self.OMAHA_VERSION)
        afe_mock.fw_version_map.set_version.assert_called_once_with(
                self.WOLF_BOARD, self.WOLF_NEW_FW_VERSION)
        self.assertEqual(cros_version, self.OMAHA_VERSION)

    def test_update_build_cros_and_fw_version_on_unibuild(self):
        """Update unibuild with old cros_version and fw_versions."""
        afe_mock = AFEMock(
                cros_version=self.CROS_VERSION,
                fw_version=self.CORAL_FW_VERSION)
        self._set_patchers(
                firmware_versions=self.CORAL_FW_VERSION_MAP)
        arguments = mock.Mock(board=self.CORAL_BOARD, nostable=False,
                              build=None)
        cros_version = install._update_build(
                afe_mock, self.report_log_mock, arguments)

        afe_mock.cros_version_map.set_version.assert_called_once_with(
                self.CORAL_BOARD, self.OMAHA_VERSION)
        afe_mock.fw_version_map.set_version.assert_any_call(
                'blue', 'Google_Coral.10068.39.0')
        afe_mock.fw_version_map.set_version.assert_any_call(
                'robo360', 'Google_Coral.10068.34.0')
        afe_mock.fw_version_map.delete_version.assert_any_call(
                'porbeagle')
        self.assertEqual(cros_version, self.OMAHA_VERSION)


if __name__ == '__main__':
    unittest.main()
