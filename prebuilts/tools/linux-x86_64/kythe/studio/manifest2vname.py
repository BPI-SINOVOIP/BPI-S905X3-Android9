#!/usr/bin/python

#
# Copyright (C) 2015 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

"""Tool to generate vnames.json file for kythe from Android repo
manifest. Needs to be run from repo root."""

import xml.etree.ElementTree
import json

doc = xml.etree.ElementTree.parse('.repo/manifests/default.xml')
manifest = doc.getroot()
# fallback patterns to be added to tail of vnames.json
tail = json.loads("""[
  {
    "pattern": "bazel-out/[^/]+/(.*)",
    "vname": {
      "corpus": "googleplex-android",
      "root": "GENERATED/studio/bazel",
      "path": "@1@"
    }
  },
  {
    "pattern": "(.*)",
    "vname": {
      "corpus": "googleplex-android",
      "path": "@1@"
    }
  }
]
""")

vnames = []
# manifest xml contains <project path="" name=""> tags
# we convert these into vname patterns that kythe understands.
for project in manifest.findall('project'):
    node = {}
    node['pattern'] = "%s/(.*)" % project.get('path')
    vname = {}
    vname['corpus'] = 'googleplex-android'
    vname['root'] = project.get('name')
    vname['path'] = '@1@'
    node['vname'] = vname
    vnames.append(node);

# add fallback vname patterns to end of json list.
vnames.extend(tail)

# print the json vnames list to stdout
# can be used for debugging or pipe to vnames.json.
print(json.dumps(vnames, indent=2, separators=(',', ':')))
