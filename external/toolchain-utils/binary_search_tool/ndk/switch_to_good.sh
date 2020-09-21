#!/bin/bash -u
#
# Copyright 2016 Google Inc. All Rights Reserved.
#
# This script is intended to be used by binary_search_state.py, as
# part of the binary search triage on Android NDK apps. This script simply
# deletes all given objects, signaling gradle to execute a recompilation of said
# object files.
#

# Input is a file, with newline seperated list of files we will be switching
OBJ_LIST_FILE=$1

# Check that number of arguments == 1
if [ $# -ne 1 ] ; then
  echo "ERROR:"
  echo "Got multiple inputs to switch script!"
  echo "Run binary_search_state.py with --file_args"
  exit 1
fi

# Remove any file that's being switched. This is because Gradle only recompiles
# if:
#   1. The resultant object file doesn't exist
#   2. The hash of the source file has changed
#
# Because we have no reliable way to edit the source file, we instead remove the
# object file and have the compiler wrapper insert the file from the appropriate
# cache (good or bad).
#
# Not entirely relevant to the Teapot example, but something to consider:
# This removing strategy has the side effect that all switched items cause the
# invocation of the compiler wrapper, which can add up and slow the build
# process. With Android's source tree, Make checks the timestamp of the object
# file. So we symlink in the appropriate file and touch it to tell Make it needs
# to be relinked. This avoids having to call the compiler wrapper in the
# majority of cases.
#
# However, a similar construct doesn't seem to exist in Gradle. It may be better
# to add a build target to Gradle that will always relink all given object
# files. This way we can avoid calling the compiler wrapper while Triaging and
# save some time. Not really necessary

cat $OBJ_LIST_FILE | xargs rm
exit 0

