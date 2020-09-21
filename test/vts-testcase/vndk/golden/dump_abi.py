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

import argparse
import csv
import importlib
import os
import subprocess
import sys


class ExternalModules(object):
    """This class imports modules dynamically and keeps them as attributes.

    Assume the user runs this script in the source directory. The VTS modules
    are outside the search path and thus have to be imported dynamically.

    Attribtues:
        ar_parser: The ar_parser module.
        elf_parser: The elf_parser module.
        vtable_parser: The vtable_parser module.
    """
    @classmethod
    def ImportParsers(cls, import_dir):
        """Imports elf_parser and vtable_parser.

        Args:
            import_dir: The directory containing vts.utils.python.library.*.
        """
        sys.path.append(import_dir)
        cls.ar_parser = importlib.import_module(
            "vts.utils.python.library.ar_parser")
        cls.elf_parser = importlib.import_module(
            "vts.utils.python.library.elf_parser")
        cls.vtable_parser = importlib.import_module(
            "vts.utils.python.library.vtable_parser")


def _CreateAndWrite(path, data):
    """Creates directories on a file path and writes data to it.

    Args:
        path: The path to the file.
        data: The data to write.
    """
    dir_name = os.path.dirname(path)
    if dir_name and not os.path.exists(dir_name):
        os.makedirs(dir_name)
    with open(path, "w") as f:
        f.write(data)


def _ExecuteCommand(cmd, **kwargs):
    """Executes a command and returns stdout.

    Args:
        cmd: A list of strings, the command to execute.
        **kwargs: The arguments passed to subprocess.Popen.

    Returns:
        A string, the stdout.
    """
    proc = subprocess.Popen(
        cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, **kwargs)
    stdout, stderr = proc.communicate()
    if proc.returncode:
        sys.exit("Command failed: %s\nstdout=%s\nstderr=%s" % (
                 cmd, stdout, stderr))
    if stderr:
        print("Warning: cmd=%s\nstdout=%s\nstderr=%s" % (cmd, stdout, stderr))
    return stdout.strip()


def GetBuildVariables(build_top_dir, abs_path, vars):
    """Gets values of variables from build config.

    Args:
        build_top_dir: The path to root directory of Android source.
        abs_path: A boolean, whether to convert the values to absolute paths.
        vars: A list of strings, the names of the variables.

    Returns:
        A list of strings which are the values of the variables.
    """
    cmd = ["build/soong/soong_ui.bash", "--dumpvars-mode",
           ("--abs-vars" if abs_path else "--vars"), " ".join(vars)]
    stdout = _ExecuteCommand(cmd, cwd=build_top_dir)
    print(stdout)
    return [line.split("=", 1)[1].strip("'") for line in stdout.splitlines()]


def FindBinary(file_name):
    """Finds an executable binary in environment variable PATH.

    Args:
        file_name: The file name to find.

    Returns:
        A string which is the path to the binary.
    """
    return _ExecuteCommand(["which", file_name])


def DumpSymbols(lib_path, dump_path, exclude_symbols):
    """Dump symbols from a library to a dump file.

    The dump file is a sorted list of symbols. Each line contains one symbol.

    Args:
        lib_path: The path to the library.
        dump_path: The path to the dump file.
        exclude_symbols: A set of strings, the symbols that should not be
                         written to the dump file.

    Returns:
        A list of strings which are the symbols written to the dump file.

    Raises:
        elf_parser.ElfError if fails to load the library.
        IOError if fails to write to the dump.
    """
    elf_parser = ExternalModules.elf_parser
    parser = None
    try:
        parser = elf_parser.ElfParser(lib_path)
        symbols = [x for x in parser.ListGlobalDynamicSymbols()
                   if x not in exclude_symbols]
    finally:
        if parser:
            parser.Close()
    if symbols:
        symbols.sort()
        _CreateAndWrite(dump_path, "\n".join(symbols) + "\n")
    return symbols


