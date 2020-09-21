# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import collections
import json
import logging
import os
import re
import tempfile
import time

import common
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.site_utils.lxc import constants
from autotest_lib.site_utils.lxc import lxc
from autotest_lib.site_utils.lxc import utils as lxc_utils

try:
    from chromite.lib import metrics
except ImportError:
    metrics = utils.metrics_mock


# Naming convention of test container, e.g., test_300_1422862512_2424, where:
# 300:        The test job ID.
# 1422862512: The tick when container is created.
# 2424:       The PID of autoserv that starts the container.
_TEST_CONTAINER_NAME_FMT = 'test_%s_%d_%d'
# Name of the container ID file.
_CONTAINER_ID_FILENAME = 'container_id.json'


class ContainerId(collections.namedtuple('ContainerId',
                                         ['job_id', 'creation_time', 'pid'])):
    """An identifier for containers."""

    # Optimization.  Avoids __dict__ creation.  Empty because this subclass has
    # no instance vars of its own.
    __slots__ = ()


    def __str__(self):
        return _TEST_CONTAINER_NAME_FMT % self


    def save(self, path):
        """Saves the ID to the given path.

        @param path: Path to a directory where the container ID will be
                     serialized.
        """
        dst = os.path.join(path, _CONTAINER_ID_FILENAME)
        with open(dst, 'w') as f:
            json.dump(self, f)

    @classmethod
    def load(cls, path):
        """Reads the ID from the given path.

        @param path: Path to check for a serialized container ID.

        @return: A container ID if one is found on the given path, or None
                 otherwise.

        @raise ValueError: If a JSON load error occurred.
        @raise TypeError: If the file was valid JSON but didn't contain a valid
                          ContainerId.
        """
        src = os.path.join(path, _CONTAINER_ID_FILENAME)

        try:
            with open(src, 'r') as f:
                return cls(*json.load(f))
        except IOError:
            # File not found, or couldn't be opened for some other reason.
            # Treat all these cases as no ID.
            return None


    @classmethod
    def create(cls, job_id, ctime=None, pid=None):
        """Creates a new container ID.

        @param job_id: The first field in the ID.
        @param ctime: The second field in the ID.  Optional. If not provided,
                      the current epoch timestamp is used.
        @param pid: The third field in the ID.  Optional.  If not provided, the
                    PID of the current process is used.
        """
        if ctime is None:
            ctime = int(time.time())
        if pid is None:
            pid = os.getpid()
        return cls(job_id, ctime, pid)


