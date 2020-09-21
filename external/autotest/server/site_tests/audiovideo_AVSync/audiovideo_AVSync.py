# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import logging
import os
import struct
import tempfile
import time

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import file_utils
from autotest_lib.client.common_lib.cros import arc_common
from autotest_lib.client.cros import constants
from autotest_lib.client.cros.chameleon import audio_test_utils
from autotest_lib.client.cros.chameleon import chameleon_port_finder
from autotest_lib.client.cros.multimedia import arc_resource_common
from autotest_lib.server import autotest
from autotest_lib.server import test
from autotest_lib.server.cros.multimedia import remote_facade_factory


class audiovideo_AVSync(test.test):
    """ Server side HDMI audio/video sync quality measurement

    This test talks to a Chameleon board and a Cros device to measure the
    audio/video sync quality under playing a 1080p 60fps video.
    """
    version = 1

    AUDIO_CAPTURE_RATE = 48000
    VIDEO_CAPTURE_RATE = 60

    BEEP_THRESHOLD = 10 ** 9

    DELAY_BEFORE_CAPTURING = 2
    DELAY_BEFORE_PLAYBACK = 2
    DELAY_AFTER_PLAYBACK = 2

    DEFAULT_VIDEO_URL = ('http://commondatastorage.googleapis.com/'
                         'chromiumos-test-assets-public/chameleon/'
                         'audiovideo_AVSync/1080p_60fps.mp4')

    WAIT_CLIENT_READY_TIMEOUT_SECS = 120

    def compute_audio_keypoint(self, data):
        """Compute audio keypoints. Audio keypoints are the starting times of
        beeps.

        @param data: Raw captured audio data in S32LE, 8 channels, 48000 Hz.

        @returns: Key points of captured data put in a list.
        """
        keypoints = []
        sample_no = 0
        last_beep_no = -100
        for i in xrange(0, len(data), 32):
            values = struct.unpack('<8i', data[i:i+32])
            if values[0] > self.BEEP_THRESHOLD:
                if sample_no - last_beep_no >= 100:
                    keypoints.append(sample_no / float(self.AUDIO_CAPTURE_RATE))
                last_beep_no = sample_no
            sample_no += 1
        return keypoints


    def compute_video_keypoint(self, checksum):
        """Compute video keypoints. Video keypoints are the times when the
        checksum changes.

        @param checksum: Checksums of frames put in a list.

        @returns: Key points of captured video data put in a list.
        """
        return [i / float(self.VIDEO_CAPTURE_RATE)
                for i in xrange(1, len(checksum))
                if checksum[i] != checksum[i - 1]]


    def log_result(self, prefix, key_audio, key_video, dropped_frame_count):
        """Log the test result to result.json and the dashboard.

        @param prefix: A string distinguishes between subtests.
        @param key_audio: Key points of captured audio data put in a list.
        @param key_video: Key points of captured video data put in a list.
        @param dropped_frame_count: Number of dropped frames.
        """
        log_path = os.path.join(self.resultsdir, 'result.json')
        diff = map(lambda x: x[0] - x[1], zip(key_audio, key_video))
        diff_range = max(diff) - min(diff)
        result = dict(
            key_audio=key_audio,
            key_video=key_video,
            av_diff=diff,
            diff_range=diff_range
        )
        if dropped_frame_count is not None:
            result['dropped_frame_count'] = dropped_frame_count

        result = json.dumps(result, indent=2)
        with open(log_path, 'w') as f:
            f.write(result)
        logging.info(str(result))

        dashboard_result = dict(
            diff_range=[diff_range, 'seconds'],
            max_diff=[max(diff), 'seconds'],
            min_diff=[min(diff), 'seconds'],
            average_diff=[sum(diff) / len(diff), 'seconds']
        )
        if dropped_frame_count is not None:
            dashboard_result['dropped_frame_count'] = [
                    dropped_frame_count, 'frames']

        for key, value in dashboard_result.iteritems():
            self.output_perf_value(description=prefix+key, value=value[0],
                                   units=value[1], higher_is_better=False)


    def run_once(self, host, video_hardware_acceleration=True,
                 video_url=DEFAULT_VIDEO_URL, arc=False):
        """Running audio/video synchronization quality measurement

        @param host: A host object representing the DUT.
        @param video_hardware_acceleration: Enables the hardware acceleration
                                            for video decoding.
        @param video_url: The ULR of the test video.
        @param arc: Tests on ARC with an Android Video Player App.
        """
        self.host = host

        factory = remote_facade_factory.RemoteFacadeFactory(
                host, results_dir=self.resultsdir, no_chrome=True)

        chrome_args = {
            'extension_paths': [constants.AUDIO_TEST_EXTENSION,
                                constants.DISPLAY_TEST_EXTENSION],
            'extra_browser_args': [],
            'arc_mode': arc_common.ARC_MODE_DISABLED,
            'autotest_ext': True
        }
        if not video_hardware_acceleration:
            chrome_args['extra_browser_args'].append(
                    '--disable-accelerated-video-decode')
        if arc:
            chrome_args['arc_mode'] = arc_common.ARC_MODE_ENABLED
        browser_facade = factory.create_browser_facade()
        browser_facade.start_custom_chrome(chrome_args)
        logging.info("created chrome")
        if arc:
            self.setup_video_app()

        chameleon_board = host.chameleon
        audio_facade = factory.create_audio_facade()
        display_facade = factory.create_display_facade()
        video_facade = factory.create_video_facade()

        audio_port_finder = chameleon_port_finder.ChameleonAudioInputFinder(
                chameleon_board)
        video_port_finder = chameleon_port_finder.ChameleonVideoInputFinder(
                chameleon_board, display_facade)
        audio_port = audio_port_finder.find_port('HDMI')
        video_port = video_port_finder.find_port('HDMI')

        chameleon_board.setup_and_reset(self.outputdir)

        _, ext = os.path.splitext(video_url)
        with tempfile.NamedTemporaryFile(prefix='playback_', suffix=ext) as f:
            # The default permission is 0o600.
            os.chmod(f.name, 0o644)

            file_utils.download_file(video_url, f.name)
            if arc:
                video_facade.prepare_arc_playback(f.name)
            else:
                video_facade.prepare_playback(f.name)

        edid_path = os.path.join(
                self.bindir, 'test_data/edids/HDMI_DELL_U2410.txt')

        video_port.plug()
        with video_port.use_edid_file(edid_path):
            audio_facade.set_chrome_active_node_type('HDMI', None)
            audio_facade.set_chrome_active_volume(100)
            audio_test_utils.check_audio_nodes(
                    audio_facade, (['HDMI'], None))
            display_facade.set_mirrored(True)
            video_port.start_monitoring_audio_video_capturing_delay()

            time.sleep(self.DELAY_BEFORE_CAPTURING)
            video_port.start_capturing_video((64, 64, 16, 16))
            audio_port.start_capturing_audio()

            time.sleep(self.DELAY_BEFORE_PLAYBACK)
            if arc:
                video_facade.start_arc_playback(blocking_secs=20)
            else:
                video_facade.start_playback(blocking=True)
            time.sleep(self.DELAY_AFTER_PLAYBACK)

            remote_path, _ = audio_port.stop_capturing_audio()
            video_port.stop_capturing_video()
            start_delay = video_port.get_audio_video_capturing_delay()

        local_path = os.path.join(self.resultsdir, 'recorded.raw')
        chameleon_board.host.get_file(remote_path, local_path)

        audio_data = open(local_path).read()
        video_data = video_port.get_captured_checksums()

        logging.info("audio capture %d bytes, %f seconds", len(audio_data),
                     len(audio_data) / float(self.AUDIO_CAPTURE_RATE) / 32)
        logging.info("video capture %d frames, %f seconds", len(video_data),
                     len(video_data) / float(self.VIDEO_CAPTURE_RATE))

        key_audio = self.compute_audio_keypoint(audio_data)
        key_video = self.compute_video_keypoint(video_data)
        # Use the capturing delay to align A/V
        key_video = map(lambda x: x + start_delay, key_video)

        dropped_frame_count = None
        if not arc:
            video_facade.dropped_frame_count()

        prefix = ''
        if arc:
            prefix = 'arc_'
        elif video_hardware_acceleration:
            prefix = 'hw_'
        else:
            prefix = 'sw_'

        self.log_result(prefix, key_audio, key_video, dropped_frame_count)


    def run_client_side_test(self):
        """Runs a client side test on Cros device in background."""
        self.client_at = autotest.Autotest(self.host)
        logging.info('Start running client side test %s',
                     arc_resource_common.PlayVideoProps.TEST_NAME)
        self.client_at.run_test(
                arc_resource_common.PlayVideoProps.TEST_NAME,
                background=True)


    def setup_video_app(self):
        """Setups Play Video app on Cros device.

        Runs a client side test on Cros device to start Chrome and ARC and
        install Play Video app.
        Wait for it to be ready.

        """
        # Removes ready tag that server side test should wait for later.
        self.remove_ready_tag()

        # Runs the client side test.
        self.run_client_side_test()

        logging.info('Waiting for client side Play Video app to be ready')

        # Waits for ready tag to be posted by client side test.
        utils.poll_for_condition(condition=self.ready_tag_exists,
                                 timeout=self.WAIT_CLIENT_READY_TIMEOUT_SECS,
                                 desc='Wait for client side test being ready',
                                 sleep_interval=1)

        logging.info('Client side Play Video app is ready')


    def cleanup(self):
        """Cleanup of the test."""
        self.touch_exit_tag()
        super(audiovideo_AVSync, self).cleanup()


    def remove_ready_tag(self):
        """Removes ready tag on Cros device."""
        if self.ready_tag_exists():
            self.host.run(command='rm %s' % (
                    arc_resource_common.PlayVideoProps.READY_TAG_FILE))


    def touch_exit_tag(self):
        """Touches exit tag on Cros device to stop client side test."""
        self.host.run(command='touch %s' % (
                arc_resource_common.PlayVideoProps.EXIT_TAG_FILE))


    def ready_tag_exists(self):
        """Checks if ready tag exists.

        @returns: True if the tag file exists. False otherwise.

        """
        return self.host.path_exists(
                arc_resource_common.PlayVideoProps.READY_TAG_FILE)
