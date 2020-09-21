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

"""Tests for acloud.public.config."""

import mock

import unittest
from acloud.internal.proto import internal_config_pb2
from acloud.internal.proto import user_config_pb2
from acloud.public import config
from acloud.public import errors


class AcloudConfigManagerTest(unittest.TestCase):
    """Test acloud configuration manager."""

    USER_CONFIG = """
service_account_name: "fake@developer.gserviceaccount.com"
service_account_private_key_path: "/path/to/service/account/key"
project: "fake-project"
zone: "us-central1-f"
machine_type: "n1-standard-1"
network: "default"
ssh_private_key_path: "/path/to/ssh/key"
storage_bucket_name: "fake_bucket"
orientation: "portrait"
resolution: "1200x1200x1200x1200"
client_id: "fake_client_id"
client_secret: "fake_client_secret"
metadata_variable {
    key: "metadata_1"
    value: "metadata_value_1"
}
"""

    INTERNAL_CONFIG = """
min_machine_size: "n1-standard-1"
disk_image_name: "avd-system.tar.gz"
disk_image_mime_type: "application/x-tar"
disk_image_extension: ".tar.gz"
disk_raw_image_name: "disk.raw"
disk_raw_image_extension: ".img"
creds_cache_file: ".fake_oauth2.dat"
user_agent: "fake_user_agent"

default_usr_cfg {
    machine_type: "n1-standard-1"
    network: "default"
    metadata_variable {
        key: "metadata_1"
        value: "metadata_value_1"
    }

    metadata_variable {
        key: "metadata_2"
        value: "metadata_value_2"
    }
}

device_resolution_map {
    key: "nexus5"
    value: "1080x1920x32x480"
}

device_default_orientation_map {
    key: "nexus5"
    value: "portrait"
}

valid_branch_and_min_build_id {
    key: "git_jb-gce-dev"
    value: 0
}
"""

    def setUp(self):
        self.config_file = mock.MagicMock()

    def testLoadUserConfig(self):
        """Test loading user config."""
        self.config_file.read.return_value = self.USER_CONFIG
        cfg = config.AcloudConfigManager.LoadConfigFromProtocolBuffer(
            self.config_file, user_config_pb2.UserConfig)
        self.assertEquals(cfg.service_account_name,
                          "fake@developer.gserviceaccount.com")
        self.assertEquals(cfg.service_account_private_key_path,
                          "/path/to/service/account/key")
        self.assertEquals(cfg.project, "fake-project")
        self.assertEquals(cfg.zone, "us-central1-f")
        self.assertEquals(cfg.machine_type, "n1-standard-1")
        self.assertEquals(cfg.network, "default")
        self.assertEquals(cfg.ssh_private_key_path, "/path/to/ssh/key")
        self.assertEquals(cfg.storage_bucket_name, "fake_bucket")
        self.assertEquals(cfg.orientation, "portrait")
        self.assertEquals(cfg.resolution, "1200x1200x1200x1200")
        self.assertEquals(cfg.client_id, "fake_client_id")
        self.assertEquals(cfg.client_secret, "fake_client_secret")
        self.assertEquals({key: val
                           for key, val in cfg.metadata_variable.iteritems()},
                          {"metadata_1": "metadata_value_1"})

    def testLoadInternalConfig(self):
        """Test loading internal config."""
        self.config_file.read.return_value = self.INTERNAL_CONFIG
        cfg = config.AcloudConfigManager.LoadConfigFromProtocolBuffer(
            self.config_file, internal_config_pb2.InternalConfig)
        self.assertEquals(cfg.min_machine_size, "n1-standard-1")
        self.assertEquals(cfg.disk_image_name, "avd-system.tar.gz")
        self.assertEquals(cfg.disk_image_mime_type, "application/x-tar")
        self.assertEquals(cfg.disk_image_extension, ".tar.gz")
        self.assertEquals(cfg.disk_raw_image_name, "disk.raw")
        self.assertEquals(cfg.disk_raw_image_extension, ".img")
        self.assertEquals(cfg.creds_cache_file, ".fake_oauth2.dat")
        self.assertEquals(cfg.user_agent, "fake_user_agent")
        self.assertEquals(cfg.default_usr_cfg.machine_type, "n1-standard-1")
        self.assertEquals(cfg.default_usr_cfg.network, "default")
        self.assertEquals({
            key: val
            for key, val in cfg.default_usr_cfg.metadata_variable.iteritems()
        }, {"metadata_1": "metadata_value_1",
            "metadata_2": "metadata_value_2"})
        self.assertEquals(
            {key: val
             for key, val in cfg.device_resolution_map.iteritems()},
            {"nexus5": "1080x1920x32x480"})
        device_resolution = {
            key: val
            for key, val in cfg.device_default_orientation_map.iteritems()
        }
        self.assertEquals(device_resolution, {"nexus5": "portrait"})
        valid_branch_and_min_build_id = {
            key: val
            for key, val in cfg.valid_branch_and_min_build_id.iteritems()
        }
        self.assertEquals(valid_branch_and_min_build_id, {"git_jb-gce-dev": 0})

    def testLoadConfigFails(self):
        """Test loading a bad file."""
        self.config_file.read.return_value = "malformed text"
        with self.assertRaises(errors.ConfigError):
            config.AcloudConfigManager.LoadConfigFromProtocolBuffer(
                self.config_file, internal_config_pb2.InternalConfig)


if __name__ == "__main__":
    unittest.main()
