#!/usr/bin/python

# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Command line tool to pack audio related modules into a zip file."""

import argparse
import logging
import os
import subprocess


MODULES = ['audio_quality_measurement.py', 'audio_data.py', 'audio_analysis.py']
ENTRY = '__main__.py'
ENTRY_TARGET = 'check_quality.py'

def add_args(parser):
    """Adds command line arguments."""
    parser.add_argument('-d', '--debug', action='store_true', default=False,
                        help='Show debug message.')
    parser.add_argument('-o', '--out', type=str, default='audio_quality.zip',
                        help='Output file name. Default is audio_quality.zip.')
    parser.add_argument('-s', '--skip-cleanup', action='store_true', default=False,
                        help='Skip cleaning up temporary files. Default is False')


def parse_args(parser):
    """Parses args.

    @param parser: An argparse.ArgumentParser.

    @returns: The namespace parsed from command line arguments.

    """
    args = parser.parse_args()
    return args


def create_link():
    """Creates a symbolic link from ENTRY to ENTRY_TARGET.

    With this symlink, python can execute the zip file directly to execute
    ENTRY_TARGET.

    """
    command = ['ln', '-sf', ENTRY_TARGET, ENTRY]
    logging.debug('Link command: %s', command)
    subprocess.check_call(command)


def pack_files(out_file):
    """Packs audio related modules into a zip file.

    Packs audio related modules in MODULES into a zip file.
    Packs the symlink pointing to ENTRY_TARGET.

    @param out_file: Zip file name.

    """
    command = ['zip']
    command.append(out_file)
    command += MODULES
    command.append(ENTRY)
    command.append(ENTRY_TARGET)
    logging.debug('Zip command: %s', command)
    subprocess.check_call(command)


def check_packed_file(out_file):
    """Checks the packed file can be executed by python.

    @param out_file: Zip file name.

    """
    command = ['python', out_file, '--help']
    logging.debug('Check command: %s', command)
    output = subprocess.check_output(command)
    logging.debug('output: %s', output)


def cleanup():
    """Cleans up the symobolic link."""
    if os.path.exists(ENTRY):
        os.unlink(ENTRY)


def repo_is_dirty():
    """Checks if a repo is dirty by git diff command.

    @returns: True if there are uncommitted changes. False otherwise.

    """
    try:
        subprocess.check_call(['git', 'diff', '--quiet'])
        subprocess.check_call(['git', 'diff', '--cached', '--quiet'])
    except subprocess.CalledProcessError:
        return True
    return False


def get_git_sha1():
    """Returns git SHA-1 hash of HEAD.

    @returns: git SHA-1 has of HEAD with minimum length 9.

    """
    return subprocess.check_output(
            ['git', 'rev-parse', '--short', 'HEAD']).strip()


def append_name_with_git_hash(out_file):
    """Append the file with git SHA-1 hash.

    For out_file like ABC.xyz, append the name ABC with git SHA-1 of HEAD, like
    ABC_f4610bdd3.xyz.
    If current repo contains uncommitted changes, it will be
    ABC_f4610bdd3_dirty.xyz.

    """
    basename, ext = os.path.splitext(out_file)
    basename += '_'
    basename += get_git_sha1()
    if repo_is_dirty():
        basename += '_dirty'
    return basename + ext


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='Pack audio related modules into a zip file.')

    add_args(parser)
    args = parse_args(parser)

    level = logging.DEBUG if args.debug else logging.INFO
    format = '%(asctime)-15s:%(levelname)s:%(pathname)s:%(lineno)d: %(message)s'
    logging.basicConfig(format=format, level=level)

    out_file = append_name_with_git_hash(args.out)

    try:
        create_link()
        pack_files(out_file)
        check_packed_file(out_file)
    finally:
        if not args.skip_cleanup:
            cleanup()

    logging.info('Packed file: %s', out_file)
