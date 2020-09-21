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
import threading

import httplib2
from googleapiclient import errors

from host_controller.tfc import command_attempt
from host_controller.tradefed import remote_operation


class InvocationThread(threading.Thread):
    """The thread that remotely executes a command task.

    Attributes:
        _remote_client: The RemoteClient which executes the command.
        _tfc_client: The TfcClient to which the command events are sent.
        _attempt: The CommandAttempt whose events are sent to TFC.
        _command: A list of strings, the command and arguments.
        device_serials: A list of strings, the serial numbers of the devices
                        which need to be allocated to the task.
        _allocated_serials: A list of strings, the serial numbers of the devices
                            which are successfully allocated.
        _tfc_heartbeat_interval: The interval of TestRunInProgress events in
                                 seconds.
    """

    def __init__(self,
                 remote_client,
                 tfc_client,
                 attempt,
                 command,
                 device_serials,
                 tfc_heartbeat_interval=5 * 60):
        """Initializes the attributes."""
        super(InvocationThread, self).__init__()
        self._remote_client = remote_client
        self._tfc_client = tfc_client
        self._attempt = attempt
        self._command = command
        self.device_serials = device_serials
        self._allocated_serials = None
        # The value in Java implementation is 5 minutes.
        self._tfc_heartbeat_interval = tfc_heartbeat_interval

    def _AllocateDevices(self):
        """Allocates all of device_serial."""
        for serial in self.device_serials:
            self._remote_client.SendOperation(
                    remote_operation.AllocateDevice(serial))
            self._allocated_serials.append(serial)

    def _StartInvocation(self):
        """Starts executing command and sends the event to TFC."""
        self._remote_client.SendOperation(
                remote_operation.ExecuteCommand(self.device_serials[0],
                                                *self._command))
        event = self._attempt.CreateCommandEvent(
                command_attempt.EventType.INVOCATION_STARTED)
        self._tfc_client.SubmitCommandEvents([event])

    def _WaitForCommandResult(self):
        """Waits for command result and keeps sending heartbeat to TFC

        Returns:
            A JSON object returned from TradeFed remote manager.
        """
        while True:
            result = self._remote_client.WaitForCommandResult(
                    self.device_serials[0], self._tfc_heartbeat_interval)
            if result:
                return result
            event = self._attempt.CreateCommandEvent(
                    command_attempt.EventType.TEST_RUN_IN_PROGRESS)
            self._tfc_client.SubmitCommandEvents([event])

    def _CompleteInvocation(self, result):
        """Sends InvocationCompleted event according to the result.

        Args:
            result: A JSON object returned from TradeFed remote manager.
        """
        if result["status"] == "INVOCATION_SUCCESS":
            event = self._attempt.CreateInvocationCompletedEvent(
                    str(result), 1, 0)
        else:
            event = self._attempt.CreateInvocationCompletedEvent(
                    str(result), 1, 1, error=str(result))
        self._tfc_client.SubmitCommandEvents([event])

    def _FreeAllocatedDevices(self):
        """Frees allocated devices and tolerates RemoteOperationException."""
        for serial in self._allocated_serials:
            try:
                self._remote_client.SendOperation(
                        remote_operation.FreeDevice(serial))
            except remote_operation.RemoteOperationException as e:
                logging.exception(e)
            except socket.error as e:
                logging.exception(e)
                break
        self._allocated_serials = []

    def _SubmitErrorEvent(self, event_type, error_msg):
        """Submits an error event and tolerates http exceptions.

        Args:
            event_type: A string, the type of the command event.
            error_msg: A string, the error message.
        """
        try:
            self._tfc_client.SubmitCommandEvents(
                [self._attempt.CreateCommandEvent(event_type, error_msg)])
        except (httplib2.HttpLib2Error, errors.HttpError) as e:
            logging.exception(e)

    # @Override
    def run(self):
        """Executes a command task with exception handling."""
        self._allocated_serials = []
        last_error = None
        error_event = command_attempt.EventType.ALLOCATION_FAILED
        try:
            self._AllocateDevices()
            error_event = command_attempt.EventType.EXECUTE_FAILED
            self._StartInvocation()
            result = self._WaitForCommandResult()
            self._CompleteInvocation(result)
            error_event = None
        except errors.HttpError as e:
            logging.exception(e)
            last_error = e
        except remote_operation.RemoteOperationException as e:
            logging.exception(e)
            last_error = e
            # ConfigurationException on TradeFed remote manager.
            if str(e).startswith("Config error: "):
                error_event = command_attempt.EventType.CONFIGURATION_ERROR
        except httplib2.HttpLib2Error as e:
            logging.exception("Cannot communicate with TradeFed cluster: %s\n"
                              "Skip submitting event %s.", e, error_event)
            last_error = e
            error_event = None
        except socket.error as e:
            logging.exception("Cannot communicate with TradeFed remote "
                              "manager: %s\nSkip freeing devices %s.",
                              e, self._allocated_serials)
            last_error = e
            self._allocated_serials = []
        finally:
            if error_event:
                self._SubmitErrorEvent(error_event, str(last_error))
            self._FreeAllocatedDevices()