def DumpVtables(lib_path, dump_path, dumper_dir, include_symbols):
    """Dump vtables from a library to a dump file.

    The dump file is the raw output of vndk-vtable-dumper.

    Args:
        lib_path: The path to the library.
        dump_path: The path to the text file.
        dumper_dir: The path to the directory containing the dumper executable
                    and library.
        include_symbols: A set of strings. A vtable is written to the dump file
                         only if its symbol is in the set.

    Returns:
        A string which is the content written to the dump file.

    Raises:
        vtable_parser.VtableError if fails to load the library.
        IOError if fails to write to the dump.
    """
    vtable_parser = ExternalModules.vtable_parser
    parser = vtable_parser.VtableParser(dumper_dir)

    def GenerateLines():
        for line in parser.CallVtableDumper(lib_path).split("\n"):
            parsed_lines.append(line)
            yield line

    lines = GenerateLines()
    dump_lines = []
    try:
        while True:
            parsed_lines = []
            vtable, entries = parser.ParseOneVtable(lines)
            if vtable in include_symbols:
                dump_lines.extend(parsed_lines)
    except StopIteration:
        pass

    dump_string = "\n".join(dump_lines).strip("\n")
    if dump_string:
        dump_string += "\n"
        _CreateAndWrite(dump_path, dump_string)
    return dump_string


def _LoadLibraryNamesFromCsv(csv_file):
    """Loads VNDK and VNDK-SP library names from an eligible list.

    Args:
        csv_file: A file object of eligible-list.csv.

    Returns:
        A list of strings, the VNDK and VNDK-SP library names with vndk/vndk-sp
        directory prefixes.
    """
    lib_names = []
    # Skip header
    next(csv_file)
    reader = csv.reader(csv_file)
    for cells in reader:
        if cells[1] not in ("VNDK", "VNDK-SP"):
            continue
        lib_name = os.path.normpath(cells[0]).replace("/system/${LIB}/", "", 1)
        if not (lib_name.startswith("vndk") or lib_name.startswith("vndk-sp")):
            continue
        lib_name = lib_name.replace("${VNDK_VER}", "{VNDK_VER}")
        lib_names.append(lib_name)
    return lib_names


def _LoadLibraryNames(file_names):
    """Loads library names from files.

    Each element in the input list can be a .so file, a text file, or a .csv
    file. The returned list consists of:
    - The .so file names in the input list.
    - The non-empty lines in the text files.
    - The libraries tagged with VNDK or VNDK-SP in the CSV.

    Args:
        file_names: A list of strings, the library or text file names.

    Returns:
        A list of strings, the library names (probably with vndk/vndk-sp
        directory prefixes).
    """
    lib_names = []
    for file_name in file_names:
        if file_name.endswith(".so"):
            lib_names.append(file_name)
        elif file_name.endswith(".csv"):
            with open(file_name, "r") as csv_file:
                lib_names.extend(_LoadLibraryNamesFromCsv(csv_file))
        else:
            with open(file_name, "r") as lib_list:
                lib_names.extend(line.strip() for line in lib_list
                                 if line.strip())
    return lib_names


def DumpAbi(output_dir, lib_names, lib_dir, object_dir, dumper_dir):
    """Generates dump from libraries.

    Args:
        output_dir: The output directory of dump files.
        lib_names: The names of the libraries to dump.
        lib_dir: The path to the directory containing the libraries to dump.
        object_dir: The path to the directory containing intermediate objects.
        dumper_dir: The path to the directory containing the vtable dumper
                    executable and library.

    Returns:
        A list of strings, the paths to the libraries not found in lib_dir.
    """
    ar_parser = ExternalModules.ar_parser
    static_symbols = set()
    for ar_name in ("libgcc", "libatomic", "libcompiler_rt-extras"):
        ar_path = os.path.join(
            object_dir, "STATIC_LIBRARIES", ar_name + "_intermediates",
            ar_name + ".a")
        static_symbols.update(ar_parser.ListGlobalSymbols(ar_path))

    missing_libs = []
    dump_dir = os.path.join(output_dir, os.path.basename(lib_dir))
    for lib_name in lib_names:
        lib_path = os.path.join(lib_dir, lib_name)
        symbol_dump_path = os.path.join(dump_dir, lib_name + "_symbol.dump")
        vtable_dump_path = os.path.join(dump_dir, lib_name + "_vtable.dump")
        print(lib_path)
        if not os.path.isfile(lib_path):
            missing_libs.append(lib_path)
            print("Warning: Not found")
            print("")
            continue
        symbols = DumpSymbols(lib_path, symbol_dump_path, static_symbols)
        if symbols:
            print("Output: " + symbol_dump_path)
        else:
            print("No symbols")
        vtables = DumpVtables(
            lib_path, vtable_dump_path, dumper_dir, set(symbols))
        if vtables:
            print("Output: " + vtable_dump_path)
        else:
            print("No vtables")
        print("")
    return missing_libs


