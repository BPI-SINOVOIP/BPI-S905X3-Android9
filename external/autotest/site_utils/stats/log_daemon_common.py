from __future__ import print_function

import sys
from logging import handlers

import common

from chromite.lib import cros_logging as logging
from infra_libs import ts_mon


def RunMatchers(stream, matchers):
    """Parses lines of |stream| using patterns and emitters from |matchers|

    @param stream: A file object to read from.
    @param matchers: A list of pairs of (matcher, emitter), where matcher is a
        regex and emitter is a function called when the regex matches.
    """
    # The input might terminate if the log gets rotated. Make sure that Monarch
    # flushes any pending metrics before quitting.
    try:
        for line in iter(stream.readline, ''):
            for matcher, emitter in matchers:
                m = matcher.match(line)
                if m:
                    emitter(m)
    finally:
        ts_mon.close()
        ts_mon.flush()


def SetupLogging(args):
    """Sets up logging based on the parsed arguments."""
    # Set up logging.
    root = logging.getLogger()
    if args.output_logfile:
        handler = handlers.RotatingFileHandler(
            args.output_logfile, maxBytes=10**6, backupCount=5)
        root.addHandler(handler)
    else:
        root.addHandler(logging.StreamHandler(sys.stdout))
    root.setLevel(logging.DEBUG)
