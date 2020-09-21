#
# Copyright (C) 2017 The Android Open Source Project
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


class ApiMessage(object):
    """The class for the messages defined by TFC API."""

    def __init__(self, all_keys, **kwargs):
        """Initializes the attributes.

        Args:
            all_keys: A collection of attribute names.
            **kwargs: The names and values of the attributes. The names must be
                      in all_keys.

        Raises:
            KeyError if any key in kwargs is not in all_keys.
        """
        for key, value in kwargs.iteritems():
            if key not in all_keys:
                raise KeyError(key)
            setattr(self, key, value)

    def ToJson(self, keys):
        """Creates a JSON object containing the specified keys.

        Args:
            keys: The attributes to be included in the object.

        Returns:
            A JSON object.
        """
        return dict((x, getattr(self, x)) for x in keys if hasattr(self, x))