def main():
    # Parse arguments
    arg_parser = argparse.ArgumentParser()
    arg_parser.add_argument("file", nargs="*",
                            help="the libraries to dump. Each file can be "
                                 ".so, text, or .csv. The text file contains "
                                 "a list of paths. The CSV file follows the"
                                 "format of eligible list output from VNDK"
                                 "definition tool. {VNDK_VER} in the library"
                                 "paths is replaced with "
                                 "\"-\" + PLATFORM_VNDK_VERSION.")
    arg_parser.add_argument("--dumper-dir", "-d", action="store",
                            help="the path to the directory containing "
                                 "bin/vndk-vtable-dumper.")
    arg_parser.add_argument("--import-path", "-i", action="store",
                            help="the directory for VTS python modules. "
                                 "Default value is $ANDROID_BUILD_TOP/test")
    arg_parser.add_argument("--output", "-o", action="store",
                            help="output directory for ABI reference dump. "
                                 "Default value is PLATFORM_VNDK_VERSION.")
    args = arg_parser.parse_args()

    # Get target architectures
    build_top_dir = os.getenv("ANDROID_BUILD_TOP")
    if not build_top_dir:
        sys.exit("env var ANDROID_BUILD_TOP is not set")

    (binder_32_bit,
     vndk_version,
     target_arch,
     target_2nd_arch) = GetBuildVariables(
        build_top_dir, abs_path=False, vars=(
            "BINDER32BIT",
            "PLATFORM_VNDK_VERSION",
            "TARGET_ARCH",
            "TARGET_2ND_ARCH"))

    (target_lib_dir,
     target_obj_dir,
     target_2nd_lib_dir,
     target_2nd_obj_dir) = GetBuildVariables(
        build_top_dir, abs_path=True, vars=(
            "TARGET_OUT_SHARED_LIBRARIES",
            "TARGET_OUT_INTERMEDIATES",
            "2ND_TARGET_OUT_SHARED_LIBRARIES",
            "2ND_TARGET_OUT_INTERMEDIATES"))

    # Import elf_parser and vtable_parser
    ExternalModules.ImportParsers(args.import_path if args.import_path else
                                  os.path.join(build_top_dir, "test"))

    # Find vtable dumper
    if args.dumper_dir:
        dumper_dir = args.dumper_dir
    else:
        dumper_path = FindBinary(
            ExternalModules.vtable_parser.VtableParser.VNDK_VTABLE_DUMPER)
        dumper_dir = os.path.dirname(os.path.dirname(dumper_path))
    print("DUMPER_DIR=" + dumper_dir)

    output_dir = os.path.join((args.output if args.output else vndk_version),
                              ("binder32" if binder_32_bit else "binder64"),
                              target_arch)
    print("OUTPUT_DIR=" + output_dir)

    lib_names = [name.format(VNDK_VER="-" + vndk_version) for
                 name in _LoadLibraryNames(args.file)]

    missing_libs = DumpAbi(output_dir, lib_names, target_lib_dir,
                           target_obj_dir, dumper_dir)
    if target_2nd_arch:
        missing_libs += DumpAbi(output_dir, lib_names, target_2nd_lib_dir,
                                target_2nd_obj_dir, dumper_dir)

    if missing_libs:
        print("Warning: Could not find libraries:")
        for lib_path in missing_libs:
            print(lib_path)


if __name__ == "__main__":
    main()
