# Copyright 2018 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import mock
import unittest

import common
from autotest_lib.server import utils
from server.hosts import cros_firmware


RW_VERSION_OUTPUT = """
flashrom(8): ed90a62cc9129d0215b4f5e4ecee8558 */build/lumpy/usr/sbin/flashrom
             ELF 64-bit LSB shared object, x86-64, version 1 (SYSV)...

BIOS image:   366248dc6d3a3d34ad62119738df721a */build/lumpy/tmp/...
BIOS version: Google_Lumpy.2.111.0
BIOS (RW) version: Google_Lumpy.2.112.0
EC image:     a5cdb921edc46a48ca64e9250b4f7a1f */build/lumpy/tmp/...
EC version:02WQA015

Package Content:
4de2580173772216cf37fdb8921a12e0 *./bin/mosys
"""

VERSION_OUTPUT = """
flashrom(8): ed90a62cc9129d0215b4f5e4ecee8558 */build/lumpy/usr/sbin/flashrom
             ELF 64-bit LSB shared object, x86-64, version 1 (SYSV)...

BIOS image:   366248dc6d3a3d34ad62119738df721a */build/lumpy/tmp/...
BIOS version: Google_Lumpy.2.111.0
EC image:     a5cdb921edc46a48ca64e9250b4f7a1f */build/lumpy/tmp/...
EC version:02WQA015

Package Content:
4de2580173772216cf37fdb8921a12e0 *./bin/mosys
"""

NO_VERSION_OUTPUT = """
flashrom(8): ed90a62cc9129d0215b4f5e4ecee8558 */build/lumpy/usr/sbin/flashrom
             ELF 64-bit LSB shared object, x86-64, version 1 (SYSV)...

BIOS image:   366248dc6d3a3d34ad62119738df721a */build/lumpy/tmp/...
EC image:     a5cdb921edc46a48ca64e9250b4f7a1f */build/lumpy/tmp/...
EC version:02WQA015

Package Content:
4de2580173772216cf37fdb8921a12e0 *./bin/mosys
"""

UNIBUILD_VERSION_OUTPUT = """
flashrom(8): 3a788e16b939f290e25771dcb1b6b542 */build/coral/usr/sbin/flashrom
             ELF 64-bit LSB shared object, x86-64, version 1 (SYSV)...

Model:        astronaut
BIOS image:   2abe9c3470e784c457ec9ee8e9f5cddf */models/astronaut/...
BIOS version: Google_Coral.10068.37.0
EC image:     6f084f024aa4f9f9981aeaa4935bca96 */models/astronaut/ec.bin
EC version:   coral_v1.1.7267-b7254f389

Model:        blue
BIOS image:   2abe9c3470e784c457ec9ee8e9f5cddf */models/blue/image-coral.bin
BIOS version: Google_Coral.10068.37.0
BIOS (RW) image:   e81aa62869e57cbe4a4baf7b4059778c */models/blue/...
BIOS (RW) version: Google_Coral.10068.39.0
EC image:     6f084f024aa4f9f9981aeaa4935bca96 */models/blue/ec.bin
EC version:   coral_v1.1.7267-b7254f389
EC (RW) version: coral_v1.1.7269-3fc31d6e2

Package Content:
61392084c8b80d805ad68e1b6019e188 *./updater4.sh
"""


class FirmwareVersionVerifierTest(unittest.TestCase):
    """Tests for FirmwareVersionVerifier."""

    def test_get_firmware_version_returns_rw_version(self):
        """Test _get_firmware_version which returns BIOS RW version."""
        fw = cros_firmware._get_firmware_version(RW_VERSION_OUTPUT)
        self.assertEqual(fw, 'Google_Lumpy.2.112.0')

    def test_get_firmware_version_returns_version(self):
        """Test _get_firmware_version which returns BIOS version."""
        fw = cros_firmware._get_firmware_version(VERSION_OUTPUT)
        self.assertEqual(fw, 'Google_Lumpy.2.111.0')

    def test_get_firmware_version_returns_none(self):
        """Test _get_firmware_version which returns None."""
        fw = cros_firmware._get_firmware_version(NO_VERSION_OUTPUT)
        self.assertIsNone(fw)

    def test_get_available_firmware_on_update_with_failure(self):
        """Test _get_available_firmware when update script exit_status=1."""
        result = utils.CmdResult(exit_status=1)
        host = mock.Mock()
        host.run.return_value = result

        fw = cros_firmware._get_available_firmware(host, 'lumpy')
        self.assertIsNone(fw)

    def test_get_available_firmware_returns_rw_version(self):
        """_get_available_firmware returns BIOS (RW) version."""
        result = utils.CmdResult(stdout=RW_VERSION_OUTPUT, exit_status=0)
        host = mock.Mock()
        host.run.return_value = result

        fw = cros_firmware._get_available_firmware(host, 'lumpy')
        self.assertEqual(fw, 'Google_Lumpy.2.112.0')

    def test_get_available_firmware_returns_version(self):
        """_get_available_firmware returns BIOS version."""
        result = utils.CmdResult(stdout=VERSION_OUTPUT, exit_status=0)
        host = mock.Mock()
        host.run.return_value = result

        fw = cros_firmware._get_available_firmware(host, 'lumpy')
        self.assertEqual(fw, 'Google_Lumpy.2.111.0')

    def test_get_available_firmware_returns_none(self):
        """_get_available_firmware returns None."""
        result = utils.CmdResult(stdout=NO_VERSION_OUTPUT, exit_status=0)
        host = mock.Mock()
        host.run.return_value = result

        fw = cros_firmware._get_available_firmware(host, 'lumpy')
        self.assertIsNone(fw)

    def test_get_available_firmware_unibuild(self):
        """_get_available_firmware on unibuild board with multiple models."""
        result = utils.CmdResult(stdout=UNIBUILD_VERSION_OUTPUT,
                                 exit_status=0)
        host = mock.Mock()
        host.run.return_value = result

        fw = cros_firmware._get_available_firmware(host, 'astronaut')
        self.assertEqual(fw, 'Google_Coral.10068.37.0')

        fw = cros_firmware._get_available_firmware(host, 'blue')
        self.assertEqual(fw, 'Google_Coral.10068.39.0')

        fw = cros_firmware._get_available_firmware(host, 'bruce')
        self.assertIsNone(fw)


if __name__ == '__main__':
    unittest.main()
