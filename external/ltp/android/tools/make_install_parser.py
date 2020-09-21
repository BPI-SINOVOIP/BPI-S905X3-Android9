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
import fileinput
import os
import os.path
import re
import pprint

# Parses the output of make install --dry-run and generates directives in the
# form
#
#   install['target'] = [ 'srcfile' ]
#
# This output is then fed into gen_android_mk which generates Android.mk.
#
# This process is split into two steps so the second step can later be replaced
# with an Android.bp generator.


class MakeInstallParser(object):
    '''Parses the output of make install --dry-run.'''

    def __init__(self, ltp_root):
        self.ltp_root = ltp_root

    def ParseFile(self, input_path):
        '''Parses the text output of make install --dry-run.

        Args:
            input_text: string, output of make install --dry-run

        Returns:
            string, directives in form install['target'] = [ 'srcfile' ]
        '''
        pattern = re.compile(r'install -m \d+ "%s%s(.*)" "/opt/ltp/(.*)"' %
                             (os.path.realpath(self.ltp_root), os.sep))
        result = []

        with open(input_path, 'r') as f:
            for line in f:
                line = line.strip()

                m = pattern.match(line)
                if not m:
                    continue

                src, target = m.groups()
                # If the file isn't in the source tree, it's not a prebuilt
                if not os.path.isfile(
                        os.path.realpath(self.ltp_root) + os.sep + src):
                    continue

                result.append("install['%s'] = ['%s']" % (target, src))

        return result

def main():
    arg_parser = argparse.ArgumentParser(
        description='Parse the LTP make install --dry-run output into a list')
    arg_parser.add_argument(
        '--ltp-root',
        dest='ltp_root',
        required=True,
        help='LTP Root dir')
    arg_parser.add_argument(
        '--dry-run-file',
        dest='input_path',
        required=True,
        help='Path to LTP make install --dry-run output file')
    args = arg_parser.parse_args()

    make_install_parser = MakeInstallParser(args.ltp_root)
    result = make_install_parser.ParseFile(args.input_path)

    print pprint.pprint(result)

if __name__ == '__main__':
    main()