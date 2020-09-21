#!/bin/bash
#
# Copyright 2015 Google Inc. All rights reserved
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http:#www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set -e

mk="$@"

cat <<EOF >Makefile
all: a b

a b:
	touch A
	echo 1 >>A
d: a
c: .KATI_IMPLICIT_OUTPUTS := d
c:
	touch C
	echo 1 >>C

c d: b
EOF

${mk} -j1 all d c
if [ -e ninja.sh ]; then ./ninja.sh -j1 -w dupbuild=err all d; fi

echo "A:"
cat A
echo "C":
cat C
