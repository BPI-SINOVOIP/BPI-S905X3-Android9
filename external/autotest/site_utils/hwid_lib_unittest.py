# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import cStringIO
import json
import mock
import os
import shutil
import tempfile
import unittest
import urllib2

import common

from autotest_lib.site_utils import hwid_lib


class HwIdUnittests(unittest.TestCase):
    """Unittest for testing get_hwid_info."""

    def setUp(self):
        # Create tmp dir and dummy key files.
        self.tmp_dir = tempfile.mkdtemp(prefix='hwid_test')
        self.dummy_key = 'dummy_key'
        self.dummy_key_file = os.path.join(self.tmp_dir, 'dummy_key')
        with open(self.dummy_key_file, 'w') as f:
            f.write(self.dummy_key)
        self.dummy_key_file_spaces = os.path.join(self.tmp_dir,
                                                  'dummy_key_spaces')
        with open(self.dummy_key_file_spaces, 'w') as f:
            f.write('  %s   ' % self.dummy_key)
        self.dummy_key_file_newline = os.path.join(self.tmp_dir,
                                                  'dummy_key_newline')
        with open(self.dummy_key_file_newline, 'w') as f:
            f.write('%s\n' % self.dummy_key)
        self.invalid_dummy_key_file = os.path.join(self.tmp_dir,
                                                   'invalid_dummy_key_file')


    def tearDown(self):
        mock.patch.stopall()
        if os.path.exists(self.tmp_dir):
            shutil.rmtree(self.tmp_dir)


    def validate_exception(self, exception, *args):
        """Helper method to validate proper exception is raised.

        @param exception: The exception class to check against.
        @param args: The unamed args to pass to func.
        """
        with self.assertRaises(exception):
            hwid_lib.get_hwid_info(*args)


    def test_none_hwid(self):
        """Test that an empty hwid raises a ValueError."""
        self.validate_exception(ValueError, None, None, None)


    def test_invalid_info_type(self):
        """Test that an invalid info type raises a ValueError."""
        self.validate_exception(ValueError, 'hwid', 'invalid_info_type', None)


    def test_fail_open_with_nonexistent_file(self):
        """Test that trying to open non-existent file will raise an IOError."""
        self.validate_exception(IOError, 'hwid', hwid_lib.HWID_INFO_BOM,
                                self.invalid_dummy_key_file)


    @mock.patch('urllib2.urlopen', side_effect=urllib2.URLError('url error'))
    def test_fail_to_open_url_urlerror(self, *args, **dargs):
        """Test that failing to open a url will raise a HwIdException."""
        self.validate_exception(hwid_lib.HwIdException, 'hwid',
                                hwid_lib.HWID_INFO_BOM, self.dummy_key_file)


    # pylint: disable=missing-docstring
    @mock.patch('urllib2.urlopen')
    def test_fail_decode_hwid(self, mock_urlopen, *args, **dargs):
        """Test that supplying bad json raises a HwIdException."""
        mock_page_contents = mock.Mock(wraps=cStringIO.StringIO('bad json'))
        mock_urlopen.return_value = mock_page_contents
        self.validate_exception(hwid_lib.HwIdException, 'hwid',
                                hwid_lib.HWID_INFO_BOM, self.dummy_key_file)
        mock_page_contents.close.assert_called_once_with()


    # pylint: disable=missing-docstring
    @mock.patch('urllib2.urlopen')
    def test_success(self, mock_urlopen, *args, **dargs):
        """Test that get_hwid_info successfully returns a hwid dict.

        We want to check that it works on all valid info types.
        """
        returned_json = '{"key1": "data1"}'
        expected_dict = json.loads(returned_json)
        for valid_info_type in hwid_lib.HWID_INFO_TYPES:
            mock_page_contents = mock.Mock(
                    wraps=cStringIO.StringIO(returned_json))
            mock_urlopen.return_value = mock_page_contents
            self.assertEqual(hwid_lib.get_hwid_info('hwid', valid_info_type,
                                                    self.dummy_key_file),
                             expected_dict)
            mock_page_contents.close.assert_called_once_with()


    # pylint: disable=missing-docstring
    @mock.patch('urllib2.urlopen')
    def test_url_properly_constructed(self, mock_urlopen, *args, **dargs):
        """Test that the url is properly constructed.

        Let's make sure that the key is properly cleaned before getting
        inserted into the url by trying all the different dummy_key_files.
        """
        info_type = hwid_lib.HWID_INFO_BOM
        hwid = 'mock_hwid'
        expected_url = ('%s/%s/%s/%s/?key=%s' % (hwid_lib.HWID_BASE_URL,
                                                 hwid_lib.HWID_VERSION,
                                                 info_type, hwid,
                                                 self.dummy_key))

        for dummy_key_file in [self.dummy_key_file,
                               self.dummy_key_file_spaces,
                               self.dummy_key_file_newline]:
            mock_page_contents = mock.Mock(wraps=cStringIO.StringIO('{}'))
            mock_urlopen.return_value = mock_page_contents
            hwid_lib.get_hwid_info(hwid, info_type, dummy_key_file)
            mock_urlopen.assert_called_with(expected_url)


    # pylint: disable=missing-docstring
    @mock.patch('urllib2.urlopen')
    def test_url_properly_constructed_again(self, mock_urlopen, *args, **dargs):
        """Test that the url is properly constructed with special hwid.

        Let's make sure that a hwid with a space is properly transformed.
        """
        info_type = hwid_lib.HWID_INFO_BOM
        hwid = 'mock hwid with space'
        hwid_quoted = 'mock%20hwid%20with%20space'
        expected_url = ('%s/%s/%s/%s/?key=%s' % (hwid_lib.HWID_BASE_URL,
                                                 hwid_lib.HWID_VERSION,
                                                 info_type, hwid_quoted,
                                                 self.dummy_key))

        mock_page_contents = mock.Mock(wraps=cStringIO.StringIO('{}'))
        mock_urlopen.return_value = mock_page_contents
        hwid_lib.get_hwid_info(hwid, info_type, self.dummy_key_file)
        mock_urlopen.assert_called_with(expected_url)


    def test_dummy_key_file(self):
        """Test that we get an empty dict with a dummy key file."""
        info_type = hwid_lib.HWID_INFO_BOM
        hwid = 'mock hwid with space'
        key_file = hwid_lib.KEY_FILENAME_NO_HWID
        self.assertEqual(hwid_lib.get_hwid_info(hwid, info_type, key_file), {})


if __name__ == '__main__':
    unittest.main()
