# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Chrome OS Parnter Concole remote actions."""

from __future__ import print_function

import base64
import logging

import common

from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib import utils
from autotest_lib.server.hosts import moblab_host
from autotest_lib.site_utils import pubsub_utils
from autotest_lib.site_utils import cloud_console_pb2 as cpcon


_PUBSUB_TOPIC = global_config.global_config.get_config_value(
        'CROS', 'cloud_notification_topic', default=None)

# Current notification version.
CURRENT_MESSAGE_VERSION = '1'

# Test upload pubsub notification attributes
LEGACY_ATTR_VERSION = 'version'
LEGACY_ATTR_GCS_URI = 'gcs_uri'
LEGACY_ATTR_MOBLAB_MAC = 'moblab_mac_address'
LEGACY_ATTR_MOBLAB_ID = 'moblab_id'
# the message data for new test result notification.
LEGACY_TEST_OFFLOAD_MESSAGE = 'NEW_TEST_RESULT'


def is_cloud_notification_enabled():
    """Checks if cloud pubsub notification is enabled.

    @returns: True if cloud pubsub notification is enabled. Otherwise, False.
    """
    return  global_config.global_config.get_config_value(
        'CROS', 'cloud_notification_enabled', type=bool, default=False)


def _get_message_type_name(message_type_enum):
    """Gets the message type name from message type enum.

    @param message_type_enum: The message type enum.

    @return The corresponding message type name as string, or 'MSG_UNKNOWN'.
    """
    return cpcon.MessageType.Name(message_type_enum)


def _get_attribute_name(attribute_enum):
    """Gets the message attribute name from attribte enum.

    @param attribute_enum: The attribute enum.

    @return The corresponding attribute name as string, or 'ATTR_INVALID'.
    """
    return cpcon.MessageAttribute.Name(attribute_enum)


class CloudConsoleClient(object):
    """The remote interface to the Cloud Console."""
    def send_heartbeat(self):
        """Sends a heartbeat.

        @returns True if the notification is successfully sent.
            Otherwise, False.
        """
        pass

    def send_event(self, event_type=None, event_data=None):
        """Sends an event notification to the remote console.

        @param event_type: The event type that is defined in the protobuffer
            file 'cloud_console.proto'.
        @param event_data: The event data.

        @returns True if the notification is successfully sent.
            Otherwise, False.
        """
        pass

    def send_log(self, msg, level=None, session_id=None):
        """Sends a log message to the remote console.

        @param msg: The log message.
        @param level: The logging level.
        @param session_id: The current session id.

        @returns True if the notification is successfully sent.
            Otherwise, False.
        """
        pass

    def send_alert(self, msg, level=None, session_id=None):
        """Sends an alert to the remote console.

        @param msg: The alert message.
        @param level: The logging level.
        @param session_id: The current session id.

        @returns True if the notification is successfully sent.
            Otherwise, False.
        """
        pass

    def send_test_job_offloaded_message(self, gcs_uri):
        """Sends a test job offloaded message to the remote console.

        @param gcs_uri: The test result Google Cloud Storage URI.

        @returns True if the notification is successfully sent.
            Otherwise, False.
        """
        pass


# Make it easy to mock out
def _create_pubsub_client(credential):
    return pubsub_utils.PubSubClient(credential)


