#!/usr/bin/python
# -*- coding:utf-8 -*-
# Copyright 2016 The Android Open Source Project
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

"""Wrapper to run pylint with the right settings."""

from __future__ import print_function

import argparse
import errno
import os
import sys


DEFAULT_PYLINTRC_PATH = os.path.join(
    os.path.dirname(os.path.realpath(__file__)), 'pylintrc')


def get_parser():
    """Return a command line parser."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--init-hook', help='Init hook commands to run.')
    parser.add_argument('--executable-path',
                        help='The path of the pylint executable.',
                        default='pylint')
    parser.add_argument('--no-rcfile',
                        help='Specify to use the executable\'s default '
                        'configuration.',
                        action='store_true')
    parser.add_argument('files', nargs='+')
    return parser


def main(argv):
    """The main entry."""
    parser = get_parser()
    opts, unknown = parser.parse_known_args(argv)

    cmd = [opts.executable_path]
    if not opts.no_rcfile:
        # We assume pylint is running in the top directory of the project,
        # so load the pylintrc file from there if it's available.
        pylintrc = os.path.abspath('pylintrc')
        if not os.path.exists(pylintrc):
            pylintrc = DEFAULT_PYLINTRC_PATH
            # If we pass a non-existent rcfile to pylint, it'll happily ignore
            # it.
            assert os.path.exists(pylintrc), 'Could not find %s' % pylintrc
        cmd += ['--rcfile', pylintrc]

    cmd += unknown + opts.files

    if opts.init_hook:
        cmd += ['--init-hook', opts.init_hook]

    try:
        os.execvp(cmd[0], cmd)
    except OSError as e:
        if e.errno == errno.ENOENT:
            print('%s: unable to run `%s`: %s' % (__file__, cmd[0], e),
                  file=sys.stderr)
            print('%s: Try installing pylint: sudo apt-get install pylint' %
                  (__file__,), file=sys.stderr)
            return 1
        else:
            raise


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
