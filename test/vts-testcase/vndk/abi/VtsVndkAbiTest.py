#!/usr/bin/env python
#
# Copyright (C) 2017 The Android Open Source Project
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

import logging
import os
import shutil
import tempfile

from vts.runners.host import asserts
from vts.runners.host import base_test
from vts.runners.host import const
from vts.runners.host import keys
from vts.runners.host import test_runner
from vts.testcases.vndk.golden import vndk_data
from vts.utils.python.controllers import android_device
from vts.utils.python.library import elf_parser
from vts.utils.python.library import vtable_parser


def _IterateFiles(root_dir):
    """A generator yielding relative and full paths in a directory.

    Args:
        root_dir: The directory to search.

    Yields:
        A tuple of (relative_path, full_path) for each regular file.
        relative_path is the relative path to root_dir. full_path is the path
        starting with root_dir.
    """
    for dir_path, dir_names, file_names in os.walk(root_dir):
        if dir_path == root_dir:
            rel_dir = ""
        else:
            rel_dir = os.path.relpath(dir_path, root_dir)
        for file_name in file_names:
            yield (os.path.join(rel_dir, file_name),
                   os.path.join(dir_path, file_name))


class VtsVndkAbiTest(base_test.BaseTestClass):
    """A test module to verify ABI compliance of vendor libraries.

    Attributes:
        _dut: the AndroidDevice under test.
        _temp_dir: The temporary directory for libraries copied from device.
        _vndk_version: String, the VNDK version supported by the device.
        data_file_path: The path to VTS data directory.
    """
    _ODM_LIB_DIR_32 = "/odm/lib"
    _ODM_LIB_DIR_64 = "/odm/lib64"
    _VENDOR_LIB_DIR_32 = "/vendor/lib"
    _VENDOR_LIB_DIR_64 = "/vendor/lib64"
    _SYSTEM_LIB_DIR_32 = "/system/lib"
    _SYSTEM_LIB_DIR_64 = "/system/lib64"

    def setUpClass(self):
        """Initializes data file path, device, and temporary directory."""
        required_params = [keys.ConfigKeys.IKEY_DATA_FILE_PATH]
        self.getUserParams(required_params)
        self._dut = self.android_devices[0]
        self._temp_dir = tempfile.mkdtemp()
        self._vndk_version = self._dut.vndk_version

    def tearDownClass(self):
        """Deletes the temporary directory."""
        logging.info("Delete %s", self._temp_dir)
        shutil.rmtree(self._temp_dir)

    def _PullOrCreateDir(self, target_dir, host_dir):
        """Copies a directory from device. Creates an empty one if not exist.

        Args:
            target_dir: The directory to copy from device.
            host_dir: The directory to copy to host.
        """
        test_cmd = "test -d " + target_dir
        logging.info("adb shell %s", test_cmd)
        result = self._dut.adb.shell(test_cmd, no_except=True)
        if result[const.EXIT_CODE]:
            logging.info("%s doesn't exist. Create %s.", target_dir, host_dir)
            os.mkdir(host_dir, 0750)
            return
        logging.info("adb pull %s %s", target_dir, host_dir)
        self._dut.adb.pull(target_dir, host_dir)

    def _DiffSymbols(self, dump_path, lib_path):
        """Checks if a library includes all symbols in a dump.

        Args:
            dump_path: The path to the dump file containing list of symbols.
            lib_path: The path to the library.

        Returns:
            A list of strings, the global symbols that are in the dump but not
            in the library.

        Raises:
            IOError if fails to load the dump.
            elf_parser.ElfError if fails to load the library.
        """
        with open(dump_path, "r") as dump_file:
            dump_symbols = set(line.strip() for line in dump_file
                               if line.strip())
        parser = elf_parser.ElfParser(lib_path)
        try:
            lib_symbols = parser.ListGlobalDynamicSymbols(include_weak=True)
        finally:
            parser.Close()
        return sorted(dump_symbols.difference(lib_symbols))

    def _DiffVtables(self, dump_path, lib_path):
        """Checks if a library includes all vtable entries in a dump.

        Args:
            dump_path: The path to the dump file containing vtables.
            lib_path: The path to the library.

        Returns:
            A list of tuples (VTABLE, SYMBOL, EXPECTED_OFFSET, ACTUAL_OFFSET).
            ACTUAL_OFFSET can be "missing" or numbers separated by comma.

        Raises:
            IOError if fails to load the dump.
            vtable_parser.VtableError if fails to load the library.
        """
        parser = vtable_parser.VtableParser(
            os.path.join(self.data_file_path, "host"))
        with open(dump_path, "r") as dump_file:
            dump_vtables = parser.ParseVtablesFromString(dump_file.read())

        lib_vtables = parser.ParseVtablesFromLibrary(lib_path)
        # TODO(b/78316564): The dumper doesn't support SHT_ANDROID_RELA.
        if not lib_vtables and self.run_as_compliance_test:
            logging.warning("%s: Cannot dump vtables",
                            os.path.relpath(lib_path, self._temp_dir))
            return []
        logging.debug("%s: %s", lib_path, lib_vtables)
        diff = []
        for vtable, dump_symbols in dump_vtables.iteritems():
            lib_inv_vtable = dict()
            if vtable in lib_vtables:
                for off, sym in lib_vtables[vtable]:
                    if sym not in lib_inv_vtable:
                        lib_inv_vtable[sym] = [off]
                    else:
                        lib_inv_vtable[sym].append(off)
            for off, sym in dump_symbols:
                if sym not in lib_inv_vtable:
                    diff.append((vtable, sym, str(off), "missing"))
                elif off not in lib_inv_vtable[sym]:
                    diff.append((vtable, sym, str(off),
                                 ",".join(str(x) for x in lib_inv_vtable[sym])))
        return diff

    def _ScanLibDirs(self, dump_dir, lib_dirs, dump_version):
        """Compares dump files with libraries copied from device.

        Args:
            dump_dir: The directory containing dump files.
            lib_dirs: The list of directories containing libraries.
            dump_version: The VNDK version of the dump files. If the device has
                          no VNDK version or has extension in vendor partition,
                          this method compares the unversioned VNDK directories
                          with the dump directories of the given version.

        Returns:
            An integer, number of incompatible libraries.
        """
        error_count = 0
        symbol_dumps = dict()
        vtable_dumps = dict()
        lib_paths = dict()
        for dump_rel_path, dump_path in _IterateFiles(dump_dir):
            if dump_rel_path.endswith("_symbol.dump"):
                lib_name = dump_rel_path.rpartition("_symbol.dump")[0]
                symbol_dumps[lib_name] = dump_path
            elif dump_rel_path.endswith("_vtable.dump"):
                lib_name = dump_rel_path.rpartition("_vtable.dump")[0]
                vtable_dumps[lib_name] = dump_path
            else:
                logging.warning("Unknown dump: %s", dump_path)
                continue
            lib_paths[lib_name] = None

        for lib_dir in lib_dirs:
            for lib_rel_path, lib_path in _IterateFiles(lib_dir):
                try:
                    vndk_dir = next(x for x in ("vndk", "vndk-sp") if
                                    lib_rel_path.startswith(x + os.path.sep))
                    lib_name = lib_rel_path.replace(
                        vndk_dir, vndk_dir + "-" + dump_version, 1)
                except StopIteration:
                    lib_name = lib_rel_path

                if lib_name in lib_paths and not lib_paths[lib_name]:
                    lib_paths[lib_name] = lib_path

        for lib_name, lib_path in lib_paths.iteritems():
            if not lib_path:
                logging.info("%s: Not found on target", lib_name)
                continue
            rel_path = os.path.relpath(lib_path, self._temp_dir)

            has_exception = False
            missing_symbols = []
            vtable_diff = []
            # Compare symbols
            if lib_name in symbol_dumps:
                try:
                    missing_symbols = self._DiffSymbols(
                        symbol_dumps[lib_name], lib_path)
                except (IOError, elf_parser.ElfError):
                    logging.exception("%s: Cannot diff symbols", rel_path)
                    has_exception = True
            # Compare vtables
            if lib_name in vtable_dumps:
                try:
                    vtable_diff = self._DiffVtables(
                        vtable_dumps[lib_name], lib_path)
                except (IOError, vtable_parser.VtableError):
                    logging.exception("%s: Cannot diff vtables", rel_path)
                    has_exception = True

            if missing_symbols:
                logging.error("%s: Missing Symbols:\n%s",
                              rel_path, "\n".join(missing_symbols))
            if vtable_diff:
                logging.error("%s: Vtable Difference:\n"
                              "vtable symbol expected actual\n%s",
                              rel_path,
                              "\n".join(" ".join(x) for x in vtable_diff))
            if has_exception or missing_symbols or vtable_diff:
                error_count += 1
            else:
                logging.info("%s: Pass", rel_path)
        return error_count

    def testAbiCompatibility(self):
        """Checks ABI compliance of VNDK libraries."""
        primary_abi = self._dut.getCpuAbiList()[0]
        binder_bitness = self._dut.getBinderBitness()
        asserts.assertTrue(binder_bitness,
                           "Cannot determine binder bitness.")
        dump_version = (self._vndk_version if self._vndk_version else
                        vndk_data.LoadDefaultVndkVersion(self.data_file_path))
        asserts.assertTrue(dump_version,
                           "Cannot load default VNDK version.")

        dump_dir = vndk_data.GetAbiDumpDirectory(
            self.data_file_path,
            dump_version,
            binder_bitness,
            primary_abi,
            self.abi_bitness)
        asserts.assertTrue(
            dump_dir,
            "No dump files. version: %s ABI: %s bitness: %s" % (
                self._vndk_version, primary_abi, self.abi_bitness))
        logging.info("dump dir: %s", dump_dir)

        odm_lib_dir = os.path.join(
            self._temp_dir, "odm_lib_dir_" + self.abi_bitness)
        vendor_lib_dir = os.path.join(
            self._temp_dir, "vendor_lib_dir_" + self.abi_bitness)
        system_lib_dir = os.path.join(
            self._temp_dir, "system_lib_dir_" + self.abi_bitness)
        logging.info("host lib dir: %s %s %s",
                     odm_lib_dir, vendor_lib_dir, system_lib_dir)
        self._PullOrCreateDir(
            getattr(self, "_ODM_LIB_DIR_" + self.abi_bitness),
            odm_lib_dir)
        self._PullOrCreateDir(
            getattr(self, "_VENDOR_LIB_DIR_" + self.abi_bitness),
            vendor_lib_dir)
        self._PullOrCreateDir(
            getattr(self, "_SYSTEM_LIB_DIR_" + self.abi_bitness),
            system_lib_dir)

        error_count = self._ScanLibDirs(
            dump_dir, [odm_lib_dir, vendor_lib_dir, system_lib_dir], dump_version)
        asserts.assertEqual(error_count, 0,
                            "Total number of errors: " + str(error_count))


if __name__ == "__main__":
    test_runner.main()
