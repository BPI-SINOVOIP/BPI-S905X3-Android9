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

"""Wrapper to run google-java-format to check for any malformatted changes."""

from __future__ import print_function

import argparse
import os
import sys
from distutils.spawn import find_executable

_path = os.path.realpath(__file__ + '/../..')
if sys.path[0] != _path:
    sys.path.insert(0, _path)
del _path

# We have to import our local modules after the sys.path tweak.  We can't use
# relative imports because this is an executable program, not a module.
# pylint: disable=wrong-import-position
import rh.shell
import rh.utils


def get_parser():
    """Return a command line parser."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--google-java-format', default='google-java-format',
                        help='The path of the google-java-format executable.')
    parser.add_argument('--google-java-format-diff',
                        default='google-java-format-diff.py',
                        help='The path of the google-java-format-diff script.')
    parser.add_argument('--fix', action='store_true',
                        help='Fix any formatting errors automatically.')
    parser.add_argument('--commit', type=str, default='HEAD',
                        help='Specify the commit to validate.')
    # While the formatter defaults to sorting imports, in the Android codebase,
    # the standard import order doesn't match the formatter's, so flip the
    # default to not sort imports, while letting callers override as desired.
    parser.add_argument('--sort-imports', action='store_true',
                        help='If true, imports will be sorted.')
    parser.add_argument('files', nargs='*',
                        help='If specified, only consider differences in '
                             'these files.')
    return parser


def main(argv):
    """The main entry."""
    parser = get_parser()
    opts = parser.parse_args(argv)

    # google-java-format-diff.py looks for google-java-format in $PATH, so find
    # the parent dir up front and inject it into $PATH when launching it.
    # TODO: Pass the path in directly once this issue is resolved:
    # https://github.com/google/google-java-format/issues/108
    format_path = find_executable(opts.google_java_format)
    if not format_path:
        print('Unable to find google-java-format at %s' %
              opts.google_java_format)
        return 1

    extra_env = {
        'PATH': '%s%s%s' % (os.path.dirname(format_path),
                            os.pathsep,
                            os.environ['PATH'])
    }

    # TODO: Delegate to the tool once this issue is resolved:
    # https://github.com/google/google-java-format/issues/107
    diff_cmd = ['git', 'diff', '--no-ext-diff', '-U0', '%s^!' % opts.commit]
    diff_cmd.extend(['--'] + opts.files)
    diff = rh.utils.run_command(diff_cmd, capture_output=True).output

    cmd = [opts.google_java_format_diff, '-p1', '--aosp']
    if opts.fix:
        cmd.extend(['-i'])
    if not opts.sort_imports:
        cmd.extend(['--skip-sorting-imports'])

    stdout = rh.utils.run_command(cmd,
                                  input=diff,
                                  capture_output=True,
                                  extra_env=extra_env).output
    if stdout != '':
        print('One or more files in your commit have Java formatting errors.')
        print('You can run `%s --fix %s` to fix this' %
              (sys.argv[0], rh.shell.cmd_to_str(argv)))
        return 1

    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
