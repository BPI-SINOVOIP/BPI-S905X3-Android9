# pylint: disable=missing-docstring
# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

def any_call(*args, **kwargs):
    """An empty method to handle any call.
    """
    pass


def decorate(f):
    """A noop decorator.
    """
    return f


def decorate_wrapper(f):
    """Wrapper of the noop decorator.

    Calling this method with any args will return the noop decorator function.
    """
    return decorate


class mock_class_type(type):
    """Type class for the mock class to handle any class methods."""

    def __getattr__(self, attr):
        # This is to support decorators like "@metrics.SecondsTimerDecorator"
        # In this case, the call returns a function which returns a noop
        # decorator function ("decorate").
        if 'Decorator' in attr:
            return decorate_wrapper
        else:
            return mock_class_base


class mock_class_base(object):
    """Base class for a mock es class."""

    __metaclass__ = mock_class_type

    def __init__(self, *args, **kwargs):
        pass


    def __getattribute__(self, name):

        # TODO(dshi): Remove this method after all reference of timer.get_client
        # is removed.
        def get_client(*args, **kwargs):
            return self

        # get_client is to support call like "timer.get_client", which returns
        # a class supporting Context when being called.
        if name == 'get_client':
            return get_client
        elif name == 'indices':
            return mock_class_base()

        return any_call


    def __enter__(self, *args, **kwargs):
        """Method to support Context class."""
        return self


    def __exit__(self, *args, **kwargs):
        """Method to support Context class."""


    def __getitem__(self, key):
        """Method to override __getitem__."""
        return self


    def __setitem__(self, key, value):
        """Method to override __setitem__."""
