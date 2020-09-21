#!/usr/bin/env python
#
# Copyright 2016 - The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import os
import argparse
import math


class Module(object):
    """class used to represent a ltp module

    Attribute:
       _lines: list of string, lines of module text
       _header: list of string, first line of module splited by :=
       _type: string, type of module
       _path: string, path of module
       _output_dir: string, output directory of module
    """
    _lines = None
    _header = None
    _type = None
    _path = None
    _output_dir = None

    def __init__(self, output_dir):
        self._output_dir = output_dir

    def parse(self, module_text):
        """parse a module text

        Args:
           module_text: string, one block of ltp module build rule.
           output_dir: string, ltp compile output directory

        Return:
           None if the input text is not a ltp module
           Self if parsed succesfully
        """
        self._lines = module_text.splitlines()
        if len(self._lines) < 2:
            self._type = None
            return None
        self._header = self._lines[0].split(' := ')
        if len(self._header) < 2:
            self._type = None
            return None
        self._type = self._header[0]
        self._path = self._header[1]
        return self

    def IsBuildSuccess(self, counts):
        """Check whether a given module specified in Android.mk file
           is succesfully built

           Returns:
               True if success
        """
        if self._type is None:
            return False

        counts[self._type] = counts.get(self._type, 0) + 1

        success = {"module_testname": self.IsBuildSuccessModuleTestname,
                   "module_libname": self.IsBuildSuccessModuleLibname,
                   "module_prebuilt": self.IsBuildSuccessModulePrebuilt,
                   }[self._type]()

        if not success:
            print "  Module build failed: " + os.path.basename(self._path)
        return success

    def IsBuildSuccessModuleTestname(self):
        """Check whether a given ltp test module in Android.mk file
           is succesfully built

           Args:
               module_path: string, the path of module on the first
                            line of the block

           Returns:
               True if success
        """

        return os.path.isfile(self._output_dir + \
                              "testcases/bin/" + \
                              os.path.basename(self._path))

    def IsBuildSuccessModuleLibname(self):
        """Check whether a given ltp lib module in Android.mk file
           is succesfully built

           Args:
               module_path: the path of module on the first line of
                            the block

           Returns:
               True if success
        """
        # TODO(yuexima) check lib build
        print "Checking module_lib is not supported now, " + \
            "assuming build success: " + self._path
        return True

    def IsBuildSuccessModulePrebuilt(self):
        """Check whether a given prebuilt module in Android.mk file
           is succesfully built

           Args:
               module_path: string, the path of module on the first
                            line of the block

           Returns:
               True if success
        """
        return os.path.isfile(self._output_dir + self._path)


class LtpModuleChecker(object):
    """LTP module result check class.
    Checks for success build of each module in LTP's Android.mk file
    and rewrite it with only successfully built modules.
    """
    _output_dir = ""
    _file_path_android_ltp_mk = ""
    _module_counts = {}

    def __init__(self, android_build_top, ltp_dir, target_product):
        self._output_dir = android_build_top + '/out/target/product/' + \
                          target_product + '/data/nativetest/ltp/'
        self._file_path_android_ltp_mk = ltp_dir + '/Android.ltp.mk'

    def Read(self, file_path):
        """Read a file and return its entire content

           Args:
               file_path: string, file path

           Returns:
               entire file content in string format
        """
        with open(file_path, 'r') as file:
            return file.read()

    def LoadModules(self):
        """Read the LTP Android.mk file and seperate modules into
           a list of string
        """
        return self.Read(self._file_path_android_ltp_mk).split("\n\n")

    def CheckModules(self):
        """Start the LTP module build result checking and counting."""
        modules = [Module(self._output_dir).parse(module)
                   for module in self.LoadModules()]
        modules_succeed = \
            [module for module in modules
             if module is not None and
             module.IsBuildSuccess(self._module_counts)
            ]

        print "module type counts:"
        print self._module_counts

        print str(len(modules_succeed)) + \
              " of " + str(sum([self._module_counts[i]
              for i in self._module_counts])) + \
              " modules were succesfully built."
        print "--Check complete."


def main():
    parser = argparse.ArgumentParser(
        description='Generate Android.mk from parsed LTP make output')
    parser.add_argument(
        '--android_build_top',
        dest='android_build_top',
        required=True,
        help='android build top directory')
    parser.add_argument(
        '--ltp_dir',
        dest='ltp_dir',
        required=True,
        help='directory for the forked ltp project')
    parser.add_argument(
        '--target_product',
        dest='target_product',
        required=True,
        help='target product name, \
                                such as "bullhead", "angler", etc.')
    args = parser.parse_args()

    checker = LtpModuleChecker(args.android_build_top, args.ltp_dir,
                               args.target_product)
    checker.CheckModules()


if __name__ == '__main__':
    main()
