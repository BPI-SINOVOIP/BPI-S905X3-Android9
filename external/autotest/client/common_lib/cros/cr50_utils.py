# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import logging
import re

from autotest_lib.client.common_lib import error


RO = 'ro'
RW = 'rw'
BID = 'bid'
CR50_PROD = '/opt/google/cr50/firmware/cr50.bin.prod'
CR50_PREPVT = '/opt/google/cr50/firmware/cr50.bin.prepvt'
CR50_STATE = '/var/cache/cr50*'
CR50_VERSION = '/var/cache/cr50-version'
GET_CR50_VERSION = 'cat %s' % CR50_VERSION
GET_CR50_MESSAGES ='grep "cr50-.*\[" /var/log/messages'
UPDATE_FAILURE = 'unexpected cr50-update exit code'
DUMMY_VER = '-1.-1.-1'
# This dictionary is used to search the usb_updater output for the version
# strings. There are two usb_updater commands that will return versions:
# 'fwver' and 'binvers'.
#
# 'fwver'   is used to get the running RO and RW versions from cr50
# 'binvers'  gets the version strings for each RO and RW region in the given
#            file
#
# The value in the dictionary is the regular expression that can be used to
# find the version strings for each region.
#
# --fwver
#   example output:
#           open_device 18d1:5014
#           found interface 3 endpoint 4, chunk_len 64
#           READY
#           -------
#           start
#           target running protocol version 6
#           keyids: RO 0xaa66150f, RW 0xde88588d
#           offsets: backup RO at 0x40000, backup RW at 0x44000
#           Current versions:
#           RO 0.0.10
#           RW 0.0.21
#   match groupdict:
#           {
#               'ro': '0.0.10',
#               'rw': '0.0.21'
#           }
#
# --binvers
#   example output:
#           read 524288(0x80000) bytes from /tmp/cr50.bin
#           RO_A:0.0.10 RW_A:0.0.21[00000000:00000000:00000000]
#           RO_B:0.0.10 RW_B:0.0.21[00000000:00000000:00000000]
#   match groupdict:
#           {
#               'rw_b': '0.0.21',
#               'rw_a': '0.0.21',
#               'ro_b': '0.0.10',
#               'ro_a': '0.0.10',
#               'bid_a': '00000000:00000000:00000000',
#               'bid_b': '00000000:00000000:00000000'
#           }
VERSION_RE = {
    '--fwver' : '\nRO (?P<ro>\S+).*\nRW (?P<rw>\S+)',
    '--binvers' : 'RO_A:(?P<ro_a>[\d\.]+).*' \
           'RW_A:(?P<rw_a>[\d\.]+)(\[(?P<bid_a>[\d\:A-z]+)\])?.*' \
           'RO_B:(?P<ro_b>\S+).*' \
           'RW_B:(?P<rw_b>[\d\.]+)(\[(?P<bid_b>[\d\:A-z]+)\])?.*',
}
UPDATE_TIMEOUT = 60
UPDATE_OK = 1

ERASED_BID_INT = 0xffffffff
# With an erased bid, the flags and board id will both be erased
ERASED_CHIP_BID = (ERASED_BID_INT, ERASED_BID_INT, ERASED_BID_INT)
# Any image with this board id will run on any device
EMPTY_IMAGE_BID = '00000000:00000000:00000000'
SYMBOLIC_BID_LENGTH = 4

usb_update = argparse.ArgumentParser()
usb_update.add_argument('-a', '--any', dest='universal', action='store_true')
# use /dev/tpm0 to send the command
usb_update.add_argument('-s', '--systemdev', dest='systemdev',
                        action='store_true')
# fwver, binver, and board id are used to get information about cr50 or an
# image.
usb_update.add_argument('-b', '--binvers', '-f', '--fwver', '-i', '--board_id',
                        dest='info_cmd', action='store_true')
# upstart and post_reset will post resets instead of rebooting immediately
usb_update.add_argument('-u', '--upstart', '-p', '--post_reset',
                        dest='post_reset', action='store_true')
usb_update.add_argument('extras', nargs=argparse.REMAINDER)


