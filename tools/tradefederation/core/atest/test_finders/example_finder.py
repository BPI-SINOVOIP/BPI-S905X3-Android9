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
Example Finder class.
"""

# pylint: disable=import-error
import test_info
import test_finder_base
from test_runners import example_test_runner


@test_finder_base.find_method_register
class ExampleFinder(test_finder_base.TestFinderBase):
    """Example finder class."""
    NAME = 'EXAMPLE'
    _TEST_RUNNER = example_test_runner.ExampleTestRunner.NAME

    @test_finder_base.register()
    def find_method_from_example_finder(self, test):
        """Example find method to demonstrate how to register it."""
        if test == 'ExampleFinderTest':
            return test_info.TestInfo(test_name=test,
                                      test_runner=self._TEST_RUNNER,
                                      build_targets=set())
        return None
