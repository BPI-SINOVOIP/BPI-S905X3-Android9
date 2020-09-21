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


from cStringIO import StringIO
import sys


class CaptureStdout(list):
    '''Capture system stdout as a list of string.

    Usage example:
        with CaptureStdout() as output:
            print 'something'

        print 'Got output list: %s' % output
    '''

    def __enter__(self):
        self.sys_stdout = sys.stdout
        sys.stdout = StringIO()
        return self

    def __exit__(self, *args):
        self.extend(sys.stdout.getvalue().splitlines())
        sys.stdout = self.sys_stdout