# Copyright (C) 2017 The Android Open-Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""A helper class for working with stored ARC build manifest files"""

import logging
import os

import lib.util as util


class BuildArtifactFetcher(object):
  """Common code for working with ARC/Android build artifacts"""

  def __init__(self, arch, variant, build_id):
    self._arch = arch
    self._variant = variant
    self._build_id = build_id

  @property
  def arch(self):
    return self._arch

  @property
  def variant(self):
    return self._variant

  @property
  def build_id(self):
    return self._build_id

  def fetch(self, remote_path, local_path):
    logging.info('Fetching %s ...', os.path.basename(local_path))
    util.makedirs(os.path.dirname(local_path))
    util.check_call(
        '/google/data/ro/projects/android/fetch_artifact',
        '--bid', self._build_id,
        '--target', 'cheets_%s-%s' % (self._arch, self._variant),
        remote_path, local_path)
