#!/usr/bin/python
#
# Copyright 2010 Google Inc. All Rights Reserved.

__author__ = 'asharif@google.com (Ahmad Sharif)'

import os
import random
import shutil
import tempfile
import unittest

from cros_utils import command_executer
from cros_utils import misc


class DivideAndMergeProfilesTest(unittest.TestCase):

  def tearDown(self):
    shutil.rmtree(self._program_dir)
    for profile_dir in self._profile_dirs:
      shutil.rmtree(profile_dir)

  def setUp(self):
    self._ce = command_executer.GetCommandExecuter()
    self._program_dir = tempfile.mkdtemp()
    self._writeProgram()
    self._writeMakefile()
    with misc.WorkingDirectory(self._program_dir):
      self._ce.RunCommand('make')
    num_profile_dirs = 2
    self._profile_dirs = []
    for i in range(num_profile_dirs):
      profile_dir = tempfile.mkdtemp()
      command = ('GCOV_PREFIX_STRIP=%s GCOV_PREFIX=$(/bin/pwd) '
                 ' %s/program' % (profile_dir.count('/'), self._program_dir))
      with misc.WorkingDirectory(profile_dir):
        self._ce.RunCommand(command)
      self._profile_dirs.append(profile_dir)
    self._merge_program = '/home/build/static/projects/crosstool/profile-merge/v14.5/profile_merge.par'

  def _writeMakefile(self):
    makefile_contents = """
CC = gcc

CFLAGS = -fprofile-generate

SRCS=$(wildcard *.c)

OBJS=$(SRCS:.c=.o)

all: program

program: $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS)

%.o: %.c
	$(CC) -c -o $@ $^ $(CFLAGS)"""

    makefile = os.path.join(self._program_dir, 'Makefile')
    with open(makefile, 'w') as f:
      print >> f, makefile_contents

  def _writeProgram(self, num_files=100):
    for i in range(num_files):
      current_file = os.path.join(self._program_dir, '%s.c' % i)
      with open(current_file, 'w') as f:
        if i != num_files - 1:
          print >> f, 'extern void foo%s();' % (i + 1)
          print >> f, 'void foo%s(){foo%s();}' % (i, i + 1)
        else:
          print >> f, "void foo%s(){printf(\"\");}" % i
        if i == 0:
          print >> f, 'int main(){foo%s(); return 0;}' % i

  def testMerge(self):
    reference_output = self._getReferenceOutput()
    my_output = self._getMyOutput()

    ret = self._diffOutputs(reference_output, my_output)
    shutil.rmtree(my_output)
    shutil.rmtree(reference_output)
    self.assertTrue(ret == 0)

  def _diffOutputs(self, reference, mine):
    command = 'diff -uNr %s %s' % (reference, mine)
    return self._ce.RunCommand(command)

  def _getMyOutput(self, args=''):
    my_output = tempfile.mkdtemp()
    my_merge_program = os.path.join(
        os.path.dirname(__file__), 'divide_and_merge_profiles.py')
    command = ('python %s --inputs=%s --output=%s '
               '--chunk_size=10 '
               '--merge_program=%s '
               '%s' % (my_merge_program, ','.join(self._profile_dirs),
                       my_output, self._merge_program, args))
    self._ce.RunCommand(command)
    return my_output

  def _getReferenceOutput(self, args=''):
    # First do a regular merge.
    reference_output = tempfile.mkdtemp()
    command = ('%s --inputs=%s --output=%s %s' %
               (self._merge_program, ','.join(self._profile_dirs),
                reference_output, args))
    self._ce.RunCommand(command)
    return reference_output

  def testMergeWithMultipliers(self):
    num_profiles = len(self._profile_dirs)
    multipliers = [str(random.randint(0, num_profiles)) \
                   for _ in range(num_profiles)]
    args = '--multipliers=%s' % ','.join(multipliers)

    reference_output = self._getReferenceOutput(args)
    my_output = self._getMyOutput(args)

    ret = self._diffOutputs(reference_output, my_output)

    shutil.rmtree(my_output)
    shutil.rmtree(reference_output)
    self.assertTrue(ret == 0)


if __name__ == '__main__':
  unittest.main()
