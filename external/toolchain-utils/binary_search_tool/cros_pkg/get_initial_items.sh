#!/bin/bash -u
#
# Copyright 2015 Google Inc. All Rights Reserved.
#
# This script is intended to be used by binary_search_state.py, as
# part of the binary search triage on ChromeOS packages.  This script
# generates the list of current ChromeOS packages, that is then used
# for doing the binary search.
#

source common/common.sh

cd ${GOOD_BUILD}/packages
find . -name "*.tbz2"


