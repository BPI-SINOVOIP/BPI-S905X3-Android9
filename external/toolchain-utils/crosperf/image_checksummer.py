# Copyright 2011 Google Inc. All Rights Reserved.
"""Compute image checksum."""

from __future__ import print_function

import os
import threading

from cros_utils import logger
from cros_utils.file_utils import FileUtils


class ImageChecksummer(object):
  """Compute image checksum."""

  class PerImageChecksummer(object):
    """Compute checksum for an image."""

    def __init__(self, label, log_level):
      self._lock = threading.Lock()
      self.label = label
      self._checksum = None
      self.log_level = log_level

    def Checksum(self):
      with self._lock:
        if not self._checksum:
          logger.GetLogger().LogOutput(
              "Acquiring checksum for '%s'." % self.label.name)
          self._checksum = None
          if self.label.image_type != 'local':
            raise RuntimeError('Called Checksum on non-local image!')
          if self.label.chromeos_image:
            if os.path.exists(self.label.chromeos_image):
              self._checksum = FileUtils().Md5File(
                  self.label.chromeos_image, log_level=self.log_level)
              logger.GetLogger().LogOutput('Computed checksum is '
                                           ': %s' % self._checksum)
          if not self._checksum:
            raise RuntimeError('Checksum computing error.')
          logger.GetLogger().LogOutput('Checksum is: %s' % self._checksum)
        return self._checksum

  _instance = None
  _lock = threading.Lock()
  _per_image_checksummers = {}

  def __new__(cls, *args, **kwargs):
    with cls._lock:
      if not cls._instance:
        cls._instance = super(ImageChecksummer, cls).__new__(
            cls, *args, **kwargs)
      return cls._instance

  def Checksum(self, label, log_level):
    if label.image_type != 'local':
      raise RuntimeError('Attempt to call Checksum on non-local image.')
    with self._lock:
      if label.name not in self._per_image_checksummers:
        self._per_image_checksummers[label.name] = (
            ImageChecksummer.PerImageChecksummer(label, log_level))
      checksummer = self._per_image_checksummers[label.name]

    try:
      return checksummer.Checksum()
    except:
      logger.GetLogger().LogError('Could not compute checksum of image in label'
                                  " '%s'." % label.name)
      raise
