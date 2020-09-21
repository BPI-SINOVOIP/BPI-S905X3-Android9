# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import glob
import logging
import os
import tempfile

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import utils

PARTITION_TEST_PATH = 'platform_SecureEraseFile_test_file'
TEST_PATH = '/mnt/stateful_partition/' + PARTITION_TEST_PATH
BINARY = '/usr/bin/secure_erase_file'
DEVNAME_PREFIX = 'DEVNAME='

class platform_SecureEraseFile(test.test):
    """Validate secure_erase_file tool behavior.

    We can't verify from this test that data has been destroyed from the
    underlying physical device, but we can confirm that it's not reachable from
    userspace.
    """
    version = 1

    def __write_test_file(self, path, blocksize, count):
        cmd = '/bin/dd if=/dev/urandom of=%s bs=%s count=%d' % (
                path, blocksize, count)
        utils.run(cmd)
        if not os.path.exists(path):
            raise error.TestError('Failed to generate test file')


    def __get_partition(self, path):
        info = os.lstat(path)
        major = os.major(info.st_dev)
        minor = os.minor(info.st_dev)
        uevent_path = '/sys/dev/block/%d:%d/uevent' % (major, minor)
        with open(uevent_path, 'r') as uevent_file:
            for l in uevent_file.readlines():
                if l.startswith(DEVNAME_PREFIX):
                    return '/dev/' + l[len(DEVNAME_PREFIX):].strip()
        raise error.TestError('Unable to find partition for path: ' + path)


    def __get_extents(self, path, partition):
        extents = []
        cmd = 'debugfs -R "extents %s" %s' % (path, partition)
        result = utils.run(cmd)
        for line in result.stdout.splitlines():
            # Discard header line.
            if line.startswith('Level'):
                continue
            fields = line.split()

            # Ignore non-leaf extents
            if fields[0].strip('/') != fields[1]:
                continue
            extents.append({'offset': fields[7], 'length': fields[10]})

        return extents


    def __verify_cleared(self, partition, extents):
        out_path = tempfile.mktemp()
        for e in extents:
            cmd = 'dd if=%s bs=4K skip=%s count=%s of=%s' % (
                   partition, e['offset'], e['length'], out_path)
            utils.run(cmd)
            with open(out_path, 'r') as out_file:
                d = out_file.read()
                for i, byte in enumerate(d):
                    if ord(byte) != 0x00 and ord(byte) != 0xFF:
                        logging.info('extent[%d] = %s', i, hex(ord(byte)))
                        raise error.TestError('Bad byte found')


    def __test_and_verify_cleared(self, blocksize, count):
        self.__write_test_file(TEST_PATH, blocksize, count)
        utils.run('sync')

        logging.info('original file contents: ')
        res = utils.run('xxd %s' % TEST_PATH)
        logging.info(res.stdout)

        partition = self.__get_partition(TEST_PATH)
        extents = self.__get_extents(PARTITION_TEST_PATH, partition)
        if len(extents) == 0:
            raise error.TestError('No extents found for ' + TEST_PATH)

        utils.run('%s %s' % (BINARY, TEST_PATH))

        # secure_erase_file confirms that the file has been erased and that its
        # contents are not accessible. If that is not the case, it will return
        # with a non-zero exit code.
        if os.path.exists(TEST_PATH):
            raise error.TestError('Secure Erase failed to unlink file.')

        self.__verify_cleared(partition, extents)


    def run_once(self):
        # Secure erase is only supported on eMMC today; pass if
        # no device is present.
        if len(glob.glob('/dev/mmcblk*')) == 0:
            raise error.TestNAError('Skipping test; no eMMC device found.')

        self.__test_and_verify_cleared('64K', 2)
        self.__test_and_verify_cleared('1M', 16)

    def after_run_once(self):
        if os.path.exists(TEST_PATH):
            os.unlink(TEST_PATH)

