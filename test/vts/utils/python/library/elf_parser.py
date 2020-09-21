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

import os
import struct

from vts.utils.python.library import elf_consts


class ElfError(Exception):
    """The exception raised by ElfParser."""
    pass


class ElfParser(object):
    """The class reads information from an ELF file.

    Attributes:
        _file: The ELF file object.
        _begin_offset: The offset of the ELF object in the file. The value is
                       non-zero if the ELF is in an archive, such as .a file.
        _file_size: Size of the file.
        bitness: Bitness of the ELF.
        _machine: The instruction set architecture of the ELF.
        _address_size: Size of address or offset in the ELF.
        _offsets: Offset of each entry in the ELF.
        _seek_read_address: The function to read an address or offset entry
                            from the ELF.
        _sh_offset: Offset of section header table in the ELF.
        _sh_size: Size of section header table entry.
        _sh_count: Number of section header table entries.
        _sh_strtab_index: Index of the section that contains section names.
        _section_headers: List of SectionHeader objects read from the ELF.
    """

    class SectionHeader(object):
        """Contains section header entries as attributes.

        Attributes:
            name_offset: Offset in the section header string table.
            type: Type of the section.
            address: The virtual memory address where the section is loaded.
            offset: The offset of the section in the ELF object.
            size: Size of the section.
            entry_size: Size of each entry in the section.
        """

        def __init__(self, elf, offset):
            """Loads a section header from ELF file.

            Args:
                elf: The instance of ElfParser.
                offset: The starting offset of the section header.
            """
            self.name_offset = elf._SeekRead32(
                offset + elf._offsets.SECTION_NAME_OFFSET)
            self.type = elf._SeekRead32(
                offset + elf._offsets.SECTION_TYPE)
            self.address = elf._seek_read_address(
                offset + elf._offsets.SECTION_ADDRESS)
            self.offset = elf._seek_read_address(
                offset + elf._offsets.SECTION_OFFSET)
            self.size = elf._seek_read_address(
                offset + elf._offsets.SECTION_SIZE)
            self.entry_size = elf._seek_read_address(
                offset + elf._offsets.SECTION_ENTRY_SIZE)

    def __init__(self, file_path, begin_offset=0):
        """Creates a parser to open and read an ELF file.

        Args:
            file_path: The path to the file.
            begin_offset: The offset of the ELF object in the file.

        Raises:
            ElfError if the file is not a valid ELF.
        """
        self._begin_offset = begin_offset
        try:
            self._file = open(file_path, "rb")
        except IOError as e:
            raise ElfError(e)
        try:
            self._LoadElfHeader()
            self._section_headers = [
                self.SectionHeader(self, self._sh_offset + i * self._sh_size)
                for i in range(self._sh_count)]
        except:
            self._file.close()
            raise

    def __del__(self):
        """Closes the ELF file."""
        self.Close()

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        """Closes the ELF file."""
        self.Close()

    def Close(self):
        """Closes the ELF file."""
        if hasattr(self, "_file"):
            self._file.close()

    def _SeekRead(self, offset, read_size):
        """Reads a byte string at specific offset in the file.

        Args:
            offset: An integer, the offset from the beginning of the ELF.
            read_size: An integer, number of bytes to read.

        Returns:
            A byte string which is the file content.

        Raises:
            ElfError if fails to seek and read.
        """
        if offset + read_size > self._file_size:
            raise ElfError("Read beyond end of file.")
        try:
            self._file.seek(self._begin_offset + offset)
            return self._file.read(read_size)
        except IOError as e:
            raise ElfError(e)

    def _SeekRead8(self, offset):
        """Reads an 1-byte integer from file."""
        return struct.unpack("B", self._SeekRead(offset, 1))[0]

    def _SeekRead16(self, offset):
        """Reads a 2-byte integer from file."""
        return struct.unpack("H", self._SeekRead(offset, 2))[0]

    def _SeekRead32(self, offset):
        """Reads a 4-byte integer from file."""
        return struct.unpack("I", self._SeekRead(offset, 4))[0]

    def _SeekRead64(self, offset):
        """Reads an 8-byte integer from file."""
        return struct.unpack("Q", self._SeekRead(offset, 8))[0]

    def _SeekReadString(self, offset):
        """Reads a null-terminated string starting from specific offset.

        Args:
            offset: The offset from the beginning of the ELF object.

        Returns:
            A byte string, excluding the null character.
        """
        ret = ""
        buf_size = 16
        self._file.seek(self._begin_offset + offset)
        while True:
            try:
                buf = self._file.read(buf_size)
            except IOError as e:
                raise ElfError(e)
            end_index = buf.find('\0')
            if end_index < 0:
                ret += buf
            else:
                ret += buf[:end_index]
                return ret
            if len(buf) != buf_size:
                raise ElfError("Null-terminated string reaches end of file.")

    def _LoadElfHeader(self):
        """Loads ELF header and initializes attributes."""
        try:
            self._file_size = os.fstat(self._file.fileno()).st_size
        except OSError as e:
            raise ElfError(e)

        magic = self._SeekRead(elf_consts.MAGIC_OFFSET, 4)
        if magic != elf_consts.MAGIC_BYTES:
            raise ElfError("Wrong magic bytes.")
        bitness = self._SeekRead8(elf_consts.BITNESS_OFFSET)
        if bitness == elf_consts.BITNESS_32:
            self.bitness = 32
            self._address_size = 4
            self._offsets = elf_consts.ElfOffsets32
            self._seek_read_address = self._SeekRead32
        elif bitness == elf_consts.BITNESS_64:
            self.bitness = 64
            self._address_size = 8
            self._offsets = elf_consts.ElfOffsets64
            self._seek_read_address = self._SeekRead64
        else:
            raise ElfError("Wrong bitness value.")

        self._machine = self._SeekRead16(elf_consts.MACHINE_OFFSET)
        self._sh_offset = self._seek_read_address(
            self._offsets.SECTION_HEADER_OFFSET)
        self._sh_size = self._SeekRead16(self._offsets.SECTION_HEADER_SIZE)
        self._sh_count = self._SeekRead16(self._offsets.SECTION_HEADER_COUNT)
        self._sh_strtab_index = self._SeekRead16(
            self._offsets.SECTION_HEADER_STRTAB_INDEX)
        if self._sh_strtab_index >= self._sh_count:
            raise ElfError("Wrong section header string table index.")

    def _LoadSectionName(self, sh):
        """Reads the name of a section.

        Args:
            sh: An instance of SectionHeader.

        Returns:
            A string, the name of the section.
        """
        strtab = self._section_headers[self._sh_strtab_index]
        return self._SeekReadString(strtab.offset + sh.name_offset)

    def _LoadDtNeeded(self, offset):
        """Reads DT_NEEDED entries from dynamic section.

        Args:
            offset: The offset of the dynamic section from the beginning of
                    the ELF object.

        Returns:
            A list of strings, the names of libraries.
        """
        strtab_address = None
        name_offsets = []
        while True:
            tag = self._seek_read_address(offset)
            offset += self._address_size
            value = self._seek_read_address(offset)
            offset += self._address_size

            if tag == elf_consts.DT_NULL:
                break
            if tag == elf_consts.DT_NEEDED:
                name_offsets.append(value)
            if tag == elf_consts.DT_STRTAB:
                strtab_address = value

        if strtab_address is None:
            raise ElfError("Cannot find string table offset in dynamic section.")

        try:
            strtab_offset = next(x.offset for x in self._section_headers
                                 if x.address == strtab_address)
        except StopIteration:
            raise ElfError("Cannot find dynamic string table.")

        names = [self._SeekReadString(strtab_offset + x)
                 for x in name_offsets]
        return names

    def MatchCpuAbi(self, abi):
        """Returns whether the ELF matches the ABI.

        Args:
            abi: A string, the name of the ABI.

        Returns:
            A boolean, whether the ELF matches the ABI.
        """
        for abi_prefix, machine in (("arm64", elf_consts.MACHINE_AARCH64),
                                    ("arm", elf_consts.MACHINE_ARM),
                                    ("mips64", elf_consts.MACHINE_MIPS),
                                    ("mips", elf_consts.MACHINE_MIPS),
                                    ("x86_64", elf_consts.MACHINE_X86_64),
                                    ("x86", elf_consts.MACHINE_X86)):
            if abi.startswith(abi_prefix):
                return self._machine == machine
        return False

    def ListDependencies(self):
        """Lists the shared libraries that the ELF depends on.

        Returns:
            A list of strings, the names of the depended libraries.
        """
        deps = []
        for sh in self._section_headers:
            if sh.type == elf_consts.SHT_DYNAMIC:
                deps.extend(self._LoadDtNeeded(sh.offset))
        return deps

    def ListGlobalSymbols(self, include_weak=False,
                          symtab_name=elf_consts.SYMTAB,
                          strtab_name=elf_consts.STRTAB):
        """Lists the dynamic symbols defined in the ELF.

        Args:
            include_weak: A boolean, whether to include weak symbols.
            symtab_name: A string, the name of the symbol table.
            strtab_name: A string, the name of the string table.

        Returns:
            A list of strings, the names of the symbols.
        """
        symtab = None
        strtab = None
        for sh in self._section_headers:
            name = self._LoadSectionName(sh)
            if name == symtab_name:
                symtab = sh
            elif name == strtab_name:
                strtab = sh
        if not symtab or not strtab or symtab.size == 0:
            raise ElfError("Cannot find symbol table.")

        include_bindings = [elf_consts.SYMBOL_BINDING_GLOBAL]
        if include_weak:
            include_bindings.append(elf_consts.SYMBOL_BINDING_WEAK)

        sym_names = []
        for offset in range(
                symtab.offset, symtab.offset + symtab.size, symtab.entry_size):
            # sym_info is a 1-byte field in symbol table entry.
            # The lower 4 bits represent the type, such as function, object,
            # and section.
            # The higher 4 bits represent the binding. Global symbols can be
            # defined at most once at link time, while weak symbols may have
            # multiple definitions.
            sym_info = self._SeekRead8(offset + self._offsets.SYMBOL_INFO)
            if (sym_info & 0xf) == elf_consts.SYMBOL_NOTYPE:
                continue
            if (sym_info >> 4) not in include_bindings:
                continue
            sym_sh_index = self._SeekRead16(
                offset + self._offsets.SYMBOL_SECTION_INDEX)
            if sym_sh_index == elf_consts.SHN_UNDEFINED:
                continue
            name_offset = self._SeekRead32(offset + self._offsets.SYMBOL_NAME)
            sym_names.append(self._SeekReadString(strtab.offset + name_offset))
        return sym_names

    def ListGlobalDynamicSymbols(self, include_weak=False):
        """Lists the global dynamic symbols defined in the ELF.

        Args:
            include_weak: A boolean, whether to include weak symbols.

        Returns:
            A list of strings, the names of the symbols.
        """
        return self.ListGlobalSymbols(
            include_weak, elf_consts.DYNSYM, elf_consts.DYNSTR)
