# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Facade to access the video-related functionality."""

import functools
import glob
import os

from autotest_lib.client.bin import utils
from autotest_lib.client.cros.multimedia import display_facade_native
from autotest_lib.client.cros.video import native_html5_player


class VideoFacadeNativeError(Exception):
    """Error in VideoFacadeNative."""
    pass


def check_arc_resource(func):
    """Decorator function for ARC related functions in VideoFacadeNative."""
    @functools.wraps(func)
    def wrapper(instance, *args, **kwargs):
        """Wrapper for the methods to check _arc_resource.

        @param instance: Object instance.

        @raises: VideoFacadeNativeError if there is no ARC resource.

        """
        if not instance._arc_resource:
            raise VideoFacadeNativeError('There is no ARC resource.')
        return func(instance, *args, **kwargs)
    return wrapper


class VideoFacadeNative(object):
    """Facede to access the video-related functionality.

    The methods inside this class only accept Python native types.

    """

    def __init__(self, resource, arc_resource=None):
        """Initializes an video facade.

        @param resource: A FacadeResource object.
        @param arc_resource: An ArcResource object.

        """
        self._resource = resource
        self._player = None
        self._arc_resource = arc_resource
        self._display_facade = display_facade_native.DisplayFacadeNative(
                resource)
        self.bindir = os.path.dirname(os.path.realpath(__file__))


    def cleanup(self):
        """Clean up the temporary files."""
        for path in glob.glob('/tmp/playback_*'):
            os.unlink(path)

        if self._arc_resource:
            self._arc_resource.cleanup()


    def prepare_playback(self, file_path, fullscreen=True):
        """Copies the html file to /tmp and loads the webpage.

        @param file_path: The path to the file.
        @param fullscreen: Plays the video in fullscreen.

        """
        # Copies the html file to /tmp to make it accessible.
        utils.get_file(
                os.path.join(self.bindir, 'video.html'),
                '/tmp/playback_video.html')

        html_path = 'file:///tmp/playback_video.html'

        tab = self._resource._browser.tabs.New()
        tab.Navigate(html_path)
        self._player = native_html5_player.NativeHtml5Player(
                tab=tab,
                full_url=html_path,
                video_id='video',
                video_src_path=file_path)
        self._player.load_video()

        if fullscreen:
            self._display_facade.set_fullscreen(True)


    def start_playback(self, blocking=False):
        """Starts video playback on the webpage.

        Before calling this method, user should call prepare_playback to
        put the files to /tmp and load the webpage.

        @param blocking: Blocks this call until playback finishes.

        """
        self._player.play()
        if blocking:
            self._player.wait_video_ended()


    def pause_playback(self):
        """Pauses playback on the webpage."""
        self._player.pause()


    def dropped_frame_count(self):
        """
        Gets the number of dropped frames.

        @returns: An integer indicates the number of dropped frame.

        """
        return self._player.dropped_frame_count()


    @check_arc_resource
    def prepare_arc_playback(self, file_path, fullscreen=True):
        """Copies the video file to be played into container and starts the app.

        User should call this method to put the file into container before
        calling start_arc_playback.

        @param file_path: Path to the file to be played on Cros host.
        @param fullscreen: Plays the video in fullscreen.

        """
        self._arc_resource.play_video.prepare_playback(file_path, fullscreen)


    @check_arc_resource
    def start_arc_playback(self, blocking_secs=None):
        """Starts playback through Play Video app.

        Before calling this method, user should call set_arc_playback_file to
        put the file into container and start the app.

        @param blocking_secs: A positive number indicates the timeout to wait
                              for the playback is finished. Set None to make
                              it non-blocking.


        """
        self._arc_resource.play_video.start_playback(blocking_secs)


    @check_arc_resource
    def pause_arc_playback(self):
        """Pauses playback through Play Video app."""
        self._arc_resource.play_video.pause_playback()


    @check_arc_resource
    def stop_arc_playback(self):
        """Stops playback through Play Video app."""
        self._arc_resource.play_video.stop_playback()
