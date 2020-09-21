# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import socket
import time

import common

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.global_config import global_config
from autotest_lib.site_utils.lxc import config as lxc_config
from autotest_lib.site_utils.lxc import constants
from autotest_lib.site_utils.lxc import container_pool
from autotest_lib.site_utils.lxc import lxc
from autotest_lib.site_utils.lxc.cleanup_if_fail import cleanup_if_fail
from autotest_lib.site_utils.lxc.base_image import BaseImage
from autotest_lib.site_utils.lxc.constants import \
    CONTAINER_POOL_METRICS_PREFIX as METRICS_PREFIX
from autotest_lib.site_utils.lxc.container import Container
from autotest_lib.site_utils.lxc.container_factory import ContainerFactory

try:
    from chromite.lib import metrics
    from infra_libs import ts_mon
except ImportError:
    import mock
    metrics = utils.metrics_mock
    ts_mon = mock.Mock()


# Timeout (in seconds) for container pool operations.
_CONTAINER_POOL_TIMEOUT = 3

_USE_LXC_POOL = global_config.get_config_value('LXC_POOL', 'use_lxc_pool',
                                               type=bool)

class ContainerBucket(object):
    """A wrapper class to interact with containers in a specific container path.
    """

    def __init__(self, container_path=constants.DEFAULT_CONTAINER_PATH,
                 container_factory=None):
        """Initialize a ContainerBucket.

        @param container_path: Path to the directory used to store containers.
                               Default is set to AUTOSERV/container_path in
                               global config.
        @param container_factory: A factory for creating Containers.
        """
        self.container_path = os.path.realpath(container_path)
        if container_factory is not None:
            self._factory = container_factory
        else:
            # Pick the correct factory class to use (pool-based, or regular)
            # based on the config variable.
            factory_class = ContainerFactory
            if _USE_LXC_POOL:
                logging.debug('Using container pool')
                factory_class = _PoolBasedFactory

            # Pass in the container path so that the bucket is hermetic (i.e. so
            # that if the container path is customized, the base image doesn't
            # fall back to using the default container path).
            try:
                base_image_ok = True
                container = BaseImage(self.container_path).get()
            except error.ContainerError as e:
                base_image_ok = False
                raise e
            finally:
                metrics.Counter(METRICS_PREFIX + '/base_image',
                                field_spec=[ts_mon.BooleanField('corrupted')]
                                ).increment(
                                    fields={'corrupted': not base_image_ok})
            self._factory = factory_class(
                base_container=container,
                lxc_path=self.container_path)
        self.container_cache = {}


    def get_all(self, force_update=False):
        """Get details of all containers.

        Retrieves all containers owned by the bucket.  Note that this doesn't
        include the base container, or any containers owned by the container
        pool.

        @param force_update: Boolean, ignore cached values if set.

        @return: A dictionary of all containers with detailed attributes,
                 indexed by container name.
        """
        info_collection = lxc.get_container_info(self.container_path)
        containers = {} if force_update else self.container_cache
        for info in info_collection:
            if info["name"] in containers:
                continue
            container = Container.create_from_existing_dir(self.container_path,
                                                           **info)
            # Active containers have an ID.  Zygotes and base containers, don't.
            if container.id is not None:
                containers[container.id] = container
        self.container_cache = containers
        return containers


    def get_container(self, container_id):
        """Get a container with matching name.

        @param container_id: ID of the container.

        @return: A container object with matching name. Returns None if no
                 container matches the given name.
        """
        if container_id in self.container_cache:
            return self.container_cache[container_id]

        return self.get_all().get(container_id, None)


    def exist(self, container_id):
        """Check if a container exists with the given name.

        @param container_id: ID of the container.

        @return: True if the container with the given ID exists, otherwise
                 returns False.
        """
        return self.get_container(container_id) != None


    def destroy_all(self):
        """Destroy all containers, base must be destroyed at the last.
        """
        containers = self.get_all().values()
        for container in sorted(
                containers, key=lambda n: 1 if n.name == constants.BASE else 0):
            key = container.id
            logging.info('Destroy container %s.', container.name)
            container.destroy()
            del self.container_cache[key]



    @metrics.SecondsTimerDecorator(
        '%s/setup_test_duration' % constants.STATS_KEY)
    @cleanup_if_fail()
    def setup_test(self, container_id, job_id, server_package_url, result_path,
                   control=None, skip_cleanup=False, job_folder=None,
                   dut_name=None):
        """Setup test container for the test job to run.

        The setup includes:
        1. Install autotest_server package from given url.
        2. Copy over local shadow_config.ini.
        3. Mount local site-packages.
        4. Mount test result directory.

        TODO(dshi): Setup also needs to include test control file for autoserv
                    to run in container.

        @param container_id: ID to assign to the test container.
        @param job_id: Job id for the test job to run in the test container.
        @param server_package_url: Url to download autotest_server package.
        @param result_path: Directory to be mounted to container to store test
                            results.
        @param control: Path to the control file to run the test job. Default is
                        set to None.
        @param skip_cleanup: Set to True to skip cleanup, used to troubleshoot
                             container failures.
        @param job_folder: Folder name of the job, e.g., 123-debug_user.
        @param dut_name: Name of the dut to run test, used as the hostname of
                         the container. Default is None.
        @return: A Container object for the test container.

        @raise ContainerError: If container does not exist, or not running.
        """
        start_time = time.time()

        if not os.path.exists(result_path):
            raise error.ContainerError('Result directory does not exist: %s',
                                       result_path)
        result_path = os.path.abspath(result_path)

        # Save control file to result_path temporarily. The reason is that the
        # control file in drone_tmp folder can be deleted during scheduler
        # restart. For test not using SSP, the window between test starts and
        # control file being picked up by the test is very small (< 2 seconds).
        # However, for tests using SSP, it takes around 1 minute before the
        # container is setup. If scheduler is restarted during that period, the
        # control file will be deleted, and the test will fail.
        if control:
            control_file_name = os.path.basename(control)
            safe_control = os.path.join(result_path, control_file_name)
            utils.run('cp %s %s' % (control, safe_control))

        # Create test container from the base container.
        container = self._factory.create_container(container_id)

        # Deploy server side package
        container.install_ssp(server_package_url)

        deploy_config_manager = lxc_config.DeployConfigManager(container)
        deploy_config_manager.deploy_pre_start()

        # Copy over control file to run the test job.
        if control:
            container.install_control_file(safe_control)

        mount_entries = [(constants.SITE_PACKAGES_PATH,
                          constants.CONTAINER_SITE_PACKAGES_PATH,
                          True),
                         (result_path,
                          os.path.join(constants.RESULT_DIR_FMT % job_folder),
                          False),
        ]

        # Update container config to mount directories.
        for source, destination, readonly in mount_entries:
            container.mount_dir(source, destination, readonly)

        # Update file permissions.
        # TODO(dshi): crbug.com/459344 Skip following action when test container
        # can be unprivileged container.
        autotest_path = os.path.join(
                container.rootfs,
                constants.CONTAINER_AUTOTEST_DIR.lstrip(os.path.sep))
        utils.run('sudo chown -R root "%s"' % autotest_path)
        utils.run('sudo chgrp -R root "%s"' % autotest_path)

        container.start(wait_for_network=True)
        deploy_config_manager.deploy_post_start()

        # Update the hostname of the test container to be `dut-name`.
        # Some TradeFed tests use hostname in test results, which is used to
        # group test results in dashboard. The default container name is set to
        # be the name of the folder, which is unique (as it is composed of job
        # id and timestamp. For better result view, the container's hostname is
        # set to be a string containing the dut hostname.
        if dut_name:
            container.set_hostname(constants.CONTAINER_UTSNAME_FORMAT %
                                   dut_name.replace('.', '-'))

        container.modify_import_order()

        container.verify_autotest_setup(job_folder)

        logging.debug('Test container %s is set up.', container.name)
        return container


