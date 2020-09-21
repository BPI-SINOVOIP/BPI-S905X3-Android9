#!/usr/bin/python
#
# Copyright 2017 - The Android Open Source Project
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

"""Generates a report on CKI syscall coverage in VTS LTP.

This module generates a report on the syscalls in the Android CKI and
their coverage in VTS LTP.

The coverage report provides, for each syscall in the CKI, the number of
enabled and disabled LTP tests for the syscall in VTS. If VTS test output is
supplied, the report instead provides the number of disabled, skipped, failing,
and passing tests for each syscall.

Assumptions are made about the structure of files in LTP source
and the naming convention.
"""

import argparse
import os.path
import re
import sys
import xml.etree.ElementTree as ET

if "ANDROID_BUILD_TOP" not in os.environ:
  print ("Please set up your Android build environment by running "
         "\". build/envsetup.sh\" and \"lunch\".")
  sys.exit(-1)

sys.path.append(os.path.join(os.environ["ANDROID_BUILD_TOP"],
                "bionic/libc/tools"))
import gensyscalls

sys.path.append(os.path.join(os.environ["ANDROID_BUILD_TOP"],
                             "test/vts-testcase/kernel/ltp/configs"))
import disabled_tests as vts_disabled
import stable_tests as vts_stable

bionic_libc_root = os.path.join(os.environ["ANDROID_BUILD_TOP"], "bionic/libc")


