# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dbus, gobject, logging, os, random, re, shutil, string, time
from dbus.mainloop.glib import DBusGMainLoop

import common, constants
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.cros_disks import DBusClient

CRYPTOHOME_CMD = '/usr/sbin/cryptohome'
GUEST_USER_NAME = '$guest'
UNAVAILABLE_ACTION = 'Unknown action or no action given.'
MOUNT_RETRY_COUNT = 20
TEMP_MOUNT_PATTERN = '/home/.shadow/%s/temporary_mount'
VAULT_PATH_PATTERN = '/home/.shadow/%s/vault'

class ChromiumOSError(error.TestError):
    """Generic error for ChromiumOS-specific exceptions."""
    pass

def __run_cmd(cmd):
    return utils.system_output(cmd + ' 2>&1', retain_output=True,
                               ignore_status=True).strip()

def get_user_hash(user):
    """Get the user hash for the given user."""
    return utils.system_output(['cryptohome', '--action=obfuscate_user',
                                '--user=%s' % user])


def user_path(user):
    """Get the user mount point for the given user."""
    return utils.system_output(['cryptohome-path', 'user', user])


def system_path(user):
    """Get the system mount point for the given user."""
    return utils.system_output(['cryptohome-path', 'system', user])


def temporary_mount_path(user):
    """Get the vault mount path used during crypto-migration for the user.

    @param user: user the temporary mount should be for
    """
    return TEMP_MOUNT_PATTERN % (get_user_hash(user))


def vault_path(user):
    """ Get the vault path for the given user.

    @param user: The user who's vault path should be returned.
    """
    return VAULT_PATH_PATTERN % (get_user_hash(user))


def ensure_clean_cryptohome_for(user, password=None):
    """Ensure a fresh cryptohome exists for user.

    @param user: user who needs a shiny new cryptohome.
    @param password: if unset, a random password will be used.
    """
    if not password:
        password = ''.join(random.sample(string.ascii_lowercase, 6))
    unmount_vault(user)
    remove_vault(user)
    mount_vault(user, password, create=True)


def get_tpm_status():
    """Get the TPM status.

    Returns:
        A TPM status dictionary, for example:
        { 'Enabled': True,
          'Owned': True,
          'Being Owned': False,
          'Ready': True,
          'Password': ''
        }
    """
    out = __run_cmd(CRYPTOHOME_CMD + ' --action=tpm_status')
    status = {}
    for field in ['Enabled', 'Owned', 'Being Owned', 'Ready']:
        match = re.search('TPM %s: (true|false)' % field, out)
        if not match:
            raise ChromiumOSError('Invalid TPM status: "%s".' % out)
        status[field] = match.group(1) == 'true'
    match = re.search('TPM Password: (\w*)', out)
    status['Password'] = ''
    if match:
        status['Password'] = match.group(1)
    return status


def get_tpm_more_status():
    """Get more of the TPM status.

    Returns:
        A TPM more status dictionary, for example:
        { 'dictionary_attack_lockout_in_effect': False,
          'attestation_prepared': False,
          'boot_lockbox_finalized': False,
          'enabled': True,
          'owned': True,
          'owner_password': ''
          'dictionary_attack_counter': 0,
          'dictionary_attack_lockout_seconds_remaining': 0,
          'dictionary_attack_threshold': 10,
          'attestation_enrolled': False,
          'initialized': True,
          'verified_boot_measured': False,
          'install_lockbox_finalized': True
        }
        An empty dictionary is returned if the command is not supported.
    """
    status = {}
    out = __run_cmd(CRYPTOHOME_CMD + ' --action=tpm_more_status | grep :')
    if out.startswith(UNAVAILABLE_ACTION):
        # --action=tpm_more_status only exists >= 41.
        logging.info('Method not supported!')
        return status
    for line in out.splitlines():
        items = line.strip().split(':')
        if items[1].strip() == 'false':
            value = False
        elif items[1].strip() == 'true':
            value = True
        elif items[1].strip().isdigit():
            value = int(items[1].strip())
        else:
            value = items[1].strip(' "')
        status[items[0]] = value
    return status


