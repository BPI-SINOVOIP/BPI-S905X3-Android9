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

import cmd
import datetime
import imp  # Python v2 compatibility
import multiprocessing
import multiprocessing.pool
import os
import re
import signal
import socket
import sys
import threading
import time
import urlparse

from host_controller import common
from host_controller.command_processor import command_build
from host_controller.command_processor import command_config
from host_controller.command_processor import command_copy
from host_controller.command_processor import command_device
from host_controller.command_processor import command_exit
from host_controller.command_processor import command_fetch
from host_controller.command_processor import command_flash
from host_controller.command_processor import command_gsispl
from host_controller.command_processor import command_info
from host_controller.command_processor import command_lease
from host_controller.command_processor import command_list
from host_controller.command_processor import command_retry
from host_controller.command_processor import command_request
from host_controller.command_processor import command_test
from host_controller.command_processor import command_upload
from host_controller.build import build_provider_ab
from host_controller.build import build_provider_gcs
from host_controller.build import build_provider_local_fs
from host_controller.build import build_provider_pab
from host_controller.utils.ipc import shared_dict
from host_controller.vti_interface import vti_endpoint_client

COMMAND_PROCESSORS = [
    command_build.CommandBuild,
    command_config.CommandConfig,
    command_copy.CommandCopy,
    command_device.CommandDevice,
    command_exit.CommandExit,
    command_fetch.CommandFetch,
    command_flash.CommandFlash,
    command_gsispl.CommandGsispl,
    command_info.CommandInfo,
    command_lease.CommandLease,
    command_list.CommandList,
    command_retry.CommandRetry,
    command_request.CommandRequest,
    command_test.CommandTest,
    command_upload.CommandUpload,
]


class NonDaemonizedProcess(multiprocessing.Process):
    """Process class which is not daemonized."""

    def _get_daemon(self):
        return False

    def _set_daemon(self, value):
        pass

    daemon = property(_get_daemon, _set_daemon)


class NonDaemonizedPool(multiprocessing.pool.Pool):
    """Pool class which is not daemonized."""

    Process = NonDaemonizedProcess


def JobMain(vti_address, in_queue, out_queue, device_status):
    """Main() for a child process that executes a leased job.

    Currently, lease jobs must use VTI (not TFC).

    Args:
        vti_client: VtiEndpointClient needed to create Console.
        in_queue: Queue to get new jobs.
        out_queue: Queue to put execution results.
        device_status: SharedDict, contains device status information.
                       shared between processes.
    """
    if not vti_address:
        print("vti address is not set. example : $ run --vti=<url>")
        return

    def SigTermHandler(signum, frame):
        """Signal handler for exiting pool process explicitly.

        Added to resolve orphaned pool process issue.
        """
        sys.exit(0)

    signal.signal(signal.SIGTERM, SigTermHandler)

    vti_client = vti_endpoint_client.VtiEndpointClient(vti_address)
    console = Console(
        vti_client,
        None,
        build_provider_pab.BuildProviderPAB(),
        None,
        job_pool=True)
    console.device_status = device_status
    multiprocessing.util.Finalize(console, console.__exit__, exitpriority=0)

    while True:
        command = in_queue.get()
        if command == "exit":
            break
        elif command == "lease":
            filepath, kwargs = vti_client.LeaseJob(socket.gethostname(), True)
            print("Job %s -> %s" % (os.getpid(), kwargs))
            if filepath is not None:
                # TODO: redirect console output and add
                # console command to access them.

                for serial in kwargs["serial"]:
                    console.device_status[serial] = common._DEVICE_STATUS_DICT[
                        "use"]
                print_to_console = True
                if not print_to_console:
                    sys.stdout = out
                    sys.stderr = err

                ret = console.ProcessConfigurableScript(
                    os.path.join(os.getcwd(), "host_controller", "campaigns",
                                 filepath), **kwargs)
                if ret:
                    job_status = "complete"
                else:
                    job_status = "infra-err"

                vti_client.StopHeartbeat(job_status)
                print("Job execution complete. "
                      "Setting job status to {}".format(job_status))

                if not print_to_console:
                    sys.stdout = sys.__stdout__
                    sys.stderr = sys.__stderr__

                for serial in kwargs["serial"]:
                    console.device_status[serial] = common._DEVICE_STATUS_DICT[
                        "ready"]
        else:
            print("Unknown job command %s" % command)


