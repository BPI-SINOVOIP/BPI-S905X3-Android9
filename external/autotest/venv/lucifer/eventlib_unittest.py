# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import pytest

from lucifer import eventlib
from lucifer.eventlib import Event


def test_run_event_command_normal(capfd):
    """Test happy path."""
    handler = _FakeHandler()

    ret = eventlib.run_event_command(
            event_handler=handler,
            args=['bash', '-c',
                  'echo starting;'
                  'echo log message >&2;'
                  'echo completed;'])

    # Handler should be called with events in order.
    assert handler.events == [(Event('starting'), ''), (Event('completed'), '')]
    # Handler should return the exit status of the command.
    assert ret == 0
    # Child stderr should go to stderr.
    out, err = capfd.readouterr()
    assert out == ''
    assert err == 'log message\n'


def test_run_event_command_normal_with_messages():
    """Test happy path with messages."""
    handler = _FakeHandler()

    ret = eventlib.run_event_command(
            event_handler=handler,
            args=['bash', '-c', 'echo starting foo'])

    # Handler should be called with events and messages.
    assert handler.events == [(Event('starting'), 'foo')]
    # Handler should return the exit status of the command.
    assert ret == 0


def test_run_event_command_with_invalid_events():
    """Test passing invalid events."""
    handler = _FakeHandler()
    eventlib.run_event_command(
            event_handler=handler,
            args=['bash', '-c', 'echo foo; echo bar'])
    # Handler should not be called with invalid events.
    assert handler.events == []


def test_run_event_command_with_failed_command():
    """Test passing invalid events."""
    handler = _FakeHandler()
    ret = eventlib.run_event_command(
            event_handler=handler,
            args=['bash', '-c', 'exit 1'])
    # Handler should return the exit status of the command.
    assert ret == 1


def test_run_event_command_should_not_hide_handler_exception():
    """Test handler exceptions."""
    handler = _RaisingHandler(_FakeError)
    with pytest.raises(_FakeError):
        eventlib.run_event_command(
                event_handler=handler,
                args=['bash', '-c', 'echo starting; echo completed'])


class _FakeHandler(object):
    """Event handler for testing; stores events."""

    def __init__(self):
        self.events = []

    def __call__(self, event, msg):
        self.events.append((event, msg))


class _RaisingHandler(object):
    """Event handler for testing; raises."""

    def __init__(self, exception):
        self._exception = exception

    def __call__(self, event, msg):
        raise self._exception


class _FakeError(Exception):
    """Fake exception for tests."""
