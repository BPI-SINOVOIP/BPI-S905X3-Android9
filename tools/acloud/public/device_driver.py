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

"""Public Device Driver APIs.

This module provides public device driver APIs that can be called
as a Python library.

TODO(fdeng): The following APIs have not been implemented
  - RebootAVD(ip):
  - RegisterSshPubKey(username, key):
  - UnregisterSshPubKey(username, key):
  - CleanupStaleImages():
  - CleanupStaleDevices():
"""

import datetime
import logging
import os

import dateutil.parser
import dateutil.tz

from acloud.public import avd
from acloud.public import errors
from acloud.public import report
from acloud.internal import constants
from acloud.internal.lib import auth
from acloud.internal.lib import android_build_client
from acloud.internal.lib import android_compute_client
from acloud.internal.lib import gstorage_client
from acloud.internal.lib import utils

logger = logging.getLogger(__name__)

ALL_SCOPES = " ".join([android_build_client.AndroidBuildClient.SCOPE,
                       gstorage_client.StorageClient.SCOPE,
                       android_compute_client.AndroidComputeClient.SCOPE])

MAX_BATCH_CLEANUP_COUNT = 100


class AndroidVirtualDevicePool(object):
    """A class that manages a pool of devices."""

    def __init__(self, cfg, devices=None):
        self._devices = devices or []
        self._cfg = cfg
        credentials = auth.CreateCredentials(cfg, ALL_SCOPES)
        self._build_client = android_build_client.AndroidBuildClient(
            credentials)
        self._storage_client = gstorage_client.StorageClient(credentials)
        self._compute_client = android_compute_client.AndroidComputeClient(
            cfg, credentials)

    def _CreateGceImageWithBuildInfo(self, build_target, build_id):
        """Creates a Gce image using build from Launch Control.

        Clone avd-system.tar.gz of a build to a cache storage bucket
        using launch control api. And then create a Gce image.

        Args:
            build_target: Target name, e.g. "gce_x86-userdebug"
            build_id: Build id, a string, e.g. "2263051", "P2804227"

        Returns:
            String, name of the Gce image that has been created.
        """
        logger.info("Creating a new gce image using build: build_id %s, "
                    "build_target %s", build_id, build_target)
        disk_image_id = utils.GenerateUniqueName(
            suffix=self._cfg.disk_image_name)
        self._build_client.CopyTo(
            build_target,
            build_id,
            artifact_name=self._cfg.disk_image_name,
            destination_bucket=self._cfg.storage_bucket_name,
            destination_path=disk_image_id)
        disk_image_url = self._storage_client.GetUrl(
            self._cfg.storage_bucket_name, disk_image_id)
        try:
            image_name = self._compute_client.GenerateImageName(build_target,
                                                                build_id)
            self._compute_client.CreateImage(image_name=image_name,
                                             source_uri=disk_image_url)
        finally:
            self._storage_client.Delete(self._cfg.storage_bucket_name,
                                        disk_image_id)
        return image_name

    def _CreateGceImageWithLocalFile(self, local_disk_image):
        """Create a Gce image with a local image file.

        The local disk image can be either a tar.gz file or a
        raw vmlinux image.
        e.g.  /tmp/avd-system.tar.gz or /tmp/android_system_disk_syslinux.img
        If a raw vmlinux image is provided, it will be archived into a tar.gz file.

        The final tar.gz file will be uploaded to a cache bucket in storage.

        Args:
            local_disk_image: string, path to a local disk image,

        Returns:
            String, name of the Gce image that has been created.

        Raises:
            DriverError: if a file with an unexpected extension is given.
        """
        logger.info("Creating a new gce image from a local file %s",
                    local_disk_image)
        with utils.TempDir() as tempdir:
            if local_disk_image.endswith(self._cfg.disk_raw_image_extension):
                dest_tar_file = os.path.join(tempdir,
                                             self._cfg.disk_image_name)
                utils.MakeTarFile(
                    src_dict={local_disk_image: self._cfg.disk_raw_image_name},
                    dest=dest_tar_file)
                local_disk_image = dest_tar_file
            elif not local_disk_image.endswith(self._cfg.disk_image_extension):
                raise errors.DriverError(
                    "Wrong local_disk_image type, must be a *%s file or *%s file"
                    % (self._cfg.disk_raw_image_extension,
                       self._cfg.disk_image_extension))

            disk_image_id = utils.GenerateUniqueName(
                suffix=self._cfg.disk_image_name)
            self._storage_client.Upload(
                local_src=local_disk_image,
                bucket_name=self._cfg.storage_bucket_name,
                object_name=disk_image_id,
                mime_type=self._cfg.disk_image_mime_type)
        disk_image_url = self._storage_client.GetUrl(
            self._cfg.storage_bucket_name, disk_image_id)
        try:
            image_name = self._compute_client.GenerateImageName()
            self._compute_client.CreateImage(image_name=image_name,
                                             source_uri=disk_image_url)
        finally:
            self._storage_client.Delete(self._cfg.storage_bucket_name,
                                        disk_image_id)
        return image_name

    def CreateDevices(self,
                      num,
                      build_target=None,
                      build_id=None,
                      gce_image=None,
                      local_disk_image=None,
                      cleanup=True,
                      extra_data_disk_size_gb=None,
                      precreated_data_image=None):
        """Creates |num| devices for given build_target and build_id.

        - If gce_image is provided, will use it to create an instance.
        - If local_disk_image is provided, will upload it to a temporary
          caching storage bucket which is defined by user as |storage_bucket_name|
          And then create an gce image with it; and then create an instance.
        - If build_target and build_id are provided, will clone the disk image
          via launch control to the temporary caching storage bucket.
          And then create an gce image with it; and then create an instance.

        Args:
            num: Number of devices to create.
            build_target: Target name, e.g. "gce_x86-userdebug"
            build_id: Build id, a string, e.g. "2263051", "P2804227"
            gce_image: string, if given, will use this image
                       instead of creating a new one.
                       implies cleanup=False.
            local_disk_image: string, path to a local disk image, e.g.
                              /tmp/avd-system.tar.gz
            cleanup: boolean, if True clean up compute engine image after creating
                     the instance.
            extra_data_disk_size_gb: Integer, size of extra disk, or None.
            precreated_data_image: A string, the image to use for the extra disk.

        Raises:
            errors.DriverError: If no source is specified for image creation.
        """
        if gce_image:
            # GCE image is provided, we can directly move to instance creation.
            logger.info("Using existing gce image %s", gce_image)
            image_name = gce_image
            cleanup = False
        elif local_disk_image:
            image_name = self._CreateGceImageWithLocalFile(local_disk_image)
        elif build_target and build_id:
            image_name = self._CreateGceImageWithBuildInfo(build_target,
                                                           build_id)
        else:
            raise errors.DriverError(
                "Invalid image source, must specify one of the following: gce_image, "
                "local_disk_image, or build_target and build id.")

        # Create GCE instances.
        try:
            for _ in range(num):
                instance = self._compute_client.GenerateInstanceName(
                    build_target, build_id)
                extra_disk_name = None
                if extra_data_disk_size_gb > 0:
                    extra_disk_name = self._compute_client.GetDataDiskName(
                        instance)
                    self._compute_client.CreateDisk(extra_disk_name,
                                                    precreated_data_image,
                                                    extra_data_disk_size_gb)
                self._compute_client.CreateInstance(instance, image_name,
                                                    extra_disk_name)
                ip = self._compute_client.GetInstanceIP(instance)
                self.devices.append(avd.AndroidVirtualDevice(
                    ip=ip, instance_name=instance))
        finally:
            if cleanup:
                self._compute_client.DeleteImage(image_name)

    def DeleteDevices(self):
        """Deletes devices.

        Returns:
            A tuple, (deleted, failed, error_msgs)
            deleted: A list of names of instances that have been deleted.
            faild: A list of names of instances that we fail to delete.
            error_msgs: A list of failure messages.
        """
        instance_names = [device.instance_name for device in self._devices]
        return self._compute_client.DeleteInstances(instance_names,
                                                    self._cfg.zone)

    def WaitForBoot(self):
        """Waits for all devices to boot up.

        Returns:
            A dictionary that contains all the failures.
            The key is the name of the instance that fails to boot,
            the value is an errors.DeviceBootTimeoutError object.
        """
        failures = {}
        for device in self._devices:
            try:
                self._compute_client.WaitForBoot(device.instance_name)
            except errors.DeviceBootTimeoutError as e:
                failures[device.instance_name] = e
        return failures

    @property
    def devices(self):
        """Returns a list of devices in the pool.

        Returns:
            A list of devices in the pool.
        """
        return self._devices


