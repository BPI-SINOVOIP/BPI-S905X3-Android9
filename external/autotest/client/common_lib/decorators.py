# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import functools
import logging


def in_context(context_name):
    """
    Call a method in the context of member variable 'context_name.'

    You can use this like:
    class Foo(object):
        def __init__(self):
            self._mutex = threading.RLock()

        @in_context('_mutex')
        def bar(self):
            # Two threads can call Foo.bar safely without
            # any other synchronization.
            print 'Locked up tight.'

        def contextless_bar(self):
            with self._mutex:
                print 'Locked up tight.'

    With the in_context decorator, self.bar is equivalent to
    self.contextless_bar.  You can use this this to declare synchronized
    methods in the style of Java.  Similar to other locking methods, this
    can land you in deadlock in a hurry if you're not aware of what you're
    doing.

    @param context_name string name of the context manager to look up in self.

    """
    def wrap(func):
        """
        This function will get called with the instance method pulled off
        of self.  It does not get the self object though, so we wrap yet
        another nested function.

        @param func Function object that we'll eventually call.

        """
        @functools.wraps(func)
        def wrapped_manager(self, *args, **kwargs):
            """ Do the actual work of acquiring the context.

            We need this layer of indirection so that we can get at self.
            We use functools.wraps does some magic so that the function
            names and docs are set correctly on the wrapped function.

            """
            context = getattr(self, context_name)
            with context:
                return func(self, *args, **kwargs)
        return wrapped_manager
    return wrap


class _CachedProperty(object):


    def __init__(self, func, name=None):
        self._func = func
        self._name = name if name is not None else func.__name__

    def __get__(self, instance, owner):
        value = self._func(instance)
        setattr(instance, self._name, value)
        return value


def cached_property(func):
    """
    A read-only property that is only run the first time the attribute is
    accessed, and then the result is saved and returned on each future
    reference.

    @param func: The function to calculate the property value.
    @returns: An object that abides by the descriptor protocol.
    """
    return _CachedProperty(func)


def test_module_available(module, raise_error=False):
    """A decorator to test if the given module is available first before
    calling a function.

    @param module: Module object. The value should be None if the module is
                   failed to be imported.
    @param raise_error: If true an import error will be raised on call if module
                        is not imported.
    """

    def decorator(f):
        """The actual decorator.

        @param f: The function to call.

        @return: The function to call based on the value of `module`
        """

        def dummy_func(*args, **kargs):
            """A dummy function silently pass."""
            logging.debug('Module %s is not found. Call %s is skipped.', module,
                          f)
            if raise_error:
                raise ImportError('Module %s is not found.' % module)

        return f if module else dummy_func

    return decorator
