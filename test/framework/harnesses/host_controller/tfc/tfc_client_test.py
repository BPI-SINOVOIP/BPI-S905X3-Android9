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

from host_controller.tfc import tfc_client
from host_controller.tfc import command_attempt
from host_controller.tfc import device_info
from host_controller.tfc import request


class TfcClientTest(unittest.TestCase):
    """A test for tfc_client.TfcClient.

    Attributes:
        _client: The tfc_client.TfcClient being tested.
        _service: The mock service that _client connects to.
    """
    _DEVICE_INFOS = [device_info.DeviceInfo(
            device_serial="ABCDEF", group_name="group0",
            run_target="sailfish", state="Available")]

    def setUp(self):
        """Creates a TFC client connecting to a mock service."""
        self._service = mock.Mock()
        self._client = tfc_client.TfcClient(self._service)

    def testNewRequest(self):
        """Tests requests.new."""
        req = request.Request(cluster="cluster0",
                              command_line="vts-codelab",
                              run_target="sailfish",
                              user="user0")
        self._client.NewRequest(req)
        self._service.assert_has_calls([
                mock.call.requests().new().execute()])

    def testLeaseHostTasks(self):
        """Tests tasks.leasehosttasks."""
        tasks = {"tasks": [{"request_id": "2",
                            "command_line": "vts-codelab --serial ABCDEF",
                            "task_id": "1-0",
                            "device_serials": ["ABCDEF"],
                            "command_id": "1"}]}
        self._service.tasks().leasehosttasks().execute.return_value = tasks
        self._client.LeaseHostTasks("cluster0", ["cluster1", "cluster2"],
                                    "host0", self._DEVICE_INFOS)
        self._service.assert_has_calls([
                mock.call.tasks().leasehosttasks().execute()])

    def testHostEvents(self):
        """Tests host_events.submit."""
        device_snapshots = [
                self._client.CreateDeviceSnapshot("vts-staging", "host0",
                                                  self._DEVICE_INFOS),
                self._client.CreateDeviceSnapshot("vts-presubmit", "host0",
                                                  self._DEVICE_INFOS)]
        self._client.SubmitHostEvents(device_snapshots)
        self._service.assert_has_calls([
                mock.call.host_events().submit().execute()])

    def testCommandEvents(self):
        """Tests command_events.submit."""
        cmd = command_attempt.CommandAttempt(
                task_id="321-0",
                attempt_id="abcd-1234",
                hostname="host0",
                device_serial="ABCDEF")
        expected_event = {
                "task_id": "321-0",
                "attempt_id": "abcd-1234",
                "hostname": "host0",
                "device_serial": "ABCDEF",
                "time": 1}

        normal_event = cmd.CreateCommandEvent(
                command_attempt.EventType.INVOCATION_STARTED,
                event_time=1)
        expected_event["type"] = command_attempt.EventType.INVOCATION_STARTED
        self.assertDictEqual(expected_event, normal_event)

        error_event = cmd.CreateCommandEvent(
                command_attempt.EventType.EXECUTE_FAILED,
                error="unit test", event_time=1.1)
        expected_event["type"] = command_attempt.EventType.EXECUTE_FAILED
        expected_event["data"] = {"error":"unit test"}
        self.assertDictEqual(expected_event, error_event)

        complete_event = cmd.CreateInvocationCompletedEvent(
                summary="complete", total_test_count=2, failed_test_count=1,
                error="unit test")
        expected_event["type"] = command_attempt.EventType.INVOCATION_COMPLETED
        expected_event["data"] = {"summary": "complete", "error": "unit test",
                                  "total_test_count": 2, "failed_test_count": 1}
        del expected_event["time"]
        self.assertDictContainsSubset(expected_event, complete_event)
        self.assertIn("time", complete_event)

        self._client.SubmitCommandEvents([
                normal_event, error_event, complete_event])
        self._service.assert_has_calls([
                mock.call.command_events().submit().execute()])

    def testWrongParameter(self):
        """Tests raising exception for wrong parameter name."""
        self.assertRaisesRegexp(KeyError, "sdk", device_info.DeviceInfo,
                                device_serial="123", sdk="25")


if __name__ == "__main__":
    unittest.main()
