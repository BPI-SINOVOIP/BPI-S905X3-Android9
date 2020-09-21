# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import logging
import os

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import global_config
from autotest_lib.server import adb_utils
from autotest_lib.server import constants
from autotest_lib.server.hosts import adb_host

DEFAULT_ACTS_INTERNAL_DIRECTORY = 'tools/test/connectivity/acts'

CONFIG_FOLDER_LOCATION = global_config.global_config.get_config_value(
    'ACTS', 'acts_config_folder', default='')

TEST_DIR_NAME = 'tests'
FRAMEWORK_DIR_NAME = 'framework'
SETUP_FILE_NAME = 'setup.py'
CONFIG_DIR_NAME = 'autotest_config'
CAMPAIGN_DIR_NAME = 'autotest_campaign'
LOG_DIR_NAME = 'logs'
ACTS_EXECUTABLE_IN_FRAMEWORK = 'acts/bin/act.py'

ACTS_TESTPATHS_ENV_KEY = 'ACTS_TESTPATHS'
ACTS_LOGPATH_ENV_KEY = 'ACTS_LOGPATH'
ACTS_PYTHONPATH_ENV_KEY = 'PYTHONPATH'


def create_acts_package_from_current_artifact(test_station, job_repo_url,
                                              target_zip_file):
    """Creates an acts package from the build branch being used.

    Creates an acts artifact from the build branch being used. This is
    determined by the job_repo_url passed in.

    @param test_station: The teststation that should be creating the package.
    @param job_repo_url: The job_repo_url to get the build info from.
    @param target_zip_file: The zip file to create form the artifact on the
                            test_station.

    @returns An ActsPackage containing all the information about the zipped
             artifact.
    """
    build_info = adb_host.ADBHost.get_build_info_from_build_url(job_repo_url)

    return create_acts_package_from_artifact(
        test_station, build_info['branch'], build_info['target'],
        build_info['build_id'], job_repo_url, target_zip_file)


def create_acts_package_from_artifact(test_station, branch, target, build_id,
                                      devserver, target_zip_file):
    """Creates an acts package from a specified branch.

    Grabs the packaged acts artifact from the branch and places it on the
    test_station.

    @param test_station: The teststation that should be creating the package.
    @param branch: The name of the branch where the artifact is to be pulled.
    @param target: The name of the target where the artifact is to be pulled.
    @param build_id: The build id to pull the artifact from.
    @param devserver: The devserver to use.
    @param target_zip_file: The zip file to create on the teststation.

    @returns An ActsPackage containing all the information about the zipped
             artifact.
    """
    devserver.trigger_download(
        target, build_id, branch, files='acts.zip', synchronous=True)

    pull_base_url = devserver.get_pull_url(target, build_id, branch)
    download_ulr = os.path.join(pull_base_url, 'acts.zip')

    test_station.download_file(download_ulr, target_zip_file)

    return ActsPackage(test_station, target_zip_file)


def create_acts_package_from_zip(test_station, zip_location, target_zip_file):
    """Creates an acts package from an existing zip.

    Creates an acts package from a zip file that already sits on the drone.

    @param test_station: The teststation to create the package on.
    @param zip_location: The location of the zip on the drone.
    @param target_zip_file: The zip file to create on the teststaiton.

    @returns An ActsPackage containing all the information about the zipped
             artifact.
    """
    if not os.path.isabs(zip_location):
        zip_location = os.path.join(CONFIG_FOLDER_LOCATION, 'acts_artifacts',
                                    zip_location)

    test_station.send_file(zip_location, target_zip_file)

    return ActsPackage(test_station, target_zip_file)


class ActsPackage(object):
    """A packaged version of acts on a teststation."""

    def __init__(self, test_station, zip_file_path):
        """
        @param test_station: The teststation this package is on.
        @param zip_file_path: The path to the zip file on the test station that
                              holds the package on the teststation.
        """
        self.test_station = test_station
        self.zip_file = zip_file_path

    def create_container(self,
                         container_directory,
                         internal_acts_directory=None):
        """Unpacks this package into a container.

        Unpacks this acts package into a container to interact with acts.

        @param container_directory: The directory on the teststation to hold
                                    the container.
        @param internal_acts_directory: The directory inside of the package
                                        that holds acts.

        @returns: An ActsContainer with info on the unpacked acts container.
        """
        self.test_station.run('unzip "%s" -x -d "%s"' %
                              (self.zip_file, container_directory))

        return ActsContainer(
            self.test_station,
            container_directory,
            acts_directory=internal_acts_directory)

    def create_environment(self,
                           container_directory,
                           devices,
                           testbed_name,
                           internal_acts_directory=None):
        """Unpacks this package into an acts testing enviroment.

        Unpacks this acts package into a test enviroment to test with acts.

        @param container_directory: The directory on the teststation to hold
                                    the test enviroment.
        @param devices: The list of devices in the environment.
        @param testbed_name: The name of the testbed.
        @param internal_acts_directory: The directory inside of the package
                                        that holds acts.

        @returns: An ActsTestingEnvironment with info on the unpacked
                  acts testing environment.
        """
        container = self.create_container(container_directory,
                                          internal_acts_directory)

        return ActsTestingEnviroment(
            devices=devices,
            container=container,
            testbed_name=testbed_name)


