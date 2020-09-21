#!/usr/bin/env python2
#
# Copyright 2010 Google Inc. All Rights Reserved.
"""Module for transferring files between various types of repositories."""

from __future__ import print_function

__author__ = 'asharif@google.com (Ahmad Sharif)'

import argparse
import datetime
import json
import os
import re
import socket
import sys
import tempfile

from automation.clients.helper import perforce
from cros_utils import command_executer
from cros_utils import logger
from cros_utils import misc

# pylint: disable=anomalous-backslash-in-string


def GetCanonicalMappings(mappings):
  canonical_mappings = []
  for mapping in mappings:
    remote_path, local_path = mapping.split()
    if local_path.endswith('/') and not remote_path.endswith('/'):
      local_path = os.path.join(local_path, os.path.basename(remote_path))
    remote_path = remote_path.lstrip('/').split('/', 1)[1]
    canonical_mappings.append(perforce.PathMapping(remote_path, local_path))
  return canonical_mappings


def SplitMapping(mapping):
  parts = mapping.split()
  assert len(parts) <= 2, 'Mapping %s invalid' % mapping
  remote_path = parts[0]
  if len(parts) == 2:
    local_path = parts[1]
  else:
    local_path = '.'
  return remote_path, local_path


class Repo(object):
  """Basic repository base class."""

  def __init__(self, no_create_tmp_dir=False):
    self.repo_type = None
    self.address = None
    self.mappings = None
    self.revision = None
    self.ignores = ['.gitignore', '.p4config', 'README.google']
    if no_create_tmp_dir:
      self._root_dir = None
    else:
      self._root_dir = tempfile.mkdtemp()
    self._ce = command_executer.GetCommandExecuter()
    self._logger = logger.GetLogger()

  def PullSources(self):
    """Pull all sources into an internal dir."""
    pass

  def SetupForPush(self):
    """Setup a repository for pushing later."""
    pass

  def PushSources(self, commit_message=None, dry_run=False, message_file=None):
    """Push to the external repo with the commit message."""
    pass

  def _RsyncExcludingRepoDirs(self, source_dir, dest_dir):
    for f in os.listdir(source_dir):
      if f in ['.git', '.svn', '.p4config']:
        continue
      dest_file = os.path.join(dest_dir, f)
      source_file = os.path.join(source_dir, f)
      if os.path.exists(dest_file):
        command = 'rm -rf %s' % dest_file
        self._ce.RunCommand(command)
      command = 'rsync -a %s %s' % (source_file, dest_dir)
      self._ce.RunCommand(command)
    return 0

  def MapSources(self, dest_dir):
    """Copy sources from the internal dir to root_dir."""
    return self._RsyncExcludingRepoDirs(self._root_dir, dest_dir)

  def GetRoot(self):
    return self._root_dir

  def SetRoot(self, directory):
    self._root_dir = directory

  def CleanupRoot(self):
    command = 'rm -rf %s' % self._root_dir
    return self._ce.RunCommand(command)

  def __str__(self):
    return '\n'.join(
        str(s) for s in [self.repo_type, self.address, self.mappings])


# Note - this type of repo is used only for "readonly", in other words, this
# only serves as a incoming repo.
class FileRepo(Repo):
  """Class for file repositories."""

  def __init__(self, address, ignores=None):
    Repo.__init__(self, no_create_tmp_dir=True)
    self.repo_type = 'file'
    self.address = address
    self.mappings = None
    self.branch = None
    self.revision = '{0} (as of "{1}")'.format(address, datetime.datetime.now())
    self.gerrit = None
    self._root_dir = self.address
    if ignores:
      self.ignores += ignores

  def CleanupRoot(self):
    """Override to prevent deletion."""
    pass


class P4Repo(Repo):
  """Class for P4 repositories."""

  def __init__(self, address, mappings, revision=None):
    Repo.__init__(self)
    self.repo_type = 'p4'
    self.address = address
    self.mappings = mappings
    self.revision = revision

  def PullSources(self):
    client_name = socket.gethostname()
    client_name += tempfile.mkstemp()[1].replace('/', '-')
    mappings = self.mappings
    p4view = perforce.View('depot2', GetCanonicalMappings(mappings))
    p4client = perforce.CommandsFactory(
        self._root_dir, p4view, name=client_name)
    command = p4client.SetupAndDo(p4client.Sync(self.revision))
    ret = self._ce.RunCommand(command)
    assert ret == 0, 'Could not setup client.'
    command = p4client.InCheckoutDir(p4client.SaveCurrentCLNumber())
    ret, o, _ = self._ce.RunCommandWOutput(command)
    assert ret == 0, 'Could not get version from client.'
    self.revision = re.search('^\d+$', o.strip(), re.MULTILINE).group(0)
    command = p4client.InCheckoutDir(p4client.Remove())
    ret = self._ce.RunCommand(command)
    assert ret == 0, 'Could not delete client.'
    return 0


