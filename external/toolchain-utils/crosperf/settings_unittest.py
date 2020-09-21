#!/usr/bin/env python2
#
# Copyright 2014 Google Inc.  All Rights Reserved.
"""unittest for settings."""

from __future__ import print_function

import mock
import unittest

import settings
import settings_factory

from field import IntegerField
from field import ListField
import download_images

from cros_utils import logger


class TestSettings(unittest.TestCase):
  """setting test class."""

  def setUp(self):
    self.settings = settings.Settings('global_name', 'global')

  def test_init(self):
    self.assertEqual(self.settings.name, 'global_name')
    self.assertEqual(self.settings.settings_type, 'global')
    self.assertIsNone(self.settings.parent)

  def test_set_parent_settings(self):
    self.assertIsNone(self.settings.parent)
    settings_parent = {'fake_parent_entry': 0}
    self.settings.SetParentSettings(settings_parent)
    self.assertIsNotNone(self.settings.parent)
    self.assertEqual(type(self.settings.parent), dict)
    self.assertEqual(self.settings.parent, settings_parent)

  def test_add_field(self):
    self.assertEqual(self.settings.fields, {})
    self.settings.AddField(
        IntegerField(
            'iterations',
            default=1,
            required=False,
            description='Number of iterations to '
            'run the test.'))
    self.assertEqual(len(self.settings.fields), 1)
    # Adding the same field twice raises an exception.
    self.assertRaises(Exception, self.settings.AddField, (IntegerField(
        'iterations',
        default=1,
        required=False,
        description='Number of iterations to run '
        'the test.')))
    res = self.settings.fields['iterations']
    self.assertIsInstance(res, IntegerField)
    self.assertEqual(res.Get(), 1)

  def test_set_field(self):
    self.assertEqual(self.settings.fields, {})
    self.settings.AddField(
        IntegerField(
            'iterations',
            default=1,
            required=False,
            description='Number of iterations to run the '
            'test.'))
    res = self.settings.fields['iterations']
    self.assertEqual(res.Get(), 1)

    self.settings.SetField('iterations', 10)
    res = self.settings.fields['iterations']
    self.assertEqual(res.Get(), 10)

    # Setting a field that's not there raises an exception.
    self.assertRaises(Exception, self.settings.SetField, 'remote',
                      'lumpy1.cros')

    self.settings.AddField(
        ListField(
            'remote',
            default=[],
            description="A comma-separated list of ip's of "
            'chromeos devices to run '
            'experiments on.'))
    self.assertEqual(type(self.settings.fields), dict)
    self.assertEqual(len(self.settings.fields), 2)
    res = self.settings.fields['remote']
    self.assertEqual(res.Get(), [])
    self.settings.SetField('remote', 'lumpy1.cros', append=True)
    self.settings.SetField('remote', 'lumpy2.cros', append=True)
    res = self.settings.fields['remote']
    self.assertEqual(res.Get(), ['lumpy1.cros', 'lumpy2.cros'])

  def test_get_field(self):
    # Getting a field that's not there raises an exception.
    self.assertRaises(Exception, self.settings.GetField, 'iterations')

    # Getting a required field that hasn't been assigned raises an exception.
    self.settings.AddField(
        IntegerField(
            'iterations',
            required=True,
            description='Number of iterations to '
            'run the test.'))
    self.assertIsNotNone(self.settings.fields['iterations'])
    self.assertRaises(Exception, self.settings.GetField, 'iterations')

    # Set the value, then get it.
    self.settings.SetField('iterations', 5)
    res = self.settings.GetField('iterations')
    self.assertEqual(res, 5)

  def test_inherit(self):
    parent_settings = settings_factory.SettingsFactory().GetSettings(
        'global', 'global')
    label_settings = settings_factory.SettingsFactory().GetSettings(
        'label', 'label')
    self.assertEqual(parent_settings.GetField('chromeos_root'), '')
    self.assertEqual(label_settings.GetField('chromeos_root'), '')
    self.assertIsNone(label_settings.parent)

    parent_settings.SetField('chromeos_root', '/tmp/chromeos')
    label_settings.SetParentSettings(parent_settings)
    self.assertEqual(parent_settings.GetField('chromeos_root'), '/tmp/chromeos')
    self.assertEqual(label_settings.GetField('chromeos_root'), '')
    label_settings.Inherit()
    self.assertEqual(label_settings.GetField('chromeos_root'), '/tmp/chromeos')

  def test_override(self):
    self.settings.AddField(
        ListField(
            'email',
            default=[],
            description='Space-seperated'
            'list of email addresses to send '
            'email to.'))

    global_settings = settings_factory.SettingsFactory().GetSettings(
        'global', 'global')

    global_settings.SetField('email', 'john.doe@google.com', append=True)
    global_settings.SetField('email', 'jane.smith@google.com', append=True)

    res = self.settings.GetField('email')
    self.assertEqual(res, [])

    self.settings.Override(global_settings)
    res = self.settings.GetField('email')
    self.assertEqual(res, ['john.doe@google.com', 'jane.smith@google.com'])

  def test_validate(self):

    self.settings.AddField(
        IntegerField(
            'iterations',
            required=True,
            description='Number of iterations '
            'to run the test.'))
    self.settings.AddField(
        ListField(
            'remote',
            default=[],
            required=True,
            description='A comma-separated list '
            "of ip's of chromeos "
            'devices to run experiments on.'))
    self.settings.AddField(
        ListField(
            'email',
            default=[],
            description='Space-seperated'
            'list of email addresses to '
            'send email to.'))

    # 'required' fields have not been assigned; should raise an exception.
    self.assertRaises(Exception, self.settings.Validate)
    self.settings.SetField('iterations', 2)
    self.settings.SetField('remote', 'x86-alex.cros', append=True)
    # Should run without exception now.
    self.settings.Validate()

  @mock.patch.object(logger, 'GetLogger')
  @mock.patch.object(download_images.ImageDownloader, 'Run')
  @mock.patch.object(download_images, 'ImageDownloader')
  def test_get_xbuddy_path(self, mock_downloader, mock_run, mock_logger):

    mock_run.return_value = 'fake_xbuddy_translation'
    mock_downloader.Run = mock_run
    board = 'lumpy'
    chromeos_root = '/tmp/chromeos'
    log_level = 'average'

    trybot_str = 'trybot-lumpy-paladin/R34-5417.0.0-b1506'
    official_str = 'lumpy-release/R34-5417.0.0'
    xbuddy_str = 'latest-dev'
    autotest_path = ''

    self.settings.GetXbuddyPath(trybot_str, autotest_path, board, chromeos_root,
                                log_level)
    self.assertEqual(mock_run.call_count, 1)
    self.assertEqual(mock_run.call_args_list[0][0],
                     ('/tmp/chromeos',
                      'remote/trybot-lumpy-paladin/R34-5417.0.0-b1506', ''))

    mock_run.reset_mock()
    self.settings.GetXbuddyPath(official_str, autotest_path, board,
                                chromeos_root, log_level)
    self.assertEqual(mock_run.call_count, 1)
    self.assertEqual(mock_run.call_args_list[0][0],
                     ('/tmp/chromeos', 'remote/lumpy-release/R34-5417.0.0', ''))

    mock_run.reset_mock()
    self.settings.GetXbuddyPath(xbuddy_str, autotest_path, board, chromeos_root,
                                log_level)
    self.assertEqual(mock_run.call_count, 1)
    self.assertEqual(mock_run.call_args_list[0][0],
                     ('/tmp/chromeos', 'remote/lumpy/latest-dev', ''))

    if mock_logger:
      return


if __name__ == '__main__':
  unittest.main()
