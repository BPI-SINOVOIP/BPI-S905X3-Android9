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

import argparse
import os
import os.path
import sys
import re
import fileinput
import pprint

AR = 'ar'
CC = 'gcc'

class MakeParser(object):
    '''Parses the output of make --dry-run.

    Attributes:
        ltp_root: string, LTP root directory
        ar_parser: archive (ar) command argument parser
        cc_parser: gcc command argument parser
        result: list of string, result string buffer
        dir_stack: list of string, directory stack for parsing make commands
    '''

    def __init__(self, ltp_root):
        self.ltp_root = ltp_root
        ar_parser = argparse.ArgumentParser()
        ar_parser.add_argument('-r', dest='r', action='store_true')
        ar_parser.add_argument('-c', dest='c', action='store')
        self.ar_parser = ar_parser

        cc_parser = argparse.ArgumentParser()
        cc_parser.add_argument('-D', dest='defines', action='append')
        cc_parser.add_argument('-I', dest='includes', action='append')
        cc_parser.add_argument('-l', dest='libraries', action='append')
        cc_parser.add_argument('-c', dest='compile', action='store_true')
        cc_parser.add_argument('-o', dest='target', action='store')
        self.cc_parser = cc_parser

        self.result = []
        self.dir_stack = []

    def GetRelativePath(self, path):
        '''Get relative path toward LTP directory.

        Args:
            path: string, a path to convert to relative path
        '''
        if path[0] == '/':
            path = os.path.realpath(path)
        else:
            path = os.path.realpath(self.ltp_root + os.sep + self.dir_stack[-1]
                                    + os.sep + path)
        return os.path.realpath(path).replace(self.ltp_root + os.sep, '')

    def GetRelativePathForExtensions(self, paths, extensions):
        '''Get relative path toward LTP directory of paths with given extension.

        Args:
            paths: list of string, paths to convert to relative path
            extensions: list of string, extension include filter
        '''
        return [self.GetRelativePath(i) for i in paths if i[-1] in extensions]

    def ParseAr(self, line):
        '''Parse a archive command line.

        Args:
            line: string, a line of ar command to parse
        '''
        args, unparsed = self.ar_parser.parse_known_args(line.split()[1:])

        sources = self.GetRelativePathForExtensions(unparsed, ['o'])

        # Support 'ar' command line with or without hyphens (-)
        #  e.g.:
        #    1. ar rcs libfoo.a foo1.o foo2.o
        #    2. ar -rc "libfoo.a" foo1.o foo2.o; ranlib "libfoo.a"
        target = None
        if not args.c and not args.r:
            for path in unparsed:
                if path.endswith('.a'):
                    target = self.GetRelativePath(path)
                    break
        else:
            target = self.GetRelativePath(args.c.replace('"', ""))

        assert len(sources) > 0
        assert target != None

        self.result.append("ar['%s'] = %s" % (target, sources))

    def ParseCc(self, line):
        '''Parse a gcc command line.

        Args:
            line: string, a line of gcc command to parse
        '''
        args, unparsed = self.cc_parser.parse_known_args(line.split()[1:])

        sources = self.GetRelativePathForExtensions(unparsed, ['c', 'o'])
        includes = [self.GetRelativePath(i)
                    for i in args.includes] if args.includes else []
        flags = []
        defines = args.defines if args.defines else []
        target = self.GetRelativePath(args.target)

        if args.defines:
            for define in args.defines:
                flags.append('-D%s' % define)

        flags.extend(i for i in unparsed if i.startswith('-Wno'))

        assert len(sources) > 0

        if args.compile:
            self.result.append("cc_compile['%s'] = %s" % (target, sources))
        else:
            libraries = args.libraries if args.libraries else []
            if sources[0].endswith('.o'):
                self.result.append("cc_link['%s'] = %s" % (target, sources))
            else:
                self.result.append("cc_compilelink['%s'] = %s" %
                                   (target, sources))
            self.result.append("cc_libraries['%s'] = %s" % (target, libraries))

        self.result.append("cc_flags['%s'] = %s" % (target, flags))
        self.result.append("cc_includes['%s'] = %s" % (target, includes))

    def ParseFile(self, input_path):
        '''Parses the output of make --dry-run.

        Args:
            input_text: string, output of make --dry-run

        Returns:
            string, generated directives in the form
                    ar['target.a'] = [ 'srcfile1.o, 'srcfile2.o', ... ]
                    cc_link['target'] = [ 'srcfile1.o', 'srcfile2.o', ... ]
                    cc_compile['target.o'] = [ 'srcfile1.c' ]
                    cc_compilelink['target'] = [ 'srcfile1.c' ]
                along with optional flags for the above directives in the form
                    cc_flags['target'] = [ '-flag1', '-flag2', ...]
                    cc_includes['target'] = [ 'includepath1', 'includepath2', ...]
                    cc_libraries['target'] = [ 'library1', 'library2', ...]
        '''
        self.result = []
        self.dir_stack = []

        entering_directory = re.compile(r"make.*: Entering directory [`,'](.*)'")
        leaving_directory = re.compile(r"make.*: Leaving directory [`,'](.*)'")

        with open(input_path, 'r') as f:
            for line in f:
                line = line.strip()

                m = entering_directory.match(line)
                if m:
                    self.dir_stack.append(self.GetRelativePath(m.group(1)))
                    continue

                m = leaving_directory.match(line)
                if m:
                    self.dir_stack.pop()
                elif line.startswith(AR):
                    self.ParseAr(line)
                elif line.startswith(CC):
                    self.ParseCc(line)

        return self.result

def main():
    arg_parser = argparse.ArgumentParser(
        description='Parse the LTP make --dry-run output into a list')
    arg_parser.add_argument(
        '--ltp-root',
        dest='ltp_root',
        required=True,
        help='LTP Root dir')
    arg_parser.add_argument(
        '--dry-run-file',
        dest='input_path',
        required=True,
        help='Path to LTP make --dry-run output file')
    args = arg_parser.parse_args()

    make_parser = MakeParser(args.ltp_root)
    result = make_parser.ParseFile(args.input_path)

    print pprint.pprint(result)

if __name__ == '__main__':
    main()
