"""
This module contains the actions that a configurable CFM test can execute.
"""
import abc
import logging
import random
import re
import sys
import time

class Action(object):
    """
    Abstract base class for all actions.
    """
    __metaclass__ = abc.ABCMeta

    def __repr__(self):
        return self.__class__.__name__

    def execute(self, context):
        """
        Executes the action.

        @param context ActionContext instance providing dependencies to the
                action.
        """
        logging.info('Executing action "%s"', self)
        self.do_execute(context)
        logging.info('Done executing action "%s"', self)

    @abc.abstractmethod
    def do_execute(self, context):
        """
        Performs the actual execution.

        Subclasses must override this method.

        @param context ActionContext instance providing dependencies to the
                action.
        """
        pass

class MuteMicrophone(Action):
    """
    Mutes the microphone in a call.
    """
    def do_execute(self, context):
        context.cfm_facade.mute_mic()

class UnmuteMicrophone(Action):
    """
    Unmutes the microphone in a call.
    """
    def do_execute(self, context):
        context.cfm_facade.unmute_mic()

class JoinMeeting(Action):
    """
    Joins a meeting.
    """
    def __init__(self, meeting_code):
        """
        Initializes.

        @param meeting_code The meeting code for the meeting to join.
        """
        super(JoinMeeting, self).__init__()
        self.meeting_code = meeting_code

    def __repr__(self):
        return 'JoinMeeting "%s"' % self.meeting_code

    def do_execute(self, context):
        context.cfm_facade.join_meeting_session(self.meeting_code)

class CreateMeeting(Action):
    """
    Creates a new meeting from the landing page.
    """
    def do_execute(self, context):
        context.cfm_facade.start_meeting_session()

class LeaveMeeting(Action):
    """
    Leaves the current meeting.
    """
    def do_execute(self, context):
        context.cfm_facade.end_meeting_session()

class RebootDut(Action):
    """
    Reboots the DUT.
    """
    def __init__(self, restart_chrome_for_cfm=False):
        """Initializes.

        To enable the cfm_facade to interact with the CFM, Chrome needs an extra
        restart. Setting restart_chrome_for_cfm toggles this extra restart.

        @param restart_chrome_for_cfm If True, restarts chrome to enable
                the cfm_facade and waits for the telemetry commands to become
                available. If false, does not do an extra restart of Chrome.
        """
        self._restart_chrome_for_cfm = restart_chrome_for_cfm

    def do_execute(self, context):
        context.host.reboot()
        if self._restart_chrome_for_cfm:
            context.cfm_facade.restart_chrome_for_cfm()
            context.cfm_facade.wait_for_meetings_telemetry_commands()

class RepeatTimes(Action):
    """
    Repeats a scenario a number of times.
    """
    def __init__(self, times, scenario):
        """
        Initializes.

        @param times The number of times to repeat the scenario.
        @param scenario The scenario to repeat.
        """
        super(RepeatTimes, self).__init__()
        self.times = times
        self.scenario = scenario

    def __str__(self):
        return 'Repeat[scenario=%s, times=%s]' % (self.scenario, self.times)

    def do_execute(self, context):
        for _ in xrange(self.times):
            self.scenario.execute(context)

class AssertFileDoesNotContain(Action):
    """
    Asserts that a file on the DUT does not contain specified regexes.
    """
    def __init__(self, path, forbidden_regex_list):
        """
        Initializes.

        @param path The file path on the DUT to check.
        @param forbidden_regex_list a list with regular expressions that should
                not appear in the file.
        """
        super(AssertFileDoesNotContain, self).__init__()
        self.path = path
        self.forbidden_regex_list = forbidden_regex_list

    def __repr__(self):
        return ('AssertFileDoesNotContain[path=%s, forbidden_regex_list=%s'
                % (self.path, self.forbidden_regex_list))

    def do_execute(self, context):
        contents = context.file_contents_collector.collect_file_contents(
                self.path)
        for forbidden_regex in self.forbidden_regex_list:
            match = re.search(forbidden_regex, contents)
            if match:
                raise AssertionError(
                        'Regex "%s" matched "%s" in "%s"'
                        % (forbidden_regex, match.group(), self.path))

