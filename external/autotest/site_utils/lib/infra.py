# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import contextlib
import getpass
import subprocess
import os

import common
from autotest_lib.server.hosts import ssh_host
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib import utils
from autotest_lib.server.cros.dynamic_suite import frontend_wrappers


@contextlib.contextmanager
def chdir(dirname=None):
    """A context manager to help change directories.

    Will chdir into the provided dirname for the lifetime of the context and
    return to cwd thereafter.

    @param dirname: The dirname to chdir into.
    """
    curdir = os.getcwd()
    try:
        if dirname is not None:
            os.chdir(dirname)
        yield
    finally:
        os.chdir(curdir)


def local_runner(cmd, stream_output=False):
    """
    Runs a command on the local system as the current user.

    @param cmd: The command to run.
    @param stream_output: If True, streams the stdout of the process.

    @returns: The output of cmd, will be stdout and stderr.
    @raises CalledProcessError: If there was a non-0 return code.
    """
    print 'Running command: %s' % cmd
    proc = subprocess.Popen(
        cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if stream_output:
        output = ''
        for newline in iter(proc.stdout.readline, ''):
            output += newline
            print newline.rstrip(os.linesep)
    else:
        output = proc.communicate()[0]

    return_code = proc.wait()
    if return_code !=0:
        print "ERROR: '%s' failed with error:\n%s" % (cmd, output)
        raise subprocess.CalledProcessError(return_code, cmd, output[:1024])
    return output


_host_objects = {}

def host_object_runner(host, **kwargs):
    """
    Returns a function that returns the output of running a command via a host
    object.

    @param host: The host to run a command on.
    @returns: A function that can invoke a command remotely.
    """
    try:
        host_object = _host_objects[host]
    except KeyError:
        username = global_config.global_config.get_config_value(
                'CROS', 'infrastructure_user')
        host_object = ssh_host.SSHHost(host, user=username)
        _host_objects[host] = host_object

    def runner(cmd):
        """
        Runs a command via a host object on the enclosed host.  Translates
        host.run errors to the subprocess equivalent to expose a common API.

        @param cmd: The command to run.
        @returns: The output of cmd.
        @raises CalledProcessError: If there was a non-0 return code.
        """
        try:
            return host_object.run(cmd).stdout
        except error.AutotestHostRunError as e:
            exit_status = e.result_obj.exit_status
            command = e.result_obj.command
            raise subprocess.CalledProcessError(exit_status, command)
    return runner


def googlesh_runner(host, **kwargs):
    """
    Returns a function that return the output of running a command via shelling
    out to `googlesh`.

    @param host: The host to run a command on
    @returns: A function that can invoke a command remotely.
    """
    def runner(cmd):
        """
        Runs a command via googlesh on the enclosed host.

        @param cmd: The command to run.
        @returns: The output of cmd.
        @raises CalledProcessError: If there was a non-0 return code.
        """
        out = subprocess.check_output(['googlesh', '-s', '-uchromeos-test',
                                       '-m%s' % host, '%s' % cmd],
                                      stderr=subprocess.STDOUT)
        return out
    return runner


def execute_command(host, cmd, **kwargs):
    """
    Executes a command on the host `host`.  This an optimization that if
    we're already chromeos-test, we can just ssh to the machine in question.
    Or if we're local, we don't have to ssh at all.

    @param host: The hostname to execute the command on.
    @param cmd: The command to run.  Special shell syntax (such as pipes)
                is allowed.
    @param kwargs: Key word arguments for the runner functions.
    @returns: The output of the command.
    """
    if utils.is_localhost(host):
        runner = local_runner
    elif getpass.getuser() == 'chromeos-test':
        runner = host_object_runner(host)
    else:
        runner = googlesh_runner(host)

    return runner(cmd, **kwargs)