class AndroidTestingEnvironment(object):
    """A container for testing android devices on a test station."""

    def __init__(self, devices, testbed_name):
        """Creates a new android testing environment.

        @param devices: The devices on the testbed to use.
        @param testbed_name: The name for the testbed.
        """
        self.devices = devices
        self.testbed_name = testbed_name

    def install_sl4a_apk(self, force_reinstall=True):
        """Install sl4a to all provided devices..

        @param force_reinstall: If true the apk will be force to reinstall.
        """
        for device in self.devices:
            adb_utils.install_apk_from_build(
                device,
                constants.SL4A_APK,
                constants.SL4A_ARTIFACT,
                package_name=constants.SL4A_PACKAGE,
                force_reinstall=force_reinstall)

    def install_apk(self, apk_info, force_reinstall=True):
        """Installs an additional apk on all adb devices.

        @param apk_info: A dictionary containing the apk info. This dictionary
                         should contain the keys:
                            apk="Name of the apk",
                            package="Name of the package".
                            artifact="Name of the artifact", if missing
                                      the package name is used."
        @param force_reinstall: If true the apk will be forced to reinstall.
        """
        for device in self.devices:
            adb_utils.install_apk_from_build(
                device,
                apk_info['apk'],
                apk_info.get('artifact') or constants.SL4A_ARTIFACT,
                package_name=apk_info['package'],
                force_reinstall=force_reinstall)


class ActsContainer(object):
    """A container for working with acts."""

    def __init__(self, test_station, container_directory, acts_directory=None):
        """
        @param test_station: The test station that the container is on.
        @param container_directory: The directory on the teststation this
                                    container operates out of.
        @param acts_directory: The directory within the container that holds
                               acts. If none then it defaults to
                               DEFAULT_ACTS_INTERNAL_DIRECTORY.
        """
        self.test_station = test_station
        self.container_directory = container_directory

        if not acts_directory:
            acts_directory = DEFAULT_ACTS_INTERNAL_DIRECTORY

        if not os.path.isabs(acts_directory):
            self.acts_directory = os.path.join(container_directory,
                                               acts_directory)
        else:
            self.acts_directory = acts_directory

        self.tests_directory = os.path.join(self.acts_directory, TEST_DIR_NAME)
        self.framework_directory = os.path.join(self.acts_directory,
                                                FRAMEWORK_DIR_NAME)

        self.acts_file = os.path.join(self.framework_directory,
                                      ACTS_EXECUTABLE_IN_FRAMEWORK)

        self.setup_file = os.path.join(self.framework_directory,
                                       SETUP_FILE_NAME)

        self.log_directory = os.path.join(container_directory,
                                          LOG_DIR_NAME)

        self.config_location = os.path.join(container_directory,
                                            CONFIG_DIR_NAME)

        self.acts_file = os.path.join(self.framework_directory,
                                      ACTS_EXECUTABLE_IN_FRAMEWORK)

        self.working_directory = os.path.join(container_directory,
                                              CONFIG_DIR_NAME)
        test_station.run('mkdir %s' % self.working_directory,
                         ignore_status=True)

    def get_test_paths(self):
        """Get all test paths within this container.

        Gets all paths that hold tests within the container.

        @returns: A list of paths on the teststation that hold tests.
        """
        get_test_paths_result = self.test_station.run('find %s -type d' %
                                                      self.tests_directory)
        test_search_dirs = get_test_paths_result.stdout.splitlines()
        return test_search_dirs

    def get_python_path(self):
        """Get the python path being used.

        Gets the python path that will be set in the enviroment for this
        container.

        @returns: A string of the PYTHONPATH enviroment variable to be used.
        """
        return '%s:$PYTHONPATH' % self.framework_directory

    def get_enviroment(self):
        """Gets the enviroment variables to be used for this container.

        @returns: A dictionary of enviroment variables to be used by this
                  container.
        """
        env = {
            ACTS_TESTPATHS_ENV_KEY: ':'.join(self.get_test_paths()),
            ACTS_LOGPATH_ENV_KEY: self.log_directory,
            ACTS_PYTHONPATH_ENV_KEY: self.get_python_path()
        }

        return env

    def upload_file(self, src, dst):
        """Uploads a file to be used by the container.

        Uploads a file from the drone to the test staiton to be used by the
        test container.

        @param src: The source file on the drone. If a relative path is given
                    it is assumed to exist in CONFIG_FOLDER_LOCATION.
        @param dst: The destination on the teststation. If a relative path is
                    given it is assumed that it is within the container.

        @returns: The full path on the teststation.
        """
        if not os.path.isabs(src):
            src = os.path.join(CONFIG_FOLDER_LOCATION, src)

        if not os.path.isabs(dst):
            dst = os.path.join(self.container_directory, dst)

        path = os.path.dirname(dst)
        self.test_station.run('mkdir "%s"' % path, ignore_status=True)

        original_dst = dst
        if os.path.basename(src) == os.path.basename(dst):
            dst = os.path.dirname(dst)

        self.test_station.send_file(src, dst)

        return original_dst