class SvnRepo(Repo):
  """Class for svn repositories."""

  def __init__(self, address, mappings):
    Repo.__init__(self)
    self.repo_type = 'svn'
    self.address = address
    self.mappings = mappings

  def PullSources(self):
    with misc.WorkingDirectory(self._root_dir):
      for mapping in self.mappings:
        remote_path, local_path = SplitMapping(mapping)
        command = 'svn co %s/%s %s' % (self.address, remote_path, local_path)
      ret = self._ce.RunCommand(command)
      if ret:
        return ret

      self.revision = ''
      for mapping in self.mappings:
        remote_path, local_path = SplitMapping(mapping)
        command = 'cd %s && svnversion -c .' % (local_path)
        ret, o, _ = self._ce.RunCommandWOutput(command)
        self.revision += o.strip().split(':')[-1]
        if ret:
          return ret
    return 0


class GitRepo(Repo):
  """Class for git repositories."""

  def __init__(self, address, branch, mappings=None, ignores=None, gerrit=None):
    Repo.__init__(self)
    self.repo_type = 'git'
    self.address = address
    self.branch = branch or 'master'
    if ignores:
      self.ignores += ignores
    self.mappings = mappings
    self.gerrit = gerrit

  def _CloneSources(self):
    with misc.WorkingDirectory(self._root_dir):
      command = 'git clone %s .' % (self.address)
      return self._ce.RunCommand(command)

  def PullSources(self):
    with misc.WorkingDirectory(self._root_dir):
      ret = self._CloneSources()
      if ret:
        return ret

      command = 'git checkout %s' % self.branch
      ret = self._ce.RunCommand(command)
      if ret:
        return ret

      command = 'git describe --always'
      ret, o, _ = self._ce.RunCommandWOutput(command)
      self.revision = o.strip()
      return ret

  def SetupForPush(self):
    with misc.WorkingDirectory(self._root_dir):
      ret = self._CloneSources()
      logger.GetLogger().LogFatalIf(
          ret, 'Could not clone git repo %s.' % self.address)

      command = 'git branch -a | grep -wq %s' % self.branch
      ret = self._ce.RunCommand(command)

      if ret == 0:
        if self.branch != 'master':
          command = ('git branch --track %s remotes/origin/%s' % (self.branch,
                                                                  self.branch))
        else:
          command = 'pwd'
        command += '&& git checkout %s' % self.branch
      else:
        command = 'git symbolic-ref HEAD refs/heads/%s' % self.branch
      command += '&& rm -rf *'
      ret = self._ce.RunCommand(command)
      return ret

  def CommitLocally(self, commit_message=None, message_file=None):
    with misc.WorkingDirectory(self._root_dir):
      command = 'pwd'
      for ignore in self.ignores:
        command += '&& echo \'%s\' >> .git/info/exclude' % ignore
      command += '&& git add -Av .'
      if message_file:
        message_arg = '-F %s' % message_file
      elif commit_message:
        message_arg = '-m \'%s\'' % commit_message
      else:
        raise RuntimeError('No commit message given!')
      command += '&& git commit -v %s' % message_arg
      return self._ce.RunCommand(command)

  def PushSources(self, commit_message=None, dry_run=False, message_file=None):
    ret = self.CommitLocally(commit_message, message_file)
    if ret:
      return ret
    push_args = ''
    if dry_run:
      push_args += ' -n '
    with misc.WorkingDirectory(self._root_dir):
      if self.gerrit:
        label = 'somelabel'
        command = 'git remote add %s %s' % (label, self.address)
        command += ('&& git push %s %s HEAD:refs/for/master' % (push_args,
                                                                label))
      else:
        command = 'git push -v %s origin %s:%s' % (push_args, self.branch,
                                                   self.branch)
      ret = self._ce.RunCommand(command)
    return ret

  def MapSources(self, root_dir):
    if not self.mappings:
      self._RsyncExcludingRepoDirs(self._root_dir, root_dir)
      return
    with misc.WorkingDirectory(self._root_dir):
      for mapping in self.mappings:
        remote_path, local_path = SplitMapping(mapping)
        remote_path.rstrip('...')
        local_path.rstrip('...')
        full_local_path = os.path.join(root_dir, local_path)
        ret = self._RsyncExcludingRepoDirs(remote_path, full_local_path)
        if ret:
          return ret
    return 0


