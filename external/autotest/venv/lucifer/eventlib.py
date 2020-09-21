# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Event subprocess module.

Event subprocesses are subprocesses that print events to stdout.

Each event is one line of ASCII text with a terminating newline
character.  The event is identified with one of the preset strings in
Event.  The event string may be followed with a single space and a
message, on the same line.  The interpretation of the message is up to
the event handler.

run_event_command() starts such a process with a synchronous event
handler.
"""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import logging

import enum
import subprocess32
from subprocess32 import PIPE

logger = logging.getLogger(__name__)


class Event(enum.Enum):
    """Status change event enum

    Members of this enum represent all possible status change events
    that can be emitted by an event command and that need to be handled
    by the caller.

    The value of enum members must be a string, which is printed by
    itself on a line to signal the event.

    This should be backward compatible with all versions of
    lucifer_run_job, which lives in the infra/lucifer repository.

    TODO(crbug.com/748234): Events starting with X are temporary to
    support gradual lucifer rollout.

    https://chromium.googlesource.com/chromiumos/infra/lucifer
    """
    # Job status
    STARTING = 'starting'
    GATHERING = 'gathering'
    X_TESTS_DONE = 'x_tests_done'
    PARSING = 'parsing'
    COMPLETED = 'completed'

    # Host status
    HOST_READY = 'host_ready'
    HOST_NEEDS_CLEANUP = 'host_needs_cleanup'
    HOST_NEEDS_RESET = 'host_needs_reset'


def run_event_command(event_handler, args):
    """Run a command that emits events.

    Events printed by the command to stdout will be handled by
    event_handler synchronously.  Exceptions raised by event_handler
    will not be caught.  If an exception escapes, the child process's
    standard file descriptors are closed and the process is waited for.
    The event command should terminate if this happens.

    event_handler is called to handle each event.  Malformed events
    emitted by the command will be logged and discarded.  The
    event_handler should take two positional arguments: an Event
    instance and a message string.

    @param event_handler: event handler.
    @param args: passed to subprocess.Popen.
    @param returns: exit status of command.
    """
    logger.debug('Starting event command with %r', args)
    with subprocess32.Popen(args, stdout=PIPE) as proc:
        logger.debug('Event command child pid is %d', proc.pid)
        _handle_subprocess_events(event_handler, proc)
    logger.debug('Event command child with pid %d exited with %d',
                 proc.pid, proc.returncode)
    return proc.returncode


def _handle_subprocess_events(event_handler, proc):
    """Handle a subprocess that emits events.

    Events printed by the subprocess will be handled by event_handler.

    @param event_handler: callable that takes an Event instance.
    @param proc: Popen instance.
    """
    while True:
        logger.debug('Reading subprocess stdout')
        line = proc.stdout.readline()
        if not line:
            break
        _handle_output_line(event_handler, line)


def _handle_output_line(event_handler, line):
    """Handle a line of output from an event subprocess.

    @param event_handler: callable that takes a StatusChangeEvent.
    @param line: line of output.
    """
    event_str, _, message = line.rstrip().partition(' ')
    try:
        event = Event(event_str)
    except ValueError:
        logger.warning('Invalid output %r received', line)
        return
    event_handler(event, message)
