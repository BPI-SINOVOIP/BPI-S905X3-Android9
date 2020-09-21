#!/usr/bin/env python
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

import argparse
from IPython.nbformat import v3, v4

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("input", help="input python file")
    parser.add_argument("output", help="output notebook file")
    args = parser.parse_args()

with open(args.input) as fpin:
    text = fpin.read()

nbook = v3.reads_py(text)
nbook = v4.upgrade(nbook)  # Upgrade v3 to v4

jsonform = v4.writes(nbook) + "\n"
with open(args.output, "w") as fpout:
    fpout.write(jsonform)


