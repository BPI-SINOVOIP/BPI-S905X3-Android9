#!/usr/bin/env python
#
# Copyright (C) 2018 The Android Open Source Project
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
"""VTS tests to verify DTBO partition/DT overlay application."""

import logging
import os
import shutil
import struct
import subprocess
import tempfile
import zlib

from vts.runners.host import asserts
from vts.runners.host import base_test
from vts.runners.host import const
from vts.runners.host import test_runner
from vts.utils.python.android import api
from vts.utils.python.file import target_file_utils
from vts.utils.python.os import path_utils

BLOCK_DEV_PATH = "/dev/block/platform"  # path to platform block devices
DEVICE_TEMP_DIR = "/data/local/tmp/"  # temporary dir in device.
FDT_PATH = "/sys/firmware/fdt"  # path to device tree.
PROPERTY_SLOT_SUFFIX = "ro.boot.slot_suffix"  # indicates current slot suffix for A/B devices
DTBO_HEADER_SIZE = 32  # size of dtbo header
DT_HEADER_FLAGS_OFFSET = 16  # offset in DT header to read compression info
COMPRESSION_FLAGS_BIT_MASK = 0x0f  # Bit mask to get compression format from flags field of DT
                                   # header for version 1 DTBO header.
DECOMPRESS_WBIT_ARG = 47  # Automatically accepts either the zlib or gzip format.
FDT_MAGIC = 0xd00dfeed  # FDT Magic


class CompressionFormat(object):
    """Enum representing DT compression format for a DT entry.
    """
    NO_COMPRESSION = 0x00
    ZLIB_COMPRESSION = 0x01
    GZIP_COMPRESSION = 0x02


