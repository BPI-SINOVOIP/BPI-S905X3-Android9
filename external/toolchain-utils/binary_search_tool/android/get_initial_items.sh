#!/bin/bash -u
#
# Copyright 2016 Google Inc. All Rights Reserved.
#
# This script is intended to be used by binary_search_state.py, as
# part of the binary search triage on the Android source tree.  This script
# generates the list of current Android object files, that is then used
# for doing the binary search.
#

source android/common.sh

cat ${BISECT_GOOD_BUILD}/_LIST

