# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
This module helps to deploy config files and shared folders from host to
container. It reads the settings from a setting file (ssp_deploy_config), and
deploy the config files based on the settings. The setting file has a json
string of a list of deployment settings. For example:
[{
    "source": "/etc/resolv.conf",
    "target": "/etc/resolv.conf",
    "append": true,
    "permission": 400
 },
 {
    "source": "ssh",
    "target": "/root/.ssh",
    "append": false,
    "permission": 400
 },
 {
    "source": "/usr/local/autotest/results/shared",
    "target": "/usr/local/autotest/results/shared",
    "mount": true,
    "readonly": false,
    "force_create": true
 }
]

Definition of each attribute for config files are as follows:
source: config file in host to be copied to container.
target: config file's location inside container.
append: true to append the content of config file to existing file inside
        container. If it's set to false, the existing file inside container will
        be overwritten.
permission: Permission to set to the config file inside container.

Example:
{
    "source": "/etc/resolv.conf",
    "target": "/etc/resolv.conf",
    "append": true,
    "permission": 400
}
The above example will:
1. Append the content of /etc/resolv.conf in host machine to file
   /etc/resolv.conf inside container.
2. Copy all files in ssh to /root/.ssh in container.
3. Change all these files' permission to 400

Definition of each attribute for sharing folders are as follows:
source: a folder in host to be mounted in container.
target: the folder's location inside container.
mount: true to mount the source folder onto the target inside container.
       A setting with false value of mount is invalid.
readonly: true if the mounted folder inside container should be readonly.
force_create: true to create the source folder if it doesn't exist.

Example:
 {
    "source": "/usr/local/autotest/results/shared",
    "target": "/usr/local/autotest/results/shared",
    "mount": true,
    "readonly": false,
    "force_create": true
 }
The above example will mount folder "/usr/local/autotest/results/shared" in the
host to path "/usr/local/autotest/results/shared" inside the container. The
folder can be written to inside container. If the source folder doesn't exist,
it will be created as `force_create` is set to true.

The setting file (ssp_deploy_config) lives in AUTOTEST_DIR folder.
For relative file path specified in ssp_deploy_config, AUTOTEST_DIR/containers
is the parent folder.
The setting file can be overridden by a shadow config, ssp_deploy_shadow_config.
For lab servers, puppet should be used to deploy ssp_deploy_shadow_config to
AUTOTEST_DIR and the configure files to AUTOTEST_DIR/containers.

The default setting file (ssp_deploy_config) contains
For SSP to work with none-lab servers, e.g., moblab and developer's workstation,
the module still supports copy over files like ssh config and autotest
shadow_config to container when AUTOTEST_DIR/containers/ssp_deploy_config is not
presented.

