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

import logging
import re
import uuid

from vts.runners.host import utils
from vts.testcases.template.binary_test import binary_test_case
from vts.utils.python.os import path_utils


class GtestTestCase(binary_test_case.BinaryTestCase):
    '''A class to represent a gtest test case.

    Attributes:
        test_suite: string, test suite name
        test_name: string, test case name which does not include test suite
        path: string, absolute test binary path on device
        tag: string, test tag
        put_tag_func: function that takes a name and tag to output a combination
        output_file_path: string, gtest output xml path on device
    '''

    # @Override
    def GetRunCommand(self, output_file_path=None, test_name=None):
        '''Get the command to run the test.

        Args:
            output_file_path: file to store the gtest results.
            test_name: name of the gtest test case.
        Returns:
            List of strings
        '''
        if output_file_path:
            self.output_file_path = output_file_path
        if not test_name:
            test_name = self.full_name
        return [('{cmd} --gtest_filter={test} '
                 '--gtest_output=xml:{output_file_path}').format(
                     cmd=super(GtestTestCase, self).GetRunCommand(),
                     test=test_name,
                     output_file_path=self.output_file_path),
                'cat {output} && rm -rf {output}'.format(
                    output=self.output_file_path)]

    @property
    def output_file_path(self):
        """Get output_file_path"""
        if not hasattr(self,
                       '_output_file_path') or self._output_file_path is None:
            self.output_file_path = '{directory}/gtest_output_{name}.xml'.format(
                directory=path_utils.TargetDirName(self.path),
                name=re.sub(r'\W+', '_', str(self)))
        return self._output_file_path

    @output_file_path.setter
    def output_file_path(self, output_file_path):
        """Set output_file_path.

        Lengths of both file name and path will be checked. If longer than
        maximum allowance, file name will be set to a random name, and
        directory will be set to relative directory.

        Args:
            output_file_path: string, intended path of output xml file
        """
        output_file_path = path_utils.TargetNormPath(output_file_path.strip())
        output_base_name = path_utils.TargetBaseName(output_file_path)
        output_dir_name = path_utils.TargetDirName(output_file_path)

        if len(output_base_name) > utils.MAX_FILENAME_LEN:
            logging.warn('File name of output file "{}" is longer than {}.'.
                         format(output_file_path, utils.MAX_FILENAME_LEN))
            output_base_name = '{}.xml'.format(uuid.uuid4())
            output_file_path = path_utils.JoinTargetPath(
                output_dir_name, output_base_name)
            logging.info('Output file path is set as "%s".', output_file_path)

        if len(output_file_path) > utils.MAX_PATH_LEN:
            logging.warn('File path of output file "{}" is longer than {}.'.
                         format(output_file_path, utils.MAX_PATH_LEN))
            output_file_path = output_base_name
            logging.info('Output file path is set as "%s".', output_file_path)

        self._output_file_path = output_file_path
