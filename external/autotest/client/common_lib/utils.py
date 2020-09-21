# Copyright (c) 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Convenience functions for use by tests or whomever.

There's no really good way to do this, as this isn't a class we can do
inheritance with, just a collection of static methods.
"""

# pylint: disable=missing-docstring

import StringIO
import errno
import inspect
import itertools
import logging
import os
import pickle
import random
import re
import resource
import select
import shutil
import signal
import socket
import string
import struct
import subprocess
import textwrap
import time
import urllib2
import urlparse
import uuid
import warnings

try:
    import hashlib
except ImportError:
    import md5
    import sha

import common

from autotest_lib.client.common_lib import env
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib import logging_manager
from autotest_lib.client.common_lib import metrics_mock_class
from autotest_lib.client.cros import constants

from autotest_lib.client.common_lib.lsbrelease_utils import *


def deprecated(func):
    """This is a decorator which can be used to mark functions as deprecated.
    It will result in a warning being emmitted when the function is used."""
    def new_func(*args, **dargs):
        warnings.warn("Call to deprecated function %s." % func.__name__,
                      category=DeprecationWarning)
        return func(*args, **dargs)
    new_func.__name__ = func.__name__
    new_func.__doc__ = func.__doc__
    new_func.__dict__.update(func.__dict__)
    return new_func


class _NullStream(object):
    def write(self, data):
        pass


    def flush(self):
        pass


TEE_TO_LOGS = object()
_the_null_stream = _NullStream()

DEVNULL = object()

DEFAULT_STDOUT_LEVEL = logging.DEBUG
DEFAULT_STDERR_LEVEL = logging.ERROR

# prefixes for logging stdout/stderr of commands
STDOUT_PREFIX = '[stdout] '
STDERR_PREFIX = '[stderr] '

# safe characters for the shell (do not need quoting)
SHELL_QUOTING_WHITELIST = frozenset(string.ascii_letters +
                                    string.digits +
                                    '_-+=')

def custom_warning_handler(message, category, filename, lineno, file=None,
                           line=None):
    """Custom handler to log at the WARNING error level. Ignores |file|."""
    logging.warning(warnings.formatwarning(message, category, filename, lineno,
                                           line))

warnings.showwarning = custom_warning_handler

def get_stream_tee_file(stream, level, prefix=''):
    if stream is None:
        return _the_null_stream
    if stream is DEVNULL:
        return None
    if stream is TEE_TO_LOGS:
        return logging_manager.LoggingFile(level=level, prefix=prefix)
    return stream


def _join_with_nickname(base_string, nickname):
    if nickname:
        return '%s BgJob "%s" ' % (base_string, nickname)
    return base_string


# TODO: Cleanup and possibly eliminate |unjoinable|, which is only used in our
# master-ssh connection process, while fixing underlying
# semantics problem in BgJob. See crbug.com/279312
class BgJob(object):
    def __init__(self, command, stdout_tee=None, stderr_tee=None, verbose=True,
                 stdin=None, stdout_level=DEFAULT_STDOUT_LEVEL,
                 stderr_level=DEFAULT_STDERR_LEVEL, nickname=None,
                 unjoinable=False, env=None, extra_paths=None):
        """Create and start a new BgJob.

        This constructor creates a new BgJob, and uses Popen to start a new
        subprocess with given command. It returns without blocking on execution
        of the subprocess.

        After starting a new BgJob, use output_prepare to connect the process's
        stdout and stderr pipes to the stream of your choice.

        When the job is running, the jobs's output streams are only read from
        when process_output is called.

        @param command: command to be executed in new subprocess. May be either
                        a list, or a string (in which case Popen will be called
                        with shell=True)
        @param stdout_tee: (Optional) a file like object, TEE_TO_LOGS or
                           DEVNULL.
                           If not given, after finishing the process, the
                           stdout data from subprocess is available in
                           result.stdout.
                           If a file like object is given, in process_output(),
                           the stdout data from the subprocess will be handled
                           by the given file like object.
                           If TEE_TO_LOGS is given, in process_output(), the
                           stdout data from the subprocess will be handled by
                           the standard logging_manager.
                           If DEVNULL is given, the stdout of the subprocess
                           will be just discarded. In addition, even after
                           cleanup(), result.stdout will be just an empty
                           string (unlike the case where stdout_tee is not
                           given).
        @param stderr_tee: Same as stdout_tee, but for stderr.
        @param verbose: Boolean, make BgJob logging more verbose.
        @param stdin: Stream object, will be passed to Popen as the new
                      process's stdin.
        @param stdout_level: A logging level value. If stdout_tee was set to
                             TEE_TO_LOGS, sets the level that tee'd
                             stdout output will be logged at. Ignored
                             otherwise.
        @param stderr_level: Same as stdout_level, but for stderr.
        @param nickname: Optional string, to be included in logging messages
        @param unjoinable: Optional bool, default False.
                           This should be True for BgJobs running in background
                           and will never be joined with join_bg_jobs(), such
                           as the master-ssh connection. Instead, it is
                           caller's responsibility to terminate the subprocess
                           correctly, e.g. by calling nuke_subprocess().
                           This will lead that, calling join_bg_jobs(),
                           process_output() or cleanup() will result in an
                           InvalidBgJobCall exception.
                           Also, |stdout_tee| and |stderr_tee| must be set to
                           DEVNULL, otherwise InvalidBgJobCall is raised.
        @param env: Dict containing environment variables used in subprocess.
        @param extra_paths: Optional string list, to be prepended to the PATH
                            env variable in env (or os.environ dict if env is
                            not specified).
        """
        self.command = command
        self.unjoinable = unjoinable
        if (unjoinable and (stdout_tee != DEVNULL or stderr_tee != DEVNULL)):
            raise error.InvalidBgJobCall(
                'stdout_tee and stderr_tee must be DEVNULL for '
                'unjoinable BgJob')
        self._stdout_tee = get_stream_tee_file(
                stdout_tee, stdout_level,
                prefix=_join_with_nickname(STDOUT_PREFIX, nickname))
        self._stderr_tee = get_stream_tee_file(
                stderr_tee, stderr_level,
                prefix=_join_with_nickname(STDERR_PREFIX, nickname))
        self.result = CmdResult(command)

        # allow for easy stdin input by string, we'll let subprocess create
        # a pipe for stdin input and we'll write to it in the wait loop
        if isinstance(stdin, basestring):
            self.string_stdin = stdin
            stdin = subprocess.PIPE
        else:
            self.string_stdin = None

        # Prepend extra_paths to env['PATH'] if necessary.
        if extra_paths:
            env = (os.environ if env is None else env).copy()
            oldpath = env.get('PATH')
            env['PATH'] = os.pathsep.join(
                    extra_paths + ([oldpath] if oldpath else []))

        if verbose:
            logging.debug("Running '%s'", command)

        if type(command) == list:
            shell = False
            executable = None
        else:
            shell = True
            executable = '/bin/bash'

        with open('/dev/null', 'w') as devnull:
            self.sp = subprocess.Popen(
                command,
                stdin=stdin,
                stdout=devnull if stdout_tee == DEVNULL else subprocess.PIPE,
                stderr=devnull if stderr_tee == DEVNULL else subprocess.PIPE,
                preexec_fn=self._reset_sigpipe,
                shell=shell, executable=executable,
                env=env, close_fds=True)

        self._cleanup_called = False
        self._stdout_file = (
            None if stdout_tee == DEVNULL else StringIO.StringIO())
        self._stderr_file = (
            None if stderr_tee == DEVNULL else StringIO.StringIO())

    def process_output(self, stdout=True, final_read=False):
        """Read from process's output stream, and write data to destinations.

        This function reads up to 1024 bytes from the background job's
        stdout or stderr stream, and writes the resulting data to the BgJob's
        output tee and to the stream set up in output_prepare.

        Warning: Calls to process_output will block on reads from the
        subprocess stream, and will block on writes to the configured
        destination stream.

        @param stdout: True = read and process data from job's stdout.
                       False = from stderr.
                       Default: True
        @param final_read: Do not read only 1024 bytes from stream. Instead,
                           read and process all data until end of the stream.

        """
        if self.unjoinable:
            raise error.InvalidBgJobCall('Cannot call process_output on '
                                         'a job with unjoinable BgJob')
        if stdout:
            pipe, buf, tee = (
                self.sp.stdout, self._stdout_file, self._stdout_tee)
        else:
            pipe, buf, tee = (
                self.sp.stderr, self._stderr_file, self._stderr_tee)

        if not pipe:
            return

        if final_read:
            # read in all the data we can from pipe and then stop
            data = []
            while select.select([pipe], [], [], 0)[0]:
                data.append(os.read(pipe.fileno(), 1024))
                if len(data[-1]) == 0:
                    break
            data = "".join(data)
        else:
            # perform a single read
            data = os.read(pipe.fileno(), 1024)
        buf.write(data)
        tee.write(data)

    def cleanup(self):
        """Clean up after BgJob.

        Flush the stdout_tee and stderr_tee buffers, close the
        subprocess stdout and stderr buffers, and saves data from
        the configured stdout and stderr destination streams to
        self.result. Duplicate calls ignored with a warning.
        """
        if self.unjoinable:
            raise error.InvalidBgJobCall('Cannot call cleanup on '
                                         'a job with a unjoinable BgJob')
        if self._cleanup_called:
            logging.warning('BgJob [%s] received a duplicate call to '
                            'cleanup. Ignoring.', self.command)
            return
        try:
            if self.sp.stdout:
                self._stdout_tee.flush()
                self.sp.stdout.close()
                self.result.stdout = self._stdout_file.getvalue()

            if self.sp.stderr:
                self._stderr_tee.flush()
                self.sp.stderr.close()
                self.result.stderr = self._stderr_file.getvalue()
        finally:
            self._cleanup_called = True

    def _reset_sigpipe(self):
        if not env.IN_MOD_WSGI:
            signal.signal(signal.SIGPIPE, signal.SIG_DFL)


def ip_to_long(ip):
    # !L is a long in network byte order
    return struct.unpack('!L', socket.inet_aton(ip))[0]


def long_to_ip(number):
    # See above comment.
    return socket.inet_ntoa(struct.pack('!L', number))


def create_subnet_mask(bits):
    return (1 << 32) - (1 << 32-bits)


def format_ip_with_mask(ip, mask_bits):
    masked_ip = ip_to_long(ip) & create_subnet_mask(mask_bits)
    return "%s/%s" % (long_to_ip(masked_ip), mask_bits)


def normalize_hostname(alias):
    ip = socket.gethostbyname(alias)
    return socket.gethostbyaddr(ip)[0]


def get_ip_local_port_range():
    match = re.match(r'\s*(\d+)\s*(\d+)\s*$',
                     read_one_line('/proc/sys/net/ipv4/ip_local_port_range'))
    return (int(match.group(1)), int(match.group(2)))


def set_ip_local_port_range(lower, upper):
    write_one_line('/proc/sys/net/ipv4/ip_local_port_range',
                   '%d %d\n' % (lower, upper))


def read_one_line(filename):
    return open(filename, 'r').readline().rstrip('\n')


def read_file(filename):
    f = open(filename)
    try:
        return f.read()
    finally:
        f.close()


def get_field(data, param, linestart="", sep=" "):
    """
    Parse data from string.
    @param data: Data to parse.
        example:
          data:
             cpu   324 345 34  5 345
             cpu0  34  11  34 34  33
             ^^^^
             start of line
             params 0   1   2  3   4
    @param param: Position of parameter after linestart marker.
    @param linestart: String to which start line with parameters.
    @param sep: Separator between parameters regular expression.
    """
    search = re.compile(r"(?<=^%s)\s*(.*)" % linestart, re.MULTILINE)
    find = search.search(data)
    if find != None:
        return re.split("%s" % sep, find.group(1))[param]
    else:
        print "There is no line which starts with %s in data." % linestart
        return None


def write_one_line(filename, line):
    open_write_close(filename, str(line).rstrip('\n') + '\n')


def open_write_close(filename, data):
    f = open(filename, 'w')
    try:
        f.write(data)
    finally:
        f.close()


def locate_file(path, base_dir=None):
    """Locates a file.

    @param path: The path of the file being located. Could be absolute or relative
        path. For relative path, it tries to locate the file from base_dir.
    @param base_dir (optional): Base directory of the relative path.

    @returns Absolute path of the file if found. None if path is None.
    @raises error.TestFail if the file is not found.
    """
    if path is None:
        return None

    if not os.path.isabs(path) and base_dir is not None:
        # Assume the relative path is based in autotest directory.
        path = os.path.join(base_dir, path)
    if not os.path.isfile(path):
        raise error.TestFail('ERROR: Unable to find %s' % path)
    return path


def matrix_to_string(matrix, header=None):
    """
    Return a pretty, aligned string representation of a nxm matrix.

    This representation can be used to print any tabular data, such as
    database results. It works by scanning the lengths of each element
    in each column, and determining the format string dynamically.

    @param matrix: Matrix representation (list with n rows of m elements).
    @param header: Optional tuple or list with header elements to be displayed.
    """
    if type(header) is list:
        header = tuple(header)
    lengths = []
    if header:
        for column in header:
            lengths.append(len(column))
    for row in matrix:
        for i, column in enumerate(row):
            column = unicode(column).encode("utf-8")
            cl = len(column)
            try:
                ml = lengths[i]
                if cl > ml:
                    lengths[i] = cl
            except IndexError:
                lengths.append(cl)

    lengths = tuple(lengths)
    format_string = ""
    for length in lengths:
        format_string += "%-" + str(length) + "s "
    format_string += "\n"

    matrix_str = ""
    if header:
        matrix_str += format_string % header
    for row in matrix:
        matrix_str += format_string % tuple(row)

    return matrix_str


def read_keyval(path, type_tag=None):
    """
    Read a key-value pair format file into a dictionary, and return it.
    Takes either a filename or directory name as input. If it's a
    directory name, we assume you want the file to be called keyval.

    @param path: Full path of the file to read from.
    @param type_tag: If not None, only keyvals with key ending
                     in a suffix {type_tag} will be collected.
    """
    if os.path.isdir(path):
        path = os.path.join(path, 'keyval')
    if not os.path.exists(path):
        return {}

    if type_tag:
        pattern = r'^([-\.\w]+)\{%s\}=(.*)$' % type_tag
    else:
        pattern = r'^([-\.\w]+)=(.*)$'

    keyval = {}
    f = open(path)
    for line in f:
        line = re.sub('#.*', '', line).rstrip()
        if not line:
            continue
        match = re.match(pattern, line)
        if match:
            key = match.group(1)
            value = match.group(2)
            if re.search('^\d+$', value):
                value = int(value)
            elif re.search('^(\d+\.)?\d+$', value):
                value = float(value)
            keyval[key] = value
        else:
            raise ValueError('Invalid format line: %s' % line)
    f.close()
    return keyval


def write_keyval(path, dictionary, type_tag=None):
    """
    Write a key-value pair format file out to a file. This uses append
    mode to open the file, so existing text will not be overwritten or
    reparsed.

    If type_tag is None, then the key must be composed of alphanumeric
    characters (or dashes+underscores). However, if type-tag is not
    null then the keys must also have "{type_tag}" as a suffix. At
    the moment the only valid values of type_tag are "attr" and "perf".

    @param path: full path of the file to be written
    @param dictionary: the items to write
    @param type_tag: see text above
    """
    if os.path.isdir(path):
        path = os.path.join(path, 'keyval')
    keyval = open(path, 'a')

    if type_tag is None:
        key_regex = re.compile(r'^[-\.\w]+$')
    else:
        if type_tag not in ('attr', 'perf'):
            raise ValueError('Invalid type tag: %s' % type_tag)
        escaped_tag = re.escape(type_tag)
        key_regex = re.compile(r'^[-\.\w]+\{%s\}$' % escaped_tag)
    try:
        for key in sorted(dictionary.keys()):
            if not key_regex.search(key):
                raise ValueError('Invalid key: %s' % key)
            keyval.write('%s=%s\n' % (key, dictionary[key]))
    finally:
        keyval.close()


def is_url(path):
    """Return true if path looks like a URL"""
    # for now, just handle http and ftp
    url_parts = urlparse.urlparse(path)
    return (url_parts[0] in ('http', 'ftp'))


def urlopen(url, data=None, timeout=5):
    """Wrapper to urllib2.urlopen with timeout addition."""

    # Save old timeout
    old_timeout = socket.getdefaulttimeout()
    socket.setdefaulttimeout(timeout)
    try:
        return urllib2.urlopen(url, data=data)
    finally:
        socket.setdefaulttimeout(old_timeout)


def urlretrieve(url, filename, data=None, timeout=300):
    """Retrieve a file from given url."""
    logging.debug('Fetching %s -> %s', url, filename)

    src_file = urlopen(url, data=data, timeout=timeout)
    try:
        dest_file = open(filename, 'wb')
        try:
            shutil.copyfileobj(src_file, dest_file)
        finally:
            dest_file.close()
    finally:
        src_file.close()


def hash(type, input=None):
    """
    Returns an hash object of type md5 or sha1. This function is implemented in
    order to encapsulate hash objects in a way that is compatible with python
    2.4 and python 2.6 without warnings.

    Note that even though python 2.6 hashlib supports hash types other than
    md5 and sha1, we are artificially limiting the input values in order to
    make the function to behave exactly the same among both python
    implementations.

    @param input: Optional input string that will be used to update the hash.
    """
    if type not in ['md5', 'sha1']:
        raise ValueError("Unsupported hash type: %s" % type)

    try:
        hash = hashlib.new(type)
    except NameError:
        if type == 'md5':
            hash = md5.new()
        elif type == 'sha1':
            hash = sha.new()

    if input:
        hash.update(input)

    return hash


def get_file(src, dest, permissions=None):
    """Get a file from src, which can be local or a remote URL"""
    if src == dest:
        return

    if is_url(src):
        urlretrieve(src, dest)
    else:
        shutil.copyfile(src, dest)

    if permissions:
        os.chmod(dest, permissions)
    return dest


def unmap_url(srcdir, src, destdir='.'):
    """
    Receives either a path to a local file or a URL.
    returns either the path to the local file, or the fetched URL

    unmap_url('/usr/src', 'foo.tar', '/tmp')
                            = '/usr/src/foo.tar'
    unmap_url('/usr/src', 'http://site/file', '/tmp')
                            = '/tmp/file'
                            (after retrieving it)
    """
    if is_url(src):
        url_parts = urlparse.urlparse(src)
        filename = os.path.basename(url_parts[2])
        dest = os.path.join(destdir, filename)
        return get_file(src, dest)
    else:
        return os.path.join(srcdir, src)


def update_version(srcdir, preserve_srcdir, new_version, install,
                   *args, **dargs):
    """
    Make sure srcdir is version new_version

    If not, delete it and install() the new version.

    In the preserve_srcdir case, we just check it's up to date,
    and if not, we rerun install, without removing srcdir
    """
    versionfile = os.path.join(srcdir, '.version')
    install_needed = True

    if os.path.exists(versionfile):
        old_version = pickle.load(open(versionfile))
        if old_version == new_version:
            install_needed = False

    if install_needed:
        if not preserve_srcdir and os.path.exists(srcdir):
            shutil.rmtree(srcdir)
        install(*args, **dargs)
        if os.path.exists(srcdir):
            pickle.dump(new_version, open(versionfile, 'w'))


def get_stderr_level(stderr_is_expected, stdout_level=DEFAULT_STDOUT_LEVEL):
    if stderr_is_expected:
        return stdout_level
    return DEFAULT_STDERR_LEVEL


def run(command, timeout=None, ignore_status=False, stdout_tee=None,
        stderr_tee=None, verbose=True, stdin=None, stderr_is_expected=None,
        stdout_level=None, stderr_level=None, args=(), nickname=None,
        ignore_timeout=False, env=None, extra_paths=None):
    """
    Run a command on the host.

    @param command: the command line string.
    @param timeout: time limit in seconds before attempting to kill the
            running process. The run() function will take a few seconds
            longer than 'timeout' to complete if it has to kill the process.
    @param ignore_status: do not raise an exception, no matter what the exit
            code of the command is.
    @param stdout_tee: optional file-like object to which stdout data
            will be written as it is generated (data will still be stored
            in result.stdout).
    @param stderr_tee: likewise for stderr.
    @param verbose: if True, log the command being run.
    @param stdin: stdin to pass to the executed process (can be a file
            descriptor, a file object of a real file or a string).
    @param stderr_is_expected: if True, stderr will be logged at the same level
            as stdout
    @param stdout_level: logging level used if stdout_tee is TEE_TO_LOGS;
            if None, a default is used.
    @param stderr_level: like stdout_level but for stderr.
    @param args: sequence of strings of arguments to be given to the command
            inside " quotes after they have been escaped for that; each
            element in the sequence will be given as a separate command
            argument
    @param nickname: Short string that will appear in logging messages
                     associated with this command.
    @param ignore_timeout: If True, timeouts are ignored otherwise if a
            timeout occurs it will raise CmdTimeoutError.
    @param env: Dict containing environment variables used in a subprocess.
    @param extra_paths: Optional string list, to be prepended to the PATH
                        env variable in env (or os.environ dict if env is
                        not specified).

    @return a CmdResult object or None if the command timed out and
            ignore_timeout is True

    @raise CmdError: the exit code of the command execution was not 0
    @raise CmdTimeoutError: the command timed out and ignore_timeout is False.
    """
    if isinstance(args, basestring):
        raise TypeError('Got a string for the "args" keyword argument, '
                        'need a sequence.')

    # In some cases, command will actually be a list
    # (For example, see get_user_hash in client/cros/cryptohome.py.)
    # So, to cover that case, detect if it's a string or not and convert it
    # into one if necessary.
    if not isinstance(command, basestring):
        command = ' '.join([sh_quote_word(arg) for arg in command])

    command = ' '.join([command] + [sh_quote_word(arg) for arg in args])

    if stderr_is_expected is None:
        stderr_is_expected = ignore_status
    if stdout_level is None:
        stdout_level = DEFAULT_STDOUT_LEVEL
    if stderr_level is None:
        stderr_level = get_stderr_level(stderr_is_expected, stdout_level)

    try:
        bg_job = join_bg_jobs(
            (BgJob(command, stdout_tee, stderr_tee, verbose, stdin=stdin,
                   stdout_level=stdout_level, stderr_level=stderr_level,
                   nickname=nickname, env=env, extra_paths=extra_paths),),
            timeout)[0]
    except error.CmdTimeoutError:
        if not ignore_timeout:
            raise
        return None

    if not ignore_status and bg_job.result.exit_status:
        raise error.CmdError(command, bg_job.result,
                             "Command returned non-zero exit status")

    return bg_job.result


def run_parallel(commands, timeout=None, ignore_status=False,
                 stdout_tee=None, stderr_tee=None,
                 nicknames=[]):
    """
    Behaves the same as run() with the following exceptions:

    - commands is a list of commands to run in parallel.
    - ignore_status toggles whether or not an exception should be raised
      on any error.

    @return: a list of CmdResult objects
    """
    bg_jobs = []
    for (command, nickname) in itertools.izip_longest(commands, nicknames):
        bg_jobs.append(BgJob(command, stdout_tee, stderr_tee,
                             stderr_level=get_stderr_level(ignore_status),
                             nickname=nickname))

    # Updates objects in bg_jobs list with their process information
    join_bg_jobs(bg_jobs, timeout)

    for bg_job in bg_jobs:
        if not ignore_status and bg_job.result.exit_status:
            raise error.CmdError(command, bg_job.result,
                                 "Command returned non-zero exit status")

    return [bg_job.result for bg_job in bg_jobs]


@deprecated
def run_bg(command):
    """Function deprecated. Please use BgJob class instead."""
    bg_job = BgJob(command)
    return bg_job.sp, bg_job.result


def join_bg_jobs(bg_jobs, timeout=None):
    """Joins the bg_jobs with the current thread.

    Returns the same list of bg_jobs objects that was passed in.
    """
    if any(bg_job.unjoinable for bg_job in bg_jobs):
        raise error.InvalidBgJobCall(
                'join_bg_jobs cannot be called for unjoinable bg_job')

    timeout_error = False
    try:
        # We are holding ends to stdin, stdout pipes
        # hence we need to be sure to close those fds no mater what
        start_time = time.time()
        timeout_error = _wait_for_commands(bg_jobs, start_time, timeout)

        for bg_job in bg_jobs:
            # Process stdout and stderr
            bg_job.process_output(stdout=True,final_read=True)
            bg_job.process_output(stdout=False,final_read=True)
    finally:
        # close our ends of the pipes to the sp no matter what
        for bg_job in bg_jobs:
            bg_job.cleanup()

    if timeout_error:
        # TODO: This needs to be fixed to better represent what happens when
        # running in parallel. However this is backwards compatable, so it will
        # do for the time being.
        raise error.CmdTimeoutError(
                bg_jobs[0].command, bg_jobs[0].result,
                "Command(s) did not complete within %d seconds" % timeout)


    return bg_jobs


def _wait_for_commands(bg_jobs, start_time, timeout):
    """Waits for background jobs by select polling their stdout/stderr.

    @param bg_jobs: A list of background jobs to wait on.
    @param start_time: Time used to calculate the timeout lifetime of a job.
    @param timeout: The timeout of the list of bg_jobs.

    @return: True if the return was due to a timeout, False otherwise.
    """

    # To check for processes which terminate without producing any output
    # a 1 second timeout is used in select.
    SELECT_TIMEOUT = 1

    read_list = []
    write_list = []
    reverse_dict = {}

    for bg_job in bg_jobs:
        if bg_job.sp.stdout:
            read_list.append(bg_job.sp.stdout)
            reverse_dict[bg_job.sp.stdout] = (bg_job, True)
        if bg_job.sp.stderr:
            read_list.append(bg_job.sp.stderr)
            reverse_dict[bg_job.sp.stderr] = (bg_job, False)
        if bg_job.string_stdin is not None:
            write_list.append(bg_job.sp.stdin)
            reverse_dict[bg_job.sp.stdin] = bg_job

    if timeout:
        stop_time = start_time + timeout
        time_left = stop_time - time.time()
    else:
        time_left = None # so that select never times out

    while not timeout or time_left > 0:
        # select will return when we may write to stdin, when there is
        # stdout/stderr output we can read (including when it is
        # EOF, that is the process has terminated) or when a non-fatal
        # signal was sent to the process. In the last case the select returns
        # EINTR, and we continue waiting for the job if the signal handler for
        # the signal that interrupted the call allows us to.
        try:
            read_ready, write_ready, _ = select.select(read_list, write_list,
                                                       [], SELECT_TIMEOUT)
        except select.error as v:
            if v[0] == errno.EINTR:
                logging.warning(v)
                continue
            else:
                raise
        # os.read() has to be used instead of
        # subproc.stdout.read() which will otherwise block
        for file_obj in read_ready:
            bg_job, is_stdout = reverse_dict[file_obj]
            bg_job.process_output(is_stdout)

        for file_obj in write_ready:
            # we can write PIPE_BUF bytes without blocking
            # POSIX requires PIPE_BUF is >= 512
            bg_job = reverse_dict[file_obj]
            file_obj.write(bg_job.string_stdin[:512])
            bg_job.string_stdin = bg_job.string_stdin[512:]
            # no more input data, close stdin, remove it from the select set
            if not bg_job.string_stdin:
                file_obj.close()
                write_list.remove(file_obj)
                del reverse_dict[file_obj]

        all_jobs_finished = True
        for bg_job in bg_jobs:
            if bg_job.result.exit_status is not None:
                continue

            bg_job.result.exit_status = bg_job.sp.poll()
            if bg_job.result.exit_status is not None:
                # process exited, remove its stdout/stdin from the select set
                bg_job.result.duration = time.time() - start_time
                if bg_job.sp.stdout:
                    read_list.remove(bg_job.sp.stdout)
                    del reverse_dict[bg_job.sp.stdout]
                if bg_job.sp.stderr:
                    read_list.remove(bg_job.sp.stderr)
                    del reverse_dict[bg_job.sp.stderr]
            else:
                all_jobs_finished = False

        if all_jobs_finished:
            return False

        if timeout:
            time_left = stop_time - time.time()

    # Kill all processes which did not complete prior to timeout
    for bg_job in bg_jobs:
        if bg_job.result.exit_status is not None:
            continue

        logging.warning('run process timeout (%s) fired on: %s', timeout,
                        bg_job.command)
        if nuke_subprocess(bg_job.sp) is None:
            # If process could not be SIGKILL'd, log kernel stack.
            logging.warning(read_file('/proc/%d/stack' % bg_job.sp.pid))
        bg_job.result.exit_status = bg_job.sp.poll()
        bg_job.result.duration = time.time() - start_time

    return True


def pid_is_alive(pid):
    """
    True if process pid exists and is not yet stuck in Zombie state.
    Zombies are impossible to move between cgroups, etc.
    pid can be integer, or text of integer.
    """
    path = '/proc/%s/stat' % pid

    try:
        stat = read_one_line(path)
    except IOError:
        if not os.path.exists(path):
            # file went away
            return False
        raise

    return stat.split()[2] != 'Z'


def signal_pid(pid, sig):
    """
    Sends a signal to a process id. Returns True if the process terminated
    successfully, False otherwise.
    """
    try:
        os.kill(pid, sig)
    except OSError:
        # The process may have died before we could kill it.
        pass

    for i in range(5):
        if not pid_is_alive(pid):
            return True
        time.sleep(1)

    # The process is still alive
    return False


def nuke_subprocess(subproc):
    # check if the subprocess is still alive, first
    if subproc.poll() is not None:
        return subproc.poll()

    # the process has not terminated within timeout,
    # kill it via an escalating series of signals.
    signal_queue = [signal.SIGTERM, signal.SIGKILL]
    for sig in signal_queue:
        signal_pid(subproc.pid, sig)
        if subproc.poll() is not None:
            return subproc.poll()


def nuke_pid(pid, signal_queue=(signal.SIGTERM, signal.SIGKILL)):
    # the process has not terminated within timeout,
    # kill it via an escalating series of signals.
    pid_path = '/proc/%d/'
    if not os.path.exists(pid_path % pid):
        # Assume that if the pid does not exist in proc it is already dead.
        logging.error('No listing in /proc for pid:%d.', pid)
        raise error.AutoservPidAlreadyDeadError('Could not kill nonexistant '
                                                'pid: %s.', pid)
    for sig in signal_queue:
        if signal_pid(pid, sig):
            return

    # no signal successfully terminated the process
    raise error.AutoservRunError('Could not kill %d for process name: %s' % (
            pid, get_process_name(pid)), None)


def system(command, timeout=None, ignore_status=False):
    """
    Run a command

    @param timeout: timeout in seconds
    @param ignore_status: if ignore_status=False, throw an exception if the
            command's exit code is non-zero
            if ignore_stauts=True, return the exit code.

    @return exit status of command
            (note, this will always be zero unless ignore_status=True)
    """
    return run(command, timeout=timeout, ignore_status=ignore_status,
               stdout_tee=TEE_TO_LOGS, stderr_tee=TEE_TO_LOGS).exit_status


def system_parallel(commands, timeout=None, ignore_status=False):
    """This function returns a list of exit statuses for the respective
    list of commands."""
    return [bg_jobs.exit_status for bg_jobs in
            run_parallel(commands, timeout=timeout, ignore_status=ignore_status,
                         stdout_tee=TEE_TO_LOGS, stderr_tee=TEE_TO_LOGS)]


def system_output(command, timeout=None, ignore_status=False,
                  retain_output=False, args=()):
    """
    Run a command and return the stdout output.

    @param command: command string to execute.
    @param timeout: time limit in seconds before attempting to kill the
            running process. The function will take a few seconds longer
            than 'timeout' to complete if it has to kill the process.
    @param ignore_status: do not raise an exception, no matter what the exit
            code of the command is.
    @param retain_output: set to True to make stdout/stderr of the command
            output to be also sent to the logging system
    @param args: sequence of strings of arguments to be given to the command
            inside " quotes after they have been escaped for that; each
            element in the sequence will be given as a separate command
            argument

    @return a string with the stdout output of the command.
    """
    if retain_output:
        out = run(command, timeout=timeout, ignore_status=ignore_status,
                  stdout_tee=TEE_TO_LOGS, stderr_tee=TEE_TO_LOGS,
                  args=args).stdout
    else:
        out = run(command, timeout=timeout, ignore_status=ignore_status,
                  args=args).stdout
    if out[-1:] == '\n':
        out = out[:-1]
    return out


def system_output_parallel(commands, timeout=None, ignore_status=False,
                           retain_output=False):
    if retain_output:
        out = [bg_job.stdout for bg_job
               in run_parallel(commands, timeout=timeout,
                               ignore_status=ignore_status,
                               stdout_tee=TEE_TO_LOGS, stderr_tee=TEE_TO_LOGS)]
    else:
        out = [bg_job.stdout for bg_job in run_parallel(commands,
                                  timeout=timeout, ignore_status=ignore_status)]
    for x in out:
        if out[-1:] == '\n': out = out[:-1]
    return out


def strip_unicode(input):
    if type(input) == list:
        return [strip_unicode(i) for i in input]
    elif type(input) == dict:
        output = {}
        for key in input.keys():
            output[str(key)] = strip_unicode(input[key])
        return output
    elif type(input) == unicode:
        return str(input)
    else:
        return input


def get_cpu_percentage(function, *args, **dargs):
    """Returns a tuple containing the CPU% and return value from function call.

    This function calculates the usage time by taking the difference of
    the user and system times both before and after the function call.
    """
    child_pre = resource.getrusage(resource.RUSAGE_CHILDREN)
    self_pre = resource.getrusage(resource.RUSAGE_SELF)
    start = time.time()
    to_return = function(*args, **dargs)
    elapsed = time.time() - start
    self_post = resource.getrusage(resource.RUSAGE_SELF)
    child_post = resource.getrusage(resource.RUSAGE_CHILDREN)

    # Calculate CPU Percentage
    s_user, s_system = [a - b for a, b in zip(self_post, self_pre)[:2]]
    c_user, c_system = [a - b for a, b in zip(child_post, child_pre)[:2]]
    cpu_percent = (s_user + c_user + s_system + c_system) / elapsed

    return cpu_percent, to_return


def get_arch(run_function=run):
    """
    Get the hardware architecture of the machine.
    If specified, run_function should return a CmdResult object and throw a
    CmdError exception.
    If run_function is anything other than utils.run(), it is used to
    execute the commands. By default (when set to utils.run()) this will
    just examine os.uname()[4].
    """

    # Short circuit from the common case.
    if run_function == run:
        return re.sub(r'i\d86$', 'i386', os.uname()[4])

    # Otherwise, use the run_function in case it hits a remote machine.
    arch = run_function('/bin/uname -m').stdout.rstrip()
    if re.match(r'i\d86$', arch):
        arch = 'i386'
    return arch

def get_arch_userspace(run_function=run):
    """
    Get the architecture by userspace (possibly different from kernel).
    """
    archs = {
        'arm': 'ELF 32-bit.*, ARM,',
        'i386': 'ELF 32-bit.*, Intel 80386,',
        'x86_64': 'ELF 64-bit.*, x86-64,',
    }

    cmd = 'file --brief --dereference /bin/sh'
    filestr = run_function(cmd).stdout.rstrip()
    for a, regex in archs.iteritems():
        if re.match(regex, filestr):
            return a

    return get_arch()


def get_num_logical_cpus_per_socket(run_function=run):
    """
    Get the number of cores (including hyperthreading) per cpu.
    run_function is used to execute the commands. It defaults to
    utils.run() but a custom method (if provided) should be of the
    same schema as utils.run. It should return a CmdResult object and
    throw a CmdError exception.
    """
    siblings = run_function('grep "^siblings" /proc/cpuinfo').stdout.rstrip()
    num_siblings = map(int,
                       re.findall(r'^siblings\s*:\s*(\d+)\s*$',
                                  siblings, re.M))
    if len(num_siblings) == 0:
        raise error.TestError('Unable to find siblings info in /proc/cpuinfo')
    if min(num_siblings) != max(num_siblings):
        raise error.TestError('Number of siblings differ %r' %
                              num_siblings)
    return num_siblings[0]


def merge_trees(src, dest):
    """
    Merges a source directory tree at 'src' into a destination tree at
    'dest'. If a path is a file in both trees than the file in the source
    tree is APPENDED to the one in the destination tree. If a path is
    a directory in both trees then the directories are recursively merged
    with this function. In any other case, the function will skip the
    paths that cannot be merged (instead of failing).
    """
    if not os.path.exists(src):
        return # exists only in dest
    elif not os.path.exists(dest):
        if os.path.isfile(src):
            shutil.copy2(src, dest) # file only in src
        else:
            shutil.copytree(src, dest, symlinks=True) # dir only in src
        return
    elif os.path.isfile(src) and os.path.isfile(dest):
        # src & dest are files in both trees, append src to dest
        destfile = open(dest, "a")
        try:
            srcfile = open(src)
            try:
                destfile.write(srcfile.read())
            finally:
                srcfile.close()
        finally:
            destfile.close()
    elif os.path.isdir(src) and os.path.isdir(dest):
        # src & dest are directories in both trees, so recursively merge
        for name in os.listdir(src):
            merge_trees(os.path.join(src, name), os.path.join(dest, name))
    else:
        # src & dest both exist, but are incompatible
        return


class CmdResult(object):
    """
    Command execution result.

    command:     String containing the command line itself
    exit_status: Integer exit code of the process
    stdout:      String containing stdout of the process
    stderr:      String containing stderr of the process
    duration:    Elapsed wall clock time running the process
    """


    def __init__(self, command="", stdout="", stderr="",
                 exit_status=None, duration=0):
        self.command = command
        self.exit_status = exit_status
        self.stdout = stdout
        self.stderr = stderr
        self.duration = duration


    def __eq__(self, other):
        if type(self) == type(other):
            return (self.command == other.command
                    and self.exit_status == other.exit_status
                    and self.stdout == other.stdout
                    and self.stderr == other.stderr
                    and self.duration == other.duration)
        else:
            return NotImplemented


    def __repr__(self):
        wrapper = textwrap.TextWrapper(width = 78,
                                       initial_indent="\n    ",
                                       subsequent_indent="    ")

        stdout = self.stdout.rstrip()
        if stdout:
            stdout = "\nstdout:\n%s" % stdout

        stderr = self.stderr.rstrip()
        if stderr:
            stderr = "\nstderr:\n%s" % stderr

        return ("* Command: %s\n"
                "Exit status: %s\n"
                "Duration: %s\n"
                "%s"
                "%s"
                % (wrapper.fill(str(self.command)), self.exit_status,
                self.duration, stdout, stderr))


class run_randomly:
    def __init__(self, run_sequentially=False):
        # Run sequentially is for debugging control files
        self.test_list = []
        self.run_sequentially = run_sequentially


    def add(self, *args, **dargs):
        test = (args, dargs)
        self.test_list.append(test)


    def run(self, fn):
        while self.test_list:
            test_index = random.randint(0, len(self.test_list)-1)
            if self.run_sequentially:
                test_index = 0
            (args, dargs) = self.test_list.pop(test_index)
            fn(*args, **dargs)


def import_site_module(path, module, dummy=None, modulefile=None):
    """
    Try to import the site specific module if it exists.

    @param path full filename of the source file calling this (ie __file__)
    @param module full module name
    @param dummy dummy value to return in case there is no symbol to import
    @param modulefile module filename

    @return site specific module or dummy

    @raises ImportError if the site file exists but imports fails
    """
    short_module = module[module.rfind(".") + 1:]

    if not modulefile:
        modulefile = short_module + ".py"

    if os.path.exists(os.path.join(os.path.dirname(path), modulefile)):
        return __import__(module, {}, {}, [short_module])
    return dummy


def import_site_symbol(path, module, name, dummy=None, modulefile=None):
    """
    Try to import site specific symbol from site specific file if it exists

    @param path full filename of the source file calling this (ie __file__)
    @param module full module name
    @param name symbol name to be imported from the site file
    @param dummy dummy value to return in case there is no symbol to import
    @param modulefile module filename

    @return site specific symbol or dummy

    @raises ImportError if the site file exists but imports fails
    """
    module = import_site_module(path, module, modulefile=modulefile)
    if not module:
        return dummy

    # special unique value to tell us if the symbol can't be imported
    cant_import = object()

    obj = getattr(module, name, cant_import)
    if obj is cant_import:
        return dummy

    return obj


def import_site_class(path, module, classname, baseclass, modulefile=None):
    """
    Try to import site specific class from site specific file if it exists

    Args:
        path: full filename of the source file calling this (ie __file__)
        module: full module name
        classname: class name to be loaded from site file
        baseclass: base class object to return when no site file present or
            to mixin when site class exists but is not inherited from baseclass
        modulefile: module filename

    Returns: baseclass if site specific class does not exist, the site specific
        class if it exists and is inherited from baseclass or a mixin of the
        site specific class and baseclass when the site specific class exists
        and is not inherited from baseclass

    Raises: ImportError if the site file exists but imports fails
    """

    res = import_site_symbol(path, module, classname, None, modulefile)
    if res:
        if not issubclass(res, baseclass):
            # if not a subclass of baseclass then mix in baseclass with the
            # site specific class object and return the result
            res = type(classname, (res, baseclass), {})
    else:
        res = baseclass

    return res


def import_site_function(path, module, funcname, dummy, modulefile=None):
    """
    Try to import site specific function from site specific file if it exists

    Args:
        path: full filename of the source file calling this (ie __file__)
        module: full module name
        funcname: function name to be imported from site file
        dummy: dummy function to return in case there is no function to import
        modulefile: module filename

    Returns: site specific function object or dummy

    Raises: ImportError if the site file exists but imports fails
    """

    return import_site_symbol(path, module, funcname, dummy, modulefile)


def _get_pid_path(program_name):
    my_path = os.path.dirname(__file__)
    return os.path.abspath(os.path.join(my_path, "..", "..",
                                        "%s.pid" % program_name))


def write_pid(program_name):
    """
    Try to drop <program_name>.pid in the main autotest directory.

    Args:
      program_name: prefix for file name
    """
    pidfile = open(_get_pid_path(program_name), "w")
    try:
        pidfile.write("%s\n" % os.getpid())
    finally:
        pidfile.close()


def delete_pid_file_if_exists(program_name):
    """
    Tries to remove <program_name>.pid from the main autotest directory.
    """
    pidfile_path = _get_pid_path(program_name)

    try:
        os.remove(pidfile_path)
    except OSError:
        if not os.path.exists(pidfile_path):
            return
        raise


def get_pid_from_file(program_name):
    """
    Reads the pid from <program_name>.pid in the autotest directory.

    @param program_name the name of the program
    @return the pid if the file exists, None otherwise.
    """
    pidfile_path = _get_pid_path(program_name)
    if not os.path.exists(pidfile_path):
        return None

    pidfile = open(_get_pid_path(program_name), 'r')

    try:
        try:
            pid = int(pidfile.readline())
        except IOError:
            if not os.path.exists(pidfile_path):
                return None
            raise
    finally:
        pidfile.close()

    return pid


def get_process_name(pid):
    """
    Get process name from PID.
    @param pid: PID of process.
    @return: Process name if PID stat file exists or 'Dead PID' if it does not.
    """
    pid_stat_path = "/proc/%d/stat"
    if not os.path.exists(pid_stat_path % pid):
        return "Dead Pid"
    return get_field(read_file(pid_stat_path % pid), 1)[1:-1]


def program_is_alive(program_name):
    """
    Checks if the process is alive and not in Zombie state.

    @param program_name the name of the program
    @return True if still alive, False otherwise
    """
    pid = get_pid_from_file(program_name)
    if pid is None:
        return False
    return pid_is_alive(pid)


def signal_program(program_name, sig=signal.SIGTERM):
    """
    Sends a signal to the process listed in <program_name>.pid

    @param program_name the name of the program
    @param sig signal to send
    """
    pid = get_pid_from_file(program_name)
    if pid:
        signal_pid(pid, sig)


def get_relative_path(path, reference):
    """Given 2 absolute paths "path" and "reference", compute the path of
    "path" as relative to the directory "reference".

    @param path the absolute path to convert to a relative path
    @param reference an absolute directory path to which the relative
        path will be computed
    """
    # normalize the paths (remove double slashes, etc)
    assert(os.path.isabs(path))
    assert(os.path.isabs(reference))

    path = os.path.normpath(path)
    reference = os.path.normpath(reference)

    # we could use os.path.split() but it splits from the end
    path_list = path.split(os.path.sep)[1:]
    ref_list = reference.split(os.path.sep)[1:]

    # find the longest leading common path
    for i in xrange(min(len(path_list), len(ref_list))):
        if path_list[i] != ref_list[i]:
            # decrement i so when exiting this loop either by no match or by
            # end of range we are one step behind
            i -= 1
            break
    i += 1
    # drop the common part of the paths, not interested in that anymore
    del path_list[:i]

    # for each uncommon component in the reference prepend a ".."
    path_list[:0] = ['..'] * (len(ref_list) - i)

    return os.path.join(*path_list)


def sh_escape(command):
    """
    Escape special characters from a command so that it can be passed
    as a double quoted (" ") string in a (ba)sh command.

    Args:
            command: the command string to escape.

    Returns:
            The escaped command string. The required englobing double
            quotes are NOT added and so should be added at some point by
            the caller.

    See also: http://www.tldp.org/LDP/abs/html/escapingsection.html
    """
    command = command.replace("\\", "\\\\")
    command = command.replace("$", r'\$')
    command = command.replace('"', r'\"')
    command = command.replace('`', r'\`')
    return command


def sh_quote_word(text, whitelist=SHELL_QUOTING_WHITELIST):
    r"""Quote a string to make it safe as a single word in a shell command.

    POSIX shell syntax recognizes no escape characters inside a single-quoted
    string.  So, single quotes can safely quote any string of characters except
    a string with a single quote character.  A single quote character must be
    quoted with the sequence '\'' which translates to:
        '  -> close current quote
        \' -> insert a literal single quote
        '  -> reopen quoting again.

    This is safe for all combinations of characters, including embedded and
    trailing backslashes in odd or even numbers.

    This is also safe for nesting, e.g. the following is a valid use:

        adb_command = 'adb shell %s' % (
                sh_quote_word('echo %s' % sh_quote_word('hello world')))

    @param text: The string to be quoted into a single word for the shell.
    @param whitelist: Optional list of characters that do not need quoting.
                      Defaults to a known good list of characters.

    @return A string, possibly quoted, safe as a single word for a shell.
    """
    if all(c in whitelist for c in text):
        return text
    return "'" + text.replace("'", r"'\''") + "'"


def configure(extra=None, configure='./configure'):
    """
    Run configure passing in the correct host, build, and target options.

    @param extra: extra command line arguments to pass to configure
    @param configure: which configure script to use
    """
    args = []
    if 'CHOST' in os.environ:
        args.append('--host=' + os.environ['CHOST'])
    if 'CBUILD' in os.environ:
        args.append('--build=' + os.environ['CBUILD'])
    if 'CTARGET' in os.environ:
        args.append('--target=' + os.environ['CTARGET'])
    if extra:
        args.append(extra)

    system('%s %s' % (configure, ' '.join(args)))


def make(extra='', make='make', timeout=None, ignore_status=False):
    """
    Run make, adding MAKEOPTS to the list of options.

    @param extra: extra command line arguments to pass to make.
    """
    cmd = '%s %s %s' % (make, os.environ.get('MAKEOPTS', ''), extra)
    return system(cmd, timeout=timeout, ignore_status=ignore_status)


def compare_versions(ver1, ver2):
    """Version number comparison between ver1 and ver2 strings.

    >>> compare_tuple("1", "2")
    -1
    >>> compare_tuple("foo-1.1", "foo-1.2")
    -1
    >>> compare_tuple("1.2", "1.2a")
    -1
    >>> compare_tuple("1.2b", "1.2a")
    1
    >>> compare_tuple("1.3.5.3a", "1.3.5.3b")
    -1

    Args:
        ver1: version string
        ver2: version string

    Returns:
        int:  1 if ver1 >  ver2
              0 if ver1 == ver2
             -1 if ver1 <  ver2
    """
    ax = re.split('[.-]', ver1)
    ay = re.split('[.-]', ver2)
    while len(ax) > 0 and len(ay) > 0:
        cx = ax.pop(0)
        cy = ay.pop(0)
        maxlen = max(len(cx), len(cy))
        c = cmp(cx.zfill(maxlen), cy.zfill(maxlen))
        if c != 0:
            return c
    return cmp(len(ax), len(ay))


def args_to_dict(args):
    """Convert autoserv extra arguments in the form of key=val or key:val to a
    dictionary.  Each argument key is converted to lowercase dictionary key.

    Args:
        args - list of autoserv extra arguments.

    Returns:
        dictionary
    """
    arg_re = re.compile(r'(\w+)[:=](.*)$')
    dict = {}
    for arg in args:
        match = arg_re.match(arg)
        if match:
            dict[match.group(1).lower()] = match.group(2)
        else:
            logging.warning("args_to_dict: argument '%s' doesn't match "
                            "'%s' pattern. Ignored.", arg, arg_re.pattern)
    return dict


def get_unused_port():
    """
    Finds a semi-random available port. A race condition is still
    possible after the port number is returned, if another process
    happens to bind it.

    Returns:
        A port number that is unused on both TCP and UDP.
    """

    def try_bind(port, socket_type, socket_proto):
        s = socket.socket(socket.AF_INET, socket_type, socket_proto)
        try:
            try:
                s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
                s.bind(('', port))
                return s.getsockname()[1]
            except socket.error:
                return None
        finally:
            s.close()

    # On the 2.6 kernel, calling try_bind() on UDP socket returns the
    # same port over and over. So always try TCP first.
    while True:
        # Ask the OS for an unused port.
        port = try_bind(0, socket.SOCK_STREAM, socket.IPPROTO_TCP)
        # Check if this port is unused on the other protocol.
        if port and try_bind(port, socket.SOCK_DGRAM, socket.IPPROTO_UDP):
            return port


def ask(question, auto=False):
    """
    Raw input with a prompt that emulates logging.

    @param question: Question to be asked
    @param auto: Whether to return "y" instead of asking the question
    """
    if auto:
        logging.info("%s (y/n) y", question)
        return "y"
    return raw_input("%s INFO | %s (y/n) " %
                     (time.strftime("%H:%M:%S", time.localtime()), question))


def rdmsr(address, cpu=0):
    """
    Reads an x86 MSR from the specified CPU, returns as long integer.
    """
    with open('/dev/cpu/%s/msr' % cpu, 'r', 0) as fd:
        fd.seek(address)
        return struct.unpack('=Q', fd.read(8))[0]


def wait_for_value(func,
                   expected_value=None,
                   min_threshold=None,
                   max_threshold=None,
                   timeout_sec=10):
    """
    Returns the value of func().  If |expected_value|, |min_threshold|, and
    |max_threshold| are not set, returns immediately.

    If |expected_value| is set, polls the return value until |expected_value| is
    reached, and returns that value.

    If either |max_threshold| or |min_threshold| is set, this function will
    will repeatedly call func() until the return value reaches or exceeds one of
    these thresholds.

    Polling will stop after |timeout_sec| regardless of these thresholds.

    @param func: function whose return value is to be waited on.
    @param expected_value: wait for func to return this value.
    @param min_threshold: wait for func value to reach or fall below this value.
    @param max_threshold: wait for func value to reach or rise above this value.
    @param timeout_sec: Number of seconds to wait before giving up and
                        returning whatever value func() last returned.

    Return value:
        The most recent return value of func().
    """
    value = None
    start_time_sec = time.time()
    while True:
        value = func()
        if (expected_value is None and \
            min_threshold is None and \
            max_threshold is None) or \
           (expected_value is not None and value == expected_value) or \
           (min_threshold is not None and value <= min_threshold) or \
           (max_threshold is not None and value >= max_threshold):
            break

        if time.time() - start_time_sec >= timeout_sec:
            break
        time.sleep(0.1)

    return value


def wait_for_value_changed(func,
                           old_value=None,
                           timeout_sec=10):
    """
    Returns the value of func().

    The function polls the return value until it is different from |old_value|,
    and returns that value.

    Polling will stop after |timeout_sec|.

    @param func: function whose return value is to be waited on.
    @param old_value: wait for func to return a value different from this.
    @param timeout_sec: Number of seconds to wait before giving up and
                        returning whatever value func() last returned.

    @returns The most recent return value of func().
    """
    value = None
    start_time_sec = time.time()
    while True:
        value = func()
        if value != old_value:
            break

        if time.time() - start_time_sec >= timeout_sec:
            break
        time.sleep(0.1)

    return value


CONFIG = global_config.global_config

# Keep checking if the pid is alive every second until the timeout (in seconds)
CHECK_PID_IS_ALIVE_TIMEOUT = 6

_LOCAL_HOST_LIST = ('localhost', '127.0.0.1')

# The default address of a vm gateway.
DEFAULT_VM_GATEWAY = '10.0.2.2'

# Google Storage bucket URI to store results in.
DEFAULT_OFFLOAD_GSURI = CONFIG.get_config_value(
        'CROS', 'results_storage_server', default=None)

# Default Moblab Ethernet Interface.
_MOBLAB_ETH_0 = 'eth0'
_MOBLAB_ETH_1 = 'eth1'

# A list of subnets that requires dedicated devserver and drone in the same
# subnet. Each item is a tuple of (subnet_ip, mask_bits), e.g.,
# ('192.168.0.0', 24))
RESTRICTED_SUBNETS = []

def _setup_restricted_subnets():
    restricted_subnets_list = CONFIG.get_config_value(
            'CROS', 'restricted_subnets', type=list, default=[])
    # TODO(dshi): Remove the code to split subnet with `:` after R51 is
    # off stable channel, and update shadow config to use `/` as
    # delimiter for consistency.
    for subnet in restricted_subnets_list:
        ip, mask_bits = subnet.split('/') if '/' in subnet \
                        else subnet.split(':')
        RESTRICTED_SUBNETS.append((ip, int(mask_bits)))

_setup_restricted_subnets()

# regex pattern for CLIENT/wireless_ssid_ config. For example, global config
# can have following config in CLIENT section to indicate that hosts in subnet
# 192.168.0.1/24 should use wireless ssid of `ssid_1`
# wireless_ssid_192.168.0.1/24: ssid_1
WIRELESS_SSID_PATTERN = 'wireless_ssid_(.*)/(\d+)'


def get_moblab_serial_number():
    """Gets the moblab public network interface.

    If the eth0 is an USB interface, try to use eth1 instead. Otherwise
    use eth0 by default.
    """
    try:
        cmd_result = run('sudo vpd -g serial_number')
        if cmd_result.stdout:
          return cmd_result.stdout
    except error.CmdError as e:
        logging.error(str(e))
        logging.info('Serial number ')
        pass
    return 'NoSerialNumber'


def ping(host, deadline=None, tries=None, timeout=60, user=None):
    """Attempt to ping |host|.

    Shell out to 'ping' if host is an IPv4 addres or 'ping6' if host is an
    IPv6 address to try to reach |host| for |timeout| seconds.
    Returns exit code of ping.

    Per 'man ping', if you specify BOTH |deadline| and |tries|, ping only
    returns 0 if we get responses to |tries| pings within |deadline| seconds.

    Specifying |deadline| or |count| alone should return 0 as long as
    some packets receive responses.

    Note that while this works with literal IPv6 addresses it will not work
    with hostnames that resolve to IPv6 only.

    @param host: the host to ping.
    @param deadline: seconds within which |tries| pings must succeed.
    @param tries: number of pings to send.
    @param timeout: number of seconds after which to kill 'ping' command.
    @return exit code of ping command.
    """
    args = [host]
    cmd = 'ping6' if re.search(r':.*:', host) else 'ping'

    if deadline:
        args.append('-w%d' % deadline)
    if tries:
        args.append('-c%d' % tries)

    if user != None:
        args = [user, '-c', ' '.join([cmd] + args)]
        cmd = 'su'

    return run(cmd, args=args, verbose=True,
                          ignore_status=True, timeout=timeout,
                          stdout_tee=TEE_TO_LOGS,
                          stderr_tee=TEE_TO_LOGS).exit_status


def host_is_in_lab_zone(hostname):
    """Check if the host is in the CLIENT.dns_zone.

    @param hostname: The hostname to check.
    @returns True if hostname.dns_zone resolves, otherwise False.
    """
    host_parts = hostname.split('.')
    dns_zone = CONFIG.get_config_value('CLIENT', 'dns_zone', default=None)
    fqdn = '%s.%s' % (host_parts[0], dns_zone)
    try:
        socket.gethostbyname(fqdn)
        return True
    except socket.gaierror:
        return False


def host_could_be_in_afe(hostname):
    """Check if the host could be in Autotest Front End.

    Report whether or not a host could be in AFE, without actually
    consulting AFE. This method exists because some systems are in the
    lab zone, but not actually managed by AFE.

    @param hostname: The hostname to check.
    @returns True if hostname is in lab zone, and does not match *-dev-*
    """
    # Do the 'dev' check first, so that we skip DNS lookup if the
    # hostname matches. This should give us greater resilience to lab
    # failures.
    return (hostname.find('-dev-') == -1) and host_is_in_lab_zone(hostname)


def get_chrome_version(job_views):
    """
    Retrieves the version of the chrome binary associated with a job.

    When a test runs we query the chrome binary for it's version and drop
    that value into a client keyval. To retrieve the chrome version we get all
    the views associated with a test from the db, including those of the
    server and client jobs, and parse the version out of the first test view
    that has it. If we never ran a single test in the suite the job_views
    dictionary will not contain a chrome version.

    This method cannot retrieve the chrome version from a dictionary that
    does not conform to the structure of an autotest tko view.

    @param job_views: a list of a job's result views, as returned by
                      the get_detailed_test_views method in rpc_interface.
    @return: The chrome version string, or None if one can't be found.
    """

    # Aborted jobs have no views.
    if not job_views:
        return None

    for view in job_views:
        if (view.get('attributes')
            and constants.CHROME_VERSION in view['attributes'].keys()):

            return view['attributes'].get(constants.CHROME_VERSION)

    logging.warning('Could not find chrome version for failure.')
    return None


def get_moblab_id():
    """Gets the moblab random id.

    The random id file is cached on disk. If it does not exist, a new file is
    created the first time.

    @returns the moblab random id.
    """
    moblab_id_filepath = '/home/moblab/.moblab_id'
    try:
        if os.path.exists(moblab_id_filepath):
            with open(moblab_id_filepath, 'r') as moblab_id_file:
                random_id = moblab_id_file.read()
        else:
            random_id = uuid.uuid1().hex
            with open(moblab_id_filepath, 'w') as moblab_id_file:
                moblab_id_file.write('%s' % random_id)
    except IOError as e:
        # Possible race condition, another process has created the file.
        # Sleep a second to make sure the file gets closed.
        logging.info(e)
        time.sleep(1)
        with open(moblab_id_filepath, 'r') as moblab_id_file:
            random_id = moblab_id_file.read()
    return random_id


def get_offload_gsuri():
    """Return the GSURI to offload test results to.

    For the normal use case this is the results_storage_server in the
    global_config.

    However partners using Moblab will be offloading their results to a
    subdirectory of their image storage buckets. The subdirectory is
    determined by the MAC Address of the Moblab device.

    @returns gsuri to offload test results to.
    """
    # For non-moblab, use results_storage_server or default.
    if not is_moblab():
        return DEFAULT_OFFLOAD_GSURI

    # For moblab, use results_storage_server or image_storage_server as bucket
    # name and mac-address/moblab_id as path.
    gsuri = DEFAULT_OFFLOAD_GSURI
    if not gsuri:
        gsuri = "%sresults/" % CONFIG.get_config_value('CROS', 'image_storage_server')

    return '%s%s/%s/' % (gsuri, get_moblab_serial_number(), get_moblab_id())


# TODO(petermayo): crosbug.com/31826 Share this with _GsUpload in
# //chromite.git/buildbot/prebuilt.py somewhere/somehow
def gs_upload(local_file, remote_file, acl, result_dir=None,
              transfer_timeout=300, acl_timeout=300):
    """Upload to GS bucket.

    @param local_file: Local file to upload
    @param remote_file: Remote location to upload the local_file to.
    @param acl: name or file used for controlling access to the uploaded
                file.
    @param result_dir: Result directory if you want to add tracing to the
                       upload.
    @param transfer_timeout: Timeout for this upload call.
    @param acl_timeout: Timeout for the acl call needed to confirm that
                        the uploader has permissions to execute the upload.

    @raise CmdError: the exit code of the gsutil call was not 0.

    @returns True/False - depending on if the upload succeeded or failed.
    """
    # https://developers.google.com/storage/docs/accesscontrol#extension
    CANNED_ACLS = ['project-private', 'private', 'public-read',
                   'public-read-write', 'authenticated-read',
                   'bucket-owner-read', 'bucket-owner-full-control']
    _GSUTIL_BIN = 'gsutil'
    acl_cmd = None
    if acl in CANNED_ACLS:
        cmd = '%s cp -a %s %s %s' % (_GSUTIL_BIN, acl, local_file, remote_file)
    else:
        # For private uploads we assume that the overlay board is set up
        # properly and a googlestore_acl.xml is present, if not this script
        # errors
        cmd = '%s cp -a private %s %s' % (_GSUTIL_BIN, local_file, remote_file)
        if not os.path.exists(acl):
            logging.error('Unable to find ACL File %s.', acl)
            return False
        acl_cmd = '%s setacl %s %s' % (_GSUTIL_BIN, acl, remote_file)
    if not result_dir:
        run(cmd, timeout=transfer_timeout, verbose=True)
        if acl_cmd:
            run(acl_cmd, timeout=acl_timeout, verbose=True)
        return True
    with open(os.path.join(result_dir, 'tracing'), 'w') as ftrace:
        ftrace.write('Preamble\n')
        run(cmd, timeout=transfer_timeout, verbose=True,
                       stdout_tee=ftrace, stderr_tee=ftrace)
        if acl_cmd:
            ftrace.write('\nACL setting\n')
            # Apply the passed in ACL xml file to the uploaded object.
            run(acl_cmd, timeout=acl_timeout, verbose=True,
                           stdout_tee=ftrace, stderr_tee=ftrace)
        ftrace.write('Postamble\n')
        return True


def gs_ls(uri_pattern):
    """Returns a list of URIs that match a given pattern.

    @param uri_pattern: a GS URI pattern, may contain wildcards

    @return A list of URIs matching the given pattern.

    @raise CmdError: the gsutil command failed.

    """
    gs_cmd = ' '.join(['gsutil', 'ls', uri_pattern])
    result = system_output(gs_cmd).splitlines()
    return [path.rstrip() for path in result if path]


def nuke_pids(pid_list, signal_queue=[signal.SIGTERM, signal.SIGKILL]):
    """
    Given a list of pid's, kill them via an esclating series of signals.

    @param pid_list: List of PID's to kill.
    @param signal_queue: Queue of signals to send the PID's to terminate them.

    @return: A mapping of the signal name to the number of processes it
        was sent to.
    """
    sig_count = {}
    # Though this is slightly hacky it beats hardcoding names anyday.
    sig_names = dict((k, v) for v, k in signal.__dict__.iteritems()
                     if v.startswith('SIG'))
    for sig in signal_queue:
        logging.debug('Sending signal %s to the following pids:', sig)
        sig_count[sig_names.get(sig, 'unknown_signal')] = len(pid_list)
        for pid in pid_list:
            logging.debug('Pid %d', pid)
            try:
                os.kill(pid, sig)
            except OSError:
                # The process may have died from a previous signal before we
                # could kill it.
                pass
        if sig == signal.SIGKILL:
            return sig_count
        pid_list = [pid for pid in pid_list if pid_is_alive(pid)]
        if not pid_list:
            break
        time.sleep(CHECK_PID_IS_ALIVE_TIMEOUT)
    failed_list = []
    for pid in pid_list:
        if pid_is_alive(pid):
            failed_list.append('Could not kill %d for process name: %s.' % pid,
                               get_process_name(pid))
    if failed_list:
        raise error.AutoservRunError('Following errors occured: %s' %
                                     failed_list, None)
    return sig_count


def externalize_host(host):
    """Returns an externally accessible host name.

    @param host: a host name or address (string)

    @return An externally visible host name or address

    """
    return socket.gethostname() if host in _LOCAL_HOST_LIST else host


def urlopen_socket_timeout(url, data=None, timeout=5):
    """
    Wrapper to urllib2.urlopen with a socket timeout.

    This method will convert all socket timeouts to
    TimeoutExceptions, so we can use it in conjunction
    with the rpc retry decorator and continue to handle
    other URLErrors as we see fit.

    @param url: The url to open.
    @param data: The data to send to the url (eg: the urlencoded dictionary
                 used with a POST call).
    @param timeout: The timeout for this urlopen call.

    @return: The response of the urlopen call.

    @raises: error.TimeoutException when a socket timeout occurs.
             urllib2.URLError for errors that not caused by timeout.
             urllib2.HTTPError for errors like 404 url not found.
    """
    old_timeout = socket.getdefaulttimeout()
    socket.setdefaulttimeout(timeout)
    try:
        return urllib2.urlopen(url, data=data)
    except urllib2.URLError as e:
        if type(e.reason) is socket.timeout:
            raise error.TimeoutException(str(e))
        raise
    finally:
        socket.setdefaulttimeout(old_timeout)


def parse_chrome_version(version_string):
    """
    Parse a chrome version string and return version and milestone.

    Given a chrome version of the form "W.X.Y.Z", return "W.X.Y.Z" as
    the version and "W" as the milestone.

    @param version_string: Chrome version string.
    @return: a tuple (chrome_version, milestone). If the incoming version
             string is not of the form "W.X.Y.Z", chrome_version will
             be set to the incoming "version_string" argument and the
             milestone will be set to the empty string.
    """
    match = re.search('(\d+)\.\d+\.\d+\.\d+', version_string)
    ver = match.group(0) if match else version_string
    milestone = match.group(1) if match else ''
    return ver, milestone


def is_localhost(server):
    """Check if server is equivalent to localhost.

    @param server: Name of the server to check.

    @return: True if given server is equivalent to localhost.

    @raise socket.gaierror: If server name failed to be resolved.
    """
    if server in _LOCAL_HOST_LIST:
        return True
    try:
        return (socket.gethostbyname(socket.gethostname()) ==
                socket.gethostbyname(server))
    except socket.gaierror:
        logging.error('Failed to resolve server name %s.', server)
        return False


def get_function_arg_value(func, arg_name, args, kwargs):
    """Get the value of the given argument for the function.

    @param func: Function being called with given arguments.
    @param arg_name: Name of the argument to look for value.
    @param args: arguments for function to be called.
    @param kwargs: keyword arguments for function to be called.

    @return: The value of the given argument for the function.

    @raise ValueError: If the argument is not listed function arguemnts.
    @raise KeyError: If no value is found for the given argument.
    """
    if arg_name in kwargs:
        return kwargs[arg_name]

    argspec = inspect.getargspec(func)
    index = argspec.args.index(arg_name)
    try:
        return args[index]
    except IndexError:
        try:
            # The argument can use a default value. Reverse the default value
            # so argument with default value can be counted from the last to
            # the first.
            return argspec.defaults[::-1][len(argspec.args) - index - 1]
        except IndexError:
            raise KeyError('Argument %s is not given a value. argspec: %s, '
                           'args:%s, kwargs:%s' %
                           (arg_name, argspec, args, kwargs))


def has_systemd():
    """Check if the host is running systemd.

    @return: True if the host uses systemd, otherwise returns False.
    """
    return os.path.basename(os.readlink('/proc/1/exe')) == 'systemd'


def version_match(build_version, release_version, update_url=''):
    """Compare release version from lsb-release with cros-version label.

    build_version is a string based on build name. It is prefixed with builder
    info and branch ID, e.g., lumpy-release/R43-6809.0.0. It may not include
    builder info, e.g., lumpy-release, in which case, update_url shall be passed
    in to determine if the build is a trybot or pgo-generate build.
    release_version is retrieved from lsb-release.
    These two values might not match exactly.

    The method is designed to compare version for following 6 scenarios with
    samples of build version and expected release version:
    1. trybot non-release build (paladin, pre-cq or test-ap build).
    build version:   trybot-lumpy-paladin/R27-3837.0.0-b123
    release version: 3837.0.2013_03_21_1340

    2. trybot release build.
    build version:   trybot-lumpy-release/R27-3837.0.0-b456
    release version: 3837.0.0

    3. buildbot official release build.
    build version:   lumpy-release/R27-3837.0.0
    release version: 3837.0.0

    4. non-official paladin rc build.
    build version:   lumpy-paladin/R27-3878.0.0-rc7
    release version: 3837.0.0-rc7

    5. chrome-perf build.
    build version:   lumpy-chrome-perf/R28-3837.0.0-b2996
    release version: 3837.0.0

    6. pgo-generate build.
    build version:   lumpy-release-pgo-generate/R28-3837.0.0-b2996
    release version: 3837.0.0-pgo-generate

    7. build version with --cheetsth suffix.
    build version:   lumpy-release/R28-3837.0.0-cheetsth
    release version: 3837.0.0

    TODO: This logic has a bug if a trybot paladin build failed to be
    installed in a DUT running an older trybot paladin build with same
    platform number, but different build number (-b###). So to conclusively
    determine if a tryjob paladin build is imaged successfully, we may need
    to find out the date string from update url.

    @param build_version: Build name for cros version, e.g.
                          peppy-release/R43-6809.0.0 or R43-6809.0.0
    @param release_version: Release version retrieved from lsb-release,
                            e.g., 6809.0.0
    @param update_url: Update url which include the full builder information.
                       Default is set to empty string.

    @return: True if the values match, otherwise returns False.
    """
    # If the build is from release, CQ or PFQ builder, cros-version label must
    # be ended with release version in lsb-release.
    if (build_version.endswith(release_version) or
            build_version.endswith(release_version + '-cheetsth')):
        return True

    if build_version.endswith('-cheetsth'):
        build_version = re.sub('-cheetsth' + '$', '', build_version)

    # Remove R#- and -b# at the end of build version
    stripped_version = re.sub(r'(R\d+-|-b\d+)', '', build_version)
    # Trim the builder info, e.g., trybot-lumpy-paladin/
    stripped_version = stripped_version.split('/')[-1]

    is_trybot_non_release_build = (
            re.match(r'.*trybot-.+-(paladin|pre-cq|test-ap|toolchain)',
                     build_version) or
            re.match(r'.*trybot-.+-(paladin|pre-cq|test-ap|toolchain)',
                     update_url))

    # Replace date string with 0 in release_version
    release_version_no_date = re.sub(r'\d{4}_\d{2}_\d{2}_\d+', '0',
                                    release_version)
    has_date_string = release_version != release_version_no_date

    is_pgo_generate_build = (
            re.match(r'.+-pgo-generate', build_version) or
            re.match(r'.+-pgo-generate', update_url))

    # Remove |-pgo-generate| in release_version
    release_version_no_pgo = release_version.replace('-pgo-generate', '')
    has_pgo_generate = release_version != release_version_no_pgo

    if is_trybot_non_release_build:
        if not has_date_string:
            logging.error('A trybot paladin or pre-cq build is expected. '
                          'Version "%s" is not a paladin or pre-cq  build.',
                          release_version)
            return False
        return stripped_version == release_version_no_date
    elif is_pgo_generate_build:
        if not has_pgo_generate:
            logging.error('A pgo-generate build is expected. Version '
                          '"%s" is not a pgo-generate build.',
                          release_version)
            return False
        return stripped_version == release_version_no_pgo
    else:
        if has_date_string:
            logging.error('Unexpected date found in a non trybot paladin or '
                          'pre-cq build.')
            return False
        # Versioned build, i.e., rc or release build.
        return stripped_version == release_version


def get_real_user():
    """Get the real user that runs the script.

    The function check environment variable SUDO_USER for the user if the
    script is run with sudo. Otherwise, it returns the value of environment
    variable USER.

    @return: The user name that runs the script.

    """
    user = os.environ.get('SUDO_USER')
    if not user:
        user = os.environ.get('USER')
    return user


def get_service_pid(service_name):
    """Return pid of service.

    @param service_name: string name of service.

    @return: pid or 0 if service is not running.
    """
    if has_systemd():
        # systemctl show prints 'MainPID=0' if the service is not running.
        cmd_result = run('systemctl show -p MainPID %s' %
                                    service_name, ignore_status=True)
        return int(cmd_result.stdout.split('=')[1])
    else:
        cmd_result = run('status %s' % service_name,
                                        ignore_status=True)
        if 'start/running' in cmd_result.stdout:
            return int(cmd_result.stdout.split()[3])
        return 0


def control_service(service_name, action='start', ignore_status=True):
    """Controls a service. It can be used to start, stop or restart
    a service.

    @param service_name: string service to be restarted.

    @param action: string choice of action to control command.

    @param ignore_status: boolean ignore if system command fails.

    @return: status code of the executed command.
    """
    if action not in ('start', 'stop', 'restart'):
        raise ValueError('Unknown action supplied as parameter.')

    control_cmd = action + ' ' + service_name
    if has_systemd():
        control_cmd = 'systemctl ' + control_cmd
    return system(control_cmd, ignore_status=ignore_status)


def restart_service(service_name, ignore_status=True):
    """Restarts a service

    @param service_name: string service to be restarted.

    @param ignore_status: boolean ignore if system command fails.

    @return: status code of the executed command.
    """
    return control_service(service_name, action='restart', ignore_status=ignore_status)


def start_service(service_name, ignore_status=True):
    """Starts a service

    @param service_name: string service to be started.

    @param ignore_status: boolean ignore if system command fails.

    @return: status code of the executed command.
    """
    return control_service(service_name, action='start', ignore_status=ignore_status)


def stop_service(service_name, ignore_status=True):
    """Stops a service

    @param service_name: string service to be stopped.

    @param ignore_status: boolean ignore if system command fails.

    @return: status code of the executed command.
    """
    return control_service(service_name, action='stop', ignore_status=ignore_status)


def sudo_require_password():
    """Test if the process can run sudo command without using password.

    @return: True if the process needs password to run sudo command.

    """
    try:
        run('sudo -n true')
        return False
    except error.CmdError:
        logging.warn('sudo command requires password.')
        return True


def is_in_container():
    """Check if the process is running inside a container.

    @return: True if the process is running inside a container, otherwise False.
    """
    result = run('grep -q "/lxc/" /proc/1/cgroup',
                            verbose=False, ignore_status=True)
    if result.exit_status == 0:
        return True

    # Check "container" environment variable for lxd/lxc containers.
    if os.environ.get('container') == 'lxc':
        return True

    return False


def is_flash_installed():
    """
    The Adobe Flash binary is only distributed with internal builds.
    """
    return (os.path.exists('/opt/google/chrome/pepper/libpepflashplayer.so')
        and os.path.exists('/opt/google/chrome/pepper/pepper-flash.info'))


def verify_flash_installed():
    """
    The Adobe Flash binary is only distributed with internal builds.
    Warn users of public builds of the extra dependency.
    """
    if not is_flash_installed():
        raise error.TestNAError('No Adobe Flash binary installed.')


def is_in_same_subnet(ip_1, ip_2, mask_bits=24):
    """Check if two IP addresses are in the same subnet with given mask bits.

    The two IP addresses are string of IPv4, e.g., '192.168.0.3'.

    @param ip_1: First IP address to compare.
    @param ip_2: Second IP address to compare.
    @param mask_bits: Number of mask bits for subnet comparison. Default to 24.

    @return: True if the two IP addresses are in the same subnet.

    """
    mask = ((2L<<mask_bits-1) -1)<<(32-mask_bits)
    ip_1_num = struct.unpack('!I', socket.inet_aton(ip_1))[0]
    ip_2_num = struct.unpack('!I', socket.inet_aton(ip_2))[0]
    return ip_1_num & mask == ip_2_num & mask


def get_ip_address(hostname):
    """Get the IP address of given hostname.

    @param hostname: Hostname of a DUT.

    @return: The IP address of given hostname. None if failed to resolve
             hostname.
    """
    try:
        if hostname:
            return socket.gethostbyname(hostname)
    except socket.gaierror as e:
        logging.error('Failed to get IP address of %s, error: %s.', hostname, e)


def get_servers_in_same_subnet(host_ip, mask_bits, servers=None,
                               server_ip_map=None):
    """Get the servers in the same subnet of the given host ip.

    @param host_ip: The IP address of a dut to look for devserver.
    @param mask_bits: Number of mask bits.
    @param servers: A list of servers to be filtered by subnet specified by
                    host_ip and mask_bits.
    @param server_ip_map: A map between the server name and its IP address.
            The map can be pre-built for better performance, e.g., when
            allocating a drone for an agent task.

    @return: A list of servers in the same subnet of the given host ip.

    """
    matched_servers = []
    if not servers and not server_ip_map:
        raise ValueError('Either `servers` or `server_ip_map` must be given.')
    if not servers:
        servers = server_ip_map.keys()
    # Make sure server_ip_map is an empty dict if it's not set.
    if not server_ip_map:
        server_ip_map = {}
    for server in servers:
        server_ip = server_ip_map.get(server, get_ip_address(server))
        if server_ip and is_in_same_subnet(server_ip, host_ip, mask_bits):
            matched_servers.append(server)
    return matched_servers


def get_restricted_subnet(hostname, restricted_subnets=RESTRICTED_SUBNETS):
    """Get the restricted subnet of given hostname.

    @param hostname: Name of the host to look for matched restricted subnet.
    @param restricted_subnets: A list of restricted subnets, default is set to
            RESTRICTED_SUBNETS.

    @return: A tuple of (subnet_ip, mask_bits), which defines a restricted
             subnet.
    """
    host_ip = get_ip_address(hostname)
    if not host_ip:
        return
    for subnet_ip, mask_bits in restricted_subnets:
        if is_in_same_subnet(subnet_ip, host_ip, mask_bits):
            return subnet_ip, mask_bits


def get_wireless_ssid(hostname):
    """Get the wireless ssid based on given hostname.

    The method tries to locate the wireless ssid in the same subnet of given
    hostname first. If none is found, it returns the default setting in
    CLIENT/wireless_ssid.

    @param hostname: Hostname of the test device.

    @return: wireless ssid for the test device.
    """
    default_ssid = CONFIG.get_config_value('CLIENT', 'wireless_ssid',
                                           default=None)
    host_ip = get_ip_address(hostname)
    if not host_ip:
        return default_ssid

    # Get all wireless ssid in the global config.
    ssids = CONFIG.get_config_value_regex('CLIENT', WIRELESS_SSID_PATTERN)

    # There could be multiple subnet matches, pick the one with most strict
    # match, i.e., the one with highest maskbit.
    matched_ssid = default_ssid
    matched_maskbit = -1
    for key, value in ssids.items():
        # The config key filtered by regex WIRELESS_SSID_PATTERN has a format of
        # wireless_ssid_[subnet_ip]/[maskbit], for example:
        # wireless_ssid_192.168.0.1/24
        # Following line extract the subnet ip and mask bit from the key name.
        match = re.match(WIRELESS_SSID_PATTERN, key)
        subnet_ip, maskbit = match.groups()
        maskbit = int(maskbit)
        if (is_in_same_subnet(subnet_ip, host_ip, maskbit) and
            maskbit > matched_maskbit):
            matched_ssid = value
            matched_maskbit = maskbit
    return matched_ssid


def parse_launch_control_build(build_name):
    """Get branch, target, build_id from the given Launch Control build_name.

    @param build_name: Name of a Launch Control build, should be formated as
                       branch/target/build_id

    @return: Tuple of branch, target, build_id
    @raise ValueError: If the build_name is not correctly formated.
    """
    branch, target, build_id = build_name.split('/')
    return branch, target, build_id


def parse_android_target(target):
    """Get board and build type from the given target.

    @param target: Name of an Android build target, e.g., shamu-eng.

    @return: Tuple of board, build_type
    @raise ValueError: If the target is not correctly formated.
    """
    board, build_type = target.split('-')
    return board, build_type


def parse_launch_control_target(target):
    """Parse the build target and type from a Launch Control target.

    The Launch Control target has the format of build_target-build_type, e.g.,
    shamu-eng or dragonboard-userdebug. This method extracts the build target
    and type from the target name.

    @param target: Name of a Launch Control target, e.g., shamu-eng.

    @return: (build_target, build_type), e.g., ('shamu', 'userdebug')
    """
    match = re.match('(?P<build_target>.+)-(?P<build_type>[^-]+)', target)
    if match:
        return match.group('build_target'), match.group('build_type')
    else:
        return None, None


def is_launch_control_build(build):
    """Check if a given build is a Launch Control build.

    @param build: Name of a build, e.g.,
                  ChromeOS build: daisy-release/R50-1234.0.0
                  Launch Control build: git_mnc_release/shamu-eng

    @return: True if the build name matches the pattern of a Launch Control
             build, False otherwise.
    """
    try:
        _, target, _ = parse_launch_control_build(build)
        build_target, _ = parse_launch_control_target(target)
        if build_target:
            return True
    except ValueError:
        # parse_launch_control_build or parse_launch_control_target failed.
        pass
    return False


def which(exec_file):
    """Finds an executable file.

    If the file name contains a path component, it is checked as-is.
    Otherwise, we check with each of the path components found in the system
    PATH prepended. This behavior is similar to the 'which' command-line tool.

    @param exec_file: Name or path to desired executable.

    @return: An actual path to the executable, or None if not found.
    """
    if os.path.dirname(exec_file):
        return exec_file if os.access(exec_file, os.X_OK) else None
    sys_path = os.environ.get('PATH')
    prefix_list = sys_path.split(os.pathsep) if sys_path else []
    for prefix in prefix_list:
        path = os.path.join(prefix, exec_file)
        if os.access(path, os.X_OK):
            return path


class TimeoutError(error.TestError):
    """Error raised when we time out when waiting on a condition."""
    pass


def poll_for_condition(condition,
                       exception=None,
                       timeout=10,
                       sleep_interval=0.1,
                       desc=None):
    """Polls until a condition becomes true.

    @param condition: function taking no args and returning bool
    @param exception: exception to throw if condition doesn't become true
    @param timeout: maximum number of seconds to wait
    @param sleep_interval: time to sleep between polls
    @param desc: description of default TimeoutError used if 'exception' is
                 None

    @return The true value that caused the poll loop to terminate.

    @raise 'exception' arg if supplied; TimeoutError otherwise
    """
    start_time = time.time()
    while True:
        value = condition()
        if value:
            return value
        if time.time() + sleep_interval - start_time > timeout:
            if exception:
                logging.error('Will raise error %r due to unexpected return: '
                              '%r', exception, value)
                raise exception

            if desc:
                desc = 'Timed out waiting for condition: ' + desc
            else:
                desc = 'Timed out waiting for unnamed condition'
            logging.error(desc)
            raise TimeoutError(desc)

        time.sleep(sleep_interval)


class metrics_mock(metrics_mock_class.mock_class_base):
    """mock class for metrics in case chromite is not installed."""
    pass
