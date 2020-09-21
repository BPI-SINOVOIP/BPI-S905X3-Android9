#!/usr/bin/env python3.4
#
#   Copyright 2017 - The Android Open Source Project
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
import re
import shellescape

from acts.libs.proc import job

GLOBAL_KEYWORDS_FILEPATH = 'vendor/google_testing/comms/framework/etc/' \
                           'commit_keywords'
LOCAL_KEYWORDS_FILEPATH = '~/.repo_acts_commit_keywords'

GIT_FILE_ADDITIONS = r'git diff --unified=0 %s^! | grep "^+" | ' \
                     r'grep -Ev "^(\+\+\+ b/)" | cut -c 2-'

GIT_FILE_NAMES = r'git diff-tree --no-commit-id --name-only -r %s'

GIT_DIFF_REGION_FOUND = 'git diff %s^! | grep -A10 -B10 %s'
COMMIT_ID_ENV_KEY = 'PREUPLOAD_COMMIT'

FIND_COMMIT_KEYWORDS = 'git diff %s^! | grep -i %s'
GET_EMAIL_ADDRESS = 'git log --format=%ce -1'


def find_in_commit_message(keyword_list):
    """Looks for keywords in commit messages.

    Args:
        keyword_list: A list of keywords
    """
    grep_args = ''
    for keyword in keyword_list:
        grep_args += '-e %s' % keyword

    result = job.run(
        FIND_COMMIT_KEYWORDS % (os.environ[COMMIT_ID_ENV_KEY], grep_args),
        ignore_status=True)

    if result.stdout:
        logging.error('Your commit message contains at least one keyword.')
        logging.error('Keyword(s) found in the following line(s):')
        logging.error(result.stdout)
        logging.error('Please fix/remove these before committing.')
        exit(1)


def get_words(string):
    """Splits a string into an array of alphabetical words."""
    s1 = re.sub('(.)([A-Z][a-z]+)', r'\1 \2', string)
    s1 = re.sub('([a-z0-9])([A-Z])', r'\1 \2', s1).lower()
    s1 = re.sub('[^a-z ]', ' ', s1)
    return s1.split()


def find_in_file_names(keyword_list):
    """Looks for keywords in file names.

    Args:
        keyword_list: a list of keywords
    """
    changed_files = job.run(GIT_FILE_NAMES % os.environ[COMMIT_ID_ENV_KEY])

    keyword_set = set(keyword_list)

    for file_name in changed_files.stdout.split('\n'):
        words = set(get_words(file_name))
        if len(keyword_set.intersection(words)) > 0:
            logging.error('Your commit has a file name that contain at least '
                          'one keyword: %s.' % file_name)
            logging.error('Please update or remove this before committing.')
            exit(1)


def find_in_code_additions(keyword_list):
    """Looks for keywords in code additions.

    Args:
        keyword_list: a list of keywords
    """
    all_additions = job.run(GIT_FILE_ADDITIONS % os.environ[COMMIT_ID_ENV_KEY])

    keyword_set = set(keyword_list)

    for line in all_additions.stdout.split('\n'):
        words = set(get_words(line))
        if len(keyword_set.intersection(words)) > 0:
            result = job.run(GIT_DIFF_REGION_FOUND %
                             (os.environ[COMMIT_ID_ENV_KEY],
                              shellescape.quote(line)))
            logging.error('Your commit has code changes that contain at least '
                          'one keyword. Below is an excerpt from the commit '
                          'diff:')
            logging.error(result.stdout)
            logging.error('Please update or remove these before committing.')
            exit(1)


def main():
    if COMMIT_ID_ENV_KEY not in os.environ:
        logging.error('Missing commit id in environment.')
        exit(1)
    keyword_file = os.path.join(
        os.path.dirname(__file__), '../../../../%s' % GLOBAL_KEYWORDS_FILEPATH)

    if not os.path.isfile(keyword_file):
        keyword_file = os.path.expanduser(LOCAL_KEYWORDS_FILEPATH)
        if not os.path.exists(keyword_file) or not os.path.isfile(keyword_file):
            result = job.run(GET_EMAIL_ADDRESS)
            if result.stdout.endswith('@google.com'):
                logging.error(
                    'You do not have the necessary file %s. Please run '
                    'tools/ignore_commit_keywords.sh, or link it with the '
                    'following command:\n ln -sf <internal_master_root>/%s %s'
                    % (LOCAL_KEYWORDS_FILEPATH, GLOBAL_KEYWORDS_FILEPATH,
                       LOCAL_KEYWORDS_FILEPATH))
                exit(1)
            return

    with open(keyword_file) as file:
        keyword_list = file.read().lower().split()

    find_in_code_additions(keyword_list)
    find_in_commit_message(keyword_list)
    find_in_file_names(keyword_list)


if __name__ == '__main__':
    main()
