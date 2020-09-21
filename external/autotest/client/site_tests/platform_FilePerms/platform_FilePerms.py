#!/usr/bin/python
#
# Copyright (c) 2010 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

__author__ = 'kdlucas@chromium.org (Kelly Lucas)'

import logging
import os
import re
import shutil
import stat

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error

class platform_FilePerms(test.test):
    """
    Test file permissions.
    """
    version = 2
    mount_path = '/bin/mount'
    standard_options = ['nosuid', 'nodev', 'noexec']
    standard_rw_options = ['rw'] + standard_options
    standard_ro_options = ['ro'] + standard_options
    loop_device = r'/dev/loop[0-9]+$'
    root_device = r'/dev/root$'
    # When adding an expectation that isn't simply "standard_*_options",
    # please leave either an explanation for why that mount is special,
    # or a bug number tracking work to harden that mount point, in a comment.
    expected_mount_options = {
        '/dev': {
            'type': ['devtmpfs'],
            'options': ['rw', 'nosuid', 'noexec', 'mode=755']},
        '/dev/pstore': {
            'type': ['pstore'],
            'options': standard_rw_options},
        '/sys/fs/pstore': {
            'type': ['pstore'],
            'options': standard_rw_options},
        '/dev/pts': { # Special case, we want to track gid/mode too.
            'type': ['devpts'],
            'options': ['rw', 'nosuid', 'noexec', 'gid=5', 'mode=620']},
        '/dev/shm': {'type': ['tmpfs'], 'options': standard_rw_options},
        '/home': {'type': ['ext4'], 'options': standard_rw_options},
        '/home/chronos': {'type': ['ext4'], 'options': standard_rw_options},
        '/media': {'type': ['tmpfs'], 'options': standard_rw_options},
        '/mnt/stateful_partition': {
            'type': ['ext4'],
            'options': standard_rw_options},
        '/mnt/stateful_partition/encrypted': {
            'type': ['ext4'],
            'options': standard_rw_options},
        '/opt/google/containers/android/rootfs/android-data/data/dalvik-cache/'
        'arm': {
            'type': ['ext4'],
            'options': standard_rw_options},
        '/opt/google/containers/android/rootfs/android-data/data/dalvik-cache/'
        'x86': {
            'type': ['ext4'],
            'options': standard_rw_options},
        '/opt/google/containers/android/rootfs/android-data/data/dalvik-cache/'
        'x86_64': {
            'type': ['ext4'],
            'options': standard_rw_options},
        # Special case - Android container has devices and suid programs.
        # Note that after the user logs in we remount it as "exec",
        # therefore we do not enforce 'noexec'.
        '/opt/google/containers/android/rootfs/root': {
            'device': loop_device,
            'type': ['squashfs'],
            'options': ['ro']},
        '/opt/google/containers/android/rootfs/root/system/lib/arm': {
            'device': loop_device,
            'type': ['squashfs'],
            'options': ['ro', 'nosuid', 'nodev']},
        '/opt/google/containers/arc-sdcard/mountpoints/container-root': {
            'device': loop_device,
            'type': ['squashfs'],
            'options': ['ro', 'noexec']},
        '/opt/google/containers/arc-downloads-filesystem/mountpoints/container-root': {
            'device': loop_device,
            'type': ['squashfs'],
            'options': ['ro', 'noexec']},
        '/opt/google/containers/arc-obb-mounter/mountpoints/container-root': {
            'device': loop_device,
            'type': ['squashfs'],
            'options': ['ro', 'noexec']},
        '/opt/google/containers/arc-removable-media/mountpoints/container-root': {
            'device': loop_device,
            'type': ['squashfs'],
            'options': ['ro', 'noexec']},
        '/run/arc/debugfs/sync': {
            'type': ['debugfs'],
            'options': standard_rw_options },
        '/run/arc/debugfs/tracing': {  # for devices in dev mode.
            'type': ['debugfs', 'tracefs'],
            'options': standard_rw_options },
        '/run/arc/media': {
            'type': ['tmpfs'] ,
            'options': standard_ro_options + ['mode=755']},
        '/run/arc/obb': {
            'type': ['tmpfs'],
            'options': standard_ro_options + ['mode=755']},
        '/run/arc/oem': {
            'type': ['tmpfs'],
            'options': standard_rw_options + ['mode=755']},
        '/run/arc/sdcard': {
            'type': ['tmpfs'],
            'options': standard_ro_options + ['mode=755']},
        '/run/arc/shared_mounts': {
            'type': ['tmpfs'],
            'options': standard_rw_options + ['mode=755']},
        '/proc': { 'type': ['proc'], 'options': standard_rw_options},
        '/run': { # Special case, we want to track mode too.
            'type': ['tmpfs'],
            'options': standard_rw_options + ['mode=755']},
        # Special case, we want to track group/mode too.
        # gid 605 == debugfs-access
        '/run/debugfs_gpu': {
            'type': ['debugfs'],
            'options': standard_rw_options + ['gid=605', 'mode=750']},
        '/run/lock': {'type': ['tmpfs'], 'options': standard_rw_options},
        '/run/imageloader/PepperFlashPlayer': {
            'type': ['squashfs'],
            'options': ['ro', 'nodev', 'nosuid']},
        '/sys': {'type': ['sysfs'], 'options': standard_rw_options},
        # TODO: /run is already a tmpfs mountpoint, so ip should probably
        # be modified to detect that, and not create a second mountpoint.
        # crbug.com/757953
        '/run/netns': {'type': ['tmpfs'], 'options': standard_rw_options},
        '/sys/fs/cgroup': {
            'type': ['tmpfs'],
            'options': standard_rw_options + ['mode=755']},
        '/sys/fs/cgroup/cpu': {
            'type': ['cgroup'],
            'options': standard_rw_options},
        '/sys/fs/cgroup/cpuacct': {
            'type': ['cgroup'],
            'options': standard_rw_options},
        '/sys/fs/cgroup/cpuset': {
            'type': ['cgroup'],
            'options': standard_rw_options},
        '/sys/fs/cgroup/devices': {
            'type': ['cgroup'],
            'options': standard_rw_options},
        '/sys/fs/cgroup/freezer': {
            'type': ['cgroup'],
            'options': standard_rw_options},
        '/sys/fs/cgroup/schedtune': {
            'type': ['cgroup'],
            'options': standard_rw_options},
        '/sys/fs/fuse/connections': {
            'type': ['fusectl'],
            'options': standard_rw_options},
        '/sys/fs/selinux': {
            'type': ['selinuxfs'],
            'options': ['rw', 'nosuid', 'noexec']},
        '/sys/kernel/debug': {
            'type': ['debugfs'],
            'options': standard_rw_options},
        '/sys/kernel/debug/tracing': {
            'type': ['tracefs'],
            'options': standard_rw_options},
        '/sys/kernel/security': {
            'type': ['securityfs'],
            'options': standard_rw_options},
        '/tmp': {'type': ['tmpfs'], 'options': standard_rw_options},
        '/var': {'type': ['ext4'], 'options': standard_rw_options},
        '/usr/share/oem': {
            'type': ['ext4'],
            'options': standard_ro_options},
        '/run/imageloader': {
            'type': ['tmpfs'],
            'options': standard_rw_options},
    }

    # /var/run and /var/lock are bind mounts of /run and /run/lock,
    # respectively. Duplicate the entries accordingly.
    expected_mount_options['/var/run'] = expected_mount_options['/run']
    expected_mount_options['/var/lock'] = expected_mount_options['/run/lock']

    # This lists mount points that host mounts created in developer mode, after
    # regular mount operations taking place during boot have completed. These
    # are ignored when checking mounts present in the live system.
    testmode_modded_fses = set(
        ['/home', '/tmp', '/usr/local', '/var/db/pkg', '/var/lib/portage'])


    def checkid(self, fs, userid):
        """
        Check that the uid and gid for |fs| match |userid|.

        @param fs: string, directory or file path.
        @param userid: userid to check for.
        Returns:
            int, the number errors (non-matches) detected.
        """
        errors = 0

        if not os.access(fs, os.F_OK):
            # The path does not exist, so exit early.
            return errors

        uid = os.stat(fs)[stat.ST_UID]
        gid = os.stat(fs)[stat.ST_GID]

        if userid != uid:
            logging.error('Wrong uid in filesystem "%s"', fs)
            errors += 1
        if userid != gid:
            logging.error('Wrong gid in filesystem "%s"', fs)
            errors += 1

        return errors


    def get_perm(self, fs):
        """
        Check the file permissions of a filesystem.

        @param fs: string, mount point for filesystem to check.
        Returns:
            int, equivalent to unix permissions.
        """
        MASK = 0777

        if not os.access(fs, os.F_OK):
            # The path does not exist, so exit early.
            return None

        fstat = os.stat(fs)
        mode = fstat[stat.ST_MODE]

        fperm = oct(mode & MASK)
        return fperm


    def read_mtab(self, mtab_path='/etc/mtab'):
        """
        Helper function to read the mtab file into a dict

        @param mtab_path: path to '/etc/mtab'
        Returns:
          dict, mount points as keys, and another dict with
          options list, device and type as values.
        """
        file_handle = open(mtab_path, 'r')
        lines = file_handle.readlines()
        file_handle.close()

        # Save mtab to the results dir to diagnose failures.
        shutil.copyfile(mtab_path,
                        os.path.join(self.resultsdir,
                                     os.path.basename(mtab_path)))

        comment_re = re.compile("#.*$")
        mounts = {}
        for line in lines:
            # remove any comments first
            line = comment_re.sub("", line)
            fields = line.split()
            # ignore malformed lines
            if len(fields) < 4:
                continue
            # Don't include rootfs in the list, because it maps to the
            # same location as /dev/root: '/' (and we don't care about
            # its options at the moment).
            if fields[0] == 'rootfs':
                continue
            # For ARC, normalize some container paths.
            fields[1] = re.sub(r'^/run/containers/android_[^/]{6}/',
                               r'/run/containers/android/',
                               fields[1])
            mounts[fields[1]] = {'device': fields[0],
                                 'type': fields[2],
                                 'options': fields[3].split(',')}
        return mounts


    def try_write(self, fs):
        """
        Try to write a file in the given filesystem.

        @param fs: string, file system to use.
        Returns:
            int, number of errors encountered:
            0 = write successful,
            >0 = write not successful.
        """

        TEXT = 'This is filler text for a test file.\n'

        tempfile = os.path.join(fs, 'test')
        try:
            fh = open(tempfile, 'w')
            fh.write(TEXT)
            fh.close()
        except OSError: # This error will occur with read only filesystem.
            return 1
        except IOError, e:
            return 1

        if os.path.exists(tempfile):
            os.remove(tempfile)

        return 0


    def check_mounted_read_only(self, filesystem):
        """
        Check the permissions of a filesystem according to /etc/mtab.

        @param filesystem: string, file system device to check.
        Returns:
            1 if rw, 0 if ro
        """

        errors = 0
        mtab = self.read_mtab()
        if not (filesystem in mtab.keys()):
            logging.error('Could not find filesystem "%s" in mtab', filesystem)
            errors += 1
            return errors # no point in continuing this test.
        if not ('ro' in mtab[filesystem]['options']):
            logging.error('Filesystem "%s" is not mounted read-only',
                          filesystem)
            errors += 1
        return errors


    def check_mount_options(self):
        """
        Check the permissions of all non-rootfs filesystems to make
        sure they have the right mount options. In order to do this,
        both the live system state, and a log-snapshot of what the system
        looked like prior to dev-mode/test-mode modifications were applied,
        are validated.

        Note that since this test is not a UITest, and takes place
        while the system waits at a login screen, mount options are
        not checked for a mounted cryptohome or guestfs. Consult the
        security_ProfilePermissions test for those checks.

        Args:
            (none)
        Returns:
            int, the number of errors identified in mount options.
        """
        errors = 0
        # Perform mount-option checks of both mount options as
        # captured during boot, and, the live system state.  After the
        # first pass (where we process mount_options.log), grow the
        # list of ignored filesystems to include all those we know are
        # tweaked by devmode/mod-for-test mode. This properly sets
        # expectations for the second pass.
        mtabs = ['/var/log/mount_options.log', '/etc/mtab']
        ignored_fses = set(['/'])
        # nsfs is ignored because it's a kernel filesystem used with
        # network namespaces, and is not a traditional filesystem where
        # arbitrary files may be created and executed.
        ignored_types = set(['ecryptfs','nsfs'])
        for mtab_path in mtabs:
            mtab = self.read_mtab(mtab_path=mtab_path)
            for fs in mtab.keys():
                if fs in ignored_fses:
                    continue

                fs_type = mtab[fs]['type']
                if fs_type in ignored_types:
                    logging.warning('Ignoring filesystem "%s" with type "%s"',
                                 fs, fs_type)
                    continue
                if fs in self.expected_mount_options:
                    mount_options = self.expected_mount_options[fs]
                else:
                    logging.error(
                            'No expectations entry for "%s" with info "%s"',
                            fs, mtab[fs])
                    errors += 1
                    continue

                if fs_type not in mount_options['type']:
                    logging.error(
                            '[%s] "%s" has type "%s", expected type "%s"',
                            mtab_path, fs, fs_type, mount_options['type'])
                    errors += 1

                device_type = mtab[fs]['device']
                if ('device' in mount_options) and (
                    re.match(mount_options['device'], device_type) is None):
                    logging.error(
                            '[%s] "%s" is device "%s", expected device "%s"',
                            mtab_path, fs, device_type, mount_options['device'])
                    errors += 1

                # For options, require the specified options to be present.
                # Do not consider it an error if extra options are present.
                # (This makes it easy to deal with options we don't wish
                # to track closely, like devtmpfs's nr_inodes= for example.)
                seen = set(mtab[fs]['options'])
                expected = set(mount_options['options'])
                missing = expected - seen
                if (missing):
                    logging.error('[%s] "%s" is missing options "%s"',
                                  mtab_path, fs, missing)
                    errors += 1

            ignored_fses.update(self.testmode_modded_fses)

        return errors


    def run_once(self):
        """
        Main testing routine for platform_FilePerms.
        """
        errors = 0

        # Root owned directories with expected permissions.
        root_dirs = {'/': ['0755'],
                     '/bin': ['0755'],
                     '/boot': ['0755'],
                     '/dev': ['0755'],
                     '/etc': ['0755'],
                     '/home': ['0755'],
                     '/lib': ['0755'],
                     '/media': ['0777'],
                     '/mnt': ['0755'],
                     '/mnt/stateful_partition': ['0755'],
                     '/opt': ['0755'],
                     '/proc': ['0555'],
                     '/run': ['0755'],
                     '/sbin': ['0755'],
                     '/sys': ['0555', '0755'],
                     '/tmp': ['0777'],
                     '/usr': ['0755'],
                     '/usr/bin': ['0755'],
                     '/usr/lib': ['0755'],
                     '/usr/local': ['0755'],
                     '/usr/sbin': ['0755'],
                     '/usr/share': ['0755'],
                     '/var': ['0755'],
                     '/var/cache': ['0755']}

        # Read-only directories
        ro_dirs = ['/', '/bin', '/boot', '/etc', '/lib', '/mnt',
                   '/opt', '/sbin', '/usr', '/usr/bin', '/usr/lib',
                   '/usr/sbin', '/usr/share']

        # Root directories writable by root
        root_rw_dirs = ['/run', '/var', '/var/lib', '/var/cache', '/var/log',
                        '/usr/local']

        # Ensure you cannot write files in read only directories.
        for dir in ro_dirs:
            if self.try_write(dir) == 0:
                logging.error('Root can write to read-only dir "%s"', dir)
                errors += 1

        # Ensure the uid and gid are correct for root owned directories.
        for dir in root_dirs:
            if self.checkid(dir, 0) > 0:
                errors += 1

        # Ensure root can write into root dirs with rw access.
        for dir in root_rw_dirs:
            if self.try_write(dir) > 0:
                errors += 1

        # Check permissions on root owned directories.
        for dir in root_dirs:
            fperms = self.get_perm(dir)
            if fperms is not None and fperms not in root_dirs[dir]:
                logging.error('"%s" has "%s" permissions', dir, fperms)
                errors += 1

        errors += self.check_mounted_read_only('/')

        # Check mount options on mount points.
        errors += self.check_mount_options()

        # If errors is not zero, there were errors.
        if errors > 0:
            raise error.TestFail('Found %d permission errors' % errors)
