#!/usr/bin/env python3.4
#
# Copyright 2017 - The Android Open Source Project
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

from acts import signals


def test_info(predicate=None, **keyed_info):
    """Adds info about test.

    Extra Info to include about the test. This info will be available in the
    test output. Note that if a key is given multiple times it will be added
    as a list of all values. If multiples of these are stacked there results
    will be merged.

    Example:
        # This test will have a variable my_var
        @test_info(my_var='THIS IS MY TEST')
        def my_test(self):
            return False

    Args:
        predicate: A func to call that if false will skip adding this test
                   info. Function signature is bool(test_obj, args, kwargs)
        **keyed_info: The key, value info to include in the extras for this
                      test.
    """

    def test_info_decoractor(func):
        return _TestInfoDecoratorFunc(func, predicate, keyed_info)

    return test_info_decoractor


def test_tracker_info(uuid, extra_environment_info=None, predicate=None):
    """Decorator for adding test tracker info to tests results.

    Will add test tracker info inside of Extras/test_tracker_info.

    Example:
        # This test will be linked to test tracker uuid abcd
        @test_tracker_info(uuid='abcd')
        def my_test(self):
            return False

    Args:
        uuid: The uuid of the test case in test tracker.
        extra_environment_info: Extra info about the test tracker environment.
        predicate: A func that if false when called will ignore this info.
    """
    return test_info(
        test_tracker_uuid=uuid,
        test_tracker_enviroment_info=extra_environment_info,
        predicate=predicate)


class _TestInfoDecoratorFunc(object):
    """Object that acts as a function decorator test info."""

    def __init__(self, func, predicate, keyed_info):
        self.func = func
        self.predicate = predicate
        self.keyed_info = keyed_info
        self.__name__ = func.__name__

    def __get__(self, instance, owner):
        """Called by Python to create a binding for an instance closure.

        When called by Python this object will create a special binding for
        that instance. That binding will know how to interact with this
        specific decorator.
        """
        return _TestInfoBinding(self, instance)

    def __call__(self, *args, **kwargs):
        """
        When called runs the underlying func and then attaches test info
        to a signal.
        """
        try:
            result = self.func(*args, **kwargs)

            if result or result is None:
                new_signal = signals.TestPass('')
            else:
                new_signal = signals.TestFailure('')
        except signals.TestSignal as signal:
            new_signal = signal

        if not isinstance(new_signal.extras, dict) and new_signal.extras:
            raise ValueError('test_info can only append to signal data '
                             'that has a dict as the extra value.')
        elif not new_signal.extras:
            new_signal.extras = {}

        gathered_extras = self._gather_local_info(None, *args, **kwargs)
        for k, v in gathered_extras.items():
            if k not in new_signal.extras:
                new_signal.extras[k] = v
            else:
                if not isinstance(new_signal.extras[k], list):
                    new_signal.extras[k] = [new_signal.extras[k]]

                new_signal.extras[k].insert(0, v)

        raise new_signal

    def gather(self, *args, **kwargs):
        """
        Gathers the info from this decorator without invoking the underlying
        function. This will also gather all child info if the underlying func
        has that ability.

        Returns: A dictionary of info.
        """
        if hasattr(self.func, 'gather'):
            extras = self.func.gather(*args, **kwargs)
        else:
            extras = {}

        self._gather_local_info(extras, *args, **kwargs)

        return extras

    def _gather_local_info(self, gather_into, *args, **kwargs):
        """Gathers info from this decorator and ignores children.

        Args:
            gather_into: Gathers into a dictionary that already exists.

        Returns: The dictionary with gathered info in it.
        """
        if gather_into is None:
            extras = {}
        else:
            extras = gather_into
        if not self.predicate or self.predicate(args, kwargs):
            for k, v in self.keyed_info.items():
                if v and k not in extras:
                    extras[k] = v
                elif v and k in extras:
                    if not isinstance(extras[k], list):
                        extras[k] = [extras[k]]
                    extras[k].insert(0, v)

        return extras


class _TestInfoBinding(object):
    """
    When Python creates an instance of an object it creates a binding object
    for each closure that contains what the instance variable should be when
    called. This object is a similar binding for _TestInfoDecoratorFunc.
    When Python tries to create a binding of a _TestInfoDecoratorFunc it
    will return one of these objects to hold the instance for that closure.
    """

    def __init__(self, target, instance):
        """
        Args:
            target: The target for creating a binding to.
            instance: The instance to bind the target with.
        """
        self.target = target
        self.instance = instance
        self.__name__ = target.__name__

    def __call__(self, *args, **kwargs):
        """
        When this object is called it will call the target with the bound
        instance.
        """
        return self.target(self.instance, *args, **kwargs)

    def gather(self, *args, **kwargs):
        """
        Will gather the target with the bound instance.
        """
        return self.target.gather(self.instance, *args, **kwargs)