class ActsTestingEnviroment(AndroidTestingEnvironment):
    """A container for running acts tests with a contained version of acts."""

    def __init__(self, container, devices, testbed_name):
        """
        @param container: The acts container to use.
        @param devices: The list of devices to use.
        @testbed_name: The name of the testbed being used.
        """
        super(ActsTestingEnviroment, self).__init__(devices=devices,
                                                    testbed_name=testbed_name)

        self.container = container

        self.configs = {}
        self.campaigns = {}

    def upload_config(self, config_file):
        """Uploads a config file to the container.

        Uploads a config file to the config folder in the container.

        @param config_file: The config file to upload. This must be a file
                            within the autotest_config directory under the
                            CONFIG_FOLDER_LOCATION.

        @returns: The full path of the config on the test staiton.
        """
        full_name = os.path.join(CONFIG_DIR_NAME, config_file)

        full_path = self.container.upload_file(full_name, full_name)
        self.configs[config_file] = full_path

        return full_path

    def upload_campaign(self, campaign_file):
        """Uploads a campaign file to the container.

        Uploads a campaign file to the campaign folder in the container.

        @param campaign_file: The campaign file to upload. This must be a file
                              within the autotest_campaign directory under the
                              CONFIG_FOLDER_LOCATION.

        @returns: The full path of the campaign on the test staiton.
        """
        full_name = os.path.join(CAMPAIGN_DIR_NAME, campaign_file)

        full_path = self.container.upload_file(full_name, full_name)
        self.campaigns[campaign_file] = full_path

        return full_path

    def setup_enviroment(self, python_bin='python'):
        """Sets up the teststation system enviroment so the container can run.

        Prepares the remote system so that the container can run. This involves
        uninstalling all versions of acts for the version of python being
        used and installing all needed dependencies.

        @param python_bin: The python binary to use.
        """
        uninstall_command = '%s %s uninstall' % (
            python_bin, self.container.setup_file)
        install_deps_command = '%s %s install_deps' % (
            python_bin, self.container.setup_file)

        self.container.test_station.run(uninstall_command)
        self.container.test_station.run(install_deps_command)

    def run_test(self,
                 config,
                 campaign=None,
                 test_case=None,
                 extra_env={},
                 python_bin='python',
                 timeout=7200,
                 additional_cmd_line_params=None):
        """Runs a test within the container.

        Runs a test within a container using the given settings.

        @param config: The name of the config file to use as the main config.
                       This should have already been uploaded with
                       upload_config. The string passed into upload_config
                       should be used here.
        @param campaign: The campaign file to use for this test. If none then
                         test_case is assumed. This file should have already
                         been uploaded with upload_campaign. The string passed
                         into upload_campaign should be used here.
        @param test_case: The test case to run the test with. If none then the
                          campaign will be used. If multiple are given,
                          multiple will be run.
        @param extra_env: Extra enviroment variables to run the test with.
        @param python_bin: The python binary to execute the test with.
        @param timeout: How many seconds to wait before timing out.
        @param additional_cmd_line_params: Adds the ability to add any string
                                           to the end of the acts.py command
                                           line string.  This is intended to
                                           add acts command line flags however
                                           this is unbounded so it could cause
                                           errors if incorrectly set.

        @returns: The results of the test run.
        """
        if not config in self.configs:
            # Check if the config has been uploaded and upload if it hasn't
            self.upload_config(config)

        full_config = self.configs[config]

        if campaign:
            # When given a campaign check if it's upload.
            if not campaign in self.campaigns:
                self.upload_campaign(campaign)

            full_campaign = self.campaigns[campaign]
        else:
            full_campaign = None

        full_env = self.container.get_enviroment()

        # Setup environment variables.
        if extra_env:
            for k, v in extra_env.items():
                full_env[k] = extra_env

        logging.info('Using env: %s', full_env)
        exports = ('export %s=%s' % (k, v) for k, v in full_env.items())
        env_command = ';'.join(exports)

        # Make sure to execute in the working directory.
        command_setup = 'cd %s' % self.container.working_directory

        if additional_cmd_line_params:
            act_base_cmd = '%s %s -c %s -tb %s %s ' % (
                    python_bin, self.container.acts_file, full_config,
                    self.testbed_name, additional_cmd_line_params)
        else:
            act_base_cmd = '%s %s -c %s -tb %s ' % (
                    python_bin, self.container.acts_file, full_config,
                    self.testbed_name)

        # Format the acts command based on what type of test is being run.
        if test_case and campaign:
            raise error.TestError(
                    'campaign and test_file cannot both have a value.')
        elif test_case:
            if isinstance(test_case, str):
                test_case = [test_case]
            if len(test_case) < 1:
                raise error.TestError('At least one test case must be given.')

            tc_str = ''
            for tc in test_case:
                tc_str = '%s %s' % (tc_str, tc)
            tc_str = tc_str.strip()

            act_cmd = '%s -tc %s' % (act_base_cmd, tc_str)
        elif campaign:
            act_cmd = '%s -tf %s' % (act_base_cmd, full_campaign)
        else:
            raise error.TestFail('No tests was specified!')

        # Format all commands into a single command.
        command_list = [command_setup, env_command, act_cmd]
        full_command = '; '.join(command_list)

        try:
            # Run acts on the remote machine.
            act_result = self.container.test_station.run(full_command,
                                                         timeout=timeout)
            excep = None
        except Exception as e:
            # Catch any error to store in the results.
            act_result = None
            excep = e

        return ActsTestResults(str(test_case) or campaign,
                               container=self.container,
                               devices=self.devices,
                               testbed_name=self.testbed_name,
                               run_result=act_result,
                               exception=excep)


