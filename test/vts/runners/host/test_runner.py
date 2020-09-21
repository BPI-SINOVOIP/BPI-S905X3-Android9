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
#

from future import standard_library
standard_library.install_aliases()

import copy
import importlib
import inspect
import logging
import os
import pkgutil
import signal
import sys
try:
    import thread
except ImportError as e:
    import _thread as thread
import threading

from vts.runners.host import base_test
from vts.runners.host import config_parser
from vts.runners.host import keys
from vts.runners.host import logger
from vts.runners.host import records
from vts.runners.host import signals
from vts.runners.host import utils


def main():
    """Execute the test class in a test module.

    This is to be used in a test script's main so the script can be executed
    directly. It will discover all the classes that inherit from BaseTestClass
    and excute them. all the test results will be aggregated into one.

    A VTS host-driven test case has three args:
       1st arg: the path of a test case config file.
       2nd arg: the serial ID of a target device (device config).
       3rd arg: the path of a test case data dir.

    Returns:
        The TestResult object that holds the results of the test run.
    """
    test_classes = []
    main_module_members = sys.modules["__main__"]
    for _, module_member in main_module_members.__dict__.items():
        if inspect.isclass(module_member):
            if issubclass(module_member, base_test.BaseTestClass):
                test_classes.append(module_member)
    # TODO(angli): Need to handle the case where more than one test class is in
    # a test script. The challenge is to handle multiple configs and how to do
    # default config in this case.
    if len(test_classes) != 1:
        logging.error("Expected 1 test class per file, found %s (%s).",
                      len(test_classes), test_classes)
        sys.exit(1)
    test_result = runTestClass(test_classes[0])
    return test_result


def runTestClass(test_class):
    """Execute one test class.

    This will create a TestRunner, execute one test run with one test class.

    Args:
        test_class: The test class to instantiate and execute.

    Returns:
        The TestResult object that holds the results of the test run.
    """
    test_cls_name = test_class.__name__
    if len(sys.argv) < 2:
        logging.warning("Missing a configuration file. Using the default.")
        test_configs = [config_parser.GetDefaultConfig(test_cls_name)]
    else:
        try:
            config_path = sys.argv[1]
            baseline_config = config_parser.GetDefaultConfig(test_cls_name)
            baseline_config[keys.ConfigKeys.KEY_TESTBED] = [
                baseline_config[keys.ConfigKeys.KEY_TESTBED]
            ]
            test_configs = config_parser.load_test_config_file(
                config_path, baseline_config=baseline_config)
        except IndexError:
            logging.error("No valid config file found.")
            sys.exit(1)
        except Exception as e:
            logging.error("Unexpected exception")
            logging.exception(e)

    test_identifiers = [(test_cls_name, None)]

    for config in test_configs:
        if keys.ConfigKeys.KEY_TEST_MAX_TIMEOUT in config:
            timeout_sec = int(config[
                keys.ConfigKeys.KEY_TEST_MAX_TIMEOUT]) / 1000.0
        else:
            timeout_sec = 60 * 60 * 3
            logging.warning("%s unspecified. Set timeout to %s seconds.",
                            keys.ConfigKeys.KEY_TEST_MAX_TIMEOUT, timeout_sec)

        watcher_enabled = threading.Event()

        def watchStdin():
            while True:
                line = sys.stdin.readline()
                if not line:
                    break
            watcher_enabled.wait()
            logging.info("Attempt to interrupt runner thread.")
            if not utils.is_on_windows():
                # Default SIGINT handler sends KeyboardInterrupt to main thread
                # and unblocks it.
                os.kill(os.getpid(), signal.SIGINT)
            else:
                # On Windows, raising CTRL_C_EVENT, which is received as
                # SIGINT, has no effect on non-console process.
                # interrupt_main() behaves like SIGINT but does not unblock
                # main thread immediately.
                thread.interrupt_main()

        watcher_thread = threading.Thread(target=watchStdin, name="watchStdin")
        watcher_thread.daemon = True
        watcher_thread.start()

        tr = TestRunner(config, test_identifiers)
        tr.parseTestConfig(config)
        try:
            watcher_enabled.set()
            tr.runTestClass(test_class, None)
        except KeyboardInterrupt as e:
            logging.exception("Aborted")
        except Exception as e:
            logging.error("Unexpected exception")
            logging.exception(e)
        finally:
            watcher_enabled.clear()
            tr.stop()
            return tr.results