def get_fwmp(cleared_fwmp=False):
    """Get the firmware management parameters.

    Args:
        cleared_fwmp: True if the space should not exist.

    Returns:
        The dictionary with the FWMP contents, for example:
        { 'flags': 0xbb41,
          'developer_key_hash':
            "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\
             000\000\000\000\000\000\000\000\000\000\000",
        }
        or a dictionary with the Error if the FWMP doesn't exist and
        cleared_fwmp is True
        { 'error': 'CRYPTOHOME_ERROR_FIRMWARE_MANAGEMENT_PARAMETERS_INVALID' }

    Raises:
         ChromiumOSError if any expected field is not found in the cryptohome
         output. This would typically happen when FWMP state does not match
         'clreared_fwmp'
    """
    out = __run_cmd(CRYPTOHOME_CMD +
                    ' --action=get_firmware_management_parameters')

    if cleared_fwmp:
        fields = ['error']
    else:
        fields = ['flags', 'developer_key_hash']

    status = {}
    for field in fields:
        match = re.search('%s: (\S+)\n' % field, out)
        if not match:
            raise ChromiumOSError('Invalid FWMP field %s: "%s".' %
                                  (field, out))
        status[field] = match.group(1)
    return status


def set_fwmp(flags, developer_key_hash=None):
    """Set the firmware management parameter contents.

    Args:
        developer_key_hash: a string with the developer key hash

    Raises:
         ChromiumOSError cryptohome cannot set the FWMP contents
    """
    cmd = (CRYPTOHOME_CMD +
          ' --action=set_firmware_management_parameters '
          '--flags=' + flags)
    if developer_key_hash:
        cmd += ' --developer_key_hash=' + developer_key_hash

    out = __run_cmd(cmd)
    if 'SetFirmwareManagementParameters success' not in out:
        raise ChromiumOSError('failed to set FWMP: %s' % out)


def is_tpm_lockout_in_effect():
    """Returns true if the TPM lockout is in effect; false otherwise."""
    status = get_tpm_more_status()
    return status.get('dictionary_attack_lockout_in_effect', None)


def get_login_status():
    """Query the login status

    Returns:
        A login status dictionary containing:
        { 'owner_user_exists': True|False,
          'boot_lockbox_finalized': True|False
        }
    """
    out = __run_cmd(CRYPTOHOME_CMD + ' --action=get_login_status')
    status = {}
    for field in ['owner_user_exists', 'boot_lockbox_finalized']:
        match = re.search('%s: (true|false)' % field, out)
        if not match:
            raise ChromiumOSError('Invalid login status: "%s".' % out)
        status[field] = match.group(1) == 'true'
    return status


def get_tpm_attestation_status():
    """Get the TPM attestation status.  Works similar to get_tpm_status().
    """
    out = __run_cmd(CRYPTOHOME_CMD + ' --action=tpm_attestation_status')
    status = {}
    for field in ['Prepared', 'Enrolled']:
        match = re.search('Attestation %s: (true|false)' % field, out)
        if not match:
            raise ChromiumOSError('Invalid attestation status: "%s".' % out)
        status[field] = match.group(1) == 'true'
    return status


def take_tpm_ownership(wait_for_ownership=True):
    """Take TPM owernship.

    Args:
        wait_for_ownership: block until TPM is owned if true
    """
    __run_cmd(CRYPTOHOME_CMD + ' --action=tpm_take_ownership')
    if wait_for_ownership:
        while not get_tpm_status()['Owned']:
            time.sleep(0.1)


def verify_ek():
    """Verify the TPM endorsement key.

    Returns true if EK is valid.
    """
    cmd = CRYPTOHOME_CMD + ' --action=tpm_verify_ek'
    return (utils.system(cmd, ignore_status=True) == 0)


def remove_vault(user):
    """Remove the given user's vault from the shadow directory."""
    logging.debug('user is %s', user)
    user_hash = get_user_hash(user)
    logging.debug('Removing vault for user %s with hash %s', user, user_hash)
    cmd = CRYPTOHOME_CMD + ' --action=remove --force --user=%s' % user
    __run_cmd(cmd)
    # Ensure that the vault does not exist.
    if os.path.exists(os.path.join(constants.SHADOW_ROOT, user_hash)):
        raise ChromiumOSError('Cryptohome could not remove the user\'s vault.')


def remove_all_vaults():
    """Remove any existing vaults from the shadow directory.

    This function must be run with root privileges.
    """
    for item in os.listdir(constants.SHADOW_ROOT):
        abs_item = os.path.join(constants.SHADOW_ROOT, item)
        if os.path.isdir(os.path.join(abs_item, 'vault')):
            logging.debug('Removing vault for user with hash %s', item)
            shutil.rmtree(abs_item)


