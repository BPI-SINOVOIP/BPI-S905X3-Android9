#!/bin/bash -u
#
# Copyright 2016 Google Inc. All Rights Reserved.
#
# This script is intended to be used by binary_search_state.py, as
# part of the binary search triage on the Android NDK apps.  This script
# generates the list of object files to be bisected. This list is generated
# by the compiler wrapper during the POPULATE_GOOD and POPULATE_BAD stages.
#

cat ${BISECT_DIR}/good/_LIST

