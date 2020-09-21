#   Copyright 2016 - Google, Inc.
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


class DataWrapper(object):
    """A special type of packed data.

    This is the base class for a type of data that wraps around another set
    of data. This data will be unpacked on every read right before the real
    value is needed. When unpacked the wrapper can perform special operations
    to get the real data.
    """

    def get_real_value(self, unwrapper):
        """Unpacks the data from the wrapper.

        Args:
            unwrapper: The object calling the unwrapping.

        Returns:
            The real value stored in the data wrapper.
        """
        value = self.unwrap(unwrapper)
        while isinstance(value, DataWrapper):
            value = value.unwrap(unwrapper)

        return value

    def unwrap(self, unwrapper):
        """Unwraps the data from this wrapper and this wrapper only.

        Unlike get_real_value, this method is abstract and is where user
        code can be written to unpack the data value. If this returns
        another data wrapper then get_real_value will continue to unwrap
        until a true value is given.

        Args:
            unwrapper: The object calling the unwrapping.

        Returns:
            The unwrapped data.
        """
        pass


class ConfigWrapper(object):
    """Object used to asses config values.

    Base class for all objects that wrap around the configs loaded into the
    application.
    """

    def get_value(self, key):
        """Reads the value of a config at a certain key.

        Args:
            key: The key of the value to read.

        Returns:
            The raw value stored at that key.
        """
        raise NotImplementedError()

    def set_value(self, key, value):
        """Writes a new value into the config at a certain key.

        Args:
            key: The key to write the value at.
            value: The new value to store at that key.
        """
        raise NotImplementedError()

    def has_value(self, key):
        """Checks if this config has a certain key.

        Args:
            key: The key to check for.

        Returns:
            True if the key exists.
        """
        raise NotImplementedError()

    def keys(self):
        """Gets all keys in this config.

        Returns:
            An iterator (or some iterable object) with all keys in this config.
        """
        raise NotImplementedError()

    def __getitem__(self, key):
        """Overrides the bracket accessor to read a value.

        Unlike get_value, this will return the true value of a config. This
        will unpack any data wrappers.

        Args:
            key: The key to read.

        Returns:
            The true value stored at the key.
        """
        value = self.get_value(key)
        if isinstance(value, DataWrapper):
            return value.get_real_value(self)
        else:
            return value

    def __setitem__(self, key, value):
        """Overrides the bracket accessor to write a value.

        Args:
            key: The key to write to.
        """
        return self.set_value(key, value)

    def __iter__(self):
        """Iterates over all values in the config.

        Returns:
            An iterator of key, value pairs.
        """
        for key in self.keys():
            yield (key, self[key])