def AssertVersionsAreEqual(name_a, ver_a, name_b, ver_b):
    """Raise an error ver_a isn't the same as ver_b

    Args:
        name_a: the name of section a
        ver_a: the version string for section a
        name_b: the name of section b
        ver_b: the version string for section b

    Raises:
        AssertionError if ver_a is not equal to ver_b
    """
    assert ver_a == ver_b, ('Versions do not match: %s %s %s %s' %
                            (name_a, ver_a, name_b, ver_b))


def GetNewestVersion(ver_a, ver_b):
    """Compare the versions. Return the newest one. If they are the same return
    None."""
    a = [int(x) for x in ver_a.split('.')]
    b = [int(x) for x in ver_b.split('.')]

    if a > b:
        return ver_a
    if b > a:
        return ver_b
    return None


def GetVersion(versions, name):
    """Return the version string from the dictionary.

    Get the version for each key in the versions dictionary that contains the
    substring name. Make sure all of the versions match and return the version
    string. Raise an error if the versions don't match.

    Args:
        version: dictionary with the partition names as keys and the
                 partition version strings as values.
        name: the string used to find the relevant items in versions.

    Returns:
        the version from versions or "-1.-1.-1" if an invalid RO was detected.
    """
    ver = None
    key = None
    for k, v in versions.iteritems():
        if name in k:
            if v == DUMMY_VER:
                logging.info('Detected invalid %s %s', name, v)
                return v
            elif ver:
                AssertVersionsAreEqual(key, ver, k, v)
            else:
                ver = v
                key = k
    return ver


def FindVersion(output, arg):
    """Find the ro and rw versions.

    Args:
        output: The string to search
        arg: string representing the usb_updater option, either '--binvers' or
             '--fwver'

    Returns:
        a tuple of the ro and rw versions
    """
    versions = re.search(VERSION_RE[arg], output)
    versions = versions.groupdict()
    ro = GetVersion(versions, RO)
    rw = GetVersion(versions, RW)
    # --binver is the only usb_updater command that may have bid keys in its
    # versions dictionary. If no bid keys exist, bid will be None.
    bid = GetVersion(versions, BID)
    # Right now most images that aren't board id locked don't support getting
    # the board id. To make all non board id locked board ids equal, replace
    # an empty board id with None
    #
    # TODO(mruthven): Remove once all cr50 images support getting the board id.
    bid = None if bid == EMPTY_IMAGE_BID else bid
    return ro, rw, bid


def GetSavedVersion(client):
    """Return the saved version from /var/cache/cr50-version

    Some boards dont have cr50.bin.prepvt. They may still have prepvt flags.
    It is possible that cr50-update wont successfully run in this case.
    Return None if the file doesn't exist.

    Returns:
        the version saved in cr50-version or None if cr50-version doesn't exist
    """
    if not client.path_exists(CR50_VERSION):
        return None

    result = client.run(GET_CR50_VERSION).stdout.strip()
    return FindVersion(result, '--fwver')


def GetRLZ(client):
    """Get the RLZ brand code from vpd.

    Args:
        client: the object to run commands on

    Returns:
        The current RLZ code or '' if the space doesn't exist
    """
    result = client.run('vpd -g rlz_brand_code', ignore_status=True)
    if (result.exit_status and (result.exit_status != 3 or
        "Vpd data 'rlz_brand_code' was not found." not in result.stderr)):
        raise error.TestFail(result)
    return result.stdout.strip()


def SetRLZ(client, rlz):
    """Set the RLZ brand code in vpd

    Args:
        client: the object to run commands on
        rlz: 4 character string.

    Raises:
        TestError if the RLZ code is too long or if setting the code failed.
    """
    rlz = rlz.strip()
    if len(rlz) > SYMBOLIC_BID_LENGTH:
        raise error.TestError('RLZ is too long. Use a max of 4 characters')

    if rlz == GetRLZ(client):
        return
    elif rlz:
          client.run('vpd -s rlz_brand_code=%s' % rlz)
    else:
          client.run('vpd -d rlz_brand_code')

    if rlz != GetRLZ(client):
        raise error.TestError('Could not set RLZ code')


def StopTrunksd(client):
    """Stop trunksd on the client"""
    if 'running' in client.run('status trunksd').stdout:
        client.run('stop trunksd')


