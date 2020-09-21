# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import re

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error

class hardware_SsdDetection(test.test):
    """Verify that a flash device is present. """

    version = 1
    # Keep a list of boards that are expected to ship with hard drive.
    boards_with_hdd = ['butterfly', 'kiev', 'parrot', 'stout']

    def setup(self):
        """
        create a empty srcdir to prevent the error that checks
        .version file
        """
        if not os.path.exists(self.srcdir):
            utils.system('mkdir %s' % self.srcdir)


    def run_once(self, check_link_speed=()):
        """
        Use rootdev to find the underlying block device even if the
        system booted to /dev/dm-0.
        """
        device = utils.get_root_device()

        def is_fixed(dev):
            """ Check the device is fixed.

            @param dev: device to check, i.e. 'sda'.
            """
            sysfs_path = '/sys/block/%s/removable' % dev
            return (os.path.exists(sysfs_path) and
                    open(sysfs_path).read().strip() == '0')

        # Catch device name like sda, mmcblk0, nvme0n1.
        device_re = re.compile(r'^/dev/([a-zA-Z0-9]+)$')
        dev = device_re.findall(device)
        if len(dev) != 1 or not is_fixed(dev[0]):
            raise error.TestFail('The main disk %s is not fixed' % dev)

        # If it is an mmcblk or nvme device, then it is SSD.
        # Else run hdparm to check for SSD.
        if re.search("nvme", device):
            return

        if re.search("mmcblk", device):
            return

        hdparm = utils.run('/sbin/hdparm -I %s' % device)

        # Check if device is a SSD
        match = re.search(r'Nominal Media Rotation Rate: (.+)$',
                          hdparm.stdout, re.MULTILINE)
        if match and match.group(1):
            if match.group(1) != 'Solid State Device':
                if utils.get_board() in self.boards_with_hdd:
                    return
                raise error.TestFail('The main disk is not a SSD, '
                    'Rotation Rate: %s' % match.group(1))
        else:
            raise error.TestFail(
                'Rotation Rate not reported from the device, '
                'unable to ensure it is a SSD')

        # Check if SSD is > 8GB in size
        match = re.search("device size with M = 1000\*1000: (.+) MBytes",
                          hdparm.stdout, re.MULTILINE)
        if match and match.group(1):
            size = int(match.group(1))
            self.write_perf_keyval({"mb_ssd_device_size" : size})
        else:
            raise error.TestFail(
                'Device size info missing from the device')

        # Check supported link speed.
        #
        # check_link_speed is an empty tuple by default, which does not perform
        # link speed checking.  You can run the test while specifying
        # check_link_speed=('1.5Gb/s', '3.0Gb/s') to check the 2 signaling
        # speeds are both supported.
        for link_speed in check_link_speed:
            if not re.search(r'Gen. signaling speed \(%s\)' % link_speed,
                             hdparm.stdout, re.MULTILINE):
                raise error.TestFail('Link speed %s not supported' % link_speed)
