# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import getpass
import logging
import sys

import common
from autotest_lib.client.common_lib import utils


def setup_logging(log_level):
    """Sets up direct logging to stdout for unittests.

    @param log_level: Level of logging to redirect to stdout, default to INFO.
    """
    # Lifted from client.common_lib.logging_config.
    FORMAT = ('%(asctime)s.%(msecs)03d %(levelname)-5.5s|%(module)18.18s:'
              '%(lineno)4.4d| %(threadName)16.16s(%(thread)d)| %(message)s')

    logger = logging.getLogger()
    logger.setLevel(log_level)
    handler = logging.StreamHandler(sys.stdout)
    handler.setLevel(log_level)
    formatter = logging.Formatter(FORMAT)
    handler.setFormatter(formatter)
    logger.handlers = []
    logger.addHandler(handler)


def verify_user(require_sudo=True):
    """Checks that the current user is not root, but has sudo.

    Running unit tests as root can mask permissions problems, as not all the
    code runs as root in production.
    """
    # Ensure this process is not running as root.
    if getpass.getuser() == 'root':
        raise EnvironmentError('Unittests should not be run as root.')

    # However, most of the unit tests do require sudo.
    # TODO(dshi): crbug.com/459344 Set remove this enforcement when test
    # container can be unprivileged container.
    if require_sudo and utils.sudo_require_password():
        logging.warn('SSP requires root privilege to run commands, please '
                     'grant root access to this process.')
        utils.run('sudo true')


class Config(object):
    """A class for parsing and storing command line options.

    A convenience class for helping with unit test setup.  A global instance of
    this class is set up by the setup function.  Clients can then check this
    object for flags set on the command line.
    """
    def parse_options(self):
        """Parses command line flags for unittests."""
        parser = argparse.ArgumentParser()
        parser.add_argument('-v', '--verbose', action='store_true',
                            help='Print out ALL entries.')
        parser.add_argument('-s', '--skip_cleanup', action='store_true',
                            help='Skip deleting test containers.')
        args, argv = parser.parse_known_args()

        for attr, value in vars(args).items():
            setattr(self, attr, value)

        # Hack: python unittest also processes args.  Construct an argv to pass
        # to it, that filters out the options it won't recognize.  Then replace
        # sys.argv with the constructed argv so that calling unittest.main "just
        # works".
        if args.verbose:
            argv.insert(0, '-v')
        argv.insert(0, sys.argv[0])
        sys.argv = argv


# Global namespace object for storing unittest options specified on the command
# line.
config = Config()


def setup(require_sudo=True):
    """Performs global setup for unit-tests."""
    config.parse_options()

    verify_user(require_sudo)

    log_level = logging.DEBUG if config.verbose else logging.INFO
    setup_logging(log_level)