class Console(cmd.Cmd):
    """The console for host controllers.

    Attributes:
        command_processors: dict of string:BaseCommandProcessor,
                            map between command string and command processors.
        device_image_info: dict containing info about device image files.
        prompt: The prompt string at the beginning of each command line.
        test_result: dict containing info about the last test result.
        test_suite_info: dict containing info about test suite package files.
        tools_info: dict containing info about custom tool files.
        scheduler_thread: dict containing threading.Thread instances(s) that
                          update configs regularly.
        _build_provider_pab: The BuildProviderPAB used to download artifacts.
        _vti_address: string, VTI service URI.
        _vti_client: VtiEndpoewrClient, used to upload data to a test
                     scheduling infrastructure.
        _tfc_client: The TfcClient that the host controllers connect to.
        _hosts: A list of HostController objects.
        _in_file: The input file object.
        _out_file: The output file object.
        _serials: A list of string where each string is a device serial.
        _device_status: SharedDict, shared with process pool.
                        contains status data on each devices.
        _job_pool: bool, True if Console is created from job pool process
                   context.
    """

    def __init__(self,
                 vti_endpoint_client,
                 tfc,
                 pab,
                 host_controllers,
                 vti_address=None,
                 in_file=sys.stdin,
                 out_file=sys.stdout,
                 job_pool=False):
        """Initializes the attributes and the parsers."""
        # cmd.Cmd is old-style class.
        cmd.Cmd.__init__(self, stdin=in_file, stdout=out_file)
        self._build_provider = {}
        self._build_provider["pab"] = pab
        self._job_pool = job_pool
        if not self._job_pool:
            self._build_provider[
                "local_fs"] = build_provider_local_fs.BuildProviderLocalFS()
            self._build_provider["gcs"] = build_provider_gcs.BuildProviderGCS()
            self._build_provider["ab"] = build_provider_ab.BuildProviderAB()
        self._vti_endpoint_client = vti_endpoint_client
        self._vti_address = vti_address
        self._tfc_client = tfc
        self._hosts = host_controllers
        self._in_file = in_file
        self._out_file = out_file
        self.prompt = "> "
        self.command_processors = {}
        self.device_image_info = {}
        self.test_result = {}
        self.test_suite_info = {}
        self.tools_info = {}
        self.fetch_info = {}
        self.test_results = {}
        self._device_status = shared_dict.SharedDict()

        if common._ANDROID_SERIAL in os.environ:
            self._serials = [os.environ[common._ANDROID_SERIAL]]
        else:
            self._serials = []

        self.InitCommandModuleParsers()
        self.SetUpCommandProcessors()

    def __exit__(self):
        """Finalizes the build provider attributes explicitly when exited."""
        for bp in self._build_provider:
            self._build_provider[bp].__del__()

    @property
    def device_status(self):
        """getter for self._device_status"""
        return self._device_status

    @device_status.setter
    def device_status(self, device_status):
        """setter for self._device_status"""
        self._device_status = device_status

    def InitCommandModuleParsers(self):
        """Init all console command modules"""
        for name in dir(self):
            if name.startswith('_Init') and name.endswith('Parser'):
                attr_func = getattr(self, name)
                if hasattr(attr_func, '__call__'):
                    attr_func()

    def SetUpCommandProcessors(self):
        """Sets up all command processors."""
        for command_processor in COMMAND_PROCESSORS:
            cp = command_processor()
            cp._SetUp(self)
            do_text = "do_%s" % cp.command
            help_text = "help_%s" % cp.command
            setattr(self, do_text, cp._Run)
            setattr(self, help_text, cp._Help)
            self.command_processors[cp.command] = cp

    def TearDown(self):
        """Removes all command processors."""
        for command_processor in self.command_processors.itervalues():
            command_processor._TearDown()
        self.command_processors.clear()

    def FormatString(self, format_string):
        """Replaces variables with the values in the console's dictionaries.

        Args:
            format_string: The string containing variables enclosed in {}.

        Returns:
            The formatted string.

        Raises:
            KeyError if a variable is not found in the dictionaries or the
            value is empty.
        """

        def ReplaceVariable(match):
            name = match.group(1)
            if name in ("build_id", "branch", "target"):
                value = self.fetch_info[name]
            elif name in ("result_zip", "suite_plan"):
                value = self.test_result[name]
            elif name in ("timestamp"):
                value = datetime.datetime.now().strftime("%Y%m%d-%H%M%S")
            else:
                value = None

            if not value:
                raise KeyError(name)

            return value

        return re.sub("{([^}]+)}", ReplaceVariable, format_string)

    def ProcessScript(self, script_file_path):
        """Processes a .py script file.

        A script file implements a function which emits a list of console
        commands to execute. That function emits an empty list or None if
        no more command needs to be processed.

        Args:
            script_file_path: string, the path of a script file (.py file).

        Returns:
            True if successful; False otherwise
        """
        if not script_file_path.endswith(".py"):
            print("Script file is not .py file: %s" % script_file_path)
            return False

        script_module = imp.load_source('script_module', script_file_path)

        commands = script_module.EmitConsoleCommands()
        if commands:
            for command in commands:
                ret = self.onecmd(command)
                if ret == False:
                    return False
        return True

    def ProcessConfigurableScript(self, script_file_path, **kwargs):
        """Processes a .py script file.

        A script file implements a function which emits a list of console
        commands to execute. That function emits an empty list or None if
        no more command needs to be processed.

        Args:
            script_file_path: string, the path of a script file (.py file).
            kwargs: extra args for the interface function defined in
                    the script file.

        Returns:
            True if successful; False otherwise
        """
        if script_file_path and "." not in script_file_path:
            script_file_path += ".py"

        if not script_file_path.endswith(".py"):
            print("Script file is not .py file: %s" % script_file_path)
            return False

        script_module = imp.load_source('script_module', script_file_path)

        commands = script_module.EmitConsoleCommands(**kwargs)
        if commands:
            for command in commands:
                ret = self.onecmd(command)
                if ret == False:
                    return False
        else:
            return False
        return True

    def _Print(self, string):
        """Prints a string and a new line character.

        Args:
            string: The string to be printed.
        """
        self._out_file.write(string + "\n")

    def _PrintObjects(self, objects, attr_names):
        """Shows objects as a table.

        Args:
            object: The objects to be shown, one object in a row.
            attr_names: The attributes to be shown, one attribute in a column.
        """
        width = [len(name) for name in attr_names]
        rows = [attr_names]
        for dev_info in objects:
            attrs = [
                _ToPrintString(getattr(dev_info, name, ""))
                for name in attr_names
            ]
            rows.append(attrs)
            for index, attr in enumerate(attrs):
                width[index] = max(width[index], len(attr))

        for row in rows:
            self._Print("  ".join(
                attr.ljust(width[index]) for index, attr in enumerate(row)))

    def DownloadTestResources(self, request_id):
        """Download all of the test resources for a TFC request id.

        Args:
            request_id: int, TFC request id
        """
        resources = self._tfc_client.TestResourceList(request_id)
        for resource in resources:
            self.DownloadTestResource(resource['url'])

    def DownloadTestResource(self, url):
        """Download a test resource with build provider, given a url.

        Args:
            url: a resource locator (not necessarily HTTP[s])
                with the scheme specifying the build provider.
        """
        parsed = urlparse.urlparse(url)
        path = (parsed.netloc + parsed.path).split('/')
        if parsed.scheme == "pab":
            if len(path) != 5:
                print("Invalid pab resource locator: %s" % url)
                return
            account_id, branch, target, build_id, artifact_name = path
            cmd = ("fetch"
                   " --type=pab"
                   " --account_id=%s"
                   " --branch=%s"
                   " --target=%s"
                   " --build_id=%s"
                   " --artifact_name=%s") % (account_id, branch, target,
                                             build_id, artifact_name)
            self.onecmd(cmd)
        elif parsed.scheme == "ab":
            if len(path) != 4:
                print("Invalid ab resource locator: %s" % url)
                return
            branch, target, build_id, artifact_name = path
            cmd = ("fetch"
                   "--type=ab"
                   " --branch=%s"
                   " --target=%s"
                   " --build_id=%s"
                   " --artifact_name=%s") % (branch, target, build_id,
                                             artifact_name)
            self.onecmd(cmd)
        elif parsed.scheme == gcs:
            cmd = "fetch --type=gcs --path=%s" % url
            self.onecmd(cmd)
        else:
            print "Invalid URL: %s" % url

    def SetSerials(self, serials):
        """Sets the default serial numbers for flashing and testing.

        Args:
            serials: A list of strings, the serial numbers.
        """
        self._serials = serials

    def GetSerials(self):
        """Returns the serial numbers saved in the console.

        Returns:
            A list of strings, the serial numbers.
        """
        return self._serials

    def JobThread(self):
        """Job thread which monitors and uploads results."""
        thread = threading.currentThread()
        while getattr(thread, "keep_running", True):
            time.sleep(1)

        if self._job_pool:
            self._job_pool.close()
            self._job_pool.terminate()
            self._job_pool.join()

    def StartJobThreadAndProcessPool(self):
        """Starts a background thread to control leased jobs."""
        self._job_in_queue = multiprocessing.Queue()
        self._job_out_queue = multiprocessing.Queue()
        self._job_pool = NonDaemonizedPool(
            common._MAX_LEASED_JOBS, JobMain,
            (self._vti_address, self._job_in_queue, self._job_out_queue,
             self._device_status))

        self._job_thread = threading.Thread(target=self.JobThread)
        self._job_thread.daemon = True
        self._job_thread.start()

    def StopJobThreadAndProcessPool(self):
        """Terminates the thread and processes that runs the leased job."""
        if hasattr(self, "_job_thread"):
            self._job_thread.keep_running = False
            self._job_thread.join()

    # @Override
    def onecmd(self, line, depth=1, ret_out_queue=None):
        """Executes command(s) and prints any exception.

        Parallel execution only for 2nd-level list element.

        Args:
            line: a list of string or string which keeps the command to run.
        """
        if not line:
            return

        if type(line) == list:
            if depth == 1:  # 1 to use multi-threading
                jobs = []
                ret_queue = multiprocessing.Queue()
                for sub_command in line:
                    p = multiprocessing.Process(
                        target=self.onecmd,
                        args=(
                            sub_command,
                            depth + 1,
                            ret_queue,
                        ))
                    jobs.append(p)
                    p.start()
                for job in jobs:
                    job.join()

                ret_cmd_list = True
                while not ret_queue.empty():
                    ret_from_subprocess = ret_queue.get()
                    ret_cmd_list = ret_cmd_list and ret_from_subprocess
                if ret_cmd_list == False:
                    return False
            else:
                for sub_command in line:
                    ret_cmd_list = self.onecmd(sub_command, depth + 1)
                    if ret_cmd_list == False and ret_out_queue:
                        ret_out_queue.put(False)
                        return False
            return

        print("Command: %s" % line)
        try:
            ret_cmd = cmd.Cmd.onecmd(self, line)
            if ret_cmd == False and ret_out_queue:
                ret_out_queue.put(ret_cmd)
            return ret_cmd
        except Exception as e:
            self._Print("%s: %s" % (type(e).__name__, e))
            if ret_out_queue:
                ret_out_queue.put(False)
            return False

    # @Override
    def emptyline(self):
        """Ignores empty lines."""
        pass

    # @Override
    def default(self, line):
        """Handles unrecognized commands.

        Returns:
            True if receives EOF; otherwise delegates to default handler.
        """
        if line == "EOF":
            return self.do_exit(line)
        return cmd.Cmd.default(self, line)


def _ToPrintString(obj):
    """Converts an object to printable string on console.

    Args:
        obj: The object to be printed.
    """
    if isinstance(obj, (list, tuple, set)):
        return ",".join(str(x) for x in obj)
    return str(obj)
