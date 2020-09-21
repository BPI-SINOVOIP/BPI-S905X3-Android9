# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.common_lib import error
from autotest_lib.server import autotest, test
from autotest_lib.client.common_lib.cros import tpm_utils


class firmware_Cr50InvalidateRW(test.test):
    """
    Verify the inactive Cr50 header on the first login after cryptohome
    restarts.

    There are two special cases this test covers: logging in after the TPM
    owner is cleared and logging in as guest.

    After the tpm owner is cleared, corrupting the header will be done on
    the second login. During guest login the owner wont be cleared.
    """
    version = 1

    GET_CRYPTOHOME_MESSAGE ='grep cryptohomed /var/log/messages'
    SUCCESS = 'Successfully invalidated inactive Cr50 RW'
    FAIL = 'Invalidating inactive Cr50 RW failed'
    NO_ATTEMPT = 'Did not try to invalidate header'
    LOGIN_ATTEMPTS = 5

    def initialize(self, host):
        super(firmware_Cr50InvalidateRW, self).initialize()

        self.host = host
        self.client_at = autotest.Autotest(self.host)

        self.last_message = None
        # get the messages already in /var/log/messages so we don't use them
        # later.
        self.check_for_invalidated_rw()


    def check_for_invalidated_rw(self):
        """Use /var/log/messages to see if the rw header was invalidated.

        Returns a string NO_ATTEMPT if cryptohome did not try to invalidate the
        header or the cryptohome message if the attempt failed or succeeded.
        """
        # Get the relevant messages from /var/log/messages
        message_str = self.host.run(self.GET_CRYPTOHOME_MESSAGE,
                                    verbose=False).stdout.strip()

        # Remove the messages we have seen in the past
        if self.last_message:
            message_str = message_str.rsplit(self.last_message, 1)[-1]
        messages = message_str.split('\n')

        # Save the last message so we can identify new messages later
        self.last_message = messages[-1]

        rv = self.NO_ATTEMPT
        # print all cryptohome messages.
        for message in messages:
            logging.debug(message)
            # Return the message that is related to the RW invalidate attempt
            if self.FAIL in message or self.SUCCESS in message:
                rv = message
        return rv


    def login(self, use_guest):
        """Run the test to login."""
        if use_guest:
            self.client_at.run_test('login_CryptohomeIncognito')
        else:
            self.client_at.run_test('login_LoginSuccess')


    def login_and_verify(self, use_guest=False, corrupt_login=None):
        """Verify the header is only invalidated on the specified login.

        login LOGIN_ATTEMPTS times. Verify that cryptohome only tries to corrupt
        the inactive cr50 header on the specified login. If it tries on a
        different login or fails to corrupt the header, raise an error.

        Args:
            use_guest: True to login as guest
            corrupt_login: The login attempt that we expect the header to be
                           corrupted on

        Raises:
            TestError if the system attempts to corrupt the header on any login
            that isn't corrupt_login or if an attepmt to corrupt the header
            fails.
        """
        for i in xrange(self.LOGIN_ATTEMPTS):
            attempt = i + 1

            self.login(use_guest)
            result = self.check_for_invalidated_rw()

            message = '%slogin %d: %s' % ('guest ' if use_guest else '',
                                          attempt, result)
            logging.info(message)

            # Anytime the invalidate attempt fails raise an error
            if self.FAIL in result:
                raise error.TestError(message)

            # The header should be invalidated only on corrupt_login. Raise
            # an error if it was invalidated on some other login or if
            # cryptohome did not try on the first one.
            if (attempt == corrupt_login) != (self.SUCCESS in result):
                raise error.TestError('Unexpected result %s' % message)


    def restart_cryptohome(self):
        """Restart cryptohome

        Cryptohome only sends the command to corrupt the header once. Once it
        has been sent it wont be sent again until cryptohome is restarted.
        """
        self.host.run('restart cryptohomed')


    def clear_tpm_owner(self):
        """Clear the tpm owner."""
        logging.info('Clearing the TPM owner')
        tpm_utils.ClearTPMOwnerRequest(self.host)


    def after_run_once(self):
        """Print the run information after each successful run"""
        logging.info('finished run %d', self.iteration)


    def run_once(self, host):
        # After clearing the tpm owner the header will be corrupted on the
        # second login
        self.clear_tpm_owner()
        self.login_and_verify(corrupt_login=2)

        # The header is corrupted on the first login after cryptohome is reset
        self.restart_cryptohome()
        self.login_and_verify(corrupt_login=1)

        # Cryptohome is reset after reboot
        self.host.reboot()
        self.login_and_verify(corrupt_login=1)

        # The header is not corrupted after guest login, but will be corrupted
        # on the first login after that.
        self.restart_cryptohome()
        self.login_and_verify(use_guest=True)
        self.login_and_verify(corrupt_login=1)
