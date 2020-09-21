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

import json

from acts.libs.config import base


class ConfigDataSource(base.ConfigWrapper):
    """Base class for a config from a data source.

    A config wrapper that derives from this type reads directly from a
    data source such as a file on disk.
    """
    pass


class DictConfigDataSource(ConfigDataSource):
    """A config data source that reads from a dictionary."""

    def __init__(self, d={}):
        """Creates a new dictionary data source.

        Args:
            d: The dictionary to use.
        """
        self.dict = d

    def get_value(self, key):
        """Reads a value from the dictionary.

        Reads a value from the dictionary. Will throw a key error if the key
        does not exist.

        Args:
            key: The key in the dictionary to read from.

        Returns:
            The value stored at that key.
        """
        return self.dict[key]

    def set_value(self, key, value):
        """Writes a value to the dictionary.

        Args:
            key: The key to write to.
            value: The new value to store.
        """

        self.dict[key] = value

    def has_value(self, key):
        """Returns true if key is in dictionary."""
        return key in self.dict

    def keys(self):
        """Gives an iterator of all keys in the dictionary."""
        return iter(self.dict)


class JsonConfigDataSource(DictConfigDataSource):
    """A data source from a json file.

    Attributes:
        file_name: The name of the file this was loaded from.
    """

    def __init__(self, json_file):
        """
        Args:
            json_file: The path to the json file on disk.
        """
        self.file_name = json_file
        with open(json_file) as fd:
            self.dict = json.load(fd)


class MergedData(base.DataWrapper):
    """A data wrapper that wraps data from multiple data sources.

    Attributes:
        values: A list of values from different data sources sorted from
                lowest to highest priority.
        """

    def __init__(self, values):
        """
        Args:
            values: The list of values to set self.values to.
        """
        self.values = values

    def __str__(self):
        return 'MergedData: %s' % self.values

    def unwrap(self, unwrapper):
        """Unwraps the merged data.

        Will attempt to have the unwrapper handle unpacking the values. If
        no unpack func is found then the highest priority values is returned.
        """
        unpack_func = getattr(unwrapper, 'unpack', None)
        if callable(unpack_func):
            return unpack_func(self.values)
        else:
            return self.values[-1]


class MergeConfigDataSource(ConfigDataSource):
    """A data source created from multiple config wrappers."""

    def __init__(self, *configs):
        """
        Args:
            configs: The configs to create this merged source from.
        """
        self.configs = list(configs)
        # Have a shadow copy for changes made at runtime.
        self.configs.append(DictConfigDataSource())

    def get_value(self, key):
        """Gets the value from all configs and packs them into a MergedData.

        Returns:
            A MergedData of all values.
        """
        values = [c.get_value(key) for c in self.configs if c.has_value(key)]
        if values:
            return MergedData(values)

    def set_value(self, key, value):
        """Writes a new value to override the old values of this key.

        Writes the new value to a runtime shadow data store that has higher
        priority than the other data sources.
        """
        for c in reversed(self.configs):
            if c.has_value(key):
                c.set_value(key, value)
                return

        self.configs[-1].set_value(key, value)

    def has_value(self, key):
        """Checks if any of the configs have a value at the key."""
        return any(c.has_value(key) for c in self.configs)

    def keys(self):
        """Gives an iterator of the set of all keys in all configs.

        Gives an iterator over the set of all keys in all configs. Keys are
        given in no particular order.
        """
        given_keys = set()
        for config in self.configs:
            for key in config.keys():
                if not key in given_keys:
                    given_keys.add(key)
                    yield key

    def __getattr__(self, key):
        """Override of undefined attributes to foward messages to configs.

        If a message is found in any of the inner configs then a special
        function will be returned to allow forwarding the message to
        the inner configs.

        Args:
            key: The key of the undefined attribute.

        Returns:
            A function that can be called to send the message to all inner
            configs. If no inner config has the message then None is given
            instead.
        """

        def inner_call(*args, **kwargs):
            for config in self.configs:
                func = getattr(config, key, None)
                if callable(func):
                    func(*args, **kwargs)

        for config in self.configs:
            func = getattr(config, key, None)
            if callable(func):
                return inner_call

        return None