def mount_vault(user, password, create=False, key_label='bar'):
    """Mount the given user's vault. Mounts should be created by calling this
    function with create=True, and can be used afterwards with create=False.
    Only try to mount existing vaults created with this function.

    """
    args = [CRYPTOHOME_CMD, '--action=mount_ex', '--user=%s' % user,
            '--password=%s' % password, '--async']
    if create:
        args += ['--key_label=%s' % key_label, '--create']
    logging.info(__run_cmd(' '.join(args)))
    # Ensure that the vault exists in the shadow directory.
    user_hash = get_user_hash(user)
    if not os.path.exists(os.path.join(constants.SHADOW_ROOT, user_hash)):
        retry = 0
        mounted = False
        while retry < MOUNT_RETRY_COUNT and not mounted:
            time.sleep(1)
            logging.info("Retry " + str(retry + 1))
            __run_cmd(' '.join(args))
            # TODO: Remove this additional call to get_user_hash(user) when
            # crbug.com/690994 is fixed
            user_hash = get_user_hash(user)
            if os.path.exists(os.path.join(constants.SHADOW_ROOT, user_hash)):
                mounted = True
            retry += 1
        if not mounted:
            raise ChromiumOSError('Cryptohome vault not found after mount.')
    # Ensure that the vault is mounted.
    if not is_permanent_vault_mounted(user=user, allow_fail=True):
        raise ChromiumOSError('Cryptohome created a vault but did not mount.')


def mount_guest():
    """Mount the guest vault."""
    args = [CRYPTOHOME_CMD, '--action=mount_guest', '--async']
    logging.info(__run_cmd(' '.join(args)))
    # Ensure that the guest vault is mounted.
    if not is_guest_vault_mounted(allow_fail=True):
        raise ChromiumOSError('Cryptohome did not mount guest vault.')


def test_auth(user, password):
    cmd = [CRYPTOHOME_CMD, '--action=check_key_ex', '--user=%s' % user,
           '--password=%s' % password, '--async']
    out = __run_cmd(' '.join(cmd))
    logging.info(out)
    return 'Key authenticated.' in out


def unmount_vault(user):
    """Unmount the given user's vault.

    Once unmounting for a specific user is supported, the user parameter will
    name the target user. See crosbug.com/20778.
    """
    __run_cmd(CRYPTOHOME_CMD + ' --action=unmount')
    # Ensure that the vault is not mounted.
    if is_vault_mounted(user, allow_fail=True):
        raise ChromiumOSError('Cryptohome did not unmount the user.')


def __get_mount_info(mount_point, allow_fail=False):
    """Get information about the active mount at a given mount point."""
    cryptohomed_path = '/proc/$(pgrep cryptohomed)/mounts'
    try:
        logging.debug("Active cryptohome mounts:\n" +
                      utils.system_output('cat %s' % cryptohomed_path))
        mount_line = utils.system_output(
            'grep %s %s' % (mount_point, cryptohomed_path),
            ignore_status=allow_fail)
    except Exception as e:
        logging.error(e)
        raise ChromiumOSError('Could not get info about cryptohome vault '
                              'through %s. See logs for complete mount-point.'
                              % os.path.dirname(str(mount_point)))
    return mount_line.split()


def __get_user_mount_info(user, allow_fail=False):
    """Get information about the active mounts for a given user.

    Returns the active mounts at the user's user and system mount points. If no
    user is given, the active mount at the shared mount point is returned
    (regular users have a bind-mount at this mount point for backwards
    compatibility; the guest user has a mount at this mount point only).
    """
    return [__get_mount_info(mount_point=user_path(user),
                             allow_fail=allow_fail),
            __get_mount_info(mount_point=system_path(user),
                             allow_fail=allow_fail)]

