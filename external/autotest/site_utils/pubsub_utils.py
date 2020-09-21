# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Script to archive old Autotest results to Google Storage.

Uses gsutil to archive files to the configured Google Storage bucket.
Upon successful copy, the local results directory is deleted.
"""

from __future__ import print_function

import os

from apiclient import discovery
from apiclient import errors
from oauth2client.client import ApplicationDefaultCredentialsError
from oauth2client.client import GoogleCredentials
from chromite.lib import cros_logging as logging

# Cloud service
PUBSUB_SERVICE_NAME = 'pubsub'
PUBSUB_VERSION = 'v1beta2'
PUBSUB_SCOPES = ['https://www.googleapis.com/auth/pubsub']
# number of retry to publish an event.
DEFAULT_PUBSUB_NUM_RETRIES = 3

class PubSubException(Exception):
    """Exception to be raised when the test to push to prod failed."""
    pass


class PubSubClient(object):
    """A generic pubsub client."""
    def __init__(self, credential_file=None):
        """Constructor for PubSubClient.

        Args:
          credential_file: The credential filename.

        Raises:
          PubSubException if the credential file does not exist or corrupted.
        """
        if not credential_file:
            raise PubSubException('You need to specify a credential file.')
        self.credential_file = credential_file
        self.credential = self._get_credential()

    def _get_credential(self):
        """Gets the pubsub service api handle."""
        if not os.path.isfile(self.credential_file):
            logging.error('No credential file found')
            raise PubSubException('Credential file does not exist:' +
                                  self.credential_file)
        try:
            credential = GoogleCredentials.from_stream(self.credential_file)
            if credential.create_scoped_required():
                credential = credential.create_scoped(PUBSUB_SCOPES)
            return credential
        except ApplicationDefaultCredentialsError as ex:
            logging.exception('Failed to get credential:%s', ex)
        except errors.Error as e:
            logging.exception('Failed to get the pubsub service handle:%s', e)

        raise PubSubException('Credential file %s does not exists:' %
                              self.credential_file)

    def _get_pubsub_service(self):
        try:
            return discovery.build(PUBSUB_SERVICE_NAME, PUBSUB_VERSION,
                                   credentials=self.credential)
        except errors.Error as e:
            logging.exception('Failed to get pubsub resource object:%s', e)
            raise PubSubException('Failed to get pubsub resource object')

    def publish_notifications(self, topic, messages=None):
        """Publishes a test result notification to a given pubsub topic.

        @param topic: The Cloud pubsub topic.
        @param messages: A list of notification messages.

        @returns A list of pubsub message ids, and empty if fails.

        @raises PubSubException if failed to publish the notification.
        """
        if not messages:
            return None

        pubsub = self._get_pubsub_service()
        try:
            body = {'messages': messages}
            resp = pubsub.projects().topics().publish(
                topic=topic, body=body).execute(
                    num_retries=DEFAULT_PUBSUB_NUM_RETRIES)
            msgIds = []
            if resp:
                msgIds = resp.get('messageIds')
                if msgIds:
                    logging.debug('Published notification message')
                else:
                    logging.error('Failed to published notification message')
            return msgIds
        except errors.Error as e:
            logging.exception('Failed to publish test result notification:%s',
                    e)
            raise PubSubException('Failed to publish the notification')