class TestRunner(object):
    """The class that instantiates test classes, executes test cases, and
    report results.

    Attributes:
        test_run_info: A dictionary containing the information needed by
                       test classes for this test run, including params,
                       controllers, and other objects. All of these will
                       be passed to test classes.
        test_configs: A dictionary that is the original test configuration
                      passed in by user.
        id: A string that is the unique identifier of this test run.
        log_path: A string representing the path of the dir under which
                  all logs from this test run should be written.
        controller_registry: A dictionary that holds the controller
                             objects used in a test run.
        controller_destructors: A dictionary that holds the controller
                                distructors. Keys are controllers' names.
        run_list: A list of tuples specifying what tests to run.
        results: The test result object used to record the results of
                 this test run.
        running: A boolean signifies whether this test run is ongoing or
                 not.
        test_cls_instances: list of test class instances that were executed
                            or scheduled to be executed.
        log_severity: string, log severity level for the test logger.
    """

    def __init__(self, test_configs, run_list):
        self.test_run_info = {}
        self.test_run_info[keys.ConfigKeys.IKEY_DATA_FILE_PATH] = getattr(
            test_configs, keys.ConfigKeys.IKEY_DATA_FILE_PATH, "./")
        self.test_configs = test_configs
        self.testbed_configs = self.test_configs[keys.ConfigKeys.KEY_TESTBED]
        self.testbed_name = self.testbed_configs[
            keys.ConfigKeys.KEY_TESTBED_NAME]
        start_time = logger.getLogFileTimestamp()
        self.id = "{}@{}".format(self.testbed_name, start_time)
        # log_path should be set before parsing configs.
        l_path = os.path.join(self.test_configs[keys.ConfigKeys.KEY_LOG_PATH],
                              self.testbed_name, start_time)
        self.log_path = os.path.abspath(l_path)
        self.log_severity = self.test_configs.get(
            keys.ConfigKeys.KEY_LOG_SEVERITY, "INFO").upper()
        logger.setupTestLogger(
            self.log_path, self.testbed_name, log_severity=self.log_severity)
        self.controller_registry = {}
        self.controller_destructors = {}
        self.run_list = run_list
        self.results = records.TestResult()
        self.running = False
        self.test_cls_instances = []

    def __enter__(self):
        return self

    def __exit__(self, *args):
        self.stop()

    def importTestModules(self, test_paths):
        """Imports test classes from test scripts.

        1. Locate all .py files under test paths.
        2. Import the .py files as modules.
        3. Find the module members that are test classes.
        4. Categorize the test classes by name.

        Args:
            test_paths: A list of directory paths where the test files reside.

        Returns:
            A dictionary where keys are test class name strings, values are
            actual test classes that can be instantiated.
        """

        def is_testfile_name(name, ext):
            if ext == ".py":
                if name.endswith("Test") or name.endswith("_test"):
                    return True
            return False

        file_list = utils.find_files(test_paths, is_testfile_name)
        test_classes = {}
        for path, name, _ in file_list:
            sys.path.append(path)
            try:
                module = importlib.import_module(name)
            except:
                for test_cls_name, _ in self.run_list:
                    alt_name = name.replace('_', '').lower()
                    alt_cls_name = test_cls_name.lower()
                    # Only block if a test class on the run list causes an
                    # import error. We need to check against both naming
                    # conventions: AaaBbb and aaa_bbb.
                    if name == test_cls_name or alt_name == alt_cls_name:
                        msg = ("Encountered error importing test class %s, "
                               "abort.") % test_cls_name
                        # This exception is logged here to help with debugging
                        # under py2, because "raise X from Y" syntax is only
                        # supported under py3.
                        logging.exception(msg)
                        raise USERError(msg)
                continue
            for member_name in dir(module):
                if not member_name.startswith("__"):
                    if member_name.endswith("Test"):
                        test_class = getattr(module, member_name)
                        if inspect.isclass(test_class):
                            test_classes[member_name] = test_class
        return test_classes

    def verifyControllerModule(self, module):
        """Verifies a module object follows the required interface for
        controllers.

        Args:
            module: An object that is a controller module. This is usually
                    imported with import statements or loaded by importlib.

        Raises:
            ControllerError is raised if the module does not match the vts.runners.host
            controller interface, or one of the required members is null.
        """
        required_attributes = ("create", "destroy",
                               "VTS_CONTROLLER_CONFIG_NAME")
        for attr in required_attributes:
            if not hasattr(module, attr):
                raise signals.ControllerError(
                    ("Module %s missing required "
                     "controller module attribute %s.") % (module.__name__,
                                                           attr))
            if not getattr(module, attr):
                raise signals.ControllerError(
                    ("Controller interface %s in %s "
                     "cannot be null.") % (attr, module.__name__))

    def registerController(self, module, start_services=True):
        """Registers a controller module for a test run.

        This declares a controller dependency of this test class. If the target
        module exists and matches the controller interface, the controller
        module will be instantiated with corresponding configs in the test
        config file. The module should be imported first.

        Params:
            module: A module that follows the controller module interface.
            start_services: boolean, controls whether services (e.g VTS agent)
                            are started on the target.

        Returns:
            A list of controller objects instantiated from controller_module.

        Raises:
            ControllerError is raised if no corresponding config can be found,
            or if the controller module has already been registered.
        """
        logging.info("cwd: %s", os.getcwd())
        logging.info("adb devices: %s", module.list_adb_devices())
        self.verifyControllerModule(module)
        module_ref_name = module.__name__.split('.')[-1]
        if module_ref_name in self.controller_registry:
            raise signals.ControllerError(
                ("Controller module %s has already "
                 "been registered. It can not be "
                 "registered again.") % module_ref_name)
        # Create controller objects.
        create = module.create
        module_config_name = module.VTS_CONTROLLER_CONFIG_NAME
        if module_config_name not in self.testbed_configs:
            raise signals.ControllerError(("No corresponding config found for"
                                           " %s") % module_config_name)
        try:
            # Make a deep copy of the config to pass to the controller module,
            # in case the controller module modifies the config internally.
            original_config = self.testbed_configs[module_config_name]
            controller_config = copy.deepcopy(original_config)
            # Add log_severity config to device controller config.
            if isinstance(controller_config, list):
                for config in controller_config:
                    if isinstance(config, dict):
                        config["log_severity"] = self.log_severity
            logging.info("controller_config: %s", controller_config)
            if "use_vts_agent" not in self.testbed_configs:
                objects = create(controller_config, start_services)
            else:
                objects = create(controller_config,
                                 self.testbed_configs["use_vts_agent"])
        except:
            logging.exception(("Failed to initialize objects for controller "
                               "%s, abort!"), module_config_name)
            raise
        if not isinstance(objects, list):
            raise signals.ControllerError(("Controller module %s did not"
                        " return a list of objects, abort.") % module_ref_name)
        self.controller_registry[module_ref_name] = objects
        logging.debug("Found %d objects for controller %s",
                      len(objects), module_config_name)
        destroy_func = module.destroy
        self.controller_destructors[module_ref_name] = destroy_func
        return objects

    def unregisterControllers(self):
        """Destroy controller objects and clear internal registry.

        This will be called at the end of each TestRunner.run call.
        """
        for name, destroy in self.controller_destructors.items():
            try:
                logging.debug("Destroying %s.", name)
                dut = self.controller_destructors[name]
                destroy(self.controller_registry[name])
            except:
                logging.exception("Exception occurred destroying %s.", name)
        self.controller_registry = {}
        self.controller_destructors = {}

    def parseTestConfig(self, test_configs):
        """Parses the test configuration and unpacks objects and parameters
        into a dictionary to be passed to test classes.

        Args:
            test_configs: A json object representing the test configurations.
        """
        self.test_run_info[
            keys.ConfigKeys.IKEY_TESTBED_NAME] = self.testbed_name
        # Unpack other params.
        self.test_run_info["registerController"] = self.registerController
        self.test_run_info[keys.ConfigKeys.IKEY_LOG_PATH] = self.log_path
        user_param_pairs = []
        for item in test_configs.items():
            if item[0] not in keys.ConfigKeys.RESERVED_KEYS:
                user_param_pairs.append(item)
        self.test_run_info[keys.ConfigKeys.IKEY_USER_PARAM] = copy.deepcopy(
            dict(user_param_pairs))

    def runTestClass(self, test_cls, test_cases=None):
        """Instantiates and executes a test class.

        If test_cases is None, the test cases listed by self.tests will be
        executed instead. If self.tests is empty as well, no test case in this
        test class will be executed.

        Args:
            test_cls: The test class to be instantiated and executed.
            test_cases: List of test case names to execute within the class.

        Returns:
            A tuple, with the number of cases passed at index 0, and the total
            number of test cases at index 1.
        """
        self.running = True
        with test_cls(self.test_run_info) as test_cls_instance:
            try:
                if test_cls_instance not in self.test_cls_instances:
                    self.test_cls_instances.append(test_cls_instance)
                cls_result = test_cls_instance.run(test_cases)
            except signals.TestAbortAll as e:
                raise e
            finally:
                self.unregisterControllers()

    def run(self):
        """Executes test cases.

        This will instantiate controller and test classes, and execute test
        classes. This can be called multiple times to repeatly execute the
        requested test cases.

        A call to TestRunner.stop should eventually happen to conclude the life
        cycle of a TestRunner.

        Args:
            test_classes: A dictionary where the key is test class name, and
                          the values are actual test classes.
        """
        if not self.running:
            self.running = True
        # Initialize controller objects and pack appropriate objects/params
        # to be passed to test class.
        self.parseTestConfig(self.test_configs)
        t_configs = self.test_configs[keys.ConfigKeys.KEY_TEST_PATHS]
        test_classes = self.importTestModules(t_configs)
        logging.debug("Executing run list %s.", self.run_list)
        try:
            for test_cls_name, test_case_names in self.run_list:
                if not self.running:
                    break
                if test_case_names:
                    logging.debug("Executing test cases %s in test class %s.",
                                  test_case_names, test_cls_name)
                else:
                    logging.debug("Executing test class %s", test_cls_name)
                try:
                    test_cls = test_classes[test_cls_name]
                except KeyError:
                    raise USERError(
                        ("Unable to locate class %s in any of the test "
                         "paths specified.") % test_cls_name)
                try:
                    self.runTestClass(test_cls, test_case_names)
                except signals.TestAbortAll as e:
                    logging.warning(
                        ("Abort all subsequent test classes. Reason: "
                         "%s"), e)
                    raise
        except Exception as e:
            logging.error("Unexpected exception")
            logging.exception(e)
        finally:
            self.unregisterControllers()

    def stop(self):
        """Releases resources from test run. Should always be called after
        TestRunner.run finishes.

        This function concludes a test run and writes out a test report.
        """
        if self.running:

            for test_cls_instance in self.test_cls_instances:
                self.results += test_cls_instance.results

            msg = "\nSummary for test run %s: %s\n" % (self.id,
                                                       self.results.summary())
            self._writeResultsJsonString()
            logging.info(msg.strip())
            logger.killTestLogger(logging.getLogger())
            self.running = False

    def _writeResultsJsonString(self):
        """Writes out a json file with the test result info for easy parsing.
        """
        path = os.path.join(self.log_path, "test_run_summary.json")
        with open(path, 'w') as f:
            f.write(self.results.jsonString())
