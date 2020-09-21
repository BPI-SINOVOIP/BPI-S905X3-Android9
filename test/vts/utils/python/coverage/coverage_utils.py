#
# Copyright (C) 2016 The Android Open Source Project
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
import argparse
import io
import json
import logging
import os
import shutil
import sys
import time
import zipfile

from vts.proto import VtsReportMessage_pb2 as ReportMsg
from vts.runners.host import keys
from vts.utils.python.archive import archive_parser
from vts.utils.python.common import cmd_utils
from vts.utils.python.controllers.adb import AdbError
from vts.utils.python.coverage import coverage_report
from vts.utils.python.coverage import gcda_parser
from vts.utils.python.coverage import gcno_parser
from vts.utils.python.coverage.parser import FileFormatError
from vts.utils.python.os import path_utils
from vts.utils.python.web import feature_utils

FLUSH_PATH_VAR = "GCOV_PREFIX"  # environment variable for gcov flush path
TARGET_COVERAGE_PATH = "/data/misc/trace/"  # location to flush coverage
LOCAL_COVERAGE_PATH = "/tmp/vts-test-coverage"  # location to pull coverage to host

# Environment for test process
COVERAGE_TEST_ENV = "GCOV_PREFIX_OVERRIDE=true GCOV_PREFIX=/data/misc/trace/self"

GCNO_SUFFIX = ".gcno"
GCDA_SUFFIX = ".gcda"
COVERAGE_SUFFIX = ".gcnodir"
GIT_PROJECT = "git_project"
MODULE_NAME = "module_name"
NAME = "name"
PATH = "path"
GEN_TAG = "/gen/"

_BUILD_INFO = "BUILD_INFO"  # name of build info artifact
_GCOV_ZIP = "gcov.zip"  # name of gcov artifact zip
_REPO_DICT = "repo-dict"  # name of dictionary from project to revision in BUILD_INFO

_CLEAN_TRACE_COMMAND = "rm -rf /data/misc/trace/*"
_FLUSH_COMMAND = (
    "GCOV_PREFIX_OVERRIDE=true GCOV_PREFIX=/data/local/tmp/flusher "
    "/data/local/tmp/vts_coverage_configure flush")
_SP_COVERAGE_PATH = "self"  # relative location where same-process coverage is dumped.

_CHECKSUM_GCNO_DICT = "checksum_gcno_dict"
_COVERAGE_ZIP = "coverage_zip"
_REVISION_DICT = "revision_dict"


