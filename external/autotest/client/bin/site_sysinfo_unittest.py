#!/usr/bin/python

"""Tests for site_sysinfo."""

__author__ = 'dshi@google.com (Dan Shi)'

import cPickle as pickle
import os
import random
import shutil
import tempfile
import unittest

import common
from autotest_lib.client.bin import site_sysinfo
from autotest_lib.client.common_lib import autotemp


class diffable_logdir_test(unittest.TestCase):
    """Tests for methods in class diffable_logdir."""


    def setUp(self):
        """Initialize a temp direcotry with test files."""
        self.tempdir = autotemp.tempdir(unique_id='diffable_logdir')
        self.src_dir = os.path.join(self.tempdir.name, 'src')
        self.dest_dir = os.path.join(self.tempdir.name, 'dest')

        self.existing_files = ['existing_file_'+str(i) for i in range(3)]
        self.existing_files_folder = ['', 'sub', 'sub/sub2']
        self.existing_files_path = [os.path.join(self.src_dir, folder, f)
                                    for f,folder in zip(self.existing_files,
                                                self.existing_files_folder)]
        self.new_files = ['new_file_'+str(i) for i in range(2)]
        self.new_files_folder = ['sub', 'sub/sub3']
        self.new_files_path = [os.path.join(self.src_dir, folder, f)
                                    for f,folder in zip(self.new_files,
                                                self.new_files_folder)]

        # Create some file with random data in source directory.
        for p in self.existing_files_path:
            self.append_text_to_file(str(random.random()), p)


    def tearDown(self):
        """Clearn up."""
        self.tempdir.clean()


    def append_text_to_file(self, text, file_path):
        """Append text to the end of a file, create the file if not existed.

        @param text: text to be appended to a file.
        @param file_path: path to the file.

        """
        dir_name = os.path.dirname(file_path)
        if not os.path.exists(dir_name):
            os.makedirs(dir_name)
        with open(file_path, 'a') as f:
            f.write(text)


    def test_diffable_logdir_success(self):
        """Test the diff function to save new data from a directory."""
        info = site_sysinfo.diffable_logdir(self.src_dir,
                                            keep_file_hierarchy=False,
                                            append_diff_in_name=False)
        # Run the first time to collect file status.
        info.run(log_dir=None, collect_init_status=True)

        # Add new files to the test directory.
        for file_name, file_path in zip(self.new_files,
                                         self.new_files_path):
            self.append_text_to_file(file_name, file_path)

        # Temp file for existing_file_2, used to hold on the inode. If the
        # file is deleted and recreated, its inode might not change.
        existing_file_2 = self.existing_files_path[2]
        existing_file_2_tmp =  existing_file_2 + '_tmp'
        os.rename(existing_file_2, existing_file_2_tmp)

        # Append data to existing file.
        for file_name, file_path in zip(self.existing_files,
                                         self.existing_files_path):
            self.append_text_to_file(file_name, file_path)

        # Remove the tmp file.
        os.remove(existing_file_2_tmp)

        # Run the second time to do diff.
        info.run(self.dest_dir, collect_init_status=False)

        # Validate files in dest_dir.
        for file_name, file_path in zip(self.existing_files+self.new_files,
                                self.existing_files_path+self.new_files_path):
            file_path = file_path.replace('src', 'dest')
            with open(file_path, 'r') as f:
                self.assertEqual(file_name, f.read())


