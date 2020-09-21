#
# Copyright (C) 2017 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import logging
import socket
import time
import uuid

import httplib2
from googleapiclient import errors

from host_controller import invocation_thread
from host_controller.tradefed import remote_operation
from host_controller.tfc import command_attempt


class HostController(object):
    """The class that relays commands between a TradeFed host and clusters.

    Attributes:
        _remote_client: The RemoteClient which runs commands.
        _tfc_client: The TfcClient from which the command tasks are leased.
        _hostname: A string, the name of the TradeFed host.
        _cluster_ids: A list of strings, the cluster IDs for leasing tasks.
        _invocation_threads: The list of running InvocationThread.
    """

    def __init__(self, remote_client, tfc_client, hostname, cluster_ids):
        """Initializes the attributes."""
        self._remote_client = remote_client
        self._tfc_client = tfc_client
        self._hostname = hostname
        self._cluster_ids = cluster_ids
        self._invocation_threads = []

    @property
    def hostname(self):
        """Returns the name of the host."""
        return self._hostname

    def _JoinInvocationThreads(self):
        """Removes terminated threads from _invocation_threads."""
        alive_threads = []
        for inv_thread in self._invocation_threads:
            inv_thread.join(0)
            if inv_thread.is_alive():
                alive_threads.append(inv_thread)
        self._invocation_threads = alive_threads

    def _CreateInvocationThread(self, task):
        """Creates an invocation thread from a command task.

        Args:
            task: The CommandTask object.

        Returns:
            An InvocationThread.
        """
        attempt_id = uuid.uuid4()
        attempt = command_attempt.CommandAttempt(
                task.task_id, attempt_id,
                self._hostname, task.device_serials[0])
        inv_thread = invocation_thread.InvocationThread(
                self._remote_client, self._tfc_client, attempt,
                task.command_line.split(), task.device_serials)
        return inv_thread

    def ListDevices(self):
        """Lists present devices on the host.

        Returns:
            A list of DeviceInfo.
        """
        devices = self._remote_client.ListDevices()
        return [dev for dev in devices if not dev.IsStub()]

    def ListAvailableDevices(self):
        """Lists available devices for command tasks.

        Returns:
            A list of DeviceInfo.
        """
        self._JoinInvocationThreads()
        allocated_serials = set()
        for inv_thread in self._invocation_threads:
            allocated_serials.update(inv_thread.device_serials)

        present_devices = self.ListDevices()
        return [dev for dev in present_devices if
                dev.IsAvailable() and
                dev.device_serial not in allocated_serials]

    def LeaseCommandTasks(self):
        """Leases command tasks and creates threads to execute them.

        Returns:
            A list of CommandTask. The leased command tasks.
        """
        available_devices = self.ListAvailableDevices()
        if not available_devices:
            return []

        tasks = self._tfc_client.LeaseHostTasks(
                self._cluster_ids[0], self._cluster_ids[1:],
                self._hostname, available_devices)
        for task in tasks:
            inv_thread = self._CreateInvocationThread(task)
            inv_thread.daemon = True
            inv_thread.start()
            self._invocation_threads.append(inv_thread)
        return tasks

    def Run(self, poll_interval):
        """Starts polling TFC for tasks.

        Args:
            poll_interval: The poll interval in seconds.
        """
        while True:
            try:
                self.LeaseCommandTasks()
            except (socket.error,
                    remote_operation.RemoteOperationException,
                    httplib2.HttpLib2Error,
                    errors.HttpError) as e:
                logging.exception(e)
            time.sleep(poll_interval)
