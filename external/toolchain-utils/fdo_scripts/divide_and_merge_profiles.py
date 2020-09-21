#!/usr/bin/python
#
# Copyright 2011 Google Inc. All Rights Reserved.
"""Script to divide and merge profiles."""

import copy
import optparse
import os
import pickle
import re
import sys
import tempfile

import build_chrome_browser
import lock_machine
import run_tests
from cros_utils import command_executer
from cros_utils import logger


class ProfileMerger:

  def __init__(self, inputs, output, chunk_size, merge_program, multipliers):
    self._inputs = inputs
    self._output = output
    self._chunk_size = chunk_size
    self._merge_program = merge_program
    self._multipliers = multipliers
    self._ce = command_executer.GetCommandExecuter()
    self._l = logger.GetLogger()

  def _GetFilesSetForInputDir(self, input_dir):
    output_file = tempfile.mktemp()
    command = "find %s -name '*.gcda' -o -name '*.imports' > %s" % (input_dir,
                                                                    output_file)
    self._ce.RunCommand(command)
    files = open(output_file, 'r').read()
    files_set = set([])
    for f in files.splitlines():
      stripped_file = f.replace(input_dir, '', 1)
      stripped_file = stripped_file.lstrip('/')
      files_set.add(stripped_file)
    return files_set

  def _PopulateFilesSet(self):
    self._files_set = set([])
    for i in self._inputs:
      current_files_set = self._GetFilesSetForInputDir(i)
      self._files_set.update(current_files_set)

  def _GetSubset(self):
    ret = []
    for i in range(self._chunk_size):
      if not self._files_set:
        break
      ret.append(self._files_set.pop())
    return ret

  def _CopyFilesTree(self, input_dir, files, output_dir):
    for f in files:
      src_file = os.path.join(input_dir, f)
      dst_file = os.path.join(output_dir, f)
      if not os.path.isdir(os.path.dirname(dst_file)):
        command = 'mkdir -p %s' % os.path.dirname(dst_file)
        self._ce.RunCommand(command)
      command = 'cp %s %s' % (src_file, dst_file)
      self._ce.RunCommand(command)

  def _DoChunkMerge(self, current_files):
    temp_dirs = []
    for i in self._inputs:
      temp_dir = tempfile.mkdtemp()
      temp_dirs.append(temp_dir)
      self._CopyFilesTree(i, current_files, temp_dir)
    # Now do the merge.
    command = ('%s --inputs=%s --output=%s' %
               (self._merge_program, ','.join(temp_dirs), self._output))
    if self._multipliers:
      command = ('%s --multipliers=%s' % (command, self._multipliers))
    ret = self._ce.RunCommand(command)
    assert ret == 0, '%s command failed!' % command
    for temp_dir in temp_dirs:
      command = 'rm -rf %s' % temp_dir
      self._ce.RunCommand(command)

  def DoMerge(self):
    self._PopulateFilesSet()
    while True:
      current_files = self._GetSubset()
      if not current_files:
        break
      self._DoChunkMerge(current_files)


def Main(argv):
  """The main function."""
  # Common initializations
  ###  command_executer.InitCommandExecuter(True)
  command_executer.InitCommandExecuter()
  l = logger.GetLogger()
  ce = command_executer.GetCommandExecuter()
  parser = optparse.OptionParser()
  parser.add_option('--inputs',
                    dest='inputs',
                    help='Comma-separated input profile directories to merge.')
  parser.add_option('--output', dest='output', help='Output profile directory.')
  parser.add_option('--chunk_size',
                    dest='chunk_size',
                    default='50',
                    help='Chunk size to divide up the profiles into.')
  parser.add_option('--merge_program',
                    dest='merge_program',
                    default='/home/xur/bin/profile_merge_v15.par',
                    help='Merge program to use to do the actual merge.')
  parser.add_option('--multipliers',
                    dest='multipliers',
                    help='multipliers to use when merging. (optional)')

  options, _ = parser.parse_args(argv)

  if not all([options.inputs, options.output]):
    l.LogError('Must supply --inputs and --output')
    return 1

  try:
    pm = ProfileMerger(
        options.inputs.split(','), options.output, int(options.chunk_size),
        options.merge_program, options.multipliers)
    pm.DoMerge()
    retval = 0
  except:
    retval = 1
  finally:
    print 'My work is done...'
  return retval


if __name__ == '__main__':
  retval = Main(sys.argv)
  sys.exit(retval)
