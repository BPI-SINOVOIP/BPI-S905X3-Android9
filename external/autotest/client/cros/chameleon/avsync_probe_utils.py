# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This module provides the utilities for avsync_probe's data processing.

We will get a lot of raw data from the avsync_probe.Capture(). One data per
millisecond.
AVSyncProbeDataParser will help to transform the raw data to more readable
formats. It also helps to calculate the audio/video sync timing if the
sound_interval_frames parameter is not None.

Example:
    capture_data = avsync_probe.Capture(12)
    parser = avsync_probe_utils.AVSyncProbeDataParser(self.resultsdir,
            capture_data, 30)

    # Use the following attributes to access data. They can be referenced in
    # AVSyncProbeDataParser Class.
    parser.video_duration_average
    parser.video_duration_std
    parser.sync_duration_averag
    parser.sync_duration_std
    parser.cumulative_frame_count
    parser.dropped_frame_count
    parser.corrupted_frame_count
    parser.binarize_data
    parser.audio_events
    parser.video_events

"""

import collections
import logging
import math
import os
import sys


# Indices for binarize_data, audio_events and video_events.
TIME_INDEX = 0
VIDEO_INDEX = 1
AUDIO_INDEX = 2
# This index is used for video_events and audio_events.
# The slot contains the time difference to the previous event.
TIME_DIFF_INDEX = 3

# SyncResult namedtuple of audio and video frame.
# time_delay < 0 means that audio comes out first.
SyncResult = collections.namedtuple(
        'SynResult', ['video_time', 'audio_time', 'time_delay'])


class GrayCode(object):
    """Converts bit patterns between binary and Gray code.

    The bit patterns of Gray code values are packed into an int value.
    For example, 4 is "110" in Gray code, which reads "6" when interpreted
    as binary.
    See "https://en.wikipedia.org/wiki/Gray_code"

    """

    @staticmethod
    def binary_to_gray(binary):
        """Binary code to gray code.

        @param binary: Binary code.
        @return: gray code.

        """
        return binary ^ (binary >> 1)

    @staticmethod
    def gray_to_binary(gray):
        """Gray code to binary code.

        @param gray: Gray code.
        @return: binary code.

        """
        result = gray
        result ^= (result >> 16)
        result ^= (result >> 8)
        result ^= (result >> 4)
        result ^= (result >> 2)
        result ^= (result >> 1)
        return result


class HysteresisSwitch(object):
    """
    Iteratively binarizes input sequence using hysteresis comparator with a
    pair of fixed thresholds.

    Hysteresis means to use 2 different thresholds
    for activating and de-activating output. It is often used for thresholding
    time-series signal while reducing small noise in the input.

    Note that the low threshold is exclusive but the high threshold is
    inclusive.
    When the same values were applied for the both, the object works as a
    non-hysteresis switch.
    (i.e. equivalent to the >= operator).

    """

    def __init__(self, low_threshold, high_threshold, init_state):
        """Init HysteresisSwitch class.

        @param low_threshold: The threshold value to deactivate the output.
                The comparison is exclusive.
        @param high_threshold: The threshold value to activate the output.
                The comparison is inclusive.
        @param init_state: True or False of the switch initial state.

        """
        if low_threshold > high_threshold:
            raise Exception('Low threshold %d exceeds the high threshold %d',
                            low_threshold, high_threshold)
        self._low_threshold = low_threshold
        self._high_threshold = high_threshold
        self._last_state = init_state

    def adjust_state(self, value):
        """Updates the state of the switch by the input value and returns the
        result.

        @param value: value for updating.
        @return the state of the switch.

        """
        if value < self._low_threshold:
            self._last_state = False

        if value >= self._high_threshold:
            self._last_state = True

        return self._last_state


class AVSyncProbeDataParser(object):
    """ Digital information extraction from the raw sensor data sequence.

    This class will transform the raw data to easier understand formats.

    Attributes:
        binarize_data: Transer the raw data to [Time, video code, is_audio].
               video code is from 0-7 repeatedly.
        video_events: Events of video frame.
        audio_events: Events of when audio happens.
        video_duration_average: (ms) The average duration during video frames.
        video_duration_std: Standard deviation of the video_duration_average.
        sync_duration_average: (ms) The average duration for audio/video sync.
        sync_duration_std: Standard deviation of sync_duration_average.
        cumulative_frame_count: Number of total video frames.
        dropped_frame_count: Total dropped video frames.
        corrupted_frame_count: Total corrupted video frames.

    """
    # Thresholds for hysteresis binarization of input signals.
    # Relative to the minumum (0.0) and maximum (1.0) values of the value range
    # of each input signal.
    _NORMALIZED_LOW_THRESHOLD = 0.6
    _NORMALIZED_HIGH_THRESHOLD = 0.7

    _VIDEO_CODE_CYCLE = (1 << 3)

    def __init__(self, log_dir, capture_raw_data, video_fps,
                 sound_interval_frames=None):
        """Inits AVSyncProbeDataParser class.

        @param log_dir: Directory for dumping each events' contents.
        @param capture_raw_data: Raw data from avsync_probe device.
                A list contains the list values of [timestamp, video0, video1,
                                                    video2, audio].
        @param video_fps: Video frames per second. Used to know if the video
                frame is dropoped or just corrupted.
        @param sound_interval_frames: The period of sound (beep) in the number
                of video frames. This class will help to calculate audio/video
                sync stats if sound_interval_frames is not None.

        """
        self.video_duration_average = None
        self.video_duration_std = None
        self.sync_duration_average = None
        self.sync_duration_std = None
        self.cumulative_frame_count = None
        self.dropped_frame_count = None

        self._log_dir = log_dir
        self._raw_data = capture_raw_data
        # Translate to millisecond for each video frame.
        self._video_duration = 1000 / video_fps
        self._sound_interval_frames = sound_interval_frames
        self._log_list_data_to_file('raw.txt', capture_raw_data)

        self.binarize_data = self._binarize_raw_data()
        # we need to get audio events before remove video preamble frames.
        # Because audio event may appear before the preamble frame, if we
        # remove the preamble frames first, we will lost the audio event.
        self.audio_events = self._detect_audio_events()
        self._remove_video_preamble()
        self.video_events = self._detect_video_events()
        self._analyze_events()
        self._calculate_statistics_report()

    def _log_list_data_to_file(self, filename, data):
        """Log the list data to file.

        It will log under self._log_dir directory.

        @param filename: The file name.
        @data: Data for logging.

        """
        filepath = os.path.join(self._log_dir, filename)
        with open(filepath, 'w') as f:
            for v in data:
                f.write('%s\n' % str(v))

    def _get_hysteresis_switch(self, index):
        """Get HysteresisSwitch by the raw data.

        @param index: The index of self._raw_data's element.
        @return: HysteresisSwitch instance by the value of the raw data.

        """
        max_value = max(x[index] for x in self._raw_data)
        min_value = min(x[index] for x in self._raw_data)
        scale = max_value - min_value
        logging.info('index %d, max %d, min %d, scale %d', index, max_value,
                     min_value, scale)
        return HysteresisSwitch(
                min_value + scale * self._NORMALIZED_LOW_THRESHOLD,
                min_value + scale * self._NORMALIZED_HIGH_THRESHOLD,
                False)

    def _binarize_raw_data(self):
        """Conducts adaptive thresholding and decoding embedded frame codes.

        Sensors[0] is timestamp.
        Sensors[1-3] are photo transistors, which outputs lower value for
        brighter light(=white pixels on screen). These are used to detect black
        and white pattern on the screen, and decoded as an integer code.

        The final channel is for audio input, which outputs higher voltage for
        larger sound volume. This will be used for detecting beep sounds added
        to the video.

        @return Decoded frame codes list for all the input frames. Each entry
                contains [Timestamp, video code, is_audio].

        """
        decoded_data = []

        hystersis_switch = []
        for i in xrange(5):
            hystersis_switch.append(self._get_hysteresis_switch(i))

        for data in self._raw_data:
            code = 0
            # Decode black-and-white pattern on video.
            # There are 3 black or white boxes sensed by the sensors.
            # Each square represents a single bit (white = 1, black = 0) coding
            # an integer in Gray code.
            for i in xrange(1, 4):
                # Lower sensor value for brighter light(square painted white).
                is_white = not hystersis_switch[i].adjust_state(data[i])
                if is_white:
                    code |= (1 << (i - 1))
            code = GrayCode.gray_to_binary(code)
            # The final channel is sound signal. Higher sensor value for
            # higher sound level.
            sound = hystersis_switch[4].adjust_state(data[4])
            decoded_data.append([data[0], code, sound])

        self._log_list_data_to_file('binarize_raw.txt', decoded_data)
        return decoded_data

    def _remove_video_preamble(self):
        """Remove preamble video frames of self.binarize_data."""
        # find preamble frame (code = 0)
        index = next(i for i, v in enumerate(self.binarize_data)
                     if v[VIDEO_INDEX] == 0)
        self.binarize_data = self.binarize_data[index:]

        # skip preamble frame (code = 0)
        index = next(i for i, v in enumerate(self.binarize_data)
                     if v[VIDEO_INDEX] != 0)
        self.binarize_data = self.binarize_data[index:]

    def _detect_events(self, detect_condition):
        """Detects events from the binarize data sequence by the
        detect_condition.

        @param detect_condition: callback function for checking event happens.
                This API will pass index and element of binarize_data to the
                callback function.

        @return: The list of events. It's the same as the binarize_data and add
                additional time_difference information.

        """
        detected_events = []
        previous_time = self.binarize_data[0][TIME_INDEX]
        for i, v in enumerate(self.binarize_data):
            if (detect_condition(i, v)):
                time = v[TIME_INDEX]
                time_difference = time - previous_time
                # Copy a new instance here, because we will append time
                # difference.
                event = list(v)
                event.append(time_difference)
                detected_events.append(event)
                previous_time = time

        return detected_events

    def _detect_audio_events(self):
        """Detects the audio start frame from the binarize data sequence.

        @return: The list of Audio events. It's the same as the binarize_data
                and add additional time_difference information.

        """
        # Only check the first audio happen event.
        detected_events = self._detect_events(
            lambda i, v: (v[AUDIO_INDEX] and not
                          self.binarize_data[i - 1][AUDIO_INDEX]))

        self._log_list_data_to_file('audio_events.txt', detected_events)
        return detected_events

    def _detect_video_events(self):
        """Detects the video frame from the binarize data sequence.

        @return: The list of Video events. It's the same as the binarize_data
                and add additional time_difference information.

        """
        # remove duplicate frames. (frames in transition state.)
        detected_events = self._detect_events(
            lambda i, v: (v[VIDEO_INDEX] !=
                          self.binarize_data[i - 1][VIDEO_INDEX]))

        self._log_list_data_to_file('video_events.txt', detected_events)
        return detected_events

    def _match_sync(self, video_time):
        """Match the audio/video sync timing.

        This function will find the closest sound in the audio_events to the
        video_time and returns a audio/video sync tuple.

        @param video_time: the time of the video which have sound.
        @return A SyncResult namedtuple containing:
                  - timestamp of the video frame which should have audio.
                  - timestamp of nearest audio frame.
                  - time delay between audio and video frame.

        """
        closest_difference = sys.maxint
        audio_time = 0
        for audio_event in self.audio_events:
            difference = audio_event[TIME_INDEX] - video_time
            if abs(difference) < abs(closest_difference):
                closest_difference = difference
                audio_time = audio_event[TIME_INDEX]
        return SyncResult(video_time, audio_time, closest_difference)

    def _calculate_statistics(self, data):
        """Calculate average and standard deviation of the list data.

        @param data: The list of values to be calcualted.
        @return: An tuple with (average, standard_deviation)

        """
        if not data:
            return (None, None)

        total = sum(data)
        average = total / len(data)
        variance = sum((v - average)**2 for v in data) / len(data)
        standard_deviation = math.sqrt(variance)
        return (average, standard_deviation)

    def _analyze_events(self):
        """Analyze audio/video events.

        This function will analyze video frame status and audio/video sync
        status.

        """
        sound_interval_frames = self._sound_interval_frames
        current_code = 0
        cumulative_frame_count = 0
        dropped_frame_count = 0
        corrupted_frame_count = 0
        sync_events = []

        for v in self.video_events:
            code = v[VIDEO_INDEX]
            time = v[TIME_INDEX]
            frame_diff = code - current_code
            # Get difference of the codes.  # The code is between 0 - 7.
            if frame_diff < 0:
                frame_diff += self._VIDEO_CODE_CYCLE

            if frame_diff != 1:
                # Check if we dropped frame or just got corrupted frame.
                # Treat the frame as corrupted frame if the frame duration is
                # less than 2 video frame duration.
                if v[TIME_DIFF_INDEX] < 2 * self._video_duration:
                    logging.warn('Corrupted frame near %s', str(v))
                    # Correct the code.
                    code = current_code + 1
                    corrupted_frame_count += 1
                    frame_diff = 1
                else:
                    logging.warn('Dropped frame near %s', str(v))
                    dropped_frame_count += (frame_diff - 1)

            cumulative_frame_count += frame_diff

            if sound_interval_frames is not None:
                # This frame corresponds to a sound.
                if cumulative_frame_count % sound_interval_frames == 1:
                    sync_events.append(self._match_sync(time))

            current_code = code
        self.cumulative_frame_count = cumulative_frame_count
        self.dropped_frame_count = dropped_frame_count
        self.corrupted_frame_count = corrupted_frame_count
        self._sync_events = sync_events
        self._log_list_data_to_file('sync.txt', sync_events)

    def _calculate_statistics_report(self):
        """Calculates statistics report."""
        video_duration_average, video_duration_std = self._calculate_statistics(
                [v[TIME_DIFF_INDEX] for v in self.video_events])
        sync_duration_average, sync_duration_std = self._calculate_statistics(
                [v.time_delay for v in self._sync_events])
        self.video_duration_average = video_duration_average
        self.video_duration_std = video_duration_std
        self.sync_duration_average = sync_duration_average
        self.sync_duration_std = sync_duration_std
