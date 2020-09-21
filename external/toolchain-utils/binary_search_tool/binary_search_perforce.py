#!/usr/bin/env python2
"""Module of binary serch for perforce."""
from __future__ import print_function

import math
import argparse
import os
import re
import sys
import tempfile

from cros_utils import command_executer
from cros_utils import logger

verbose = True


def _GetP4ClientSpec(client_name, p4_paths):
  p4_string = ''
  for p4_path in p4_paths:
    if ' ' not in p4_path:
      p4_string += ' -a %s' % p4_path
    else:
      p4_string += " -a \"" + (' //' + client_name + '/').join(p4_path) + "\""

  return p4_string


def GetP4Command(client_name, p4_port, p4_paths, checkoutdir, p4_snapshot=''):
  command = ''

  if p4_snapshot:
    command += 'mkdir -p ' + checkoutdir
    for p4_path in p4_paths:
      real_path = p4_path[1]
      if real_path.endswith('...'):
        real_path = real_path.replace('/...', '')
        command += (
            '; mkdir -p ' + checkoutdir + '/' + os.path.dirname(real_path))
        command += ('&& rsync -lr ' + p4_snapshot + '/' + real_path + ' ' +
                    checkoutdir + '/' + os.path.dirname(real_path))
    return command

  command += ' export P4CONFIG=.p4config'
  command += ' && mkdir -p ' + checkoutdir
  command += ' && cd ' + checkoutdir
  command += ' && cp ${HOME}/.p4config .'
  command += ' && chmod u+w .p4config'
  command += " && echo \"P4PORT=" + p4_port + "\" >> .p4config"
  command += " && echo \"P4CLIENT=" + client_name + "\" >> .p4config"
  command += (' && g4 client ' + _GetP4ClientSpec(client_name, p4_paths))
  command += ' && g4 sync '
  command += ' && cd -'
  return command


class BinarySearchPoint(object):
  """Class of binary search point."""

  def __init__(self, revision, status, tag=None):
    self.revision = revision
    self.status = status
    self.tag = tag


class BinarySearcher(object):
  """Class of binary searcher."""

  def __init__(self, logger_to_set=None):
    self.sorted_list = []
    self.index_log = []
    self.status_log = []
    self.skipped_indices = []
    self.current = 0
    self.points = {}
    self.lo = 0
    self.hi = 0
    if logger_to_set is not None:
      self.logger = logger_to_set
    else:
      self.logger = logger.GetLogger()

  def SetSortedList(self, sorted_list):
    assert len(sorted_list) > 0
    self.sorted_list = sorted_list
    self.index_log = []
    self.hi = len(sorted_list) - 1
    self.lo = 0
    self.points = {}
    for i in range(len(self.sorted_list)):
      bsp = BinarySearchPoint(self.sorted_list[i], -1, 'Not yet done.')
      self.points[i] = bsp

  def SetStatus(self, status, tag=None):
    message = ('Revision: %s index: %d returned: %d' %
               (self.sorted_list[self.current], self.current, status))
    self.logger.LogOutput(message, print_to_console=verbose)
    assert status == 0 or status == 1 or status == 125
    self.index_log.append(self.current)
    self.status_log.append(status)
    bsp = BinarySearchPoint(self.sorted_list[self.current], status, tag)
    self.points[self.current] = bsp

    if status == 125:
      self.skipped_indices.append(self.current)

    if status == 0 or status == 1:
      if status == 0:
        self.lo = self.current + 1
      elif status == 1:
        self.hi = self.current
      self.logger.LogOutput('lo: %d hi: %d\n' % (self.lo, self.hi))
      self.current = (self.lo + self.hi) / 2

    if self.lo == self.hi:
      message = ('Search complete. First bad version: %s'
                 ' at index: %d' % (self.sorted_list[self.current], self.lo))
      self.logger.LogOutput(message)
      return True

    for index in range(self.lo, self.hi):
      if index not in self.skipped_indices:
        return False
    self.logger.LogOutput(
        'All skipped indices between: %d and %d\n' % (self.lo, self.hi),
        print_to_console=verbose)
    return True

  # Does a better job with chromeos flakiness.
  def GetNextFlakyBinary(self):
    t = (self.lo, self.current, self.hi)
    q = [t]
    while len(q):
      element = q.pop(0)
      if element[1] in self.skipped_indices:
        # Go top
        to_add = (element[0], (element[0] + element[1]) / 2, element[1])
        q.append(to_add)
        # Go bottom
        to_add = (element[1], (element[1] + element[2]) / 2, element[2])
        q.append(to_add)
      else:
        self.current = element[1]
        return
    assert len(q), 'Queue should never be 0-size!'

  def GetNextFlakyLinear(self):
    current_hi = self.current
    current_lo = self.current
    while True:
      if current_hi < self.hi and current_hi not in self.skipped_indices:
        self.current = current_hi
        break
      if current_lo >= self.lo and current_lo not in self.skipped_indices:
        self.current = current_lo
        break
      if current_lo < self.lo and current_hi >= self.hi:
        break

      current_hi += 1
      current_lo -= 1

  def GetNext(self):
    self.current = (self.hi + self.lo) / 2
    # Try going forward if current is skipped.
    if self.current in self.skipped_indices:
      self.GetNextFlakyBinary()

    # TODO: Add an estimated time remaining as well.
    message = ('Estimated tries: min: %d max: %d\n' %
               (1 + math.log(self.hi - self.lo, 2),
                self.hi - self.lo - len(self.skipped_indices)))
    self.logger.LogOutput(message, print_to_console=verbose)
    message = ('lo: %d hi: %d current: %d version: %s\n' %
               (self.lo, self.hi, self.current, self.sorted_list[self.current]))
    self.logger.LogOutput(message, print_to_console=verbose)
    self.logger.LogOutput(str(self), print_to_console=verbose)
    return self.sorted_list[self.current]

  def SetLoRevision(self, lo_revision):
    self.lo = self.sorted_list.index(lo_revision)

  def SetHiRevision(self, hi_revision):
    self.hi = self.sorted_list.index(hi_revision)

  def GetAllPoints(self):
    to_return = ''
    for i in range(len(self.sorted_list)):
      to_return += ('%d %d %s\n' % (self.points[i].status, i,
                                    self.points[i].revision))

    return to_return

  def __str__(self):
    to_return = ''
    to_return += 'Current: %d\n' % self.current
    to_return += str(self.index_log) + '\n'
    revision_log = []
    for index in self.index_log:
      revision_log.append(self.sorted_list[index])
    to_return += str(revision_log) + '\n'
    to_return += str(self.status_log) + '\n'
    to_return += 'Skipped indices:\n'
    to_return += str(self.skipped_indices) + '\n'
    to_return += self.GetAllPoints()
    return to_return


