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

from vts.utils.python.library import elf_parser


class ArError(Exception):
    """The exception raised by the functions in this module."""
    pass


def _IterateArchive(archive_path):
    """A generator yielding the offsets of the ELF objects in an archive.

    An archive file is a magic string followed by an array of file members.
    Each file member consists of a header and the file content. The header
    begins on a 2-byte boundary and is defined by the following structure. All
    fields are ASCII strings padded with space characters.

    BYTES FIELD
     0~15 file name
    16~27 modification time
    28~33 user id
    34~39 group id
    40~47 permission
    48~57 size
    58~59 magic bytes

    Args:
        archive_path: The path to the archive file.

    Yields:
        An integer, the offset of the ELF object.

    Raises:
        ArError if the file is not a valid archive.
    """
    ar = None
    try:
        ar = open(archive_path, "rb")
        if ar.read(8) != "!<arch>\n":
            raise ArError("Wrong magic string.")
        while True:
            header = ar.read(60)
            if len(header) != 60:
                break
            obj_name = header[0:16].rstrip(" ")
            obj_size = int(header[48:58].rstrip(" "))
            obj_offset = ar.tell()
            # skip string tables
            if obj_name not in ("/", "//", "/SYM64/"):
                yield obj_offset
            obj_offset += obj_size
            ar.seek(obj_offset + obj_offset % 2)
    except IOError as e:
        raise ArError(e)
    finally:
        if ar:
            ar.close()


def ListGlobalSymbols(archive_path):
    """Lists global symbols in an ELF archive.

    Args:
        archive_path: The path to the archive file.

    Returns:
        A List of strings, the global symbols in the archive.

    Raises:
        ArError if fails to load the archive.
        elf_parser.ElfError if fails to load any library in the archive.
    """
    symbols = []
    for offset in _IterateArchive(archive_path):
        with elf_parser.ElfParser(archive_path, offset) as parser:
            symbols.extend(parser.ListGlobalSymbols())
    return symbols