class CKI_Coverage(object):
  """Determines current test coverage of CKI system calls in LTP.

  Many of the system calls in the CKI are tested by LTP. For a given
  system call an LTP test may or may not exist, that LTP test may or may
  not be currently compiling properly for Android, the test may not be
  stable, the test may not be running due to environment issues or
  passing. This class looks at various sources of information to determine
  the current test coverage of system calls in the CKI from LTP.

  Note that due to some deviations in LTP of tests from the common naming
  convention there there may be tests that are flagged here as not having
  coverage when in fact they do.
  """

  LTP_SYSCALL_ROOT = os.path.join(os.environ["ANDROID_BUILD_TOP"],
                                  "external/ltp/testcases/kernel/syscalls")
  DISABLED_IN_LTP_PATH = os.path.join(os.environ["ANDROID_BUILD_TOP"],
                        "external/ltp/android/tools/disabled_tests.txt")

  ltp_full_set = []

  cki_syscalls = []

  disabled_in_ltp = []
  disabled_in_vts_ltp = vts_disabled.DISABLED_TESTS
  stable_in_vts_ltp = vts_stable.STABLE_TESTS

  syscall_tests = {}
  disabled_tests = {}
  failing_tests = {}
  skipped_tests = {}
  passing_tests = {}

  test_results = {}

  def __init__(self, arch):
    self._arch = arch

  def load_ltp_tests(self):
    """Load the list of LTP syscall tests.

    Load the list of all syscall tests existing in LTP.
    """
    for path, dirs, files in os.walk(self.LTP_SYSCALL_ROOT):
      for filename in files:
        basename, ext = os.path.splitext(filename)
        if ext != ".c": continue
        self.ltp_full_set.append(basename)

  def load_ltp_disabled_tests(self):
    """Load the list of LTP tests not being compiled.

    The LTP repository in Android contains a list of tests which are not
    compiled due to incompatibilities with Android.
    """
    with open(self.DISABLED_IN_LTP_PATH) as fp:
      for line in fp:
        line = line.strip()
        if not line: continue
        test_re = re.compile(r"^(\w+)")
        test_match = re.match(test_re, line)
        if not test_match: continue
        self.disabled_in_ltp.append(test_match.group(1))

  def parse_test_results(self, results):
    """Parse xml from VTS output to collect LTP results.

    Parse xml to collect pass/fail results for each LTP
    test. A failure occurs if a test has a result other than pass or fail.

    Args:
      results: Path to VTS output XML file.
    """
    tree = ET.parse(results)
    root = tree.getroot()
    found = False

    # find LTP module
    for module in root.findall("Module"):
      if module.attrib["name"] != "VtsKernelLtp": continue

      # ARM arch and ABI strings don't match exactly, x86 and mips do.
      if self._arch == "arm":
        if not module.attrib["abi"].startswith("armeabi-"): continue
      elif self._arch == "arm64":
        if not module.attrib["abi"].startswith("arm64-"): continue
      elif self._arch != module.attrib["abi"]: continue

      # find LTP testcase
      for testcase in module.findall("TestCase"):
        if testcase.attrib["name"] != "KernelLtpTest": continue
        found = True

        # iterate over each LTP test
        for test in testcase.findall("Test"):
          test_re = re.compile(r"^syscalls.(\w+)_((32bit)|(64bit))$")
          test_match = re.match(test_re, test.attrib["name"])
          if not test_match: continue

          test_name = test_match.group(1)

          if test.attrib["result"] == "pass":
            self.test_results[test_name] = "pass"
          elif test.attrib["result"] == "fail":
            self.test_results[test_name] = "fail"
          else:
            print ("Unknown VTS LTP test result for %s is %s" %
                   (test_name, test.attrib["result"]))
            sys.exit(-1)
    if not found:
      print "Error: LTP test results for arch %s not found in supplied test results." % self._arch
      sys.exit(-1)

  def ltp_test_special_cases(self, syscall, test):
    """Detect special cases in syscall to LTP mapping.

    Most syscall tests in LTP follow a predictable naming
    convention, but some do not. Detect known special cases.

    Args:
      syscall: The name of a syscall.
      test: The name of a testcase.

    Returns:
      A boolean indicating whether the given syscall is tested
      by the given testcase.
    """
    if syscall == "clock_nanosleep" and test == "clock_nanosleep2_01":
      return True
    if syscall == "fadvise" and test.startswith("posix_fadvise"):
      return True
    if syscall == "futex" and test.startswith("futex_"):
      return True
    if syscall == "inotify_add_watch" or syscall == "inotify_rm_watch":
      test_re = re.compile(r"^inotify\d+$")
      if re.match(test_re, test):
        return True
    if syscall == "newfstatat":
      test_re = re.compile(r"^fstatat\d+$")
      if re.match(test_re, test):
        return True

    return False

  def match_syscalls_to_tests(self, syscalls):
    """Match syscalls with tests in LTP.

    Create a mapping from CKI syscalls and tests in LTP. This mapping can
    largely be determined using a common naming convention in the LTP file
    hierarchy but there are special cases that have to be taken care of.

    Args:
      syscalls: List of syscall structures containing all syscalls
        in the CKI.
    """
    for syscall in syscalls:
      if self._arch not in syscall:
        continue
      self.cki_syscalls.append(syscall["name"])
      self.syscall_tests[syscall["name"]] = []
      # LTP does not use the 64 at the end of syscall names for testcases.
      ltp_syscall_name = syscall["name"]
      if ltp_syscall_name.endswith("64"):
        ltp_syscall_name = ltp_syscall_name[0:-2]
      # Most LTP syscalls have source files for the tests that follow
      # a naming convention in the regexp below. Exceptions exist though.
      # For now those are checked for specifically.
      test_re = re.compile(r"^%s_?0?\d\d?$" % ltp_syscall_name)
      for test in self.ltp_full_set:
        if (re.match(test_re, test) or
            self.ltp_test_special_cases(ltp_syscall_name, test)):
          # The filenames of the ioctl tests in LTP do not match the name
          # of the testcase defined in that source, which is what shows
          # up in VTS.
          if ltp_syscall_name == "ioctl":
            test = "ioctl01_02"
          self.syscall_tests[syscall["name"]].append(test)
    self.cki_syscalls.sort()

  def update_test_status(self):
    """Populate test configuration and output for all CKI syscalls.

    Go through VTS test configuration and test results (if provided) to
    populate data for all CKI syscalls.
    """
    for syscall in self.cki_syscalls:
      self.disabled_tests[syscall] = []
      self.skipped_tests[syscall] = []
      self.failing_tests[syscall] = []
      self.passing_tests[syscall] = []
      if not self.syscall_tests[syscall]:
        continue
      for test in self.syscall_tests[syscall]:
        if (test in self.disabled_in_ltp or
            "syscalls.%s" % test in self.disabled_in_vts_ltp or
            ("syscalls.%s_32bit" % test not in self.stable_in_vts_ltp and
             "syscalls.%s_64bit" % test not in self.stable_in_vts_ltp)):
          self.disabled_tests[syscall].append(test)
          continue

        if not self.test_results:
          continue

        if test not in self.test_results:
          self.skipped_tests[syscall].append(test)
        elif self.test_results[test] == "fail":
          self.failing_tests[syscall].append(test)
        elif self.test_results[test] == "pass":
          self.passing_tests[syscall].append(test)
        else:
          print ("Warning - could not resolve test %s status for syscall %s" %
                 (test, syscall))

  def output_results(self):
    """Pretty print the CKI syscall LTP coverage results.

    Pretty prints a table of the CKI syscall LTP coverage, pointing out
    syscalls which have no passing tests in VTS LTP.
    """
    if not self.test_results:
      self.output_limited_results()
      return
    count = 0
    uncovered = 0
    for syscall in self.cki_syscalls:
      if not count % 20:
        print ("%25s   Disabled Skipped Failing Passing -------------" %
               "-------------")
      sys.stdout.write("%25s   %s        %s       %s       %s" %
                       (syscall, len(self.disabled_tests[syscall]),
                        len(self.skipped_tests[syscall]),
                        len(self.failing_tests[syscall]),
                        len(self.passing_tests[syscall])))
      if not self.passing_tests[syscall]:
        print " <-- uncovered"
        uncovered += 1
      else:
        print ""
      count += 1
    print ""
    print ("Total uncovered syscalls: %s out of %s" %
           (uncovered, len(self.cki_syscalls)))

  def output_limited_results(self):
    """Pretty print the CKI syscall LTP coverage without VTS test results.

    When no VTS test results are supplied then only the count of enabled
    and disabled LTP tests may be shown.
    """
    count = 0
    uncovered = 0
    for syscall in self.cki_syscalls:
      if not count % 20:
        print ("%25s   Disabled Enabled -------------" %
               "-------------")
      sys.stdout.write("%25s   %s        %s" %
                       (syscall, len(self.disabled_tests[syscall]),
                        len(self.syscall_tests[syscall]) -
                        len(self.disabled_tests[syscall])))
      if (len(self.syscall_tests[syscall]) -
          len(self.disabled_tests[syscall]) <= 0):
        print " <-- uncovered"
        uncovered += 1
      else:
        print ""
      count += 1
    print ""
    print ("Total uncovered syscalls: %s out of %s" %
           (uncovered, len(self.cki_syscalls)))