def _AddDeletionResultToReport(report_obj, deleted, failed, error_msgs,
                               resource_name):
    """Adds deletion result to a Report object.

    This function will add the following to report.data.
      "deleted": [
         {"name": "resource_name", "type": "resource_name"},
       ],
      "failed": [
         {"name": "resource_name", "type": "resource_name"},
       ],
    This function will append error_msgs to report.errors.

    Args:
        report_obj: A Report object.
        deleted: A list of names of the resources that have been deleted.
        failed: A list of names of the resources that we fail to delete.
        error_msgs: A list of error message strings to be added to the report.
        resource_name: A string, representing the name of the resource.
    """
    for name in deleted:
        report_obj.AddData(key="deleted",
                           value={"name": name,
                                  "type": resource_name})
    for name in failed:
        report_obj.AddData(key="failed",
                           value={"name": name,
                                  "type": resource_name})
    report_obj.AddErrors(error_msgs)
    if failed or error_msgs:
        report_obj.SetStatus(report.Status.FAIL)


def _FetchSerialLogsFromDevices(compute_client, instance_names, output_file,
                                port):
    """Fetch serial logs from a port for a list of devices to a local file.

    Args:
        compute_client: An object of android_compute_client.AndroidComputeClient
        instance_names: A list of instance names.
        output_file: A path to a file ending with "tar.gz"
        port: The number of serial port to read from, 0 for serial output, 1 for
              logcat.
    """
    with utils.TempDir() as tempdir:
        src_dict = {}
        for instance_name in instance_names:
            serial_log = compute_client.GetSerialPortOutput(
                instance=instance_name, port=port)
            file_name = "%s.log" % instance_name
            file_path = os.path.join(tempdir, file_name)
            src_dict[file_path] = file_name
            with open(file_path, "w") as f:
                f.write(serial_log.encode("utf-8"))
        utils.MakeTarFile(src_dict, output_file)


