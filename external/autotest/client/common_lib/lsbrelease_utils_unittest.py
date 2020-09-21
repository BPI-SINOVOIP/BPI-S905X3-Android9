#!/usr/bin/python2
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittests for the lsbrelease_utils module."""

import unittest

import common
from autotest_lib.client.common_lib import lsbrelease_utils


# pylint: disable=line-too-long
_GUADO_MOBLAB_LSB_RELEASE_REDACTED = """
DEVICETYPE=CHROMEBOX
CHROMEOS_RELEASE_BUILDER_PATH=guado_moblab-release/R61-9641.0.0
GOOGLE_RELEASE=9641.0.0
CHROMEOS_DEVSERVER=
CHROMEOS_RELEASE_BOARD=guado_moblab
CHROMEOS_RELEASE_BUILD_NUMBER=9641
CHROMEOS_RELEASE_BRANCH_NUMBER=0
CHROMEOS_RELEASE_CHROME_MILESTONE=61
CHROMEOS_RELEASE_PATCH_NUMBER=0
CHROMEOS_RELEASE_TRACK=testimage-channel
CHROMEOS_RELEASE_DESCRIPTION=9641.0.0 (Official Build) dev-channel guado_moblab test
CHROMEOS_RELEASE_BUILD_TYPE=Official Build
CHROMEOS_RELEASE_NAME=Chrome OS
CHROMEOS_RELEASE_VERSION=9641.0.0
CHROMEOS_AUSERVER=https://tools.google.com/service/update2
"""

_LINK_LSB_RELEASE_REDACTED = """
DEVICETYPE=CHROMEBOOK
CHROMEOS_RELEASE_BUILDER_PATH=link-release/R61-9641.0.0
GOOGLE_RELEASE=9641.0.0
CHROMEOS_DEVSERVER=
CHROMEOS_RELEASE_BOARD=link
CHROMEOS_RELEASE_BUILD_NUMBER=9641
CHROMEOS_RELEASE_BRANCH_NUMBER=0
CHROMEOS_RELEASE_CHROME_MILESTONE=61
CHROMEOS_RELEASE_PATCH_NUMBER=0
CHROMEOS_RELEASE_TRACK=testimage-channel
CHROMEOS_RELEASE_DESCRIPTION=9641.0.0 (Official Build) dev-channel link test
CHROMEOS_RELEASE_BUILD_TYPE=Official Build
CHROMEOS_RELEASE_NAME=Chrome OS
CHROMEOS_RELEASE_VERSION=9641.0.0
CHROMEOS_AUSERVER=https://tools.google.com/service/update2
"""

_GALE_LSB_RELEASE_REDACTED = """
DEVICETYPE=OTHER
HWID_OVERRIDE=GALE DOGFOOD
CHROMEOS_RELEASE_BUILDER_PATH=gale-release/R61-9641.0.0
GOOGLE_RELEASE=9641.0.0
CHROMEOS_DEVSERVER=
CHROMEOS_RELEASE_BOARD=gale
CHROMEOS_RELEASE_BUILD_NUMBER=9641
CHROMEOS_RELEASE_BRANCH_NUMBER=0
CHROMEOS_RELEASE_CHROME_MILESTONE=61
CHROMEOS_RELEASE_PATCH_NUMBER=0
CHROMEOS_RELEASE_TRACK=testimage-channel
CHROMEOS_RELEASE_DESCRIPTION=9641.0.0 (Official Build) dev-channel gale test
CHROMEOS_RELEASE_BUILD_TYPE=Official Build
CHROMEOS_RELEASE_NAME=Chrome OS
CHROMEOS_RELEASE_VERSION=9641.0.0
CHROMEOS_AUSERVER=https://tools.google.com/service/update2
"""

# pylint: disable=line-too-long
_WHIRLWIND_LSB_RELEASE_REDACTED = """
DEVICETYPE=OTHER
HWID_OVERRIDE=WHIRLWIND DOGFOOD
CHROMEOS_RELEASE_BUILDER_PATH=whirlwind-release/R61-9641.0.0
GOOGLE_RELEASE=9641.0.0
CHROMEOS_DEVSERVER=
CHROMEOS_RELEASE_BOARD=whirlwind
CHROMEOS_RELEASE_BUILD_NUMBER=9641
CHROMEOS_RELEASE_BRANCH_NUMBER=0
CHROMEOS_RELEASE_CHROME_MILESTONE=61
CHROMEOS_RELEASE_PATCH_NUMBER=0
CHROMEOS_RELEASE_TRACK=testimage-channel
CHROMEOS_RELEASE_DESCRIPTION=9641.0.0 (Official Build) dev-channel whirlwind test
CHROMEOS_RELEASE_BUILD_TYPE=Official Build
CHROMEOS_RELEASE_NAME=Chrome OS
CHROMEOS_RELEASE_VERSION=9641.0.0
CHROMEOS_AUSERVER=https://tools.google.com/service/update2
"""


class LsbreleaseUtilsTestCase(unittest.TestCase):
    """Validates the helper free functions in lsbrelease_utils."""

    def test_is_jetstream_with_link_lsbrelease(self):
        """Test helper function."""
        self.assertFalse(lsbrelease_utils.is_jetstream(
            _LINK_LSB_RELEASE_REDACTED))

    def test_is_jetstream_with_moblab_lsbrelease(self):
        """Test helper function."""
        self.assertFalse(lsbrelease_utils.is_jetstream(
            _GUADO_MOBLAB_LSB_RELEASE_REDACTED))

    def test_is_jestream_with_gale_lsbrelease(self):
        """Test helper function."""
        self.assertTrue(lsbrelease_utils.is_jetstream(
            _GALE_LSB_RELEASE_REDACTED))

    def test_is_jestream_with_whirlwind_lsbrelease(self):
        """Test helper function."""
        self.assertTrue(lsbrelease_utils.is_jetstream(
            _WHIRLWIND_LSB_RELEASE_REDACTED))

    def test_is_moblab_with_empty_lsbrelease(self):
        """is_moblab correctly validates trivial lsb-release information."""
        self.assertFalse(lsbrelease_utils.is_moblab(''))

    def test_is_moblab_with_link_lsbrelease(self):
        """is_moblab correctly validates the contents from some other board."""
        self.assertFalse(lsbrelease_utils.is_moblab(
                _LINK_LSB_RELEASE_REDACTED))

    def test_is_moblab_with_moblab_lsbrelease(self):
        """is_moblab correctly validates the contents from a moblab device."""
        self.assertTrue(lsbrelease_utils.is_moblab(
                _GUADO_MOBLAB_LSB_RELEASE_REDACTED))

    def test_get_chromeos_release_version(self):
        """Test helper function."""
        result = lsbrelease_utils.get_chromeos_release_builder_path(
                _GUADO_MOBLAB_LSB_RELEASE_REDACTED)

        self.assertEqual(result, 'guado_moblab-release/R61-9641.0.0')


if __name__ == '__main__':
    unittest.main()
