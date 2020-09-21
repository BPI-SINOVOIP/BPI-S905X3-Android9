# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import test_webrtc_peer_connection


class video_WebRtcMainFeedSwitching(test.test):
    """
    Tests a simulated Video Call with one main and several small feeds.

    Simulates speaker switching by swapping the source video object between the
    main feed and one randomly chosen small feed.
    """
    version = 1

    def run_once(self, mode = 'functional'):
        """
        Runs the test.

        @param mode: 'functional' or 'performance' depending on desired mode.
        """
        kwargs = {
                'own_script': 'main-feed-switching.js',
                'common_script': 'loopback-peerconnection.js',
                'bindir': self.bindir,
                'tmpdir': self.tmpdir,
                'debugdir': self.debugdir,
                'num_peer_connections': 5,
                'iteration_delay_millis': 50
        }

        if mode == 'functional':
            test = test_webrtc_peer_connection.WebRtcPeerConnectionTest(
                    title = 'Main Feed Switching',
                    **kwargs)
            test.run_test()
        elif mode == 'performance':
            test = test_webrtc_peer_connection\
                    .WebRtcPeerConnectionPerformanceTest(
                            title = 'Main Feed Switching Performance Test',
                            **kwargs)
            test.run_test()
            test.collector.write_metrics(self.output_perf_value)
        else:
            raise error.TestError('mode must be "functional" or "performance"')

