#
# Copyright 2017 - The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import logging
from vts.runners.host import asserts
from vts.runners.host import const

_PERMISSION_GROUPS = 3  # 3 permission groups: owner, group, all users
_READ_PERMISSION = 4
_WRITE_PERMISSION = 2
_EXECUTE_PERMISSION = 1


def _Test(shell, *args):
    """Executes test command on device.

    Args:
        shell: an instance of the VTS shell.
        *args: strings, the command line arguments.

    Returns:
        boolean, whether the condition is true.
    """
    cmd = "test %s" % " ".join(args)
    results = shell.Execute(cmd)
    return results[const.EXIT_CODE][0] == 0


def Exists(filepath, shell):
    """Determines if a file or directory exists.

    Args:
        filepath: string, the path to a file or a directory.
        shell: an instance of the VTS shell.

    Returns:
        True if exists, False otherwise.
    """
    return _Test(shell, "-e", filepath)


def IsDirectory(path, shell):
    """Determines if a path is a directory.

    Args:
        path: string, a path on device.
        shell: an instance of the VTS shell.

    Returns:
        True if the path is a directory, False otherwise.
    """
    return _Test(shell, "-d", path)


def FindFiles(shell, path, name_pattern, options=None):
    """Searches a path for files on device.

    Args:
        shell: the ShellMirrorObject.
        path: string, the path to search on device.
        name_pattern: string, the file name pattern.
        options: string, other options passed to find command.

    Returns:
        list of strings, the paths to the found files.

    Raises:
        IOError if the pattern contains quotes, or the path does not exist.
    """
    if '"' in name_pattern or "'" in name_pattern:
        raise IOError("File name pattern contains quotes")
    cmd = "find %s -name \"%s\"" % (path, name_pattern)
    if options is not None:
        cmd += " " + options
    results = shell.Execute(cmd)
    logging.info("%s: Shell command '%s' results: %s", path, cmd, results)

    if results[const.EXIT_CODE][0] != 0:
        raise IOError(results[const.STDERR][0])

    stdout = str(results[const.STDOUT][0])
    # Filter out empty strings before return.
    return filter(None, stdout.strip().split("\n"))


def ReadFileContent(filepath, shell):
    """Read the content of a file and perform assertions.

    Args:
        filepath: string, path to file
        shell: an instance of the VTS shell

    Returns:
        string, content of file

    Raises:
        IOError if the file does not exist.
    """
    cmd = "cat %s" % filepath
    results = shell.Execute(cmd)
    logging.info("%s: Shell command '%s' results: %s", filepath, cmd, results)

    # checks the exit code
    if results[const.EXIT_CODE][0] != 0:
        raise IOError(results[const.STDERR][0])

    return results[const.STDOUT][0]


def GetPermission(path, shell):
    """Read the file permission bits of a path.

    Args:
        filepath: string, path to a file or directory
        shell: an instance of the VTS shell

    Returns:
        String, octal permission bits for the path

    Raises:
        IOError if the path does not exist or has invalid permission bits.
    """
    cmd = "stat -c %%a %s" % path
    results = shell.Execute(cmd)
    logging.info("%s: Shell command '%s' results: %s", path, cmd, results)

    # checks the exit code
    if results[const.EXIT_CODE][0] != 0:
        raise IOError(results[const.STDERR][0])

    accessBits = results[const.STDOUT][0].strip()
    if len(accessBits) != 3:
        raise IOError("%s: Wrong number of access bits (%s)" % (path,
                                                                accessBits))
    return accessBits


