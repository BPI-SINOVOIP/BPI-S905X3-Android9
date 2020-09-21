#!/usr/bin/env python2

# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for apache_log_metrics.py"""

from __future__ import print_function

import StringIO
import subprocess
import tempfile
import threading
import time
import unittest

import tail_until_writer_finished


class TestTailUntilWriterFinished(unittest.TestCase):
    """Tests tail_until_writer_finished."""

    def SkipIfMissingInotifyTools(self):
        """The tail_until_writer_finished module requires 'inotifywait'."""
        try:
          subprocess.call(['inotifywait'], stderr=subprocess.PIPE)
        except OSError:
          raise unittest.SkipTest('inotify-tools must be installed.')

    def testTail(self):
        """Tests reading a file from the end."""
        self.GetsEntireInput(seek_to_end=True)

    def testRead(self):
        """Tests reading a file from the beginning."""
        self.GetsEntireInput(seek_to_end=False)

    def GetsEntireInput(self, seek_to_end):
        """Tails a temp file in a thread.

        Check that it read the file correctly.

        @param seek_to_end: Whether to .seek to the end of the file before
            reading.
        """
        self.SkipIfMissingInotifyTools()

        f = tempfile.NamedTemporaryFile()
        output = StringIO.StringIO()

        f.write('This line will not get read if we seek to end.\n')
        f.flush()

        def Tail():
            """Tails the file into |output| with a 64k chunk size."""
            tail_until_writer_finished.TailFile(
                f.name, 0.1, 64000, outfile=output, seek_to_end=seek_to_end)

        thread = threading.Thread(target=Tail)
        thread.start()

        # There is a race here: the thread must start the inotify process before
        # we close the file. This shouldn't take long at all, so add a small
        # sleep.
        time.sleep(0.3)

        for i in range(100):
            f.write(str(i) + '\n')
            f.flush()
        f.close()
        thread.join()

        expected = ''.join([str(i) + '\n' for i in range(100)])
        if not seek_to_end:
            expected = ('This line will not get read if we seek to end.\n'
                        + expected)
        self.assertEqual(output.getvalue(), expected)


if __name__ == '__main__':
    unittest.main()