class RevisionInfo(object):
  """Class of reversion info."""

  def __init__(self, date, client, description):
    self.date = date
    self.client = client
    self.description = description
    self.status = -1


class VCSBinarySearcher(object):
  """Class of VCS binary searcher."""

  def __init__(self):
    self.bs = BinarySearcher()
    self.rim = {}
    self.current_ce = None
    self.checkout_dir = None
    self.current_revision = None

  def Initialize(self):
    pass

  def GetNextRevision(self):
    pass

  def CheckoutRevision(self, revision):
    pass

  def SetStatus(self, status):
    pass

  def Cleanup(self):
    pass

  def SetGoodRevision(self, revision):
    if revision is None:
      return
    assert revision in self.bs.sorted_list
    self.bs.SetLoRevision(revision)

  def SetBadRevision(self, revision):
    if revision is None:
      return
    assert revision in self.bs.sorted_list
    self.bs.SetHiRevision(revision)


class P4BinarySearcher(VCSBinarySearcher):
  """Class of P4 binary searcher."""

  def __init__(self, p4_port, p4_paths, test_command):
    VCSBinarySearcher.__init__(self)
    self.p4_port = p4_port
    self.p4_paths = p4_paths
    self.test_command = test_command
    self.checkout_dir = tempfile.mkdtemp()
    self.ce = command_executer.GetCommandExecuter()
    self.client_name = 'binary-searcher-$HOSTNAME-$USER'
    self.job_log_root = '/home/asharif/www/coreboot_triage/'
    self.changes = None

  def Initialize(self):
    self.Cleanup()
    command = GetP4Command(self.client_name, self.p4_port, self.p4_paths, 1,
                           self.checkout_dir)
    self.ce.RunCommand(command)
    command = 'cd %s && g4 changes ...' % self.checkout_dir
    _, out, _ = self.ce.RunCommandWOutput(command)
    self.changes = re.findall(r'Change (\d+)', out)
    change_infos = re.findall(r'Change (\d+) on ([\d/]+) by '
                              r"([^\s]+) ('[^']*')", out)
    for change_info in change_infos:
      ri = RevisionInfo(change_info[1], change_info[2], change_info[3])
      self.rim[change_info[0]] = ri
    # g4 gives changes in reverse chronological order.
    self.changes.reverse()
    self.bs.SetSortedList(self.changes)

  def SetStatus(self, status):
    self.rim[self.current_revision].status = status
    return self.bs.SetStatus(status)

  def GetNextRevision(self):
    next_revision = self.bs.GetNext()
    self.current_revision = next_revision
    return next_revision

  def CleanupCLs(self):
    if not os.path.isfile(self.checkout_dir + '/.p4config'):
      command = 'cd %s' % self.checkout_dir
      command += ' && cp ${HOME}/.p4config .'
      command += " && echo \"P4PORT=" + self.p4_port + "\" >> .p4config"
      command += " && echo \"P4CLIENT=" + self.client_name + "\" >> .p4config"
      self.ce.RunCommand(command)
    command = 'cd %s' % self.checkout_dir
    command += '; g4 changes -c %s' % self.client_name
    _, out, _ = self.ce.RunCommandWOUTPUOT(command)
    changes = re.findall(r'Change (\d+)', out)
    if len(changes) != 0:
      command = 'cd %s' % self.checkout_dir
      for change in changes:
        command += '; g4 revert -c %s' % change
      self.ce.RunCommand(command)

  def CleanupClient(self):
    command = 'cd %s' % self.checkout_dir
    command += '; g4 revert ...'
    command += '; g4 client -d %s' % self.client_name
    self.ce.RunCommand(command)

  def Cleanup(self):
    self.CleanupCLs()
    self.CleanupClient()

  def __str__(self):
    to_return = ''
    for change in self.changes:
      ri = self.rim[change]
      if ri.status == -1:
        to_return = '%s\t%d\n' % (change, ri.status)
      else:
        to_return += ('%s\t%d\t%s\t%s\t%s\t%s\t%s\t%s\n' %
                      (change, ri.status, ri.date, ri.client, ri.description,
                       self.job_log_root + change + '.cmd',
                       self.job_log_root + change + '.out',
                       self.job_log_root + change + '.err'))
    return to_return


