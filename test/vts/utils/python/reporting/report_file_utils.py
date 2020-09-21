#
# Copyright (C) 2017 The Android Open Source Project
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

import datetime
import logging
import os
import shutil


def NotNoneStr(item):
    '''Convert a veriable to string only if it is not None'''
    return str(item) if item is not None else None


class ReportFileUtil(object):
    '''Utility class for report file saving.

    Contains methods to save report files or read incremental parts of
    report files to a destination folder and get URLs.
    Used by profiling util, systrace util, and host log reporting.

    Attributes:
        _flatten_source_dir: bool, whether to flatten source directory
                             structure in destination directory. Current
                             implementation assumes no duplicated fine names
        _use_destination_date_dir: bool, whether to create date directory
                                   in destination directory
        _source_dir: string, source directory that contains report files
        _destination_dir: string, destination directory for report saving
        _url_prefix: string, a prefix added to relative destination file paths.
                     If set to None, will use parent directory path.
    '''

    def __init__(self,
                 flatten_source_dir=False,
                 use_destination_date_dir=False,
                 source_dir=None,
                 destination_dir=None,
                 url_prefix=None):
        source_dir = NotNoneStr(source_dir)
        destination_dir = NotNoneStr(destination_dir)
        url_prefix = NotNoneStr(url_prefix)

        self._flatten_source_dir = flatten_source_dir
        self._use_destination_date_dir = use_destination_date_dir
        self._source_dir = source_dir
        self._destination_dir = destination_dir
        self._url_prefix = url_prefix

    def _ConvertReportPath(self,
                           src_path,
                           root_dir=None,
                           new_file_name=None,
                           file_name_prefix=None):
        '''Convert report source file path to destination path and url.

        Args:
            src_path: string, source report file path.
            new_file_name: string, new file name to use on destination.
            file_name_prefix: string, prefix added to destination file name.
                              if new_file_name is set, prefix will be added
                              to new_file_name as well.

        Returns:
            tuple(string, string), containing destination path and url
        '''
        root_dir = NotNoneStr(root_dir)
        new_file_name = NotNoneStr(new_file_name)
        file_name_prefix = NotNoneStr(file_name_prefix)

        dir_path = os.path.dirname(src_path)

        relative_path = os.path.basename(src_path)
        if new_file_name:
            relative_path = new_file_name
        if file_name_prefix:
            relative_path = file_name_prefix + relative_path
        if not self._flatten_source_dir and root_dir:
            relative_path = os.path.join(
                os.path.relpath(dir_path, root_dir), relative_path)
        if self._use_destination_date_dir:
            now = datetime.datetime.now()
            date = now.strftime('%Y-%m-%d')
            relative_path = os.path.join(date, relative_path)

        dest_path = os.path.join(self._destination_dir, relative_path)

        url = dest_path
        if self._url_prefix is not None:
            url = self._url_prefix + relative_path

        return dest_path, url

    def _PushReportFile(self, src_path, dest_path):
        '''Push a report file to destination.

        Args:
            src_path: string, source path of report file
            dest_path: string, destination path of report file
        '''
        src_path = NotNoneStr(src_path)
        dest_path = NotNoneStr(dest_path)

        parent_dir = os.path.dirname(dest_path)
        if not os.path.exists(parent_dir):
            try:
                os.makedirs(parent_dir)
            except OSError as e:
                logging.exception(e)
        shutil.copy(src_path, dest_path)

    def SaveReport(self, src_path, new_file_name=None, file_name_prefix=None):
        '''Save report file to destination.

        Args:
            src_path: string, source report file path.
            new_file_name: string, new file name to use on destination.
            file_name_prefix: string, prefix added to destination file name.
                              if new_file_name is set, prefix will be added
                              to new_file_name as well.

        Returns:
            string, destination URL of saved report file.
            If url_prefix is set to None, will return destination path of
            report files. If error happens during read or write operation,
            this method will return None.
        '''
        src_path = NotNoneStr(src_path)
        new_file_name = NotNoneStr(new_file_name)
        file_name_prefix = NotNoneStr(file_name_prefix)

        try:
            dest_path, url = self._ConvertReportPath(
                src_path,
                new_file_name=new_file_name,
                file_name_prefix=file_name_prefix)
            self._PushReportFile(src_path, dest_path)

            return url
        except IOError as e:
            logging.exception(e)

    def SaveReportsFromDirectory(self,
                                 source_dir=None,
                                 file_name_prefix=None,
                                 file_path_filters=None):
        '''Save report files from source directory to destination.

        Args:
            source_dir: string, source directory where report files are stored.
                        if None, class attribute source_dir will be used.
                        Default is None.
            file_name_prefix: string, prefix added to destination file name
            file_path_filter: function, a functions that return True (pass) or
                              False (reject) given original file path.

        Returns:
            A list of string, containing destination URLs of saved report files.
            If url_prefix is set to None, will return destination path of
            report files. If error happens during read or write operation,
            this method will return None.
        '''
        source_dir = NotNoneStr(source_dir)
        file_name_prefix = NotNoneStr(file_name_prefix)
        if not source_dir:
            source_dir = self._source_dir

        try:
            urls = []

            for (dirpath, dirnames, filenames) in os.walk(
                    source_dir, followlinks=False):
                for filename in filenames:
                    src_path = os.path.join(dirpath, filename)
                    dest_path, url = self._ConvertReportPath(
                        src_path,
                        root_dir=source_dir,
                        file_name_prefix=file_name_prefix)

                    if file_path_filters and not file_path_filters(src_path):
                        continue

                    #TODO(yuexima): handle duplicated destination file names
                    self._PushReportFile(src_path, dest_path)
                    urls.append(url)

            return urls
        except IOError as e:
            logging.exception(e)
