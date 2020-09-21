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

import os
import unittest

try:
    from unittest import mock
except ImportError:
    import mock

try:
    import StringIO as string_io_module
except ImportError:
    import io as string_io_module

from host_controller.build import build_flasher
from host_controller.tfc import command_task
from host_controller.tfc import device_info
from host_controller import console


class ConsoleTest(unittest.TestCase):
    """A test for console.Console.

    Attribute:
        _out_file: The console output buffer.
        _host_controller: A mock tfc_host_controller.HostController.
        _build_provider_pab: A mock build_provider_pab.BuildProviderPAB.
        _tfc_client: A mock tfc_client.TfcClient.
        _vti_client A mock vti_endpoint_client.VtiEndpointClient.
        _console: The console being tested.
    """
    _DEVICES = [
        device_info.DeviceInfo(
            device_serial="ABC001",
            run_target="sailfish",
            state="Available",
            build_id="111111",
            sdk_version="27")
    ]
    _TASKS = [
        command_task.CommandTask(
            request_id="1",
            task_id="1-0",
            command_id="2",
            command_line="vts -m SampleShellTest",
            device_serials=["ABC001"])
    ]

    def setUp(self):
        """Creates the console."""
        self._out_file = string_io_module.StringIO()
        self._host_controller = mock.Mock()
        self._build_provider_pab = mock.Mock()
        self._tfc_client = mock.Mock()
        self._vti_client = mock.Mock()
        self._console = console.Console(
            self._vti_client, self._tfc_client, self._build_provider_pab,
            [self._host_controller],
            None, self._out_file)

    def tearDown(self):
        """Closes the output file."""
        self._out_file.close()

    def _IssueCommand(self, command_line):
        """Issues a command in the console.

        Args:
            command_line: A string, the input to the console.

        Returns:
            A string, the output of the console.
        """
        out_position = self._out_file.tell()
        self._console.onecmd(command_line)
        self._out_file.seek(out_position)
        return self._out_file.read()

    def testLease(self):
        """Tests the lease command."""
        self._host_controller.LeaseCommandTasks.return_value = self._TASKS
        output = self._IssueCommand("lease")
        expected = (
            "request_id  command_id  task_id  device_serials  command_line          \n"
            "1           2           1-0      ABC001          vts -m SampleShellTest\n"
        )
        self.assertEqual(expected, output)
        output = self._IssueCommand("lease --host 0")
        self.assertEqual(expected, output)

    def testRequest(self):
        """Tests the request command."""
        user = "user0"
        cluster = "cluster0"
        run_target = "sailfish"
        command_line = "vts -m SampleShellTest"
        self._IssueCommand("request --user %s --cluster %s --run-target %s "
                           "-- %s" % (user, cluster, run_target, command_line))
        req = self._tfc_client.NewRequest.call_args[0][0]
        self.assertEqual(user, req.user)
        self.assertEqual(cluster, req.cluster)
        self.assertEqual(run_target, req.run_target)
        self.assertEqual(command_line, req.command_line)

    def testListHosts(self):
        """Tests the list command."""
        self._host_controller.hostname = "host0"
        output = self._IssueCommand("list hosts")
        self.assertEqual("index  name\n" "[  0]  host0\n", output)

    def testListDevices(self):
        """Tests the list command."""
        self._host_controller.ListDevices.return_value = self._DEVICES
        self._host_controller.hostname = "host0"
        output = self._IssueCommand("list devices")
        expected = (
            "[  0]  host0\n"
            "device_serial  state      run_target  build_id  sdk_version  stub\n"
            "ABC001         Available  sailfish    111111    27               \n"
        )
        self.assertEqual(expected, output)
        output = self._IssueCommand("list devices --host 0")
        self.assertEqual(expected, output)

    def testWrongHostIndex(self):
        """Tests host index out of range."""
        output = self._IssueCommand("list devices --host 1")
        expected = "IndexError: "
        self.assertTrue(output.startswith(expected))
        output = self._IssueCommand("lease --host 1")
        self.assertTrue(output.startswith(expected))

    @mock.patch('host_controller.console.build_flasher.BuildFlasher')
    def testFetchPOSTAndFlash(self, mock_class):
        """Tests fetching from pab and flashing."""
        self._build_provider_pab.GetArtifact.return_value = ({
            "system.img":
            "/mock/system.img",
            "odm.img":
            "/mock/odm.img"
        }, {}, {"build_id": "build_id"}, {})
        self._IssueCommand(
            "fetch --branch=aosp-master-ndk --target=darwin_mac "
            "--account_id=100621237 "
            "--artifact_name=foo-{id}.tar.bz2 --method=POST")
        self._build_provider_pab.GetArtifact.assert_called_with(
            account_id='100621237',
            branch='aosp-master-ndk',
            target='darwin_mac',
            artifact_name='foo-{id}.tar.bz2',
            build_id='latest',
            method='POST')

        flasher = mock.Mock()
        mock_class.return_value = flasher
        self._IssueCommand("flash --current system=system.img odm=odm.img")
        flasher.Flash.assert_called_with({
            "system": "/mock/system.img",
            "odm": "/mock/odm.img"
        })

    def testFetchAndEnvironment(self):
        """Tests fetching from pab and check stored os environment"""
        build_id_return = "4328532"
        target_return = "darwin_mac"
        expected_fetch_info = {"build_id": build_id_return}

        self._build_provider_pab.GetArtifact.return_value = ({
            "system.img":
            "/mock/system.img",
            "odm.img":
            "/mock/odm.img"
        }, {}, expected_fetch_info, {})
        self._IssueCommand("fetch --branch=aosp-master-ndk --target=%s "
                           "--account_id=100621237 "
                           "--artifact_name=foo-{id}.tar.bz2 --method=POST"
                           % target_return)
        self._build_provider_pab.GetArtifact.assert_called_with(
            account_id='100621237',
            branch='aosp-master-ndk',
            target='darwin_mac',
            artifact_name='foo-{id}.tar.bz2',
            build_id='latest',
            method='POST')

        expected = expected_fetch_info["build_id"]
        self.assertEqual(build_id_return, expected)

    @mock.patch('host_controller.console.build_flasher.BuildFlasher')
    def testFlashGSI(self, mock_class):
        flasher = mock.Mock()
        mock_class.return_value = flasher
        self._IssueCommand("flash --gsi=system.img")
        flasher.FlashGSI.assert_called_with('system.img', None)

    @mock.patch('host_controller.console.build_flasher.BuildFlasher')
    def testFlashGSIWithVbmeta(self, mock_class):
        flasher = mock.Mock()
        mock_class.return_value = flasher
        self._IssueCommand("flash --gsi=system.img --vbmeta=vbmeta.img")
        flasher.FlashGSI.assert_called_with('system.img', 'vbmeta.img')

    @mock.patch('host_controller.console.build_flasher.BuildFlasher')
    def testFlashall(self, mock_class):
        flasher = mock.Mock()
        mock_class.return_value = flasher
        self._IssueCommand("flash --build_dir=path/to/dir/")
        flasher.Flashall.assert_called_with('path/to/dir/')

    @mock.patch('host_controller.console.importlib')
    @mock.patch('host_controller.console.issubclass')
    def testImportFlasher(self, mock_issubclass, mock_importlib):
        mock_issubclass.return_value = True
        flasher_module = mock.Mock()
        flasher = mock.Mock()
        mock_importlib.import_module.return_value = flasher_module
        flasher_module.Flasher.return_value = flasher
        self._IssueCommand("flash --serial ABC001 "
                           "--flasher_type test.flasher.Flasher "
                           "--flasher_path /test/flasher "
                           "-- --unit test")
        mock_issubclass.assert_called_once_with(
            flasher_module.Flasher, build_flasher.BuildFlasher)
        mock_importlib.import_module.assert_called_with("test.flasher")
        flasher_module.Flasher.assert_called_with("ABC001", "/test/flasher")
        flasher.Flash.assert_called_with({}, {}, "--unit", "test")
        flasher.WaitForDevice.assert_called_with()


if __name__ == "__main__":
    unittest.main()
