#
# Copyright (C) 2016 The Android Open Source Project
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
"""Parses the contents of a Unix archive file generated using the 'ar' command.

The constructor returns an Archive object, which contains dictionary from
file name to file content.


    Typical usage example:

    archive = Archive(content)
    archive.Parse()
"""

import io

class Archive(object):
    """Archive object parses and stores Unix archive contents.

    Stores the file names and contents as it parses the archive.

    Attributes:
        files: a dictionary from file name (string) to file content (binary)
    """

    GLOBAL_SIG = '!<arch>\n'  # Unix global signature
    STRING_TABLE_ID = '//'
    STRING_TABLE_TERMINATOR = '/\n'
    SYM_TABLE_ID = '__.SYMDEF'
    FILE_ID_LENGTH = 16  # Number of bytes to store file identifier
    FILE_ID_TERMINATOR = '/'
    FILE_TIMESTAMP_LENGTH = 12  # Number of bytes to store file mod timestamp
    OWNER_ID_LENGTH = 6  # Number of bytes to store file owner ID
    GROUP_ID_LENGTH = 6  # Number of bytes to store file group ID
    FILE_MODE_LENGTH = 8  # Number of bytes to store file mode
    CONTENT_SIZE_LENGTH = 10  # Number of bytes to store content size
    END_TAG = '`\n'  # Header end tag

    def __init__(self, file_content):
        """Initialize and parse the archive contents.

        Args:
          file_content: Binary contents of the archive file.
        """

        self.files = {}
        self._content = file_content
        self._cursor = 0
        self._string_table = dict()

    def ReadBytes(self, n):
        """Reads n bytes from the content stream.

        Args:
            n: The integer number of bytes to read.

        Returns:
            The n-bit string (binary) of data from the content stream.

        Raises:
            ValueError: invalid file format.
        """
        if self._cursor + n > len(self._content):
            raise ValueError('Invalid file. EOF reached unexpectedly.')

        content = self._content[self._cursor : self._cursor + n]
        self._cursor += n
        return content

    def Parse(self):
        """Verifies the archive header and arses the contents of the archive.

        Raises:
            ValueError: invalid file format.
        """
        # Check global header
        sig = self.ReadBytes(len(self.GLOBAL_SIG))
        if sig != self.GLOBAL_SIG:
            raise ValueError('File is not a valid Unix archive.')

        # Read files in archive
        while self._cursor < len(self._content):
            self.ReadFile()

    def ReadFile(self):
        """Reads a file from the archive content stream.

        Raises:
            ValueError: invalid file format.
        """
        name = self.ReadBytes(self.FILE_ID_LENGTH).strip()
        self.ReadBytes(self.FILE_TIMESTAMP_LENGTH)
        self.ReadBytes(self.OWNER_ID_LENGTH)
        self.ReadBytes(self.GROUP_ID_LENGTH)
        self.ReadBytes(self.FILE_MODE_LENGTH)
        size = self.ReadBytes(self.CONTENT_SIZE_LENGTH)
        content_size = int(size)

        if self.ReadBytes(len(self.END_TAG)) != self.END_TAG:
            raise ValueError('File is not a valid Unix archive. Missing end tag.')

        content = self.ReadBytes(content_size)
        if name == self.STRING_TABLE_ID:
            acc = 0
            names = content.split(self.STRING_TABLE_TERMINATOR)
            for string in names:
                self._string_table[acc] = string
                acc += len(string) + len(self.STRING_TABLE_TERMINATOR)
        elif name != self.SYM_TABLE_ID:
            if name.endswith(self.FILE_ID_TERMINATOR):
                name = name[:-len(self.FILE_ID_TERMINATOR)]
            elif name.startswith(self.FILE_ID_TERMINATOR):
                offset = int(name[len(self.FILE_ID_TERMINATOR):])
                if offset not in self._string_table:
                    raise ValueError('Offset %s not in string table.', offset)
                name = self._string_table[offset]
            self.files[name] = content
