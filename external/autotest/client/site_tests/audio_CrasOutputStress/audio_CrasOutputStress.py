# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import random
import re
import subprocess
import time

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error


class audio_CrasOutputStress(test.test):
    """Checks if output buffer will drift to super high level."""
    version = 1
    _MAX_OUTPUT_STREAMS = 3
    _LOOP_COUNT = 300
    _OUTPUT_BUFFER_LEVEL = '.*?SET_DEV_WAKE.*?hw_level.*?(\d+).*?'
    _BUFFER_DRIFT_CRITERIA = 4096

    def run_once(self):
        """
        Repeatedly add output streams of random configurations and
        remove them to verify if output buffer level would drift.
        """
        self._output_streams = []
        self._rates = ['48000', '44100']
        self._block_sizes = ['512', '1024']

        loop_count = 0
        while loop_count < self._LOOP_COUNT:
            if len(self._output_streams) < self._MAX_OUTPUT_STREAMS:
                cmd = ['cras_test_client', '--playback_file', '/dev/zero',
                       '--rate', self._rates[random.randint(0, 1)],
                       '--block_size', self._block_sizes[random.randint(0, 1)]]
                proc = subprocess.Popen(cmd)
                self._output_streams.append(proc)
                time.sleep(0.01)
            else:
                self._output_streams[0].kill()
                self._output_streams.remove(self._output_streams[0])
                time.sleep(0.1)
            loop_count += 1

        # Get the buffer level.
        buffer_level = self._get_buffer_level()

        # Clean up all streams.
        while len(self._output_streams) > 0:
            self._output_streams[0].kill()
            self._output_streams.remove(self._output_streams[0])

        if buffer_level > self._BUFFER_DRIFT_CRITERIA:
            raise error.TestFail('Buffer level %d drift too high', buffer_level)

    def _get_buffer_level(self):
        """Gets a rough number about current buffer level.

        @returns: The current buffer level.

        """
        proc = subprocess.Popen(['cras_test_client', '--dump_a'], stdout=subprocess.PIPE)
        output, err = proc.communicate()
        buffer_level = 0
        for line in output.split('\n'):
            search = re.match(self._OUTPUT_BUFFER_LEVEL, line)
            if search:
                tmp = int(search.group(1))
                if tmp > buffer_level:
                    buffer_level = tmp
        return buffer_level