def is_vault_mounted(user, regexes=None, allow_fail=False):
    """Check whether a vault is mounted for the given user.

    user: If no user is given, the shared mount point is checked, determining
      whether a vault is mounted for any user.
    regexes: dictionary of regexes to matches against the mount information.
      The mount filesystem for the user's user and system mounts point must
      match one of the keys.
      The mount source point must match the selected device regex.

    In addition, if mounted over ext4, we check the directory is encrypted.
    """
    if regexes is None:
        regexes = {
            constants.CRYPTOHOME_FS_REGEX_ANY :
               constants.CRYPTOHOME_DEV_REGEX_ANY
        }
    user_mount_info = __get_user_mount_info(user=user, allow_fail=allow_fail)
    for mount_info in user_mount_info:
        # Look at each /proc/../mount lines that match mount point for a given
        # user user/system mount (/home/user/.... /home/root/...)

        # We should have at least 3 arguments (source, mount, type of mount)
        if len(mount_info) < 3:
            return False

        device_regex = None
        for fs_regex in regexes.keys():
            if re.match(fs_regex, mount_info[2]):
                device_regex = regexes[fs_regex]
                break

        if not device_regex:
            # The thrid argument in not the expectd mount point type.
            return False

        # Check if the mount source match the device regex: it can be loose,
        # (anything) or stricter if we expect guest filesystem.
        if not re.match(device_regex, mount_info[0]):
            return False

        if (re.match(constants.CRYPTOHOME_FS_REGEX_EXT4, mount_info[2])
            and not(re.match(constants.CRYPTOHOME_DEV_REGEX_LOOP_DEVICE,
                             mount_info[0]))):
            # Ephemeral cryptohome uses ext4 mount from a loop device,
            # otherwise it should be ext4 crypto. Check there is an encryption
            # key for that directory.
            find_key_cmd_list = ['e4crypt  get_policy %s' % (mount_info[1]),
                                 'cut -d \' \' -f 2']
            key = __run_cmd(' | ' .join(find_key_cmd_list))
            cmd_list = ['keyctl show @s',
                        'grep %s' % (key),
                        'wc -l']
            out = __run_cmd(' | '.join(cmd_list))
            if int(out) != 1:
                return False
    return True


def is_guest_vault_mounted(allow_fail=False):
    """Check whether a vault is mounted for the guest user.
       It should be a mount of an ext4 partition on a loop device
       or be backed by tmpfs.
    """
    return is_vault_mounted(
        user=GUEST_USER_NAME,
        regexes={
            # Remove tmpfs support when it becomes unnecessary as all guest
            # modes will use ext4 on a loop device.
            constants.CRYPTOHOME_FS_REGEX_EXT4 :
                constants.CRYPTOHOME_DEV_REGEX_LOOP_DEVICE,
            constants.CRYPTOHOME_FS_REGEX_TMPFS :
                constants.CRYPTOHOME_DEV_REGEX_GUEST,
        },
        allow_fail=allow_fail)

def is_permanent_vault_mounted(user, allow_fail=False):
    """Check if user is mounted over ecryptfs or ext4 crypto. """
    return is_vault_mounted(
        user=user,
        regexes={
            constants.CRYPTOHOME_FS_REGEX_ECRYPTFS :
                constants.CRYPTOHOME_DEV_REGEX_REGULAR_USER_SHADOW,
            constants.CRYPTOHOME_FS_REGEX_EXT4 :
                constants.CRYPTOHOME_DEV_REGEX_REGULAR_USER_DEVICE,
        },
        allow_fail=allow_fail)

def get_mounted_vault_path(user, allow_fail=False):
    """Get the path where the decrypted data for the user is located."""
    return os.path.join(constants.SHADOW_ROOT, get_user_hash(user), 'mount')


def canonicalize(credential):
    """Perform basic canonicalization of |email_address|.

    Perform basic canonicalization of |email_address|, taking into account that
    gmail does not consider '.' or caps inside a username to matter. It also
    ignores everything after a '+'. For example,
    c.masone+abc@gmail.com == cMaSone@gmail.com, per
    http://mail.google.com/support/bin/answer.py?hl=en&ctx=mail&answer=10313
    """
    if not credential:
      return None

    parts = credential.split('@')
    if len(parts) != 2:
        raise error.TestError('Malformed email: ' + credential)

    (name, domain) = parts
    name = name.partition('+')[0]
    if (domain == constants.SPECIAL_CASE_DOMAIN):
        name = name.replace('.', '')
    return '@'.join([name, domain]).lower()


def crash_cryptohomed():
    # Try to kill cryptohomed so we get something to work with.
    pid = __run_cmd('pgrep cryptohomed')
    try:
        pid = int(pid)
    except ValueError, e:  # empty or invalid string
        raise error.TestError('Cryptohomed was not running')
    utils.system('kill -ABRT %d' % pid)
    # CONT just in case cryptohomed had a spurious STOP.
    utils.system('kill -CONT %d' % pid)
    utils.poll_for_condition(
        lambda: utils.system('ps -p %d' % pid,
                             ignore_status=True) != 0,
            timeout=180,
            exception=error.TestError(
                'Timeout waiting for cryptohomed to coredump'))