class CoverageFeature(feature_utils.Feature):
    """Feature object for coverage functionality.

    Attributes:
        enabled: boolean, True if coverage is enabled, False otherwise
        web: (optional) WebFeature, object storing web feature util for test run
        local_coverage_path: path to store the coverage files.
        _device_resource_dict: a map from device serial number to host resources directory.
        _hal_names: the list of hal names for which to process coverage.
        _coverage_report_file_prefix: prefix of the output coverage report file.
    """

    _TOGGLE_PARAM = keys.ConfigKeys.IKEY_ENABLE_COVERAGE
    _REQUIRED_PARAMS = [keys.ConfigKeys.IKEY_ANDROID_DEVICE]
    _OPTIONAL_PARAMS = [
        keys.ConfigKeys.IKEY_MODULES,
        keys.ConfigKeys.IKEY_OUTPUT_COVERAGE_REPORT,
        keys.ConfigKeys.IKEY_GLOBAL_COVERAGE,
        keys.ConfigKeys.IKEY_EXCLUDE_COVERAGE_PATH,
        keys.ConfigKeys.IKEY_COVERAGE_REPORT_PATH,
    ]

    _DEFAULT_EXCLUDE_PATHS = [
        "bionic", "external/libcxx", "system/core", "system/libhidl",
        "system/libfmq"
    ]

    def __init__(self, user_params, web=None):
        """Initializes the coverage feature.

        Args:
            user_params: A dictionary from parameter name (String) to parameter value.
            web: (optional) WebFeature, object storing web feature util for test run
            local_coverage_path: (optional) path to store the .gcda files and coverage reports.
        """
        self.ParseParameters(self._TOGGLE_PARAM, self._REQUIRED_PARAMS,
                             self._OPTIONAL_PARAMS, user_params)
        self.web = web
        self._device_resource_dict = {}
        self._hal_names = None

        timestamp_seconds = str(int(time.time() * 1000000))
        self.local_coverage_path = os.path.join(LOCAL_COVERAGE_PATH,
                                                timestamp_seconds)
        if os.path.exists(self.local_coverage_path):
            logging.info("removing existing coverage path: %s",
                         self.local_coverage_path)
            shutil.rmtree(self.local_coverage_path)
        os.makedirs(self.local_coverage_path)

        self._coverage_report_dir = getattr(
            self, keys.ConfigKeys.IKEY_COVERAGE_REPORT_PATH, None)

        self._coverage_report_file_prefix = ""

        self.global_coverage = getattr(
            self, keys.ConfigKeys.IKEY_GLOBAL_COVERAGE, True)
        if self.enabled:
            android_devices = getattr(self,
                                      keys.ConfigKeys.IKEY_ANDROID_DEVICE)
            if not isinstance(android_devices, list):
                logging.warn("Android device information not available.")
                self.enabled = False
            for device in android_devices:
                serial = device.get(keys.ConfigKeys.IKEY_SERIAL)
                coverage_resource_path = device.get(
                    keys.ConfigKeys.IKEY_GCOV_RESOURCES_PATH)
                if not serial:
                    logging.error("Missing serial information in device: %s",
                                  device)
                    continue
                if not coverage_resource_path:
                    logging.error("Missing coverage resource path in device: %s",
                                  device)
                    continue
                self._device_resource_dict[str(serial)] = str(
                    coverage_resource_path)
        logging.info("Coverage enabled: %s", self.enabled)

    def _FindGcnoSummary(self, gcda_file_path, gcno_file_parsers):
        """Find the corresponding gcno summary for given gcda file.

        Identify the corresponding gcno summary for given gcda file from a list
        of gcno files with the same checksum as the gcda file by matching
        the the gcda file path.
        Note: if none of the gcno summary contains the source file same as the
        given gcda_file_path (e.g. when the corresponding source file does not
        contain any executable codes), just return the last gcno summary in the
        list as a fall back solution.

        Args:
            gcda_file_path: the path of gcda file (without extensions).
            gcno_file_parsers: a list of gcno file parser that has the same
                               chechsum.

        Returns:
            The corresponding gcno summary for given gcda file.
        """
        gcno_summary = None
        # For each gcno files with the matched checksum, compare the
        # gcda_file_path to find the corresponding gcno summary.
        for gcno_file_parser in gcno_file_parsers:
            try:
                gcno_summary = gcno_file_parser.Parse()
            except FileFormatError:
                logging.error("Error parsing gcno for gcda %s", gcda_file_path)
                break
            legacy_build = "soong/.intermediates" not in gcda_file_path
            for key in gcno_summary.functions:
                src_file_path = gcno_summary.functions[key].src_file_name
                src_file_name = src_file_path.rsplit(".", 1)[0]
                # If build with legacy compile system, compare only the base
                # source file name. Otherwise, compare the full source file name
                # (with path info).
                if legacy_build:
                    base_src_file_name = os.path.basename(src_file_name)
                    if gcda_file_path.endswith(base_src_file_name):
                        return gcno_summary
                else:
                    if gcda_file_path.endswith(src_file_name):
                        return gcno_summary
        # If no gcno file matched with the gcda_file_name, return the last
        # gcno summary as a fall back solution.
        return gcno_summary

    def _GetChecksumGcnoDict(self, cov_zip):
        """Generates a dictionary from gcno checksum to GCNOParser object.

        Processes the gcnodir files in the zip file to produce a mapping from gcno
        checksum to the GCNOParser object wrapping the gcno content.
        Note there might be multiple gcno files corresponds to the same checksum.

        Args:
            cov_zip: the zip file containing gcnodir files from the device build

        Returns:
            the dictionary of gcno checksums to GCNOParser objects
        """
        checksum_gcno_dict = dict()
        fnames = cov_zip.namelist()
        instrumented_modules = [
            f for f in fnames if f.endswith(COVERAGE_SUFFIX)
        ]
        for instrumented_module in instrumented_modules:
            # Read the gcnodir file
            archive = archive_parser.Archive(
                cov_zip.open(instrumented_module).read())
            try:
                archive.Parse()
            except ValueError:
                logging.error("Archive could not be parsed: %s", name)
                continue

            for gcno_file_path in archive.files:
                gcno_stream = io.BytesIO(archive.files[gcno_file_path])
                gcno_file_parser = gcno_parser.GCNOParser(gcno_stream)
                if gcno_file_parser.checksum in checksum_gcno_dict:
                    checksum_gcno_dict[gcno_file_parser.checksum].append(
                        gcno_file_parser)
                else:
                    checksum_gcno_dict[gcno_file_parser.checksum] = [
                        gcno_file_parser
                    ]
        return checksum_gcno_dict

    def _ClearTargetGcov(self, dut, serial, path_suffix=None):
        """Removes gcov data from the device.

        Finds and removes all gcda files relative to TARGET_COVERAGE_PATH.
        Args:
            dut: the device under test.
            path_suffix: optional string path suffix.
        """
        path = TARGET_COVERAGE_PATH
        if path_suffix:
            path = path_utils.JoinTargetPath(path, path_suffix)
        self._ExecuteOneAdbShellCommand(dut, serial, _CLEAN_TRACE_COMMAND)

    def InitializeDeviceCoverage(self, dut=None, serial=None):
        """Initializes the device for coverage before tests run.

        Flushes, then finds and removes all gcda files under
        TARGET_COVERAGE_PATH before tests run.

        Args:
            dut: the device under test.
        """
        self._ExecuteOneAdbShellCommand(dut, serial, _FLUSH_COMMAND)
        logging.info("Removing existing gcda files.")
        self._ClearTargetGcov(dut, serial)

    def _GetGcdaDict(self, dut, serial):
        """Retrieves GCDA files from device and creates a dictionary of files.

        Find all GCDA files on the target device, copy them to the host using
        adb, then return a dictionary mapping from the gcda basename to the
        temp location on the host.

        Args:
            dut: the device under test.

        Returns:
            A dictionary with gcda basenames as keys and contents as the values.
        """
        logging.info("Creating gcda dictionary")
        gcda_dict = {}
        logging.info("Storing gcda tmp files to: %s", self.local_coverage_path)

        self._ExecuteOneAdbShellCommand(dut, serial, _FLUSH_COMMAND)

        gcda_files = set()
        if self._hal_names:
            searchString = "|".join(self._hal_names)
            entries = []
            try:
                entries = dut.adb.shell(
                    "lshal -itp 2> /dev/null | grep -E \"{0}\"".format(
                        searchString)).splitlines()
            except AdbError as e:
                logging.error("failed to get pid entries")

            pids = set(pid.strip()
                       for pid in map(lambda entry: entry.split()[-1], entries)
                       if pid.isdigit())
            pids.add(_SP_COVERAGE_PATH)
            for pid in pids:
                path = path_utils.JoinTargetPath(TARGET_COVERAGE_PATH, pid)
                try:
                    files = dut.adb.shell("find %s -name \"*.gcda\"" % path)
                    gcda_files.update(files.split("\n"))
                except AdbError as e:
                    logging.info("No gcda files found in path: \"%s\"", path)
        else:
            cmd = ("find %s -name \"*.gcda\"" % TARGET_COVERAGE_PATH)
            result = self._ExecuteOneAdbShellCommand(dut, serial, cmd)
            if result:
                gcda_files.update(result.split("\n"))

        for gcda in gcda_files:
            if gcda:
                basename = os.path.basename(gcda.strip())
                file_name = os.path.join(self.local_coverage_path, basename)
                if dut is None:
                    results = cmd_utils.ExecuteShellCommand(
                        "adb -s %s pull %s %s " % (serial, gcda, file_name))
                    if (results[cmd_utils.EXIT_CODE][0]):
                        logging.error(
                            "Fail to execute command: %s. error: %s" %
                            (cmd, str(results[cmd_utils.STDERR][0])))
                else:
                    dut.adb.pull("%s %s" % (gcda, file_name))
                gcda_content = open(file_name, "rb").read()
                gcda_dict[gcda.strip()] = gcda_content
        self._ClearTargetGcov(dut, serial)
        return gcda_dict

    def _OutputCoverageReport(self, isGlobal, coverage_report_msg=None):
        logging.info("outputing coverage data")
        timestamp_seconds = str(int(time.time() * 1000000))
        coverage_report_file_name = "coverage_report_" + timestamp_seconds + ".txt"
        if self._coverage_report_file_prefix:
            coverage_report_file_name = "coverage_report_" + self._coverage_report_file_prefix + ".txt"

        coverage_report_file = None
        if (self._coverage_report_dir):
            if not os.path.exists(self._coverage_report_dir):
                os.makedirs(self._coverage_report_dir)
            coverage_report_file = os.path.join(self._coverage_report_dir,
                                            coverage_report_file_name)
        else:
            coverage_report_file = os.path.join(self.local_coverage_path,
                                            coverage_report_file_name)

        logging.info("Storing coverage report to: %s", coverage_report_file)
        if self.web and self.web.enabled:
            coverage_report_msg = ReportMsg.TestReportMessage()
            if isGlobal:
                for c in self.web.report_msg.coverage:
                    coverage = coverage_report_msg.coverage.add()
                    coverage.CopyFrom(c)
            else:
                for c in self.web.current_test_report_msg.coverage:
                    coverage = coverage_report_msg.coverage.add()
                    coverage.CopyFrom(c)
        if coverage_report_msg is not None:
            with open(coverage_report_file, "w+") as f:
                f.write(str(coverage_report_msg))

    def _AutoProcess(self, cov_zip, revision_dict, gcda_dict, isGlobal):
        """Process coverage data and appends coverage reports to the report message.

        Matches gcno files with gcda files and processes them into a coverage report
        with references to the original source code used to build the system image.
        Coverage information is appended as a CoverageReportMessage to the provided
        report message.

        Git project information is automatically extracted from the build info and
        the source file name enclosed in each gcno file. Git project names must
        resemble paths and may differ from the paths to their project root by at
        most one. If no match is found, then coverage information will not be
        be processed.

        e.g. if the project path is test/vts, then its project name may be
             test/vts or <some folder>/test/vts in order to be recognized.

        Args:
            cov_zip: the ZipFile object containing the gcno coverage artifacts.
            revision_dict: the dictionary from project name to project version.
            gcda_dict: the dictionary of gcda basenames to gcda content (binary string)
            isGlobal: boolean, True if the coverage data is for the entire test, False if only for
                      the current test case.
        """
        checksum_gcno_dict = self._GetChecksumGcnoDict(cov_zip)
        output_coverage_report = getattr(
            self, keys.ConfigKeys.IKEY_OUTPUT_COVERAGE_REPORT, False)
        exclude_coverage_path = getattr(
            self, keys.ConfigKeys.IKEY_EXCLUDE_COVERAGE_PATH, [])
        for idx, path in enumerate(exclude_coverage_path):
            base_name = os.path.basename(path)
            if base_name and "." not in base_name:
                path = path if path.endswith("/") else path + "/"
                exclude_coverage_path[idx] = path
        exclude_coverage_path.extend(self._DEFAULT_EXCLUDE_PATHS)

        coverage_dict = dict()
        coverage_report_message = ReportMsg.TestReportMessage()

        for gcda_name in gcda_dict:
            if GEN_TAG in gcda_name:
                # skip coverage measurement for intermediate code.
                logging.warn("Skip for gcda file: %s", gcda_name)
                continue

            gcda_stream = io.BytesIO(gcda_dict[gcda_name])
            gcda_file_parser = gcda_parser.GCDAParser(gcda_stream)
            file_name = gcda_name.rsplit(".", 1)[0]

            if not gcda_file_parser.checksum in checksum_gcno_dict:
                logging.info("No matching gcno file for gcda: %s", gcda_name)
                continue
            gcno_file_parsers = checksum_gcno_dict[gcda_file_parser.checksum]
            gcno_summary = self._FindGcnoSummary(file_name, gcno_file_parsers)
            if gcno_summary is None:
                logging.error("No gcno file found for gcda %s.", gcda_name)
                continue

            # Process and merge gcno/gcda data
            try:
                gcda_file_parser.Parse(gcno_summary)
            except FileFormatError:
                logging.error("Error parsing gcda file %s", gcda_name)
                continue

            coverage_report.GenerateLineCoverageVector(
                gcno_summary, exclude_coverage_path, coverage_dict)

        for src_file_path in coverage_dict:
            # Get the git project information
            # Assumes that the project name and path to the project root are similar
            revision = None
            for project_name in revision_dict:
                # Matches cases when source file root and project name are the same
                if src_file_path.startswith(str(project_name)):
                    git_project_name = str(project_name)
                    git_project_path = str(project_name)
                    revision = str(revision_dict[project_name])
                    logging.info("Source file '%s' matched with project '%s'",
                                 src_file_path, git_project_name)
                    break

                parts = os.path.normpath(str(project_name)).split(os.sep, 1)
                # Matches when project name has an additional prefix before the
                # project path root.
                if len(parts) > 1 and src_file_path.startswith(parts[-1]):
                    git_project_name = str(project_name)
                    git_project_path = parts[-1]
                    revision = str(revision_dict[project_name])
                    logging.info("Source file '%s' matched with project '%s'",
                                 src_file_path, git_project_name)
                    break

            if not revision:
                logging.info("Could not find git info for %s", src_file_path)
                continue

            coverage_vec = coverage_dict[src_file_path]
            total_count, covered_count = coverage_report.GetCoverageStats(
                coverage_vec)
            if self.web and self.web.enabled:
                self.web.AddCoverageReport(coverage_vec, src_file_path,
                                           git_project_name, git_project_path,
                                           revision, covered_count,
                                           total_count, isGlobal)
            else:
                coverage = coverage_report_message.coverage.add()
                coverage.total_line_count = total_count
                coverage.covered_line_count = covered_count
                coverage.line_coverage_vector.extend(coverage_vec)

                src_file_path = os.path.relpath(src_file_path,
                                                git_project_path)
                coverage.file_path = src_file_path
                coverage.revision = revision
                coverage.project_name = git_project_name

        if output_coverage_report:
            self._OutputCoverageReport(isGlobal, coverage_report_message)

    # TODO: consider to deprecate the manual process.
    def _ManualProcess(self, cov_zip, revision_dict, gcda_dict, isGlobal):
        """Process coverage data and appends coverage reports to the report message.

        Opens the gcno files in the cov_zip for the specified modules and matches
        gcno/gcda files. Then, coverage vectors are generated for each set of matching
        gcno/gcda files and appended as a CoverageReportMessage to the provided
        report message. Unlike AutoProcess, coverage information is only processed
        for the modules explicitly defined in 'modules'.

        Args:
            cov_zip: the ZipFile object containing the gcno coverage artifacts.
            revision_dict: the dictionary from project name to project version.
            gcda_dict: the dictionary of gcda basenames to gcda content (binary string)
            isGlobal: boolean, True if the coverage data is for the entire test, False if only for
                      the current test case.
        """
        output_coverage_report = getattr(
            self, keys.ConfigKeys.IKEY_OUTPUT_COVERAGE_REPORT, True)
        modules = getattr(self, keys.ConfigKeys.IKEY_MODULES, None)
        covered_modules = set(cov_zip.namelist())
        for module in modules:
            if MODULE_NAME not in module or GIT_PROJECT not in module:
                logging.error(
                    "Coverage module must specify name and git project: %s",
                    module)
                continue
            project = module[GIT_PROJECT]
            if PATH not in project or NAME not in project:
                logging.error("Project name and path not specified: %s",
                              project)
                continue

            name = str(module[MODULE_NAME]) + COVERAGE_SUFFIX
            git_project = str(project[NAME])
            git_project_path = str(project[PATH])

            if name not in covered_modules:
                logging.error("No coverage information for module %s", name)
                continue
            if git_project not in revision_dict:
                logging.error(
                    "Git project not present in device revision dict: %s",
                    git_project)
                continue

            revision = str(revision_dict[git_project])
            archive = archive_parser.Archive(cov_zip.open(name).read())
            try:
                archive.Parse()
            except ValueError:
                logging.error("Archive could not be parsed: %s", name)
                continue

            for gcno_file_path in archive.files:
                file_name_path = gcno_file_path.rsplit(".", 1)[0]
                file_name = os.path.basename(file_name_path)
                gcno_content = archive.files[gcno_file_path]
                gcno_stream = io.BytesIO(gcno_content)
                try:
                    gcno_summary = gcno_parser.GCNOParser(gcno_stream).Parse()
                except FileFormatError:
                    logging.error("Error parsing gcno file %s", gcno_file_path)
                    continue
                src_file_path = None

                # Match gcno file with gcda file
                gcda_name = file_name + GCDA_SUFFIX
                if gcda_name not in gcda_dict:
                    logging.error("No gcda file found %s.", gcda_name)
                    continue

                src_file_path = self._ExtractSourceName(
                    gcno_summary, file_name)

                if not src_file_path:
                    logging.error("No source file found for %s.",
                                  gcno_file_path)
                    continue

                # Process and merge gcno/gcda data
                gcda_content = gcda_dict[gcda_name]
                gcda_stream = io.BytesIO(gcda_content)
                try:
                    gcda_parser.GCDAParser(gcda_stream).Parse(gcno_summary)
                except FileFormatError:
                    logging.error("Error parsing gcda file %s", gcda_content)
                    continue

                if self.web and self.web.enabled:
                    coverage_vec = coverage_report.GenerateLineCoverageVector(
                        src_file_path, gcno_summary)
                    total_count, covered_count = coverage_report.GetCoverageStats(
                        coverage_vec)
                    self.web.AddCoverageReport(coverage_vec, src_file_path,
                                               git_project, git_project_path,
                                               revision, covered_count,
                                               total_count, isGlobal)

        if output_coverage_report:
            self._OutputCoverageReport(isGlobal)

    def SetCoverageData(self, dut=None, serial=None, isGlobal=False):
        """Sets and processes coverage data.

        Organizes coverage data and processes it into a coverage report in the
        current test case

        Requires feature to be enabled; no-op otherwise.

        Args:
            dut:  the device object for which to pull coverage data
            isGlobal: True if the coverage data is for the entire test, False if
                      if the coverage data is just for the current test case.
        """
        if not self.enabled:
            return

        if serial is None:
            serial = "default" if dut is None else dut.adb.shell(
                "getprop ro.serialno").strip()

        if not serial in self._device_resource_dict:
            logging.error("Invalid device provided: %s", serial)
            return

        resource_path = self._device_resource_dict[serial]
        if not resource_path:
            logging.error("coverage resource path not found.")
            return

        gcda_dict = self._GetGcdaDict(dut, serial)
        logging.info("coverage file paths %s", str([fp for fp in gcda_dict]))

        cov_zip = zipfile.ZipFile(os.path.join(resource_path, _GCOV_ZIP))

        revision_dict = json.load(
            open(os.path.join(resource_path, _BUILD_INFO)))[_REPO_DICT]

        if not hasattr(self, keys.ConfigKeys.IKEY_MODULES):
            # auto-process coverage data
            self._AutoProcess(cov_zip, revision_dict, gcda_dict, isGlobal)
        else:
            # explicitly process coverage data for the specified modules
            self._ManualProcess(cov_zip, revision_dict, gcda_dict, isGlobal)

        # cleanup the downloaded gcda files.
        logging.info("cleanup gcda files.")
        files = os.listdir(self.local_coverage_path)
        for item in files:
            if item.endswith(".gcda"):
                os.remove(os.path.join(self.local_coverage_path, item))

    def SetHalNames(self, names=[]):
        """Sets the HAL names for which to process coverage.

        Args:
            names: list of strings, names of hal (e.g. android.hardware.light@2.0)
        """
        self._hal_names = list(names)

    def SetCoverageReportFilePrefix(self, prefix):
        """Sets the prefix for outputting the coverage report file.

        Args:
            prefix: strings, prefix of the coverage report file.
        """
        self._coverage_report_file_prefix = prefix

    def SetCoverageReportDirectory(self, corverage_report_dir):
        """Sets the path for storing the coverage report file.

        Args:
            corverage_report_dir: strings, dir to store the coverage report file.
        """
        self._coverage_report_dir = corverage_report_dir

    def _ExecuteOneAdbShellCommand(self, dut, serial, cmd):
        """Helper method to execute a shell command and return results.

        Args:
            dut: the device under test.
            cmd: string, command to execute.
        Returns:
            stdout result of the command, None if command fails.
        """
        if dut is None:
            results = cmd_utils.ExecuteShellCommand("adb -s %s shell %s" %
                                                    (serial, cmd))
            if (results[cmd_utils.EXIT_CODE][0]):
                logging.error("Fail to execute command: %s. error: %s" %
                              (cmd, str(results[cmd_utils.STDERR][0])))
                return None
            else:
                return results[cmd_utils.STDOUT][0]
        else:
            try:
                return dut.adb.shell(cmd)
            except AdbError as e:
                logging.warn("Fail to execute command: %s. error: %s" %
                             (cmd, str(e)))
                return None