class VtsFirmwareDtboVerification(base_test.BaseTestClass):
    """Validates DTBO partition and DT overlay application.

    Attributes:
        temp_dir: The temporary directory on host.
        device_path: The temporary directory on device.
    """

    def setUpClass(self):
        """Initializes the DUT and creates temporary directories."""
        self.dut = self.android_devices[0]
        self.shell = self.dut.shell
        self.adb = self.dut.adb
        self.temp_dir = tempfile.mkdtemp()
        logging.info("Create %s", self.temp_dir)
        self.device_path = str(
            path_utils.JoinTargetPath(DEVICE_TEMP_DIR, self.abi_bitness))
        self.shell.Execute("mkdir %s -p" % self.device_path)

    def setUp(self):
        """Checks if the the preconditions to run the test are met."""
        asserts.skipIf("x86" in self.dut.cpu_abi, "Skipping test for x86 ABI")

    def DecompressDTEntries(self, host_dtbo_image, unpacked_dtbo_path):
        """Decompresses DT entries based on the flag field in the DT Entry header.

        Args:
            host_dtbo_image: Path to the DTBO image on host.
            unpacked_dtbo_path: Path where DTBO was unpacked.
        """

        with open(host_dtbo_image, "rb") as file_in:
            buf = file_in.read(DTBO_HEADER_SIZE)
            (magic, total_size, header_size, dt_entry_size, dt_entry_count,
             dt_entries_offset, page_size, version) = struct.unpack_from(">8I", buf, 0)
            if version > 0:
                for dt_entry_idx in range(dt_entry_count):
                    file_in.seek(dt_entries_offset +
                                 DT_HEADER_FLAGS_OFFSET)
                    dt_entries_offset += dt_entry_size
                    flags = struct.unpack(">I", file_in.read(1 * 4))[0]
                    compression_format = flags & COMPRESSION_FLAGS_BIT_MASK
                    if (compression_format not in [CompressionFormat.ZLIB_COMPRESSION,
                                                   CompressionFormat.GZIP_COMPRESSION]):
                        asserts.assertEqual(compression_format, CompressionFormat.NO_COMPRESSION,
                                            "Unknown compression format %d" % compression_format)
                        continue
                    dt_entry_path = "%s.%s" % (unpacked_dtbo_path, dt_entry_idx)
                    logging.info("decompressing %s", dt_entry_path)
                    with open(dt_entry_path, "rb") as f:
                        unzipped_dtbo_file = zlib.decompress(f.read(), DECOMPRESS_WBIT_ARG)
                    with open(dt_entry_path, "r+b") as f:
                        f.write(unzipped_dtbo_file)
                        f.seek(0)
                        fdt_magic = struct.unpack(">I", f.read(1 * 4))[0]
                        asserts.assertEqual(fdt_magic, FDT_MAGIC,
                                            "Bad FDT(Flattened Device Tree) Format")

    def testCheckDTBOPartition(self):
        """Validates DTBO partition using mkdtboimg.py."""
        try:
            slot_suffix = str(self.dut.getProp(PROPERTY_SLOT_SUFFIX))
        except ValueError as e:
            logging.exception(e)
            slot_suffix = ""
        current_dtbo_partition = "dtbo" + slot_suffix
        dtbo_path = target_file_utils.FindFiles(
            self.shell, BLOCK_DEV_PATH, current_dtbo_partition, "-type l")
        logging.info("DTBO path %s", dtbo_path)
        if not dtbo_path:
            asserts.fail("Unable to find path to dtbo image on device.")
        host_dtbo_image = os.path.join(self.temp_dir, "dtbo")
        self.adb.pull("%s %s" % (dtbo_path[0], host_dtbo_image))
        mkdtboimg_bin_path = os.path.join("host", "bin", "mkdtboimg.py")
        unpacked_dtbo_path = os.path.join(self.temp_dir, "dumped_dtbo")
        dtbo_dump_cmd = [
            "python", "%s" % mkdtboimg_bin_path, "dump",
            "%s" % host_dtbo_image, "-b",
            "%s" % unpacked_dtbo_path
        ]
        try:
            subprocess.check_call(dtbo_dump_cmd)
        except Exception as e:
            logging.exception(e)
            logging.error('dtbo_dump_cmd is: %s', dtbo_dump_cmd)
            asserts.fail("Invalid DTBO Image")
        # TODO(b/109892148) Delete code below once decompress option is enabled for mkdtboimg.py
        self.DecompressDTEntries(host_dtbo_image, unpacked_dtbo_path)

    def testVerifyOverlay(self):
        """Verifies application of DT overlays."""
        overlay_idx_string = self.adb.shell(
            "cat /proc/cmdline | "
            "grep -o \"androidboot.dtbo_idx=[^ ]*\" |"
            "cut -d \"=\" -f 2")
        asserts.assertNotEqual(
            len(overlay_idx_string), 0,
            "Kernel command line missing androidboot.dtbo_idx")
        overlay_idx_list = overlay_idx_string.split(",")
        overlay_arg = []
        for idx in overlay_idx_list:
            overlay_file = "dumped_dtbo." + idx.rstrip()
            overlay_path = os.path.join(self.temp_dir, overlay_file)
            self.adb.push(overlay_path, self.device_path)
            overlay_arg.append(overlay_file)
        final_dt_path = path_utils.JoinTargetPath(self.device_path, "final_dt")
        self.shell.Execute("cp %s %s" % (FDT_PATH, final_dt_path))
        verification_test_path = path_utils.JoinTargetPath(
            self.device_path, "ufdt_verify_overlay")
        chmod_cmd = "chmod 755 %s" % verification_test_path
        results = self.shell.Execute(chmod_cmd)
        asserts.assertEqual(results[const.EXIT_CODE][0], 0, "Unable to chmod")
        cd_cmd = "cd %s" % (self.device_path)
        verify_cmd = "./ufdt_verify_overlay final_dt %s" % (
            " ".join(overlay_arg))
        cmd = str("%s && %s" % (cd_cmd, verify_cmd))
        logging.info(cmd)
        results = self.shell.Execute(cmd)
        asserts.assertEqual(results[const.EXIT_CODE][0], 0,
                            "Incorrect Overlay Application")

    def tearDownClass(self):
        """Deletes temporary directories."""
        shutil.rmtree(self.temp_dir)
        self.shell.Execute("rm -rf %s" % self.device_path)


if __name__ == "__main__":
    test_runner.main()
