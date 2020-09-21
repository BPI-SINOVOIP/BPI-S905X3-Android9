# Copyright (c) 2014-2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Download images from Cloud Storage."""

from __future__ import print_function

import ast
import os

import test_flag

from cros_utils import command_executer

GS_UTIL = 'chromium/tools/depot_tools/gsutil.py'


class MissingImage(Exception):
  """Raised when the requested image does not exist in gs://"""


class MissingFile(Exception):
  """Raised when the requested file does not exist in gs://"""


class RunCommandExceptionHandler(object):
  """Handle Exceptions from calls to RunCommand"""

  def __init__(self, logger_to_use, log_level, cmd_exec, command):
    self.logger = logger_to_use
    self.log_level = log_level
    self.ce = cmd_exec
    self.cleanup_command = command

  def HandleException(self, _, e):
    # Exception handler, Run specified command
    if self.log_level != 'verbose' and self.cleanup_command is not None:
      self.logger.LogOutput('CMD: %s' % self.cleanup_command)
    if self.cleanup_command is not None:
      _ = self.ce.RunCommand(self.cleanup_command)
    # Raise exception again
    raise e


class ImageDownloader(object):
  """Download images from Cloud Storage."""

  def __init__(self, logger_to_use=None, log_level='verbose', cmd_exec=None):
    self._logger = logger_to_use
    self.log_level = log_level
    self._ce = cmd_exec or command_executer.GetCommandExecuter(
        self._logger, log_level=self.log_level)

  def GetBuildID(self, chromeos_root, xbuddy_label):
    # Get the translation of the xbuddy_label into the real Google Storage
    # image name.
    command = ('cd ~/trunk/src/third_party/toolchain-utils/crosperf; '
               "python translate_xbuddy.py '%s'" % xbuddy_label)
    _, build_id_tuple_str, _ = self._ce.ChrootRunCommandWOutput(
        chromeos_root, command)
    if not build_id_tuple_str:
      raise MissingImage("Unable to find image for '%s'" % xbuddy_label)

    build_id_tuple = ast.literal_eval(build_id_tuple_str)
    build_id = build_id_tuple[0]

    return build_id

  def DownloadImage(self, chromeos_root, build_id, image_name):
    if self.log_level == 'average':
      self._logger.LogOutput('Preparing to download %s image to local '
                             'directory.' % build_id)

    # Make sure the directory for downloading the image exists.
    download_path = os.path.join(chromeos_root, 'chroot/tmp', build_id)
    image_path = os.path.join(download_path, 'chromiumos_test_image.bin')
    if not os.path.exists(download_path):
      os.makedirs(download_path)

    # Check to see if the image has already been downloaded.  If not,
    # download the image.
    if not os.path.exists(image_path):
      gsutil_cmd = os.path.join(chromeos_root, GS_UTIL)
      command = '%s cp %s %s' % (gsutil_cmd, image_name, download_path)

      if self.log_level != 'verbose':
        self._logger.LogOutput('CMD: %s' % command)
      status = self._ce.RunCommand(command)
      downloaded_image_name = os.path.join(download_path,
                                           'chromiumos_test_image.tar.xz')
      if status != 0 or not os.path.exists(downloaded_image_name):
        raise MissingImage('Cannot download image: %s.' % downloaded_image_name)

    return image_path

  def UncompressImage(self, chromeos_root, build_id):
    # Check to see if the file has already been uncompresssed, etc.
    if os.path.exists(
        os.path.join(chromeos_root, 'chroot/tmp', build_id,
                     'chromiumos_test_image.bin')):
      return

    # Uncompress and untar the downloaded image.
    download_path = os.path.join(chromeos_root, 'chroot/tmp', build_id)
    command = ('cd %s ; tar -Jxf chromiumos_test_image.tar.xz ' % download_path)
    # Cleanup command for exception handler
    clean_cmd = ('cd %s ; rm -f chromiumos_test_image.bin ' % download_path)
    exception_handler = RunCommandExceptionHandler(self._logger, self.log_level,
                                                   self._ce, clean_cmd)
    if self.log_level != 'verbose':
      self._logger.LogOutput('CMD: %s' % command)
      print('(Uncompressing and un-tarring may take a couple of minutes...'
            'please be patient.)')
    retval = self._ce.RunCommand(
        command, except_handler=exception_handler.HandleException)
    if retval != 0:
      if self.log_level != 'verbose':
        self._logger.LogOutput('CMD: %s' % clean_cmd)
        print('(Removing file chromiumos_test_image.bin.)')
      # Remove partially uncompressed file
      _ = self._ce.RunCommand(clean_cmd)
      # Raise exception for failure to uncompress
      raise MissingImage('Cannot uncompress image: %s.' % build_id)

    # Remove compressed image
    command = ('cd %s ; rm -f chromiumos_test_image.tar.xz; ' % download_path)
    if self.log_level != 'verbose':
      self._logger.LogOutput('CMD: %s' % command)
      print('(Removing file chromiumos_test_image.tar.xz.)')
    # try removing file, its ok to have an error, print if encountered
    retval = self._ce.RunCommand(command)
    if retval != 0:
      print('(Warning: Could not remove file chromiumos_test_image.tar.xz .)')

  def DownloadSingleAutotestFile(self, chromeos_root, build_id,
                                 package_file_name):
    # Verify if package files exist
    status = 0
    gs_package_name = ('gs://chromeos-image-archive/%s/%s' %
                       (build_id, package_file_name))
    gsutil_cmd = os.path.join(chromeos_root, GS_UTIL)
    if not test_flag.GetTestMode():
      cmd = '%s ls %s' % (gsutil_cmd, gs_package_name)
      status = self._ce.RunCommand(cmd)
    if status != 0:
      raise MissingFile(
          'Cannot find autotest package file: %s.' % package_file_name)

    if self.log_level == 'average':
      self._logger.LogOutput('Preparing to download %s package to local '
                             'directory.' % package_file_name)

    # Make sure the directory for downloading the package exists.
    download_path = os.path.join(chromeos_root, 'chroot/tmp', build_id)
    package_path = os.path.join(download_path, package_file_name)
    if not os.path.exists(download_path):
      os.makedirs(download_path)

    # Check to see if the package file has already been downloaded.  If not,
    # download it.
    if not os.path.exists(package_path):
      command = '%s cp %s %s' % (gsutil_cmd, gs_package_name, download_path)

      if self.log_level != 'verbose':
        self._logger.LogOutput('CMD: %s' % command)
      status = self._ce.RunCommand(command)
      if status != 0 or not os.path.exists(package_path):
        raise MissingFile('Cannot download package: %s .' % package_path)

  def UncompressSingleAutotestFile(self, chromeos_root, build_id,
                                   package_file_name, uncompress_cmd):
    # Uncompress file
    download_path = os.path.join(chromeos_root, 'chroot/tmp', build_id)
    command = ('cd %s ; %s %s' % (download_path, uncompress_cmd,
                                  package_file_name))

    if self.log_level != 'verbose':
      self._logger.LogOutput('CMD: %s' % command)
      print('(Uncompressing autotest file %s .)' % package_file_name)
    retval = self._ce.RunCommand(command)
    if retval != 0:
      raise MissingFile('Cannot uncompress file: %s.' % package_file_name)
    # Remove uncompressed downloaded file
    command = ('cd %s ; rm -f %s' % (download_path, package_file_name))
    if self.log_level != 'verbose':
      self._logger.LogOutput('CMD: %s' % command)
      print('(Removing processed autotest file %s .)' % package_file_name)
    # try removing file, its ok to have an error, print if encountered
    retval = self._ce.RunCommand(command)
    if retval != 0:
      print('(Warning: Could not remove file %s .)' % package_file_name)

  def VerifyAutotestFilesExist(self, chromeos_root, build_id, package_file):
    # Quickly verify if the files are there
    status = 0
    gs_package_name = ('gs://chromeos-image-archive/%s/%s' % (build_id,
                                                              package_file))
    gsutil_cmd = os.path.join(chromeos_root, GS_UTIL)
    if not test_flag.GetTestMode():
      cmd = '%s ls %s' % (gsutil_cmd, gs_package_name)
      if self.log_level != 'verbose':
        self._logger.LogOutput('CMD: %s' % cmd)
      status = self._ce.RunCommand(cmd)
      if status != 0:
        print('(Warning: Could not find file %s )' % gs_package_name)
        return 1
    # Package exists on server
    return 0

  def DownloadAutotestFiles(self, chromeos_root, build_id):
    # Download autest package files (3 files)
    autotest_packages_name = ('autotest_packages.tar')
    autotest_server_package_name = ('autotest_server_package.tar.bz2')
    autotest_control_files_name = ('control_files.tar')

    download_path = os.path.join(chromeos_root, 'chroot/tmp', build_id)
    # Autotest directory relative path wrt chroot
    autotest_rel_path = os.path.join('/tmp', build_id, 'autotest_files')
    # Absolute Path to download files
    autotest_path = os.path.join(chromeos_root, 'chroot/tmp', build_id,
                                 'autotest_files')

    if not os.path.exists(autotest_path):
      # Quickly verify if the files are present on server
      # If not, just exit with warning
      status = self.VerifyAutotestFilesExist(chromeos_root, build_id,
                                             autotest_packages_name)
      if status != 0:
        default_autotest_dir = '~/trunk/src/third_party/autotest/files'
        print(
            '(Warning: Could not find autotest packages .)\n'
            '(Warning: Defaulting autotest path to %s .' % default_autotest_dir)
        return default_autotest_dir

      # Files exist on server, download and uncompress them
      self.DownloadSingleAutotestFile(chromeos_root, build_id,
                                      autotest_packages_name)
      self.DownloadSingleAutotestFile(chromeos_root, build_id,
                                      autotest_server_package_name)
      self.DownloadSingleAutotestFile(chromeos_root, build_id,
                                      autotest_control_files_name)

      self.UncompressSingleAutotestFile(chromeos_root, build_id,
                                        autotest_packages_name, 'tar -xvf ')
      self.UncompressSingleAutotestFile(
          chromeos_root, build_id, autotest_server_package_name, 'tar -jxvf ')
      self.UncompressSingleAutotestFile(
          chromeos_root, build_id, autotest_control_files_name, 'tar -xvf ')
      # Rename created autotest directory to autotest_files
      command = ('cd %s ; mv autotest autotest_files' % download_path)
      if self.log_level != 'verbose':
        self._logger.LogOutput('CMD: %s' % command)
        print('(Moving downloaded autotest files to autotest_files)')
      retval = self._ce.RunCommand(command)
      if retval != 0:
        raise MissingFile('Could not create directory autotest_files')

    return autotest_rel_path

  def Run(self, chromeos_root, xbuddy_label, autotest_path):
    build_id = self.GetBuildID(chromeos_root, xbuddy_label)
    image_name = ('gs://chromeos-image-archive/%s/chromiumos_test_image.tar.xz'
                  % build_id)

    # Verify that image exists for build_id, before attempting to
    # download it.
    status = 0
    if not test_flag.GetTestMode():
      gsutil_cmd = os.path.join(chromeos_root, GS_UTIL)
      cmd = '%s ls %s' % (gsutil_cmd, image_name)
      status = self._ce.RunCommand(cmd)
    if status != 0:
      raise MissingImage('Cannot find official image: %s.' % image_name)

    image_path = self.DownloadImage(chromeos_root, build_id, image_name)
    self.UncompressImage(chromeos_root, build_id)

    if self.log_level != 'quiet':
      self._logger.LogOutput('Using image from %s.' % image_path)

    if autotest_path == '':
      autotest_path = self.DownloadAutotestFiles(chromeos_root, build_id)

    return image_path, autotest_path
