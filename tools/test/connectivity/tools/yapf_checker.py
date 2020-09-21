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
import logging
import os
import sys

from acts.libs.proc import job

COMMIT_ID_ENV_KEY = 'PREUPLOAD_COMMIT'
REPO_PATH_KEY = 'REPO_PATH'
GIT_COMMAND = 'git diff-tree --no-commit-id --name-only -r %s'
YAPF_COMMAND = 'yapf -d -p %s'
YAPF_OLD_COMMAND = 'yapf -d %s'
YAPF_INPLACE_FORMAT = 'yapf -p -i %s'


def main():
    if COMMIT_ID_ENV_KEY not in os.environ:
        logging.error('Missing commit id in environment.')
        exit(1)

    if REPO_PATH_KEY not in os.environ:
        logging.error('Missing repo path in environment.')
        exit(1)

    commit_id = os.environ[COMMIT_ID_ENV_KEY]
    full_git_command = GIT_COMMAND % commit_id

    files = job.run(full_git_command).stdout.splitlines()
    full_files = [os.path.abspath(f) for f in files if f.endswith('.py')]
    if not full_files:
        return

    files_param_string = ' '.join(full_files)

    result = job.run(YAPF_COMMAND % files_param_string, ignore_status=True)
    yapf_inplace_format = YAPF_INPLACE_FORMAT

    if result.stdout:
        logging.error(result.stdout)
        logging.error('INVALID FORMATTING.')
        logging.error('Please run:\n'
                      '%s' % yapf_inplace_format % files_param_string)
        exit(1)


if __name__ == '__main__':
    main()
