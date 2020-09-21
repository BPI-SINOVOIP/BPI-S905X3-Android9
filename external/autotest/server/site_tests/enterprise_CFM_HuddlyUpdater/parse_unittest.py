# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

import parse


class ParseHuddlyInfoTest(unittest.TestCase):
    """Tests the output of huddly-updater --info."""

    CHUNK_FILENAME = './samples/huddly-updater-info.log'

    def test_parser(self):
        want = {
            'package': {
                'app': '0.5.1',
                'boot': '0.2.1',
                'hw_rev': '6'
            },
            'peripheral': {
                'app': '0.5.1',
                'boot': '0.2.1',
                'hw_rev': '6'
            }
        }

        with open(filename, 'r') as fhandle:
            chunk = fhandle.read()

        got = parse.parse_fw_vers(chunk)
        self.assertDictEqual(want, got)


if __name__ == '__main__':
    unittest.main()
