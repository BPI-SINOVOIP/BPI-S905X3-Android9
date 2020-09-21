#!/usr/bin/python -B

# Copyright 2017 The Android Open Source Project
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

"""Utility methods to work with Zip archives."""

import itertools

from operator import attrgetter
from zipfile import ZipFile


def ZipCompare(path_a, path_b):
  """Compares the contents of two Zip archives, returns True if equal."""

  with ZipFile(path_a, 'r') as zip_a:
    info_a = zip_a.infolist()

  with ZipFile(path_b, 'r') as zip_b:
    info_b = zip_b.infolist()

  if len(info_a) != len(info_b):
    return False

  info_a.sort(key=attrgetter('filename'))
  info_b.sort(key=attrgetter('filename'))

  return all(
      a.filename == b.filename and
      a.file_size == b.file_size and
      a.CRC == b.CRC
      for a, b in itertools.izip(info_a, info_b))