def UsbUpdater(client, args):
    """Run usb_update with the given args.

    Args:
        client: the object to run commands on
        args: a list of strings that contiain the usb_updater args

    Returns:
        the result of usb_update
    """
    options = usb_update.parse_args(args)

    if options.systemdev:
        StopTrunksd(client)

    # If we are updating the cr50 image, usb_update will return a non-zero exit
    # status so we should ignore it.
    ignore_status = not options.info_cmd
    # immediate reboots are only honored if the command is sent using /dev/tpm0
    expect_reboot = ((options.systemdev or options.universal) and
            not options.post_reset and not options.info_cmd)

    result = client.run('usb_updater %s' % ' '.join(args),
                        ignore_status=ignore_status,
                        ignore_timeout=expect_reboot,
                        timeout=UPDATE_TIMEOUT)

    # After a posted reboot, the usb_update exit code should equal 1.
    if result.exit_status and result.exit_status != UPDATE_OK:
        logging.debug(result)
        raise error.TestFail('Unexpected usb_update exit code after %s %d' %
                             (' '.join(args), result.exit_status))
    return result


def GetVersionFromUpdater(client, args):
    """Return the version from usb_updater"""
    result = UsbUpdater(client, args).stdout.strip()
    return FindVersion(result, args[0])


def GetFwVersion(client):
    """Get the running version using 'usb_updater --fwver'"""
    return GetVersionFromUpdater(client, ['--fwver', '-a'])


def GetBinVersion(client, image=CR50_PROD):
    """Get the image version using 'usb_updater --binvers image'"""
    return GetVersionFromUpdater(client, ['--binvers', image])


def GetVersionString(ver):
    """Combine the RO and RW tuple into a understandable string"""
    return 'RO %s RW %s%s' % (ver[0], ver[1],
           ' BID %s' % ver[2] if ver[2] else '')


def GetRunningVersion(client):
    """Get the running Cr50 version.

    The version from usb_updater and /var/cache/cr50-version should be the
    same. Get both versions and make sure they match.

    Args:
        client: the object to run commands on

    Returns:
        running_ver: a tuple with the ro and rw version strings

    Raises:
        TestFail
        - If the version in /var/cache/cr50-version is not the same as the
          version from 'usb_updater --fwver'
    """
    running_ver = GetFwVersion(client)
    saved_ver = GetSavedVersion(client)

    if saved_ver:
        AssertVersionsAreEqual('Running', GetVersionString(running_ver),
                               'Saved', GetVersionString(saved_ver))
    return running_ver


def GetActiveCr50ImagePath(client):
    """Get the path the device uses to update cr50

    Extract the active cr50 path from the cr50-update messages. This path is
    determined by cr50-get-name based on the board id flag value.

    Args:
        client: the object to run commands on

    Raises:
        TestFail
            - If cr50-update uses more than one path or if the path we find
              is not a known cr50 update path.
    """
    ClearUpdateStateAndReboot(client)
    messages = client.run(GET_CR50_MESSAGES).stdout.strip()
    paths = set(re.findall('/opt/google/cr50/firmware/cr50.bin[\S]+', messages))
    if not paths:
        raise error.TestFail('Could not determine cr50-update path')
    path = paths.pop()
    if len(paths) > 1 or (path != CR50_PROD and path != CR50_PREPVT):
        raise error.TestFail('cannot determine cr50 path')
    return path


def CheckForFailures(client, last_message):
    """Check for any unexpected cr50-update exit codes.

    This only checks the cr50 update messages that have happened since
    last_message. If a unexpected exit code is detected it will raise an error>

    Args:
        client: the object to run commands on
        last_message: the last cr50 message from the last update run

    Returns:
        the last cr50 message in /var/log/messages

    Raises:
        TestFail
            - If there is a unexpected cr50-update exit code after last_message
              in /var/log/messages
    """
    messages = client.run(GET_CR50_MESSAGES).stdout.strip()
    if last_message:
        messages = messages.rsplit(last_message, 1)[-1].split('\n')
        failures = []
        for message in messages:
            if UPDATE_FAILURE in message:
                failures.append(message)
        if len(failures):
            logging.info(messages)
            raise error.TestFail('Detected unexpected exit code during update: '
                                 '%s' % failures)
    return messages[-1]


