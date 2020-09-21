# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A collection of classes representing TCPC firmware blobs.
"""

import logging
import os
import subprocess


class ChipUtilsError(Exception):
    """Error in the chip_utils module."""



class generic_chip(object):

    """A chip we don't actually support."""

    chip_name = 'unknown'
    fw_name = None

    def __init__(self):
        self.fw_ver = None
        self.fw_file_name = None

    def set_fw_ver_from_string(self, version):
        """Sets version property from string."""
        self.fw_ver = int(version, 0)

    def set_from_file(self, file_name):
        """Sets chip params from file name.

        The typical firmware blob file name format is: <chip>_0x00.bin

        Args:
            file_name: Firmware blob file name.

        Raises:
            ValueError: Failed to decompose firmware file name.
        """

        basename = os.path.basename(file_name)
        if not basename.startswith(self.chip_name):
            raise ValueError('filename did not start with %s' % self.chip_name)
        fname = basename.split('.')[0]
        if '_' in fname:
            rev = fname.split('_')[-1]
            self.set_fw_ver_from_string(rev)
        else:
            logging.info('No fw ver found in filename %s', basename)
        self.fw_file_name = file_name


class ps8751(generic_chip):

    """The PS8751 TCPC chip."""

    chip_name = 'ps8751'
    fw_name = 'ps8751_a3'
    cbfs_bin_name = fw_name + '.bin'
    cbfs_hash_name = fw_name + '.hash'

    def fw_ver_from_hash(self, blob):
        """Return the firmware version encoded in the firmware hash."""

        return blob[1]

    def compute_hash_bytes(self):
        """Generates the firmware blob hash."""

        if self.fw_ver is None:
            raise ChipUtilsError('fw_ver not initialized')

        h = bytearray(2)
        h[0] = 0xa3
        h[1] = self.fw_ver
        return h


class anx3429(generic_chip):

    """The ANX3429 TCPC chip."""

    chip_name = 'anx3429'
    fw_name = 'anx3429_ocm'
    cbfs_bin_name = fw_name + '.bin'
    cbfs_hash_name = fw_name + '.hash'

    def fw_ver_from_hash(self, blob):
        """Return the firmware version encoded in the firmware hash."""

        return blob[0]

    def compute_hash_bytes(self):
        """Generates the firmware blob hash."""

        if self.fw_ver is None:
            raise ChipUtilsError('fw_ver not initialized')

        h = bytearray(1)
        h[0] = self.fw_ver
        return h


class ecrw(generic_chip):

    """Chrome EC RW portion."""

    chip_name = 'ecrw'
    fw_name = 'ecrw'
    cbfs_bin_name = fw_name
    cbfs_hash_name = fw_name + '.hash'

    def compute_hash_bytes(self):
        """Generates the firmware blob hash."""

        if self.fw_file_name is None:
            raise ChipUtilsError('fw_file_name not initialized')

        if not os.path.exists(self.fw_file_name):
            raise ChipUtilsError('%s does not exist' % self.fw_file_name)

        # openssl outputs the result to stdout
        cmd = 'openssl dgst -sha256 -binary %s' % self.fw_file_name
        return subprocess.check_output(cmd, shell=True)


chip_id_map = {
    '0x8751': ps8751,
    '0x3429': anx3429,
}
