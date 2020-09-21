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

from acts.libs.config import base


class ConfigDataHandler(object):
    """An object that handles formatting config data.

    An object that handles formatting a specific slot of data stored in
    the configs.

    Attributes:
        owner: The config object that owns this data handler.
        key: The key of the config value this hander manages. This can change
             between calls.
    """

    def __init__(self):
        self.owner = None
        self.key = None

    def get_value(self):
        """Formats the value stored at key from the inner config.

        Returns:
            The formated value.
        """
        return self.get_inner()

    def set_value(self, value):
        """Unformatts the passed in data and stores it in the inner config.

        Args:
            values: The value to unformat.
        """
        return self.set_inner(value)

    def get_inner(self):
        """Gets the unpacked value stored in the inner config.

        Returns:
            The unpacked value stored in the inner config.
        """
        value = self.owner.inner.get_value(self.key)
        if isinstance(value, base.DataWrapper):
            return value.get_real_value(self)
        else:
            return value

    def set_inner(self, key, value):
        """Stores a value in the inner config."""
        return self.owner.inner.set_value(self.key, value)


class ConfigSchema(base.ConfigWrapper):
    """A config data wrapper that handles formatting config data.

    A config data wrapper that can take another config object and manage
    formatting it into the real type of data that is stored at that value.
    All class attributes of type ConfigDataHandler will become data handlers
    for configs of that field name. The only exception to this is
    DEFAULT_HANDLER, which is used as the data hander for all keys that
    do not have a data handler.

    Attributes:
        DEFAULT_HANDLER: The data handler to use when no hander is found.
        inner: The config wrapper this schema is wrapping around.
    """
    DEFAULT_HANDLER = ConfigDataHandler()

    def __init__(self, inner):
        """
        Args:
            inner: The config wrapper to format.
        """
        self.inner = inner
        self._data_handlers = {}

        d = type(self).__dict__
        for k, v in d.items():
            if isinstance(v, ConfigDataHandler):
                self._data_handlers[k] = v
                v.owner = self
                v.key = k

    def get_value(self, key):
        """Gets the value after being formatted by a data handler.

        Args:
            key: The key of the data to get.

        Returns:
            The formatted data.
        """
        handler = self._data_handlers.get(key)
        if handler:
            return handler.get_value()
        else:
            self.DEFAULT_HANDLER.key = key
            return self.DEFAULT_HANDLER.get_value()

    def set_value(self, key, value):
        """Unformats a value and sets it in the inner config."""
        handler = self._data_handlers.get(key)
        if handler:
            return handler.set_value(value)
        else:
            self.DEFAULT_HANDLER.key = key
            return self.DEFAULT_HANDLER.set_value(value)

    def has_value(self, key):
        """Checks if the inner config contains a key."""
        return self.inner.has_value(key)

    def keys(self):
        """Returns the keys of the inner config."""
        return self.inner.keys()

    def __getattr__(self, key):
        """Overrides not found attributes to allow method calling to inner.

        Will allow calling a method on all data handlers and the inner config.
        """

        def inner_call(*args, **kwargs):
            for key in self.keys():
                handler = self._data_handlers.get(key)
                if not handler:
                    handler = self.DEFAULT_HANDLER
                    handler.key = key
                inner_func = getattr(handler, key, None)
                if callable(inner_func):
                    inner_func(*args, **kwargs)

            if self.inner:
                inner_func = getattr(self.inner, key, None)
                if callable(inner_func):
                    inner_func(*args, **kwargs)

        # Only return a callable func if there is an inner callable func
        for k, v in self._data_handlers.items():
            inner_func = getattr(v, key, None)
            if callable(inner_func):
                return inner_call

        if self.inner:
            inner_func = getattr(self.inner, key, None)
            if callable(inner_func):
                return inner_call

        return None