def create_ecryptfs_homedir(user, password):
    """Creates a new home directory as ecryptfs.

    If a home directory for the user exists already, it will be removed.
    The resulting home directory will be mounted.

    @param user: Username to create the home directory for.
    @param password: Password to use when creating the home directory.
    """
    unmount_vault(user)
    remove_vault(user)
    args = [
            CRYPTOHOME_CMD,
            '--action=mount_ex',
            '--user=%s' % user,
            '--password=%s' % password,
            '--key_label=foo',
            '--ecryptfs',
            '--create']
    logging.info(__run_cmd(' '.join(args)))
    if not is_vault_mounted(user, regexes={
        constants.CRYPTOHOME_FS_REGEX_ECRYPTFS :
            constants.CRYPTOHOME_DEV_REGEX_REGULAR_USER_SHADOW
    }, allow_fail=True):
        raise ChromiumOSError('Ecryptfs home could not be created')


def do_dircrypto_migration(user, password, timeout=600):
    """Start dircrypto migration for the user.

    @param user: The user to migrate.
    @param password: The password used to mount the users vault
    @param timeout: How long in seconds to wait for the migration to finish
    before failing.
    """
    unmount_vault(user)
    args = [
            CRYPTOHOME_CMD,
            '--action=mount_ex',
            '--to_migrate_from_ecryptfs',
            '--user=%s' % user,
            '--password=%s' % password]
    logging.info(__run_cmd(' '.join(args)))
    if not __get_mount_info(temporary_mount_path(user), allow_fail=True):
        raise ChromiumOSError('Failed to mount home for migration')
    args = [CRYPTOHOME_CMD, '--action=migrate_to_dircrypto', '--user=%s' % user]
    logging.info(__run_cmd(' '.join(args)))
    utils.poll_for_condition(
        lambda: not __get_mount_info(
                temporary_mount_path(user), allow_fail=True),
        timeout=timeout,
        exception=error.TestError(
                'Timeout waiting for dircrypto migration to finish'))


def change_password(user, password, new_password):
    args = [
            CRYPTOHOME_CMD,
            '--action=migrate_key',
            '--async',
            '--user=%s' % user,
            '--old_password=%s' % password,
            '--password=%s' % new_password]
    out = __run_cmd(' '.join(args))
    logging.info(out)
    if 'Key migration succeeded.' not in out:
        raise ChromiumOSError('Key migration failed.')


