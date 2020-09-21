# Copyright 2009 Google Inc. Released under the GPL v2

"""
This file contains the implementation of a host object for the local machine.
"""
import distutils.core
import glob
import os
import platform
import shutil
import sys

import common
from autotest_lib.client.common_lib import hosts, error
from autotest_lib.client.bin import utils


class LocalHost(hosts.Host):
    """This class represents a host running locally on the host."""


    def _initialize(self, hostname=None, bootloader=None, *args, **dargs):
        super(LocalHost, self)._initialize(*args, **dargs)

        # hostname will be an actual hostname when this client was created
        # by an autoserv process
        if not hostname:
            hostname = platform.node()
        self.hostname = hostname
        self.bootloader = bootloader
        self.tmp_dirs = []


    def close(self):
        """Cleanup after we're done."""
        for tmp_dir in self.tmp_dirs:
            self.run('rm -rf "%s"' % (utils.sh_escape(tmp_dir)),
                     ignore_status=True)


    def wait_up(self, timeout=None):
        # a local host is always up
        return True


    def run(self, command, timeout=3600, ignore_status=False,
            ignore_timeout=False,
            stdout_tee=utils.TEE_TO_LOGS, stderr_tee=utils.TEE_TO_LOGS,
            stdin=None, args=(), **kwargs):
        """
        @see common_lib.hosts.Host.run()
        """
        try:
            return utils.run(
                command, timeout=timeout, ignore_status=ignore_status,
                ignore_timeout=ignore_timeout, stdout_tee=stdout_tee,
                stderr_tee=stderr_tee, stdin=stdin, args=args)
        except error.CmdTimeoutError as e:
            # CmdTimeoutError is a subclass of CmdError, so must be caught first
            new_error = error.AutotestHostRunTimeoutError(
                    e.command, e.result_obj, additional_text=e.additional_text)
            raise error.AutotestHostRunTimeoutError, new_error, \
                    sys.exc_info()[2]
        except error.CmdError as e:
            new_error = error.AutotestHostRunCmdError(
                    e.command, e.result_obj, additional_text=e.additional_text)
            raise error.AutotestHostRunCmdError, new_error, sys.exc_info()[2]


    def list_files_glob(self, path_glob):
        """
        Get a list of files on a remote host given a glob pattern path.
        """
        return glob.glob(path_glob)


    def symlink_closure(self, paths):
        """
        Given a sequence of path strings, return the set of all paths that
        can be reached from the initial set by following symlinks.

        @param paths: sequence of path strings.
        @return: a sequence of path strings that are all the unique paths that
                can be reached from the given ones after following symlinks.
        """
        paths = set(paths)
        closure = set()

        while paths:
            path = paths.pop()
            if not os.path.exists(path):
                continue
            closure.add(path)
            if os.path.islink(path):
                link_to = os.path.join(os.path.dirname(path),
                                       os.readlink(path))
                if link_to not in closure:
                    paths.add(link_to)

        return closure


    def _copy_file(self, source, dest, delete_dest=False, preserve_perm=False,
                   preserve_symlinks=False):
        """Copy files from source to dest, will be the base for {get,send}_file.

        If source is a directory and ends with a trailing slash, only the
        contents of the source directory will be copied to dest, otherwise
        source itself will be copied under dest.

        @param source: The file/directory on localhost to copy.
        @param dest: The destination path on localhost to copy to.
        @param delete_dest: A flag set to choose whether or not to delete
                            dest if it exists.
        @param preserve_perm: Tells get_file() to try to preserve the sources
                              permissions on files and dirs.
        @param preserve_symlinks: Try to preserve symlinks instead of
                                  transforming them into files/dirs on copy.
        """
        # We copy dest under source if either:
        #  1. Source is a directory and doesn't end with /.
        #  2. Source is a file and dest is a directory.
        source_is_dir = os.path.isdir(source)
        if ((source_is_dir and not source.endswith(os.sep)) or
            (not source_is_dir and os.path.isdir(dest))):
            dest = os.path.join(dest, os.path.basename(source))

        if delete_dest and os.path.exists(dest):
            # Check if it's a file or a dir and use proper remove method.
            if os.path.isdir(dest):
                shutil.rmtree(dest)
                os.mkdir(dest)
            else:
                os.remove(dest)

        if preserve_symlinks and os.path.islink(source):
            os.symlink(os.readlink(source), dest)
        # If source is a dir, use distutils.dir_util.copytree since
        # shutil.copy_tree has weird limitations.
        elif os.path.isdir(source):
            distutils.dir_util.copy_tree(source, dest,
                    preserve_symlinks=preserve_symlinks,
                    preserve_mode=preserve_perm,
                    update=1)
        else:
            shutil.copyfile(source, dest)

        if preserve_perm:
            shutil.copymode(source, dest)


    def get_file(self, source, dest, delete_dest=False, preserve_perm=True,
                 preserve_symlinks=False):
        """Copy files from source to dest.

        If source is a directory and ends with a trailing slash, only the
        contents of the source directory will be copied to dest, otherwise
        source itself will be copied under dest. This is to match the
        behavior of AbstractSSHHost.get_file().

        @param source: The file/directory on localhost to copy.
        @param dest: The destination path on localhost to copy to.
        @param delete_dest: A flag set to choose whether or not to delete
                            dest if it exists.
        @param preserve_perm: Tells get_file() to try to preserve the sources
                              permissions on files and dirs.
        @param preserve_symlinks: Try to preserve symlinks instead of
                                  transforming them into files/dirs on copy.
        """
        self._copy_file(source, dest, delete_dest=delete_dest,
                        preserve_perm=preserve_perm,
                        preserve_symlinks=preserve_symlinks)


    def send_file(self, source, dest, delete_dest=False,
                  preserve_symlinks=False, excludes=None):
        """Copy files from source to dest.

        If source is a directory and ends with a trailing slash, only the
        contents of the source directory will be copied to dest, otherwise
        source itself will be copied under dest. This is to match the
        behavior of AbstractSSHHost.send_file().

        @param source: The file/directory on the drone to send to the device.
        @param dest: The destination path on the device to copy to.
        @param delete_dest: A flag set to choose whether or not to delete
                            dest on the device if it exists.
        @param preserve_symlinks: Controls if symlinks on the source will be
                                  copied as such on the destination or
                                  transformed into the referenced
                                  file/directory.
        @param excludes: A list of file pattern that matches files not to be
                         sent. `send_file` will fail if exclude is set, since
                         local copy does not support --exclude.
        """
        if excludes:
            raise error.AutotestHostRunError(
                    '--exclude is not supported in LocalHost.send_file method. '
                    'excludes: %s' % ','.join(excludes), None)
        self._copy_file(source, dest, delete_dest=delete_dest,
                        preserve_symlinks=preserve_symlinks)


    def get_tmp_dir(self, parent='/tmp'):
        """
        Return the pathname of a directory on the host suitable
        for temporary file storage.

        The directory and its content will be deleted automatically
        on the destruction of the Host object that was used to obtain
        it.

        @param parent: The leading path to make the tmp dir.
        """
        self.run('mkdir -p "%s"' % parent)
        tmp_dir = self.run('mktemp -d -p "%s"' % parent).stdout.rstrip()
        self.tmp_dirs.append(tmp_dir)
        return tmp_dir
