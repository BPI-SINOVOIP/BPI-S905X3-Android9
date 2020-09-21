# Copyright (c) 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
# The objective of this test is to identify crashes outlined in the below
# directories while tests run.

import logging

class CrashDetector(object):
    """ Identifies all crash files outlined in the below (crash_paths)
    directories while tests run and get_crash_files finally retuns the list of
    crash files found so that the count/file names can be used for further
    analysis. At present this test looks for .meta and .kcrash files as defined
    in crash_signs variable"""
    version = 1

    def __init__(self, host):
        """Initilize variables and lists used in script

        @param host: this function runs on client or as defined

        """
        self.client = host
        self.crash_paths = ['/var/spool', '/home/chronos', '/home/chronos/u-*']
        self.crash_signs = ('\( -name "*.meta*" -o -name "*.kcrash*" \) '
                            '-printf "%f \n"')
        self.files_found = []
        self.crash_files_list = []


    def remove_crash_files(self):
        """Delete crash files from host."""
        for crash_path in self.crash_paths:
            self.client.run('rm -rf %s/crash' % crash_path,
                          ignore_status=True)


    def add_crash_files(self, crash_files, crash_path):
        """Checks if files list is empty and then adds to files_list

        @param crash_files: list of crash files found in crash_path
        @param crash_path: the path under which crash files are searched for

        """
        for crash_file in crash_files:
            if crash_file is not '':
                self.files_found.append(crash_file)
                logging.info('CRASH DETECTED in %s/crash: %s',
                             crash_path, crash_file)


    def find_crash_files(self):
        """Gets crash files from crash_paths and adds them to list."""
        for crash_path in self.crash_paths:
            if str(self.client.run('ls %s' % crash_path,
                   ignore_status=True)).find('crash') != -1:
                crash_out = self.client.run("find {0} {1} ".format(crash_path,
                                    self.crash_signs),ignore_status=True).stdout
                crash_files = crash_out.strip().split('\n')
                self.add_crash_files(crash_files, crash_path)


    def get_new_crash_files(self):
        """ Gets the newly generated files since last .

        @returns list of newly generated crashes
        """
        self.find_crash_files()
        files_collected = set(self.files_found)
        crash_files = set(self.crash_files_list)

        diff = list(files_collected.difference(crash_files))
        if diff:
            self.crash_files_list.extend(diff)
        return diff


    def is_new_crash_present(self):
        """Checks for kernel, browser, process crashes on host

        @returns False if there are no crashes; True otherwise

        """
        if self.get_new_crash_files():
            return True
        return False


    def get_crash_files(self):
        """Gets the list of crash_files collected during the tests whenever
        is_new_crash_present method is called.

        @returns the list of crash files collected duing the test run
        """
        self.is_new_crash_present()
        return self.crash_files_list
