#!/usr/bin/env python2
#
# Copyright 2014 Google Inc.  All Rights Reserved
"""Download image unittest."""

from __future__ import print_function

import os
import mock
import unittest

import download_images
from cros_utils import command_executer
from cros_utils import logger

import test_flag

MOCK_LOGGER = logger.GetLogger(log_dir='', mock=True)


class ImageDownloaderTestcast(unittest.TestCase):
  """The image downloader test class."""

  def __init__(self, *args, **kwargs):
    super(ImageDownloaderTestcast, self).__init__(*args, **kwargs)
    self.called_download_image = False
    self.called_uncompress_image = False
    self.called_get_build_id = False
    self.called_download_autotest_files = False

  @mock.patch.object(os, 'makedirs')
  @mock.patch.object(os.path, 'exists')
  def test_download_image(self, mock_path_exists, mock_mkdirs):

    # Set mock and test values.
    mock_cmd_exec = mock.Mock(spec=command_executer.CommandExecuter)
    test_chroot = '/usr/local/home/chromeos'
    test_build_id = 'lumpy-release/R36-5814.0.0'
    image_path = ('gs://chromeos-image-archive/%s/chromiumos_test_image.tar.xz'
                  % test_build_id)

    downloader = download_images.ImageDownloader(
        logger_to_use=MOCK_LOGGER, cmd_exec=mock_cmd_exec)

    # Set os.path.exists to always return False and run downloader
    mock_path_exists.return_value = False
    test_flag.SetTestMode(True)
    self.assertRaises(download_images.MissingImage, downloader.DownloadImage,
                      test_chroot, test_build_id, image_path)

    # Verify os.path.exists was called twice, with proper arguments.
    self.assertEqual(mock_path_exists.call_count, 2)
    mock_path_exists.assert_called_with(
        '/usr/local/home/chromeos/chroot/tmp/lumpy-release/'
        'R36-5814.0.0/chromiumos_test_image.bin')
    mock_path_exists.assert_any_call(
        '/usr/local/home/chromeos/chroot/tmp/lumpy-release/R36-5814.0.0')

    # Verify we called os.mkdirs
    self.assertEqual(mock_mkdirs.call_count, 1)
    mock_mkdirs.assert_called_with(
        '/usr/local/home/chromeos/chroot/tmp/lumpy-release/R36-5814.0.0')

    # Verify we called RunCommand once, with proper arguments.
    self.assertEqual(mock_cmd_exec.RunCommand.call_count, 1)
    expected_args = (
        '/usr/local/home/chromeos/chromium/tools/depot_tools/gsutil.py '
        'cp gs://chromeos-image-archive/lumpy-release/R36-5814.0.0/'
        'chromiumos_test_image.tar.xz '
        '/usr/local/home/chromeos/chroot/tmp/lumpy-release/R36-5814.0.0')

    mock_cmd_exec.RunCommand.assert_called_with(expected_args)

    # Reset the velues in the mocks; set os.path.exists to always return True.
    mock_path_exists.reset_mock()
    mock_cmd_exec.reset_mock()
    mock_path_exists.return_value = True

    # Run downloader
    downloader.DownloadImage(test_chroot, test_build_id, image_path)

    # Verify os.path.exists was called twice, with proper arguments.
    self.assertEqual(mock_path_exists.call_count, 2)
    mock_path_exists.assert_called_with(
        '/usr/local/home/chromeos/chroot/tmp/lumpy-release/'
        'R36-5814.0.0/chromiumos_test_image.bin')
    mock_path_exists.assert_any_call(
        '/usr/local/home/chromeos/chroot/tmp/lumpy-release/R36-5814.0.0')

    # Verify we made no RunCommand or ChrootRunCommand calls (since
    # os.path.exists returned True, there was no work do be done).
    self.assertEqual(mock_cmd_exec.RunCommand.call_count, 0)
    self.assertEqual(mock_cmd_exec.ChrootRunCommand.call_count, 0)

  @mock.patch.object(os.path, 'exists')
  def test_uncompress_image(self, mock_path_exists):

    # set mock and test values.
    mock_cmd_exec = mock.Mock(spec=command_executer.CommandExecuter)
    test_chroot = '/usr/local/home/chromeos'
    test_build_id = 'lumpy-release/R36-5814.0.0'

    downloader = download_images.ImageDownloader(
        logger_to_use=MOCK_LOGGER, cmd_exec=mock_cmd_exec)

    # Set os.path.exists to always return False and run uncompress.
    mock_path_exists.return_value = False
    self.assertRaises(download_images.MissingImage, downloader.UncompressImage,
                      test_chroot, test_build_id)

    # Verify os.path.exists was called once, with correct arguments.
    self.assertEqual(mock_path_exists.call_count, 1)
    mock_path_exists.assert_called_with(
        '/usr/local/home/chromeos/chroot/tmp/lumpy-release/'
        'R36-5814.0.0/chromiumos_test_image.bin')

    # Verify RunCommand was called twice with correct arguments.
    self.assertEqual(mock_cmd_exec.RunCommand.call_count, 2)
    # Call 1, should have 2 arguments
    self.assertEqual(len(mock_cmd_exec.RunCommand.call_args_list[0]), 2)
    actual_arg = mock_cmd_exec.RunCommand.call_args_list[0][0]
    expected_arg = (
        'cd /usr/local/home/chromeos/chroot/tmp/lumpy-release/R36-5814.0.0 ; '
        'tar -Jxf chromiumos_test_image.tar.xz ',)
    self.assertEqual(expected_arg, actual_arg)
    # 2nd arg must be exception handler
    except_handler_string = 'RunCommandExceptionHandler.HandleException'
    self.assertTrue(
        except_handler_string in repr(
            mock_cmd_exec.RunCommand.call_args_list[0][1]))

    # Call 2, should have 2 arguments
    self.assertEqual(len(mock_cmd_exec.RunCommand.call_args_list[1]), 2)
    actual_arg = mock_cmd_exec.RunCommand.call_args_list[1][0]
    expected_arg = (
        'cd /usr/local/home/chromeos/chroot/tmp/lumpy-release/R36-5814.0.0 ; '
        'rm -f chromiumos_test_image.bin ',)
    self.assertEqual(expected_arg, actual_arg)
    # 2nd arg must be empty
    self.assertTrue('{}' in repr(mock_cmd_exec.RunCommand.call_args_list[1][1]))

    # Set os.path.exists to always return True and run uncompress.
    mock_path_exists.reset_mock()
    mock_cmd_exec.reset_mock()
    mock_path_exists.return_value = True
    downloader.UncompressImage(test_chroot, test_build_id)

    # Verify os.path.exists was called once, with correct arguments.
    self.assertEqual(mock_path_exists.call_count, 1)
    mock_path_exists.assert_called_with(
        '/usr/local/home/chromeos/chroot/tmp/lumpy-release/'
        'R36-5814.0.0/chromiumos_test_image.bin')

    # Verify RunCommand was not called.
    self.assertEqual(mock_cmd_exec.RunCommand.call_count, 0)

  def test_run(self):

    # Set test arguments
    test_chroot = '/usr/local/home/chromeos'
    test_build_id = 'remote/lumpy/latest-dev'
    test_empty_autotest_path = ''
    test_autotest_path = '/tmp/autotest'

    # Set values to test/check.
    self.called_download_image = False
    self.called_uncompress_image = False
    self.called_get_build_id = False
    self.called_download_autotest_files = False

    # Define fake stub functions for Run to call
    def FakeGetBuildID(unused_root, unused_xbuddy_label):
      self.called_get_build_id = True
      return 'lumpy-release/R36-5814.0.0'

    def GoodDownloadImage(root, build_id, image_path):
      if root or build_id or image_path:
        pass
      self.called_download_image = True
      return 'chromiumos_test_image.bin'

    def BadDownloadImage(root, build_id, image_path):
      if root or build_id or image_path:
        pass
      self.called_download_image = True
      raise download_images.MissingImage('Could not download image')

    def FakeUncompressImage(root, build_id):
      if root or build_id:
        pass
      self.called_uncompress_image = True
      return 0

    def FakeDownloadAutotestFiles(root, build_id):
      if root or build_id:
        pass
      self.called_download_autotest_files = True
      return 'autotest'

    # Initialize downloader
    downloader = download_images.ImageDownloader(logger_to_use=MOCK_LOGGER)

    # Set downloader to call fake stubs.
    downloader.GetBuildID = FakeGetBuildID
    downloader.UncompressImage = FakeUncompressImage
    downloader.DownloadImage = GoodDownloadImage
    downloader.DownloadAutotestFiles = FakeDownloadAutotestFiles

    # Call Run.
    image_path, autotest_path = downloader.Run(test_chroot, test_build_id,
                                               test_empty_autotest_path)

    # Make sure it called both _DownloadImage and _UncompressImage
    self.assertTrue(self.called_download_image)
    self.assertTrue(self.called_uncompress_image)
    # Make sure it called DownloadAutotestFiles
    self.assertTrue(self.called_download_autotest_files)
    # Make sure it returned an image and  autotest path returned from this call
    self.assertTrue(image_path == 'chromiumos_test_image.bin')
    self.assertTrue(autotest_path == 'autotest')

    # Call Run with a non-empty autotest path
    self.called_download_autotest_files = False

    image_path, autotest_path = downloader.Run(test_chroot, test_build_id,
                                               test_autotest_path)

    # Verify that downloadAutotestFiles was not called
    self.assertFalse(self.called_download_autotest_files)
    # Make sure it returned the specified autotest path returned from this call
    self.assertTrue(autotest_path == test_autotest_path)

    # Reset values; Now use fake stub that simulates DownloadImage failing.
    self.called_download_image = False
    self.called_uncompress_image = False
    self.called_download_autotest_files = False
    downloader.DownloadImage = BadDownloadImage

    # Call Run again.
    self.assertRaises(download_images.MissingImage, downloader.Run, test_chroot,
                      test_autotest_path, test_build_id)

    # Verify that UncompressImage and downloadAutotestFiles were not called,
    # since _DownloadImage "failed"
    self.assertTrue(self.called_download_image)
    self.assertFalse(self.called_uncompress_image)
    self.assertFalse(self.called_download_autotest_files)


if __name__ == '__main__':
  unittest.main()
