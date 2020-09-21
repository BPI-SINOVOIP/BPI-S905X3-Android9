# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, os
import time

from autotest_lib.client.common_lib import error


_PASSWD_FILE = '/var/tmp/tpm_password'
_RM_DIRS = ('/home/.shadow/* ' +
            '/home/chronos/.oobe_completed ' +
            '/home/chronos/Local\ State ' +
            '/var/cache/app_pack ' +
            '/var/cache/shill/default.profile ' +
            '/var/lib/tpm ' +
            '/var/lib/whitelist/* ')


class NoTPMPasswordException(Exception):
    """No TPM Password could be found."""
    pass


def TPMStatus(client):
    """Returns a dictionary with TPM status.

    @param client: client object to run commands on.
    """
    out = client.run('cryptohome --action=tpm_status').stdout.strip()
    out = out.replace('TPM ', '')
    lines = out.split('\n')
    status = {}
    for item in lines:
        item = item.split(':')
        if not item[0]:
            continue
        if len(item) == 1:
            item.append('')
        item = map(lambda x : x.strip(), item)
        item[1] = True if item[1] == 'true' else item[1]
        item[1] = False if item[1] == 'false' else item[1]
        status[item[0]] = item[1]
    return status


def IsTPMAvailable(client):
    """Returns True if the TPM is unowned and enabled.

    @param client: client object to run commands on.
    """
    status = TPMStatus(client)
    return status['Enabled'] and not status['Owned']


def ClearTPMServer(client, out_dir):
    """Clears the TPM and reboots from a server-side autotest.

    @param client: client object to run commands on.
    @param out_dir: temporary directory to store the retrieved password file.
    """
    if IsTPMAvailable(client):
        logging.debug('TPM is not owned')
        return

    client.run('stop ui')
    try:
        password = TPMStatus(client)['Password']
        if not password:
            try:
                client.get_file(_PASSWD_FILE, out_dir)
            except error.AutoservRunError:
                raise NoTPMPasswordException(
                        'TPM Password file %s doesn\'t exist, falling back on '
                        'clear_tpm_owner_request to clear the TPM. You may '
                        'need to have the firmware clear the TPM, for instance '
                        'by toggling the dev switch.' % _PASSWD_FILE)
            with open(os.path.join(out_dir,
                      os.path.basename(_PASSWD_FILE))) as f:
                password = f.read().rstrip()
        if not password:
            raise NoTPMPasswordException(
                    'TPM Password file %s empty, falling back on '
                    'clear_tpm_owner_request to clear the TPM. You may need to '
                    'have the firmware clear the TPM, for instance by toggling '
                    'the dev switch.' % _PASSWD_FILE)

        res = client.run('tpm_clear --pass ' + password).stdout.strip()
        logging.warn(repr(res))
    except NoTPMPasswordException as e:
        logging.warn(e.args[0])
        client.run('crossystem clear_tpm_owner_request=1')

    CleanupAndReboot(client)


def ClearTPMOwnerRequest(client, wait_for_ready=False, timeout=60):
    """Clears the TPM using crossystem command.

    @param client: client object to run commands on.
    @param wait_for_ready: wait until the TPM status is ready
    @param timeout: number of seconds to wait for the TPM to become ready.
    """
    if not client.run('crossystem clear_tpm_owner_request=1',
                      ignore_status=True).exit_status == 0:
        raise error.TestFail('Unable to clear TPM.')

    CleanupAndReboot(client)

    if wait_for_ready:
        status = ''
        end_time = time.time() + timeout
        # Wait for cryptohome to send a successful reply.
        while 'GetTpmStatus success' not in status and time.time() < end_time:
            status = client.run('cryptohome --action=tpm_more_status',
                    ignore_status=True).stdout.strip()
            logging.debug(status)
            time.sleep(1)


def CleanupAndReboot(client):
    """Cleanup and reboot the device.

    @param client: client object to run commands on.
    """
    client.run('sudo rm -rf ' + _RM_DIRS, ignore_status=True)
    client.run('sync', ignore_status=True)
    client.reboot()
