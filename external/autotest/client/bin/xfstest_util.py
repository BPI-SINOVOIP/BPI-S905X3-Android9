# Copyright (c) 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Library to run xfstests in autotest.

"""

import os
from autotest_lib.client.bin import partition

class xfstests_env:
    """
    Setup the environment for running xfstests.
    """
    XFS_MOUNT_PATH = '/mnt/stateful_partition/unencrypted/cache'

    env_names={}
    env_partition={}
    env_vp={}
    env_device={}
    fs_types={}

    def setup_partitions(self, job, fs_types, crypto=False,
                         env_names=['TEST', 'SCRATCH']):
        """
        xfttests needs 2 partitions: TEST and SCRATCH.
        - TEST_DEV: "device containing TEST PARTITION"
        - TEST_DIR: "mount point of TEST PARTITION"
        - SCRATCH_DEV "device containing SCRATCH PARTITION"
        - SCRATCH_MNT "mount point for SCRATCH PARTITION"

        @param job: The job object.
        """

        self.env_names = env_names
        self.fs_types = fs_types
        for name in self.env_names:
            file_name = 'xfstests_%s' % name
            file_img = os.path.join(
                self.XFS_MOUNT_PATH, '%s.img' % file_name)
            self.env_vp[name] = partition.virtual_partition(
                file_img=file_img, file_size=1024)
            self.env_device[name] = self.env_vp[name].device

            # You can use real block devices, such as /dev/sdc1 by populating
            # env_device directly, but going through the virtual partition
            # object.

            # By default, we create a directory under autotest
            mountpoint = os.path.join(job.tmpdir, file_name)
            if not os.path.isdir(mountpoint):
                os.makedirs(mountpoint)

            self.env_partition[name] = job.partition(
                device=self.env_device[name], mountpoint=mountpoint)

        #
        # Job configuration, instead of editing xfstests config files, set them
        # right here as environment variables
        #
        for name in self.env_names:
            os.environ['%s_DEV' % name] = self.env_partition[name].device

        test_dir = self.env_partition['TEST'].mountpoint

        os.environ['TEST_DIR'] = test_dir
        os.environ['SCRATCH_MNT'] = self.env_partition['SCRATCH'].mountpoint

        # ChromeOS does not need special option when SELinux is enabled.
        os.environ['SELINUX_MOUNT_OPTIONS'] = ' '

        mkfs_args = ''
        mnt_options = ''
        if crypto:
            mkfs_args += '-O encrypt'
            mnt_options += '-o test_dummy_encryption'

        for fs_type in self.fs_types:
            for name in self.env_names:
                self.env_partition[name].mkfs(fstype=fs_type, args=mkfs_args)

        os.environ['EXT_MOUNT_OPTIONS'] = mnt_options

    def unmount_partitions(self):
        """
        Unmount the partition created.
        """
        for name in self.env_names:
            self.env_partition[name].unmount(ignore_status=True)
            self.env_vp[name].destroy()
