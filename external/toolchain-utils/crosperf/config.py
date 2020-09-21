# Copyright 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""A configure file."""
config = {}


def GetConfig(key):
  return config.get(key)


def AddConfig(key, value):
  config[key] = value
