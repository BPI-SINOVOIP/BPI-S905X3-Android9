#!/usr/bin/python
# Copyright (c) 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import mock
import unittest

import common
from autotest_lib.client.common_lib import error
from autotest_lib.server.hosts import jetstream_host


class JetstreamHostTestCase(unittest.TestCase):

    @mock.patch.object(jetstream_host.logging, 'exception')
    @mock.patch.object(jetstream_host.JetstreamHost, 'cleanup_services')
    def test_cleanup(self, mock_cleanup, mock_exception_logging):
         host = jetstream_host.JetstreamHost('')
         host.prepare_for_update()
         mock_cleanup.assert_called_with()
         mock_exception_logging.assert_not_called()

    @mock.patch.object(jetstream_host.logging, 'exception')
    @mock.patch.object(jetstream_host.JetstreamHost, 'cleanup_services')
    def test_failed_cleanup(self, mock_cleanup, mock_exception_logging):
         mock_cleanup.side_effect = error.AutoservRunError('failed', None)
         host = jetstream_host.JetstreamHost('')
         host.prepare_for_update()
         mock_cleanup.assert_called_with()
         mock_exception_logging.assert_called()


if __name__ == "__main__":
    unittest.main()