class RepoReader(object):
  """Class for reading repositories."""

  def __init__(self, filename):
    self.filename = filename
    self.main_dict = {}
    self.input_repos = []
    self.output_repos = []

  def ParseFile(self):
    with open(self.filename) as f:
      self.main_dict = json.load(f)
      self.CreateReposFromDict(self.main_dict)
    return [self.input_repos, self.output_repos]

  def CreateReposFromDict(self, main_dict):
    for key, repo_list in main_dict.items():
      for repo_dict in repo_list:
        repo = self.CreateRepoFromDict(repo_dict)
        if key == 'input':
          self.input_repos.append(repo)
        elif key == 'output':
          self.output_repos.append(repo)
        else:
          logger.GetLogger().LogFatal('Unknown key: %s found' % key)

  def CreateRepoFromDict(self, repo_dict):
    repo_type = repo_dict.get('type', None)
    repo_address = repo_dict.get('address', None)
    repo_mappings = repo_dict.get('mappings', None)
    repo_ignores = repo_dict.get('ignores', None)
    repo_branch = repo_dict.get('branch', None)
    gerrit = repo_dict.get('gerrit', None)
    revision = repo_dict.get('revision', None)

    if repo_type == 'p4':
      repo = P4Repo(repo_address, repo_mappings, revision=revision)
    elif repo_type == 'svn':
      repo = SvnRepo(repo_address, repo_mappings)
    elif repo_type == 'git':
      repo = GitRepo(
          repo_address,
          repo_branch,
          mappings=repo_mappings,
          ignores=repo_ignores,
          gerrit=gerrit)
    elif repo_type == 'file':
      repo = FileRepo(repo_address)
    else:
      logger.GetLogger().LogFatal('Unknown repo type: %s' % repo_type)
    return repo


@logger.HandleUncaughtExceptions
def Main(argv):
  parser = argparse.ArgumentParser()
  parser.add_argument(
      '-i',
      '--input_file',
      dest='input_file',
      help='The input file that contains repo descriptions.')

  parser.add_argument(
      '-n',
      '--dry_run',
      dest='dry_run',
      action='store_true',
      default=False,
      help='Do a dry run of the push.')

  parser.add_argument(
      '-F',
      '--message_file',
      dest='message_file',
      default=None,
      help=('Use contents of the log file as the commit '
            'message.'))

  options = parser.parse_args(argv)
  if not options.input_file:
    parser.print_help()
    return 1
  rr = RepoReader(options.input_file)
  [input_repos, output_repos] = rr.ParseFile()

  # Make sure FileRepo is not used as output destination.
  for output_repo in output_repos:
    if output_repo.repo_type == 'file':
      logger.GetLogger().LogFatal(
          'FileRepo is only supported as an input repo.')

  for output_repo in output_repos:
    ret = output_repo.SetupForPush()
    if ret:
      return ret

  input_revisions = []
  for input_repo in input_repos:
    ret = input_repo.PullSources()
    if ret:
      return ret
    input_revisions.append(input_repo.revision)

  for input_repo in input_repos:
    for output_repo in output_repos:
      ret = input_repo.MapSources(output_repo.GetRoot())
      if ret:
        return ret

  commit_message = 'Synced repos to: %s' % ','.join(input_revisions)
  for output_repo in output_repos:
    ret = output_repo.PushSources(
        commit_message=commit_message,
        dry_run=options.dry_run,
        message_file=options.message_file)
    if ret:
      return ret

  if not options.dry_run:
    for output_repo in output_repos:
      output_repo.CleanupRoot()
    for input_repo in input_repos:
      input_repo.CleanupRoot()

  return ret


if __name__ == '__main__':
  retval = Main(sys.argv[1:])
  sys.exit(retval)
