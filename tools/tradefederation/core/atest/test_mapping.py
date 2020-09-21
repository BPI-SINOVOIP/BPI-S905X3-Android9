# Copyright 2018, The Android Open Source Project
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

"""
Classes for test mapping related objects
"""


import copy


class TestDetail(object):
    """Stores the test details set in a TEST_MAPPING file."""

    def __init__(self, details):
        """TestDetail constructor

        Parse test detail from a dictionary, e.g.,
        {
          "name": "SettingsUnitTests",
          "options": [
            {
              "instrumentation-arg":
                  "annotation=android.platform.test.annotations.Presubmit"
            }
          ]
        }

        Args:
            details: A dictionary of test detail.
        """
        self.name = details['name']
        self.options = []
        options = details.get('options', [])
        for option in options:
            assert len(option) == 1, 'Each option can only have one key.'
            self.options.append(copy.deepcopy(option).popitem())
        self.options.sort(key=lambda o: o[0])

    def __str__(self):
        """String value of the TestDetail object."""
        if not self.options:
            return self.name
        options = ''
        for option in self.options:
            options += '%s: %s, ' % option

        return '%s (%s)' % (self.name, options.strip(', '))

    def __hash__(self):
        """Get the hash of TestDetail based on the details"""
        return hash(str(self))

    def __eq__(self, other):
        return str(self) == str(other)