class AssertUsbDevices(Action):
    """
    Asserts that USB devices with given specs matches a predicate.
    """
    def __init__(
            self,
            usb_device_specs,
            predicate=lambda usb_device_list: len(usb_device_list) == 1):
        """
        Initializes with a spec to assert and a predicate.

        @param usb_device_specs a list of UsbDeviceSpecs for the devices to
                check.
        @param predicate A function that accepts a list of UsbDevices
                and returns true if the list is as expected or false otherwise.
                If the method returns false an AssertionError is thrown.
                The default predicate checks that there is exactly one item
                in the list.
        """
        super(AssertUsbDevices, self).__init__()
        self._usb_device_specs = usb_device_specs
        self._predicate = predicate

    def do_execute(self, context):
        usb_devices = context.usb_device_collector.get_devices_by_spec(
                *self._usb_device_specs)
        if not self._predicate(usb_devices):
            raise AssertionError(
                    'Assertion failed for usb device specs %s. '
                    'Usb devices were: %s'
                    % (self._usb_device_specs, usb_devices))

    def __str__(self):
        return 'AssertUsbDevices for specs %s' % str(self._usb_device_specs)

class SelectScenarioAtRandom(Action):
    """
    Executes a randomly selected scenario a number of times.

    Note that there is no validation performed - you have to take care
    so that it makes sense to execute the supplied scenarios in any order
    any number of times.
    """
    def __init__(
            self,
            scenarios,
            run_times,
            random_seed=random.randint(0, sys.maxsize)):
        """
        Initializes.

        @param scenarios An iterable with scenarios to choose from.
        @param run_times The number of scenarios to run. I.e. the number of
            times a random scenario is selected.
        @param random_seed The seed to use for the random generator. Providing
            the same seed as an earlier run will execute the scenarios in the
            same order. Optional, by default a random seed is used.
        """
        super(SelectScenarioAtRandom, self).__init__()
        self._scenarios = scenarios
        self._run_times = run_times
        self._random_seed = random_seed
        self._random = random.Random(random_seed)

    def do_execute(self, context):
        for _ in xrange(self._run_times):
            self._random.choice(self._scenarios).execute(context)

    def __repr__(self):
        return ('SelectScenarioAtRandom [seed=%s, run_times=%s, scenarios=%s]'
                % (self._random_seed, self._run_times, self._scenarios))


class PowerCycleUsbPort(Action):
    """
    Power cycle USB ports that a specific peripheral type is attached to.
    """
    def __init__(
            self,
            usb_device_specs,
            wait_for_change_timeout=10,
            filter_function=lambda x: x):
        """
        Initializes.

        @param usb_device_specs List of UsbDeviceSpecs of the devices to power
            cycle the port for.
        @param wait_for_change_timeout The timeout in seconds for waiting
            for devices to disappeard/appear after turning power off/on.
            If the devices do not disappear/appear within the timeout an
            error is raised.
        @param filter_function Function accepting a list of UsbDevices and
            returning a list of UsbDevices that should be power cycled. The
            default is to return the original list, i.e. power cycle all
            devices matching the usb_device_specs.

        @raises TimeoutError if the devices do not turn off/on within
            wait_for_change_timeout seconds.
        """
        self._usb_device_specs = usb_device_specs
        self._filter_function = filter_function
        self._wait_for_change_timeout = wait_for_change_timeout

    def do_execute(self, context):
        def _get_devices():
            return context.usb_device_collector.get_devices_by_spec(
                    *self._usb_device_specs)
        devices = _get_devices()
        devices_to_cycle = self._filter_function(devices)
        # If we are asked to power cycle a device connected to a USB hub (for
        # example a Mimo which has an internal hub) the devices's bus and port
        # cannot be used. Those values represent the bus and port of the hub.
        # Instead we must locate the device that is actually connected to the
        # physical USB port. This device is the parent at level 1 of the current
        # device. If the device is not connected to a hub, device.get_parent(1)
        # will return the device itself.
        devices_to_cycle = [device.get_parent(1) for device in devices_to_cycle]
        logging.debug('Power cycling devices: %s', devices_to_cycle)
        port_ids = [(d.bus, d.port) for d in devices_to_cycle]
        context.usb_port_manager.set_port_power(port_ids, False)
        # TODO(kerl): We should do a better check than counting devices.
        # Possibly implementing __eq__() in UsbDevice and doing a proper
        # intersection to see which devices are running or not.
        expected_devices_after_power_off = len(devices) - len(devices_to_cycle)
        _wait_for_condition(
                lambda: len(_get_devices()) == expected_devices_after_power_off,
                self._wait_for_change_timeout)
        context.usb_port_manager.set_port_power(port_ids, True)
        _wait_for_condition(
                lambda: len(_get_devices()) == len(devices),
                self._wait_for_change_timeout)

    def __repr__(self):
        return ('PowerCycleUsbPort[usb_device_specs=%s, '
                'wait_for_change_timeout=%s]'
                % (str(self._usb_device_specs), self._wait_for_change_timeout))


