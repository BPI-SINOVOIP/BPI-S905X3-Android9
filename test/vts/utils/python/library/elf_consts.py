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

# This file contains ELF constants and offsets.

# Common ELF header
MAGIC_OFFSET = 0
MAGIC_BYTES = b"\x7fELF"
BITNESS_OFFSET = 4
BITNESS_32 = 1
BITNESS_64 = 2
MACHINE_OFFSET = 0x12
MACHINE_X86 = 0x03
MACHINE_MIPS = 0x08
MACHINE_ARM = 0x28
MACHINE_X86_64 = 0x3e
MACHINE_AARCH64 = 0xb7
# Section type
SHT_DYNAMIC = 6
# Section index
SHN_UNDEFINED = 0
# Tag in dynamic section
DT_NULL = 0
DT_NEEDED = 1
DT_STRTAB = 5
# Section name
DYNSTR = ".dynstr"
DYNSYM = ".dynsym"
STRTAB = ".strtab"
SYMTAB = ".symtab"
# Type in symbol table
SYMBOL_NOTYPE = 0
# Binding in symbol table
SYMBOL_BINDING_GLOBAL = 1
SYMBOL_BINDING_WEAK = 2


class ElfOffsets32(object):
    """Offset of each entry in 32-bit ELF"""
    # Offset from ELF header
    SECTION_HEADER_OFFSET = 0x20
    SECTION_HEADER_SIZE = 0x2e
    SECTION_HEADER_COUNT = 0x30
    SECTION_HEADER_STRTAB_INDEX = 0x32
    # Offset from section header
    SECTION_NAME_OFFSET = 0x00
    SECTION_TYPE = 0x04
    SECTION_ADDRESS = 0x0c
    SECTION_OFFSET = 0x10
    SECTION_SIZE = 0x14
    SECTION_ENTRY_SIZE = 0x24
    # Offset from symbol table entry
    SYMBOL_NAME = 0x00
    SYMBOL_INFO = 0x0c
    SYMBOL_SECTION_INDEX = 0x0e


class ElfOffsets64(object):
    """Offset of each entry in 64-bit ELF"""
    # Offset from ELF header
    SECTION_HEADER_OFFSET = 0x28
    SECTION_HEADER_SIZE = 0x3a
    SECTION_HEADER_COUNT = 0x3c
    SECTION_HEADER_STRTAB_INDEX = 0x3e
    # Offset from section header
    SECTION_NAME_OFFSET = 0x00
    SECTION_TYPE = 0x04
    SECTION_ADDRESS = 0x10
    SECTION_OFFSET = 0x18
    SECTION_SIZE = 0x20
    SECTION_ENTRY_SIZE = 0x38
    # Offset from symbol table entry
    SYMBOL_NAME = 0x00
    SYMBOL_INFO = 0x04
    SYMBOL_SECTION_INDEX = 0x06
