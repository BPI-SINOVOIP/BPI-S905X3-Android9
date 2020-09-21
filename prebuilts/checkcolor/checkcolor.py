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

"""Script used by developers to run hardcoded color check."""

import argparse
import errno
import os
import shutil
import subprocess
import sys

MAIN_DIRECTORY = os.path.normpath(os.path.dirname(__file__))
CHECKCOLOR_JAR = os.path.join(MAIN_DIRECTORY, 'checkcolor.jar')
LINT_BASELINE_FILENAME = 'color-check-baseline.xml'
COLOR_CHECK_README_PATH = 'prebuilts/checkcolor/README.md'
TMP_FOLDER = '/tmp/color'


def CopyProject(root):
  """Copy the project to tmp folder and remove the translation files.

  Args:
    root: Root folder of the project
  """
  cmd = 'cp -r {0}/* {1}/&& rm -r {1}/res/values-?? {1}/res/values-??-*'.format(root, TMP_FOLDER)
  os.system(cmd)

def Mkdir():
  """Create the tmp folder is necessary."""
  if os.path.exists(TMP_FOLDER):
    shutil.rmtree(TMP_FOLDER)
  os.makedirs(TMP_FOLDER)


def RunCheckOnProject(root):
  """Run color lint check on project.

  Args:
    root: Root folder of the project.
  Returns:
    exitcode and stdout
  """
  checkcolor_env = os.environ.copy()
  checkcolor_env['ANDROID_LINT_JARS'] = CHECKCOLOR_JAR

  Mkdir()
  CopyProject(root)

  try:
    baseline = os.path.join(root, LINT_BASELINE_FILENAME)
    check = subprocess.Popen(['lint', '--check', 'HardCodedColor', '--disable', 'LintError',
                              '--baseline', baseline, '--exitcode', TMP_FOLDER],
                             stdout=subprocess.PIPE, env=checkcolor_env)
    stdout, _ = check.communicate()
    exitcode = check.returncode
  except OSError as e:
    if e.errno == errno.ENOENT:
      print 'Error in color lint check'
      if not os.environ.get('ANDROID_BUILD_TOP'):
        print 'Try setting up your environment first:'
        print '    source build/envsetup.sh && lunch <target>'
      sys.exit(1)

  return (exitcode, stdout)


def main():
  """Run color lint check on all projects listed in parameter -p."""
  parser = argparse.ArgumentParser(description='Check hardcoded colors.')
  parser.add_argument('--project', '-p', nargs='+')
  args = parser.parse_args()
  exitcode = 0

  for rootdir in args.project:
    code, message = RunCheckOnProject(rootdir)
    if code != 0:
      exitcode = 1
      print message

  if exitcode == 1:
    print 'Please check link for more info:', COLOR_CHECK_README_PATH
  sys.exit(exitcode)


if __name__ == '__main__':
  main()
