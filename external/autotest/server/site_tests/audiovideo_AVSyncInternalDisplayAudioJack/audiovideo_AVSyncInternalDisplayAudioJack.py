# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import tempfile

from autotest_lib.client.common_lib import file_utils
from autotest_lib.client.cros.chameleon import avsync_probe_utils
from autotest_lib.server import test
from autotest_lib.server.cros.multimedia import remote_facade_factory


class audiovideo_AVSyncInternalDisplayAudioJack(test.test):
    """Server side audio/video sync quality measurement.

    This test measure the audio/video sync between internal display and audio
    jack.

    """
    version = 1

    def run_once(self, host, video_url, capture_seconds, video_fps,
                 sound_interval_frames, perf_prefix):
        """Running audio/video synchronization quality measurement

        @param host: A host object representing the DUT.
        @param video_url: The ULR of the test video.
        @param capture_seconds: How long do we capture the data from
                avsync_probe device.
        @param video_fps: Video frames per second of the video. We need the
                data to detect corrupted video frame.
        @param sound_interval_frames: The period of sound (beep) in the number
                of video frames.
        @param perf_prefix: The prefix name of perf graph.

        """
        factory = remote_facade_factory.RemoteFacadeFactory(
                host, results_dir=self.resultsdir, no_chrome=True)

        chameleon_board = host.chameleon
        audio_facade = factory.create_audio_facade()
        browser_facade = factory.create_browser_facade()
        video_facade = factory.create_video_facade()
        avsync_probe = chameleon_board.get_avsync_probe()
        chameleon_board.setup_and_reset(self.outputdir)

        browser_facade.start_default_chrome()

        _, ext = os.path.splitext(video_url)
        with tempfile.NamedTemporaryFile(prefix='playback_', suffix=ext) as f:
            # The default permission is 0o600.
            os.chmod(f.name, 0o644)

            file_utils.download_file(video_url, f.name)
            video_facade.prepare_playback(f.name)

        audio_facade.set_chrome_active_volume(100)
        video_facade.start_playback()
        capture_data = avsync_probe.Capture(capture_seconds)
        parser = avsync_probe_utils.AVSyncProbeDataParser(
                self.resultsdir, capture_data, video_fps, sound_interval_frames)

        logging.info('Video frame stats:')
        logging.info('average: %f', parser.video_duration_average)
        logging.info('standard deviation: %f', parser.video_duration_std)
        logging.info('Sync stats:')
        logging.info('average: %f', parser.sync_duration_average)
        logging.info('standard deviation: %f', parser.sync_duration_std)
        logging.info('Number of total frames: %d',
                     parser.cumulative_frame_count)
        logging.info('Number of corrupted frames: %d',
                     parser.corrupted_frame_count)
        logging.info('Number of dropoped frames: %d',
                     parser.dropped_frame_count)
        logging.info('Number of dropoped frames by player: %d',
                     video_facade.dropped_frame_count())

        video_graph_name = '%s_video' % perf_prefix
        audiovideo_graph_name = '%s_audiovideo' % perf_prefix
        self.output_perf_value(description='Video frame duration average',
                               value=parser.video_duration_average, units='ms',
                               graph=video_graph_name)
        self.output_perf_value(description='Video frame duration std',
                               value=parser.video_duration_std,
                               graph=video_graph_name)
        self.output_perf_value(description='Corrupted video frames',
                               value=parser.corrupted_frame_count,
                               higher_is_better=False, graph=video_graph_name)
        self.output_perf_value(description='Dropped video frames',
                               value=parser.dropped_frame_count,
                               higher_is_better=False, graph=video_graph_name)
        self.output_perf_value(description='Dropped video frames by player',
                               value=video_facade.dropped_frame_count(),
                               higher_is_better=False, graph=video_graph_name)

        self.output_perf_value(description='Audio/Video Sync duration average',
                               value=parser.sync_duration_average, units='ms',
                               higher_is_better=False,
                               graph=audiovideo_graph_name)
        self.output_perf_value(description='Audio/Video Sync std',
                               value=parser.sync_duration_std,
                               higher_is_better=False,
                               graph=audiovideo_graph_name)
