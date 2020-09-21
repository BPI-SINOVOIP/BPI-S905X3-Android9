# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This module provides helper method to parse /etc/lsb-release file to extract
# various information.

import logging
import os
import re

import common
from autotest_lib.client.cros import constants


JETSTREAM_BOARDS = frozenset(['arkham', 'gale', 'whirlwind'])

def _lsbrelease_search(regex, group_id=0, lsb_release_content=None):
    """Searches /etc/lsb-release for a regex match.

    @param regex: Regex to match.
    @param group_id: The group in the regex we are searching for.
                     Default is group 0.
    @param lsb_release_content: A string represents the content of lsb-release.
            If the caller is from drone, it can pass in the file content here.

    @returns the string in the specified group if there is a match or None if
             not found.

    @raises IOError if /etc/lsb-release can not be accessed.
    """
    if lsb_release_content is None:
        with open(constants.LSB_RELEASE) as lsb_release_file:
            lsb_release_content = lsb_release_file.read()
    for line in lsb_release_content.split('\n'):
        m = re.match(regex, line)
        if m:
            return m.group(group_id)
    return None


def get_current_board(lsb_release_content=None):
    """Return the current board name.

    @param lsb_release_content: A string represents the content of lsb-release.
            If the caller is from drone, it can pass in the file content here.

    @return current board name, e.g "lumpy", None on fail.
    """
    return _lsbrelease_search(r'^CHROMEOS_RELEASE_BOARD=(.+)$', group_id=1,
                              lsb_release_content=lsb_release_content)


def get_chromeos_channel(lsb_release_content=None):
    """Get chromeos channel in device under test as string. None on fail.

    @param lsb_release_content: A string represents the content of lsb-release.
            If the caller is from drone, it can pass in the file content here.

    @return chromeos channel in device under test as string. None on fail.
    """
    return _lsbrelease_search(
        r'^CHROMEOS_RELEASE_DESCRIPTION=.+ (.+)-channel.*$',
        group_id=1, lsb_release_content=lsb_release_content)


def get_chromeos_release_version(lsb_release_content=None):
    """Get chromeos version in device under test as string. None on fail.

    @param lsb_release_content: A string represents the content of lsb-release.
            If the caller is from drone, it can pass in the file content here.

    @return chromeos version in device under test as string. None on fail.
    """
    return _lsbrelease_search(r'^CHROMEOS_RELEASE_VERSION=(.+)$', group_id=1,
                              lsb_release_content=lsb_release_content)


def get_chromeos_release_builder_path(lsb_release_content=None):
    """Get chromeos builder path from device under test as string.

    @param lsb_release_content: A string representing the content of
            lsb-release. If the caller is from drone, it can pass in the file
            content here.

    @return chromeos builder path in device under test as string. None on fail.
    """
    return _lsbrelease_search(r'^CHROMEOS_RELEASE_BUILDER_PATH=(.+)$',
                              group_id=1,
                              lsb_release_content=lsb_release_content)


def get_chromeos_release_milestone(lsb_release_content=None):
    """Get chromeos milestone in device under test as string. None on fail.

    @param lsb_release_content: A string represents the content of lsb-release.
            If the caller is from drone, it can pass in the file content here.

    @return chromeos release milestone in device under test as string.
            None on fail.
    """
    return _lsbrelease_search(r'^CHROMEOS_RELEASE_CHROME_MILESTONE=(.+)$',
                              group_id=1,
                              lsb_release_content=lsb_release_content)


def is_moblab(lsb_release_content=None):
    """Return if we are running on a Moblab system or not.

    @param lsb_release_content: A string represents the content of lsb-release.
            If the caller is from drone, it can pass in the file content here.

    @return the board string if this is a Moblab device or None if it is not.
    """
    if lsb_release_content is not None:
        return _lsbrelease_search(r'.*moblab',
                                  lsb_release_content=lsb_release_content)

    if os.path.exists(constants.LSB_RELEASE):
        return _lsbrelease_search(r'.*moblab')

    try:
        from chromite.lib import cros_build_lib
        if cros_build_lib.IsInsideChroot():
            return None
    except ImportError as e:
        logging.error('Unable to determine if this is a moblab system: %s', e)


def is_jetstream(lsb_release_content=None):
    """Parses lsb_contents to determine if the host is a Jetstream host.

    @param lsb_release_content: The string contents of lsb-release.
            If None, the local lsb-release is used.

    @return True if the host is a Jetstream device, otherwise False.
    """
    board = get_current_board(lsb_release_content=lsb_release_content)
    return board in JETSTREAM_BOARDS

def is_gce_board(lsb_release_content=None):
    """Parses lsb_contents to determine if host is a GCE board.

    @param lsb_release_content: The string contents of lsb-release.
            If None, the local lsb-release is used.

    @return True if the host is a GCE board otherwise False.
    """
    return is_lakitu(lsb_release_content=lsb_release_content)

def is_lakitu(lsb_release_content=None):
    """Parses lsb_contents to determine if host is lakitu.

    @param lsb_release_content: The string contents of lsb-release.
            If None, the local lsb-release is used.

    @return True if the host is lakitu otherwise False.
    """
    board = get_current_board(lsb_release_content=lsb_release_content)
    if board is not None:
        return board.startswith('lakitu')
    return False

def get_chrome_milestone(lsb_release_content=None):
    """Get the value for the Chrome milestone.

    @param lsb_release_content: A string represents the content of lsb-release.
            If the caller is from drone, it can pass in the file content here.

    @return the value for the Chrome milestone
    """
    return _lsbrelease_search(r'^CHROMEOS_RELEASE_CHROME_MILESTONE=(.+)$',
                              group_id=1,
                              lsb_release_content=lsb_release_content)


def get_device_type(lsb_release_content=None):
    """Get the device type string, e.g. "CHROMEBOOK" or "CHROMEBOX".

    @param lsb_release_content: A string represents the content of lsb-release.
            If the caller is from drone, it can pass in the file content here.

    @return the DEVICETYPE value for this machine.
    """
    return _lsbrelease_search(r'^DEVICETYPE=(.+)$', group_id=1,
                              lsb_release_content=lsb_release_content)


def is_arc_available(lsb_release_content=None):
    """Returns True if the device has ARC installed.

    @param lsb_release_content: A string represents the content of lsb-release.
            If the caller is from drone, it can pass in the file content here.

    @return True if the device has ARC installed.
    """
    return (_lsbrelease_search(r'^CHROMEOS_ARC_VERSION',
                               lsb_release_content=lsb_release_content)
            is not None)
