# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.graphics import graphics_utils
from autotest_lib.client.cros.power import power_utils, power_rapl, power_status


PLAYBACK_TEST_TIME_S = 10
PLAYER_ENDED_STATE = 'Ended'
PLAYER_PAUSE_STATE = 'Paused'
PLAYER_PLAYING_STATE = 'Playing'
WAIT_TIMEOUT_S = 20


class YouTubeHelper(object):
    """A helper class contains YouTube related utility functions.

       To use this class, please call wait_for_player_state(playing) as below
       before calling set_video_duration. Please note that set_video_duration
       must be called in order to access few functions which uses the video
       length member variable.

       yh = youtube_helper.YouTubeHelper(tab)
       yh.wait_for_player_state(PLAYER_PLAYING_STATE)
       yh.set_video_duration()

    """

    def __init__(self, youtube_tab, power_logging=False):
        self._tab = youtube_tab
        self._video_duration = 0
        self.power_logger = None

        if power_logging and power_utils.has_rapl_support():
            self.power_logger = power_status.PowerLogger(
                power_rapl.create_rapl())
            self.power_logger.start()

    def set_video_duration(self):
        """Sets the video duration."""
        self._video_duration = (int(self._tab.EvaluateJavaScript(
            'player.getDuration()')))

    def get_power_measurement(self):
        """Return power measurement.

        @return: power readings. None if power_logging is not enabled.
        """
        if self.power_logger:
            return self.power_logger.calc()
        return None

    def video_current_time(self):
        """Returns video's current playback time.

        Returns:
            returns the current playback location in seconds (int).

        """
        return int(self._tab.EvaluateJavaScript('player.getCurrentTime()'))

    def get_player_status(self):
        """Returns the player status."""
        return self._tab.EvaluateJavaScript(
            '(typeof playerStatus !== \'undefined\') && '
            'playerStatus.innerHTML')

    def set_playback_quality(self, quality):
        """Set the video quality to the quality passed in the arg.

        @param quality: video quality to set.

        """
        self._tab.ExecuteJavaScript(
            'player.setPlaybackQuality("%s")' % quality)

    def get_playback_quality(self):
        """Returns the playback quality."""
        return self._tab.EvaluateJavaScript('player.getPlaybackQuality()')

    def wait_for_player_state(self, expected_status):
        """Wait till the player status changes to expected_status.

        If the status doesn't change for long, the test will time out after
        WAIT_TIMEOUT_S and fails.

        @param expected_status: status which is expected for the test
                                to continue.

        """
        utils.poll_for_condition(
            lambda: self.get_player_status() == expected_status,
            exception=error.TestError(
                'Video failed to load. Player expected status: %s'
                ' and current status: %s.'
                % (expected_status, self.get_player_status())),
            timeout=WAIT_TIMEOUT_S,
            sleep_interval=1)

    def verify_video_playback(self):
        """Verify the video playback."""
        logging.info('Verifying the YouTube video playback.')
        playback = 0  # seconds
        prev_playback = 0
        count = 0
        while (self.video_current_time() < self._video_duration
               and playback < PLAYBACK_TEST_TIME_S):
            time.sleep(1)
            if self.video_current_time() <= prev_playback:
                if count < 2:
                    logging.info('Retrying to video playback test.')
                    self._tab.ExecuteJavaScript(
                        'player.seekTo(%d, true)'
                        % (self.video_current_time() + 2))
                    time.sleep(1)
                    count = count + 1
                else:
                    player_status = self.get_player_status()
                    raise error.TestError(
                        'Video is not playing. Player status: %s.' %
                        player_status)
            prev_playback = self.video_current_time()
            playback = playback + 1

    def wait_for_expected_resolution(self, expected_quality):
        """Wait for some time for getting expected resolution.

        @param expected_quality: quality to be set.

        """
        for _ in range(WAIT_TIMEOUT_S):
            dummy_quality = self.get_playback_quality()
            if dummy_quality == expected_quality:
                logging.info('Expected resolution is set.')
                break
            else:
                logging.info('Waiting for expected resolution.')
                time.sleep(0.1)

    def verify_video_resolutions(self, power_measurement=False):
        """Verify available video resolutions.

        Video resolution should be 360p, 480p, 720p and 1080p.

        """
        logging.info('Verifying the video resolutions.')
        video_qualities = self._tab.EvaluateJavaScript(
            'player.getAvailableQualityLevels()')
        logging.info('Available video resolutions: %s', video_qualities)
        if not video_qualities:
            raise error.TestError(
                'Player failed to return available video qualities.')
        video_qualities.reverse()

        running_quality = self.get_playback_quality()
        index = video_qualities.index(running_quality)
        supporting_qualities = video_qualities[index:]
        logging.info("new video quality %s ", supporting_qualities)

        width, height = graphics_utils.get_internal_resolution()
        logging.info('checking resolution: %d width  %d height', width, height)
        for quality in supporting_qualities:
            logging.info('Playing video in %s quality.', quality)

            if quality == "hd1080":
                self._tab.ExecuteJavaScript('player.setSize(1920, 1080)')
            if quality == "hd720":
                self._tab.ExecuteJavaScript('player.setSize(1280, 720)')
            if quality == "large":
                self._tab.ExecuteJavaScript('player.setSize(853, 480)')
            if quality == "medium":
                self._tab.ExecuteJavaScript('player.setSize(640, 360)')
            if quality == "small":
                self._tab.ExecuteJavaScript('player.setSize(320, 240)')

            self.set_playback_quality(quality)
            self.wait_for_player_state(PLAYER_PLAYING_STATE)
            self.wait_for_expected_resolution(quality)
            current_quality = self.get_playback_quality()

            if current_quality != quality:
                raise error.TestError(
                    'Expected video quality: %s. Current video quality: %s'
                    % (quality, current_quality))

            if power_measurement and self.power_logger:
                # Seeking to the beginning and ensure the player is playing.
                self._tab.ExecuteJavaScript('player.seekTo(0, true)')
                self._tab.ExecuteJavaScript('player.playVideo()')
                self.wait_for_player_state(PLAYER_PLAYING_STATE)
                with self.power_logger.checkblock('youtube-' + quality):
                    time.sleep(10)
            else:
                time.sleep(1)

    def verify_player_states(self):
        """Verify the player states like play, pause, ended and seek."""
        logging.info('Verifying the player states.')
        self._tab.ExecuteJavaScript('player.pauseVideo()')
        self.wait_for_player_state(PLAYER_PAUSE_STATE)
        self._tab.ExecuteJavaScript('player.playVideo()')
        self.wait_for_player_state(PLAYER_PLAYING_STATE)
        # We are seeking the player position to (video length - 2 seconds).
        # Since the player waits for WAIT_TIMEOUT_S for the status change,
        # the video should be ended before we hit the timeout.
        video_end_test_duration = (self._video_duration -
                                   self.video_current_time() - 2)
        if video_end_test_duration >= WAIT_TIMEOUT_S:
            self._tab.ExecuteJavaScript(
                'player.seekTo(%d, true)' % (self._video_duration - 5))
            self.wait_for_player_state(PLAYER_ENDED_STATE)
        else:
            raise error.TestError(
                'Test video is not long enough for the video end test.')
        # Verifying seek back from the end position.
        self._tab.ExecuteJavaScript('player.seekTo(%d, true)'
                                    % (self._video_duration / 2))
        self.wait_for_player_state(PLAYER_PLAYING_STATE)
        # So the playback doesn't stay at the mid.
        seek_test = False
        for _ in range(WAIT_TIMEOUT_S):
            logging.info('Waiting for seek position to change.')
            time.sleep(1)
            seek_position = self.video_current_time()
            if (seek_position > self._video_duration / 2
                    and seek_position < self._video_duration):
                seek_test = True
                break
        if not seek_test:
            raise error.TestError(
                'Seek location is wrong. '
                'Video length: %d, seek position: %d.' %
                (self._video_duration, seek_position))
