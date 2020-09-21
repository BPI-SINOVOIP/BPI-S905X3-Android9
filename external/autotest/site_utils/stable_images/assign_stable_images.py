#!/usr/bin/python
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Automatically update the afe_stable_versions table.

This command updates the stable repair version for selected boards
in the lab.  For each board, if the version that Omaha is serving
on the Beta channel for the board is more recent than the current
stable version in the AFE database, then the AFE is updated to use
the version on Omaha.

The upgrade process is applied to every "managed board" in the test
lab.  Generally, a managed board is a board with both spare and
critical scheduling pools.

See `autotest_lib.site_utils.lab_inventory` for the full definition
of "managed board".

The command supports a `--dry-run` option that reports changes that
would be made, without making the actual RPC calls to change the
database.

"""

import argparse
import json
import subprocess
import sys

import common
from autotest_lib.client.common_lib import utils
from autotest_lib.server.cros.dynamic_suite import frontend_wrappers
from autotest_lib.site_utils import lab_inventory


# _OMAHA_STATUS - URI of a file in GoogleStorage with a JSON object
# summarizing all versions currently being served by Omaha.
#
# The principle data is in an array named 'omaha_data'.  Each entry
# in the array contains information relevant to one image being
# served by Omaha, including the following information:
#   * The board name of the product, as known to Omaha.
#   * The channel associated with the image.
#   * The Chrome and Chrome OS version strings for the image
#     being served.
#
_OMAHA_STATUS = 'gs://chromeos-build-release-console/omaha_status.json'


# _BUILD_METADATA_PATTERN - Format string for the URI of a file in
# GoogleStorage with a JSON object that contains metadata about
# a given build.  The metadata includes the version of firmware
# bundled with the build.
#
_BUILD_METADATA_PATTERN = 'gs://chromeos-image-archive/%s/metadata.json'


# _DEFAULT_BOARD - The distinguished board name used to identify a
# stable version mapping that is used for any board without an explicit
# mapping of its own.
#
# _DEFAULT_VERSION_TAG - A string used to signify that there is no
# mapping for a board, in other words, the board is mapped to the
# default version.
#
_DEFAULT_BOARD = 'DEFAULT'
_DEFAULT_VERSION_TAG = '(default)'


# _FIRMWARE_UPGRADE_BLACKLIST - a set of boards that are exempt from
# automatic stable firmware version assignment.  This blacklist is
# here out of an abundance of caution, on the general principle of "if
# it ain't broke, don't fix it."  Specifically, these are old, legacy
# boards and:
#   * They're working fine with whatever firmware they have in the lab
#     right now.  Moreover, because of their age, we can expect that
#     they will never get any new firmware updates in future.
#   * Servo support is spotty or missing, so there's no certainty
#     that DUTs bricked by a firmware update can be repaired.
#   * Because of their age, they are somewhere between hard and
#     impossible to replace.  In some cases, they are also already
#     in short supply.
#
# N.B.  HARDCODED BOARD NAMES ARE EVIL!!!  This blacklist uses hardcoded
# names because it's meant to define a list of legacies that will shrivel
# and die over time.
#
# DO NOT ADD TO THIS LIST.  If there's a new use case that requires
# extending the blacklist concept, you should find a maintainable
# solution that deletes this code.
#
# TODO(jrbarnette):  When any board is past EOL, and removed from the
# lab, it can be removed from the blacklist.  When all the boards are
# past EOL, the blacklist should be removed.

_FIRMWARE_UPGRADE_BLACKLIST = set([
        'butterfly',
        'daisy',
        'daisy_skate',
        'daisy_spring',
        'lumpy',
        'parrot',
        'parrot_ivb',
        'peach_pi',
        'peach_pit',
        'stout',
        'stumpy',
        'x86-alex',
        'x86-mario',
        'x86-zgb',
    ])


def _get_by_key_path(dictdict, key_path):
    """
    Traverse a sequence of keys in a dict of dicts.

    The `dictdict` parameter is a dict of nested dict values, and
    `key_path` a list of keys.

    A single-element key path returns `dictdict[key_path[0]]`, a
    two-element path returns `dictdict[key_path[0]][key_path[1]]`, and
    so forth.  If any key in the path is not found, return `None`.

    @param dictdict   A dictionary of nested dictionaries.
    @param key_path   The sequence of keys to look up in `dictdict`.
    @return The value found by successive dictionary lookups, or `None`.
    """
    value = dictdict
    for key in key_path:
        value = value.get(key)
        if value is None:
            break
    return value


def _get_model_firmware_versions(metadata_json, board):
    """
    Get the firmware version for each model for a unibuild board.

    @param metadata_json    The metadata_json dict parsed from the metadata.json
                            file generated by the build.
    @param board            The board name of the unibuild.
    @return If no models found for a board, return {board: None}; elase, return
            a dict mapping from model name to its upgrade firmware version.
    """
    model_firmware_versions = {}
    key_path = ['board-metadata', board, 'models']
    model_versions = _get_by_key_path(metadata_json, key_path)

    if model_versions is not None:
        for model, fw_versions in model_versions.iteritems():
            fw_version = (fw_versions.get('main-readwrite-firmware-version') or
                          fw_versions.get('main-readonly-firmware-version'))
            model_firmware_versions[model] = fw_version
    else:
        model_firmware_versions[board] = None

    return model_firmware_versions


def get_firmware_versions(version_map, board, cros_version):
    """
    Get the firmware versions for a given board and CrOS version.

    Typically, CrOS builds bundle firmware that is installed at update
    time. The returned firmware version value will be `None` if the build isn't
    found in storage, if there is no firmware found for the build, or if the
    board is blacklisted from firmware updates in the test lab.

    @param version_map    An AFE cros version map object; used to
                          locate the build in storage.
    @param board          The board for the firmware version to be
                          determined.
    @param cros_version   The CrOS version bundling the firmware.
    @return A dict mapping from board to firmware version string for
            non-unibuild board, or a dict mapping from models to firmware
            versions for a unibuild board (see return type of
            _get_model_firmware_versions)
    """
    if board in _FIRMWARE_UPGRADE_BLACKLIST:
        return {board: None}
    try:
        image_path = version_map.format_image_name(board, cros_version)
        uri = _BUILD_METADATA_PATTERN % image_path
        metadata_json = _read_gs_json_data(uri)
        unibuild = bool(_get_by_key_path(metadata_json, ['unibuild']))
        if unibuild:
            return _get_model_firmware_versions(metadata_json, board)
        else:
            key_path = ['board-metadata', board, 'main-firmware-version']
            fw_version = _get_by_key_path(metadata_json, key_path)
            return {board: fw_version}
    except Exception as e:
        # TODO(jrbarnette): If we get here, it likely means that
        # the repair build for our board doesn't exist.  That can
        # happen if a board doesn't release on the Beta channel for
        # at least 6 months.
        #
        # We can't allow this error to propogate up the call chain
        # because that will kill assigning versions to all the other
        # boards that are still OK, so for now we ignore it.  We
        # really should do better.
        print ('Failed to get firmware version for board %s: %s.' % (board, e))
        return {board: None}


class _VersionUpdater(object):
    """
    Class to report and apply version changes.

    This class is responsible for the low-level logic of applying
    version upgrades and reporting them as command output.

    This class exists to solve two problems:
     1. To distinguish "normal" vs. "dry-run" modes.  Each mode has a
        subclass; methods that perform actual AFE updates are
        implemented for the normal mode subclass only.
     2. To provide hooks for unit tests.  The unit tests override both
        the reporting and modification behaviors, in order to test the
        higher level logic that decides what changes are needed.

    Methods meant merely to report changes to command output have names
    starting with "report" or "_report".  Methods that are meant to
    change the AFE in normal mode have names starting with "_do"
    """

    def __init__(self, afe):
        image_types = [afe.CROS_IMAGE_TYPE, afe.FIRMWARE_IMAGE_TYPE]
        self._version_maps = {
            image_type: afe.get_stable_version_map(image_type)
                for image_type in image_types
        }
        self._cros_map = self._version_maps[afe.CROS_IMAGE_TYPE]
        self._selected_map = None

    def select_version_map(self, image_type):
        """
        Select an AFE version map object based on `image_type`.

        This creates and remembers an AFE version mapper object to be
        used for making changes in normal mode.

        @param image_type   Image type parameter for the version mapper
                            object.
        """
        self._selected_map = self._version_maps[image_type]
        return self._selected_map

    def announce(self):
        """Announce the start of processing to the user."""
        pass

    def report(self, message):
        """
        Report a pre-formatted message for the user.

        The message is printed to stdout, followed by a newline.

        @param message The message to be provided to the user.
        """
        print message

    def report_default_changed(self, old_default, new_default):
        """
        Report that the default version mapping is changing.

        This merely reports a text description of the pending change
        without executing it.

        @param old_default  The original default version.
        @param new_default  The new default version to be applied.
        """
        self.report('Default %s -> %s' % (old_default, new_default))

    def _report_board_changed(self, board, old_version, new_version):
        """
        Report a change in one board's assigned version mapping.

        This merely reports a text description of the pending change
        without executing it.

        @param board        The board with the changing version.
        @param old_version  The original version mapped to the board.
        @param new_version  The new version to be applied to the board.
        """
        template = '    %-22s %s -> %s'
        self.report(template % (board, old_version, new_version))

    def report_board_unchanged(self, board, old_version):
        """
        Report that a board's version mapping is unchanged.

        This reports that a board has a non-default mapping that will be
        unchanged.

        @param board        The board that is not changing.
        @param old_version  The board's version mapping.
        """
        self._report_board_changed(board, '(no change)', old_version)

    def _do_set_mapping(self, board, new_version):
        """
        Change one board's assigned version mapping.

        @param board        The board with the changing version.
        @param new_version  The new version to be applied to the board.
        """
        pass

    def _do_delete_mapping(self, board):
        """
        Delete one board's assigned version mapping.

        @param board        The board with the version to be deleted.
        """
        pass

    def set_mapping(self, board, old_version, new_version):
        """
        Change and report a board version mapping.

        @param board        The board with the changing version.
        @param old_version  The original version mapped to the board.
        @param new_version  The new version to be applied to the board.
        """
        self._report_board_changed(board, old_version, new_version)
        self._do_set_mapping(board, new_version)

    def upgrade_default(self, new_default):
        """
        Apply a default version change.

        @param new_default  The new default version to be applied.
        """
        self._do_set_mapping(_DEFAULT_BOARD, new_default)

    def delete_mapping(self, board, old_version):
        """
        Delete a board version mapping, and report the change.

        @param board        The board with the version to be deleted.
        @param old_version  The board's verson prior to deletion.
        """
        assert board != _DEFAULT_BOARD
        self._report_board_changed(board,
                                   old_version,
                                   _DEFAULT_VERSION_TAG)
        self._do_delete_mapping(board)


class _DryRunUpdater(_VersionUpdater):
    """Code for handling --dry-run execution."""

    def announce(self):
        self.report('Dry run:  no changes will be made.')


class _NormalModeUpdater(_VersionUpdater):
    """Code for handling normal execution."""

    def _do_set_mapping(self, board, new_version):
        self._selected_map.set_version(board, new_version)

    def _do_delete_mapping(self, board):
        self._selected_map.delete_version(board)


def _read_gs_json_data(gs_uri):
    """
    Read and parse a JSON file from googlestorage.

    This is a wrapper around `gsutil cat` for the specified URI.
    The standard output of the command is parsed as JSON, and the
    resulting object returned.

    @return A JSON object parsed from `gs_uri`.
    """
    with open('/dev/null', 'w') as ignore_errors:
        sp = subprocess.Popen(['gsutil', 'cat', gs_uri],
                              stdout=subprocess.PIPE,
                              stderr=ignore_errors)
        try:
            json_object = json.load(sp.stdout)
        finally:
            sp.stdout.close()
            sp.wait()
    return json_object


def _make_omaha_versions(omaha_status):
    """
    Convert parsed omaha versions data to a versions mapping.

    Returns a dictionary mapping board names to the currently preferred
    version for the Beta channel as served by Omaha.  The mappings are
    provided by settings in the JSON object `omaha_status`.

    The board names are the names as known to Omaha:  If the board name
    in the AFE contains '_', the corresponding Omaha name uses '-'
    instead.  The boards mapped may include boards not in the list of
    managed boards in the lab.

    @return A dictionary mapping Omaha boards to Beta versions.
    """
    def _entry_valid(json_entry):
        return json_entry['channel'] == 'beta'

    def _get_omaha_data(json_entry):
        board = json_entry['board']['public_codename']
        milestone = json_entry['milestone']
        build = json_entry['chrome_os_version']
        version = 'R%d-%s' % (milestone, build)
        return (board, version)

    return dict(_get_omaha_data(e) for e in omaha_status['omaha_data']
                    if _entry_valid(e))


def _get_upgrade_versions(cros_versions, omaha_versions, boards):
    """
    Get the new stable versions to which we should update.

    The new versions are returned as a tuple of a dictionary mapping
    board names to versions, plus a new default board setting.  The
    new default is determined as the most commonly used version
    across the given boards.

    The new dictionary will have a mapping for every board in `boards`.
    That mapping will be taken from `cros_versions`, unless the board has
    a mapping in `omaha_versions` _and_ the omaha version is more recent
    than the AFE version.

    @param cros_versions    The current board->version mappings in the
                            AFE.
    @param omaha_versions   The current board->version mappings from
                            Omaha for the Beta channel.
    @param boards           Set of boards to be upgraded.
    @return Tuple of (mapping, default) where mapping is a dictionary
            mapping boards to versions, and default is a version string.
    """
    upgrade_versions = {}
    version_counts = {}
    afe_default = cros_versions[_DEFAULT_BOARD]
    for board in boards:
        version = cros_versions.get(board, afe_default)
        omaha_version = omaha_versions.get(board.replace('_', '-'))
        if (omaha_version is not None and
                utils.compare_versions(version, omaha_version) < 0):
            version = omaha_version
        upgrade_versions[board] = version
        version_counts.setdefault(version, 0)
        version_counts[version] += 1
    return (upgrade_versions,
            max(version_counts.items(), key=lambda x: x[1])[0])


def _get_firmware_upgrades(cros_version_map, cros_versions):
    """
    Get the new firmware versions to which we should update.

    @param cros_version_map An instance of frontend._CrosVersionMap.
    @param cros_versions    Current board->cros version mappings in the
                            AFE.
    @return A dictionary mapping boards/models to firmware upgrade versions.
            If the build is unibuild, the key is a model name; else, the key
            is a board name.
    """
    firmware_upgrades = {}
    for board, version in cros_versions.iteritems():
        firmware_upgrades.update(get_firmware_versions(
            cros_version_map, board, version))

    return firmware_upgrades


def _apply_cros_upgrades(updater, old_versions, new_versions,
                         new_default):
    """
    Change CrOS stable version mappings in the AFE.

    The input `old_versions` dictionary represents the content of the
    `afe_stable_versions` database table; it contains mappings for a
    default version, plus exceptions for boards with non-default
    mappings.

    The `new_versions` dictionary contains a mapping for every board,
    including boards that will be mapped to the new default version.

    This function applies the AFE changes necessary to produce the new
    AFE mappings indicated by `new_versions` and `new_default`.  The
    changes are ordered so that at any moment, every board is mapped
    either according to the old or the new mapping.

    @param updater        Instance of _VersionUpdater responsible for
                          making the actual database changes.
    @param old_versions   The current board->version mappings in the
                          AFE.
    @param new_versions   New board->version mappings obtained by
                          applying Beta channel upgrades from Omaha.
    @param new_default    The new default build for the AFE.
    """
    old_default = old_versions[_DEFAULT_BOARD]
    if old_default != new_default:
        updater.report_default_changed(old_default, new_default)
    updater.report('Applying stable version changes:')
    default_count = 0
    for board, new_build in new_versions.items():
        if new_build == new_default:
            default_count += 1
        elif board in old_versions and new_build == old_versions[board]:
            updater.report_board_unchanged(board, new_build)
        else:
            old_build = old_versions.get(board)
            if old_build is None:
                old_build = _DEFAULT_VERSION_TAG
            updater.set_mapping(board, old_build, new_build)
    if old_default != new_default:
        updater.upgrade_default(new_default)
    for board, new_build in new_versions.items():
        if new_build == new_default and board in old_versions:
            updater.delete_mapping(board, old_versions[board])
    updater.report('%d boards now use the default mapping' %
                   default_count)


def _apply_firmware_upgrades(updater, old_versions, new_versions):
    """
    Change firmware version mappings in the AFE.

    The input `old_versions` dictionary represents the content of the
    firmware mappings in the `afe_stable_versions` database table.
    There is no default version; missing boards simply have no current
    version.

    This function applies the AFE changes necessary to produce the new
    AFE mappings indicated by `new_versions`.

    TODO(jrbarnette) This function ought to remove any mapping not found
    in `new_versions`.  However, in theory, that's only needed to
    account for boards that are removed from the lab, and that hasn't
    happened yet.

    @param updater        Instance of _VersionUpdater responsible for
                          making the actual database changes.
    @param old_versions   The current board->version mappings in the
                          AFE.
    @param new_versions   New board->version mappings obtained by
                          applying Beta channel upgrades from Omaha.
    """
    unchanged = 0
    no_version = 0
    for board, new_firmware in new_versions.items():
        if new_firmware is None:
            no_version += 1
        elif board not in old_versions:
            updater.set_mapping(board, '(nothing)', new_firmware)
        else:
            old_firmware = old_versions[board]
            if new_firmware != old_firmware:
                updater.set_mapping(board, old_firmware, new_firmware)
            else:
                unchanged += 1
    updater.report('%d boards have no firmware mapping' % no_version)
    updater.report('%d boards are unchanged' % unchanged)


def _parse_command_line(argv):
    """
    Parse the command line arguments.

    Create an argument parser for this command's syntax, parse the
    command line, and return the result of the ArgumentParser
    parse_args() method.

    @param argv Standard command line argument vector; argv[0] is
                assumed to be the command name.
    @return Result returned by ArgumentParser.parse_args().

    """
    parser = argparse.ArgumentParser(
            prog=argv[0],
            description='Update the stable repair version for all '
                        'boards')
    parser.add_argument('-n', '--dry-run', dest='updater_mode',
                        action='store_const', const=_DryRunUpdater,
                        help='print changes without executing them')
    parser.add_argument('extra_boards', nargs='*', metavar='BOARD',
                        help='Names of additional boards to be updated.')
    arguments = parser.parse_args(argv[1:])
    if not arguments.updater_mode:
        arguments.updater_mode = _NormalModeUpdater
    return arguments


def main(argv):
    """
    Standard main routine.

    @param argv  Command line arguments including `sys.argv[0]`.
    """
    arguments = _parse_command_line(argv)
    afe = frontend_wrappers.RetryingAFE(server=None)
    updater = arguments.updater_mode(afe)
    updater.announce()
    boards = (set(arguments.extra_boards) |
              lab_inventory.get_managed_boards(afe))

    cros_version_map = updater.select_version_map(afe.CROS_IMAGE_TYPE)
    cros_versions = cros_version_map.get_all_versions()
    omaha_versions = _make_omaha_versions(
            _read_gs_json_data(_OMAHA_STATUS))
    upgrade_versions, new_default = (
        _get_upgrade_versions(cros_versions, omaha_versions, boards))
    _apply_cros_upgrades(updater, cros_versions,
                         upgrade_versions, new_default)

    updater.report('\nApplying firmware updates:')
    firmware_version_map = updater.select_version_map(afe.FIRMWARE_IMAGE_TYPE)
    fw_versions = firmware_version_map.get_all_versions()
    firmware_upgrades = _get_firmware_upgrades(
            cros_version_map, upgrade_versions)
    _apply_firmware_upgrades(updater, fw_versions, firmware_upgrades)


if __name__ == '__main__':
    main(sys.argv)
