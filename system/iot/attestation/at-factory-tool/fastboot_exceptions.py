# !/usr/bin/python
# Copyright 2017 The Android Open Source Project
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

"""Exceptions for At-Factory-Tool Manager (atftman)."""


class DeviceNotFoundException(Exception):

  def __init__(self):
    Exception.__init__(self)
    self.msg = 'Device Not Found!'

  def SetMsg(self, msg):
    self.msg = msg

  def __str__(self):
    return self.msg


class NoAlgorithmAvailableException(Exception):
  pass


class FastbootFailure(Exception):

  def __init__(self, msg):
    Exception.__init__(self)
    self.msg = msg

  def __str__(self):
    return self.msg


class ProductNotSpecifiedException(Exception):

  def __str__(self):
    return 'Product Attribute File Not Selected!'


class ProductAttributesFileFormatError(Exception):

  def __init__(self, msg):
    Exception.__init__(self)
    self.msg = msg

  def __str__(self):
    return self.msg