class ActsTestResults(object):
    """The packaged results of a test run."""
    acts_result_to_autotest = {
        'PASS': 'GOOD',
        'FAIL': 'FAIL',
        'UNKNOWN': 'WARN',
        'SKIP': 'ABORT'
    }

    def __init__(self,
                 name,
                 container,
                 devices,
                 testbed_name,
                 run_result=None,
                 exception=None):
        """
        @param name: A name to identify the test run.
        @param testbed_name: The name the testbed was run with, if none the
                             default name of the testbed is used.
        @param run_result: The raw i/o result of the test run.
        @param log_directory: The directory that acts logged to.
        @param exception: An exception that was thrown while running the test.
        """
        self.name = name
        self.run_result = run_result
        self.exception = exception
        self.log_directory = container.log_directory
        self.test_station = container.test_station
        self.testbed_name = testbed_name
        self.devices = devices

        self.reported_to = set()

        self.json_results = {}
        self.results_dir = None
        if self.log_directory:
            self.results_dir = os.path.join(self.log_directory,
                                            self.testbed_name, 'latest')
            results_file = os.path.join(self.results_dir,
                                        'test_run_summary.json')
            cat_log_result = self.test_station.run('cat %s' % results_file,
                                                   ignore_status=True)
            if not cat_log_result.exit_status:
                self.json_results = json.loads(cat_log_result.stdout)

    def log_output(self):
        """Logs the output of the test."""
        if self.run_result:
            logging.debug('ACTS Output:\n%s', self.run_result.stdout)

    def save_test_info(self, test):
        """Save info about the test.

        @param test: The test to save.
        """
        for device in self.devices:
            device.save_info(test.resultsdir)

    def rethrow_exception(self):
        """Re-throws the exception thrown during the test."""
        if self.exception:
            raise self.exception

    def upload_to_local(self, local_dir):
        """Saves all acts results to a local directory.

        @param local_dir: The directory on the local machine to save all results
                          to.
        """
        if self.results_dir:
            self.test_station.get_file(self.results_dir, local_dir)

    def report_to_autotest(self, test):
        """Reports the results to an autotest test object.

        Reports the results to the test and saves all acts results under the
        tests results directory.

        @param test: The autotest test object to report to. If this test object
                     has already recived our report then this call will be
                     ignored.
        """
        if test in self.reported_to:
            return

        if self.results_dir:
            self.upload_to_local(test.resultsdir)

        if not 'Results' in self.json_results:
            return

        results = self.json_results['Results']
        for result in results:
            verdict = self.acts_result_to_autotest[result['Result']]
            details = result['Details']
            test.job.record(verdict, None, self.name, status=(details or ''))

        self.reported_to.add(test)
