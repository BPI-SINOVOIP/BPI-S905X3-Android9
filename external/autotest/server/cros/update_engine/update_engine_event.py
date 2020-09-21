# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Update event types.
EVENT_TYPE_DOWNLOAD_COMPLETE = '1'
EVENT_TYPE_INSTALL_COMPLETE = '2'
EVENT_TYPE_UPDATE_COMPLETE = '3'
EVENT_TYPE_DOWNLOAD_STARTED = '13'
EVENT_TYPE_DOWNLOAD_FINISHED = '14'
EVENT_TYPE_REBOOTED_AFTER_UPDATE = '54'

# Update event results.
EVENT_RESULT_ERROR = '0'
EVENT_RESULT_SUCCESS = '1'
EVENT_RESULT_UPDATE_DEFERRED = '9'

# Omaha event types/results, from update_engine/omaha_request_action.h
# These are stored in dict form in order to easily print out the keys.
EVENT_TYPE_DICT = {
    EVENT_TYPE_DOWNLOAD_COMPLETE: 'download_complete',
    EVENT_TYPE_INSTALL_COMPLETE: 'install_complete',
    EVENT_TYPE_UPDATE_COMPLETE: 'update_complete',
    EVENT_TYPE_DOWNLOAD_STARTED: 'download_started',
    EVENT_TYPE_DOWNLOAD_FINISHED: 'download_finished',
    EVENT_TYPE_REBOOTED_AFTER_UPDATE: 'rebooted_after_update'
}

EVENT_RESULT_DICT = {
    EVENT_RESULT_ERROR: 'error',
    EVENT_RESULT_SUCCESS: 'success',
    EVENT_RESULT_UPDATE_DEFERRED: 'update_deferred'
}


def get_event_type(event_type_code):
    """Utility to look up the different event type codes."""
    return EVENT_TYPE_DICT[event_type_code]


def get_event_result(event_result_code):
    """Utility to look up the different event result codes."""
    return EVENT_RESULT_DICT[event_result_code]


class UpdateEngineEvent(object):
    """This class represents a single EXPECTED update engine event.

    This class's data will be compared against an ACTUAL event returned by
    update_engine.
    """

    def __init__(self, event_type=None, event_result=None, version=None,
                 previous_version=None, on_error=None):
        """Initializes an event.

        @param event_type: Expected event type.
        @param event_result: Expected event result code.
        @param version: Expected reported image version.
        @param previous_version: Expected reported previous image version.
        @param on_error: a function to call when the event's data is invalid.
        """
        self._expected_attrs = {
            'event_type': event_type,
            'event_result': event_result,
            'version': version,
            'previous_version': previous_version,
        }
        self._on_error = on_error


    def _verify_event_attribute(self, attr_name, expected_attr_val,
                                actual_attr_val):
        """Compares a single attribute to ensure expected matches actual.

        @param attr_name: name of the attribute to verify.
        @param expected_attr_val: expected attribute value.
        @param actual_attr_val: actual attribute value.

        @return True if actual value is present and matches, False otherwise.
        """
        # None values are assumed to be missing and non-matching.
        if actual_attr_val is None:
            return False

        # We allow expected version numbers (e.g. 2940.0.0) to be contained in
        # actual values (2940.0.0-a1) for developer images.
        if (actual_attr_val == expected_attr_val or
           ('version' in attr_name and expected_attr_val in actual_attr_val)):
            return True

        return False


    def __str__(self):
        """Returns a comma separated list of the event data."""
        return '{%s}' % ', '.join(['%s:%s' % (k, v) for k, v in
                                   self._expected_attrs.iteritems()])

    def equals(self, actual_event):
        """Compares this expected event with an actual event from the update.

        @param actual_event: a dictionary containing event attributes.

        @return An error message, or None if all attributes as expected.
        """
        mismatched_attrs = [
            attr_name for attr_name, expected_attr_val
            in self._expected_attrs.iteritems()
            if (expected_attr_val and
                not self._verify_event_attribute(attr_name, expected_attr_val,
                                                 actual_event.get(attr_name)))]

        if not mismatched_attrs:
            return None

        return self._on_error(self._expected_attrs, actual_event,
                              mismatched_attrs)