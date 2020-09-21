#!/usr/bin/python
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import urlparse
import unittest

import common
from autotest_lib.server import afe_urls


class AfeUrlsTestCase(unittest.TestCase):

    """Tests for AfeUrls."""

    def assertURLEqual(self, a, b):
        """Assert two URLs are equal.

        @param a First URL to compare
        @param b First URL to compare

        """
        urlsplit = urlparse.urlsplit
        self.assertEqual(urlsplit(a), urlsplit(b))

    def test__geturl(self):
        """Test _geturl() happy path."""
        urls = afe_urls.AfeUrls('http://localhost/afe/')
        got = urls._geturl({'foo': 'bar', 'spam': 'eggs'})
        self.assertURLEqual(got, 'http://localhost/afe/#foo=bar&spam=eggs')

    def test_get_host_url(self):
        """Test get_host_url() happy path."""
        urls = afe_urls.AfeUrls('http://localhost/afe/')
        got = urls.get_host_url(42)
        self.assertURLEqual(
            got,
            'http://localhost/afe/#tab_id=view_host&object_id=42')

    def test_root_url(self):
        """Test happy path for root_url attribute."""
        urls = afe_urls.AfeUrls('http://localhost/afe/')
        self.assertEqual(urls.root_url, 'http://localhost/afe/')

    def test_equal(self):
        """Test happy path for equality."""
        urls1 = afe_urls.AfeUrls('http://localhost/afe/')
        urls2 = afe_urls.AfeUrls('http://localhost/afe/')
        self.assertEqual(urls1, urls2)

    def test_from_hostname(self):
        """Test from_hostname() happy path."""
        urls = afe_urls.AfeUrls.from_hostname('sharanohiar')
        self.assertEqual(urls.root_url, 'http://sharanohiar/afe/')


if __name__ == '__main__':
    unittest.main()
