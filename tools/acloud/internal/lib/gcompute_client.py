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

"""A client that manages Google Compute Engine.

** ComputeClient **

ComputeClient is a wrapper around Google Compute Engine APIs.
It provides a set of methods for managing a google compute engine project,
such as creating images, creating instances, etc.

Design philosophy: We tried to make ComputeClient as stateless as possible,
and it only keeps states about authentication. ComputeClient should be very
generic, and only knows how to talk to Compute Engine APIs.
"""
import copy
import functools
import logging
import os

from acloud.internal.lib import base_cloud_client
from acloud.internal.lib import utils
from acloud.public import errors

logger = logging.getLogger(__name__)


class OperationScope(object):
    """Represents operation scope enum."""
    ZONE = "zone"
    REGION = "region"
    GLOBAL = "global"


class ComputeClient(base_cloud_client.BaseCloudApiClient):
    """Client that manages GCE."""

    # API settings, used by BaseCloudApiClient.
    API_NAME = "compute"
    API_VERSION = "v1"
    SCOPE = " ".join(["https://www.googleapis.com/auth/compute",
                      "https://www.googleapis.com/auth/devstorage.read_write"])
    # Default settings for gce operations
    DEFAULT_INSTANCE_SCOPE = [
        "https://www.googleapis.com/auth/devstorage.read_only",
        "https://www.googleapis.com/auth/logging.write"
    ]
    OPERATION_TIMEOUT_SECS = 15 * 60  # 15 mins
    OPERATION_POLL_INTERVAL_SECS = 5
    MACHINE_SIZE_METRICS = ["guestCpus", "memoryMb"]
    ACCESS_DENIED_CODE = 403

    def __init__(self, acloud_config, oauth2_credentials):
        """Initialize.

        Args:
            acloud_config: An AcloudConfig object.
            oauth2_credentials: An oauth2client.OAuth2Credentials instance.
        """
        super(ComputeClient, self).__init__(oauth2_credentials)
        self._project = acloud_config.project

    def _GetOperationStatus(self, operation, operation_scope, scope_name=None):
        """Get status of an operation.

        Args:
            operation: An Operation resource in the format of json.
            operation_scope: A value from OperationScope, "zone", "region",
                             or "global".
            scope_name: If operation_scope is "zone" or "region", this should be
                        the name of the zone or region, e.g. "us-central1-f".

        Returns:
            Status of the operation, one of "DONE", "PENDING", "RUNNING".

        Raises:
            errors.DriverError: if the operation fails.
        """
        operation_name = operation["name"]
        if operation_scope == OperationScope.GLOBAL:
            api = self.service.globalOperations().get(project=self._project,
                                                      operation=operation_name)
            result = self.Execute(api)
        elif operation_scope == OperationScope.ZONE:
            api = self.service.zoneOperations().get(project=self._project,
                                                    operation=operation_name,
                                                    zone=scope_name)
            result = self.Execute(api)
        elif operation_scope == OperationScope.REGION:
            api = self.service.regionOperations().get(project=self._project,
                                                      operation=operation_name,
                                                      region=scope_name)
            result = self.Execute(api)

        if result.get("error"):
            errors_list = result["error"]["errors"]
            raise errors.DriverError("Get operation state failed, errors: %s" %
                                     str(errors_list))
        return result["status"]

    def WaitOnOperation(self, operation, operation_scope, scope_name=None):
        """Wait for an operation to finish.

        Args:
            operation: An Operation resource in the format of json.
            operation_scope: A value from OperationScope, "zone", "region",
                             or "global".
            scope_name: If operation_scope is "zone" or "region", this should be
                        the name of the zone or region, e.g. "us-central1-f".
        """
        timeout_exception = errors.GceOperationTimeoutError(
            "Operation hits timeout, did not complete within %d secs." %
            self.OPERATION_TIMEOUT_SECS)
        utils.PollAndWait(
            func=self._GetOperationStatus,
            expected_return="DONE",
            timeout_exception=timeout_exception,
            timeout_secs=self.OPERATION_TIMEOUT_SECS,
            sleep_interval_secs=self.OPERATION_POLL_INTERVAL_SECS,
            operation=operation,
            operation_scope=operation_scope,
            scope_name=scope_name)

    def GetProject(self):
        """Get project information.

        Returns:
          A project resource in json.
        """
        api = self.service.projects().get(project=self._project)
        return self.Execute(api)

    def GetDisk(self, disk_name, zone):
        """Get disk information.

        Args:
          disk_name: A string.
          zone: String, name of zone.

        Returns:
          An disk resource in json.
          https://cloud.google.com/compute/docs/reference/latest/disks#resource
        """
        api = self.service.disks().get(project=self._project,
                                       zone=zone,
                                       disk=disk_name)
        return self.Execute(api)

    def CheckDiskExists(self, disk_name, zone):
        """Check if disk exists.

        Args:
          disk_name: A string
          zone: String, name of zone.

        Returns:
          True if disk exists, otherwise False.
        """
        try:
            self.GetDisk(disk_name, zone)
            exists = True
        except errors.ResourceNotFoundError:
            exists = False
        logger.debug("CheckDiskExists: disk_name: %s, result: %s", disk_name,
                     exists)
        return exists

    def CreateDisk(self, disk_name, source_image, size_gb, zone):
        """Create a gce disk.

        Args:
            disk_name: A string
            source_image: A stirng, name of the image.
            size_gb: Integer, size in gb.
            zone: Name of the zone, e.g. us-central1-b.
        """
        logger.info("Creating disk %s, size_gb: %d", disk_name, size_gb)
        source_image = "projects/%s/global/images/%s" % (
            self._project, source_image) if source_image else None
        body = {
            "name": disk_name,
            "sizeGb": size_gb,
            "type": "projects/%s/zones/%s/diskTypes/pd-standard" % (
                self._project, zone),
        }
        api = self.service.disks().insert(project=self._project,
                                          sourceImage=source_image,
                                          zone=zone,
                                          body=body)
        operation = self.Execute(api)
        try:
            self.WaitOnOperation(operation=operation,
                                 operation_scope=OperationScope.ZONE,
                                 scope_name=zone)
        except errors.DriverError:
            logger.error("Creating disk failed, cleaning up: %s", disk_name)
            if self.CheckDiskExists(disk_name, zone):
                self.DeleteDisk(disk_name, zone)
            raise
        logger.info("Disk %s has been created.", disk_name)

    def DeleteDisk(self, disk_name, zone):
        """Delete a gce disk.

        Args:
            disk_name: A string, name of disk.
            zone: A string, name of zone.
        """
        logger.info("Deleting disk %s", disk_name)
        api = self.service.disks().delete(project=self._project,
                                          zone=zone,
                                          disk=disk_name)
        operation = self.Execute(api)
        self.WaitOnOperation(operation=operation,
                             operation_scope=OperationScope.GLOBAL)
        logger.info("Deleted disk %s", disk_name)

    def DeleteDisks(self, disk_names, zone):
        """Delete multiple disks.

        Args:
            disk_names: A list of disk names.
            zone: A string, name of zone.

        Returns:
            A tuple, (deleted, failed, error_msgs)
            deleted: A list of names of disks that have been deleted.
            failed: A list of names of disks that we fail to delete.
            error_msgs: A list of failure messages.
        """
        if not disk_names:
            logger.warn("Nothing to delete. Arg disk_names is not provided.")
            return [], [], []
        # Batch send deletion requests.
        logger.info("Deleting disks: %s", disk_names)
        delete_requests = {}
        for disk_name in set(disk_names):
            request = self.service.disks().delete(project=self._project,
                                                  disk=disk_name,
                                                  zone=zone)
            delete_requests[disk_name] = request
        return self._BatchExecuteAndWait(delete_requests,
                                         OperationScope.GLOBAL)

    def ListDisks(self, zone, disk_filter=None):
        """List disks.

        Args:
            zone: A string, representing zone name. e.g. "us-central1-f"
            disk_filter: A string representing a filter in format of
                             FIELD_NAME COMPARISON_STRING LITERAL_STRING
                             e.g. "name ne example-instance"
                             e.g. "name eq "example-instance-[0-9]+""

        Returns:
            A list of disks.
        """
        return self.ListWithMultiPages(api_resource=self.service.disks().list,
                                       project=self._project,
                                       zone=zone,
                                       filter=disk_filter)

    def CreateImage(self, image_name, source_uri):
        """Create a Gce image.

        Args:
            image_name: A string
            source_uri: Full Google Cloud Storage URL where the disk image is
                        stored. e.g. "https://storage.googleapis.com/my-bucket/
                        avd-system-2243663.tar.gz"
        """
        logger.info("Creating image %s, source_uri %s", image_name, source_uri)
        body = {"name": image_name, "rawDisk": {"source": source_uri, }, }
        api = self.service.images().insert(project=self._project, body=body)
        operation = self.Execute(api)
        try:
            self.WaitOnOperation(operation=operation,
                                 operation_scope=OperationScope.GLOBAL)
        except errors.DriverError:
            logger.error("Creating image failed, cleaning up: %s", image_name)
            if self.CheckImageExists(image_name):
                self.DeleteImage(image_name)
            raise
        logger.info("Image %s has been created.", image_name)

    def CheckImageExists(self, image_name):
        """Check if image exists.

        Args:
            image_name: A string

        Returns:
            True if image exists, otherwise False.
        """
        try:
            self.GetImage(image_name)
            exists = True
        except errors.ResourceNotFoundError:
            exists = False
        logger.debug("CheckImageExists: image_name: %s, result: %s",
                     image_name, exists)
        return exists

    def GetImage(self, image_name):
        """Get image information.

        Args:
            image_name: A string

        Returns:
            An image resource in json.
            https://cloud.google.com/compute/docs/reference/latest/images#resource
        """
        api = self.service.images().get(project=self._project,
                                        image=image_name)
        return self.Execute(api)

    def DeleteImage(self, image_name):
        """Delete an image.

        Args:
            image_name: A string
        """
        logger.info("Deleting image %s", image_name)
        api = self.service.images().delete(project=self._project,
                                           image=image_name)
        operation = self.Execute(api)
        self.WaitOnOperation(operation=operation,
                             operation_scope=OperationScope.GLOBAL)
        logger.info("Deleted image %s", image_name)

    def DeleteImages(self, image_names):
        """Delete multiple images.

        Args:
            image_names: A list of image names.

        Returns:
            A tuple, (deleted, failed, error_msgs)
            deleted: A list of names of images that have been deleted.
            failed: A list of names of images that we fail to delete.
            error_msgs: A list of failure messages.
        """
        if not image_names:
            return [], [], []
        # Batch send deletion requests.
        logger.info("Deleting images: %s", image_names)
        delete_requests = {}
        for image_name in set(image_names):
            request = self.service.images().delete(project=self._project,
                                                   image=image_name)
            delete_requests[image_name] = request
        return self._BatchExecuteAndWait(delete_requests,
                                         OperationScope.GLOBAL)

    def ListImages(self, image_filter=None):
        """List images.

        Args:
            image_filter: A string representing a filter in format of
                          FIELD_NAME COMPARISON_STRING LITERAL_STRING
                          e.g. "name ne example-image"
                          e.g. "name eq "example-image-[0-9]+""

        Returns:
            A list of images.
        """
        return self.ListWithMultiPages(api_resource=self.service.images().list,
                                       project=self._project,
                                       filter=image_filter)

    def GetInstance(self, instance, zone):
        """Get information about an instance.

        Args:
            instance: A string, representing instance name.
            zone: A string, representing zone name. e.g. "us-central1-f"

        Returns:
            An instance resource in json.
            https://cloud.google.com/compute/docs/reference/latest/instances#resource
        """
        api = self.service.instances().get(project=self._project,
                                           zone=zone,
                                           instance=instance)
        return self.Execute(api)

    def StartInstance(self, instance, zone):
        """Start |instance| in |zone|.

        Args:
            instance: A string, representing instance name.
            zone: A string, representing zone name. e.g. "us-central1-f"

        Raises:
            errors.GceOperationTimeoutError: Operation takes too long to finish.
        """
        api = self.service.instances().start(project=self._project,
                                             zone=zone,
                                             instance=instance)
        operation = self.Execute(api)
        try:
            self.WaitOnOperation(operation=operation,
                                 operation_scope=OperationScope.ZONE,
                                 scope_name=zone)
        except errors.GceOperationTimeoutError:
            logger.error("Start instance failed: %s", instance)
            raise
        logger.info("Instance %s has been started.", instance)

    def StartInstances(self, instances, zone):
        """Start |instances| in |zone|.

        Args:
            instances: A list of strings, representing instance names's list.
            zone: A string, representing zone name. e.g. "us-central1-f"

        Returns:
            A tuple, (done, failed, error_msgs)
            done: A list of string, representing the names of instances that
              have been executed.
            failed: A list of string, representing the names of instances that
              we failed to execute.
            error_msgs: A list of string, representing the failure messages.
        """
        action = functools.partial(self.service.instances().start,
                                   project=self._project,
                                   zone=zone)
        return self._BatchExecuteOnInstances(instances, zone, action)

    def StopInstance(self, instance, zone):
        """Stop |instance| in |zone|.

        Args:
            instance: A string, representing instance name.
            zone: A string, representing zone name. e.g. "us-central1-f"

        Raises:
            errors.GceOperationTimeoutError: Operation takes too long to finish.
        """
        api = self.service.instances().stop(project=self._project,
                                            zone=zone,
                                            instance=instance)
        operation = self.Execute(api)
        try:
            self.WaitOnOperation(operation=operation,
                                 operation_scope=OperationScope.ZONE,
                                 scope_name=zone)
        except errors.GceOperationTimeoutError:
            logger.error("Stop instance failed: %s", instance)
            raise
        logger.info("Instance %s has been terminated.", instance)

    def StopInstances(self, instances, zone):
        """Stop |instances| in |zone|.

        Args:
          instances: A list of strings, representing instance names's list.
          zone: A string, representing zone name. e.g. "us-central1-f"

        Returns:
            A tuple, (done, failed, error_msgs)
            done: A list of string, representing the names of instances that
                  have been executed.
            failed: A list of string, representing the names of instances that
                    we failed to execute.
            error_msgs: A list of string, representing the failure messages.
        """
        action = functools.partial(self.service.instances().stop,
                                   project=self._project,
                                   zone=zone)
        return self._BatchExecuteOnInstances(instances, zone, action)

    def SetScheduling(self,
                      instance,
                      zone,
                      automatic_restart=True,
                      on_host_maintenance="MIGRATE"):
        """Update scheduling config |automatic_restart| and |on_host_maintenance|.

        See //cloud/cluster/api/mixer_instances.proto Scheduling for config option.

        Args:
            instance: A string, representing instance name.
            zone: A string, representing zone name. e.g. "us-central1-f".
            automatic_restart: Boolean, determine whether the instance will
                               automatically restart if it crashes or not,
                               default to True.
            on_host_maintenance: enum["MIGRATE", "TERMINATED]
                                 The instance's maintenance behavior, which
                                 determines whether the instance is live
                                 "MIGRATE" or "TERMINATED" when there is
                                 a maintenance event.

        Raises:
            errors.GceOperationTimeoutError: Operation takes too long to finish.
        """
        body = {"automaticRestart": automatic_restart,
                "OnHostMaintenance": on_host_maintenance}
        api = self.service.instances().setScheduling(project=self._project,
                                                     zone=zone,
                                                     instance=instance,
                                                     body=body)
        operation = self.Execute(api)
        try:
            self.WaitOnOperation(operation=operation,
                                 operation_scope=OperationScope.ZONE,
                                 scope_name=zone)
        except errors.GceOperationTimeoutError:
            logger.error("Set instance scheduling failed: %s", instance)
            raise
        logger.info("Instance scheduling changed:\n"
                    "    automaticRestart: %s\n"
                    "    onHostMaintenance: %s\n",
                    str(automatic_restart).lower(), on_host_maintenance)

    def ListInstances(self, zone, instance_filter=None):
        """List instances.

        Args:
            zone: A string, representing zone name. e.g. "us-central1-f"
            instance_filter: A string representing a filter in format of
                             FIELD_NAME COMPARISON_STRING LITERAL_STRING
                             e.g. "name ne example-instance"
                             e.g. "name eq "example-instance-[0-9]+""

        Returns:
            A list of instances.
        """
        return self.ListWithMultiPages(
            api_resource=self.service.instances().list,
            project=self._project,
            zone=zone,
            filter=instance_filter)

    def SetSchedulingInstances(self,
                               instances,
                               zone,
                               automatic_restart=True,
                               on_host_maintenance="MIGRATE"):
        """Update scheduling config |automatic_restart| and |on_host_maintenance|.

        See //cloud/cluster/api/mixer_instances.proto Scheduling for config option.

        Args:
            instances: A list of string, representing instance names.
            zone: A string, representing zone name. e.g. "us-central1-f".
            automatic_restart: Boolean, determine whether the instance will
                               automatically restart if it crashes or not,
                               default to True.
            on_host_maintenance: enum["MIGRATE", "TERMINATED]
                                 The instance's maintenance behavior, which
                                 determines whether the instance is live
                                 "MIGRATE" or "TERMINATED" when there is
                                 a maintenance event.

        Returns:
            A tuple, (done, failed, error_msgs)
            done: A list of string, representing the names of instances that
                  have been executed.
            failed: A list of string, representing the names of instances that
                    we failed to execute.
            error_msgs: A list of string, representing the failure messages.
        """
        body = {"automaticRestart": automatic_restart,
                "OnHostMaintenance": on_host_maintenance}
        action = functools.partial(self.service.instances().setScheduling,
                                   project=self._project,
                                   zone=zone,
                                   body=body)
        return self._BatchExecuteOnInstances(instances, zone, action)

    def _BatchExecuteOnInstances(self, instances, zone, action):
        """Batch processing operations requiring computing time.

        Args:
            instances: A list of instance names.
            zone: A string, e.g. "us-central1-f".
            action: partial func, all kwargs for this gcloud action has been
                    defined in the caller function (e.g. See "StartInstances")
                    except 'instance' which will be defined by iterating the
                    |instances|.

        Returns:
            A tuple, (done, failed, error_msgs)
            done: A list of string, representing the names of instances that
                  have been executed.
            failed: A list of string, representing the names of instances that
                    we failed to execute.
            error_msgs: A list of string, representing the failure messages.
        """
        if not instances:
            return [], [], []
        # Batch send requests.
        logger.info("Batch executing instances: %s", instances)
        requests = {}
        for instance_name in set(instances):
            requests[instance_name] = action(instance=instance_name)
        return self._BatchExecuteAndWait(requests,
                                         operation_scope=OperationScope.ZONE,
                                         scope_name=zone)

    def _BatchExecuteAndWait(self, requests, operation_scope, scope_name=None):
        """Batch processing requests and wait on the operation.

        Args:
          requests: A dictionary. The key is a string representing the resource
                    name. For example, an instance name, or an image name.
          operation_scope: A value from OperationScope, "zone", "region",
                           or "global".
          scope_name: If operation_scope is "zone" or "region", this should be
                      the name of the zone or region, e.g. "us-central1-f".

        Returns:
          A tuple, (done, failed, error_msgs)
          done: A list of string, representing the resource names that have
                been executed.
          failed: A list of string, representing resource names that
                  we failed to execute.
          error_msgs: A list of string, representing the failure messages.
        """
        results = self.BatchExecute(requests)
        # Initialize return values
        failed = []
        error_msgs = []
        for resource_name, (_, error) in results.iteritems():
            if error is not None:
                failed.append(resource_name)
                error_msgs.append(str(error))
        done = []
        # Wait for the executing operations to finish.
        logger.info("Waiting for executing operations")
        for resource_name in requests.iterkeys():
            operation, _ = results[resource_name]
            if operation:
                try:
                    self.WaitOnOperation(operation, operation_scope,
                                         scope_name)
                    done.append(resource_name)
                except errors.DriverError as exc:
                    failed.append(resource_name)
                    error_msgs.append(str(exc))
        return done, failed, error_msgs

    def ListZones(self):
        """List all zone instances in the project.

        Returns:
            Gcompute response instance. For example:
            {
              "id": "projects/google.com%3Aandroid-build-staging/zones",
              "kind": "compute#zoneList",
              "selfLink": "https://www.googleapis.com/compute/v1/projects/"
                  "google.com:android-build-staging/zones"
              "items": [
                {
                  'creationTimestamp': '2014-07-15T10:44:08.663-07:00',
                  'description': 'asia-east1-c',
                  'id': '2222',
                  'kind': 'compute#zone',
                  'name': 'asia-east1-c',
                  'region': 'https://www.googleapis.com/compute/v1/projects/'
                      'google.com:android-build-staging/regions/asia-east1',
                  'selfLink': 'https://www.googleapis.com/compute/v1/projects/'
                      'google.com:android-build-staging/zones/asia-east1-c',
                  'status': 'UP'
                }, {
                  'creationTimestamp': '2014-05-30T18:35:16.575-07:00',
                  'description': 'asia-east1-b',
                  'id': '2221',
                  'kind': 'compute#zone',
                  'name': 'asia-east1-b',
                  'region': 'https://www.googleapis.com/compute/v1/projects/'
                      'google.com:android-build-staging/regions/asia-east1',
                  'selfLink': 'https://www.googleapis.com/compute/v1/projects'
                      '/google.com:android-build-staging/zones/asia-east1-b',
                  'status': 'UP'
                }]
            }
            See cloud cluster's api/mixer_zones.proto
        """
        api = self.service.zones().list(project=self._project)
        return self.Execute(api)

    def _GetNetworkArgs(self, network):
        """Helper to generate network args that is used to create an instance.

        Args:
            network: A string, e.g. "default".

        Returns:
            A dictionary representing network args.
        """
        return {
            "network": self.GetNetworkUrl(network),
            "accessConfigs": [{"name": "External NAT",
                               "type": "ONE_TO_ONE_NAT"}]
        }

    def _GetDiskArgs(self, disk_name, image_name):
        """Helper to generate disk args that is used to create an instance.

        Args:
            disk_name: A string
            image_name: A string

        Returns:
            A dictionary representing disk args.
        """
        return [{
            "type": "PERSISTENT",
            "boot": True,
            "mode": "READ_WRITE",
            "autoDelete": True,
            "initializeParams": {
                "diskName": disk_name,
                "sourceImage": self.GetImage(image_name)["selfLink"],
            },
        }]

    def CreateInstance(self,
                       instance,
                       image_name,
                       machine_type,
                       metadata,
                       network,
                       zone,
                       disk_args=None):
        """Create a gce instance with a gce image.

        Args:
          instance: instance name.
          image_name: A source image used to create this disk.
          machine_type: A string, representing machine_type, e.g. "n1-standard-1"
          metadata: A dictionary that maps a metadata name to its value.
          network: A string, representing network name, e.g. "default"
          zone: A string, representing zone name, e.g. "us-central1-f"
          disk_args: A list of extra disk args, see _GetDiskArgs for example,
                     if None, will create a disk using the given image.
        """
        disk_args = (disk_args or self._GetDiskArgs(instance, image_name))
        body = {
            "machineType": self.GetMachineType(machine_type, zone)["selfLink"],
            "name": instance,
            "networkInterfaces": [self._GetNetworkArgs(network)],
            "disks": disk_args,
            "serviceAccounts": [
                {"email": "default",
                 "scopes": self.DEFAULT_INSTANCE_SCOPE}
            ],
        }

        if metadata:
            metadata_list = [{"key": key,
                              "value": val}
                             for key, val in metadata.iteritems()]
            body["metadata"] = {"items": metadata_list}
        logger.info("Creating instance: project %s, zone %s, body:%s",
                    self._project, zone, body)
        api = self.service.instances().insert(project=self._project,
                                              zone=zone,
                                              body=body)
        operation = self.Execute(api)
        self.WaitOnOperation(operation,
                             operation_scope=OperationScope.ZONE,
                             scope_name=zone)
        logger.info("Instance %s has been created.", instance)

    def DeleteInstance(self, instance, zone):
        """Delete a gce instance.

        Args:
          instance: A string, instance name.
          zone: A string, e.g. "us-central1-f"
        """
        logger.info("Deleting instance: %s", instance)
        api = self.service.instances().delete(project=self._project,
                                              zone=zone,
                                              instance=instance)
        operation = self.Execute(api)
        self.WaitOnOperation(operation,
                             operation_scope=OperationScope.ZONE,
                             scope_name=zone)
        logger.info("Deleted instance: %s", instance)

    def DeleteInstances(self, instances, zone):
        """Delete multiple instances.

        Args:
            instances: A list of instance names.
            zone: A string, e.g. "us-central1-f".

        Returns:
            A tuple, (deleted, failed, error_msgs)
            deleted: A list of names of instances that have been deleted.
            failed: A list of names of instances that we fail to delete.
            error_msgs: A list of failure messages.
        """
        action = functools.partial(self.service.instances().delete,
                                   project=self._project,
                                   zone=zone)
        return self._BatchExecuteOnInstances(instances, zone, action)

    def ResetInstance(self, instance, zone):
        """Reset the gce instance.

        Args:
            instance: A string, instance name.
            zone: A string, e.g. "us-central1-f".
        """
        logger.info("Resetting instance: %s", instance)
        api = self.service.instances().reset(project=self._project,
                                             zone=zone,
                                             instance=instance)
        operation = self.Execute(api)
        self.WaitOnOperation(operation,
                             operation_scope=OperationScope.ZONE,
                             scope_name=zone)
        logger.info("Instance has been reset: %s", instance)

    def GetMachineType(self, machine_type, zone):
        """Get URL for a given machine typle.

        Args:
            machine_type: A string, name of the machine type.
            zone: A string, e.g. "us-central1-f"

        Returns:
            A machine type resource in json.
            https://cloud.google.com/compute/docs/reference/latest/
            machineTypes#resource
        """
        api = self.service.machineTypes().get(project=self._project,
                                              zone=zone,
                                              machineType=machine_type)
        return self.Execute(api)

    def GetNetworkUrl(self, network):
        """Get URL for a given network.

        Args:
            network: A string, representing network name, e.g "default"

        Returns:
            A URL that points to the network resource, e.g.
            https://www.googleapis.com/compute/v1/projects/<project id>/
            global/networks/default
        """
        api = self.service.networks().get(project=self._project,
                                          network=network)
        result = self.Execute(api)
        return result["selfLink"]

    def CompareMachineSize(self, machine_type_1, machine_type_2, zone):
        """Compare the size of two machine types.

        Args:
            machine_type_1: A string representing a machine type, e.g. n1-standard-1
            machine_type_2: A string representing a machine type, e.g. n1-standard-1
            zone: A string representing a zone, e.g. "us-central1-f"

        Returns:
            1 if size of the first type is greater than the second type.
            2 if size of the first type is smaller than the second type.
            0 if they are equal.

        Raises:
            errors.DriverError: For malformed response.
        """
        machine_info_1 = self.GetMachineType(machine_type_1, zone)
        machine_info_2 = self.GetMachineType(machine_type_2, zone)
        for metric in self.MACHINE_SIZE_METRICS:
            if metric not in machine_info_1 or metric not in machine_info_2:
                raise errors.DriverError(
                    "Malformed machine size record: Can't find '%s' in %s or %s"
                    % (metric, machine_info_1, machine_info_2))
            if machine_info_1[metric] - machine_info_2[metric] > 0:
                return 1
            elif machine_info_1[metric] - machine_info_2[metric] < 0:
                return -1
        return 0

    def GetSerialPortOutput(self, instance, zone, port=1):
        """Get serial port output.

        Args:
            instance: string, instance name.
            zone: string, zone name.
            port: int, which COM port to read from, 1-4, default to 1.

        Returns:
            String, contents of the output.

        Raises:
            errors.DriverError: For malformed response.
        """
        api = self.service.instances().getSerialPortOutput(
            project=self._project,
            zone=zone,
            instance=instance,
            port=port)
        result = self.Execute(api)
        if "contents" not in result:
            raise errors.DriverError(
                "Malformed response for GetSerialPortOutput: %s" % result)
        return result["contents"]

    def GetInstanceNamesByIPs(self, ips, zone):
        """Get Instance names by IPs.

        This function will go through all instances, which
        could be slow if there are too many instances.  However, currently
        GCE doesn't support search for instance by IP.

        Args:
            ips: A set of IPs.
            zone: String, name of the zone.

        Returns:
            A dictionary where key is IP and value is instance name or None
            if instance is not found for the given IP.
        """
        ip_name_map = dict.fromkeys(ips)
        for instance in self.ListInstances(zone):
            try:
                ip = instance["networkInterfaces"][0]["accessConfigs"][0][
                    "natIP"]
                if ip in ips:
                    ip_name_map[ip] = instance["name"]
            except (IndexError, KeyError) as e:
                logger.error("Could not get instance names by ips: %s", str(e))
        return ip_name_map

    def GetInstanceIP(self, instance, zone):
        """Get Instance IP given instance name.

        Args:
          instance: String, representing instance name.
          zone: String, name of the zone.

        Returns:
          string, IP of the instance.
        """
        # TODO(fdeng): This is for accessing external IP.
        # We should handle internal IP as well when the script is running
        # on a GCE instance in the same network of |instance|.
        instance = self.GetInstance(instance, zone)
        return instance["networkInterfaces"][0]["accessConfigs"][0]["natIP"]

    def SetCommonInstanceMetadata(self, body):
        """Set project-wide metadata.

        Args:
          body: Metadata body.
                metdata is in the following format.
                {
                  "kind": "compute#metadata",
                  "fingerprint": "a-23icsyx4E=",
                  "items": [
                    {
                      "key": "google-compute-default-region",
                      "value": "us-central1"
                    }, ...
                  ]
                }
        """
        api = self.service.projects().setCommonInstanceMetadata(
            project=self._project, body=body)
        operation = self.Execute(api)
        self.WaitOnOperation(operation, operation_scope=OperationScope.GLOBAL)

    def AddSshRsa(self, user, ssh_rsa_path):
        """Add the public rsa key to the project's metadata.

        Compute engine instances that are created after will
        by default contain the key.

        Args:
            user: the name of the user which the key belongs to.
            ssh_rsa_path: The absolute path to public rsa key.
        """
        if not os.path.exists(ssh_rsa_path):
            raise errors.DriverError("RSA file %s does not exist." %
                                     ssh_rsa_path)

        logger.info("Adding ssh rsa key from %s to project %s for user: %s",
                    ssh_rsa_path, self._project, user)
        project = self.GetProject()
        with open(ssh_rsa_path) as f:
            rsa = f.read()
            rsa = rsa.strip() if rsa else rsa
            utils.VerifyRsaPubKey(rsa)
        metadata = project["commonInstanceMetadata"]
        for item in metadata.setdefault("items", []):
            if item["key"] == "sshKeys":
                sshkey_item = item
                break
        else:
            sshkey_item = {"key": "sshKeys", "value": ""}
            metadata["items"].append(sshkey_item)

        entry = "%s:%s" % (user, rsa)
        logger.debug("New RSA entry: %s", entry)
        sshkey_item["value"] = "\n".join([sshkey_item["value"].strip(), entry
                                          ]).strip()
        self.SetCommonInstanceMetadata(metadata)

    def CheckAccess(self):
        """Check if the user has read access to the cloud project.

        Returns:
            True if the user has at least read access to the project.
            False otherwise.

        Raises:
            errors.HttpError if other unexpected error happens when
            accessing the project.
        """
        api = self.service.zones().list(project=self._project)
        retry_http_codes = copy.copy(self.RETRY_HTTP_CODES)
        retry_http_codes.remove(self.ACCESS_DENIED_CODE)
        try:
            self.Execute(api, retry_http_codes=retry_http_codes)
        except errors.HttpError as e:
            if e.code == self.ACCESS_DENIED_CODE:
                return False
            raise
        return True
