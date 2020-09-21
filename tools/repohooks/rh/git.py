# -*- coding:utf-8 -*-
# Copyright 2016 The Android Open Source Project
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

"""Git helper functions."""

from __future__ import print_function

import os
import re
import sys

_path = os.path.realpath(__file__ + '/../..')
if sys.path[0] != _path:
    sys.path.insert(0, _path)
del _path

# pylint: disable=wrong-import-position
import rh.utils


def get_upstream_remote():
    """Returns the current upstream remote name."""
    # First get the current branch name.
    cmd = ['git', 'rev-parse', '--abbrev-ref', 'HEAD']
    result = rh.utils.run_command(cmd, capture_output=True)
    branch = result.output.strip()

    # Then get the remote associated with this branch.
    cmd = ['git', 'config', 'branch.%s.remote' % branch]
    result = rh.utils.run_command(cmd, capture_output=True)
    return result.output.strip()


def get_upstream_branch():
    """Returns the upstream tracking branch of the current branch.

    Raises:
      Error if there is no tracking branch
    """
    cmd = ['git', 'symbolic-ref', 'HEAD']
    result = rh.utils.run_command(cmd, capture_output=True)
    current_branch = result.output.strip().replace('refs/heads/', '')
    if not current_branch:
        raise ValueError('Need to be on a tracking branch')

    cfg_option = 'branch.' + current_branch + '.%s'
    cmd = ['git', 'config', cfg_option % 'merge']
    result = rh.utils.run_command(cmd, capture_output=True)
    full_upstream = result.output.strip()
    # If remote is not fully qualified, add an implicit namespace.
    if '/' not in full_upstream:
        full_upstream = 'refs/heads/%s' % full_upstream
    cmd = ['git', 'config', cfg_option % 'remote']
    result = rh.utils.run_command(cmd, capture_output=True)
    remote = result.output.strip()
    if not remote or not full_upstream:
        raise ValueError('Need to be on a tracking branch')

    return full_upstream.replace('heads', 'remotes/' + remote)


def get_commit_for_ref(ref):
    """Returns the latest commit for this ref."""
    cmd = ['git', 'rev-parse', ref]
    result = rh.utils.run_command(cmd, capture_output=True)
    return result.output.strip()


def get_remote_revision(ref, remote):
    """Returns the remote revision for this ref."""
    prefix = 'refs/remotes/%s/' % remote
    if ref.startswith(prefix):
        return ref[len(prefix):]
    return ref


def get_patch(commit):
    """Returns the patch for this commit."""
    cmd = ['git', 'format-patch', '--stdout', '-1', commit]
    return rh.utils.run_command(cmd, capture_output=True).output


def _try_utf8_decode(data):
    """Attempts to decode a string as UTF-8.

    Returns:
      The decoded Unicode object, or the original string if parsing fails.
    """
    try:
        return unicode(data, 'utf-8', 'strict')
    except UnicodeDecodeError:
        return data


def get_file_content(commit, path):
    """Returns the content of a file at a specific commit.

    We can't rely on the file as it exists in the filesystem as people might be
    uploading a series of changes which modifies the file multiple times.

    Note: The "content" of a symlink is just the target.  So if you're expecting
    a full file, you should check that first.  One way to detect is that the
    content will not have any newlines.
    """
    cmd = ['git', 'show', '%s:%s' % (commit, path)]
    return rh.utils.run_command(cmd, capture_output=True).output


# RawDiffEntry represents a line of raw formatted git diff output.
RawDiffEntry = rh.utils.collection(
    'RawDiffEntry',
    src_mode=0, dst_mode=0, src_sha=None, dst_sha=None,
    status=None, score=None, src_file=None, dst_file=None, file=None)


# This regular expression pulls apart a line of raw formatted git diff output.
DIFF_RE = re.compile(
    r':(?P<src_mode>[0-7]*) (?P<dst_mode>[0-7]*) '
    r'(?P<src_sha>[0-9a-f]*)(\.)* (?P<dst_sha>[0-9a-f]*)(\.)* '
    r'(?P<status>[ACDMRTUX])(?P<score>[0-9]+)?\t'
    r'(?P<src_file>[^\t]+)\t?(?P<dst_file>[^\t]+)?')


def raw_diff(path, target):
    """Return the parsed raw format diff of target

    Args:
      path: Path to the git repository to diff in.
      target: The target to diff.

    Returns:
      A list of RawDiffEntry's.
    """
    entries = []

    cmd = ['git', 'diff', '--no-ext-diff', '-M', '--raw', target]
    diff = rh.utils.run_command(cmd, cwd=path, capture_output=True).output
    diff_lines = diff.strip().splitlines()
    for line in diff_lines:
        match = DIFF_RE.match(line)
        if not match:
            raise ValueError('Failed to parse diff output: %s' % line)
        diff = RawDiffEntry(**match.groupdict())
        diff.src_mode = int(diff.src_mode)
        diff.dst_mode = int(diff.dst_mode)
        diff.file = diff.dst_file if diff.dst_file else diff.src_file
        entries.append(diff)

    return entries


def get_affected_files(commit):
    """Returns list of file paths that were modified/added.

    Returns:
      A list of modified/added (and perhaps deleted) files
    """
    return raw_diff(os.getcwd(), '%s^!' % commit)


def get_commits(ignore_merged_commits=False):
    """Returns a list of commits for this review."""
    cmd = ['git', 'log', '%s..' % get_upstream_branch(), '--format=%H']
    if ignore_merged_commits:
        cmd.append('--first-parent')
    return rh.utils.run_command(cmd, capture_output=True).output.split()


def get_commit_desc(commit):
    """Returns the full commit message of a commit."""
    cmd = ['git', 'log', '--format=%B', commit + '^!']
    return rh.utils.run_command(cmd, capture_output=True).output


def find_repo_root(path=None):
    """Locate the top level of this repo checkout starting at |path|."""
    if path is None:
        path = os.getcwd()
    orig_path = path

    path = os.path.abspath(path)
    while not os.path.exists(os.path.join(path, '.repo')):
        path = os.path.dirname(path)
        if path == '/':
            raise ValueError('Could not locate .repo in %s' % orig_path)

    return path
