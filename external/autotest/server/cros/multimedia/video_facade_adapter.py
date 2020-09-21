# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""An adapter to remotely access the video facade on DUT."""

import os
import tempfile


class VideoFacadeRemoteAdapter(object):
    """VideoFacadeRemoteAdapter is an adapter to remotely control DUT video.

    The Autotest host object representing the remote DUT, passed to this
    class on initialization, can be accessed from its _client property.

    """
    def __init__(self, host, remote_facade_proxy):
        """Construct a VideoFacadeRemoteAdapter.

        @param host: Host object representing a remote host.
        @param remote_facade_proxy: RemoteFacadeProxy object.

        """
        self._client = host
        self._proxy = remote_facade_proxy


    @property
    def _video_proxy(self):
        """Gets the proxy to DUT video facade.

        @return XML RPC proxy to DUT video facade.

        """
        return self._proxy.video


    def send_playback_file(self, path):
        """Copies a file to client.

        The files on the client side will be deleted by VideoFacadeNative
        after the test.

        @param path: A path to the file.

        @returns: A new path to the file on client.

        """
        _, ext = os.path.splitext(path)
        _, client_file_path = tempfile.mkstemp(
                prefix='playback_', suffix=ext)
        self._client.send_file(path, client_file_path)
        return client_file_path


    def prepare_playback(self, file_path, fullscreen=True):
        """Copies the html file and the video file to /tmp and load the webpage.

        @param file_path: The path to the file.
        @param fullscreen: Plays the video in fullscreen.

        """
        client_file_path = self.send_playback_file(file_path)
        self._video_proxy.prepare_playback(client_file_path, fullscreen)


    def start_playback(self, blocking=False):
        """Starts video playback on the webpage.

        Before calling this method, user should call prepare_playback to
        put the files to /tmp and load the webpage.

        @param blocking: Blocks this call until playback finishes.

        """
        self._video_proxy.start_playback(blocking)


    def pause_playback(self):
        """Pauses playback on the webpage."""
        self._video_proxy.pause_playback()


    def dropped_frame_count(self):
        """
        Gets the number of dropped frames.

        @returns: An integer indicates the number of dropped frame.

        """
        return self._video_proxy.dropped_frame_count()


    def prepare_arc_playback(self, file_path, fullscreen=True):
        """Copies the video file to be played into container.

        User should call this method to put the file into container before
        calling start_arc_playback.

        @param file_path: Path to the file to be played on Server.
        @param fullscreen: Plays the video in fullscreen.

        """
        client_file_path = self.send_playback_file(file_path)
        self._video_proxy.prepare_arc_playback(client_file_path, fullscreen)


    def start_arc_playback(self, blocking_secs=None):
        """Starts playback through Play Video app.

        Before calling this method, user should call set_arc_playback_file to
        put the file into container and start the app.

        @param blocking_secs: A positive number indicates the timeout to wait
                              for the playback is finished. Set None to make
                              it non-blocking.

        """
        self._video_proxy.start_arc_playback(blocking_secs)


    def pause_arc_playback(self):
        """Pauses playback through Play Video app."""
        self._video_proxy.pause_arc_playback()


    def stop_arc_playback(self):
        """Stops playback through Play Video app."""
        self._video_proxy.stop_arc_playback()
