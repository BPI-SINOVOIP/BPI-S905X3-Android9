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
Test finder base class.
"""
from collections import namedtuple


Finder = namedtuple('Finder', ['test_finder_instance', 'find_method'])


def find_method_register(cls):
    """Class decorater to find all registered find methods."""
    cls.find_methods = []
    cls.get_all_find_methods = lambda x: x.find_methods
    for methodname in dir(cls):
        method = getattr(cls, methodname)
        if hasattr(method, '_registered'):
            cls.find_methods.append(Finder(None, method))
    return cls


def register():
    """Decorator to register find methods."""

    def wrapper(func):
        """Wrapper for the register decorator."""
        #pylint: disable=protected-access
        func._registered = True
        return func
    return wrapper


# This doesn't really do anything since there are no find methods defined but
# it's here anyways as an example for other test type classes.
@find_method_register
class TestFinderBase(object):
    """Base class for test finder class."""

    def __init__(self, *args, **kwargs):
        pass
