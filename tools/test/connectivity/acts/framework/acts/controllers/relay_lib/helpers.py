#!/usr/bin/env python
#
#   Copyright 2016 - The Android Open Source Project
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
from acts.controllers.relay_lib.errors import RelayConfigError
from six import string_types

MISSING_KEY_ERR_MSG = 'key "%s" missing from %s. Offending object:\n %s'
TYPE_MISMATCH_ERR_MSG = 'Key "%s" is of type %s. Expecting %s.' \
                        ' Offending object:\n %s'


def validate_key(key, dictionary, expected_type, source):
    """Validates if a key exists and its value is the correct type.
    Args:
        key: The key in dictionary.
        dictionary: The dictionary that should contain key.
        expected_type: the type that key's value should have.
        source: The name of the object being checked. Used for error messages.

    Returns:
        The value of dictionary[key] if no error was raised.

    Raises:
        RelayConfigError if the key does not exist, or is not of expected_type.
    """
    if key not in dictionary:
        raise RelayConfigError(MISSING_KEY_ERR_MSG % (key, source, dictionary))
    if expected_type == str:
        if not isinstance(dictionary[key], string_types):
            raise RelayConfigError(TYPE_MISMATCH_ERR_MSG %
                                   (key, dictionary[key], expected_type,
                                    dictionary))
    elif not isinstance(dictionary[key], expected_type):
        raise RelayConfigError(TYPE_MISMATCH_ERR_MSG %
                               (key, dictionary[key], expected_type,
                                dictionary))
    return dictionary[key]