def VerifyUpdate(client, ver='', last_message=''):
    """Verify that the saved update state is correct and there were no
    unexpected cr50-update exit codes since the last update.

    Args:
        client: the object to run commands on
        ver: the expected version tuple (ro ver, rw ver)
        last_message: the last cr50 message from the last update run

    Returns:
        new_ver: a tuple containing the running ro and rw versions
        last_message: The last cr50 update message in /var/log/messages
    """
    # Check that there were no unexpected reboots from cr50-result
    last_message = CheckForFailures(client, last_message)
    logging.debug('last cr50 message %s', last_message)

    new_ver = GetRunningVersion(client)
    if ver != '':
        if DUMMY_VER != ver[0]:
            AssertVersionsAreEqual('Old RO', ver[0], 'Updated RO', new_ver[0])
        AssertVersionsAreEqual('Old RW', ver[1], 'Updated RW', new_ver[1])
    return new_ver, last_message


def HasPrepvtImage(client):
    """Returns True if cr50.bin.prepvt exists on the dut"""
    return client.path_exists(CR50_PREPVT)


def ClearUpdateStateAndReboot(client):
    """Removes the cr50 status files in /var/cache and reboots the AP"""
    # If any /var/cache/cr50* files exist, remove them.
    result = client.run('ls %s' % CR50_STATE, ignore_status=True)
    if not result.exit_status:
        client.run('rm %s' % ' '.join(result.stdout.split()))
    elif result.exit_status != 2:
        # Exit status 2 means the file didn't exist. If the command fails for
        # some other reason, raise an error.
        logging.debug(result)
        raise error.TestFail(result.stderr)
    client.reboot()


def InstallImage(client, src, dest=CR50_PROD):
    """Copy the image at src to dest on the dut

    Args:
        client: the object to run commands on
        src: the image location of the server
        dest: the desired location on the dut

    Returns:
        The filename where the image was copied to on the dut, a tuple
        containing the RO and RW version of the file
    """
    # Send the file to the DUT
    client.send_file(src, dest)

    ver = GetBinVersion(client, dest)
    client.run('sync')
    return dest, ver


def GetBoardIdInfoTuple(board_id_str):
    """Convert the string into board id args.

    Split the board id string board_id:(mask|board_id_inv):flags to a tuple of
    its parts. The board id will be converted to its symbolic value. The flags
    and the mask/board_id_inv will be converted to an int.

    Returns:
        the symbolic board id, mask|board_id_inv, and flags
    """
    if not board_id_str:
        return None

    board_id, param2, flags = board_id_str.split(':')
    board_id = GetSymbolicBoardId(board_id)
    return board_id, int(param2, 16), int(flags, 16)


def GetBoardIdInfoString(board_id_info, symbolic=False):
    """Convert the board id list or str into a symbolic or non symbolic str.

    This can be used to convert the board id info list into a symbolic or non
    symbolic board id string. It can also be used to convert a the board id
    string into a board id string with a symbolic or non symbolic board id

    Args:
        board_id_info: A string of the form board_id:(mask|board_id_inv):flags
                       or a list with the board_id, (mask|board_id_inv), flags

    Returns:
        (board_id|symbolic_board_id):(mask|board_id_inv):flags. Will return
        None if if the given board id info is not valid
    """
    if not board_id_info:
        return None

    # Get the board id string, the mask value, and the flag value based on the
    # board_id_info type
    if isinstance(board_id_info, str):
        board_id, param2, flags = GetBoardIdInfoTuple(board_id_info)
    else:
        board_id, param2, flags = board_id_info

    # Get the hex string for board id
    board_id = '%08x' % GetIntBoardId(board_id)

    # Convert the board id hex to a symbolic board id
    if symbolic:
        board_id = GetSymbolicBoardId(board_id)

    # Return the board_id_str:8_digit_hex_mask: 8_digit_hex_flags
    return '%s:%08x:%08x' % (board_id, param2, flags)


