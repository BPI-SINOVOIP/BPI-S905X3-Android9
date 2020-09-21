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

"""Tests for acloud.public.device_driver."""

import datetime
import uuid

import dateutil.parser
import mock

import unittest
from acloud.internal.lib import auth
from acloud.internal.lib import android_build_client
from acloud.internal.lib import android_compute_client
from acloud.internal.lib import driver_test_lib
from acloud.internal.lib import gstorage_client
from acloud.public import device_driver


class DeviceDriverTest(driver_test_lib.BaseDriverTest):
    """Test device_driver."""

    def setUp(self):
        """Set up the test."""
        super(DeviceDriverTest, self).setUp()
        self.build_client = mock.MagicMock()
        self.Patch(android_build_client, "AndroidBuildClient",
            return_value=self.build_client)
        self.storage_client = mock.MagicMock()
        self.Patch(
            gstorage_client, "StorageClient", return_value=self.storage_client)
        self.compute_client = mock.MagicMock()
        self.Patch(
            android_compute_client,
            "AndroidComputeClient",
            return_value=self.compute_client)
        self.Patch(auth, "CreateCredentials", return_value=mock.MagicMock())

    def _CreateCfg(self):
        """A helper method that creates a mock configuration object."""
        cfg = mock.MagicMock()
        cfg.service_account_name = "fake@service.com"
        cfg.service_account_private_key_path = "/fake/path/to/key"
        cfg.zone = "fake_zone"
        cfg.disk_image_name = "fake_image.tar.gz"
        cfg.disk_image_mime_type = "fake/type"
        cfg.storage_bucket_name = "fake_bucket"
        cfg.extra_data_disk_size_gb = 4
        cfg.precreated_data_image_map = {
            4: "extradisk-image-4gb",
            10: "extradisk-image-10gb"
        }
        cfg.ssh_private_key_path = ""
        cfg.ssh_public_key_path = ""

        return cfg

    def testCreateAndroidVirtualDevices(self):
        """Test CreateAndroidVirtualDevices."""
        cfg = self._CreateCfg()
        fake_gs_url = "fake_gs_url"
        fake_ip = "140.1.1.1"
        fake_instance = "fake-instance"
        fake_image = "fake-image"
        fake_build_target = "fake_target"
        fake_build_id = "12345"

        # Mock uuid
        fake_uuid = mock.MagicMock(hex="1234")
        self.Patch(uuid, "uuid4", return_value=fake_uuid)
        fake_gs_object = fake_uuid.hex + "-" + cfg.disk_image_name
        self.storage_client.GetUrl.return_value = fake_gs_url

        # Mock compute client methods
        disk_name = "extradisk-image-4gb"
        self.compute_client.GetInstanceIP.return_value = fake_ip
        self.compute_client.GenerateImageName.return_value = fake_image
        self.compute_client.GenerateInstanceName.return_value = fake_instance
        self.compute_client.GetDataDiskName.return_value = disk_name

        # Verify
        r = device_driver.CreateAndroidVirtualDevices(
        cfg, fake_build_target, fake_build_id)
        self.build_client.CopyTo.assert_called_with(
        fake_build_target, fake_build_id, artifact_name=cfg.disk_image_name,
        destination_bucket=cfg.storage_bucket_name,
        destination_path=fake_gs_object)
        self.compute_client.CreateImage.assert_called_with(
        image_name=fake_image, source_uri=fake_gs_url)
        self.compute_client.CreateInstance.assert_called_with(
        fake_instance, fake_image, disk_name)
        self.compute_client.DeleteImage.assert_called_with(fake_image)
        self.storage_client.Delete(cfg.storage_bucket_name, fake_gs_object)

        self.assertEquals(
            r.data,
            {
                "devices": [
                    {
                        "instance_name": fake_instance,
                        "ip": fake_ip,
                    },
                ],
            }
        )
        self.assertEquals(r.command, "create")
        self.assertEquals(r.status, "SUCCESS")


    def testDeleteAndroidVirtualDevices(self):
        """Test DeleteAndroidVirtualDevices."""
        instance_names = ["fake-instance-1", "fake-instance-2"]
        self.compute_client.DeleteInstances.return_value = (instance_names, [],
                                                            [])
        cfg = self._CreateCfg()
        r = device_driver.DeleteAndroidVirtualDevices(cfg, instance_names)
        self.compute_client.DeleteInstances.assert_called_once_with(
            instance_names, cfg.zone)
        self.assertEquals(r.data, {
            "deleted": [
                {
                    "name": instance_names[0],
                    "type": "instance",
                },
                {
                    "name": instance_names[1],
                    "type": "instance",
                },
            ],
        })
        self.assertEquals(r.command, "delete")
        self.assertEquals(r.status, "SUCCESS")

    def testCleanup(self):
        expiration_mins = 30
        before_deadline = "2015-10-29T12:00:30.018-07:00"
        after_deadline = "2015-10-29T12:45:30.018-07:00"
        now = "2015-10-29T13:00:30.018-07:00"
        self.Patch(device_driver, "datetime")
        device_driver.datetime.datetime.now.return_value = dateutil.parser.parse(
            now)
        device_driver.datetime.timedelta.return_value = datetime.timedelta(
            minutes=expiration_mins)
        fake_instances = [
            {
                "name": "fake_instance_1",
                "creationTimestamp": before_deadline,
            }, {
                "name": "fake_instance_2",
                "creationTimestamp": after_deadline,
            }
        ]
        fake_images = [
            {
                "name": "extradisk-image-4gb",
                "creationTimestamp": before_deadline,
            }, {
                "name": "fake_image_1",
                "creationTimestamp": before_deadline,
            }, {
                "name": "fake_image_2",
                "creationTimestamp": after_deadline,
            }
        ]
        fake_disks = [
            {
                "name": "fake_disk_1",
                "creationTimestamp": before_deadline,
            }, {
                "name": "fake_disk_2",
                "creationTimestamp": before_deadline,
                "users": ["some-instance-using-the-disk"]
            }, {
                "name": "fake_disk_3",
                "creationTimestamp": after_deadline,
            }
        ]
        fake_objects = [
            {
                "name": "fake_object_1",
                "timeCreated": before_deadline,
            }, {
                "name": "fake_object_2",
                "timeCreated": after_deadline,
            }
        ]
        self.compute_client.ListInstances.return_value = fake_instances
        self.compute_client.ListImages.return_value = fake_images
        self.compute_client.ListDisks.return_value = fake_disks
        self.storage_client.List.return_value = fake_objects
        self.compute_client.DeleteInstances.return_value = (
            ["fake_instance_1"], [], [])
        self.compute_client.DeleteImages.return_value = (["fake_image_1"], [],
                                                         [])
        self.compute_client.DeleteDisks.return_value = (["fake_disk_1"], [],
                                                        [])
        self.storage_client.DeleteFiles.return_value = (["fake_object_1"], [],
                                                        [])
        cfg = self._CreateCfg()
        r = device_driver.Cleanup(cfg, expiration_mins)
        self.assertEqual(r.errors, [])
        expected_report_data = {
            "deleted": [
                {"name": "fake_instance_1",
                 "type": "instance"},
                {"name": "fake_image_1",
                 "type": "image"},
                {"name": "fake_disk_1",
                 "type": "disk"},
                {"name": "fake_object_1",
                 "type": "cached_build_artifact"},
            ]
        }
        self.assertEqual(r.data, expected_report_data)

        self.compute_client.ListInstances.assert_called_once_with(
            zone=cfg.zone)
        self.compute_client.DeleteInstances.assert_called_once_with(
            instances=["fake_instance_1"], zone=cfg.zone)

        self.compute_client.ListImages.assert_called_once_with()
        self.compute_client.DeleteImages.assert_called_once_with(
            image_names=["fake_image_1"])

        self.compute_client.ListDisks.assert_called_once_with(zone=cfg.zone)
        self.compute_client.DeleteDisks.assert_called_once_with(
            disk_names=["fake_disk_1"], zone=cfg.zone)

        self.storage_client.List.assert_called_once_with(
            bucket_name=cfg.storage_bucket_name)
        self.storage_client.DeleteFiles.assert_called_once_with(
            bucket_name=cfg.storage_bucket_name,
            object_names=["fake_object_1"])


if __name__ == "__main__":
    unittest.main()
