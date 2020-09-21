#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2017, ARM Limited, Google, and contributors.
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may
# not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

if [ "x$TARGET_PRODUCT" == "x" ]; then
	echo "WARNING: Its recommended to launch from android build"
	echo "environment to take advantage of product/device-specific"
	echo "functionality."
else
	lisadir="$(gettop)/$(get_build_var BOARD_LISA_TARGET_SCRIPTS)"

	if [ -d $lisadir/targetdev ]; then
		export PYTHONPATH=$lisadir:$PYTHONPATH
		echo "Welcome to LISA $TARGET_PRODUCT environment"
		echo "Target-specific scripts are located in $lisadir"
	else
		echo "LISA scripts don't exist for $TARGET_PRODUCT, skipping"
	fi
fi

if [ -z  "$CATAPULT_HOME" ]; then
        export CATAPULT_HOME=$LISA_HOME/../chromium-trace/catapult/
        echo "Systrace will run from: $(readlink -f $CATAPULT_HOME)"
fi

monsoon_path="$LISA_HOME/../../cts/tools/utils/"
export PATH="$monsoon_path:$PATH"
echo "Monsoon will run from: $(readlink -f $monsoon_path/monsoon.py)"

export PYTHONPATH=$LISA_HOME/../devlib:$PYTHONPATH
export PYTHONPATH=$LISA_HOME/../trappy:$PYTHONPATH
export PYTHONPATH=$LISA_HOME/../bart:$PYTHONPATH
export DEVICE_LISA_HOME=$lisadir