def _CreateSshKeyPairIfNecessary(cfg):
    """Create ssh key pair if necessary.

    Args:
        cfg: An Acloudconfig instance.

    Raises:
        error.DriverError: If it falls into an unexpected condition.
    """
    if not cfg.ssh_public_key_path:
        logger.warning("ssh_public_key_path is not specified in acloud config. "
                       "Project-wide public key will "
                       "be used when creating AVD instances. "
                       "Please ensure you have the correct private half of "
                       "a project-wide public key if you want to ssh into the "
                       "instances after creation.")
    elif cfg.ssh_public_key_path and not cfg.ssh_private_key_path:
        logger.warning("Only ssh_public_key_path is specified in acloud config,"
                       " but ssh_private_key_path is missing. "
                       "Please ensure you have the correct private half "
                       "if you want to ssh into the instances after creation.")
    elif cfg.ssh_public_key_path and cfg.ssh_private_key_path:
        utils.CreateSshKeyPairIfNotExist(
                cfg.ssh_private_key_path, cfg.ssh_public_key_path)
    else:
        # Should never reach here.
        raise errors.DriverError(
                "Unexpected error in _CreateSshKeyPairIfNecessary")


def CreateAndroidVirtualDevices(cfg,
                                build_target=None,
                                build_id=None,
                                num=1,
                                gce_image=None,
                                local_disk_image=None,
                                cleanup=True,
                                serial_log_file=None,
                                logcat_file=None):
    """Creates one or multiple android devices.

    Args:
        cfg: An AcloudConfig instance.
        build_target: Target name, e.g. "gce_x86-userdebug"
        build_id: Build id, a string, e.g. "2263051", "P2804227"
        num: Number of devices to create.
        gce_image: string, if given, will use this gce image
                   instead of creating a new one.
                   implies cleanup=False.
        local_disk_image: string, path to a local disk image, e.g.
                          /tmp/avd-system.tar.gz
        cleanup: boolean, if True clean up compute engine image and
                 disk image in storage after creating the instance.
        serial_log_file: A path to a file where serial output should
                         be saved to.
        logcat_file: A path to a file where logcat logs should be saved.

    Returns:
        A Report instance.
    """
    r = report.Report(command="create")
    credentials = auth.CreateCredentials(cfg, ALL_SCOPES)
    compute_client = android_compute_client.AndroidComputeClient(cfg,
                                                                 credentials)
    try:
        _CreateSshKeyPairIfNecessary(cfg)
        device_pool = AndroidVirtualDevicePool(cfg)
        device_pool.CreateDevices(
            num,
            build_target,
            build_id,
            gce_image,
            local_disk_image,
            cleanup,
            extra_data_disk_size_gb=cfg.extra_data_disk_size_gb,
            precreated_data_image=cfg.precreated_data_image_map.get(
                cfg.extra_data_disk_size_gb))
        failures = device_pool.WaitForBoot()
        # Write result to report.
        for device in device_pool.devices:
            device_dict = {"ip": device.ip,
                           "instance_name": device.instance_name}
            if device.instance_name in failures:
                r.AddData(key="devices_failing_boot", value=device_dict)
                r.AddError(str(failures[device.instance_name]))
            else:
                r.AddData(key="devices", value=device_dict)
        if failures:
            r.SetStatus(report.Status.BOOT_FAIL)
        else:
            r.SetStatus(report.Status.SUCCESS)

        # Dump serial and logcat logs.
        if serial_log_file:
            _FetchSerialLogsFromDevices(
                compute_client,
                instance_names=[d.instance_name for d in device_pool.devices],
                port=constants.DEFAULT_SERIAL_PORT,
                output_file=serial_log_file)
        if logcat_file:
            _FetchSerialLogsFromDevices(
                compute_client,
                instance_names=[d.instance_name for d in device_pool.devices],
                port=constants.LOGCAT_SERIAL_PORT,
                output_file=logcat_file)
    except errors.DriverError as e:
        r.AddError(str(e))
        r.SetStatus(report.Status.FAIL)
    return r


