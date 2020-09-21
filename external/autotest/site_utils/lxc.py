# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This tool can be used to set up a base container for test. For example,
  python lxc.py -s -p /tmp/container
This command will download and setup base container in directory /tmp/container.
After that command finishes, you can run lxc command to work with the base
container, e.g.,
  lxc-start -P /tmp/container -n base -d
  lxc-attach -P /tmp/container -n base
"""

import argparse
import logging

import common
from autotest_lib.client.bin import utils
from autotest_lib.site_utils import lxc


def parse_options():
    """Parse command line inputs.

    @raise argparse.ArgumentError: If command line arguments are invalid.
    """
    parser = argparse.ArgumentParser()
    parser.add_argument('-s', '--setup', action='store_true',
                        default=False,
                        help='Set up base container.')
    parser.add_argument('-p', '--path', type=str,
                        help='Directory to store the container.',
                        default=lxc.DEFAULT_CONTAINER_PATH)
    parser.add_argument('-f', '--force_delete', action='store_true',
                        default=False,
                        help=('Force to delete existing containers and rebuild '
                              'base containers.'))
    parser.add_argument('-n', '--name', type=str,
                        help='Name of the base container.',
                        default=lxc.BASE)
    options = parser.parse_args()
    if not options.setup and not options.force_delete:
        raise argparse.ArgumentError(
                'Use --setup to setup a base container, or --force_delete to '
                'delete all containers in given path.')
    return options


def main():
    """main script."""
    # Force to run the setup as superuser.
    # TODO(dshi): crbug.com/459344 Set remove this enforcement when test
    # container can be unprivileged container.
    if utils.sudo_require_password():
        logging.warn('SSP requires root privilege to run commands, please '
                     'grant root access to this process.')
        utils.run('sudo true')

    options = parse_options()
    image = lxc.BaseImage(container_path=options.path)
    if options.setup:
        image.setup(name=options.name, force_delete=options.force_delete)
    elif options.force_delete:
        image.cleanup()


if __name__ == '__main__':
    main()
