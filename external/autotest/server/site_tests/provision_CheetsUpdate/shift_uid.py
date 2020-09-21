#!/usr/bin/python
#
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
"""
Recursively shifts the UID/GIDs of the target directory for user namespacing.
"""

import argparse
import grp
import pwd
import os
import sys

def main():
  parser = argparse.ArgumentParser()
  parser.add_argument('target', help='recursively shifts UIDs/GIDs of target')
  args = parser.parse_args()

  for root, dirs, files in os.walk(args.target):
    for f in files:
      path = os.path.join(root, f)
      stat_info = os.lstat(path)
      if (stat_info.st_uid < 655360):
        os.lchown(path, stat_info.st_uid + 655360, stat_info.st_gid + 655360)
    for d in dirs:
      path = os.path.join(root, d)
      stat_info = os.lstat(path)
      if (stat_info.st_uid < 655360):
        os.lchown(path, stat_info.st_uid + 655360, stat_info.st_gid + 655360)

if __name__ == '__main__':
  main()