class _PoolBasedFactory(ContainerFactory):
    """A ContainerFactory that queries the running container pool.

    Implementation falls back to the regular container factory behaviour
    (i.e. locally cloning a container) if the pool is unavailable or if it does
    not return a bucket before the specified timeout.
    """

    def __init__(self, *args, **kwargs):
        super(_PoolBasedFactory, self).__init__(*args, **kwargs)
        try:
            self._client = container_pool.Client()
        except (socket.error, socket.timeout) as e:
            # If an error occurs connecting to the container pool, fall back to
            # the default container factory.
            logging.exception('Container pool connection failed.')
            self._client = None


    def create_container(self, new_id):
        """Creates a new container.

        Attempts to retrieve a container from the container pool.  If that
        operation fails, this falls back to the parent class behaviour.

        @param new_id: ContainerId to assign to the new container.  Containers
                       must be assigned an ID before they can be released from
                       the container pool.

        @return: The new container.
        """
        container = None
        if self._client:
            try:
                container = self._client.get_container(new_id,
                                                       _CONTAINER_POOL_TIMEOUT)
            except Exception:
                logging.exception('Error communicating with container pool.')
            else:
                if container is not None:
                    logging.debug('Retrieved container from pool: %s',
                                  container.name)
                    return container
        metrics.Counter(METRICS_PREFIX + '/containers_served',
                        field_spec = [ts_mon.BooleanField('from_pool')]
                        ).increment(fields={
                            'from_pool': (container is not None)})
        if container is not None:
            return container

        # If the container pool did not yield a container, make one locally.
        logging.warning('Unable to obtain container from pre-populated pool.  '
                        'Creating container locally.  This slows server tests '
                        'down and should be debugged even if local creation '
                        'works out.')
        return super(_PoolBasedFactory, self).create_container(new_id)
