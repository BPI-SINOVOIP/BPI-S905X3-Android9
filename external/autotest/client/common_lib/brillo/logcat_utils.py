# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import collections
import re

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import utils

LogcatLine = collections.namedtuple('LogcatLine', ['pid', 'tag', 'message'])

def wait_for_logcat_log(message_tag, message_pattern,
                        process_id=None, timeout_seconds=30, host=None):
    """Wait for a line to show up in logcat.

    @param message_tag: string "tag" of the line, as understood by logcat.
    @param message_pattern: regular expression pattern that describes the
            entire text of the message to look for (e.g. '.*' matches all
            messages).  This is in grep's regex language.
    @param process_id: optional integer process id to match on.
    @param timeout_seconds: number of seconds to wait for the log line.
    @param host: host object to look for the log line on.  Defaults to
            our local host.

    """
    run = host.run if host is not None else utils.run

    process_id_option = ''
    if process_id is not None:
        process_id_option = '--pid %d' % process_id

    result = run('logcat %s --format=process -e %s -m 1' % (
                         process_id_option, message_pattern),
                 timeout=timeout_seconds,
                 ignore_timeout=True)
    if result is None:
        raise error.TestFail('Timed out waiting for a log with message "%s"' %
                             message_pattern)

    lines = result.stdout.strip().splitlines()

    if len(lines) == 0:
        raise error.TestError('Logcat did not return any lines')

    line = lines[-1]

    match = re.match(r'^.\( *(\d+)\) (.*) \(([^(]+)\)$', line)
    if match:
        return LogcatLine(pid=match.group(1),
                          message=match.group(2),
                          tag=match.group(3))
    raise error.TestError('Failed to match logcat line "%s"' % line)
