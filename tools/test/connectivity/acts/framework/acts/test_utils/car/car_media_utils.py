#!/usr/bin/env python3.4
#
#   Copyright 2016 - Google
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

#   Utilities that can be used for testing media related usecases.

# Events dispatched from the RPC Server
EVENT_PLAY_RECEIVED = "playReceived"
EVENT_PAUSE_RECEIVED = "pauseReceived"
EVENT_SKIP_NEXT_RECEIVED = "skipNextReceived"
EVENT_SKIP_PREV_RECEIVED = "skipPrevReceived"

# Passthrough Commands sent to the RPC Server
CMD_MEDIA_PLAY = "play"
CMD_MEDIA_PAUSE = "pause"
CMD_MEDIA_SKIP_NEXT = "skipNext"
CMD_MEDIA_SKIP_PREV = "skipPrev"

# MediaMetaData keys. Keep them the same as in BluetoothMediaFacade.
MEDIA_KEY_TITLE = "keyTitle"
MEDIA_KEY_ALBUM = "keyAlbum"
MEDIA_KEY_ARTIST = "keyArtist"
MEDIA_KEY_DURATION = "keyDuration"
MEDIA_KEY_NUM_TRACKS = "keyNumTracks"


def verifyEventReceived(log, device, event, timeout):
    """
    Verify if the event was received from the given device.
    When a fromDevice talks to a toDevice and expects an event back,
    this util function can be used to see if the toDevice posted it.
    Args:
        log:        The logging object
        device:     The device to pop the event from
        event:      The event we are interested in.
        timeout:    The time in seconds before we timeout
    Returns:
        True        if the event was received
        False       if we timed out waiting for the event
    """
    try:
        device.ed.pop_event(event, timeout)
    except Exception:
        log.info(" {} Event Not received".format(event))
        return False
    log.info("Event Received : {}".format(event))
    return True


def send_media_passthrough_cmd(log,
                               fromDevice,
                               toDevice,
                               cmd,
                               expctEvent,
                               timeout=1.0):
    """
    Send a media passthrough command from one device to another
    via bluetooth.
    Args:
        log:        The logging object
        fromDevice: The device to send the command from
        toDevice:   The device the command is sent to
        cmd:        The passthrough command to send
        expctEvent: The expected event
        timeout:    The time in seconds before we timeout, deafult = 1sec
    Returns:
        True        if the event was received
        False       if we timed out waiting for the event
    """
    log.info("Sending passthru : {}".format(cmd))
    fromDevice.droid.bluetoothMediaPassthrough(cmd)
    return verifyEventReceived(log, toDevice, expctEvent, timeout)


def log_metadata(log, metadata):
    """
    Log the Metadata to the console.
    Args:
        log:        The logging object
        metadata:   Dictionary of the song's metadata
    """
    title = metadata[MEDIA_KEY_TITLE]
    album = metadata[MEDIA_KEY_ALBUM]
    artist = metadata[MEDIA_KEY_ARTIST]
    duration = metadata[MEDIA_KEY_DURATION]
    numTracks = metadata[MEDIA_KEY_NUM_TRACKS]
    log.info("Playing Artist: {}, Album: {}, Title: {}".format(artist, album,
                                                               title))
    log.info("Duration: {}, NumTracks: {}".format(duration, numTracks))


def compare_metadata(log, metadata1, metadata2):
    """
    Compares the Metadata between the two devices
    Args:
        log:        The logging object
        metadata1    Media Metadata of device1
        metadata2    Media Metadata of device2
    Returns:
        True        if the Metadata matches
        False       if the Metadata do not match
    """
    log.info("Device1 metadata:")
    log_metadata(log, metadata1)
    log.info("Device2 metadata:")
    log_metadata(log, metadata2)

    if not (metadata1[MEDIA_KEY_TITLE] == metadata2[MEDIA_KEY_TITLE]):
        log.info("Song Titles do not match")
        return False

    if not (metadata1[MEDIA_KEY_ALBUM] == metadata2[MEDIA_KEY_ALBUM]):
        log.info("Song Albums do not match")
        return False

    if not (metadata1[MEDIA_KEY_ARTIST] == metadata2[MEDIA_KEY_ARTIST]):
        log.info("Song Artists do not match")
        return False

    if not (metadata1[MEDIA_KEY_DURATION] == metadata2[MEDIA_KEY_DURATION]):
        log.info("Song Duration do not match")
        return False

    if not (metadata1[MEDIA_KEY_NUM_TRACKS] == metadata2[MEDIA_KEY_NUM_TRACKS]
    ):
        log.info("Song Num Tracks do not match")
        return False

    return True


def check_metadata(log, device1, device2):
    """
    Gets the now playing metadata from 2 devices and checks if they are the same
    Args:
        log:        The logging object
        device1     Device 1
        device2     Device 2
    Returns:
        True        if the Metadata matches
        False       if the Metadata do not match
    """
    metadata1 = device1.droid.bluetoothMediaGetCurrentMediaMetaData()
    if metadata1 is None:
        return False

    metadata2 = device2.droid.bluetoothMediaGetCurrentMediaMetaData()
    if metadata2 is None:
        return False
    return compare_metadata(log, metadata1, metadata2)


def isMediaSessionActive(log, device, mediaSession):
    """
    Checks if the passed mediaSession is active.
    Used to see if the device is playing music.
    Args:
        log:            The logging object
        device          Device to check
        mediaSession    MediaSession to check if it is active
    Returns:
        True            if the given mediaSession is active
        False           if the given mediaSession is not active
    """
    # Get a list of MediaSession tags (String) that is currently active
    activeSessions = device.droid.bluetoothMediaGetActiveMediaSessions()
    if len(activeSessions) > 0:
        for session in activeSessions:
            log.info(session)
            if (session == mediaSession):
                return True
    log.info("No Media Sessions")
    return False
