#!/usr/bin/env python

import argparse
import logging
import os
import re
import shutil
from genmakefiles import GenerateMakefiles

AOSP_PATH = os.getcwd()
UNWANTED_DIRS = [
  'build/',
  'test/',
  'third_party/',
  'buildtools/',
  'tools/gyp',
  'tools/swarming_client'
]

parser = argparse.ArgumentParser(description='Merge a new version of V8.')
parser.add_argument('--v8_path', \
  required=True, help='Absolute path to the new version of V8 to be merged.')
args = parser.parse_args()

# Find all the files that are in both the old and the new repo; remove them
# (ie. leave the Android-only files in place)
new_files = [f for f in os.listdir(args.v8_path) if not f.startswith(".")]
current_files = [f for f in os.listdir(AOSP_PATH) if not f.startswith(".")]
to_remove = [f for f in current_files if f in new_files]

for file in to_remove:
  path = os.path.join(AOSP_PATH, file)
  if os.path.isfile(path):
    os.remove(path)
  else:
    shutil.rmtree(path)

# Copy accross the new files.
all_new_v8 = os.path.join(args.v8_path, "*")
rsync_cmd = "rsync -r --exclude=\"*.git*\" " + all_new_v8 + \
  " . > /dev/null"
if os.system(rsync_cmd) != 0:
  raise RuntimeError("Could not rsync the new V8.")

# Now remove extra stuff that AOSP doesn't want (to save space)
for file in UNWANTED_DIRS:
  path = os.path.join(AOSP_PATH, file)
  shutil.rmtree(path)

# Create new makefiles
GenerateMakefiles()

# Update V8_MERGE_REVISION
major_version = 0
minor_version = 0
build_number = 0
patch_level = 0

success = True
with open(os.path.join(AOSP_PATH, "include/v8-version.h"), 'r') as f:
  content = f.read()
  result = re.search("V8_MAJOR_VERSION (\\d*)", content)
  if result != None:
    major_version = result.group(1)
  else:
    success = False
  result = re.search("V8_MINOR_VERSION (\\d*)", content)
  if result != None:
    minor_version = result.group(1)
  else:
    success = False
  result = re.search("V8_BUILD_NUMBER (\\d*)", content)
  if result != None:
    build_number = result.group(1)
  else:
    success = False
  result = re.search("V8_PATCH_LEVEL (\\d*)", content)
  if result != None:
    patch_level = result.group(1)
  else:
    success = False

version_string = major_version + "." + \
  minor_version + "." + \
  build_number + "." + \
  patch_level

if not success:
  logging.warning("Couldn't extract V8 version to update V8_MERGE_REVISION." + \
     " Got " + version_string)
else:
  with open(os.path.join(AOSP_PATH, "V8_MERGE_REVISION"), 'w') as f:
    f.write("v8 " + version_string + "\n")
    f.write("https://chromium.googlesource.com/v8/v8/+/" + version_string)

  # Done!
  print "Merged V8 " + version_string + ". Test, commit and upload!"