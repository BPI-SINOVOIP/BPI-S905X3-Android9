#!/usr/bin/python

#
# Copyright 2017, The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

"""Script that is used by developers to run style checks on Kotlin files."""

import argparse
import errno
import os
import subprocess
import sys

MAIN_DIRECTORY = os.path.normpath(os.path.dirname(__file__))
KTLINT_JAR = os.path.join(MAIN_DIRECTORY, 'ktlint-android-all.jar')


def main(args=None):
  parser = argparse.ArgumentParser()
  parser.add_argument('--file', '-f', nargs='*')
  parser.add_argument('--format', '-F', dest='format', action='store_true')
  parser.add_argument('--noformat', dest='format', action='store_false')
  parser.set_defaults(format=False)
  args = parser.parse_args()
  ktlint_args = [f for f in args.file if f.endswith('.kt')]
  if args.format:
    ktlint_args += ['-F']
  if not ktlint_args:
    sys.exit(0)

  ktlint_env = os.environ.copy()
  ktlint_env['JAVA_CMD'] = 'java'
  try:
    check = subprocess.Popen(['java', '-jar', KTLINT_JAR] + ktlint_args,
                             stdout=subprocess.PIPE, env=ktlint_env)
    stdout, _ = check.communicate()
    if stdout:
      print 'prebuilts/ktlint found errors in files you changed:'
      print stdout
      sys.exit(1)
    else:
      sys.exit(0)
  except OSError as e:
    if e.errno == errno.ENOENT:
      print 'Error running ktlint!'
      sys.exit(1)

if __name__ == '__main__':
  main()
