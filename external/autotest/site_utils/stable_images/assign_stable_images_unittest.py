# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Unit tests for functions in `assign_stable_images`.
"""


import json
import mock
import os
import sys
import unittest

import common
from autotest_lib.server import frontend
from autotest_lib.site_utils.stable_images import assign_stable_images


# _OMAHA_TEST_DATA - File with JSON data to be used as test input to
#   `_make_omaha_versions()`.  In the file, the various items in the
#   `omaha_data` list are selected to capture various specific test
#   cases:
#     + Board with no "beta" channel.
#     + Board with "beta" and another channel.
#     + Board with only a "beta" channel.
#     + Board with no "chrome_version" entry.
#     + Obsolete board with "is_active" set to false.
# The JSON content of the file is a subset of an actual
# `omaha_status.json` file copied when the unit test was last
# updated.
#
# _EXPECTED_OMAHA_VERSIONS - The expected output produced by
#   _STUB_OMAHA_DATA.
#
_OMAHA_TEST_DATA = 'test_omaha_status.json'

_EXPECTED_OMAHA_VERSIONS = {'auron-paine': 'R55-8872.54.0',
                            'gale': 'R55-8872.40.9',
                            'kevin': 'R55-8872.64.0',
                            'zako-freon': 'R41-6680.52.0'}

_DEFAULT_BOARD = assign_stable_images._DEFAULT_BOARD


class OmahaDataTests(unittest.TestCase):
    """Tests for the `_make_omaha_versions()` function."""

    def test_make_omaha_versions(self):
        """
        Test `_make_omaha_versions()` against one simple input.

        This is a trivial sanity test that confirms that a single
        hard-coded input returns a correct hard-coded output.
        """
        module_dir = os.path.dirname(sys.modules[__name__].__file__)
        data_file_path = os.path.join(module_dir, _OMAHA_TEST_DATA)
        omaha_versions = assign_stable_images._make_omaha_versions(
                json.load(open(data_file_path, 'r')))
        self.assertEqual(omaha_versions, _EXPECTED_OMAHA_VERSIONS)


class KeyPathTests(unittest.TestCase):
    """Tests for the `_get_by_key_path()` function."""

    DICTDICT = {'level0': 'OK', 'level1_a': {'level1_b': 'OK'}}

    def _get_by_key_path(self, keypath):
        get_by_key_path = assign_stable_images._get_by_key_path
        return get_by_key_path(self.DICTDICT, keypath)

    def _check_path_valid(self, keypath):
        self.assertEqual(self._get_by_key_path(keypath), 'OK')

    def _check_path_invalid(self, keypath):
        self.assertIsNone(self._get_by_key_path(keypath))

    def test_one_element(self):
        """Test a single-element key path with a valid key."""
        self._check_path_valid(['level0'])

    def test_two_element(self):
        """Test a two-element key path with a valid key."""
        self._check_path_valid(['level1_a', 'level1_b'])

    def test_one_element_invalid(self):
        """Test a single-element key path with an invalid key."""
        self._check_path_invalid(['absent'])

    def test_two_element_invalid(self):
        """Test a two-element key path with an invalid key."""
        self._check_path_invalid(['level1_a', 'absent'])


class GetFirmwareUpgradesTests(unittest.TestCase):
    """Tests for _get_firmware_upgrades."""


    def setUp(self):
        self.version_map = frontend._CrosVersionMap(mock.Mock())


    @mock.patch.object(assign_stable_images, 'get_firmware_versions')
    def test_get_firmware_upgrades(self, mock_get_firmware_versions):
        """Test _get_firmware_upgrades."""
        mock_get_firmware_versions.side_effect = [
                {'auron_paine': 'fw_version'},
                {'blue': 'fw_version',
                 'robo360': 'fw_version',
                 'porbeagle': 'fw_version'}
        ]
        cros_versions = {
                'coral': 'R64-10176.65.0',
                'auron_paine': 'R64-10176.65.0'
        }
        boards = ['auron_paine', 'coral']

        firmware_upgrades = assign_stable_images._get_firmware_upgrades(
            self.version_map, cros_versions)
        expected_firmware_upgrades = {
                'auron_paine': 'fw_version',
                'blue': 'fw_version',
                'robo360': 'fw_version',
                'porbeagle': 'fw_version'
        }
        self.assertEqual(firmware_upgrades, expected_firmware_upgrades)


class GetFirmwareVersionsTests(unittest.TestCase):
    """Tests for get_firmware_versions."""


    def setUp(self):
        self.version_map = frontend._CrosVersionMap(mock.Mock())
        self.cros_version = 'R64-10176.65.0'


    @mock.patch.object(assign_stable_images, '_read_gs_json_data')
    def test_get_firmware_versions_on_normal_build(self, mock_read_gs):
        """Test get_firmware_versions on normal build."""
        metadata_json = """
{
    "unibuild": false,
    "board-metadata":{
        "auron_paine":{
             "main-firmware-version":"Google_Auron_paine.6301.58.98"
        }
   }
}
        """
        mock_read_gs.return_value = json.loads(metadata_json)
        board = 'auron_paine'

        fw_version = assign_stable_images.get_firmware_versions(
                self.version_map, board, self.cros_version)
        expected_version = {board: "Google_Auron_paine.6301.58.98"}
        self.assertEqual(fw_version, expected_version)


    @mock.patch.object(assign_stable_images, '_read_gs_json_data',
                       side_effect = Exception('GS ERROR'))
    def test_get_firmware_versions_with_exceptions(self, mock_read_gs):
        """Test get_firmware_versions on normal build with exceptions."""
        afe_mock = mock.Mock()
        version_map = frontend._CrosVersionMap(afe_mock)

        fw_version = assign_stable_images.get_firmware_versions(
                self.version_map, 'auron_paine', self.cros_version)
        self.assertEqual(fw_version, {'auron_paine': None})


    @mock.patch.object(assign_stable_images, '_read_gs_json_data')
    def test_get_firmware_versions_on_unibuild(self, mock_read_gs):
        """Test get_firmware_version on uni-build."""
        metadata_json = """
{
    "unibuild": true,
    "board-metadata":{
        "coral":{
            "kernel-version":"4.4.114-r1354",
            "models":{
                "blue":{
                    "main-readonly-firmware-version":"Google_Coral.10068.37.0",
                    "main-readwrite-firmware-version":"Google_Coral.10068.39.0"
                },
                "robo360":{
                    "main-readonly-firmware-version":"Google_Coral.10068.34.0",
                    "main-readwrite-firmware-version":null
                },
                "porbeagle":{
                    "main-readonly-firmware-version":null,
                    "main-readwrite-firmware-version":null
                }
            }
        }
    }
}
"""
        mock_read_gs.return_value = json.loads(metadata_json)

        fw_version = assign_stable_images.get_firmware_versions(
                self.version_map, 'coral', self.cros_version)
        expected_version = {
                'blue': 'Google_Coral.10068.39.0',
                'robo360': 'Google_Coral.10068.34.0',
                'porbeagle': None
        }
        self.assertEqual(fw_version, expected_version)


    @mock.patch.object(assign_stable_images, '_read_gs_json_data')
    def test_get_firmware_versions_on_unibuild_no_models(self, mock_read_gs):
        """Test get_firmware_versions on uni-build without models dict."""
        metadata_json = """
{
    "unibuild": true,
    "board-metadata":{
        "coral":{
            "kernel-version":"4.4.114-r1354"
        }
    }
}
"""
        mock_read_gs.return_value = json.loads(metadata_json)

        fw_version = assign_stable_images.get_firmware_versions(
                self.version_map, 'coral', self.cros_version)
        self.assertEqual(fw_version, {'coral': None})


class GetUpgradeTests(unittest.TestCase):
    """Tests for the `_get_upgrade_versions()` function."""

    # _VERSIONS - a list of sample version strings such as may be used
    #   for Chrome OS, sorted from oldest to newest.  These are used to
    #   construct test data in multiple test cases, below.
    _VERSIONS = ['R1-1.0.0', 'R1-1.1.0', 'R2-4.0.0']

    def test_board_conversions(self):
        """
        Test proper mapping of names from the AFE to Omaha.

        Board names in Omaha don't have '_' characters; when an AFE
        board contains '_' characters, they must be converted to '-'.

        Assert that for various forms of name in the AFE mapping, the
        converted name is the one looked up in the Omaha mapping.
        """
        board_equivalents = [
            ('a-b', 'a-b'), ('c_d', 'c-d'),
            ('e_f-g', 'e-f-g'), ('hi', 'hi')]
        afe_versions = {
            _DEFAULT_BOARD: self._VERSIONS[0]
        }
        omaha_versions = {}
        expected = {}
        boards = set()
        for afe_board, omaha_board in board_equivalents:
            boards.add(afe_board)
            afe_versions[afe_board] = self._VERSIONS[1]
            omaha_versions[omaha_board] = self._VERSIONS[2]
            expected[afe_board] = self._VERSIONS[2]
        upgrades, _ = assign_stable_images._get_upgrade_versions(
                afe_versions, omaha_versions, boards)
        self.assertEqual(upgrades, expected)

    def test_afe_default(self):
        """
        Test that the AFE default board mapping is honored.

        If a board isn't present in the AFE dictionary, the mapping
        for `_DEFAULT_BOARD` should be used.

        Primary assertions:
          * When a board is present in the AFE mapping, its version
            mapping is used.
          * When a board is not present in the AFE mapping, the default
            version mapping is used.

        Secondarily, assert that when a mapping is absent from Omaha,
        the AFE mapping is left unchanged.
        """
        afe_versions = {
            _DEFAULT_BOARD: self._VERSIONS[0],
            'a': self._VERSIONS[1]
        }
        boards = set(['a', 'b'])
        expected = {
            'a': self._VERSIONS[1],
            'b': self._VERSIONS[0]
        }
        upgrades, _ = assign_stable_images._get_upgrade_versions(
                afe_versions, {}, boards)
        self.assertEqual(upgrades, expected)

    def test_omaha_upgrade(self):
        """
        Test that upgrades from Omaha are detected.

        Primary assertion:
          * If a board is found in Omaha, and the version in Omaha is
            newer than the AFE version, the Omaha version is the one
            used.

        Secondarily, asserts that version comparisons between various
        specific version strings are all correct.
        """
        boards = set(['a'])
        for i in range(0, len(self._VERSIONS) - 1):
            afe_versions = {_DEFAULT_BOARD: self._VERSIONS[i]}
            for j in range(i+1, len(self._VERSIONS)):
                omaha_versions = {b: self._VERSIONS[j] for b in boards}
                upgrades, _ = assign_stable_images._get_upgrade_versions(
                        afe_versions, omaha_versions, boards)
                self.assertEqual(upgrades, omaha_versions)

    def test_no_upgrade(self):
        """
        Test that if Omaha is behind the AFE, it is ignored.

        Primary assertion:
          * If a board is found in Omaha, and the version in Omaha is
            older than the AFE version, the AFE version is the one used.

        Secondarily, asserts that version comparisons between various
        specific version strings are all correct.
        """
        boards = set(['a'])
        for i in range(1, len(self._VERSIONS)):
            afe_versions = {_DEFAULT_BOARD: self._VERSIONS[i]}
            expected = {b: self._VERSIONS[i] for b in boards}
            for j in range(0, i):
                omaha_versions = {b: self._VERSIONS[j] for b in boards}
                upgrades, _ = assign_stable_images._get_upgrade_versions(
                        afe_versions, omaha_versions, boards)
                self.assertEqual(upgrades, expected)

    def test_ignore_unused_boards(self):
        """
        Test that unlisted boards are ignored.

        Assert that boards present in the AFE or Omaha mappings aren't
        included in the return mappings when they aren't in the passed
        in set of boards.
        """
        unused_boards = set(['a', 'b'])
        used_boards = set(['c', 'd'])
        afe_versions = {b: self._VERSIONS[0] for b in unused_boards}
        afe_versions[_DEFAULT_BOARD] = self._VERSIONS[1]
        expected = {b: self._VERSIONS[1] for b in used_boards}
        omaha_versions = expected.copy()
        omaha_versions.update(
                {b: self._VERSIONS[0] for b in unused_boards})
        upgrades, _ = assign_stable_images._get_upgrade_versions(
                afe_versions, omaha_versions, used_boards)
        self.assertEqual(upgrades, expected)

    def test_default_unchanged(self):
        """
        Test correct handling when the default build is unchanged.

        Assert that if in Omaha, one board in a set of three upgrades
        from the AFE default, that the returned default board mapping is
        the original default in the AFE.
        """
        boards = set(['a', 'b', 'c'])
        afe_versions = {_DEFAULT_BOARD: self._VERSIONS[0]}
        omaha_versions = {b: self._VERSIONS[0] for b in boards}
        omaha_versions['c'] = self._VERSIONS[1]
        _, new_default = assign_stable_images._get_upgrade_versions(
                afe_versions, omaha_versions, boards)
        self.assertEqual(new_default, self._VERSIONS[0])

    def test_default_upgrade(self):
        """
        Test correct handling when the default build must change.

        Assert that if in Omaha, two boards in a set of three upgrade
        from the AFE default, that the returned default board mapping is
        the new build in Omaha.
        """
        boards = set(['a', 'b', 'c'])
        afe_versions = {_DEFAULT_BOARD: self._VERSIONS[0]}
        omaha_versions = {b: self._VERSIONS[1] for b in boards}
        omaha_versions['c'] = self._VERSIONS[0]
        _, new_default = assign_stable_images._get_upgrade_versions(
                afe_versions, omaha_versions, boards)
        self.assertEqual(new_default, self._VERSIONS[1])


# Sample version string values to be used when testing
# `_apply_upgrades()`.
#
# _OLD_DEFAULT - Test value representing the default version mapping
#   in the `old_versions` dictionary in a call to `_apply_upgrades()`.
# _NEW_DEFAULT - Test value representing the default version mapping
#   in the `new_versions` dictionary when a version update is being
#   tested.
# _OLD_VERSION - Test value representing an arbitrary version for a
#   board that is mapped in the `old_versions` dictionary in a call to
#   `_apply_upgrades()`.
# _NEW_VERSION - Test value representing an arbitrary version for a
#   board that is mapped in the `new_versions` dictionary in a call to
#   `_apply_upgrades()`.
#
_OLD_DEFAULT = 'old-default-version'
_NEW_DEFAULT = 'new-default-version'
_OLD_VERSION = 'old-board-version'
_NEW_VERSION = 'new-board-version'


class _StubAFE(object):
    """Stubbed out version of `server.frontend.AFE`."""

    CROS_IMAGE_TYPE = 'cros-image-type'
    FIRMWARE_IMAGE_TYPE = 'firmware-image-type'

    def get_stable_version_map(self, image_type):
        return image_type


class _TestUpdater(assign_stable_images._VersionUpdater):
    """
    Subclass of `_VersionUpdater` for testing.

    This class extends `_VersionUpdater` to provide support for testing
    various assertions about the behavior of the base class and its
    interactions with `_apply_cros_upgrades()` and
    `_apply_firmware_upgrades()`.

    The class tests assertions along the following lines:
      * When applied to the original mappings, the calls to
        `_do_set_mapping()` and `_do_delete_mapping()` create the
        expected final mapping state.
      * Calls to report state changes are made with the expected
        values.
      * There's a one-to-one match between reported and actually
        executed changes.

    """

    def __init__(self, testcase):
        super(_TestUpdater, self).__init__(_StubAFE())
        self._testcase = testcase
        self._default_changed = None
        self._reported_mappings = None
        self._updated_mappings = None
        self._reported_deletions = None
        self._actual_deletions = None
        self._original_mappings = None
        self._mappings = None
        self._expected_mappings = None
        self._unchanged_boards = None

    def pretest_init(self, initial_versions, expected_versions):
        """
        Initialize for testing.

        @param initial_versions   Mappings to be used as the starting
                                  point for testing.
        @param expected_versions  The expected final value of the
                                  mappings after the test.
        """
        self._default_changed = False
        self._reported_mappings = {}
        self._updated_mappings = {}
        self._reported_deletions = set()
        self._actual_deletions = set()
        self._original_mappings = initial_versions.copy()
        self._mappings = initial_versions.copy()
        self._expected_mappings = expected_versions
        self._unchanged_boards = set()

    def check_results(self, change_default):
        """
        Assert that observed changes match expectations.

        Asserts the following:
          * The `report_default_changed()` method was called (or not)
            based on whether `change_default` is true (or not).
          * The changes reported via `_report_board_changed()` match
            the changes actually applied.
          * The final mappings after applying requested changes match
            the actually expected mappings.

        @param old_versions   Parameter to be passed to
                              `_apply_cros_upgrades()`.
        @param new_versions   Parameter to be passed to
                              `_apply_cros_upgrades()`.
        @param change_default   Whether the test should include a change
                                to the default version mapping.
        """
        self._testcase.assertEqual(change_default,
                                   self._default_changed)
        self._testcase.assertEqual(self._reported_mappings,
                                   self._updated_mappings)
        self._testcase.assertEqual(self._reported_deletions,
                                   self._actual_deletions)
        self._testcase.assertEqual(self._mappings,
                                   self._expected_mappings)

    def report(self, message):
        """Report message."""
        pass

    def report_default_changed(self, old_default, new_default):
        """
        Override of our parent class' method for test purposes.

        Saves a record of the report for testing the final result in
        `apply_upgrades()`, above.

        Assert the following:
          * The old and new default values match the values that
            were passed in the original call's arguments.
          * This function is not being called for a second time.

        @param old_default  The original default version.
        @param new_default  The new default version to be applied.
        """
        self._testcase.assertNotEqual(old_default, new_default)
        self._testcase.assertEqual(old_default,
                                   self._original_mappings[_DEFAULT_BOARD])
        self._testcase.assertEqual(new_default,
                                   self._expected_mappings[_DEFAULT_BOARD])
        self._testcase.assertFalse(self._default_changed)
        self._default_changed = True
        self._reported_mappings[_DEFAULT_BOARD] = new_default

    def _report_board_changed(self, board, old_version, new_version):
        """
        Override of our parent class' method for test purposes.

        Saves a record of the report for testing the final result in
        `apply_upgrades()`, above.

        Assert the following:
          * The change being reported actually reports two different
            versions.
          * If the board isn't mapped to the default version, then the
            reported old version is the actually mapped old version.
          * If the board isn't changing to the default version, then the
            reported new version is the expected new version.
          * This is not a second report for this board.

        The implementation implicitly requires that the specified board
        have a valid mapping.

        @param board        The board with the changing version.
        @param old_version  The original version mapped to the board.
        @param new_version  The new version to be applied to the board.
        """
        self._testcase.assertNotEqual(old_version, new_version)
        if board in self._original_mappings:
            self._testcase.assertEqual(old_version,
                                       self._original_mappings[board])
        if board in self._expected_mappings:
            self._testcase.assertEqual(new_version,
                                       self._expected_mappings[board])
            self._testcase.assertNotIn(board, self._reported_mappings)
            self._reported_mappings[board] = new_version
        else:
            self._testcase.assertNotIn(board, self._reported_deletions)
            self._reported_deletions.add(board)

    def report_board_unchanged(self, board, old_version):
        """
        Override of our parent class' method for test purposes.

        Assert the following:
          * The version being reported as unchanged is actually mapped.
          * The reported old version matches the expected value.
          * This is not a second report for this board.

        @param board        The board that is not changing.
        @param old_version  The board's version mapping.
        """
        self._testcase.assertIn(board, self._original_mappings)
        self._testcase.assertEqual(old_version,
                                   self._original_mappings[board])
        self._testcase.assertNotIn(board, self._unchanged_boards)
        self._unchanged_boards.add(board)

    def _do_set_mapping(self, board, new_version):
        """
        Override of our parent class' method for test purposes.

        Saves a record of the change for testing the final result in
        `apply_upgrades()`, above.

        Assert the following:
          * This is not a second change for this board.
          * If we're changing the default mapping, then every board
            that will be changing to a non-default mapping has been
            updated.

        @param board        The board with the changing version.
        @param new_version  The new version to be applied to the board.
        """
        self._mappings[board] = new_version
        self._testcase.assertNotIn(board, self._updated_mappings)
        self._updated_mappings[board] = new_version
        if board == _DEFAULT_BOARD:
            for board in self._expected_mappings:
                self._testcase.assertIn(board, self._mappings)

    def _do_delete_mapping(self, board):
        """
        Override of our parent class' method for test purposes.

        Saves a record of the change for testing the final result in
        `apply_upgrades()`, above.

        Assert that the board has a mapping prior to deletion.

        @param board        The board with the version to be deleted.
        """
        self._testcase.assertNotEqual(board, _DEFAULT_BOARD)
        self._testcase.assertIn(board, self._mappings)
        del self._mappings[board]
        self._actual_deletions.add(board)


class ApplyCrOSUpgradesTests(unittest.TestCase):
    """Tests for the `_apply_cros_upgrades()` function."""

    def _apply_upgrades(self, old_versions, new_versions, change_default):
        """
        Test a single call to `_apply_cros_upgrades()`.

        All assertions are handled by an instance of `_TestUpdater`.

        @param old_versions   Parameter to be passed to
                              `_apply_cros_upgrades()`.
        @param new_versions   Parameter to be passed to
                              `_apply_cros_upgrades()`.
        @param change_default   Whether the test should include a change
                                to the default version mapping.
        """
        old_versions[_DEFAULT_BOARD] = _OLD_DEFAULT
        if change_default:
            new_default = _NEW_DEFAULT
        else:
            new_default = _OLD_DEFAULT
        expected_versions = {
            b: v for b, v in new_versions.items() if v != new_default
        }
        expected_versions[_DEFAULT_BOARD] = new_default
        updater = _TestUpdater(self)
        updater.pretest_init(old_versions, expected_versions)
        assign_stable_images._apply_cros_upgrades(
            updater, old_versions, new_versions, new_default)
        updater.check_results(change_default)

    def test_no_changes(self):
        """
        Test an empty upgrade that does nothing.

        Test the boundary case of an upgrade where there are no boards,
        and the default does not change.
        """
        self._apply_upgrades({}, {}, False)

    def test_change_default(self):
        """
        Test an empty upgrade that merely changes the default.

        Test the boundary case of an upgrade where there are no boards,
        but the default is upgraded.
        """
        self._apply_upgrades({}, {}, True)

    def test_board_default_no_changes(self):
        """
        Test that a board at default stays with an unchanged default.

        Test the case of a board that is mapped to the default, where
        neither the board nor the default change.
        """
        self._apply_upgrades({}, {'board': _OLD_DEFAULT}, False)

    def test_board_left_behind(self):
        """
        Test a board left at the old default after a default upgrade.

        Test the case of a board that stays mapped to the old default as
        the default board is upgraded.
        """
        self._apply_upgrades({}, {'board': _OLD_DEFAULT}, True)

    def test_board_upgrade_from_default(self):
        """
        Test upgrading a board from a default that doesn't change.

        Test the case of upgrading a board from default to non-default,
        where the default doesn't change.
        """
        self._apply_upgrades({}, {'board': _NEW_VERSION}, False)

    def test_board_and_default_diverge(self):
        """
        Test upgrading a board that diverges from the default.

        Test the case of upgrading a board and default together from the
        same to different versions.
        """
        self._apply_upgrades({}, {'board': _NEW_VERSION}, True)

    def test_board_tracks_default(self):
        """
        Test upgrading a board to track a default upgrade.

        Test the case of upgrading a board and the default together.
        """
        self._apply_upgrades({}, {'board': _NEW_DEFAULT}, True)

    def test_board_non_default_no_changes(self):
        """
        Test an upgrade with no changes to a board or the default.

        Test the case of an upgrade with a board in it, where neither
        the board nor the default change.
        """
        self._apply_upgrades({'board': _NEW_VERSION},
                             {'board': _NEW_VERSION},
                             False)

    def test_board_upgrade_and_keep_default(self):
        """
        Test a board upgrade with an unchanged default.

        Test the case of upgrading a board while the default stays the
        same.
        """
        self._apply_upgrades({'board': _OLD_VERSION},
                             {'board': _NEW_VERSION},
                             False)

    def test_board_upgrade_and_change_default(self):
        """
        Test upgrading a board and the default separately.

        Test the case of upgrading both a board and the default, each
        from and to different versions.
        """
        self._apply_upgrades({'board': _OLD_VERSION},
                             {'board': _NEW_VERSION},
                             True)

    def test_board_leads_default(self):
        """
        Test a board that upgrades ahead of the new default.

        Test the case of upgrading both a board and the default, where
        the board's old version is the new default version.
        """
        self._apply_upgrades({'board': _NEW_DEFAULT},
                             {'board': _NEW_VERSION},
                             True)

    def test_board_lags_to_old_default(self):
        """
        Test a board that upgrades behind the old default.

        Test the case of upgrading both a board and the default, where
        the board's new version is the old default version.
        """
        self._apply_upgrades({'board': _OLD_VERSION},
                             {'board': _OLD_DEFAULT},
                             True)

    def test_board_joins_old_default(self):
        """
        Test upgrading a board to a default that doesn't change.

        Test the case of upgrading board to the default, where the
        default mapping stays unchanged.
        """
        self._apply_upgrades({'board': _OLD_VERSION},
                             {'board': _OLD_DEFAULT},
                             False)

    def test_board_joins_new_default(self):
        """
        Test upgrading a board to match the new default.

        Test the case of upgrading board and the default to the same
        version.
        """
        self._apply_upgrades({'board': _OLD_VERSION},
                             {'board': _NEW_DEFAULT},
                             True)

    def test_board_becomes_default(self):
        """
        Test a board that becomes default after a default upgrade.

        Test the case of upgrading the default to a version already
        mapped for an existing board.
        """
        self._apply_upgrades({'board': _NEW_DEFAULT},
                             {'board': _NEW_DEFAULT},
                             True)


class ApplyFirmwareUpgradesTests(unittest.TestCase):
    """Tests for the `_apply_firmware_upgrades()` function."""

    def _apply_upgrades(self, old_versions, new_versions):
        """
        Test a single call to `_apply_firmware_upgrades()`.

        All assertions are handled by an instance of `_TestUpdater`.

        @param old_versions   Parameter to be passed to
                              `_apply_firmware_upgrades()`.
        @param new_versions   Parameter to be passed to
                              `_apply_firmware_upgrades()`.
        """
        updater = _TestUpdater(self)
        updater.pretest_init(old_versions, new_versions)
        assign_stable_images._apply_firmware_upgrades(
            updater, old_versions, new_versions)
        updater.check_results(False)

    def test_no_changes(self):
        """
        Test an empty upgrade that does nothing.

        Test the boundary case of an upgrade where there are no boards.
        """
        self._apply_upgrades({}, {})

    def test_board_added(self):
        """
        Test an upgrade that adds a new board.

        Test the case of an upgrade where a board that was previously
        unmapped is added.
        """
        self._apply_upgrades({}, {'board': _NEW_VERSION})

    def test_board_unchanged(self):
        """
        Test an upgrade with no changes to a board.

        Test the case of an upgrade with a board that stays the same.
        """
        self._apply_upgrades({'board': _NEW_VERSION},
                             {'board': _NEW_VERSION})

    def test_board_upgrade_and_change_default(self):
        """
        Test upgrading a board.

        Test the case of upgrading a board to a new version.
        """
        self._apply_upgrades({'board': _OLD_VERSION},
                             {'board': _NEW_VERSION})


if __name__ == '__main__':
    unittest.main()
