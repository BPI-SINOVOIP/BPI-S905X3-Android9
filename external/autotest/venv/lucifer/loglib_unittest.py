# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import argparse
import unittest

import mock

from lucifer import loglib


class LogLibTestCase(unittest.TestCase):
    """Module unit tests."""

    def test_configure_logging(self):
        """Test that dict can be evaluated."""
        with mock.patch('logging.config.dictConfig',
                        autospec=True) as dictConfig:
            loglib.configure_logging(name='unittest')
        dictConfig.assert_called_once_with(mock.ANY)

    def test_parse_and_config_defaults(self):
        """Test default args satisfy configure_logging_with_args()."""
        parser = argparse.ArgumentParser(prog='unittest')
        loglib.add_logging_options(parser)
        args = parser.parse_args([])
        with mock.patch.object(loglib, 'configure_logging',
                               autospec=True) as configure:
            loglib.configure_logging_with_args(parser, args)
        configure.assert_called_once_with(name='unittest')
