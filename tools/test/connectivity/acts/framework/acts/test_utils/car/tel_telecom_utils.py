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

# Defines utilities that can be used for making calls independent of
# subscription IDs. This can be useful when making calls over mediums not SIM
# based.

import queue
import time

from acts import logger
from acts.test_utils.tel import tel_defines

def dial_number(log, ad, uri):
    """Dial a Tel: URI

    Args:
        log: log object
        ad: android device object
        uri: Tel uri dial

    Returns:
        True if success, False if fail.
    """
    if "tel:" not in uri:
        uri = "tel:" + uri
    log.info("Dialing up droid {} call uri {}".format(
        ad.serial, uri))

    # Start tracking updates.
    ad.droid.telecomStartListeningForCallAdded()

    ad.droid.telecomCallTelUri(uri)

    event = None
    try:
        event = ad.ed.pop_event(
            tel_defines.EventTelecomCallAdded,
            tel_defines.MAX_WAIT_TIME_CONNECTION_STATE_UPDATE)
    except queue.Empty:
        log.info(
            "Did not get {} event!".format(tel_defines.EventTelecomCallAdded))
        # Return failure.
        return False
    finally:
        ad.droid.telecomStopListeningForCallAdded()

    call_id = event['data']['CallId']
    log.info("Call ID: {} dev {}".format(call_id, ad.serial))

    if not call_id:
        log.info("CallId is empty!")
        return False
    if not wait_for_dialing(log, ad):
        return False

    return call_id

def wait_for_call_state(log, ad, call_id, state):
    """Wait for the given call id to transition to the given call state.

    Args:
        log: log object
        ad: android device object
        call_id: ID of the call that we're waiting for the call state to
        transition into.
        state: desired final state.

    Returns:
        True if success, False if fail.
    """
    # Lets track the call now.
    ad.droid.telecomCallStartListeningForEvent(
        call_id, tel_defines.EVENT_CALL_STATE_CHANGED)

    # We may have missed the update so do a quick check.
    if ad.droid.telecomCallGetCallState(call_id) == state:
        log.info("Call ID {} already in {} dev {}!".format(
            call_id, state, ad.serial))
        return state

    # If not then we need to poll for the event.
    try:
        event = ad.ed.pop_event(
            tel_defines.EventTelecomCallStateChanged,
            tel_defines.MAX_WAIT_TIME_CONNECTION_STATE_UPDATE)
        call_state = event['data']['Event']
    except queue.Empty:
        log.info("Did not get into state {} dev {}".format(
            state, ad.serial))
        return None
    finally:
        ad.droid.telecomCallStopListeningForEvent(call_id,
            tel_defines.EventTelecomCallStateChanged)
    return call_state

def hangup_call(log, ad, call_id):
    """Hangup a number

    Args:
        log: log object
        ad: android device object
        call_id: Call to hangup.

    Returns:
        True if success, False if fail.
    """
    log.info("Hanging up droid {} call {}".format(
        ad.serial, call_id))
    # First check that we are in call, otherwise fail.
    if not ad.droid.telecomIsInCall():
        log.info("We are not in-call {}".format(ad.serial))
        return False

    # Make sure we are registered with the events.
    ad.droid.telecomStartListeningForCallRemoved()

    # Disconnect call.
    ad.droid.telecomCallDisconnect(call_id)

    # Wait for removed event.
    event = None
    try:
        event = ad.ed.pop_event(
            tel_defines.EventTelecomCallRemoved,
            tel_defines.MAX_WAIT_TIME_CONNECTION_STATE_UPDATE)
    except queue.Empty:
        log.info("Did not get TelecomCallRemoved event")
        return False
    finally:
        ad.droid.telecomStopListeningForCallRemoved()

    log.info("Removed call {}".format(event))
    if event['data']['CallId'] != call_id:
        return False

    return True

def wait_for_not_in_call(log, ad):
    """Wait for the droid to be OUT OF CALLING state.

    Args:
        log: log object
        ad: android device object

    Returns:
        True if success, False if fail.
    """
    ad.droid.telecomStartListeningForCallRemoved()

    calls = ad.droid.telecomCallGetCallIds()

    timeout = time.time() + tel_defines.MAX_WAIT_TIME_CONNECTION_STATE_UPDATE
    remaining_time = tel_defines.MAX_WAIT_TIME_CONNECTION_STATE_UPDATE

    if len(calls) == 0:
        log.info("Got calls {} for {}".format(
            calls, ad.serial))
        try:
            while len(calls) > 0:
                event = ad.ed.pop_event(
                    tel_defines.EventTelecomCallRemoved,
                    remaining_time)
                remaining_time = timeout - time.time()
                if (remaining_time <= 0
                        or len(ad.droid.telecomCallGetCallIds() == 0)):
                    break

        except queue.Empty:
            log.info("wait_for_not_in_call Did not get {} droid {}".format(
                tel_defines.EventTelecomCallRemoved,
                ad.serial))
            return False
        finally:
            ad.droid.telecomStopListeningForCallRemoved()

    # Either we removed the only call or we never had a call previously, either
    # ways simply check if we are in in call now.
    return (not ad.droid.telecomIsInCall())

