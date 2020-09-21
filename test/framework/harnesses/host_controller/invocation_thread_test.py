#!/usr/bin/env python
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

import unittest

try:
    from unittest import mock
except ImportError:
    import mock

from host_controller import invocation_thread
from host_controller.tfc import command_attempt
from host_controller.tradefed import remote_operation


class InvocationThreadTest(unittest.TestCase):
    """A test for invocation_thread.InvocationThread.

    Attributes:
        _remote_client: A mock remote_client.RemoteClient.
        _tfc_client: A mock tfc_client.TfcClient.
        _inv_thread: The InvocationThread being tested.
    """

    def setUp(self):
        """Creates the InvocationThread."""
        self._remote_client = mock.Mock()
        self._tfc_client = mock.Mock()
        attempt = command_attempt.CommandAttempt(
                task_id="321-0",
                attempt_id="abcd-1234",
                hostname="host0",
                device_serial="ABCDEF")
        command = ["vts", "-m", "SampleShellTest"]
        serials = ["serial123", "serial456"]
        self._inv_thread = invocation_thread.InvocationThread(
                self._remote_client, self._tfc_client,
                attempt, command, serials)

    def _GetSubmittedEventTypes(self):
        """Gets the types of the events submitted by the mock TfcClient.

        Returns:
            A list of strings, the event types.
        """
        event_types = []
        for args, kwargs in self._tfc_client.SubmitCommandEvents.call_args_list:
            event_types.extend(event["type"] for event in args[0])
        return event_types

    def _GetSentOperationTypes(self):
        """Gets the types of the operations sent by the mock RemoteClient.

        Returns:
            A list of strings, the operation types.
        """
        operation_types = [args[0].type for args, kwargs in
                           self._remote_client.SendOperation.call_args_list]
        return operation_types

    def testAllocationFailed(self):
        """Tests AllocationFailed event."""
        self._remote_client.SendOperation.side_effect = (
                lambda op: _RaiseExceptionForOperation(op, "ALLOCATE_DEVICE"))
        self._inv_thread.run()
        self.assertEqual([command_attempt.EventType.ALLOCATION_FAILED],
                         self._GetSubmittedEventTypes())
        self.assertEqual(["ALLOCATE_DEVICE"],
                         self._GetSentOperationTypes())

    def testExecuteFailed(self):
        """Tests ExecuteFailed event."""
        self._remote_client.SendOperation.side_effect = (
                lambda op: _RaiseExceptionForOperation(op, "EXEC_COMMAND"))
        self._inv_thread.run()
        self.assertEqual([command_attempt.EventType.EXECUTE_FAILED],
                         self._GetSubmittedEventTypes())
        self.assertEqual(["ALLOCATE_DEVICE",
                          "ALLOCATE_DEVICE",
                          "EXEC_COMMAND",
                          "FREE_DEVICE",
                          "FREE_DEVICE"],
                         self._GetSentOperationTypes())

    def testConfigurationError(self):
        """Tests ConfigurationError event."""
        self._remote_client.SendOperation.side_effect = (
                lambda op: _RaiseExceptionForOperation(op, "EXEC_COMMAND",
                                                       "Config error: test"))
        self._inv_thread.run()
        self.assertEqual([command_attempt.EventType.CONFIGURATION_ERROR],
                         self._GetSubmittedEventTypes())
        self.assertEqual(["ALLOCATE_DEVICE",
                          "ALLOCATE_DEVICE",
                          "EXEC_COMMAND",
                          "FREE_DEVICE",
                          "FREE_DEVICE"],
                         self._GetSentOperationTypes())

    def testInvocationCompleted(self):
        """Tests InvocationCompleted event."""
        self._remote_client.WaitForCommandResult.side_effect = (
                None, {"status": "INVOCATION_SUCCESS"})
        self._inv_thread.run()
        self.assertEqual([command_attempt.EventType.INVOCATION_STARTED,
                          command_attempt.EventType.TEST_RUN_IN_PROGRESS,
                          command_attempt.EventType.INVOCATION_COMPLETED],
                         self._GetSubmittedEventTypes())
        # GET_LAST_COMMAND_RESULT isn't called in mock WaitForCommandResult.
        self.assertEqual(["ALLOCATE_DEVICE",
                          "ALLOCATE_DEVICE",
                          "EXEC_COMMAND",
                          "FREE_DEVICE",
                          "FREE_DEVICE"],
                         self._GetSentOperationTypes())


def _RaiseExceptionForOperation(operation, op_type, error_msg="unit test"):
    """Raises exception for specific operation type.

    Args:
        operation: A remote_operation.RemoteOperation object.
        op_type: A string, the expected type.
        error_msg: The message in the exception.

    Raises:
        RemoteOperationException if the operation's type matches op_type.
    """
    if operation.type == op_type:
        raise remote_operation.RemoteOperationException(error_msg)


if __name__ == "__main__":
    unittest.main()
