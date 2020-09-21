# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os

import common
from autotest_lib.client.bin import utils as common_utils
from autotest_lib.client.common_lib.global_config import global_config


# Name of the base container.
BASE = global_config.get_config_value('AUTOSERV', 'container_base_name')

# Path to folder that contains autotest code inside container.
CONTAINER_AUTOTEST_DIR = '/usr/local/autotest'

# Naming convention of the result directory in test container.
RESULT_DIR_FMT = os.path.join(CONTAINER_AUTOTEST_DIR, 'results',
                              '%s')
# Attributes to retrieve about containers.
ATTRIBUTES = ['name', 'state']

SSP_ENABLED = global_config.get_config_value('AUTOSERV', 'enable_ssp_container',
                                             type=bool, default=True)
# url to the folder stores base container.
CONTAINER_BASE_FOLDER_URL = global_config.get_config_value('AUTOSERV',
                                                    'container_base_folder_url')
CONTAINER_BASE_URL_FMT = '%s/%%s.tar.xz' % CONTAINER_BASE_FOLDER_URL
CONTAINER_BASE_URL = CONTAINER_BASE_URL_FMT % BASE
# Default directory used to store LXC containers.
DEFAULT_CONTAINER_PATH = global_config.get_config_value('AUTOSERV',
                                                        'container_path')
# Default directory for host mounts
DEFAULT_SHARED_HOST_PATH = global_config.get_config_value(
        'AUTOSERV',
        'container_shared_host_path')

# The name of the linux domain socket used by the container pool.  Just one
# exists, so this is just a hard-coded string.
DEFAULT_CONTAINER_POOL_SOCKET = 'container_pool_socket'

# Default size for the lxc container pool.
DEFAULT_CONTAINER_POOL_SIZE = 20

# Location of the host mount point in the container.
CONTAINER_HOST_DIR = '/host'

# Path to drone_temp folder in the container, which stores the control file for
# test job to run.
CONTROL_TEMP_PATH = os.path.join(CONTAINER_AUTOTEST_DIR, 'drone_tmp')

# Bash command to return the file count in a directory. Test the existence first
# so the command can return an error code if the directory doesn't exist.
COUNT_FILE_CMD = '[ -d %(dir)s ] && ls %(dir)s | wc -l'

# Command line to append content to a file
APPEND_CMD_FMT = ('echo \'%(content)s\' | sudo tee --append %(file)s'
                  '> /dev/null')

# Flag to indicate it's running in a Moblab. Due to crbug.com/457496, lxc-ls has
# different behavior in Moblab.
IS_MOBLAB = common_utils.is_moblab()

if IS_MOBLAB:
    SITE_PACKAGES_PATH = '/usr/lib64/python2.7/site-packages'
    CONTAINER_SITE_PACKAGES_PATH = '/usr/local/lib/python2.7/dist-packages/'
else:
    SITE_PACKAGES_PATH = os.path.join(common.autotest_dir, 'site-packages')
    CONTAINER_SITE_PACKAGES_PATH = os.path.join(CONTAINER_AUTOTEST_DIR,
                                                'site-packages')

# TODO(dshi): If we are adding more logic in how lxc should interact with
# different systems, we should consider code refactoring to use a setting-style
# object to store following flags mapping to different systems.
# TODO(crbug.com/464834): Snapshot clone is disabled until Moblab can
# support overlayfs or aufs, which requires a newer kernel.
SUPPORT_SNAPSHOT_CLONE = not IS_MOBLAB

# Number of seconds to wait for network to be up in a container.
NETWORK_INIT_TIMEOUT = 300
# Network bring up is slower in Moblab.
NETWORK_INIT_CHECK_INTERVAL = 2 if IS_MOBLAB else 0.1

# Number of seconds to download files from devserver. We chose a timeout that
# is on the same order as the permitted CTS runtime for normal jobs (1h). In
# principle we should not retry timeouts as they indicate server/network
# overload, but we may be tempted to retry for other failures.
DEVSERVER_CALL_TIMEOUT = 3600
# Number of retries to download files from devserver. There is no point in
# having more than one retry for a file download.
DEVSERVER_CALL_RETRY = 2
# Average delay before attempting a retry to download from devserver. This
# value needs to be large enough to allow an overloaded server/network to
# calm down even in the face of retries.
DEVSERVER_CALL_DELAY = 600

# Type string for container related metadata.
CONTAINER_CREATE_METADB_TYPE = 'container_create'
CONTAINER_CREATE_RETRY_METADB_TYPE = 'container_create_retry'
CONTAINER_RUN_TEST_METADB_TYPE = 'container_run_test'

# The container's hostname MUST start with `test-` or `test_`. DHCP server in
# MobLab uses that prefix to determine the lease time.  Note that `test_` is not
# a valid hostname as hostnames cannot contain underscores.  Work is underway to
# migrate to `test-`.  See crbug/726131.
CONTAINER_UTSNAME_FORMAT = 'test-%s'

STATS_KEY = 'chromeos/autotest/lxc'

CONTAINER_POOL_METRICS_PREFIX = 'chromeos/autotest/container_pool'