class LogdirTestCase(unittest.TestCase):
    """Tests logdir.run"""

    def setUp(self):
        self.tempdir = tempfile.mkdtemp()
        self.addCleanup(shutil.rmtree, self.tempdir)

        self.from_dir = os.path.join(self.tempdir, 'from')
        self.to_dir = os.path.join(self.tempdir, 'to')
        os.mkdir(self.from_dir)
        os.mkdir(self.to_dir)

    def _destination_path(self, relative_path, from_dir=None):
        """The expected destination path for a subdir of the source directory"""
        if from_dir is None:
            from_dir = self.from_dir
        return '%s%s' % (self.to_dir, os.path.join(from_dir, relative_path))

    def test_run_recreates_absolute_source_path(self):
        """When copying files, the absolute path from the source is recreated
        in the destination folder.
        """
        os.mkdir(os.path.join(self.from_dir, 'fubar'))
        logdir = site_sysinfo.logdir(self.from_dir)
        logdir.run(self.to_dir)
        destination_path= self._destination_path('fubar')
        self.assertTrue(os.path.exists(destination_path),
                        msg='Failed to copy to %s' % destination_path)

    def test_run_skips_symlinks(self):
        os.mkdir(os.path.join(self.from_dir, 'real'))
        os.symlink(os.path.join(self.from_dir, 'real'),
                   os.path.join(self.from_dir, 'symlink'))

        logdir = site_sysinfo.logdir(self.from_dir)
        logdir.run(self.to_dir)

        destination_path_real = self._destination_path('real')
        self.assertTrue(os.path.exists(destination_path_real),
                        msg='real directory was not copied to %s' %
                        destination_path_real)
        destination_path_link = self._destination_path('symlink')
        self.assertFalse(
                os.path.exists(destination_path_link),
                msg='symlink was copied to %s' % destination_path_link)

    def test_run_resolves_symlinks_to_source_root(self):
        """run tries hard to get to the source directory before copying.

        Within the source folder, we skip any symlinks, but we first try to
        resolve symlinks to the source root itself.
        """
        os.mkdir(os.path.join(self.from_dir, 'fubar'))
        from_symlink = os.path.join(self.tempdir, 'from_symlink')
        os.symlink(self.from_dir, from_symlink)

        logdir = site_sysinfo.logdir(from_symlink)
        logdir.run(self.to_dir)

        destination_path = self._destination_path('fubar')
        self.assertTrue(os.path.exists(destination_path),
                        msg='Failed to copy to %s' % destination_path)

    def test_run_excludes_common_patterns(self):
        os.mkdir(os.path.join(self.from_dir, 'autoserv2344'))
        deeper_subdir = os.path.join('prefix', 'autoserv', 'suffix')
        os.makedirs(os.path.join(self.from_dir, deeper_subdir))

        logdir = site_sysinfo.logdir(self.from_dir)
        logdir.run(self.to_dir)

        destination_path = self._destination_path('autoserv2344')
        self.assertFalse(os.path.exists(destination_path),
                         msg='Copied banned file to %s' % destination_path)
        destination_path = self._destination_path(deeper_subdir)
        self.assertFalse(os.path.exists(destination_path),
                         msg='Copied banned file to %s' % destination_path)

    def test_run_ignores_exclude_patterns_in_leading_dirs(self):
        """Exclude patterns should only be applied to path suffixes within
        from_dir, not to the root directory itself.
        """
        exclude_pattern_dir = os.path.join(self.from_dir, 'autoserv2344')
        os.makedirs(os.path.join(exclude_pattern_dir, 'fubar'))
        logdir = site_sysinfo.logdir(exclude_pattern_dir)
        logdir.run(self.to_dir)
        destination_path = self._destination_path('fubar',
                                                  from_dir=exclude_pattern_dir)
        self.assertTrue(os.path.exists(destination_path),
                        msg='Failed to copy to %s' % destination_path)

    def test_pickle_unpickle_equal(self):
        """Sanity check pickle-unpickle round-trip."""
        logdir = site_sysinfo.logdir(
                self.from_dir,
                excludes=(site_sysinfo.logdir.DEFAULT_EXCLUDES + ('a',)))
        # base_job uses protocol 2 to pickle. We follow suit.
        logdir_pickle = pickle.dumps(logdir, protocol=2)
        unpickled_logdir = pickle.loads(logdir_pickle)

        self.assertEqual(unpickled_logdir, logdir)

    def test_pickle_enforce_required_attributes(self):
        """Some attributes from this object should never be dropped.

        When running a client test against an older build, we pickle the objects
        of this class from newer version of the class and unpicle to older
        versions of the class. The older versions require some attributes to be
        present.
        """
        logdir = site_sysinfo.logdir(
                self.from_dir,
                excludes=(site_sysinfo.logdir.DEFAULT_EXCLUDES + ('a',)))
        # base_job uses protocol 2 to pickle. We follow suit.
        logdir_pickle = pickle.dumps(logdir, protocol=2)
        logdir = pickle.loads(logdir_pickle)

        self.assertEqual(logdir.additional_exclude, 'a')

    def test_pickle_enforce_required_attributes_default(self):
        """Some attributes from this object should never be dropped.

        When running a client test against an older build, we pickle the objects
        of this class from newer version of the class and unpicle to older
        versions of the class. The older versions require some attributes to be
        present.
        """
        logdir = site_sysinfo.logdir(self.from_dir)
        # base_job uses protocol 2 to pickle. We follow suit.
        logdir_pickle = pickle.dumps(logdir, protocol=2)
        logdir = pickle.loads(logdir_pickle)

        self.assertEqual(logdir.additional_exclude, None)

    def test_unpickle_handle_missing__excludes(self):
        """Sanely handle missing _excludes attribute from pickles

        This can happen when running brand new version of this class that
        introduced this attribute from older server side code in prod.
        """
        logdir = site_sysinfo.logdir(self.from_dir)
        delattr(logdir, '_excludes')
        # base_job uses protocol 2 to pickle. We follow suit.
        logdir_pickle = pickle.dumps(logdir, protocol=2)
        logdir = pickle.loads(logdir_pickle)

        self.assertEqual(logdir._excludes,
                         site_sysinfo.logdir.DEFAULT_EXCLUDES)

    def test_unpickle_handle_missing__excludes_default(self):
        """Sanely handle missing _excludes attribute from pickles

        This can happen when running brand new version of this class that
        introduced this attribute from older server side code in prod.
        """
        logdir = site_sysinfo.logdir(
                self.from_dir,
                excludes=(site_sysinfo.logdir.DEFAULT_EXCLUDES + ('a',)))
        delattr(logdir, '_excludes')
        # base_job uses protocol 2 to pickle. We follow suit.
        logdir_pickle = pickle.dumps(logdir, protocol=2)
        logdir = pickle.loads(logdir_pickle)

        self.assertEqual(
                logdir._excludes,
                (site_sysinfo.logdir.DEFAULT_EXCLUDES + ('a',)))


if __name__ == '__main__':
    unittest.main()
