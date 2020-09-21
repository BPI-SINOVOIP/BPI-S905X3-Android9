# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import contextlib, logging, time
from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import cryptohome


def run_cmd(cmd):
    return utils.system_output(cmd + ' 2>&1', retain_output=True,
                               ignore_status=True)


def wait_for_tpm_ready():
    for n in xrange(0, 20):
        tpm_status = cryptohome.get_tpm_status()
        if tpm_status['Ready'] == True:
            return
        time.sleep(10)
    raise error.TestError("TPM never became ready")


# This context manager ensures we mount a vault and don't forget
# to unmount it at the end of the test.
@contextlib.contextmanager
def vault_mounted(user, password):
    cryptohome.mount_vault(user, password, create=True)
    yield
    try:
        cryptohome.unmount_vault(user)
    except:
        pass


def test_file_path(user):
    return "%s/TESTFILE" % cryptohome.user_path(user)


# TODO(ejcaruso): add dump_keyset action to cryptohome utils instead
# of calling it directly here
def expect_wrapped_keyset(user):
    output = run_cmd(
        "/usr/sbin/cryptohome --action=dump_keyset --user=%s" % user)
    if output.find("TPM_WRAPPED") < 0:
        raise error.TestError(
            "Cryptohome did not create a TPM-wrapped keyset.")


class platform_CryptohomeTPMReOwn(test.test):
    """
    Test of cryptohome functionality to re-create a user's vault directory if
    the TPM is cleared and re-owned and the vault keyset is TPM-wrapped.
    """
    version = 1
    preserve_srcdir = True

    def _test_mount_cryptohome(self):
        cryptohome.remove_vault(self.user)
        wait_for_tpm_ready()
        with vault_mounted(self.user, self.password):
            run_cmd("echo TEST_CONTENT > %s" % test_file_path(self.user))
        expect_wrapped_keyset(self.user)


    def _test_mount_cryptohome_after_reboot(self):
        wait_for_tpm_ready()
        with vault_mounted(self.user, self.password):
            output = run_cmd("cat %s" % test_file_path(self.user))
        if output.find("TEST_CONTENT") < 0:
            raise error.TestError(
                "Cryptohome did not contain original test file")


    def _test_mount_cryptohome_check_recreate(self):
        wait_for_tpm_ready()
        with vault_mounted(self.user, self.password):
            output = run_cmd("cat %s" % test_file_path(self.user))
        if output.find("TEST_CONTENT") >= 0:
            raise error.TestError(
                "Cryptohome not re-created, found original test file")
        expect_wrapped_keyset(self.user)


    def run_once(self, subtest='None'):
        self.user = 'this_is_a_local_test_account@chromium.org'
        self.password = 'this_is_a_test_password'

        logging.info("Running client subtest %s", subtest)
        if subtest == 'take_tpm_ownership':
            cryptohome.take_tpm_ownership()
        elif subtest == 'mount_cryptohome':
            self._test_mount_cryptohome()
        elif subtest == 'mount_cryptohome_after_reboot':
            self._test_mount_cryptohome_after_reboot()
        elif subtest == 'mount_cryptohome_check_recreate':
            self._test_mount_cryptohome_check_recreate()