def DeleteAndroidVirtualDevices(cfg, instance_names):
    """Deletes android devices.

    Args:
        cfg: An AcloudConfig instance.
        instance_names: A list of names of the instances to delete.

    Returns:
        A Report instance.
    """
    r = report.Report(command="delete")
    credentials = auth.CreateCredentials(cfg, ALL_SCOPES)
    compute_client = android_compute_client.AndroidComputeClient(cfg,
                                                                 credentials)
    try:
        deleted, failed, error_msgs = compute_client.DeleteInstances(
            instance_names, cfg.zone)
        _AddDeletionResultToReport(
            r, deleted,
            failed, error_msgs,
            resource_name="instance")
        if r.status == report.Status.UNKNOWN:
            r.SetStatus(report.Status.SUCCESS)
    except errors.DriverError as e:
        r.AddError(str(e))
        r.SetStatus(report.Status.FAIL)
    return r


def _FindOldItems(items, cut_time, time_key):
    """Finds items from |items| whose timestamp is earlier than |cut_time|.

    Args:
        items: A list of items. Each item is a dictionary represent
               the properties of the item. It should has a key as noted
               by time_key.
        cut_time: A datetime.datatime object.
        time_key: String, key for the timestamp.

    Returns:
        A list of those from |items| whose timestamp is earlier than cut_time.
    """
    cleanup_list = []
    for item in items:
        t = dateutil.parser.parse(item[time_key])
        if t < cut_time:
            cleanup_list.append(item)
    return cleanup_list


