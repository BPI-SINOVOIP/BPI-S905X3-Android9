# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import division

import base64
import logging
import os
import subprocess
import tempfile
import time

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error, file_utils
from autotest_lib.client.common_lib import utils as common_utils
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros.video import constants
from autotest_lib.client.cros.video import helper_logger
from autotest_lib.client.cros.video import histogram_verifier

DOWNLOAD_BASE = ('http://commondatastorage.googleapis.com/'
                 'chromiumos-test-assets-public/crowd/')

FAKE_FILE_ARG = '--use-file-for-fake-video-capture="%s"'
FAKE_STREAM_ARG = '--use-fake-device-for-media-stream="fps=%d"'
EXTRA_BROWSER_ARGS = ['--use-fake-ui-for-media-stream',
                      '--enable-experimental-web-platform-features']
DISABLE_HW_ENCODE_ARGS = ['--disable-accelerated-video-encode']
JS_INVOCATION = 'startRecording("%s");'
JS_VIDEO_BUFFER = 'videoBuffer'
JS_VIDEO_BUFFER_SIZE = 'videoBufferSize'
TEST_PROGRESS = 'testProgress'
ELAPSED_TIME = 'elapsedTime'
LOOPBACK_PAGE = 'loopback_mediarecorder.html'
TMP_VIDEO_FILE = '/tmp/test_video.webm'
RECORDING_TIMEOUT = 30
WAIT_FOR_IDLE_CPU_TIMEOUT = 10.0
CPU_IDLE_USAGE = 0.1

STABILIZATION_DURATION = 3
MEASUREMENT_DURATION = 10

FRAME_PROCESSING_TIME = 'frame_processing_time'
CPU_USAGE = 'cpu_usage'
HW_PREFIX = 'hw_'
SW_PREFIX = 'sw_'
PERCENT = 'percent'
TIME_UNIT = 'millisecond'


