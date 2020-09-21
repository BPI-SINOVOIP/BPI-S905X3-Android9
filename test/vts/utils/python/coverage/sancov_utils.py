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
import json
import logging
import os
import shutil
import subprocess
import tempfile
import zipfile

from vts.runners.host import keys
from vts.utils.python.web import feature_utils
from vts.utils.python.controllers.adb import AdbError
from vts.utils.python.coverage import sancov_parser


class SancovFeature(feature_utils.Feature):
    """Feature object for sanitizer coverage functionality.

    Attributes:
        enabled: boolean, True if sancov is enabled, False otherwise
        web: (optional) WebFeature, object storing web feature util for test run
    """
    _DEFAULT_EXCLUDE_PATHS = [
        'bionic', 'external/libcxx', 'system/core', 'system/libhidl'
    ]
    _TOGGLE_PARAM = keys.ConfigKeys.IKEY_ENABLE_SANCOV
    _REQUIRED_PARAMS = [keys.ConfigKeys.IKEY_ANDROID_DEVICE]

    _PROCESS_INIT_COMMAND = (
        '\"echo coverage=1 > /data/asan/system/asan.options.{0} && '
        'echo coverage_dir={1}/{2} >> /data/asan/system/asan.options.{0} && '
        'rm -rf {1}/{2} &&'
        'mkdir {1}/{2} && '
        'killall {0}\"')
    _FLUSH_COMMAND = '/data/local/tmp/vts_coverage_configure flush {0}'
    _TARGET_SANCOV_PATH = '/data/misc/trace'
    _SEARCH_PATHS = [
        (os.path.join('data', 'asan', 'vendor', 'bin'), None),
        (os.path.join('vendor', 'bin'), None),
        (os.path.join('data', 'asan', 'vendor', 'lib'), 32),
        (os.path.join('vendor', 'lib'), 32),
        (os.path.join('data', 'asan', 'vendor', 'lib64'), 64),
        (os.path.join('vendor', 'lib64'), 64)
    ]

    _BUILD_INFO = 'BUILD_INFO'
    _REPO_DICT = 'repo-dict'
    _SYMBOLS_ZIP = 'symbols.zip'

    def __init__(self,
                 user_params,
                 web=None,
                 exclude_paths=_DEFAULT_EXCLUDE_PATHS):
        """Initializes the sanitizer coverage feature.

        Args:
            user_params: A dictionary from parameter name (String) to parameter value.
            web: (optional) WebFeature, object storing web feature util for test run.
            exclude_paths: (optional) list of strings, paths to exclude for coverage.
        """
        self.ParseParameters(
            self._TOGGLE_PARAM, self._REQUIRED_PARAMS, user_params=user_params)
        self.web = web
        self._device_resource_dict = {}
        self._file_vectors = {}
        self._exclude_paths = exclude_paths
        if self.enabled:
            android_devices = getattr(self,
                                      keys.ConfigKeys.IKEY_ANDROID_DEVICE)
            if not isinstance(android_devices, list):
                logging.warn('Android device information not available')
                self.enabled = False
            for device in android_devices:
                serial = str(device.get(keys.ConfigKeys.IKEY_SERIAL))
                sancov_resource_path = str(
                    device.get(keys.ConfigKeys.IKEY_SANCOV_RESOURCES_PATH))
                if not serial or not sancov_resource_path:
                    logging.warn('Missing sancov information in device: %s',
                                 device)
                    continue
                self._device_resource_dict[serial] = sancov_resource_path
        logging.info('Sancov enabled: %s', self.enabled)

    def InitializeDeviceCoverage(self, dut, hals):
        """Initializes the sanitizer coverage on the device for the provided HAL.

        Args:
            dut: The device under test.
            hals: A list of the HAL name and version (string) for which to
                  measure coverage (e.g. ['android.hardware.light@2.0'])
        """
        serial = dut.adb.shell('getprop ro.serialno').strip()
        if serial not in self._device_resource_dict:
            logging.error("Invalid device provided: %s", serial)
            return


        for hal in hals:
            entries = dut.adb.shell('lshal -itp 2> /dev/null | grep {0}'.format(
                hal)).splitlines()
            pids = set([pid.strip()
                        for pid in map(lambda entry: entry.split()[-1], entries)
                        if pid.isdigit()])

            if len(pids) == 0:
                logging.warn('No matching processes IDs found for HAL %s',
                             hal)
                return
            processes = dut.adb.shell('ps -p {0} -o comm='.format(' '.join(
                pids))).splitlines()
            process_names = set(
                [name.strip() for name in processes
                 if name.strip() and not name.endswith(' (deleted)')])

            if len(process_names) == 0:
                logging.warn('No matching processes names found for HAL %s',
                             hal)
                return

            for process_name in process_names:
                cmd = self._PROCESS_INIT_COMMAND.format(
                    process_name, self._TARGET_SANCOV_PATH, hal)
                try:
                    dut.adb.shell(cmd.format(process_name))
                except AdbError as e:
                    logging.error('Command failed: \"%s\"', cmd)
                    continue

    def FlushDeviceCoverage(self, dut, hals):
        """Flushes the sanitizer coverage on the device for the provided HAL.

        Args:
            dut: The device under test.
            hals: A list of HAL name and version (string) for which to flush
                  coverage (e.g. ['android.hardware.light@2.0-service'])
        """
        serial = dut.adb.shell('getprop ro.serialno').strip()
        if serial not in self._device_resource_dict:
            logging.error('Invalid device provided: %s', serial)
            return
        for hal in hals:
            dut.adb.shell(
                self._FLUSH_COMMAND.format(hal))

    def _InitializeFileVectors(self, serial, binary_path):
        """Parse the binary and read the debugging information.

        Parse the debugging information in the binary to determine executable lines
        of code for all of the files included in the binary.

        Args:
            serial: The serial of the device under test.
            binary_path: The path to the unstripped binary on the host.
        """
        file_vectors = self._file_vectors[serial]
        args = ['readelf', '--debug-dump=decodedline', binary_path]
        with tempfile.TemporaryFile('w+b') as tmp:
            subprocess.call(args, stdout=tmp)
            tmp.seek(0)
            file = None
            for entry in tmp:
                entry_parts = entry.split()
                if len(entry_parts) == 0:
                    continue
                elif len(entry_parts) < 3 and entry_parts[-1].endswith(':'):
                    file = entry_parts[-1].rsplit(':')[0]
                    for path in self._exclude_paths:
                        if file.startswith(path):
                            file = None
                            break
                    continue
                elif len(entry_parts) == 3 and file is not None:
                    line_no_string = entry_parts[1]
                    try:
                        line = int(line_no_string)
                    except ValueError:
                        continue
                    if file not in file_vectors:
                        file_vectors[file] = [-1] * line
                    if line > len(file_vectors[file]):
                        file_vectors[file].extend([-2] * (
                            line - len(file_vectors[file])))
                    file_vectors[file][line - 1] = 0

    def _UpdateLineCounts(self, serial, lines):
        """Update the line counts with the symbolized output lines.

        Increment the line counts using the symbolized line information.

        Args:
            serial: The serial of the device under test.
            lines: A list of strings in the format returned by addr2line (e.g. <file>:<line no>).
        """
        file_vectors = self._file_vectors[serial]
        for line in lines:
            file, line_no_string = line.rsplit(':', 1)
            if file == '??':  # some lines cannot be symbolized and will report as '??'
                continue
            try:
                line_no = int(line_no_string)
            except ValueError:
                continue  # some lines cannot be symbolized and will report as '??'
            if not file in file_vectors:  # file is excluded
                continue
            if line_no > len(file_vectors[file]):
                file_vectors[file].extend([-1] *
                                          (line_no - len(file_vectors[file])))
            if file_vectors[file][line_no - 1] < 0:
                file_vectors[file][line_no - 1] = 0
            file_vectors[file][line_no - 1] += 1

    def Upload(self):
        """Append the coverage information to the web proto report.
        """
        if not self.web or not self.web.enabled:
            return

        for device_serial in self._device_resource_dict:
            resource_path = self._device_resource_dict[device_serial]
            rev_map = json.load(
                open(os.path.join(resource_path, self._BUILD_INFO)))[
                    self._REPO_DICT]

            for file in self._file_vectors[device_serial]:

                # Get the git project information
                # Assumes that the project name and path to the project root are similar
                revision = None
                for project_name in rev_map:
                    # Matches when source file root and project name are the same
                    if file.startswith(str(project_name)):
                        git_project_name = str(project_name)
                        git_project_path = str(project_name)
                        revision = str(rev_map[project_name])
                        break

                    parts = os.path.normpath(str(project_name)).split(os.sep,
                                                                      1)
                    # Matches when project name has an additional prefix before the project path root.
                    if len(parts) > 1 and file.startswith(parts[-1]):
                        git_project_name = str(project_name)
                        git_project_path = parts[-1]
                        revision = str(rev_map[project_name])
                        break

                if not revision:
                    logging.info("Could not find git info for %s", file)
                    continue

                covered_count = sum(
                    map(lambda count: 1 if count > 0 else 0,
                        self._file_vectors[device_serial][file]))
                total_count = sum(
                    map(lambda count: 1 if count >= 0 else 0,
                        self._file_vectors[device_serial][file]))
                self.web.AddCoverageReport(
                    self._file_vectors[device_serial][file], file,
                    git_project_name, git_project_path, revision,
                    covered_count, total_count, True)

    def ProcessDeviceCoverage(self, dut, hals):
        """Process device coverage.

        Fetch sancov files from the target, parse the sancov files, symbolize the output,
        and update the line counters.

        Args:
            dut: The device under test.
            hals: A list of HAL name and version (string) for which to process
                  coverage (e.g. ['android.hardware.light@2.0'])
        """
        serial = dut.adb.shell('getprop ro.serialno').strip()
        product = dut.adb.shell('getprop ro.build.product').strip()

        if not serial in self._device_resource_dict:
            logging.error('Invalid device provided: %s', serial)
            return

        if serial not in self._file_vectors:
            self._file_vectors[serial] = {}

        symbols_zip = zipfile.ZipFile(
            os.path.join(self._device_resource_dict[serial],
                         self._SYMBOLS_ZIP))


        sancov_files = []
        for hal in hals:
            sancov_files.extend(dut.adb.shell('find {0}/{1} -name \"*.sancov\"'.format(
                self._TARGET_SANCOV_PATH, hal)).splitlines())
        temp_dir = tempfile.mkdtemp()

        binary_to_sancov = {}
        for file in sancov_files:
            dut.adb.pull(file, temp_dir)
            binary, pid, _ = os.path.basename(file).rsplit('.', 2)
            bitness, offsets = sancov_parser.ParseSancovFile(
                os.path.join(temp_dir, os.path.basename(file)))
            binary_to_sancov[binary] = (bitness, offsets)

        for hal in hals:
            dut.adb.shell('rm -rf {0}/{1}'.format(self._TARGET_SANCOV_PATH,
                                                  hal))

        search_root = os.path.join('out', 'target', 'product', product,
                                   'symbols')
        for path, bitness in self._SEARCH_PATHS:
            for name in [f for f in symbols_zip.namelist()
                         if f.startswith(os.path.join(search_root, path))]:
                basename = os.path.basename(name)
                if basename in binary_to_sancov and (
                        bitness is None or
                        binary_to_sancov[basename][0] == bitness):
                    with symbols_zip.open(
                            name) as source, tempfile.NamedTemporaryFile(
                                'w+b') as target:
                        shutil.copyfileobj(source, target)
                        target.seek(0)
                        self._InitializeFileVectors(serial, target.name)
                        addrs = map(lambda addr: '{0:#x}'.format(addr),
                                    binary_to_sancov[basename][1])
                        args = ['addr2line', '-pe', target.name]
                        args.extend(addrs)
                        with tempfile.TemporaryFile('w+b') as tmp:
                            subprocess.call(args, stdout=tmp)
                            tmp.seek(0)
                            c = tmp.read().split()
                            self._UpdateLineCounts(serial, c)
                        del binary_to_sancov[basename]
        shutil.rmtree(temp_dir)
