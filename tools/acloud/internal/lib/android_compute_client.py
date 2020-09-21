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

"""A client that manages Android compute engine instances.

** AndroidComputeClient **

AndroidComputeClient derives from ComputeClient. It manges a google
compute engine project that is setup for running Android instances.
It knows how to create android GCE images and instances.

** Class hierarchy **

  base_cloud_client.BaseCloudApiClient
                ^
                |
       gcompute_client.ComputeClient
                ^
                |
    gcompute_client.AndroidComputeClient

TODO(fdeng):
  Merge caci/framework/gce_manager.py
  with this module, update callers of gce_manager.py to use this module.
"""

import getpass
import logging
import os
import uuid

from acloud.internal.lib import gcompute_client
from acloud.internal.lib import utils
from acloud.public import errors

logger = logging.getLogger(__name__)


class AndroidComputeClient(gcompute_client.ComputeClient):
    """Client that manages Anadroid Virtual Device."""

    INSTANCE_NAME_FMT = "ins-{uuid}-{build_id}-{build_target}"
    IMAGE_NAME_FMT = "img-{uuid}-{build_id}-{build_target}"
    DATA_DISK_NAME_FMT = "data-{instance}"
    BOOT_COMPLETED_MSG = "VIRTUAL_DEVICE_BOOT_COMPLETED"
    BOOT_TIMEOUT_SECS = 5 * 60  # 5 mins, usually it should take ~2 mins
    BOOT_CHECK_INTERVAL_SECS = 10
    NAME_LENGTH_LIMIT = 63
    # If the generated name ends with '-', replace it with REPLACER.
    REPLACER = "e"

    def __init__(self, acloud_config, oauth2_credentials):
        """Initialize.

        Args:
            acloud_config: An AcloudConfig object.
            oauth2_credentials: An oauth2client.OAuth2Credentials instance.
        """
        super(AndroidComputeClient, self).__init__(acloud_config,
                                                   oauth2_credentials)
        self._zone = acloud_config.zone
        self._machine_type = acloud_config.machine_type
        self._min_machine_size = acloud_config.min_machine_size
        self._network = acloud_config.network
        self._orientation = acloud_config.orientation
        self._resolution = acloud_config.resolution
        self._metadata = acloud_config.metadata_variable.copy()
        self._ssh_public_key_path = acloud_config.ssh_public_key_path

    @classmethod
    def _FormalizeName(cls, name):
        """Formalize the name to comply with RFC1035.

        The name must be 1-63 characters long and match the regular expression
        [a-z]([-a-z0-9]*[a-z0-9])? which means the first character must be a
        lowercase letter, and all following characters must be a dash,
        lowercase letter, or digit, except the last character, which cannot be
        a dash.

        Args:
          name: A string.

        Returns:
          name: A string that complies with RFC1035.
        """
        name = name.replace("_", "-").lower()
        name = name[:cls.NAME_LENGTH_LIMIT]
        if name[-1] == "-":
          name = name[:-1] + cls.REPLACER
        return name

    def _CheckMachineSize(self):
        """Check machine size.

        Check if the desired machine type |self._machine_type| meets
        the requirement of minimum machine size specified as
        |self._min_machine_size|.

        Raises:
            errors.DriverError: if check fails.
        """
        if self.CompareMachineSize(self._machine_type, self._min_machine_size,
                                   self._zone) < 0:
            raise errors.DriverError(
                "%s does not meet the minimum required machine size %s" %
                (self._machine_type, self._min_machine_size))

    @classmethod
    def GenerateImageName(cls, build_target=None, build_id=None):
        """Generate an image name given build_target, build_id.

        Args:
            build_target: Target name, e.g. "gce_x86-userdebug"
            build_id: Build id, a string, e.g. "2263051", "P2804227"

        Returns:
            A string, representing image name.
        """
        if not build_target and not build_id:
            return "image-" + uuid.uuid4().hex
        name = cls.IMAGE_NAME_FMT.format(build_target=build_target,
                                         build_id=build_id,
                                         uuid=uuid.uuid4().hex[:8])
        return cls._FormalizeName(name)

    @classmethod
    def GetDataDiskName(cls, instance):
        """Get data disk name for an instance.

        Args:
            instance: An instance_name.

        Returns:
            The corresponding data disk name.
        """
        name = cls.DATA_DISK_NAME_FMT.format(instance=instance)
        return cls._FormalizeName(name)

    @classmethod
    def GenerateInstanceName(cls, build_target=None, build_id=None):
        """Generate an instance name given build_target, build_id.

        Target is not used as instance name has a length limit.

        Args:
            build_target: Target name, e.g. "gce_x86-userdebug"
            build_id: Build id, a string, e.g. "2263051", "P2804227"

        Returns:
            A string, representing instance name.
        """
        if not build_target and not build_id:
            return "instance-" + uuid.uuid4().hex
        name = cls.INSTANCE_NAME_FMT.format(
            build_target=build_target,
            build_id=build_id,
            uuid=uuid.uuid4().hex[:8]).replace("_", "-").lower()
        return cls._FormalizeName(name)

    def CreateDisk(self, disk_name, source_image, size_gb):
        """Create a gce disk.

        Args:
            disk_name: A string.
            source_image: A string, name to the image name.
            size_gb: Integer, size in gigabytes.
        """
        if self.CheckDiskExists(disk_name, self._zone):
            raise errors.DriverError(
                "Failed to create disk %s, already exists." % disk_name)
        if source_image and not self.CheckImageExists(source_image):
            raise errors.DriverError(
                "Failed to create disk %s, source image %s does not exist." %
                (disk_name, source_image))
        super(AndroidComputeClient, self).CreateDisk(disk_name,
                                                     source_image=source_image,
                                                     size_gb=size_gb,
                                                     zone=self._zone)

    def CreateImage(self, image_name, source_uri):
        """Create a gce image.

        Args:
            image_name: String, name of the image.
            source_uri: A full Google Storage URL to the disk image.
                        e.g. "https://storage.googleapis.com/my-bucket/
                              avd-system-2243663.tar.gz"
        """
        if not self.CheckImageExists(image_name):
            super(AndroidComputeClient, self).CreateImage(image_name,
                                                          source_uri)

    def _GetExtraDiskArgs(self, extra_disk_name):
        """Get extra disk arg for given disk.

        Args:
            extra_disk_name: Name of the disk.

        Returns:
            A dictionary of disk args.
        """
        return [{
            "type": "PERSISTENT",
            "mode": "READ_WRITE",
            "source": "projects/%s/zones/%s/disks/%s" % (
                self._project, self._zone, extra_disk_name),
            "autoDelete": True,
            "boot": False,
            "interface": "SCSI",
            "deviceName": extra_disk_name,
        }]

    @staticmethod
    def _LoadSshPublicKey(ssh_public_key_path):
        """Load the content of ssh public key from a file.

        Args:
            ssh_public_key_path: String, path to the public key file.
                               E.g. ~/.ssh/acloud_rsa.pub
        Returns:
            String, content of the file.

        Raises:
            errors.DriverError if the public key file does not exist
            or the content is not valid.
        """
        key_path = os.path.expanduser(ssh_public_key_path)
        if not os.path.exists(key_path):
            raise errors.DriverError(
                "SSH public key file %s does not exist." % key_path)

        with open(key_path) as f:
            rsa = f.read()
            rsa = rsa.strip() if rsa else rsa
            utils.VerifyRsaPubKey(rsa)
        return rsa

    def CreateInstance(self, instance, image_name, extra_disk_name=None):
        """Create a gce instance given an gce image.

        Args:
            instance: instance name.
            image_name: A string, the name of the GCE image.
        """
        self._CheckMachineSize()
        disk_args = self._GetDiskArgs(instance, image_name)
        if extra_disk_name:
            disk_args.extend(self._GetExtraDiskArgs(extra_disk_name))
        metadata = self._metadata.copy()
        metadata["cfg_sta_display_resolution"] = self._resolution
        metadata["t_force_orientation"] = self._orientation

        # Add per-instance ssh key
        if self._ssh_public_key_path:
            rsa = self._LoadSshPublicKey(self._ssh_public_key_path)
            logger.info("ssh_public_key_path is specified in config: %s, "
                        "will add the key to the instance.",
                        self._ssh_public_key_path)
            metadata["sshKeys"] = "%s:%s" % (getpass.getuser(), rsa)
        else:
            logger.warning(
                "ssh_public_key_path is not specified in config, "
                "only project-wide key will be effective.")

        super(AndroidComputeClient, self).CreateInstance(
            instance, image_name, self._machine_type, metadata, self._network,
            self._zone, disk_args)

    def CheckBoot(self, instance):
        """Check once to see if boot completes.

        Args:
            instance: string, instance name.

        Returns:
            True if the BOOT_COMPLETED_MSG appears in serial port output.
            otherwise False.
        """
        try:
            return self.BOOT_COMPLETED_MSG in self.GetSerialPortOutput(
                instance=instance, port=1)
        except errors.HttpError as e:
            if e.code == 400:
                logger.debug("CheckBoot: Instance is not ready yet %s",
                              str(e))
                return False
            raise

    def WaitForBoot(self, instance):
        """Wait for boot to completes or hit timeout.

        Args:
            instance: string, instance name.
        """
        logger.info("Waiting for instance to boot up: %s", instance)
        timeout_exception = errors.DeviceBootTimeoutError(
            "Device %s did not finish on boot within timeout (%s secs)" %
            (instance, self.BOOT_TIMEOUT_SECS)),
        utils.PollAndWait(func=self.CheckBoot,
                          expected_return=True,
                          timeout_exception=timeout_exception,
                          timeout_secs=self.BOOT_TIMEOUT_SECS,
                          sleep_interval_secs=self.BOOT_CHECK_INTERVAL_SECS,
                          instance=instance)
        logger.info("Instance boot completed: %s", instance)

    def GetInstanceIP(self, instance):
        """Get Instance IP given instance name.

        Args:
            instance: String, representing instance name.

        Returns:
            string, IP of the instance.
        """
        return super(AndroidComputeClient, self).GetInstanceIP(instance,
                                                               self._zone)

    def GetSerialPortOutput(self, instance, port=1):
        """Get serial port output.

        Args:
            instance: string, instance name.
            port: int, which COM port to read from, 1-4, default to 1.

        Returns:
            String, contents of the output.

        Raises:
            errors.DriverError: For malformed response.
        """
        return super(AndroidComputeClient, self).GetSerialPortOutput(
            instance, self._zone, port)

    def GetInstanceNamesByIPs(self, ips):
        """Get Instance names by IPs.

        This function will go through all instances, which
        could be slow if there are too many instances.  However, currently
        GCE doesn't support search for instance by IP.

        Args:
            ips: A set of IPs.

        Returns:
            A dictionary where key is ip and value is instance name or None
            if instance is not found for the given IP.
        """
        return super(AndroidComputeClient, self).GetInstanceNamesByIPs(
            ips, self._zone)
