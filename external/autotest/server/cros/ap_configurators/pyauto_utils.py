# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Utilities for PyAuto."""

import httplib
import logging
import os
import shutil
import socket
import sys
import tempfile
import unittest
import urlparse
import zipfile


class ExistingPathReplacer(object):
  """Facilitates backing up a given path (file or dir)..

  Often you want to manipulate a directory or file for testing but don't want to
  meddle with the existing contents.  This class lets you make a backup, and
  reinstate the backup when done.  A backup is made in an adjacent directory,
  so you need to make sure you have write permissions to the parent directory.

  Works seemlessly in cases where the requested path already exists, or not.

  Automatically reinstates the backed up path (if any) when object is deleted.
  """
  _path = ''
  _backup_dir = None  # dir to which existing content is backed up
  _backup_basename = ''

  def __init__(self, path, path_type='dir'):
    """Initialize the object, making backups if necessary.

    Args:
      path: the requested path to file or directory
      path_type: path type. Options: 'file', 'dir'. Default: 'dir'
    """
    assert path_type in ('file', 'dir'), 'Invalid path_type: %s' % path_type
    self._path_type = path_type
    self._path = path
    if os.path.exists(self._path):
      if 'dir' == self._path_type:
        assert os.path.isdir(self._path), '%s is not a directory' % self._path
      else:
        assert os.path.isfile(self._path), '%s is not a file' % self._path
      # take a backup
      self._backup_basename = os.path.basename(self._path)
      self._backup_dir = tempfile.mkdtemp(dir=os.path.dirname(self._path),
                                          prefix='bkp-' + self._backup_basename)
      logging.info('Backing up %s in %s' % (self._path, self._backup_dir))
      shutil.move(self._path,
                  os.path.join(self._backup_dir, self._backup_basename))
    self._CreateRequestedPath()

  def __del__(self):
    """Cleanup. Reinstate backup."""
    self._CleanupRequestedPath()
    if self._backup_dir:  # Reinstate, if backed up.
      from_path = os.path.join(self._backup_dir, self._backup_basename)
      logging.info('Reinstating backup from %s to %s' % (from_path, self._path))
      shutil.move(from_path, self._path)
    self._RemoveBackupDir()

  def _CreateRequestedPath(self):
    # Create intermediate dirs if needed.
    if not os.path.exists(os.path.dirname(self._path)):
      os.makedirs(os.path.dirname(self._path))
    if 'dir' == self._path_type:
      os.mkdir(self._path)
    else:
      open(self._path, 'w').close()

  def _CleanupRequestedPath(self):
    if os.path.exists(self._path):
      if os.path.isdir(self._path):
        shutil.rmtree(self._path, ignore_errors=True)
      else:
        os.remove(self._path)

  def _RemoveBackupDir(self):
    if self._backup_dir and os.path.isdir(self._backup_dir):
      shutil.rmtree(self._backup_dir, ignore_errors=True)


def RemovePath(path):
  """Remove the given path (file or dir)."""
  if os.path.isdir(path):
    shutil.rmtree(path, ignore_errors=True)
    return
  try:
    os.remove(path)
  except OSError:
    pass


def UnzipFilenameToDir(filename, dir):
  """Unzip |filename| to directory |dir|.

  This works with as low as python2.4 (used on win).
  """
  zf = zipfile.ZipFile(filename)
  pushd = os.getcwd()
  if not os.path.isdir(dir):
    os.mkdir(dir)
  os.chdir(dir)
  # Extract files.
  for info in zf.infolist():
    name = info.filename
    if name.endswith('/'):  # dir
      if not os.path.isdir(name):
        os.makedirs(name)
    else:  # file
      dir = os.path.dirname(name)
      if not os.path.isdir(dir):
        os.makedirs(dir)
      out = open(name, 'wb')
      out.write(zf.read(name))
      out.close()
    # Set permissions. Permission info in external_attr is shifted 16 bits.
    os.chmod(name, info.external_attr >> 16L)
  os.chdir(pushd)


def GetCurrentPlatform():
  """Get a string representation for the current platform.

  Returns:
    'mac', 'win' or 'linux'
  """
  if sys.platform == 'darwin':
    return 'mac'
  if sys.platform == 'win32':
    return 'win'
  if sys.platform.startswith('linux'):
    return 'linux'
  raise RuntimeError('Unknown platform')


