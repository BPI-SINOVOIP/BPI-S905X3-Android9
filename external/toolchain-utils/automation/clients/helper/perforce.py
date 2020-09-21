# Copyright 2011 Google Inc. All Rights Reserved.

__author__ = 'kbaclawski@google.com (Krystian Baclawski)'

import collections
import os.path

from automation.common import command as cmd


class PathMapping(object):
  """Stores information about relative path mapping (remote to local)."""

  @classmethod
  def ListFromPathDict(cls, prefix_path_dict):
    """Takes {'prefix1': ['path1',...], ...} and returns a list of mappings."""

    mappings = []

    for prefix, paths in sorted(prefix_path_dict.items()):
      for path in sorted(paths):
        mappings.append(cls(os.path.join(prefix, path)))

    return mappings

  @classmethod
  def ListFromPathTuples(cls, tuple_list):
    """Takes a list of tuples and returns a list of mappings.

    Args:
      tuple_list: [('remote_path1', 'local_path1'), ...]

    Returns:
      a list of mapping objects
    """
    mappings = []
    for remote_path, local_path in tuple_list:
      mappings.append(cls(remote_path, local_path))

    return mappings

  def __init__(self, remote, local=None, common_suffix=None):
    suffix = self._FixPath(common_suffix or '')

    self.remote = os.path.join(remote, suffix)
    self.local = os.path.join(local or remote, suffix)

  @staticmethod
  def _FixPath(path_s):
    parts = [part for part in path_s.strip('/').split('/') if part]

    if not parts:
      return ''

    return os.path.join(*parts)

  def _GetRemote(self):
    return self._remote

  def _SetRemote(self, path_s):
    self._remote = self._FixPath(path_s)

  remote = property(_GetRemote, _SetRemote)

  def _GetLocal(self):
    return self._local

  def _SetLocal(self, path_s):
    self._local = self._FixPath(path_s)

  local = property(_GetLocal, _SetLocal)

  def GetAbsolute(self, depot, client):
    return (os.path.join('//', depot, self.remote),
            os.path.join('//', client, self.local))

  def __str__(self):
    return '%s(%s => %s)' % (self.__class__.__name__, self.remote, self.local)


class View(collections.MutableSet):
  """Keeps all information about local client required to work with perforce."""

  def __init__(self, depot, mappings=None, client=None):
    self.depot = depot

    if client:
      self.client = client

    self._mappings = set(mappings or [])

  @staticmethod
  def _FixRoot(root_s):
    parts = root_s.strip('/').split('/', 1)

    if len(parts) != 1:
      return None

    return parts[0]

  def _GetDepot(self):
    return self._depot

  def _SetDepot(self, depot_s):
    depot = self._FixRoot(depot_s)
    assert depot, 'Not a valid depot name: "%s".' % depot_s
    self._depot = depot

  depot = property(_GetDepot, _SetDepot)

  def _GetClient(self):
    return self._client

  def _SetClient(self, client_s):
    client = self._FixRoot(client_s)
    assert client, 'Not a valid client name: "%s".' % client_s
    self._client = client

  client = property(_GetClient, _SetClient)

  def add(self, mapping):
    assert type(mapping) is PathMapping
    self._mappings.add(mapping)

  def discard(self, mapping):
    assert type(mapping) is PathMapping
    self._mappings.discard(mapping)

  def __contains__(self, value):
    return value in self._mappings

  def __len__(self):
    return len(self._mappings)

  def __iter__(self):
    return iter(mapping for mapping in self._mappings)

  def AbsoluteMappings(self):
    return iter(mapping.GetAbsolute(self.depot, self.client)
                for mapping in self._mappings)


class CommandsFactory(object):
  """Creates shell commands used for interaction with Perforce."""

  def __init__(self, checkout_dir, p4view, name=None, port=None):
    self.port = port or 'perforce2:2666'
    self.view = p4view
    self.view.client = name or 'p4-automation-$HOSTNAME-$JOB_ID'
    self.checkout_dir = checkout_dir
    self.p4config_path = os.path.join(self.checkout_dir, '.p4config')

  def Initialize(self):
    return cmd.Chain('mkdir -p %s' % self.checkout_dir, 'cp ~/.p4config %s' %
                     self.checkout_dir, 'chmod u+w %s' % self.p4config_path,
                     'echo "P4PORT=%s" >> %s' % (self.port, self.p4config_path),
                     'echo "P4CLIENT=%s" >> %s' %
                     (self.view.client, self.p4config_path))

  def Create(self):
    # TODO(kbaclawski): Could we support value list for options consistently?
    mappings = ['-a \"%s %s\"' % mapping
                for mapping in self.view.AbsoluteMappings()]

    # First command will create client with default mappings.  Second one will
    # replace default mapping with desired.  Unfortunately, it seems that it
    # cannot be done in one step.  P4EDITOR is defined to /bin/true because we
    # don't want "g4 client" to enter real editor and wait for user actions.
    return cmd.Wrapper(
        cmd.Chain(
            cmd.Shell('g4', 'client'),
            cmd.Shell('g4', 'client', '--replace', *mappings)),
        env={'P4EDITOR': '/bin/true'})

  def SaveSpecification(self, filename=None):
    return cmd.Pipe(cmd.Shell('g4', 'client', '-o'), output=filename)

  def Sync(self, revision=None):
    sync_arg = '...'
    if revision:
      sync_arg = '%s@%s' % (sync_arg, revision)
    return cmd.Shell('g4', 'sync', sync_arg)

  def SaveCurrentCLNumber(self, filename=None):
    return cmd.Pipe(
        cmd.Shell('g4', 'changes', '-m1', '...#have'),
        cmd.Shell('sed', '-E', '"s,Change ([0-9]+) .*,\\1,"'),
        output=filename)

  def Remove(self):
    return cmd.Shell('g4', 'client', '-d', self.view.client)

  def SetupAndDo(self, *commands):
    return cmd.Chain(self.Initialize(),
                     self.InCheckoutDir(self.Create(), *commands))

  def InCheckoutDir(self, *commands):
    return cmd.Wrapper(cmd.Chain(*commands), cwd=self.checkout_dir)

  def CheckoutFromSnapshot(self, snapshot):
    cmds = cmd.Chain()

    for mapping in self.view:
      local_path, file_part = mapping.local.rsplit('/', 1)

      if file_part == '...':
        remote_dir = os.path.join(snapshot, local_path)
        local_dir = os.path.join(self.checkout_dir, os.path.dirname(local_path))

        cmds.extend([
            cmd.Shell('mkdir', '-p', local_dir), cmd.Shell(
                'rsync', '-lr', remote_dir, local_dir)
        ])

    return cmds