def GetSymbolicBoardId(board_id):
    """Convert an integer board id to a symbolic string

    Args:
        board_id: the board id to convert to the symbolic board id

    Returns:
        the 4 character symbolic board id
    """
    symbolic_board_id = ''
    board_id = GetIntBoardId(board_id)

    # Convert the int to a symbolic board id
    for i in range(SYMBOLIC_BID_LENGTH):
        symbolic_board_id += chr((board_id >> (i * 8)) & 0xff)
    symbolic_board_id = symbolic_board_id[::-1]

    # Verify the created board id is 4 characters
    if len(symbolic_board_id) != SYMBOLIC_BID_LENGTH:
        raise error.TestFail('Created invalid symbolic board id %s' %
                             symbolic_board_id)
    return symbolic_board_id


def ConvertSymbolicBoardId(symbolic_board_id):
    """Convert the symbolic board id str to an int

    Args:
        symbolic_board_id: a ASCII string. It can be up to 4 characters

    Returns:
        the symbolic board id string converted to an int
    """
    board_id = 0
    for c in symbolic_board_id:
        board_id = ord(c) | (board_id << 8)
    return board_id


def GetIntBoardId(board_id):
    """"Return the usb_updater interpretation of board_id

    Args:
        board_id: a int or string value of the board id

    Returns:
        a int representation of the board id
    """
    if type(board_id) == int:
        return board_id

    if len(board_id) <= SYMBOLIC_BID_LENGTH:
        return ConvertSymbolicBoardId(board_id)

    return int(board_id, 16)


def GetExpectedFlags(flags):
    """If flags are not specified, usb_updater will set them to 0xff00

    Args:
        flags: The int value or None

    Returns:
        the original flags or 0xff00 if flags is None
    """
    return flags if flags != None else 0xff00


def GetChipBoardId(client):
    """Return the board id and flags

    Args:
        client: the object to run commands on

    Returns:
        a tuple with the int values of board id, board id inv, flags

    Raises:
        TestFail if the second board id response field is not ~board_id
    """
    result = UsbUpdater(client, ['-a', '-i']).stdout.strip()
    board_id_info = result.split('Board ID space: ')[-1].strip().split(':')
    board_id, board_id_inv, flags = [int(val, 16) for val in board_id_info]
    logging.info('BOARD_ID: %x:%x:%x', board_id, board_id_inv, flags)

    if board_id == board_id_inv == flags == ERASED_BID_INT:
        logging.info('board id is erased')
    elif board_id & board_id_inv:
        raise error.TestFail('board_id_inv should be ~board_id got %x %x' %
                             (board_id, board_id_inv))
    return board_id, board_id_inv, flags


def CheckChipBoardId(client, board_id, flags):
    """Compare the given board_id and flags to the running board_id and flags

    Interpret board_id and flags how usb_updater would interpret them, then
    compare those interpreted values to the running board_id and flags.

    Args:
        client: the object to run commands on
        board_id: a hex str, symbolic str, or int value for board_id
        flags: the int value of flags or None

    Raises:
        TestFail if the new board id info does not match
    """
    # Read back the board id and flags
    new_board_id, _, new_flags = GetChipBoardId(client)

    expected_board_id = GetIntBoardId(board_id)
    expected_flags = GetExpectedFlags(flags)

    if new_board_id != expected_board_id or new_flags != expected_flags:
        raise error.TestFail('Failed to set board id expected %x:%x, but got '
                             '%x:%x' % (expected_board_id, expected_flags,
                             new_board_id, new_flags))


def SetChipBoardId(client, board_id, flags=None):
    """Sets the board id and flags

    Args:
        client: the object to run commands on
        board_id: a string of the symbolic board id or board id hex value. If
                  the string is less than 4 characters long it will be
                  considered a symbolic value
        flags: a int flag value. If board_id is a symbolic value, then this will
               be ignored.

    Raises:
        TestFail if we were unable to set the flags to the correct value
    """

    board_id_arg = board_id
    if flags != None:
        board_id_arg += ':' + hex(flags)

    # Set the board id using the given board id and flags
    result = UsbUpdater(client, ['-a', '-i', board_id_arg]).stdout.strip()

    CheckChipBoardId(client, board_id, flags)
