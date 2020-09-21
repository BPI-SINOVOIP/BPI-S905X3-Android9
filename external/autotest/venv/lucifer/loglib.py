# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Shared logging functions"""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import logging
import logging.config


def add_logging_options(parser):
    """Add logging configuration options to argument parser.

    @param parser: ArgumentParser instance.
    """
    # Unused for the moment, but will be useful when we need to add
    # logging options.
    del parser


def configure_logging_with_args(parser, args):
    """Convenience function for calling configure_logging().

    @param parser: ArgumentParser instance.
    @param args: Return value from ArgumentParser.parse_args().
    """
    # Unused for the moment, but will be useful when we need to add
    # logging options.
    del args
    configure_logging(name=parser.prog)


def configure_logging(name):
    """Configure logging globally.

    @param name: Name to prepend to log messages.
                 This should be the name of the program.
    """
    logging.config.dictConfig({
        'version': 1,
        'formatters': {
            'stderr': {
                'format': ('{name}: '
                           '%(asctime)s:%(levelname)s'
                           ':%(module)s:%(funcName)s:%(lineno)d'
                           ':%(message)s'
                           .format(name=name)),
            },
        },
        'handlers': {
            'stderr': {
                'class': 'logging.StreamHandler',
                'formatter': 'stderr' ,
            }
        },
        'root': {
            'level': 'DEBUG',
            'handlers': ['stderr'],
        },
        'disable_existing_loggers': False,
    })