def wait_for_dialing(log, ad):
    """Wait for the droid to be in dialing state.

    Args:
        log: log object
        ad: android device object

    Returns:
        True if success, False if fail.
    """
    # Start listening for events before anything else happens.
    ad.droid.telecomStartListeningForCallAdded()

    call_id = None
    # Now check if we already have calls matching the state.
    calls_in_state = get_calls_in_states(log, ad,
                                         [tel_defines.CALL_STATE_CONNECTING,
                                         tel_defines.CALL_STATE_DIALING])

    # If not then we need to poll for the calls themselves.
    if len(calls_in_state) == 0:
        event = None
        try:
            event = ad.ed.pop_event(
                tel_defines.EventTelecomCallAdded,
                tel_defines.MAX_WAIT_TIME_CONNECTION_STATE_UPDATE)
        except queue.Empty:
            log.info("Did not get {}".format(
                tel_defines.EventTelecomCallAdded))
            return False
        finally:
            ad.droid.telecomStopListeningForCallAdded()
        call_id = event['data']['CallId']
    else:
        call_id = calls_in_state[0]

    # We may still not be in-call if the call setup is going on.
    # We wait for the call state to move to dialing.
    log.info("call id {} droid {}".format(call_id, ad.serial))
    curr_state = wait_for_call_state(
        log, ad, call_id, tel_defines.CALL_STATE_DIALING)
    if curr_state == tel_defines.CALL_STATE_CONNECTING:
        log.info("Got connecting state now waiting for dialing state.")
        if wait_for_call_state(
            log, ad, call_id, tel_defines.CALL_STATE_DIALING) != \
            tel_defines.CALL_STATE_DIALING:
            return False
    else:
        if curr_state != tel_defines.CALL_STATE_DIALING:
            log.info("First state is not dialing")
            return False

    # Finally check the call state.
    log.info("wait_for_dialing returns " + str(ad.droid.telecomIsInCall()) +
             " " + str(ad.serial))
    return ad.droid.telecomIsInCall()

def wait_for_ringing(log, ad):
    """Wait for the droid to be in ringing state.

    Args:
        log: log object
        ad: android device object

    Returns:
        True if success, False if fail.
    """
    log.info("waiting for ringing {}".format(ad.serial))
    # Start listening for events before anything else happens.
    ad.droid.telecomStartListeningForCallAdded()

    # First check if we re in call, then simply return.
    if ad.droid.telecomIsInCall():
        log.info("Device already in call {}".format(ad.serial))
        ad.droid.telecomStopListeningForCallAdded()
        return True

    call_id = None
    # Now check if we already have calls matching the state.
    calls_in_state = ad.droid.telecomCallGetCallIds()

    if len(calls_in_state) == 0:
        event = None
        try:
            event = ad.ed.pop_event(
                tel_defines.EventTelecomCallAdded,
                tel_defines.MAX_WAIT_TIME_CALLEE_RINGING)
        except queue.Empty:
            log.info("Did not get {} droid {}".format(
                tel_defines.EventTelecomCallAdded,
                ad.serial))
            return False
        finally:
            ad.droid.telecomStopListeningForCallAdded()
        call_id = event['data']['CallId']
        log.info("wait_for_ringing call found {} dev {}".format(
            call_id, ad.serial))
    else:
        call_id = calls_in_state[0]

    # A rining call is ringing when it is added so simply wait for ringing
    # state.
    if wait_for_call_state(
        log, ad, call_id, tel_defines.CALL_STATE_RINGING) != \
        tel_defines.CALL_STATE_RINGING:
        log.info("No ringing call id {} droid {}".format(
            call_id, ad.serial))
        return False
    return True

def get_calls_in_states(log, ad, call_states):
    """Return the list of calls that are any of the states passed in call_states

    Args:
        log: log object
        ad: android device object
        call_states: List of desired call states

    Returns:
        List containing call_ids.
    """
    # Get the list of calls.
    call_ids = ad.droid.telecomCallGetCallIds()
    call_in_state = []
    for call_id in call_ids:
        call = ad.droid.telecomCallGetCallById(call_id)
        log.info("Call id: {} desc: {}".format(call_id, call))
        if call['State'] in call_states:
            log.info("Adding call id {} to result set.".format(call_id))
            call_in_state.append(call_id)
    return call_in_state
