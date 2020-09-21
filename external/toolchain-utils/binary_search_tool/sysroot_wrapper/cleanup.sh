#!/bin/bash
#
# Copyright 2016 Google Inc. All Rights Reserved.
#
# This script is part of the ChromeOS object binary search triage process.
# It should be the last script called by the user, after the user has
# successfully run the bisection tool and found their bad items. This script
# will perform all necessary cleanup for the bisection tool.
#

rm common/common.sh