def PrintPerfResult(graph_name, series_name, data_point, units,
                    show_on_waterfall=False):
  """Prints a line to stdout that is specially formatted for the perf bots.

  Args:
    graph_name: String name for the graph on which to plot the data.
    series_name: String name for the series (line on the graph) associated with
                 the data.  This is also the string displayed on the waterfall
                 if |show_on_waterfall| is True.
    data_point: Numeric data value to plot on the graph for the current build.
                This can be a single value or an array of values.  If an array,
                the graph will plot the average of the values, along with error
                bars.
    units: The string unit of measurement for the given |data_point|.
    show_on_waterfall: Whether or not to display this result directly on the
                       buildbot waterfall itself (in the buildbot step running
                       this test on the waterfall page, not the stdio page).
  """
  waterfall_indicator = ['', '*'][show_on_waterfall]
  print '%sRESULT %s: %s= %s %s' % (
      waterfall_indicator, graph_name, series_name,
      str(data_point).replace(' ', ''), units)
  sys.stdout.flush()


def Shard(ilist, shard_index, num_shards):
  """Shard a given list and return the group at index |shard_index|.

  Args:
    ilist: input list
    shard_index: 0-based sharding index
    num_shards: shard count
  """
  chunk_size = len(ilist) / num_shards
  chunk_start = shard_index * chunk_size
  if shard_index == num_shards - 1:  # Exhaust the remainder in the last shard.
    chunk_end = len(ilist)
  else:
    chunk_end = chunk_start + chunk_size
  return ilist[chunk_start:chunk_end]


def WaitForDomElement(pyauto, driver, xpath):
  """Wait for the UI element to appear.

  Args:
    pyauto: an instance of pyauto.PyUITest.
    driver: an instance of chrome driver or a web element.
    xpath: the xpath of the element to wait for.

  Returns:
    The element if it is found.
    NoSuchElementException if it is not found.
  """
  pyauto.WaitUntil(lambda: len(driver.find_elements_by_xpath(xpath)) > 0)
  return driver.find_element_by_xpath(xpath)


def DoesUrlExist(url):
  """Determines whether a resource exists at the given URL.

  Args:
    url: URL to be verified.

  Returns:
    True if url exists, otherwise False.
  """
  parsed = urlparse.urlparse(url)
  try:
    conn = httplib.HTTPConnection(parsed.netloc)
    conn.request('HEAD', parsed.path)
    response = conn.getresponse()
  except (socket.gaierror, socket.error):
    return False
  finally:
    conn.close()
  # Follow both permanent (301) and temporary (302) redirects.
  if response.status == 302 or response.status == 301:
    return DoesUrlExist(response.getheader('location'))
  return response.status == 200


class _GTestTextTestResult(unittest._TextTestResult):
  """A test result class that can print formatted text results to a stream.

  Results printed in conformance with gtest output format, like:
  [ RUN        ] autofill.AutofillTest.testAutofillInvalid: "test desc."
  [         OK ] autofill.AutofillTest.testAutofillInvalid
  [ RUN        ] autofill.AutofillTest.testFillProfile: "test desc."
  [         OK ] autofill.AutofillTest.testFillProfile
  [ RUN        ] autofill.AutofillTest.testFillProfileCrazyCharacters: "Test."
  [         OK ] autofill.AutofillTest.testFillProfileCrazyCharacters
  """

  def __init__(self, stream, descriptions, verbosity):
    unittest._TextTestResult.__init__(self, stream, descriptions, verbosity)

  def _GetTestURI(self, test):
    if sys.version_info[:2] <= (2, 4):
      return '%s.%s' % (unittest._strclass(test.__class__),
                        test._TestCase__testMethodName)
    return '%s.%s' % (unittest._strclass(test.__class__), test._testMethodName)

  def getDescription(self, test):
    return '%s: "%s"' % (self._GetTestURI(test), test.shortDescription())

  def startTest(self, test):
    unittest.TestResult.startTest(self, test)
    self.stream.writeln('[ RUN        ] %s' % self.getDescription(test))

  def addSuccess(self, test):
    unittest.TestResult.addSuccess(self, test)
    self.stream.writeln('[         OK ] %s' % self._GetTestURI(test))

  def addError(self, test, err):
    unittest.TestResult.addError(self, test, err)
    self.stream.writeln('[      ERROR ] %s' % self._GetTestURI(test))

  def addFailure(self, test, err):
    unittest.TestResult.addFailure(self, test, err)
    self.stream.writeln('[     FAILED ] %s' % self._GetTestURI(test))


class GTestTextTestRunner(unittest.TextTestRunner):
  """Test Runner for displaying test results in textual format.

  Results are displayed in conformance with gtest output.
  """

  def __init__(self, verbosity=1):
    unittest.TextTestRunner.__init__(self, stream=sys.stderr,
                                     verbosity=verbosity)

  def _makeResult(self):
    return _GTestTextTestResult(self.stream, self.descriptions, self.verbosity)
