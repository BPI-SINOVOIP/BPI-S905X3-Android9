#!/usr/bin/env python3.4
#
#   Copyright 2016 - The Android Open Source Project
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

import test_main
from acts.libs.proc import job

# Get the names of all files that have been modified from last upstream sync.
GIT_MODIFIED_FILES = 'git show --pretty="" --name-only @{u}..'


def main():
    files = job.run(GIT_MODIFIED_FILES, ignore_status=True).stdout
    for file in files.split():
        if file.startswith('tools/lab/'):
            test_main.main()


if __name__ == '__main__':
    main()