def _HasPermission(permission_bits, groupIndex, permission):
    """Determines if the permission bits grant a permission to a group.

    Args:
        permission_bits: string, the octal permissions string (e.g. 741)
        groupIndex: int, the index of the group into the permissions string.
                    (e.g. 0 is owner group). If set to -1, then all groups are
                    checked.
        permission: the value of the permission.

    Returns:
        True if the group(s) has read permission.

    Raises:
        ValueError if the group or permission bits are invalid
    """
    if groupIndex >= _PERMISSION_GROUPS:
        raise ValueError("Invalid group: %s" % str(groupIndex))

    if len(permission_bits) != _PERMISSION_GROUPS:
        raise ValueError("Invalid permission bits: %s" % str(permission_bits))

    # Define the start/end group index
    start = groupIndex
    end = groupIndex + 1
    if groupIndex < 0:
        start = 0
        end = _PERMISSION_GROUPS

    for i in range(start, end):
        perm = int(permission_bits[i])  # throws ValueError if not an integer
        if perm > 7:
            raise ValueError("Invalid permission bit: %s" % str(perm))
        if perm & permission == 0:
            # Return false if any group lacks the permission
            return False
    # Return true if no group lacks the permission
    return True


def IsReadable(permission_bits):
    """Determines if the permission bits grant read permission to any group.

    Args:
        permission_bits: string, the octal permissions string (e.g. 741)

    Returns:
        True if any group has read permission.

    Raises:
        ValueError if the group or permission bits are invalid
    """
    return any([
        _HasPermission(permission_bits, i, _READ_PERMISSION)
        for i in range(_PERMISSION_GROUPS)
    ])


def IsWritable(permission_bits):
    """Determines if the permission bits grant write permission to any group.

    Args:
        permission_bits: string, the octal permissions string (e.g. 741)

    Returns:
        True if any group has write permission.

    Raises:
        ValueError if the group or permission bits are invalid
    """
    return any([
        _HasPermission(permission_bits, i, _WRITE_PERMISSION)
        for i in range(_PERMISSION_GROUPS)
    ])


def IsExecutable(permission_bits):
    """Determines if the permission bits grant execute permission to any group.

    Args:
        permission_bits: string, the octal permissions string (e.g. 741)

    Returns:
        True if any group has execute permission.

    Raises:
        ValueError if the group or permission bits are invalid
    """
    return any([
        _HasPermission(permission_bits, i, _EXECUTE_PERMISSION)
        for i in range(_PERMISSION_GROUPS)
    ])


def IsReadOnly(permission_bits):
    """Determines if the permission bits grant read-only permission.

    Read-only permission is granted if some group has read access but no group
    has write access.

    Args:
        permission_bits: string, the octal permissions string (e.g. 741)

    Returns:
        True if any group has read permission, none have write.

    Raises:
        ValueError if the group or permission bits are invalid
    """
    return IsReadable(permission_bits) and not IsWritable(permission_bits)


def IsWriteOnly(permission_bits):
    """Determines if the permission bits grant write-only permission.

    Write-only permission is granted if some group has write access but no group
    has read access.

    Args:
        permission_bits: string, the octal permissions string (e.g. 741)

    Returns:
        True if any group has write permission, none have read.

    Raises:
        ValueError if the group or permission bits are invalid
    """
    return IsWritable(permission_bits) and not IsReadable(permission_bits)


def IsReadWrite(permission_bits):
    """Determines if the permission bits grant read/write permissions.

    Read-write permission is granted if some group has read access and some has
    write access. The groups may be different.

    Args:
        permission_bits: string, the octal permissions string (e.g. 741)

    Returns:
        True if read and write permissions are granted to any group(s).

    Raises:
        ValueError if the group or permission bits are invalid
    """
    return IsReadable(permission_bits) and IsWritable(permission_bits)


def assertPermissionsAndExistence(shell, path, check_permission):
    """Asserts that the specified path exists and has the correct permission.

    Args:
        path: string, path to validate existence and permissions
        check_permission: function which takes unix permissions in octal
                          format and returns True if the permissions are
                          correct, False otherwise.
    """
    asserts.assertTrue(Exists(path, shell), "%s: File does not exist." % path)
    try:
        permission = GetPermission(path, shell)
        asserts.assertTrue(
            check_permission(permission),
            "%s: File has invalid permissions (%s)" % (path, permission))
    except (ValueError, IOError) as e:
        asserts.fail("Failed to assert permissions: %s" % str(e))