class PubSubBasedClient(CloudConsoleClient):
    """A Cloud PubSub based implementation of the CloudConsoleClient interface.
    """
    def __init__(
            self,
            credential=moblab_host.MOBLAB_SERVICE_ACCOUNT_LOCATION,
            pubsub_topic=_PUBSUB_TOPIC):
        """Constructor.

        @param credential: The service account credential filename. Default to
            '/home/moblab/.service_account.json'.
        @param pubsub_topic: The cloud pubsub topic name to use.
        """
        super(PubSubBasedClient, self).__init__()
        self._pubsub_client = _create_pubsub_client(credential)
        self._pubsub_topic = pubsub_topic


    def _create_message(self, data, msg_attributes):
        """Creates a cloud pubsub notification object.

        @param data: The message data as a string.
        @param msg_attributes: The message attribute map.

        @returns: A pubsub message object with data and attributes.
        """
        message = {}
        if data:
            message['data'] = data
        if msg_attributes:
            message['attributes'] = msg_attributes
        return message

    def _create_message_attributes(self, message_type_enum):
        """Creates a cloud pubsub notification message attribute map.

        Fills in the version, moblab mac address, and moblab id information
        as attributes.

        @param message_type_enum The message type enum.

        @returns: A pubsub messsage attribute map.
        """
        msg_attributes = {}
        msg_attributes[_get_attribute_name(cpcon.ATTR_MESSAGE_TYPE)] = (
                _get_message_type_name(message_type_enum))
        msg_attributes[_get_attribute_name(cpcon.ATTR_MESSAGE_VERSION)] = (
                CURRENT_MESSAGE_VERSION)
        msg_attributes[_get_attribute_name(cpcon.ATTR_MOBLAB_MAC_ADDRESS)] = (
                utils.get_moblab_serial_number())
        msg_attributes[_get_attribute_name(cpcon.ATTR_MOBLAB_ID)] = (
                utils.get_moblab_id())
        return msg_attributes

    def _create_test_job_offloaded_message(self, gcs_uri):
        """Construct a test result notification.

        TODO(ntang): switch LEGACY to new message format.
        @param gcs_uri: The test result Google Cloud Storage URI.

        @returns The notification message.
        """
        data = base64.b64encode(LEGACY_TEST_OFFLOAD_MESSAGE)
        msg_attributes = {}
        msg_attributes[LEGACY_ATTR_VERSION] = CURRENT_MESSAGE_VERSION
        msg_attributes[LEGACY_ATTR_MOBLAB_MAC] = (
                utils.get_moblab_serial_number())
        msg_attributes[LEGACY_ATTR_MOBLAB_ID] = utils.get_moblab_id()
        msg_attributes[LEGACY_ATTR_GCS_URI] = gcs_uri

        return self._create_message(data, msg_attributes)


    def send_test_job_offloaded_message(self, gcs_uri):
        """Notify the cloud console a test job is offloaded.

        @param gcs_uri: The test result Google Cloud Storage URI.

        @returns True if the notification is successfully sent.
            Otherwise, False.
        """
        logging.info('Notification on gcs_uri %s', gcs_uri)
        message = self._create_test_job_offloaded_message(gcs_uri)
        return self._publish_notification(message)


    def _publish_notification(self, message):
        msg_ids = self._pubsub_client.publish_notifications(
                self._pubsub_topic, [message])

        if msg_ids:
            logging.debug('Successfully sent out a notification')
            return True
        logging.warning('Failed to send notification %s', str(message))
        return False

    def send_heartbeat(self):
        """Sends a heartbeat.

        @returns True if the heartbeat notification is successfully sent.
            Otherwise, False.
        """
        logging.info('Sending a heartbeat')

        event = cpcon.Heartbeat()
        # Don't sent local timestamp for now.
        data = event.SerializeToString()
        try:
            attributes = self._create_message_attributes(
                    cpcon.MSG_MOBLAB_HEARTBEAT)
            message = self._create_message(data, attributes)
        except ValueError:
            logging.exception('Failed to create message.')
            return False
        return self._publish_notification(message)

    def send_event(self, event_type=None, event_data=None):
        """Sends an event notification to the remote console.

        @param event_type: The event type that is defined in the protobuffer
            file 'cloud_console.proto'.
        @param event_data: The event data.

        @returns True if the notification is successfully sent.
            Otherwise, False.
        """
        logging.info('Send an event.')
        if not event_type:
            logging.info('Failed to send event without a type.')
            return False

        event = cpcon.RemoteEventMessage()
        if event_data:
            event.data = event_data
        else:
            event.data = ''
        event.type = event_type
        data = event.SerializeToString()
        try:
            attributes = self._create_message_attributes(
                    cpcon.MSG_MOBLAB_REMOTE_EVENT)
            message = self._create_message(data, attributes)
        except ValueError:
            logging.exception('Failed to create message.')
            return False
        return self._publish_notification(message)