class Sleep(Action):
    """
    Action that sleeps for a number of seconds.
    """
    def __init__(self, num_seconds):
        """
        Initializes.

        @param num_seconds The number of seconds to sleep.
        """
        self._num_seconds = num_seconds

    def do_execute(self, context):
        time.sleep(self._num_seconds)

    def __repr__(self):
        return 'Sleep[num_seconds=%s]' % self._num_seconds


class RetryAssertAction(Action):
    """
    Action that retries an assertion action a number of times if it fails.

    An example use case for this action is to verify that a peripheral device
    appears after power cycling. E.g.:
        PowerCycleUsbPort(ATRUS),
        RetryAssertAction(AssertUsbDevices(ATRUS), 10)
    """
    def __init__(self, action, num_tries, retry_delay_seconds=1):
        """
        Initializes.

        @param action The action to execute.
        @param num_tries The number of times to try the action before failing
            for real. Must be more than 0.
        @param retry_delay_seconds The number of seconds to sleep between
            retries.

        @raises ValueError if num_tries is below 1.
        """
        super(RetryAssertAction, self).__init__()
        if num_tries < 1:
            raise ValueError('num_tries must be > 0. Was %s' % num_tries)
        self._action = action
        self._num_tries = num_tries
        self._retry_delay_seconds = retry_delay_seconds

    def do_execute(self, context):
        for attempt in xrange(self._num_tries):
            try:
                self._action.execute(context)
                return
            except AssertionError as e:
                if attempt == self._num_tries - 1:
                    raise e
                else:
                    logging.info(
                            'Action %s failed, will retry %d more times',
                             self._action,
                             self._num_tries - attempt - 1,
                             exc_info=True)
                    time.sleep(self._retry_delay_seconds)

    def __repr__(self):
        return ('RetryAssertAction[action=%s, '
                'num_tries=%s, retry_delay_seconds=%s]'
                % (self._action, self._num_tries, self._retry_delay_seconds))


class AssertNoNewCrashes(Action):
    """
    Asserts that no new crash files exist on disk.
    """
    def do_execute(self, context):
        new_crash_files = context.crash_detector.get_new_crash_files()
        if new_crash_files:
            raise AssertionError(
                    'New crash files detected: %s' % str(new_crash_files))


class TimeoutError(RuntimeError):
    """
    Error raised when an operation times out.
    """
    pass


def _wait_for_condition(condition, timeout_seconds=10):
    """
    Wait for a condition to become true.

    Checks the condition every second.

    @param condition The condition to check - a function returning a boolean.
    @param timeout_seconds The timeout in seconds.

    @raises TimeoutError in case the condition does not become true within
        the timeout.
    """
    if condition():
        return
    for _ in xrange(timeout_seconds):
        time.sleep(1)
        if condition():
            return
    raise TimeoutError('Timeout after %s seconds waiting for condition %s'
                       % (timeout_seconds, condition))