if __name__ == '__main__':
    """ Tools to process coverage data.

    Usage:
      python coverage_utils.py operation [--serial=device_serial_number]
      [--report_prefix=prefix_of_coverage_report]

    Example:
      python coverage_utils.py init_coverage
      python coverage_utils.py get_coverage --serial HT7821A00243
      python coverage_utils.py get_coverage --serial HT7821A00243 --report_prefix=test
    """
    logging.basicConfig(level=logging.INFO)
    parser = argparse.ArgumentParser(description="Coverage process tool.")
    parser.add_argument(
        "--report_prefix",
        dest="report_prefix",
        required=False,
        help="Prefix of the coverage report.")
    parser.add_argument(
        "--report_path",
        dest="report_path",
        required=False,
        help="directory to store the coverage reports.")
    parser.add_argument(
        "--serial", dest="serial", required=True, help="Device serial number.")
    parser.add_argument(
        "--gcov_rescource_path",
        dest="gcov_rescource_path",
        required=True,
        help="Directory that stores gcov resource files.")
    parser.add_argument(
        "operation",
        help=
        "Operation for processing coverage data, e.g. 'init_coverage', get_coverage'"
    )
    args = parser.parse_args()

    if args.operation != "init_coverage" and args.operation != "get_coverage":
        print "Unsupported operation. Exiting..."
        sys.exit(1)
    user_params = {
        keys.ConfigKeys.IKEY_ENABLE_COVERAGE:
        True,
        keys.ConfigKeys.IKEY_ANDROID_DEVICE: [{
            keys.ConfigKeys.IKEY_SERIAL:args.serial,
            keys.ConfigKeys.IKEY_GCOV_RESOURCES_PATH:args.gcov_rescource_path,
        }],
        keys.ConfigKeys.IKEY_OUTPUT_COVERAGE_REPORT:
        True,
        keys.ConfigKeys.IKEY_GLOBAL_COVERAGE:
        True
    }
    coverage = CoverageFeature(user_params)
    if args.operation == "init_coverage":
        coverage.InitializeDeviceCoverage(serial=args.serial)
    elif args.operation == "get_coverage":
        if args.report_prefix:
            coverage.SetCoverageReportFilePrefix(args.report_prefix)
        if args.report_path:
            coverage.SetCoverageReportDirectory(args.report_path)
        coverage.SetCoverageData(serial=args.serial, isGlobal=True)