if __name__ == "__main__":
  parser = argparse.ArgumentParser(description="Output list of system calls "
          "in the Common Kernel Interface and their VTS LTP coverage. If VTS "
          "test output is supplied, output includes system calls which have "
          "tests in VTS LTP, but the tests are skipped or are failing.")
  parser.add_argument("arch", help="architecture of Android platform")
  parser.add_argument("-l", action="store_true",
                      help="list CKI syscalls only, without coverage")
  parser.add_argument("-r", "--results", help="path to VTS test_result.xml")
  args = parser.parse_args()
  if args.arch not in gensyscalls.all_arches:
    print "Arch must be one of the following:"
    print gensyscalls.all_arches
    exit(-1)

  cki = gensyscalls.SysCallsTxtParser()
  cki.parse_file(os.path.join(bionic_libc_root, "SYSCALLS.TXT"))
  cki.parse_file(os.path.join(bionic_libc_root, "SECCOMP_WHITELIST.TXT"))
  cki.parse_file(os.path.join(bionic_libc_root, "SECCOMP_WHITELIST_GLOBAL.TXT"))
  if args.l:
    for syscall in cki.syscalls:
      if args.arch in syscall:
        print syscall["name"]
    exit(0)

  cki_cov = CKI_Coverage(args.arch)

  cki_cov.load_ltp_tests()
  cki_cov.load_ltp_disabled_tests()
  if args.results:
    cki_cov.parse_test_results(args.results)
  cki_cov.match_syscalls_to_tests(cki.syscalls)
  cki_cov.update_test_status()

  beta_string = ("*** WARNING: This script is still in development and may\n"
                 "*** report both false positives and negatives.")
  print beta_string
  cki_cov.output_results()
  print beta_string