class P4GCCBinarySearcher(P4BinarySearcher):
  """Class of P4 gcc binary searcher."""

  # TODO: eventually get these patches from g4 instead of creating them manually
  def HandleBrokenCLs(self, current_revision):
    cr = int(current_revision)
    problematic_ranges = []
    problematic_ranges.append([44528, 44539])
    problematic_ranges.append([44528, 44760])
    problematic_ranges.append([44335, 44882])
    command = 'pwd'
    for pr in problematic_ranges:
      if cr in range(pr[0], pr[1]):
        patch_file = '/home/asharif/triage_tool/%d-%d.patch' % (pr[0], pr[1])
        f = open(patch_file)
        patch = f.read()
        f.close()
        files = re.findall('--- (//.*)', patch)
        command += '; cd %s' % self.checkout_dir
        for f in files:
          command += '; g4 open %s' % f
        command += '; patch -p2 < %s' % patch_file
    self.current_ce.RunCommand(command)

  def CheckoutRevision(self, current_revision):
    job_logger = logger.Logger(
        self.job_log_root, current_revision, True, subdir='')
    self.current_ce = command_executer.GetCommandExecuter(job_logger)

    self.CleanupCLs()
    # Change the revision of only the gcc part of the toolchain.
    command = ('cd %s/gcctools/google_vendor_src_branch/gcc '
               '&& g4 revert ...; g4 sync @%s' % (self.checkout_dir,
                                                  current_revision))
    self.current_ce.RunCommand(command)

    self.HandleBrokenCLs(current_revision)


def Main(argv):
  """The main function."""
  # Common initializations
  ###  command_executer.InitCommandExecuter(True)
  ce = command_executer.GetCommandExecuter()

  parser = argparse.ArgumentParser()
  parser.add_argument(
      '-n',
      '--num_tries',
      dest='num_tries',
      default='100',
      help='Number of tries.')
  parser.add_argument(
      '-g',
      '--good_revision',
      dest='good_revision',
      help='Last known good revision.')
  parser.add_argument(
      '-b',
      '--bad_revision',
      dest='bad_revision',
      help='Last known bad revision.')
  parser.add_argument(
      '-s', '--script', dest='script', help='Script to run for every version.')
  options = parser.parse_args(argv)
  # First get all revisions
  p4_paths = [
      '//depot2/gcctools/google_vendor_src_branch/gcc/gcc-4.4.3/...',
      '//depot2/gcctools/google_vendor_src_branch/binutils/'
      'binutils-2.20.1-mobile/...',
      '//depot2/gcctools/google_vendor_src_branch/'
      'binutils/binutils-20100303/...'
  ]
  p4gccbs = P4GCCBinarySearcher('perforce2:2666', p4_paths, '')

  # Main loop:
  terminated = False
  num_tries = int(options.num_tries)
  script = os.path.expanduser(options.script)

  try:
    p4gccbs.Initialize()
    p4gccbs.SetGoodRevision(options.good_revision)
    p4gccbs.SetBadRevision(options.bad_revision)
    while not terminated and num_tries > 0:
      current_revision = p4gccbs.GetNextRevision()

      # Now run command to get the status
      ce = command_executer.GetCommandExecuter()
      command = '%s %s' % (script, p4gccbs.checkout_dir)
      status = ce.RunCommand(command)
      message = ('Revision: %s produced: %d status\n' % (current_revision,
                                                         status))
      logger.GetLogger().LogOutput(message, print_to_console=verbose)
      terminated = p4gccbs.SetStatus(status)
      num_tries -= 1
      logger.GetLogger().LogOutput(str(p4gccbs), print_to_console=verbose)

    if not terminated:
      logger.GetLogger().LogOutput(
          'Tries: %d expired.' % num_tries, print_to_console=verbose)
    logger.GetLogger().LogOutput(str(p4gccbs.bs), print_to_console=verbose)
  except (KeyboardInterrupt, SystemExit):
    logger.GetLogger().LogOutput('Cleaning up...')
  finally:
    logger.GetLogger().LogOutput(str(p4gccbs.bs), print_to_console=verbose)
    status = p4gccbs.Cleanup()


if __name__ == '__main__':
  Main(sys.argv[1:])