class CryptohomeProxy(DBusClient):
    """A DBus proxy client for testing the Cryptohome DBus server.
    """
    CRYPTOHOME_BUS_NAME = 'org.chromium.Cryptohome'
    CRYPTOHOME_OBJECT_PATH = '/org/chromium/Cryptohome'
    CRYPTOHOME_INTERFACE = 'org.chromium.CryptohomeInterface'
    ASYNC_CALL_STATUS_SIGNAL = 'AsyncCallStatus'
    ASYNC_CALL_STATUS_SIGNAL_ARGUMENTS = (
        'async_id', 'return_status', 'return_code'
    )
    DBUS_PROPERTIES_INTERFACE = 'org.freedesktop.DBus.Properties'


    def __init__(self, bus_loop=None):
        self.main_loop = gobject.MainLoop()
        if bus_loop is None:
            bus_loop = DBusGMainLoop(set_as_default=True)
        self.bus = dbus.SystemBus(mainloop=bus_loop)
        super(CryptohomeProxy, self).__init__(self.main_loop, self.bus,
                                              self.CRYPTOHOME_BUS_NAME,
                                              self.CRYPTOHOME_OBJECT_PATH)
        self.iface = dbus.Interface(self.proxy_object,
                                    self.CRYPTOHOME_INTERFACE)
        self.properties = dbus.Interface(self.proxy_object,
                                         self.DBUS_PROPERTIES_INTERFACE)
        self.handle_signal(self.CRYPTOHOME_INTERFACE,
                           self.ASYNC_CALL_STATUS_SIGNAL,
                           self.ASYNC_CALL_STATUS_SIGNAL_ARGUMENTS)


    # Wrap all proxied calls to catch cryptohomed failures.
    def __call(self, method, *args):
        try:
            return method(*args, timeout=180)
        except dbus.exceptions.DBusException, e:
            if e.get_dbus_name() == 'org.freedesktop.DBus.Error.NoReply':
                logging.error('Cryptohome is not responding. Sending ABRT')
                crash_cryptohomed()
                raise ChromiumOSError('cryptohomed aborted. Check crashes!')
            raise e


    def __wait_for_specific_signal(self, signal, data):
      """Wait for the |signal| with matching |data|
         Returns the resulting dict on success or {} on error.
      """
      # Do not bubble up the timeout here, just return {}.
      result = {}
      try:
          result = self.wait_for_signal(signal)
      except utils.TimeoutError:
          return {}
      for k in data.keys():
          if not result.has_key(k) or result[k] != data[k]:
            return {}
      return result


    # Perform a data-less async call.
    # TODO(wad) Add __async_data_call.
    def __async_call(self, method, *args):
        # Clear out any superfluous async call signals.
        self.clear_signal_content(self.ASYNC_CALL_STATUS_SIGNAL)
        out = self.__call(method, *args)
        logging.debug('Issued call ' + str(method) +
                      ' with async_id ' + str(out))
        result = {}
        try:
            # __wait_for_specific_signal has a 10s timeout
            result = utils.poll_for_condition(
                lambda: self.__wait_for_specific_signal(
                    self.ASYNC_CALL_STATUS_SIGNAL, {'async_id' : out}),
                timeout=180,
                desc='matching %s signal' % self.ASYNC_CALL_STATUS_SIGNAL)
        except utils.TimeoutError, e:
            logging.error('Cryptohome timed out. Sending ABRT.')
            crash_cryptohomed()
            raise ChromiumOSError('cryptohomed aborted. Check crashes!')
        return result


    def mount(self, user, password, create=False, async=True):
        """Mounts a cryptohome.

        Returns True if the mount succeeds or False otherwise.
        TODO(ellyjones): Migrate mount_vault() to use a multi-user-safe
        heuristic, then remove this method. See <crosbug.com/20778>.
        """
        if async:
            return self.__async_call(self.iface.AsyncMount, user, password,
                                     create, False, [])['return_status']
        out = self.__call(self.iface.Mount, user, password, create, False, [])
        # Sync returns (return code, return status)
        return out[1] if len(out) > 1 else False


    def unmount(self, user):
        """Unmounts a cryptohome.

        Returns True if the unmount suceeds or false otherwise.
        TODO(ellyjones): Once there's a per-user unmount method, use it. See
        <crosbug.com/20778>.
        """
        return self.__call(self.iface.Unmount)


    def is_mounted(self, user):
        """Tests whether a user's cryptohome is mounted."""
        return (utils.is_mountpoint(user_path(user))
                and utils.is_mountpoint(system_path(user)))


    def require_mounted(self, user):
        """Raises a test failure if a user's cryptohome is not mounted."""
        utils.require_mountpoint(user_path(user))
        utils.require_mountpoint(system_path(user))


    def migrate(self, user, oldkey, newkey, async=True):
        """Migrates the specified user's cryptohome from one key to another."""
        if async:
            return self.__async_call(self.iface.AsyncMigrateKey,
                                     user, oldkey, newkey)['return_status']
        return self.__call(self.iface.MigrateKey, user, oldkey, newkey)


    def remove(self, user, async=True):
        if async:
            return self.__async_call(self.iface.AsyncRemove,
                                     user)['return_status']
        return self.__call(self.iface.Remove, user)


    def ensure_clean_cryptohome_for(self, user, password=None):
        """Ensure a fresh cryptohome exists for user.

        @param user: user who needs a shiny new cryptohome.
        @param password: if unset, a random password will be used.
        """
        if not password:
            password = ''.join(random.sample(string.ascii_lowercase, 6))
        self.remove(user)
        self.mount(user, password, create=True)

    def lock_install_attributes(self, attrs):
        """Set and lock install attributes for the device.

        @param attrs: dict of install attributes.
        """
        take_tpm_ownership()
        for key, value in attrs.items():
            if not self.__call(self.iface.InstallAttributesSet, key,
                               dbus.ByteArray(value + '\0')):
                return False
        return self.__call(self.iface.InstallAttributesFinalize)
