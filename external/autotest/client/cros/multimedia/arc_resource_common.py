# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This module is shared by client and server."""

class MicrophoneProps(object):
    """Properties of microphone app that both server and client should know."""
    TEST_NAME = 'cheets_MicrophoneApp'
    READY_TAG_FILE = '/tmp/.cheets_MicrophoneApp.ready'
    EXIT_TAG_FILE = '/tmp/.cheets_MicrophoneApp.exit'
    # Allow some time for microphone app startup and recorder preparation.
    RECORD_FUZZ_SECS  = 5
    # The recording duration is fixed in microphone app.
    RECORD_SECS = 10


class PlayMusicProps(object):
    """Properties of Play Music app that both server and client should know."""
    TEST_NAME = 'cheets_PlayMusicApp'
    READY_TAG_FILE = '/tmp/.cheets_PlayMusicApp.ready'
    EXIT_TAG_FILE = '/tmp/.cheets_PlayMusicApp.exit'


class PlayVideoProps(object):
    """Properties of Play Video app that both server and client should know."""
    TEST_NAME = 'cheets_PlayVideoApp'
    READY_TAG_FILE = '/tmp/.cheets_PlayVideoApp.ready'
    EXIT_TAG_FILE = '/tmp/.cheets_PlayVideoApp.exit'
