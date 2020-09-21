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

# Defines utilities that can be used for making calls indenpendent of
# subscription IDs. This can be useful when making calls over mediums not SIM
# based.

# Make a phone call to the specified URI. It is assumed that we are making the
# call to the user selected default account.
#
# We usually want to make sure that the call has ended up in a good state.
#
# NOTE: This util is applicable to only non-conference type calls. It is best
# suited to test cases where only one call is in action at any point of time.

import queue
import time

from acts import logger
from acts.test_utils.tel import tel_defines

def dial_number(log, ad, uri):
    """Dial a number

    Args:
        log: log object
        ad: android device object
        uri: Tel number to dial

    Returns:
        True if success, False if fail.
    """
    log.info("Dialing up droid {} call uri {}".format(
        ad.serial, uri))

    # First check that we are not in call.
    if ad.droid.telecomIsInCall():
        log.info("We're still in call {}".format(ad.serial))
        return False

    # Start tracking updates.
    ad.droid.telecomStartListeningForCallAdded()
    #If a phone number is passed in
    if "tel:" not in uri:
        uri = "tel:" + uri
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
    # NOTE: Disable this everywhere we return.
    ad.droid.telecomCallStartListeningForEvent(
        call_id, tel_defines.EVENT_CALL_STATE_CHANGED)

    # We may have missed the update so do a quick check.
    if ad.droid.telecomCallGetCallState(call_id) == state:
        log.info("Call ID {} already in {} dev {}!".format(
            call_id, state, ad.serial))
        ad.droid.telecomCallStopListeningForEvent(call_id,
            tel_defines.EventTelecomCallStateChanged)
        return True

    # If not then we need to poll for the event.
    # We return if we have found a match or we timeout for further updates of
    # the call
    end_time = time.time() + 10
    while True and time.time() < end_time:
        try:
            event = ad.ed.pop_event(
                tel_defines.EventTelecomCallStateChanged,
                tel_defines.MAX_WAIT_TIME_CONNECTION_STATE_UPDATE)
            call_state = event['data']['Event']
            if call_state == state:
                ad.droid.telecomCallStopListeningForEvent(call_id,
                    tel_defines.EventTelecomCallStateChanged)
                return True
            else:
                log.info("Droid {} in call {} state {}".format(
                    ad.serial, call_id, call_state))
                continue
        except queue.Empty:
            log.info("Did not get into state {} dev {}".format(
                state, ad.serial))
            ad.droid.telecomCallStopListeningForEvent(call_id,
                tel_defines.EventTelecomCallStateChanged)
            return False
    return False

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

def hangup_conf(log, ad, conf_id):
    """Hangup a conference call

    Args:
        log: log object
        ad: android device object
        conf_id: Conf call to hangup.

    Returns:
        True if success, False if fail.
    """
    log.info("hangup_conf: Hanging up droid {} call {}".format(
        ad.serial, conf_id))

    # First check that we are in call, otherwise fail.
    if not ad.droid.telecomIsInCall():
        log.info("We are not in-call {}".format(ad.serial))
        return False

    # Get the list of children for this conference.
    all_calls = get_call_id_children(log, ad, conf_id)

    # All calls that needs disconnecting (Parent + Children)
    all_calls.add(conf_id)

    # Make sure we are registered with the events.
    ad.droid.telecomStartListeningForCallRemoved()

    # Disconnect call.
    ad.droid.telecomCallDisconnect(conf_id)

    # Wait for removed event.
    while len(all_calls) > 0:
        event = None
        try:
            event = ad.ed.pop_event(
                tel_defines.EventTelecomCallRemoved,
                tel_defines.MAX_WAIT_TIME_CONNECTION_STATE_UPDATE)
        except queue.Empty:
            log.info("Did not get TelecomCallRemoved event")
            ad.droid.telecomStopListeningForCallRemoved()
            return False

        removed_call_id = event['data']['CallId']
        all_calls.remove(removed_call_id)
        log.info("Removed call {} left calls {}".format(removed_call_id, all_calls))

    ad.droid.telecomStopListeningForCallRemoved()
    return True

