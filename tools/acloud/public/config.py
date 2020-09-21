#!/usr/bin/env python
#
# Copyright 2016 - The Android Open Source Project
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

"""Config manager.

Three protobuf messages are defined in
   driver/internal/config/proto/internal_config.proto
   driver/internal/config/proto/user_config.proto

Internal config file     User config file
      |                         |
      v                         v
  InternalConfig           UserConfig
  (proto message)        (proto message)
        |                     |
        |                     |
        |->   AcloudConfig  <-|

At runtime, AcloudConfigManager performs the following steps.
- Load driver config file into a InternalConfig message instance.
- Load user config file into a UserConfig message instance.
- Create AcloudConfig using InternalConfig and UserConfig.

TODO(fdeng):
  1. Add support for override configs with command line args.
  2. Scan all configs to find the right config for given branch and build_id.
     Raise an error if the given build_id is smaller than min_build_id
     only applies to release build id.
     Raise an error if the branch is not supported.

"""

import logging
import os

from acloud.internal.proto import internal_config_pb2
from acloud.internal.proto import user_config_pb2
from acloud.public import errors
from google.protobuf import text_format
_CONFIG_DATA_PATH = os.path.join(
    os.path.dirname(os.path.abspath(__file__)), "data")

logger = logging.getLogger(__name__)


class AcloudConfig(object):
    """A class that holds all configurations for acloud."""

    REQUIRED_FIELD = [
        "project", "zone", "machine_type", "network", "storage_bucket_name",
        "min_machine_size", "disk_image_name", "disk_image_mime_type"
    ]

    def __init__(self, usr_cfg, internal_cfg):
        """Initialize.

        Args:
            usr_cfg: A protobuf object that holds the user configurations.
            internal_cfg: A protobuf object that holds internal configurations.
        """
        self.service_account_name = usr_cfg.service_account_name
        self.service_account_private_key_path = (
            usr_cfg.service_account_private_key_path)
        self.creds_cache_file = internal_cfg.creds_cache_file
        self.user_agent = internal_cfg.user_agent
        self.client_id = usr_cfg.client_id
        self.client_secret = usr_cfg.client_secret

        self.project = usr_cfg.project
        self.zone = usr_cfg.zone
        self.machine_type = (usr_cfg.machine_type or
                             internal_cfg.default_usr_cfg.machine_type)
        self.network = (usr_cfg.network or
                        internal_cfg.default_usr_cfg.network)
        self.ssh_private_key_path = usr_cfg.ssh_private_key_path
        self.ssh_public_key_path = usr_cfg.ssh_public_key_path
        self.storage_bucket_name = usr_cfg.storage_bucket_name
        self.metadata_variable = {
            key: val
            for key, val in
            internal_cfg.default_usr_cfg.metadata_variable.iteritems()
        }
        self.metadata_variable.update(usr_cfg.metadata_variable)

        self.device_resolution_map = {
            device: resolution
            for device, resolution in
            internal_cfg.device_resolution_map.iteritems()
        }
        self.device_default_orientation_map = {
            device: orientation
            for device, orientation in
            internal_cfg.device_default_orientation_map.iteritems()
        }
        self.no_project_access_msg_map = {
            project: msg for project, msg
            in internal_cfg.no_project_access_msg_map.iteritems()}
        self.min_machine_size = internal_cfg.min_machine_size
        self.disk_image_name = internal_cfg.disk_image_name
        self.disk_image_mime_type = internal_cfg.disk_image_mime_type
        self.disk_image_extension = internal_cfg.disk_image_extension
        self.disk_raw_image_name = internal_cfg.disk_raw_image_name
        self.disk_raw_image_extension = internal_cfg.disk_raw_image_extension
        self.valid_branch_and_min_build_id = {
            branch: min_build_id
            for branch, min_build_id in
            internal_cfg.valid_branch_and_min_build_id.iteritems()
        }
        self.precreated_data_image_map = {
            size_gb: image_name
            for size_gb, image_name in
            internal_cfg.precreated_data_image.iteritems()
        }
        self.extra_data_disk_size_gb = (
            usr_cfg.extra_data_disk_size_gb or
            internal_cfg.default_usr_cfg.extra_data_disk_size_gb)
        if self.extra_data_disk_size_gb > 0:
            if "cfg_sta_persistent_data_device" not in usr_cfg.metadata_variable:
                # If user did not set it explicity, use default.
                self.metadata_variable["cfg_sta_persistent_data_device"] = (
                    internal_cfg.default_extra_data_disk_device)
            if "cfg_sta_ephemeral_data_size_mb" in usr_cfg.metadata_variable:
                raise errors.ConfigError(
                    "The following settings can't be set at the same time: "
                    "extra_data_disk_size_gb and"
                    "metadata variable cfg_sta_ephemeral_data_size_mb.")
            if "cfg_sta_ephemeral_data_size_mb" in self.metadata_variable:
                del self.metadata_variable["cfg_sta_ephemeral_data_size_mb"]

        # Fields that can be overriden by args
        self.orientation = usr_cfg.orientation
        self.resolution = usr_cfg.resolution

        # Verify validity of configurations.
        self.Verify()

    def OverrideWithArgs(self, parsed_args):
        """Override configuration values with args passed in from cmd line.

        Args:
            parsed_args: Args parsed from command line.
        """
        if parsed_args.which == "create" and parsed_args.spec:
            if not self.resolution:
                self.resolution = self.device_resolution_map.get(
                    parsed_args.spec, "")
            if not self.orientation:
                self.orientation = self.device_default_orientation_map.get(
                    parsed_args.spec, "")
        if parsed_args.email:
            self.service_account_name = parsed_args.email

    def Verify(self):
        """Verify configuration fields."""
        missing = [f for f in self.REQUIRED_FIELD if not getattr(self, f)]
        if missing:
            raise errors.ConfigError(
                "Missing required configuration fields: %s" % missing)
        if (self.extra_data_disk_size_gb and self.extra_data_disk_size_gb
                not in self.precreated_data_image_map):
            raise errors.ConfigError(
                "Supported extra_data_disk_size_gb options(gb): %s, "
                "invalid value: %d" % (self.precreated_data_image_map.keys(),
                                       self.extra_data_disk_size_gb))


