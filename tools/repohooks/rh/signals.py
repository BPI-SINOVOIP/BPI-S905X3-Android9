# -*- coding:utf-8 -*-
# Copyright 2016 The Android Open Source Project
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

"""Signal related functionality."""

from __future__ import print_function

import os
import signal
import sys

_path = os.path.realpath(__file__ + '/../..')
if sys.path[0] != _path:
    sys.path.insert(0, _path)
del _path


def relay_signal(handler, signum, frame):
    """Notify a listener returned from getsignal of receipt of a signal.

    Returns:
      True if it was relayed to the target, False otherwise.
      False in particular occurs if the target isn't relayable.
    """
    if handler in (None, signal.SIG_IGN):
        return True
    elif handler == signal.SIG_DFL:
        # This scenario is a fairly painful to handle fully, thus we just
        # state we couldn't handle it and leave it to client code.
        return False
    handler(signum, frame)
    return True


def signal_module_usable(_signal=signal.signal, _SIGUSR1=signal.SIGUSR1):
    """Verify that the signal module is usable and won't segfault on us.

    See http://bugs.python.org/issue14173.  This function detects if the
    signals module is no longer safe to use (which only occurs during
    final stages of the interpreter shutdown) and heads off a segfault
    if signal.* was accessed.

    This shouldn't be used by anything other than functionality that is
    known and unavoidably invoked by finalizer code during python shutdown.

    Finally, the default args here are intentionally binding what we need
    from the signal module to do the necessary test; invoking code shouldn't
    pass any options, nor should any developer ever remove those default
    options.

    Note that this functionality is intended to be removed just as soon
    as all consuming code installs their own SIGTERM handlers.
    """
    # Track any signals we receive while doing the check.
    received, actual = [], None
    def handler(signum, frame):
        received.append([signum, frame])
    try:
        # Play with sigusr1, since it's not particularly used.
        actual = _signal(_SIGUSR1, handler)
        _signal(_SIGUSR1, actual)
        return True
    except (TypeError, AttributeError, SystemError, ValueError):
        # The first three exceptions can be thrown depending on the state of the
        # signal module internal Handlers array; we catch all, and interpret it
        # as if we were invoked during sys.exit cleanup.
        # The last exception can be thrown if we're trying to be used in a
        # thread which is not the main one.  This can come up with standard
        # python modules such as BaseHTTPServer.HTTPServer.
        return False
    finally:
        # And now relay those signals to the original handler.  Not all may
        # be delivered- the first may throw an exception for example.  Not our
        # problem however.
        for signum, frame in received:
            actual(signum, frame)
