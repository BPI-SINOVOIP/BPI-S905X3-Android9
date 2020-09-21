# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import test_webrtc_peer_connection


class webrtc_PausePlayPeerConnections(test.test):
    """Tests many peerconnections randomly paused and played."""
    version = 1

    def run_once(self, mode = 'functional', element_type='video'):
        """
        Runs the test.

        @param mode: 'functional' or 'performance' depending on desired mode.
        @param element_type: the element type to use for feeds, video or audio.
        """
        kwargs = {
            'own_script': 'pause-play.js',
            'common_script': 'loopback-peerconnection.js',
            'bindir': self.bindir,
            'tmpdir': self.tmpdir,
            'debugdir': self.debugdir,
            'num_peer_connections': 10,
            'iteration_delay_millis': 20,
            'before_start_hook': lambda tab: tab.EvaluateJavaScript(
                    "elementType = '{}'".format(element_type))
        }

        if mode == 'functional':
            test = test_webrtc_peer_connection.WebRtcPeerConnectionTest(
                    title = 'Pause Play Peerconnections',
                    **kwargs)
            test.run_test()
        elif mode == 'performance':
            test = test_webrtc_peer_connection\
                    .WebRtcPeerConnectionPerformanceTest(
                            title = 'Pause Play Peerconnections '
                                    + 'Performance test',
                            **kwargs)
            test.run_test()
            test.collector.write_metrics(self.output_perf_value)
        else:
            raise error.TestError('mode must be "functional" or "performance"')