class Container(object):
    """A wrapper class of an LXC container.

    The wrapper class provides methods to interact with a container, e.g.,
    start, stop, destroy, run a command. It also has attributes of the
    container, including:
    name: Name of the container.
    state: State of the container, e.g., ABORTING, RUNNING, STARTING, STOPPED,
           or STOPPING.

    lxc-ls can also collect other attributes of a container including:
    ipv4: IP address for IPv4.
    ipv6: IP address for IPv6.
    autostart: If the container will autostart at system boot.
    pid: Process ID of the container.
    memory: Memory used by the container, as a string, e.g., "6.2MB"
    ram: Physical ram used by the container, as a string, e.g., "6.2MB"
    swap: swap used by the container, as a string, e.g., "1.0MB"

    For performance reason, such info is not collected for now.

    The attributes available are defined in ATTRIBUTES constant.
    """

    def __init__(self, container_path, name, attribute_values, src=None,
                 snapshot=False):
        """Initialize an object of LXC container with given attribute values.

        @param container_path: Directory that stores the container.
        @param name: Name of the container.
        @param attribute_values: A dictionary of attribute values for the
                                 container.
        @param src: An optional source container.  If provided, the source
                    continer is cloned, and the new container will point to the
                    clone.
        @param snapshot: If a source container was specified, this argument
                         specifies whether or not to create a snapshot clone.
                         The default is to attempt to create a snapshot.
                         If a snapshot is requested and creating the snapshot
                         fails, a full clone will be attempted.
        """
        self.container_path = os.path.realpath(container_path)
        # Path to the rootfs of the container. This will be initialized when
        # property rootfs is retrieved.
        self._rootfs = None
        self.name = name
        for attribute, value in attribute_values.iteritems():
            setattr(self, attribute, value)

        # Clone the container
        if src is not None:
            # Clone the source container to initialize this one.
            lxc_utils.clone(src.container_path, src.name, self.container_path,
                            self.name, snapshot)
            # Newly cloned containers have no ID.
            self._id = None
        else:
            # This may be an existing container.  Try to read the ID.
            try:
                self._id = ContainerId.load(
                        os.path.join(self.container_path, self.name))
            except (ValueError, TypeError):
                # Ignore load errors.  ContainerBucket currently queries every
                # container quite frequently, and emitting exceptions here would
                # cause any invalid containers on a server to block all
                # ContainerBucket.get_all calls (see crbug/783865).
                # TODO(kenobi): Containers with invalid ID files are probably
                # the result of an aborted or failed operation.  There is a
                # non-zero chance that such containers would contain leftover
                # state, or themselves be corrupted or invalid.  Should we
                # provide APIs for checking if a container is in this state?
                logging.exception('Error loading ID for container %s:',
                                  self.name)
                self._id = None


    @classmethod
    def create_from_existing_dir(cls, lxc_path, name, **kwargs):
        """Creates a new container instance for an lxc container that already
        exists on disk.

        @param lxc_path: The LXC path for the container.
        @param name: The container name.

        @raise error.ContainerError: If the container doesn't already exist.

        @return: The new container.
        """
        return cls(lxc_path, name, kwargs)


    # Containers have a name and an ID.  The name is simply the name of the LXC
    # container.  The ID is the actual key that is used to identify the
    # container to the autoserv system.  In the case of a JIT-created container,
    # we have the ID at the container's creation time so we use that to name the
    # container.  This may not be the case for other types of containers.
    @classmethod
    def clone(cls, src, new_name=None, new_path=None, snapshot=False,
              cleanup=False):
        """Creates a clone of this container.

        @param src: The original container.
        @param new_name: Name for the cloned container.  If this is not
                         provided, a random unique container name will be
                         generated.
        @param new_path: LXC path for the cloned container (optional; if not
                         specified, the new container is created in the same
                         directory as the source container).
        @param snapshot: Whether to snapshot, or create a full clone.  Note that
                         snapshot cloning is not supported on all platforms.  If
                         this code is running on a platform that does not
                         support snapshot clones, this flag is ignored.
        @param cleanup: If a container with the given name and path already
                        exist, clean it up first.
        """
        if new_path is None:
            new_path = src.container_path

        if new_name is None:
            _, new_name = os.path.split(
                tempfile.mkdtemp(dir=new_path, prefix='container.'))
            logging.debug('Generating new name for container: %s', new_name)
        else:
            # If a container exists at this location, clean it up first
            container_folder = os.path.join(new_path, new_name)
            if lxc_utils.path_exists(container_folder):
                if not cleanup:
                    raise error.ContainerError('Container %s already exists.' %
                                               new_name)
                container = Container.create_from_existing_dir(new_path,
                                                               new_name)
                try:
                    container.destroy()
                except error.CmdError as e:
                    # The container could be created in a incompleted
                    # state. Delete the container folder instead.
                    logging.warn('Failed to destroy container %s, error: %s',
                                 new_name, e)
                    utils.run('sudo rm -rf "%s"' % container_folder)
            # Create the directory prior to creating the new container.  This
            # puts the ownership of the container under the current process's
            # user, rather than root.  This is necessary to enable the
            # ContainerId to serialize properly.
            os.mkdir(container_folder)

        # Create and return the new container.
        new_container = cls(new_path, new_name, {}, src, snapshot)

        return new_container


    def refresh_status(self):
        """Refresh the status information of the container.
        """
        containers = lxc.get_container_info(self.container_path, name=self.name)
        if not containers:
            raise error.ContainerError(
                    'No container found in directory %s with name of %s.' %
                    (self.container_path, self.name))
        attribute_values = containers[0]
        for attribute, value in attribute_values.iteritems():
            setattr(self, attribute, value)


    @property
    def rootfs(self):
        """Path to the rootfs of the container.

        This property returns the path to the rootfs of the container, that is,
        the folder where the container stores its local files. It reads the
        attribute lxc.rootfs from the config file of the container, e.g.,
            lxc.rootfs = /usr/local/autotest/containers/t4/rootfs
        If the container is created with snapshot, the rootfs is a chain of
        folders, separated by `:` and ordered by how the snapshot is created,
        e.g.,
            lxc.rootfs = overlayfs:/usr/local/autotest/containers/base/rootfs:
            /usr/local/autotest/containers/t4_s/delta0
        This function returns the last folder in the chain, in above example,
        that is `/usr/local/autotest/containers/t4_s/delta0`

        Files in the rootfs will be accessible directly within container. For
        example, a folder in host "[rootfs]/usr/local/file1", can be accessed
        inside container by path "/usr/local/file1". Note that symlink in the
        host can not across host/container boundary, instead, directory mount
        should be used, refer to function mount_dir.

        @return: Path to the rootfs of the container.
        """
        if not self._rootfs:
            lxc_rootfs = self._get_lxc_config('lxc.rootfs')[0]
            cloned_from_snapshot = ':' in lxc_rootfs
            if cloned_from_snapshot:
                self._rootfs = lxc_rootfs.split(':')[-1]
            else:
                self._rootfs = lxc_rootfs
        return self._rootfs


    def attach_run(self, command, bash=True):
        """Attach to a given container and run the given command.

        @param command: Command to run in the container.
        @param bash: Run the command through bash -c "command". This allows
                     pipes to be used in command. Default is set to True.

        @return: The output of the command.

        @raise error.CmdError: If container does not exist, or not running.
        """
        cmd = 'sudo lxc-attach -P %s -n %s' % (self.container_path, self.name)
        if bash and not command.startswith('bash -c'):
            command = 'bash -c "%s"' % utils.sh_escape(command)
        cmd += ' -- %s' % command
        # TODO(dshi): crbug.com/459344 Set sudo to default to False when test
        # container can be unprivileged container.
        return utils.run(cmd)


    def is_network_up(self):
        """Check if network is up in the container by curl base container url.

        @return: True if the network is up, otherwise False.
        """
        try:
            self.attach_run('curl --head %s' % constants.CONTAINER_BASE_URL)
            return True
        except error.CmdError as e:
            logging.debug(e)
            return False


    @metrics.SecondsTimerDecorator(
        '%s/container_start_duration' % constants.STATS_KEY)
    def start(self, wait_for_network=True):
        """Start the container.

        @param wait_for_network: True to wait for network to be up. Default is
                                 set to True.

        @raise ContainerError: If container does not exist, or fails to start.
        """
        cmd = 'sudo lxc-start -P %s -n %s -d' % (self.container_path, self.name)
        output = utils.run(cmd).stdout
        if not self.is_running():
            raise error.ContainerError(
                    'Container %s failed to start. lxc command output:\n%s' %
                    (os.path.join(self.container_path, self.name),
                     output))

        if wait_for_network:
            logging.debug('Wait for network to be up.')
            start_time = time.time()
            utils.poll_for_condition(
                condition=self.is_network_up,
                timeout=constants.NETWORK_INIT_TIMEOUT,
                sleep_interval=constants.NETWORK_INIT_CHECK_INTERVAL)
            logging.debug('Network is up after %.2f seconds.',
                          time.time() - start_time)


    @metrics.SecondsTimerDecorator(
        '%s/container_stop_duration' % constants.STATS_KEY)
    def stop(self):
        """Stop the container.

        @raise ContainerError: If container does not exist, or fails to start.
        """
        cmd = 'sudo lxc-stop -P %s -n %s' % (self.container_path, self.name)
        output = utils.run(cmd).stdout
        self.refresh_status()
        if self.state != 'STOPPED':
            raise error.ContainerError(
                    'Container %s failed to be stopped. lxc command output:\n'
                    '%s' % (os.path.join(self.container_path, self.name),
                            output))


    @metrics.SecondsTimerDecorator(
        '%s/container_destroy_duration' % constants.STATS_KEY)
    def destroy(self, force=True):
        """Destroy the container.

        @param force: Set to True to force to destroy the container even if it's
                      running. This is faster than stop a container first then
                      try to destroy it. Default is set to True.

        @raise ContainerError: If container does not exist or failed to destroy
                               the container.
        """
        logging.debug('Destroying container %s/%s',
                      self.container_path,
                      self.name)
        cmd = 'sudo lxc-destroy -P %s -n %s' % (self.container_path,
                                                self.name)
        if force:
            cmd += ' -f'
        utils.run(cmd)


    def mount_dir(self, source, destination, readonly=False):
        """Mount a directory in host to a directory in the container.

        @param source: Directory in host to be mounted.
        @param destination: Directory in container to mount the source directory
        @param readonly: Set to True to make a readonly mount, default is False.
        """
        # Destination path in container must be relative.
        destination = destination.lstrip('/')
        # Create directory in container for mount.  Changes to container rootfs
        # require sudo.
        utils.run('sudo mkdir -p %s' % os.path.join(self.rootfs, destination))
        mount = ('%s %s none bind%s 0 0' %
                 (source, destination, ',ro' if readonly else ''))
        self._set_lxc_config('lxc.mount.entry', mount)

    def verify_autotest_setup(self, job_folder):
        """Verify autotest code is set up properly in the container.

        @param job_folder: Name of the job result folder.

        @raise ContainerError: If autotest code is not set up properly.
        """
        # Test autotest code is setup by verifying a list of
        # (directory, minimum file count)
        directories_to_check = [
                (constants.CONTAINER_AUTOTEST_DIR, 3),
                (constants.RESULT_DIR_FMT % job_folder, 0),
                (constants.CONTAINER_SITE_PACKAGES_PATH, 3)]
        for directory, count in directories_to_check:
            result = self.attach_run(command=(constants.COUNT_FILE_CMD %
                                              {'dir': directory})).stdout
            logging.debug('%s entries in %s.', int(result), directory)
            if int(result) < count:
                raise error.ContainerError('%s is not properly set up.' %
                                           directory)
        # lxc-attach and run command does not run in shell, thus .bashrc is not
        # loaded. Following command creates a symlink in /usr/bin/ for gsutil
        # if it's installed.
        # TODO(dshi): Remove this code after lab container is updated with
        # gsutil installed in /usr/bin/
        self.attach_run('test -f /root/gsutil/gsutil && '
                        'ln -s /root/gsutil/gsutil /usr/bin/gsutil || true')


    def modify_import_order(self):
        """Swap the python import order of lib and local/lib.

        In Moblab, the host's python modules located in
        /usr/lib64/python2.7/site-packages is mounted to following folder inside
        container: /usr/local/lib/python2.7/dist-packages/. The modules include
        an old version of requests module, which is used in autotest
        site-packages. For test, the module is only used in
        dev_server/symbolicate_dump for requests.call and requests.codes.OK.
        When pip is installed inside the container, it installs requests module
        with version of 2.2.1 in /usr/lib/python2.7/dist-packages/. The version
        is newer than the one used in autotest site-packages, but not the latest
        either.
        According to /usr/lib/python2.7/site.py, modules in /usr/local/lib are
        imported before the ones in /usr/lib. That leads to pip to use the older
        version of requests (0.11.2), and it will fail. On the other hand,
        requests module 2.2.1 can't be installed in CrOS (refer to CL:265759),
        and higher version of requests module can't work with pip.
        The only fix to resolve this is to switch the import order, so modules
        in /usr/lib can be imported before /usr/local/lib.
        """
        site_module = '/usr/lib/python2.7/site.py'
        self.attach_run("sed -i ':a;N;$!ba;s/\"local\/lib\",\\n/"
                        "\"lib_placeholder\",\\n/g' %s" % site_module)
        self.attach_run("sed -i ':a;N;$!ba;s/\"lib\",\\n/"
                        "\"local\/lib\",\\n/g' %s" % site_module)
        self.attach_run('sed -i "s/lib_placeholder/lib/g" %s' %
                        site_module)


    def is_running(self):
        """Returns whether or not this container is currently running."""
        self.refresh_status()
        return self.state == 'RUNNING'


    def set_hostname(self, hostname):
        """Sets the hostname within the container.

        This method can only be called on a running container.

        @param hostname The new container hostname.

        @raise ContainerError: If the container is not running.
        """
        if not self.is_running():
            raise error.ContainerError(
                    'set_hostname can only be called on running containers.')

        self.attach_run('hostname %s' % (hostname))
        self.attach_run(constants.APPEND_CMD_FMT % {
                'content': '127.0.0.1 %s' % (hostname),
                'file': '/etc/hosts'})


    def install_ssp(self, ssp_url):
        """Downloads and installs the given server package.

        @param ssp_url: The URL of the ssp to download and install.
        """
        usr_local_path = os.path.join(self.rootfs, 'usr', 'local')
        autotest_pkg_path = os.path.join(usr_local_path,
                                         'autotest_server_package.tar.bz2')
        # Changes within the container rootfs require sudo.
        utils.run('sudo mkdir -p %s'% usr_local_path)

        lxc.download_extract(ssp_url, autotest_pkg_path, usr_local_path)


    def install_control_file(self, control_file):
        """Installs the given control file.

        The given file will be copied into the container.

        @param control_file: Path to the control file to install.
        """
        dst = os.path.join(constants.CONTROL_TEMP_PATH,
                           os.path.basename(control_file))
        self.copy(control_file, dst)


    def copy(self, host_path, container_path):
        """Copies files into the container.

        @param host_path: Path to the source file/dir to be copied.
        @param container_path: Path to the destination dir (in the container).
        """
        dst_path = os.path.join(self.rootfs,
                                container_path.lstrip(os.path.sep))
        self._do_copy(src=host_path, dst=dst_path)


    @property
    def id(self):
        """Returns the container ID."""
        return self._id


    @id.setter
    def id(self, new_id):
        """Sets the container ID."""
        self._id = new_id;
        # Persist the ID so other container objects can pick it up.
        self._id.save(os.path.join(self.container_path, self.name))


    def _do_copy(self, src, dst):
        """Copies files and directories on the host system.

        @param src: The source file or directory.
        @param dst: The destination file or directory.  If the path to the
                    destination does not exist, it will be created.
        """
        # Create the dst dir. mkdir -p will not fail if dst_dir exists.
        dst_dir = os.path.dirname(dst)
        # Make sure the source ends with `/.` if it's a directory. Otherwise
        # command cp will not work.
        if os.path.isdir(src) and os.path.split(src)[1] != '.':
            src = os.path.join(src, '.')
        utils.run("sudo sh -c 'mkdir -p \"%s\" && cp -RL \"%s\" \"%s\"'" %
                  (dst_dir, src, dst))

    def _set_lxc_config(self, key, value):
        """Sets an LXC config value for this container.

        Configuration changes made while a container is running don't take
        effect until the container is restarted.  Since this isn't a scenario
        that should ever come up in our use cases, calling this method on a
        running container will cause a ContainerError.

        @param key: The LXC config key to set.
        @param value: The value to use for the given key.

        @raise error.ContainerError: If the container is already started.
        """
        if self.is_running():
            raise error.ContainerError(
                '_set_lxc_config(%s, %s) called on a running container.' %
                (key, value))
        config_file = os.path.join(self.container_path, self.name, 'config')
        config = '%s = %s' % (key, value)
        utils.run(
            constants.APPEND_CMD_FMT % {'content': config, 'file': config_file})


    def _get_lxc_config(self, key):
        """Retrieves an LXC config value from the container.

        @param key The key of the config value to retrieve.
        """
        cmd = ('sudo lxc-info -P %s -n %s -c %s' %
               (self.container_path, self.name, key))
        config = utils.run(cmd).stdout.strip().splitlines()

        # Strip the decoration from line 1 of the output.
        match = re.match('%s = (.*)' % key, config[0])
        if not match:
            raise error.ContainerError(
                    'Config %s not found for container %s. (%s)' %
                    (key, self.name, ','.join(config)))
        config[0] = match.group(1)
        return config