class video_MediaRecorderPerf(test.test):
    """This test measures the performance of MediaRecorder."""
    version = 1

    def get_mkv_result_listener(self):
        # We want to import mkvparse in a method instead of in the global scope
        # because when emerging the test, global import of mkvparse would fail
        # since host environment does not have the path to mkvparse library set
        # up.
        import mkvparse
        class MatroskaResultListener(mkvparse.MatroskaHandler):
            def __init__(self):
                self.all_timestamps_ = set()
                self.video_track_num_ = 0

            def tracks_available(self):
                for k in self.tracks:
                    t = self.tracks[k]
                    if t['type'] == 'video':
                        self.video_track_num_ = t['TrackNumber'][1]
                        break

            def frame(self, track_id, timestamp, data, more_laced_frames, duration,
                      keyframe, invisible, discardable):
                if track_id == self.video_track_num_:
                    timestamp = round(timestamp, 6)
                    self.all_timestamps_.add(timestamp)

            def get_num_frames(self):
                return len(self.all_timestamps_)

        return MatroskaResultListener()

    def video_record_completed(self):
        """Checks if MediaRecorder has recorded videos.

        @returns True if videos have been recorded, False otherwise.
        """
        def video_recorded():
            """Check the testProgress variable in HTML page."""

            # Wait for TEST_PROGRESS appearing on web page.
            test_progress = self.tab.EvaluateJavaScript(TEST_PROGRESS)
            return test_progress == 1

        try:
            common_utils.poll_for_condition(
                    video_recorded, timeout=RECORDING_TIMEOUT,
                    exception=error.TestError('Cannot find testProgress.'),
                    sleep_interval=1)
        except error.TestError:
            return False
        else:
            return True

    def compute_cpu_usage(self):
        time.sleep(STABILIZATION_DURATION)
        cpu_usage_start = utils.get_cpu_usage()
        time.sleep(MEASUREMENT_DURATION)
        cpu_usage_end = utils.get_cpu_usage()
        return utils.compute_active_cpu_time(cpu_usage_start,
                                             cpu_usage_end) * 100

    def wait_for_idle_cpu(self):
       if not utils.wait_for_idle_cpu(WAIT_FOR_IDLE_CPU_TIMEOUT,
                                      CPU_IDLE_USAGE):
           logging.warning('Could not get idle CPU post login.')
       if not utils.wait_for_cool_machine():
           logging.warning('Could not get cold machine post login.')

    def get_record_performance(self, cr, codec, is_hw_encode):
        self.wait_for_idle_cpu()
        cr.browser.platform.SetHTTPServerDirectories(self.bindir)
        self.tab = cr.browser.tabs.New()
        self.tab.Navigate(cr.browser.platform.http_server.UrlOf(
                os.path.join(self.bindir, LOOPBACK_PAGE)))
        self.tab.WaitForDocumentReadyStateToBeComplete()
        self.tab.EvaluateJavaScript(JS_INVOCATION % codec)
        cpu_usage = self.compute_cpu_usage()
        if not self.video_record_completed():
            raise error.TestFail('Video record did not complete')

        histogram_verifier.verify(
                cr,
                constants.MEDIA_RECORDER_VEA_USED_HISTOGRAM,
                constants.MEDIA_RECORDER_VEA_USED_BUCKET if is_hw_encode
                        else constants.MEDIA_RECORDER_VEA_NOT_USED_BUCKET)
        video_buffer = self.tab.EvaluateJavaScript(JS_VIDEO_BUFFER)
        elapsed_time = self.tab.EvaluateJavaScript(ELAPSED_TIME)
        video_buffer_size = self.tab.EvaluateJavaScript(JS_VIDEO_BUFFER_SIZE)
        video_buffer = video_buffer.encode('ascii', 'ignore')
        if video_buffer_size != len(video_buffer):
            raise error.TestFail('Video buffer from JS is truncated: Was %d.\
                    Got %d' % (video_buffer_size, len(video_buffer)))

        import mkvparse
        video_byte_array = bytearray(base64.b64decode(video_buffer))
        mkv_listener = self.get_mkv_result_listener()
        with tempfile.TemporaryFile() as video_file:
            video_file.write(video_byte_array)
            video_file.seek(0)
            mkvparse.mkvparse(video_file, mkv_listener)
            logging.info('(%s) Number of frames: %d time: %d',
                    'hw' if is_hw_encode else 'sw',
                    mkv_listener.get_num_frames(),
                    elapsed_time)
        return (elapsed_time / mkv_listener.get_num_frames(), cpu_usage)

    @helper_logger.video_log_wrapper
    def run_once(self, codec, fps, video_file):
        """ Report cpu usage and frame processing time with HW and SW encode.

        Use MediaRecorder to record a videos with HW encode and SW encode, and
        report the frame processing time and CPU usage of both.

        @param codec: a string indicating the codec used to encode video stream,
                ex. 'h264'.
        @param fps: an integer specifying FPS of the fake input video stream.
        @param video_file: a string specifying the name of the video file to be
                used as fake input video stream.
        """
        # Download test video.
        url = DOWNLOAD_BASE + video_file
        local_path = os.path.join(self.bindir, video_file)
        file_utils.download_file(url, local_path)
        fake_file_arg = (FAKE_FILE_ARG % local_path)
        fake_stream_arg = (FAKE_STREAM_ARG % fps)

        processing_time_sw = 0
        processing_time_hw = 0
        cpu_usage_sw = 0
        cpu_usage_hw = 0
        with chrome.Chrome(
                extra_browser_args=EXTRA_BROWSER_ARGS +
                [fake_file_arg, fake_stream_arg] +
                DISABLE_HW_ENCODE_ARGS +
                [helper_logger.chrome_vmodule_flag()],
                init_network_controller=True) as cr:
            (processing_time_sw, cpu_usage_sw) = self.get_record_performance(
                    cr, codec, False)
            self.output_perf_value(description=(SW_PREFIX + FRAME_PROCESSING_TIME),
                    value=processing_time_sw, units=TIME_UNIT, higher_is_better=False)
            self.output_perf_value(description=(SW_PREFIX + CPU_USAGE),
                    value=cpu_usage_sw, units=PERCENT, higher_is_better=False)

        with chrome.Chrome(
                extra_browser_args=EXTRA_BROWSER_ARGS +
                [fake_file_arg, fake_stream_arg] +
                [helper_logger.chrome_vmodule_flag()],
                init_network_controller=True) as cr:
            (processing_time_hw, cpu_usage_hw) = self.get_record_performance(
                    cr, codec, True)
            self.output_perf_value(description=(HW_PREFIX + FRAME_PROCESSING_TIME),
                    value=processing_time_hw, units=TIME_UNIT, higher_is_better=False)
            self.output_perf_value(description=(HW_PREFIX + CPU_USAGE),
                    value=cpu_usage_hw, units=PERCENT, higher_is_better=False)

        log = 'sw processing_time=%f cpu=%d hw processing_time=%f cpu=%d' % (
                processing_time_sw,
                cpu_usage_sw,
                processing_time_hw,
                cpu_usage_hw)
        logging.info(log)

