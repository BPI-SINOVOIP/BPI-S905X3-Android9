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
import stat

from vts.runners.host import utils
from vts.utils.python.common import cmd_utils


class VtableError(Exception):
    """The exception raised by VtableParser."""
    pass


class VtableParser(object):
    """This class parses vtables in libraries.

    The implementation is to call vndk-vtable-dumper to dump vtables from
    libraries, and then parse the output. A dump consists of multiple vtables.
    The following is the sample of one vtable:
    vtable for android::hardware::details::HidlInstrumentor
    _ZTVN7android8hardware7details16HidlInstrumentorE: 2 entries
    16    (int (*)(...)) _ZN7android8hardware7details16HidlInstrumentorD2Ev
    24    (int (*)(...)) _ZN7android8hardware7details16HidlInstrumentorD0Ev

    Attributes:
        _dumper_path: The path to the directory containing
                      bin/vndk-vtable-dumper, bin/vndk-vtable-dumper.exe,
                      lib64/libLLVM.so, and lib/libLLVM.dll.
        VNDK_VTABLE_DUMPER: The dumper file name on Linux.
        VNDK_VTABLE_DUMPER_EXE: The dumper file name on Windows.
    """
    VNDK_VTABLE_DUMPER = "vndk-vtable-dumper"
    VNDK_VTABLE_DUMPER_EXE = "vndk-vtable-dumper.exe"
    _LINE_1_PATTERN = re.compile("^vtable for .+$")
    _LINE_2_PATTERN = re.compile("^([^:]+): (\\d+) entries$")
    _LINE_3_PATTERN = re.compile(
            "^(\\d+) +" + re.escape("(int (*)(...))") + " (.+)$")

    def __init__(self, dumper_path):
        """Initializes the dumper path and its permission."""
        self._dumper_path = dumper_path

        if not utils.is_on_windows():
            bin_path = os.path.join(
                self._dumper_path, "bin", self.VNDK_VTABLE_DUMPER)
            try:
                mode = os.stat(bin_path).st_mode
                os.chmod(bin_path,
                         mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)
            except OSError as e:
                logging.warning("Fail to chmod vtable dumper: %s", e)

    def CallVtableDumper(self, lib_path):
        """Calls vndk-vtable-dumper and returns its output.

        Args:
            lib_path: The host-side path to the library.

        Returns:
            A string which is the output of vndk-vtable-dumper

        Raises:
            VtableError if the command fails.
        """
        bin_dir = os.path.join(self._dumper_path, "bin")
        if not utils.is_on_windows():
            bin_path = os.path.join(bin_dir, self.VNDK_VTABLE_DUMPER)
            cmd = "%s %s -mangled" % (bin_path, lib_path)
        else:
            bin_path = os.path.join(bin_dir, self.VNDK_VTABLE_DUMPER_EXE)
            dll_dir = os.path.join(self._dumper_path, "lib")
            cmd = 'set "PATH=%s;%%PATH%%" && %s %s -mangled' % (
                    dll_dir, bin_path, lib_path)
        logging.debug("Execute command: %s" % cmd)
        stdout, stderr, exit_code = cmd_utils.ExecuteOneShellCommand(cmd)
        logging.debug("stdout: " + stdout)
        logging.debug("stderr: " + stderr)
        if exit_code:
            raise VtableError("command: %s\n"
                              "exit code: %s\n"
                              "stdout: %s\n"
                              "stderr: %s" % (cmd, exit_code, stdout, stderr))
        return stdout

    def ParseOneVtable(self, lines):
        """Parses one vtable from output of vndk-vtable-dumper.

        Args:
            lines: A generator of strings which yields lines from the dump.

        Returns:
            A tuple of vtable and the symbols.
            (VTABLE_SYMBOL, [(OFFSET_0, SYMBOL_0), (OFFSET_1, SYMBOL_1), ...])

        Raises:
            StopIteration if no more vtables to parse.
            VTableError if the dump format is wrong.
        """
        # Demangled vtable name
        while True:
            try:
                line = lines.next()
            except StopIteration:
                raise
            if line:
                break
        if not self._LINE_1_PATTERN.match(line):
            raise VtableError("Wrong format: %s" % line)
        # Mangled vtable name and number of entries
        try:
            line = lines.next()
        except StopIteration:
            raise VtableError("Reach end of dump.")
        matched = self._LINE_2_PATTERN.match(line)
        if not matched:
            raise VtableError("Wrong format: %s" % line)
        groups = matched.groups()
        vtable = groups[0]
        entry_count = int(groups[1])
        # The entries
        symbols = []
        for index in range(entry_count):
            try:
                line = lines.next()
            except StopIteration:
                raise VtableError("Reach end of dump.")
            matched = self._LINE_3_PATTERN.match(line)
            if not matched:
                raise VtableError("Wrong format: %s" % line)
            groups = matched.groups()
            symbols.append((int(groups[0]), groups[1]))
        return vtable, symbols

    def ParseVtablesFromString(self, dumper_output):
        """Parses all vtables from a string.

        Args:
            dumper_output: A string which is the output of vndk-vtable-dumper.

        Returns:
            A dict of vtables.
            {VTABLE_SYMBOL: [(OFFSET_0, SYMBOL_0), (OFFSET_1, SYMBOL_1), ...]}

        Raises:
            VtableError if fails to parse the string.
        """
        lines = (x for x in dumper_output.split("\n"))
        vtables = dict()
        try:
            while True:
                vtable, symbols = self.ParseOneVtable(lines)
                vtables[vtable] = symbols
        except StopIteration:
            return vtables

    def ParseVtablesFromLibrary(self, lib_path):
        """Parses all vtables in a library.

        Args:
            lib_path: The host-side path to the library.

        Returns:
            A dict of vtables.
            {VTABLE_SYMBOL: [(OFFSET_0, SYMBOL_0), (OFFSET_1, SYMBOL_1), ...]}

        Raises:
            VtableError if the dumper command fails or format error.
        """
        stdout = self.CallVtableDumper(lib_path)
        return self.ParseVtablesFromString(stdout)
