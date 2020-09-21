#!/usr/bin/env bash
#
# Copyright 2016 - The Android Open Source Project
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
#

TOOLS_DIR=$(realpath $(dirname $0))
LTP_ROOT=$(realpath $TOOLS_DIR/../..)

if ! [ -f $LTP_ROOT/include/config.h ]; then
  echo "LTP has not been configured!"
  echo "Please run \"cd $LTP_ROOT; make autotools; ./configure\" first"
  exit 1
fi

OUTPUT=$TOOLS_DIR/make_dry_run.dump
echo "Dumping output to $OUTPUT from command 'make -C $LTP_ROOT/testcases --dry-run'"
make -C $LTP_ROOT/testcases --dry-run > $OUTPUT

OUTPUT=$TOOLS_DIR/make_install_dry_run.dump
echo "Dumping output to $OUTPUT from command 'make -C $LTP_ROOT/testcases install --dry-run'"
make -C $LTP_ROOT/testcases install --dry-run > $OUTPUT

echo "Distclean $LTP_ROOT ..."
make -C $LTP_ROOT distclean

echo "Finished!"