def Cleanup(cfg, expiration_mins):
    """Cleans up stale gce images, gce instances, and disk images in storage.

    Args:
        cfg: An AcloudConfig instance.
        expiration_mins: Integer, resources older than |expiration_mins| will
                         be cleaned up.

    Returns:
        A Report instance.
    """
    r = report.Report(command="cleanup")
    try:
        cut_time = (datetime.datetime.now(dateutil.tz.tzlocal()) -
                    datetime.timedelta(minutes=expiration_mins))
        logger.info(
            "Cleaning up any gce images/instances and cached build artifacts."
            "in google storage that are older than %s", cut_time)
        credentials = auth.CreateCredentials(cfg, ALL_SCOPES)
        compute_client = android_compute_client.AndroidComputeClient(
            cfg, credentials)
        storage_client = gstorage_client.StorageClient(credentials)

        # Cleanup expired instances
        items = compute_client.ListInstances(zone=cfg.zone)
        cleanup_list = [
            item["name"]
            for item in _FindOldItems(items, cut_time, "creationTimestamp")
        ]
        logger.info("Found expired instances: %s", cleanup_list)
        for i in range(0, len(cleanup_list), MAX_BATCH_CLEANUP_COUNT):
            result = compute_client.DeleteInstances(
                instances=cleanup_list[i:i + MAX_BATCH_CLEANUP_COUNT],
                zone=cfg.zone)
            _AddDeletionResultToReport(r, *result, resource_name="instance")

        # Cleanup expired images
        items = compute_client.ListImages()
        skip_list = cfg.precreated_data_image_map.viewvalues()
        cleanup_list = [
            item["name"]
            for item in _FindOldItems(items, cut_time, "creationTimestamp")
            if item["name"] not in skip_list
        ]
        logger.info("Found expired images: %s", cleanup_list)
        for i in range(0, len(cleanup_list), MAX_BATCH_CLEANUP_COUNT):
            result = compute_client.DeleteImages(
                image_names=cleanup_list[i:i + MAX_BATCH_CLEANUP_COUNT])
            _AddDeletionResultToReport(r, *result, resource_name="image")

        # Cleanup expired disks
        # Disks should have been attached to instances with autoDelete=True.
        # However, sometimes disks may not be auto deleted successfully.
        items = compute_client.ListDisks(zone=cfg.zone)
        cleanup_list = [
            item["name"]
            for item in _FindOldItems(items, cut_time, "creationTimestamp")
            if not item.get("users")
        ]
        logger.info("Found expired disks: %s", cleanup_list)
        for i in range(0, len(cleanup_list), MAX_BATCH_CLEANUP_COUNT):
            result = compute_client.DeleteDisks(
                disk_names=cleanup_list[i:i + MAX_BATCH_CLEANUP_COUNT],
                zone=cfg.zone)
            _AddDeletionResultToReport(r, *result, resource_name="disk")

        # Cleanup expired google storage
        items = storage_client.List(bucket_name=cfg.storage_bucket_name)
        cleanup_list = [
            item["name"]
            for item in _FindOldItems(items, cut_time, "timeCreated")
        ]
        logger.info("Found expired cached artifacts: %s", cleanup_list)
        for i in range(0, len(cleanup_list), MAX_BATCH_CLEANUP_COUNT):
            result = storage_client.DeleteFiles(
                bucket_name=cfg.storage_bucket_name,
                object_names=cleanup_list[i:i + MAX_BATCH_CLEANUP_COUNT])
            _AddDeletionResultToReport(
                r, *result, resource_name="cached_build_artifact")

        # Everything succeeded, write status to report.
        if r.status == report.Status.UNKNOWN:
            r.SetStatus(report.Status.SUCCESS)
    except errors.DriverError as e:
        r.AddError(str(e))
        r.SetStatus(report.Status.FAIL)
    return r


def AddSshRsa(cfg, user, ssh_rsa_path):
    """Add public ssh rsa key to the project.

    Args:
        cfg: An AcloudConfig instance.
        user: the name of the user which the key belongs to.
        ssh_rsa_path: The absolute path to public rsa key.

    Returns:
        A Report instance.
    """
    r = report.Report(command="sshkey")
    try:
        credentials = auth.CreateCredentials(cfg, ALL_SCOPES)
        compute_client = android_compute_client.AndroidComputeClient(
            cfg, credentials)
        compute_client.AddSshRsa(user, ssh_rsa_path)
        r.SetStatus(report.Status.SUCCESS)
    except errors.DriverError as e:
        r.AddError(str(e))
        r.SetStatus(report.Status.FAIL)
    return r


def CheckAccess(cfg):
    """Check if user has access.

    Args:
         cfg: An AcloudConfig instance.
    """
    credentials = auth.CreateCredentials(cfg, ALL_SCOPES)
    compute_client = android_compute_client.AndroidComputeClient(
            cfg, credentials)
    logger.info("Checking if user has access to project %s", cfg.project)
    if not compute_client.CheckAccess():
        logger.error("User does not have access to project %s", cfg.project)
        # Print here so that command line user can see it.
        print "Looks like you do not have access to %s. " % cfg.project
        if cfg.project in cfg.no_project_access_msg_map:
            print cfg.no_project_access_msg_map[cfg.project]
