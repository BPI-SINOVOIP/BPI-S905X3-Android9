#!/bin/bash
#
# Copyright 2011 Google Inc. All Rights Reserved.
# Author: kbaclawski@google.com (Krystian Baclawski)
#

export PYTHONPATH="$(pwd)"

python dejagnu/main.py $@
