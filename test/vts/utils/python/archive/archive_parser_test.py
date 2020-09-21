#!/usr/bin/env python
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

import os
import unittest

from vts.utils.python.archive import archive_parser


class ArchiveParserTest(unittest.TestCase):
    """Unit tests for archive_parser of vts.utils.python.archive.
    """

    def testReadHeaderPass(self):
        """Tests that archive is read when header is correct.

        Parses archive content containing only the signature.
        """
        try:
            archive = archive_parser.Archive(archive_parser.Archive.GLOBAL_SIG)
            archive.Parse()
        except ValueError:
            self.fail('Archive reader read improperly.')

    def testReadHeaderFail(self):
        """Tests that parser throws error when header is invalid.

        Parses archive content lacking the correct signature.
        """
        archive = archive_parser.Archive('Fail.')
        self.assertRaises(ValueError, archive.Parse)

    def testReadFile(self):
        """Tests that file is read correctly.

        Tests that correctly formatted file in archive is read correctly.
        """
        content = archive_parser.Archive.GLOBAL_SIG
        file_name = 'test_file'
        content += file_name + ' ' * (archive_parser.Archive.FILE_ID_LENGTH -
                                      len(file_name))
        content += ' ' * archive_parser.Archive.FILE_TIMESTAMP_LENGTH
        content += ' ' * archive_parser.Archive.OWNER_ID_LENGTH
        content += ' ' * archive_parser.Archive.GROUP_ID_LENGTH
        content += ' ' * archive_parser.Archive.FILE_MODE_LENGTH

        message = 'test file contents'
        message_size = str(len(message))
        content += message_size + ' ' * (archive_parser.Archive.CONTENT_SIZE_LENGTH -
                                         len(message_size))
        content += archive_parser.Archive.END_TAG
        content += message
        archive = archive_parser.Archive(content)
        archive.Parse()
        self.assertIn(file_name, archive.files)
        self.assertEquals(archive.files[file_name], message)

if __name__ == "__main__":
    unittest.main()
