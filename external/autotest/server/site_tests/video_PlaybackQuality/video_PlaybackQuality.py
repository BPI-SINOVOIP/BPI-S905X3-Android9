# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import tempfile
from PIL import Image

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import file_utils
from autotest_lib.client.cros.chameleon import chameleon_port_finder
from autotest_lib.client.cros.chameleon import chameleon_stream_server
from autotest_lib.client.cros.chameleon import edid
from autotest_lib.server import test
from autotest_lib.server.cros.multimedia import remote_facade_factory


class video_PlaybackQuality(test.test):
    """Server side video playback quality measurement.

    This test measures the video playback quality by chameleon.
    It will output 2 performance data. Number of Corrupted Frames and Number of
    Dropped Frames.

    """
    version = 1

    # treat 0~0x30 as 0
    COLOR_MARGIN_0 = 0x30
    # treat (0xFF-0x60)~0xFF as 0xFF.
    COLOR_MARGIN_255 = 0xFF - 0x60

    # If we can't find the expected frame after TIMEOUT_FRAMES, raise exception.
    TIMEOUT_FRAMES = 120

    # RGB for black. Used for preamble and postamble.
    RGB_BLACK = [0, 0, 0]

    # Expected color bar rgb. The color order in the array is the same order in
    # the video frames.
    EXPECTED_RGB = [('Blue', [0, 0, 255]), ('Green', [0, 255, 0]),
                    ('Cyan', [0, 255, 255]), ('Red', [255, 0, 0]),
                    ('Magenta', [255, 0, 255]), ('Yellow', [255, 255, 0]),
                    ('White', [255, 255, 255])]

    def _save_frame_to_file(self, resolution, frame, filename):
        """Save video frame to file under results directory.

        This function will append .png filename extension.

        @param resolution: A tuple (width, height) of resolution.
        @param frame: The video frame data.
        @param filename: File name.

        """
        image = Image.fromstring('RGB', resolution, frame)
        image.save('%s/%s.png' % (self.resultsdir, filename))

    def _check_rgb_value(self, value, expected_value):
        """Check value of the RGB.

        This function will check if the value is in the range of expected value
        and its margin.

        @param value: The value for checking.
        @param expected_value: Expected value. It's ether 0 or 0xFF.
        @returns: True if the value is in range. False otherwise.

        """
        if expected_value <= value <= self.COLOR_MARGIN_0:
            return True

        if expected_value >= value >= self.COLOR_MARGIN_255:
            return True

        return False

    def _check_rgb(self, frame, expected_rgb):
        """Check the RGB raw data of all pixels in a video frame.

        Because checking all pixels may take more than one video frame time. If
        we want to analyze the video frame on the fly, we need to skip pixels
        for saving the checking time.
        The parameter of how many pixels to skip is self._skip_check_pixels.

        @param frame: Array of all pixels of video frame.
        @param expected_rgb: Expected values for RGB.
        @returns: number of error pixels.

        """
        error_number = 0

        for i in xrange(0, len(frame), 3 * (self._skip_check_pixels + 1)):
            if not self._check_rgb_value(ord(frame[i]), expected_rgb[0]):
                error_number += 1
                continue

            if not self._check_rgb_value(ord(frame[i + 1]), expected_rgb[1]):
                error_number += 1
                continue

            if not self._check_rgb_value(ord(frame[i + 2]), expected_rgb[2]):
                error_number += 1

        return error_number

    def _find_and_skip_preamble(self, description):
        """Find and skip the preamble video frames.

        @param description: Description of the log and file name.

        """
        # find preamble which is the first black frame.
        number_of_frames = 0
        while True:
            video_frame = self._stream_server.receive_realtime_video_frame()
            (frame_number, width, height, _, frame) = video_frame
            if self._check_rgb(frame, self.RGB_BLACK) == 0:
                logging.info('Find preamble at frame %d', frame_number)
                break
            if number_of_frames > self.TIMEOUT_FRAMES:
                raise error.TestFail('%s found no preamble' % description)
            number_of_frames += 1
            self._save_frame_to_file((width, height), frame,
                                     '%s_pre_%d' % (description, frame_number))
        # skip preamble.
        # After finding preamble, find the first frame that is not black.
        number_of_frames = 0
        while True:
            video_frame = self._stream_server.receive_realtime_video_frame()
            (frame_number, _, _, _, frame) = video_frame
            if self._check_rgb(frame, self.RGB_BLACK) != 0:
                logging.info('End preamble at frame %d', frame_number)
                self._save_frame_to_file((width, height), frame,
                                         '%s_end_preamble' % description)
                break
            if number_of_frames > self.TIMEOUT_FRAMES:
                raise error.TestFail('%s found no color bar' % description)
            number_of_frames += 1

    def _store_wrong_frames(self, frame_number, resolution, frames):
        """Store wrong frames for debugging.

        @param frame_number: latest frame number.
        @param resolution: A tuple (width, height) of resolution.
        @param frames: Array of video frames. The latest video frame is in the
                front.

        """
        for index, frame in enumerate(frames):
            if not frame:
                continue
            element = ((frame_number - index), resolution, frame)
            self._wrong_frames.append(element)

    def _check_color_bars(self, description):
        """Check color bars for video playback quality.

        This function will read video frame from stream server and check if the
        color is right by self._check_rgb until read postamble.
        If only some pixels are wrong, the frame will be counted to corrupted
        frame. If all pixels are wrong, the frame will be counted to wrong
        frame.

        @param description: Description of log and file name.
        @return A tuple (corrupted_frame_count, wrong_frame_count) for quality
                data.

        """
        # store the recent 2 video frames for debugging.
        # Put the latest frame in the front.
        frame_history = [None, None]
        # Check index for color bars.
        check_index = 0
        corrupted_frame_count = 0
        wrong_frame_count = 0
        while True:
            # Because the first color bar is skipped in _find_and_skip_preamble,
            # we start from the 2nd color.
            check_index = (check_index + 1) % len(self.EXPECTED_RGB)
            video_frame = self._stream_server.receive_realtime_video_frame()
            (frame_number, width, height, _, frame) = video_frame
            # drop old video frame and store new one
            frame_history.pop(-1)
            frame_history.insert(0, frame)
            color_name = self.EXPECTED_RGB[check_index][0]
            expected_rgb = self.EXPECTED_RGB[check_index][1]
            error_number = self._check_rgb(frame, expected_rgb)

            # The video frame is correct, go to next video frame.
            if not error_number:
                continue

            # Total pixels need to be adjusted by the _skip_check_pixels.
            total_pixels = width * height / (self._skip_check_pixels + 1)
            log_string = ('[%s] Number of error pixels %d on frame %d, '
                          'expected color %s, RGB %r' %
                          (description, error_number, frame_number, color_name,
                           expected_rgb))

            self._store_wrong_frames(frame_number, (width, height),
                                     frame_history)
            # clean history after they are stored.
            frame_history = [None, None]

            # Some pixels are wrong.
            if error_number != total_pixels:
                corrupted_frame_count += 1
                logging.warn('[Corrupted]%s', log_string)
                continue

            # All pixels are wrong.
            # Check if we get postamble where all pixels are black.
            if self._check_rgb(frame, self.RGB_BLACK) == 0:
                logging.info('Find postamble at frame %d', frame_number)
                break

            wrong_frame_count += 1
            logging.info('[Wrong]%s', log_string)
            # Adjust the check index due to frame drop.
            # The screen should keep the old frame or go to next video frame
            # due to frame drop.
            # Check if color is the same as the previous frame.
            # If it is not the same as previous frame, we assign the color of
            # next frame without checking.
            previous_index = ((check_index + len(self.EXPECTED_RGB) - 1)
                              % len(self.EXPECTED_RGB))
            if not self._check_rgb(frame, self.EXPECTED_RGB[previous_index][1]):
                check_index = previous_index
            else:
                check_index = (check_index + 1) % len(self.EXPECTED_RGB)

        return (corrupted_frame_count, wrong_frame_count)

    def _dump_wrong_frames(self, description):
        """Dump wrong frames to files.

        @param description: Description of the file name.

        """
        for frame_number, resolution, frame in self._wrong_frames:
            self._save_frame_to_file(resolution, frame,
                                     '%s_%d' % (description, frame_number))
        self._wrong_frames = []

    def _prepare_playback(self):
        """Prepare playback video."""
        # Workaround for white bar on rightmost and bottommost on samus when we
        # set fullscreen from fullscreen.
        self._display_facade.set_fullscreen(False)
        self._video_facade.prepare_playback(self._video_tempfile.name)

    def _get_playback_quality(self, description, capture_dimension):
        """Get the playback quality.

        This function will playback the video and analysis each video frames.
        It will output performance data too.

        @param description: Description of the log, file name and performance
                data.
        @param capture_dimension: A tuple (width, height) of the captured video
                frame.
        """
        logging.info('Start to get %s playback quality', description)
        self._prepare_playback()
        self._chameleon_port.start_capturing_video(capture_dimension)
        self._stream_server.reset_video_session()
        self._stream_server.dump_realtime_video_frame(
            False, chameleon_stream_server.RealtimeMode.BestEffort)

        self._video_facade.start_playback()
        self._find_and_skip_preamble(description)

        (corrupted_frame_count, wrong_frame_count) = (
            self._check_color_bars(description))

        self._stream_server.stop_dump_realtime_video_frame()
        self._chameleon_port.stop_capturing_video()
        self._video_facade.pause_playback()
        self._dump_wrong_frames(description)

        dropped_frame_count = self._video_facade.dropped_frame_count()

        graph_name = '%s_%s' % (self._video_description, description)
        self.output_perf_value(description='Corrupted frames',
                               value=corrupted_frame_count, units='frame',
                               higher_is_better=False, graph=graph_name)
        self.output_perf_value(description='Wrong frames',
                               value=wrong_frame_count, units='frame',
                               higher_is_better=False, graph=graph_name)
        self.output_perf_value(description='Dropped frames',
                               value=dropped_frame_count, units='frame',
                               higher_is_better=False, graph=graph_name)

    def run_once(self, host, video_url, video_description, test_regions,
                 skip_check_pixels=5):
        """Runs video playback quality measurement.

        @param host: A host object representing the DUT.
        @param video_url: The ULR of the test video.
        @param video_description: a string describes the video to play which
                will be part of entry name in dashboard.
        @param test_regions: An array of tuples (description, capture_dimension)
                for the testing region of video. capture_dimension is a tuple
                (width, height).
        @param skip_check_pixels: We will check one pixel and skip number of
                pixels. 0 means no skip. 1 means check 1 pixel and skip 1 pixel.
                Because we may take more than 1 video frame time for checking
                all pixels. Skip some pixles for saving time.

        """
        # Store wrong video frames for dumping and debugging.
        self._video_url = video_url
        self._video_description = video_description
        self._wrong_frames = []
        self._skip_check_pixels = skip_check_pixels

        factory = remote_facade_factory.RemoteFacadeFactory(
                host, results_dir=self.resultsdir, no_chrome=True)
        chameleon_board = host.chameleon
        browser_facade = factory.create_browser_facade()
        display_facade = factory.create_display_facade()
        self._display_facade = display_facade
        self._video_facade = factory.create_video_facade()
        self._stream_server = chameleon_stream_server.ChameleonStreamServer(
            chameleon_board.host.hostname)

        chameleon_board.setup_and_reset(self.outputdir)
        self._stream_server.connect()

        # Download the video to self._video_tempfile.name
        _, ext = os.path.splitext(video_url)
        self._video_tempfile = tempfile.NamedTemporaryFile(suffix=ext)
        # The default permission is 0o600.
        os.chmod(self._video_tempfile.name, 0o644)
        file_utils.download_file(video_url, self._video_tempfile.name)

        browser_facade.start_default_chrome()
        display_facade.set_mirrored(False)

        edid_path = os.path.join(self.bindir, 'test_data', 'edids',
                                 'EDIDv2_1920x1080')
        finder = chameleon_port_finder.ChameleonVideoInputFinder(
                chameleon_board, display_facade)
        for chameleon_port in finder.iterate_all_ports():
            self._chameleon_port = chameleon_port

            connector_type = chameleon_port.get_connector_type()
            logging.info('See the display on Chameleon: port %d (%s)',
                         chameleon_port.get_connector_id(),
                         connector_type)

            with chameleon_port.use_edid(
                    edid.Edid.from_file(edid_path, skip_verify=True)):
                resolution = utils.wait_for_value_changed(
                    display_facade.get_external_resolution,
                    old_value=None)
                if resolution is None:
                    raise error.TestFail('No external display detected on DUT')

            display_facade.move_to_display(
                display_facade.get_first_external_display_id())

            for description, capture_dimension in test_regions:
                self._get_playback_quality('%s_%s' % (connector_type,
                                                      description),
                                           capture_dimension)
