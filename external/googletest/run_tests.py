#!/usr/bin/env python
#
# Copyright (C) 2016 The Android Open Source Project
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
#
"""Runs all tests for gtest/gmock."""
import argparse
import logging
import os
import sys


# pylint: disable=design


def logger():
    """Return the default logger for the module."""
    return logging.getLogger(__name__)


def call(cmd, *args, **kwargs):
    """Proxy for subprocess.call with logging."""
    import subprocess
    logger().info('call `%s`', ' '.join(cmd))
    return subprocess.call(cmd, *args, **kwargs)


def parse_args():
    "Parse and return command line arguments."""
    parser = argparse.ArgumentParser()
    parser.add_argument('--host', action='store_true')
    parser.add_argument('-v', '--verbose', action='store_true')
    return parser.parse_args()


def main():
    "Program entry point."""
    args = parse_args()
    log_level = logging.INFO
    if args.verbose:
        log_level = logging.DEBUG
    logging.basicConfig(level=log_level)

    if args.host:
        test_location = os.path.join(os.environ['ANDROID_HOST_OUT'], 'bin')
    else:
        data_dir = os.path.join(os.environ['OUT'], 'data')
        test_location = os.path.join(data_dir, 'nativetest64')
        if not os.path.exists(test_location):
            test_location = os.path.join(data_dir, 'nativetest')

    num_tests = 0
    failures = []
    logger().debug('Scanning %s for tests', test_location)
    for test in os.listdir(test_location):
        if not test.startswith('gtest') and not test.startswith('gmock'):
            logger().debug('Skipping %s', test)
            continue
        num_tests += 1

        if args.host:
            cmd = [os.path.join(test_location, test)]
            if call(cmd) != 0:
                failures.append(test)
        else:
            device_dir = test_location.replace(os.environ['OUT'], '')
            cmd = ['adb', 'shell', 'cd {} && ./{}'.format(device_dir, test)]
            if call(cmd) != 0:
                failures.append(test)

    if num_tests == 0:
        logger().error('No tests found!')
        sys.exit(1)

    num_failures = len(failures)
    num_passes = num_tests - num_failures
    logger().info('%d/%d tests passed', num_passes, num_tests)
    if len(failures) > 0:
        logger().error('Failures:\n%s', '\n'.join(failures))
    else:
        logger().info('All tests passed!')
    sys.exit(num_failures)


if __name__ == '__main__':
    main()
