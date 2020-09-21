# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Meeting related operations"""

from __future__ import print_function

import logging
import random

MIN_VOL = 1
MAX_VOL = 100

def restart_chrome(handle, is_meeting, recovery):
    """
    Restart chrome and wait for telemetry commands to be ready.
    @param handle: CfM telemetry remote facade,
    @param is_meeting: True, None if CfM running MEET mode,
                       False if CfM running hangout mode
    @returns: True, None if success,
              False otherwise.
    """
    try:
        if recovery:
            logging.info('+++Restart chrome')
            handle.restart_chrome_for_cfm()
        if is_meeting:
            handle.wait_for_meetings_telemetry_commands()
        else:
            handle.wait_for_hangouts_telemetry_commands()
    except Exception as e:
        errmsg = 'Fail to run telemetry api restart_chrome_for_cfm.'
        logging.exception(errmsg)
        return False, errmsg
    return True, None

def join_meeting(handle, is_meeting, meet_code):
    """
    Join meeting.
    @param handle: CfM telemetry remote facade,
    @param is_meeting: True, None if CfM running MEET mode,
                       False if CfM running hangout mode
    @param meeting_code: meeting code
    @returns: True, None if CfM joins meeting successfully,
              False otherwise.
    """
    try:
        if is_meeting:
            logging.info('+++Start meet meeting')
            if meet_code:
                handle.join_meeting_session(meet_code)
            else:
                handle.start_meeting_session()
        else:
            logging.info('+++start hangout meeting')
            if meet_code:
                handle.start_new_hangout_session(meet_code)
            else:
                errmsg = 'Meeting code is required for hangout meet.'
                logging.exception(errmsg)
                return False, errmsg
        logging.info('+++Meeting %s joined.', meet_code)
        return True, None
    except Exception as e:
        errmsg = 'Fail to run telemetry api to join meeting.'
        logging.exception(errmsg)
        return False, errmsg

def leave_meeting(handle, is_meeting):
    """
    Leave meeting.
    @param handle: CfM telemetry remote facade,
    @param is_meeting: True, None if CfM running MEET mode,
                       False if CfM running hangout mode
    @returns: True, None if CfM leaves meeting successfully,
              False otherwise.

    """
    try:
        if is_meeting:
            handle.end_meeting_session()
        else:
            handle.end_hangout_session()
    except Exception as e:
        errmsg = 'Fail to run telemetry api to leave meeting.'
        logging.exception(errmsg)
        return False, errmsg
    logging.info('+++meet ended')
    return True, None


def mute_unmute_camera(handle, is_muted):
    """
    @param handle: CfM telemetry remote facade,
    @param is_muted: True, None if camera is muted,
                     False otherwise.
    @returns: True, None if camera is muted/unmuted successfully,
              False otherwise.
    """
    try:
        if is_muted:
            logging.info('+++unmute camera')
            handle.unmute_camera()
        else:
            logging.info('+++mute camera')
            handle.mute_camera()
    except Exception as e:
        errmsg = 'Fail to run telemetry api to mute/unmute camera.'
        logging.exception(errmsg)
        return False, errmsg
    return True, None


def mute_unmute_mic(handle, is_muted):
    """
    @param handle: CfM telemetry remote facade,
    @param is_muted: True, None if mic is muted,
                     False otherwise.
    @returns: True, None if camera is muted/unmuted successfully,
              False otherwise.
    """
    try:
         if is_muted:
             logging.info('+++unmute mic')
             handle.unmute_mic()
         else:
             logging.info('+++mute mic')
             handle.mute_mic()
    except Exception as e:
        errmsg = 'Fail to run telemetry api to mute/unmute mic.'
        logging.exception(errmsg)
        return False, errmsg
    return True, None


def set_speaker_volume(handle, volume):
    """
    Change speaker's volume.
    @param handle: CfM telemetry remote facade
    @param volume: volume for speaker
    """
    try:
        handle.set_speaker_volume(volume)
    except Exception as e:
        errmsg = 'Fail to run telemetry api to set speaker volume.'
        logging.exception(errmsg)
        return False, errmsg
    return True, str(volume)


def speaker_volume_test(handle, step, mode, randommode):
    """
    Change speaker's volume.
    @param handle: CfM telemetry remote facade,
    @param step: volume to be increased or decreased in one call
    @param mode: if it equal to 1, update volume directly to
                 targeted value,
                 else, update volume in multiple calls.
    @param randommode: if True, None, the value of volume to be change in
                 each call is randomly set,
                 else, the value is fixed defined by step.
    """
    test_volume = random.randrange(MIN_VOL, MAX_VOL)
    if mode == 1:
        return set_speaker_volume(handle, test_volume)
    step = max(2, step)
    try:
        current = int(handle.get_speaker_volume())
    except Exception as e:
        errmsg = 'Fail to run telemetry api to set speaker volume.'
        logging.exception(errmsg)
        return False, errmsg

    if test_volume > current:
        while test_volume > current:
            if randommode:
                transit_volume = current + random.randrange(1, step)
            else:
                transit_volume = current + step

            if transit_volume > test_volume:
                transit_volume = test_volume

            handle.set_speaker_volume(transit_volume)
            try:
                current = int(handle.get_speaker_volume())
            except Exception as e:
                errmsg = 'Fail to run telemetry api to set speaker volume.'
                logging.exception(errmsg)
                return False, errmsg

            logging.info('+++set vol %d, current %d, target %d',
                         transit_volume, current, test_volume)
    else:
        while test_volume < current:
            if randommode:
                transit_volume = current - random.randrange(1, step)
            else:
                transit_volume = current - step
            if transit_volume < test_volume:
                transit_volume = test_volume
            handle.set_speaker_volume(transit_volume)
            try:
                current = int(handle.get_speaker_volume())
            except Exception as e:
                errmsg = 'Fail to run telemetry api to set speaker volume.'
                logging.exception(errmsg)
                return False, errmsg

            logging.info('+++set vol %d, current %d, target %d',
                          transit_volume, current, test_volume)
    return True, str(current)