"""

import collections
import getpass
import json
import os
import socket

import common
from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib import utils
from autotest_lib.site_utils.lxc import constants
from autotest_lib.site_utils.lxc import utils as lxc_utils


config = global_config.global_config

# Path to ssp_deploy_config and ssp_deploy_shadow_config.
SSP_DEPLOY_CONFIG_FILE = os.path.join(common.autotest_dir,
                                      'ssp_deploy_config.json')
SSP_DEPLOY_SHADOW_CONFIG_FILE = os.path.join(common.autotest_dir,
                                             'ssp_deploy_shadow_config.json')
# A temp folder used to store files to be appended to the files inside
# container.
_APPEND_FOLDER = '/usr/local/ssp_append'

DeployConfig = collections.namedtuple(
        'DeployConfig', ['source', 'target', 'append', 'permission'])
MountConfig = collections.namedtuple(
        'MountConfig', ['source', 'target', 'mount', 'readonly',
                        'force_create'])


class SSPDeployError(Exception):
    """Exception raised if any error occurs when setting up test container."""


class DeployConfigManager(object):
    """An object to deploy config to container.

    The manager retrieves deploy configs from ssp_deploy_config or
    ssp_deploy_shadow_config, and sets up the container accordingly.
    For example:
    1. Copy given config files to specified location inside container.
    2. Append the content of given config files to specific files inside
       container.
    3. Make sure the config files have proper permission inside container.

    """

    @staticmethod
    def validate_path(deploy_config):
        """Validate the source and target in deploy_config dict.

        @param deploy_config: A dictionary of deploy config to be validated.

        @raise SSPDeployError: If any path in deploy config is invalid.
        """
        target = deploy_config['target']
        source = deploy_config['source']
        if not os.path.isabs(target):
            raise SSPDeployError('Target path must be absolute path: %s' %
                                 target)
        if not os.path.isabs(source):
            if source.startswith('~'):
                # This is to handle the case that the script is run with sudo.
                inject_user_path = ('~%s%s' % (utils.get_real_user(),
                                               source[1:]))
                source = os.path.expanduser(inject_user_path)
            else:
                source = os.path.join(common.autotest_dir, source)
            # Update the source setting in deploy config with the updated path.
            deploy_config['source'] = source


    @staticmethod
    def validate(deploy_config):
        """Validate the deploy config.

        Deploy configs need to be validated and pre-processed, e.g.,
        1. Target must be an absolute path.
        2. Source must be updated to be an absolute path.

        @param deploy_config: A dictionary of deploy config to be validated.

        @return: A DeployConfig object that contains the deploy config.

        @raise SSPDeployError: If the deploy config is invalid.

        """
        DeployConfigManager.validate_path(deploy_config)
        return DeployConfig(**deploy_config)


    @staticmethod
    def validate_mount(deploy_config):
        """Validate the deploy config for mounting a directory.

        Deploy configs need to be validated and pre-processed, e.g.,
        1. Target must be an absolute path.
        2. Source must be updated to be an absolute path.
        3. Mount must be true.

        @param deploy_config: A dictionary of deploy config to be validated.

        @return: A DeployConfig object that contains the deploy config.

        @raise SSPDeployError: If the deploy config is invalid.

        """
        DeployConfigManager.validate_path(deploy_config)
        c = MountConfig(**deploy_config)
        if not c.mount:
            raise SSPDeployError('`mount` must be true.')
        if not c.force_create and not os.path.exists(c.source):
            raise SSPDeployError('`source` does not exist.')
        return c


    def __init__(self, container, config_file=None):
        """Initialize the deploy config manager.

        @param container: The container needs to deploy config.
        @param config_file: An optional config file.  For testing.
        """
        self.container = container
        # If shadow config is used, the deployment procedure will skip some
        # special handling of config file, e.g.,
        # 1. Set enable_master_ssh to False in autotest shadow config.
        # 2. Set ssh logleve to ERROR for all hosts.
        if config_file is None:
            self.is_shadow_config = os.path.exists(
                    SSP_DEPLOY_SHADOW_CONFIG_FILE)
            config_file = (
                    SSP_DEPLOY_SHADOW_CONFIG_FILE if self.is_shadow_config
                    else SSP_DEPLOY_CONFIG_FILE)
        else:
            self.is_shadow_config = False

        with open(config_file) as f:
            deploy_configs = json.load(f)
        self.deploy_configs = [self.validate(c) for c in deploy_configs
                               if 'append' in c]
        self.mount_configs = [self.validate_mount(c) for c in deploy_configs
                              if 'mount' in c]
        tmp_append = os.path.join(self.container.rootfs,
                                  _APPEND_FOLDER.lstrip(os.path.sep))
        if lxc_utils.path_exists(tmp_append):
            utils.run('sudo rm -rf "%s"' % tmp_append)
        utils.run('sudo mkdir -p "%s"' % tmp_append)


    def _deploy_config_pre_start(self, deploy_config):
        """Deploy a config before container is started.

        Most configs can be deployed before the container is up. For configs
        require a reboot to take effective, they must be deployed in this
        function.

        @param deploy_config: Config to be deployed.
        """
        if not lxc_utils.path_exists(deploy_config.source):
            return
        # Path to the target file relative to host.
        if deploy_config.append:
            target = os.path.join(_APPEND_FOLDER,
                                  os.path.basename(deploy_config.target))
        else:
            target = deploy_config.target

        self.container.copy(deploy_config.source, target)


    def _deploy_config_post_start(self, deploy_config):
        """Deploy a config after container is started.

        For configs to be appended after the existing config files in container,
        they must be copied to a temp location before container is up (deployed
        in function _deploy_config_pre_start). After the container is up, calls
        can be made to append the content of such configs to existing config
        files.

        @param deploy_config: Config to be deployed.

        """
        if deploy_config.append:
            source = os.path.join(_APPEND_FOLDER,
                                  os.path.basename(deploy_config.target))
            self.container.attach_run('cat \'%s\' >> \'%s\'' %
                                      (source, deploy_config.target))
        self.container.attach_run(
                'chmod -R %s \'%s\'' %
                (deploy_config.permission, deploy_config.target))


    def _modify_shadow_config(self):
        """Update the shadow config used in container with correct values.

        This only applies when no shadow SSP deploy config is applied. For
        default SSP deploy config, autotest shadow_config.ini is from autotest
        directory, which requires following modification to be able to work in
        container. If one chooses to use a shadow SSP deploy config file, the
        autotest shadow_config.ini must be from a source with following
        modification:
        1. Disable master ssh connection in shadow config, as it is not working
           properly in container yet, and produces noise in the log.
        2. Update AUTOTEST_WEB/host and SERVER/hostname to be the IP of the host
           if any is set to localhost or 127.0.0.1. Otherwise, set it to be the
           FQDN of the config value.
        3. Update SSP/user, which is used as the user makes RPC inside the
           container. This allows the RPC to pass ACL check as if the call is
           made in the host.

        """
        shadow_config = os.path.join(constants.CONTAINER_AUTOTEST_DIR,
                                     'shadow_config.ini')

        # Inject "AUTOSERV/enable_master_ssh: False" in shadow config as
        # container does not support master ssh connection yet.
        self.container.attach_run(
                'echo $\'\n[AUTOSERV]\nenable_master_ssh: False\n\' >> %s' %
                shadow_config)

        host_ip = lxc_utils.get_host_ip()
        local_names = ['localhost', '127.0.0.1']

        db_host = config.get_config_value('AUTOTEST_WEB', 'host')
        if db_host.lower() in local_names:
            new_host = host_ip
        else:
            new_host = socket.getfqdn(db_host)
        self.container.attach_run('echo $\'\n[AUTOTEST_WEB]\nhost: %s\n\' >> %s'
                                  % (new_host, shadow_config))

        afe_host = config.get_config_value('SERVER', 'hostname')
        if afe_host.lower() in local_names:
            new_host = host_ip
        else:
            new_host = socket.getfqdn(afe_host)
        self.container.attach_run('echo $\'\n[SERVER]\nhostname: %s\n\' >> %s' %
                                  (new_host, shadow_config))

        # Update configurations in SSP section:
        # user: The user running current process.
        # is_moblab: True if the autotest server is a Moblab instance.
        # host_container_ip: IP address of the lxcbr0 interface. Process running
        #     inside container can make RPC through this IP.
        self.container.attach_run(
                'echo $\'\n[SSP]\nuser: %s\nis_moblab: %s\n'
                'host_container_ip: %s\n\' >> %s' %
                (getpass.getuser(), bool(utils.is_moblab()),
                 lxc_utils.get_host_ip(), shadow_config))


    def _modify_ssh_config(self):
        """Modify ssh config for it to work inside container.

        This is only called when default ssp_deploy_config is used. If shadow
        deploy config is manually set up, this function will not be called.
        Therefore, the source of ssh config must be properly updated to be able
        to work inside container.

        """
        # Remove domain specific flags.
        ssh_config = '/root/.ssh/config'
        self.container.attach_run('sed -i \'s/UseProxyIf=false//g\' \'%s\'' %
                                  ssh_config)
        # TODO(dshi): crbug.com/451622 ssh connection loglevel is set to
        # ERROR in container before master ssh connection works. This is
        # to avoid logs being flooded with warning `Permanently added
        # '[hostname]' (RSA) to the list of known hosts.` (crbug.com/478364)
        # The sed command injects following at the beginning of .ssh/config
        # used in config. With such change, ssh command will not post
        # warnings.
        # Host *
        #   LogLevel Error
        self.container.attach_run(
                'sed -i \'1s/^/Host *\\n  LogLevel ERROR\\n\\n/\' \'%s\'' %
                ssh_config)

        # Inject ssh config for moblab to ssh to dut from container.
        if utils.is_moblab():
            # ssh to moblab itself using moblab user.
            self.container.attach_run(
                    'echo $\'\nHost 192.168.231.1\n  User moblab\n  '
                    'IdentityFile %%d/.ssh/testing_rsa\' >> %s' %
                    '/root/.ssh/config')
            # ssh to duts using root user.
            self.container.attach_run(
                    'echo $\'\nHost *\n  User root\n  '
                    'IdentityFile %%d/.ssh/testing_rsa\' >> %s' %
                    '/root/.ssh/config')


    def deploy_pre_start(self):
        """Deploy configs before the container is started.
        """
        for deploy_config in self.deploy_configs:
            self._deploy_config_pre_start(deploy_config)
        for mount_config in self.mount_configs:
            if (mount_config.force_create and
                not os.path.exists(mount_config.source)):
                utils.run('mkdir -p %s' % mount_config.source)
            self.container.mount_dir(mount_config.source,
                                     mount_config.target,
                                     mount_config.readonly)


    def deploy_post_start(self):
        """Deploy configs after the container is started.
        """
        for deploy_config in self.deploy_configs:
            self._deploy_config_post_start(deploy_config)
        # Autotest shadow config requires special handling to update hostname
        # of `localhost` with host IP. Shards always use `localhost` as value
        # of SERVER\hostname and AUTOTEST_WEB\host.
        self._modify_shadow_config()
        # Only apply special treatment for files deployed by the default
        # ssp_deploy_config
        if not self.is_shadow_config:
            self._modify_ssh_config()
