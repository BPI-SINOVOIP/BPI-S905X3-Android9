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

import csv
import logging
import os

# The tags in VNDK spreadsheet:
# Low-level NDK libraries that can be used by framework and vendor modules.
LL_NDK = "LL-NDK"

# LL-NDK dependencies that vendor modules cannot directly access.
LL_NDK_PRIVATE = "LL-NDK-Private"

# Same-process HAL implementation in vendor partition.
SP_HAL = "SP-HAL"

# Framework libraries that can be used by vendor modules except same-process HAL
# and its dependencies in vendor partition.
VNDK = "VNDK"

# VNDK dependencies that vendor modules cannot directly access.
VNDK_PRIVATE = "VNDK-Private"

# Same-process HAL dependencies in framework.
VNDK_SP = "VNDK-SP"

# VNDK-SP dependencies that vendor modules cannot directly access.
VNDK_SP_PRIVATE = "VNDK-SP-Private"

# The ABI dump directories. 64-bit comes before 32-bit in order to sequentially
# search for longest prefix.
_ABI_NAMES = ("arm64", "arm", "mips64", "mips", "x86_64", "x86")

# The data directory.
_GOLDEN_DIR = os.path.join("vts", "testcases", "vndk", "golden")


def LoadDefaultVndkVersion(data_file_path):
    """Loads the name of the data directory for devices with no VNDK version.

    Args:
        data_file_path: The path to VTS data directory.

    Returns:
        A string, the directory name.
        None if fails to load the name.
    """
    try:
        with open(os.path.join(data_file_path, _GOLDEN_DIR,
                               "platform_vndk_version.txt"), "r") as f:
            return f.read().strip()
    except IOError:
        logging.error("Cannot load default VNDK version.")
        return None


def GetAbiDumpDirectory(data_file_path, version, binder_bitness, abi_name,
                        abi_bitness):
    """Returns the VNDK dump directory on host.

    Args:
        data_file_path: The path to VTS data directory.
        version: A string, the VNDK version.
        binder_bitness: A string or an integer, 32 or 64.
        abi_name: A string, the ABI of the library dump.
        abi_bitness: A string or an integer, 32 or 64.

    Returns:
        A string, the path to the dump directory.
        None if there is no directory for the version and ABI.
    """
    try:
        abi_dir = next(x for x in _ABI_NAMES if abi_name.startswith(x))
    except StopIteration:
        logging.warning("Unknown ABI %s.", abi_name)
        return None

    version_dir = (version if version else
                   LoadDefaultVndkVersion(data_file_path))
    if not version_dir:
        return None

    dump_dir = os.path.join(
        data_file_path, _GOLDEN_DIR, version_dir,
        "binder64" if str(binder_bitness) == "64" else "binder32",
        abi_dir, "lib64" if str(abi_bitness) == "64" else "lib")

    if not os.path.isdir(dump_dir):
        logging.warning("%s is not a directory.", dump_dir)
        return None

    return dump_dir


def LoadVndkLibraryLists(data_file_path, version, *tags):
    """Find the VNDK libraries with specific tags.

    Args:
        data_file_path: The path to VTS data directory.
        version: A string, the VNDK version.
        *tags: Strings, the tags of the libraries to find.

    Returns:
        A tuple of lists containing library names. Each list corresponds to
        one tag in the argument. For SP-HAL, the returned names are regular
        expressions.
        None if the spreadsheet for the version is not found.
    """
    version_dir = (version if version else
                   LoadDefaultVndkVersion(data_file_path))
    if not version_dir:
        return None

    path = os.path.join(
        data_file_path, _GOLDEN_DIR, version_dir, "eligible-list.csv")
    if not os.path.isfile(path):
        logging.warning("Cannot load %s.", path)
        return None

    dir_suffix = "-" + version if version else ""
    vndk_lists = tuple([] for x in tags)
    with open(path) as csv_file:
        # Skip header
        next(csv_file)
        reader = csv.reader(csv_file)
        for cells in reader:
            for tag_index, tag in enumerate(tags):
                if tag == cells[1]:
                    lib_name = cells[0].replace("${VNDK_VER}", dir_suffix)
                    if lib_name.startswith("[regex]"):
                        lib_name = lib_name[len("[regex]"):]
                    vndk_lists[tag_index].extend(
                        lib_name.replace("${LIB}", lib)
                        for lib in ("lib", "lib64"))
    return vndk_lists
