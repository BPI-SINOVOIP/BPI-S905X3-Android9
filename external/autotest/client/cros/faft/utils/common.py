# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A common module for FAFT client."""

class LazyInitHandlerProxy:
    """Proxy of a given handler_class for lazy initialization."""
    _loaded = False
    _obj = None

    def __init__(self, handler_class, *args, **kargs):
        self._handler_class = handler_class
        self._args = args
        self._kargs = kargs

    def _load(self):
        self._obj = self._handler_class()
        self._obj.init(*self._args, **self._kargs)
        self._loaded = True

    def __getattr__(self, name):
        if not self._loaded:
            self._load()
        return getattr(self._obj, name)

    def reload(self):
        """Reload the handler class."""
        self._loaded = False