class AcloudConfigManager(object):
    """A class that loads configurations."""

    _DEFAULT_INTERNAL_CONFIG_PATH = os.path.join(_CONFIG_DATA_PATH,
                                                 "default.config")

    def __init__(self,
                 user_config_path,
                 internal_config_path=_DEFAULT_INTERNAL_CONFIG_PATH):
        """Initialize.

        Args:
            user_config_path: path to the user config.
            internal_config_path: path to the internal conifg.
        """
        self._user_config_path = user_config_path
        self._internal_config_path = internal_config_path

    def Load(self):
        """Load the configurations."""
        internal_cfg = None
        usr_cfg = None
        try:
            with open(self._internal_config_path) as config_file:
                internal_cfg = self.LoadConfigFromProtocolBuffer(
                    config_file, internal_config_pb2.InternalConfig)

            with open(self._user_config_path, "r") as config_file:
                usr_cfg = self.LoadConfigFromProtocolBuffer(
                    config_file, user_config_pb2.UserConfig)
        except OSError as e:
            raise errors.ConfigError("Could not load config files: %s" %
                                     str(e))
        return AcloudConfig(usr_cfg, internal_cfg)

    @staticmethod
    def LoadConfigFromProtocolBuffer(config_file, message_type):
        """Load config from a text-based protocol buffer file.

        Args:
            config_file: A python File object.
            message_type: A proto message class.

        Returns:
            An instance of type "message_type" populated with data
            from the file.
        """
        try:
            config = message_type()
            text_format.Merge(config_file.read(), config)
            return config
        except text_format.ParseError as e:
            raise errors.ConfigError("Could not parse config: %s" % str(e))