def accept_call(log, ad, call_id):
    """Accept a number

    Args:
        log: log object
        ad: android device object
        call_id: Call to accept.

    Returns:
        True if success, False if fail.
    """
    log.info("Accepting call at droid {} call {}".format(
        ad.serial, call_id))
    # First check we are in call, otherwise fail.
    if not ad.droid.telecomIsInCall():
        log.info("We are not in-call {}".format(ad.serial))
        return False

    # Accept the call and wait for the call to be accepted on AG.
    ad.droid.telecomCallAnswer(call_id, tel_defines.VT_STATE_AUDIO_ONLY)
    if not wait_for_call_state(log, ad, call_id, tel_defines.CALL_STATE_ACTIVE):
        log.error("Call {} on droid {} not active".format(
            call_id, ad.serial))
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
    if len(calls) > 1:
        log.info("More than one call {} {}".format(calls, ad.serial))
        return False

    if len(calls) > 0:
        log.info("Got calls {} for {}".format(
            calls, ad.serial))
        try:
            event = ad.ed.pop_event(
                tel_defines.EventTelecomCallRemoved,
                tel_defines.MAX_WAIT_TIME_CONNECTION_STATE_UPDATE)
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

    # First check if we re in call, then simply return.
    if ad.droid.telecomIsInCall():
        ad.droid.telecomStopListeningForCallAdded()
        return True

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
    if not wait_for_call_state(
        log, ad, call_id, tel_defines.CALL_STATE_DIALING):
        return False

    # Finally check the call state.
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

    for c_id in calls_in_state:
        if ad.droid.telecomCallGetCallState(c_id) == tel_defines.CALL_STATE_RINGING:
            return True

    event = None
    call_id = None
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

    # If the call already existed then we would have returned above otherwise
    # we will verify that the newly added call is indeed ringing.
    if not wait_for_call_state(
        log, ad, call_id, tel_defines.CALL_STATE_RINGING):
        log.info("No ringing call id {} droid {}".format(
            call_id, ad.serial))
        return False
    return True

def wait_for_active(log, ad):
    """Wait for the droid to be in active call state.

    Args:
        log: log object
        ad: android device object

    Returns:
        True if success, False if fail.
    """
    log.info("waiting for active {}".format(ad.serial))
    # Start listening for events before anything else happens.
    ad.droid.telecomStartListeningForCallAdded()

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

    # We have found a new call to be added now wait it to transition into
    # active state.
    if not wait_for_call_state(
        log, ad, call_id, tel_defines.CALL_STATE_ACTIVE):
        log.info("No active call id {} droid {}".format(
            call_id, ad.serial))
        return False
    return True

def wait_for_conference(log, ad, conf_calls):
    """Wait for the droid to be in a conference with calls specified
    in conf_calls.

    Args:
        log: log object
        ad: android device object
        conf_calls: List of calls that should transition to conference

    Returns:
        call_id if success, None if fail.
    """
    conf_calls = set(conf_calls)

    log.info("waiting for conference {}".format(ad.serial))
    ad.droid.telecomStartListeningForCallAdded()

    call_ids = ad.droid.telecomCallGetCallIds()

    # Check if we have a conference call and if the children match
    for call_id in call_ids:
        call_chld = get_call_id_children(log, ad, call_id)
        if call_chld == conf_calls:
            ad.droid.telecomStopListeningForCallAdded()
            return call_id

    # If not poll for new calls.
    event = None
    call_id = None
    try:
        event = ad.ed.pop_event(
            tel_defines.EventTelecomCallAdded,
            tel_defines.MAX_WAIT_TIME_CALLEE_RINGING)
        log.info("wait_for_conference event {} droid {}".format(
            event, ad.serial))
    except queue.Empty:
        log.error("Did not get {} droid {}".format(
            tel_defines.EventTelecomCallAdded,
            ad.serial))
        return None
    finally:
        ad.droid.telecomStopListeningForCallAdded()
    call_id = event['data']['CallId']

    # Now poll until the children change.
    ad.droid.telecomCallStartListeningForEvent(
        call_id, tel_defines.EVENT_CALL_CHILDREN_CHANGED)

    event = None
    while True:
        try:
            event = ad.ed.pop_event(
                tel_defines.EventTelecomCallChildrenChanged,
                tel_defines.MAX_WAIT_TIME_CONNECTION_STATE_UPDATE)
            call_chld = set(event['data']['Event'])
            log.info("wait_for_conference children chld event {}".format(call_chld))
            if call_chld == conf_calls:
                ad.droid.telecomCallStopListeningForEvent(
                    call_id, tel_defines.EVENT_CALL_CHILDREN_CHANGED)
                return call_id
        except queue.Empty:
            log.error("Did not get {} droid {}".format(
                tel_defines.EventTelecomCallChildrenChanged, ad.serial))
            ad.droid.telecomCallStopListeningForEvent(
                call_id, tel_defines.EVENT_CALL_CHILDREN_CHANGED)
            return None

def get_call_id_children(log, ad, call_id):
    """Return the list of calls that are children to call_id

    Args:
        log: log object
        ad: android device object
        call_id: Conference call id

    Returns:
        List containing call_ids.
    """
    call = ad.droid.telecomCallGetCallById(call_id)
    call_chld = set(call['Children'])
    log.info("get_call_id_children droid {} call {} children {}".format(
        ad.serial, call, call_chld))
    return call_chld

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
