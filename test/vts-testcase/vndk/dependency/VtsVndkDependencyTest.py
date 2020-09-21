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
import re
import shutil
import tempfile

from vts.runners.host import asserts
from vts.runners.host import base_test
from vts.runners.host import keys
from vts.runners.host import test_runner
from vts.runners.host import utils
from vts.testcases.vndk.golden import vndk_data
from vts.utils.python.controllers import android_device
from vts.utils.python.file import target_file_utils
from vts.utils.python.library import elf_parser
from vts.utils.python.os import path_utils
from vts.utils.python.vndk import vndk_utils


class VtsVndkDependencyTest(base_test.BaseTestClass):
    """A test case to verify vendor library dependency.

    Attributes:
        data_file_path: The path to VTS data directory.
        _dut: The AndroidDevice under test.
        _temp_dir: The temporary directory to which the odm and vendor
                   partitions are copied.
        _ll_ndk: Set of strings. The names of low-level NDK libraries in
                 /system/lib[64].
        _sp_hal: List of patterns. The names of the same-process HAL libraries
                 expected to be in /vendor/lib[64].
        _vndk: Set of strings. The names of VNDK core libraries in
               /system/lib[64]/vndk-${VER}.
        _vndk_sp: Set of strings. The names of VNDK-SP libraries in
                  /system/lib[64]/vndk-sp-${VER}.
        _SP_HAL_LINK_PATHS: Format strings of same-process HAL's link paths.
        _VENDOR_LINK_PATHS: Format strings of vendor processes' link paths.
    """
    _TARGET_ROOT_DIR = "/"
    _TARGET_ODM_DIR = "/odm"
    _TARGET_VENDOR_DIR = "/vendor"

    _SP_HAL_LINK_PATHS = [
        "/odm/{LIB}/egl", "/odm/{LIB}/hw", "/odm/{LIB}",
        "/vendor/{LIB}/egl", "/vendor/{LIB}/hw", "/vendor/{LIB}"
    ]
    _VENDOR_LINK_PATHS = [
        "/odm/{LIB}/hw", "/odm/{LIB}/egl", "/odm/{LIB}",
        "/vendor/{LIB}/hw", "/vendor/{LIB}/egl", "/vendor/{LIB}"
    ]

    class ElfObject(object):
        """Contains dependencies of an ELF file on target device.

        Attributes:
            target_path: String. The path to the ELF file on target.
            name: String. File name of the ELF.
            target_dir: String. The directory containing the ELF file on target.
            bitness: Integer. Bitness of the ELF.
            deps: List of strings. The names of the depended libraries.
        """

        def __init__(self, target_path, bitness, deps):
            self.target_path = target_path
            self.name = path_utils.TargetBaseName(target_path)
            self.target_dir = path_utils.TargetDirName(target_path)
            self.bitness = bitness
            self.deps = deps

    def setUpClass(self):
        """Initializes device, temporary directory, and VNDK lists."""
        required_params = [keys.ConfigKeys.IKEY_DATA_FILE_PATH]
        self.getUserParams(required_params)
        self._dut = self.android_devices[0]
        self._temp_dir = tempfile.mkdtemp()
        for target_dir in (self._TARGET_ODM_DIR, self._TARGET_VENDOR_DIR):
            if target_file_utils.IsDirectory(target_dir, self._dut.shell):
                logging.info("adb pull %s %s", target_dir, self._temp_dir)
                self._dut.adb.pull(target_dir, self._temp_dir)
            else:
                logging.info("Skip adb pull %s", target_dir)

        vndk_lists = vndk_data.LoadVndkLibraryLists(
            self.data_file_path,
            self._dut.vndk_version,
            vndk_data.SP_HAL,
            vndk_data.LL_NDK,
            vndk_data.VNDK,
            vndk_data.VNDK_SP)
        asserts.assertTrue(vndk_lists, "Cannot load VNDK library lists.")

        sp_hal_strings = vndk_lists[0]
        self._sp_hal = [re.compile(x) for x in sp_hal_strings]
        (self._ll_ndk,
         self._vndk,
         self._vndk_sp) = (
            set(path_utils.TargetBaseName(path) for path in vndk_list)
            for vndk_list in vndk_lists[1:])

        logging.debug("LL_NDK: %s", self._ll_ndk)
        logging.debug("SP_HAL: %s", sp_hal_strings)
        logging.debug("VNDK: %s", self._vndk)
        logging.debug("VNDK_SP: %s", self._vndk_sp)

    def tearDownClass(self):
        """Deletes the temporary directory."""
        logging.info("Delete %s", self._temp_dir)
        shutil.rmtree(self._temp_dir)

    def _LoadElfObjects(self, host_dir, target_dir, abi_list,
                        elf_error_handler):
        """Scans a host directory recursively and loads all ELF files in it.

        Args:
            host_dir: The host directory to scan.
            target_dir: The path from which host_dir is copied.
            abi_list: A list of strings, the ABIs of the ELF files to load.
            elf_error_handler: A function that takes 2 arguments
                               (target_path, exception). It is called when
                               the parser fails to read an ELF file.

        Returns:
            List of ElfObject.
        """
        objs = []
        for root_dir, file_name in utils.iterate_files(host_dir):
            full_path = os.path.join(root_dir, file_name)
            rel_path = os.path.relpath(full_path, host_dir)
            target_path = path_utils.JoinTargetPath(
                target_dir, *rel_path.split(os.path.sep))
            try:
                elf = elf_parser.ElfParser(full_path)
            except elf_parser.ElfError:
                logging.debug("%s is not an ELF file", target_path)
                continue
            if not any(elf.MatchCpuAbi(x) for x in abi_list):
                logging.debug("%s does not match the ABI", target_path)
                elf.Close()
                continue
            try:
                deps = elf.ListDependencies()
            except elf_parser.ElfError as e:
                elf_error_handler(target_path, e)
                continue
            finally:
                elf.Close()

            logging.info("%s depends on: %s", target_path, ", ".join(deps))
            objs.append(self.ElfObject(target_path, elf.bitness, deps))
        return objs

    def _DfsDependencies(self, lib, searched, searchable):
        """Depth-first-search for library dependencies.

        Args:
            lib: ElfObject. The library to search dependencies.
            searched: The set of searched libraries.
            searchable: The dictionary that maps file names to libraries.
        """
        if lib in searched:
            return
        searched.add(lib)
        for dep_name in lib.deps:
            if dep_name in searchable:
                self._DfsDependencies(searchable[dep_name], searched,
                                      searchable)

    def _FindLibsInSpHalNamespace(self, bitness, objs):
        """Finds libraries in SP-HAL link paths.

        Args:
            bitness: 32 or 64, the bitness of the returned libraries.
            objs: List of ElfObject, the libraries/executables in odm and
                  vendor partitions.

        Returns:
            Dict of {string: ElfObject}, the library name and the first library
            in SP-HAL link paths.
        """
        sp_hal_link_paths = [vndk_utils.FormatVndkPath(x, bitness) for
                             x in self._SP_HAL_LINK_PATHS]
        vendor_libs = [obj for obj in objs if
                       obj.bitness == bitness and
                       obj.target_dir in sp_hal_link_paths]
        linkable_libs = dict()
        for obj in vendor_libs:
            if obj.name not in linkable_libs:
                linkable_libs[obj.name] = obj
            else:
                linkable_libs[obj.name] = min(
                    linkable_libs[obj.name], obj,
                    key=lambda x: sp_hal_link_paths.index(x.target_dir))
        return linkable_libs

    def _FilterDisallowedDependencies(self, objs, is_allowed_dependency):
        """Returns libraries with disallowed dependencies.

        Args:
            objs: A collection of ElfObject, the libraries/executables.
            is_allowed_dependency: A function that takes the library name as the
                                   argument and returns whether objs can depend
                                   on the library.

        Returns:
            List of tuples (path, disallowed_dependencies). The library with
            disallowed dependencies and list of the dependencies.
        """
        dep_errors = []
        for obj in objs:
            disallowed_libs = [
                x for x in obj.deps if not is_allowed_dependency(x)]
            if disallowed_libs:
                dep_errors.append((obj.target_path, disallowed_libs))
        return dep_errors

    def _TestVendorDependency(self, vendor_objs, vendor_libs):
        """Tests if vendor libraries/executables have disallowed dependencies.

        A vendor library/executable is allowed to depend on
        - LL-NDK
        - VNDK
        - VNDK-SP
        - Other libraries in vendor link paths.

        Args:
            vendor_objs: Collection of ElfObject, the libraries/executables in
                         odm and vendor partitions, excluding VNDK-SP extension
                         and SP-HAL.
            vendor_libs: Set of ElfObject, the libraries in vendor link paths,
                         including SP-HAL.

        Returns:
            List of tuples (path, disallowed_dependencies).
        """
        vendor_lib_names = set(x.name for x in vendor_libs)
        is_allowed_dep = lambda x: (x in self._ll_ndk or
                                    x in self._vndk or
                                    x in self._vndk_sp or
                                    x in vendor_lib_names)
        return self._FilterDisallowedDependencies(vendor_objs, is_allowed_dep)

    def _TestVndkSpExtDependency(self, vndk_sp_ext_deps, vendor_libs):
        """Tests if VNDK-SP extension libraries have disallowed dependencies.

        A VNDK-SP extension library/dependency is allowed to depend on
        - LL-NDK
        - VNDK-SP
        - Libraries in vendor link paths
        - Other VNDK-SP extension libraries, which is a subset of VNDK-SP

        However, it is not allowed to indirectly depend on VNDK. i.e., the
        depended vendor libraries must not depend on VNDK.

        Args:
            vndk_sp_ext_deps: Collection of ElfObject, the VNDK-SP extension
                              libraries and dependencies.
            vendor_libs: Set of ElfObject, the libraries in vendor link paths.

        Returns:
            List of tuples (path, disallowed_dependencies).
        """
        vendor_lib_names = set(x.name for x in vendor_libs)
        is_allowed_dep = lambda x: (x in self._ll_ndk or
                                    x in self._vndk_sp or
                                    x in vendor_lib_names)
        return self._FilterDisallowedDependencies(
            vndk_sp_ext_deps, is_allowed_dep)

    def _TestSpHalDependency(self, sp_hal_libs):
        """Tests if SP-HAL libraries have disallowed dependencies.

        A same-process HAL library is allowed to depend on
        - LL-NDK
        - VNDK-SP
        - Other same-process HAL libraries and dependencies

        Args:
            sp_hal_libs: Set of ElfObject, the Same-process HAL libraries and
                         the dependencies.

        Returns:
            List of tuples (path, disallowed_dependencies).
        """
        sp_hal_lib_names = set(x.name for x in sp_hal_libs)
        is_allowed_dep = lambda x: (x in self._ll_ndk or
                                    x in self._vndk_sp or
                                    x in sp_hal_lib_names)
        return self._FilterDisallowedDependencies(sp_hal_libs, is_allowed_dep)

    def _TestElfDependency(self, bitness, objs):
        """Tests vendor libraries/executables and SP-HAL dependencies.

        Args:
            bitness: 32 or 64, the bitness of the vendor libraries.
            objs: List of ElfObject. The libraries/executables in odm and
                  vendor partitions.

        Returns:
            List of tuples (path, disallowed_dependencies).
        """
        vndk_sp_ext_dirs = vndk_utils.GetVndkSpExtDirectories(bitness)
        vendor_link_paths = [vndk_utils.FormatVndkPath(x, bitness) for
                             x in self._VENDOR_LINK_PATHS]

        vendor_libs = set(obj for obj in objs if
                          obj.bitness == bitness and
                          obj.target_dir in vendor_link_paths)
        logging.info("%d-bit odm and vendor libraries including SP-HAL: %s",
                     bitness, ", ".join(x.name for x in vendor_libs))

        sp_hal_namespace = self._FindLibsInSpHalNamespace(bitness, objs)

        # Find same-process HAL and dependencies
        sp_hal_libs = set()
        for obj in sp_hal_namespace.itervalues():
            if any(x.match(obj.target_path) for x in self._sp_hal):
                self._DfsDependencies(obj, sp_hal_libs, sp_hal_namespace)
        logging.info("%d-bit SP-HAL libraries: %s",
                     bitness, ", ".join(x.name for x in sp_hal_libs))

        # Find VNDK-SP extension libraries and their dependencies.
        vndk_sp_ext_libs = set(obj for obj in objs if
                               obj.bitness == bitness and
                               obj.target_dir in vndk_sp_ext_dirs)
        vndk_sp_ext_deps = set()
        for lib in vndk_sp_ext_libs:
            self._DfsDependencies(lib, vndk_sp_ext_deps, sp_hal_namespace)
        logging.info("%d-bit VNDK-SP extension libraries and dependencies: %s",
                     bitness, ", ".join(x.name for x in vndk_sp_ext_deps))

        vendor_objs = {obj for obj in objs if
                       obj.bitness == bitness and
                       obj not in sp_hal_libs and
                       obj not in vndk_sp_ext_deps}
        dep_errors = self._TestVendorDependency(vendor_objs, vendor_libs)

        # vndk_sp_ext_deps and sp_hal_libs may overlap. Their dependency
        # restrictions are the same.
        dep_errors.extend(self._TestVndkSpExtDependency(
            vndk_sp_ext_deps - sp_hal_libs, vendor_libs))

        if not vndk_utils.IsVndkRuntimeEnforced(self._dut):
            logging.warning("Ignore dependency errors: %s", dep_errors)
            dep_errors = []

        dep_errors.extend(self._TestSpHalDependency(sp_hal_libs))
        return dep_errors

    def testElfDependency(self):
        """Tests vendor libraries/executables and SP-HAL dependencies."""
        read_errors = []
        abi_list = self._dut.getCpuAbiList()
        objs = self._LoadElfObjects(
            self._temp_dir, self._TARGET_ROOT_DIR, abi_list,
            lambda p, e: read_errors.append((p, str(e))))

        dep_errors = self._TestElfDependency(32, objs)
        if self._dut.is64Bit:
            dep_errors.extend(self._TestElfDependency(64, objs))

        if read_errors:
            error_lines = ("%s: %s" % (x[0], x[1]) for x in read_errors)
            logging.error("%d read errors:\n%s",
                          len(read_errors), "\n".join(error_lines))
        if dep_errors:
            error_lines = ("%s: %s" % (x[0], ", ".join(x[1]))
                           for x in dep_errors)
            logging.error("%d disallowed dependencies:\n%s",
                          len(dep_errors), "\n".join(error_lines))
        error_count = len(read_errors) + len(dep_errors)
        asserts.assertEqual(error_count, 0,
                            "Total number of errors: " + str(error_count))


if __name__ == "__main__":
    test_runner.main()
