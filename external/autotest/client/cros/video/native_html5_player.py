# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


from autotest_lib.client.cros.video import video_player

import py_utils
import logging


class NativeHtml5Player(video_player.VideoPlayer):
    """
    Provides an interface to interact with native html5 player in chrome.

    """


    def inject_source_file(self):
        """
        Injects the path to the video file under test into the html doc.


        """
        self.tab.ExecuteJavaScript(
            'loadVideoSource("%s")' % self.video_src_path)


    def is_video_ready(self):
        """
        Determines if a native html5 video is ready by using javascript.

        returns: bool, True if video is ready, else False.

        """
        return self.tab.EvaluateJavaScript('canplay()')


    def is_javascript_ready(self):
        """
        returns: True if javascript variables and functions have been defined,

        else False.

        """
        return self.tab.EvaluateJavaScript(
                    'typeof script_ready!="undefined" && script_ready == true')


    def play(self):
        """
        Plays the video.

        """
        self.tab.ExecuteJavaScript('play()')


    def pause(self):
        """
        Pauses the video.

        """
        self.tab.ExecuteJavaScript('pause()')


    def paused(self):
        """
        Checks whether video paused.

        """
        cmd = '%s.paused' % self.video_id
        return self.tab.EvaluateJavaScript(cmd)


    def ended(self):
        """
        Checks whether video paused.

        """
        cmd = '%s.ended' % self.video_id
        return self.tab.EvaluateJavaScript(cmd)


    def currentTime(self):
        """
        Returns the current time of the video element.

        """
        return self.tab.EvaluateJavaScript('currentTime()')


    def seek_to(self, t):
        """
        Seeks a video to a time stamp.

        @param t: timedelta, time value to seek to.

        """
        cmd = '%s.currentTime=%.3f' % (self.video_id, t.total_seconds())
        self.tab.ExecuteJavaScript(cmd)


    def has_video_finished_seeking(self):
        """
        Determines if the video has finished seeking.

        """
        return self.tab.EvaluateJavaScript('finishedSeeking()')


    def wait_for_error(self):
        """
        Determines if the video has any errors

        """
        return self.tab.WaitForJavaScriptCondition('errorDetected();',
                                                   timeout=30)


    def reload_page(self):
        """
        Reloads current page

        """
        self.tab.ExecuteJavaScript('location.reload()')


    def enable_VideoControls(self):
        """
        For enabling controls

        """
        self.tab.ExecuteJavaScript('setControls()')


    def dropped_frame_count(self):
        """
        Gets the number of dropped frames.

        @returns: An integer indicates the number of dropped frame.

        """
        cmd = '%s.webkitDroppedFrameCount' % self.video_id
        return self.tab.EvaluateJavaScript(cmd)


    def duration(self):
        """
        Gets the duration of the video.

        @returns: An number indicates the duration of the video.

        """
        cmd = '%s.duration' % self.video_id
        return self.tab.EvaluateJavaScript(cmd)


    def wait_video_ended(self):
        """
        Waits until the video playback is ended.

        """
        cmd = '%s.ended' % self.video_id
        self.tab.WaitForJavaScriptCondition(cmd, timeout=(self.duration() * 2))


    def wait_ended_or_error(self):
        """
        Waits until the video ends or an error happens.

        """
        try:
            # unit of timeout is second.
            self.tab.WaitForJavaScriptCondition('endOrError()',
                                                timeout=(self.duration() + 10))
        except py_utils.TimeoutException:
            logging.error('Timeout in waiting endOrError()')
            raise


    def check_error(self):
        """
        Check whether an error happens.

        """
        return self.tab.EvaluateJavaScript('errorDetected()')


    def get_error_info(self):
        """
        Get error code and message
        @returns string,string: error code and message

        """
        error_code = self.tab.EvaluateJavaScript(
                          '%s.error.code' % self.video_id)
        error_message = self.tab.EvaluateJavaScript(
                          '%s.error.message' % self.video_id)
        return error_code, error_message
