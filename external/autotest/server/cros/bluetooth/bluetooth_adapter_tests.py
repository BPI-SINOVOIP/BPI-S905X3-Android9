# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Server side bluetooth adapter subtests."""

import inspect
import functools
import logging
import re
import time

from autotest_lib.client.bin import utils
from autotest_lib.client.bin.input import input_event_recorder as recorder
from autotest_lib.client.common_lib import error
from autotest_lib.server import test
from autotest_lib.client.bin.input.linux_input import (
        BTN_LEFT, BTN_RIGHT, EV_KEY, EV_REL, REL_X, REL_Y, REL_WHEEL)


REBOOTING_CHAMELEON = False

Event = recorder.Event


# Delay binding the methods since host is only available at run time.
SUPPORTED_DEVICE_TYPES = {
    'MOUSE': lambda host: host.chameleon.get_bluetooth_hid_mouse,
    'LE_MOUSE': lambda host: host.chameleon.get_bluetooth_hog_mouse
}


def method_name():
    """Get the method name of a class.

    This function is supposed to be invoked inside a class and will
    return current method name who invokes this function.

    @returns: the string of the method name inside the class.
    """
    return inspect.getouterframes(inspect.currentframe())[1][3]


def _run_method(method, method_name, *args, **kwargs):
    """Run a target method and capture exceptions if any.

    This is just a wrapper of the target method so that we do not need to
    write the exception capturing structure repeatedly. The method could
    be either a device method or a facade method.

    @param method: the method to run
    @param method_name: the name of the method

    @returns: the return value of target method() if successful.
              False otherwise.

    """
    result = False
    try:
        result = method(*args, **kwargs)
    except Exception as e:
        logging.error('%s: %s', method_name, e)
    except:
        logging.error('%s: unexpected error', method_name)
    return result


def get_bluetooth_emulated_device(host, device_type):
    """Get the bluetooth emulated device object.

    @param host: the DUT, usually a chromebook
    @param device_type : the bluetooth HID device type, e.g., 'MOUSE'

    @returns: the bluetooth device object

    """

    def _retry_device_method(method_name, legal_falsy_values=[]):
        """retry the emulated device's method.

        The method is invoked as device.xxxx() e.g., device.GetAdvertisedName().

        Note that the method name string is provided to get the device's actual
        method object at run time through getattr(). The rebinding is required
        because a new device may have been created previously or during the
        execution of fix_serial_device().

        Given a device's method, it is not feasible to get the method name
        through __name__ attribute. This limitation is due to the fact that
        the device is a dotted object of an XML RPC server proxy.
        As an example, with the method name 'GetAdvertisedName', we could
        derive the correspoinding method device.GetAdvertisedName. On the
        contrary, given device.GetAdvertisedName, it is not feasible to get the
        method name by device.GetAdvertisedName.__name__

        Also note that if the device method fails at the first time, we would
        try to fix the problem by re-creating the serial device and see if the
        problem is fixed. If not, we will reboot the chameleon board and see
        if the problem is fixed. If yes, execute the target method the second
        time.

        The default values exist for uses of this function before the options
        were added, ideally we should change zero_ok to False.

        @param method_name: the string of the method name.
        @param legal_falsy_values: Values that are falsy but might be OK.

        @returns: the result returned by the device's method.

        """
        result = _run_method(getattr(device, method_name), method_name)
        if _is_successful(result, legal_falsy_values):
            return result

        logging.error('%s failed the 1st time. Try to fix the serial device.',
                      method_name)

        # Try to fix the serial device if possible.
        if not fix_serial_device(host, device):
            return False

        logging.info('%s: retry the 2nd time.', method_name)
        return _run_method(getattr(device, method_name), method_name)


    if device_type not in SUPPORTED_DEVICE_TYPES:
        raise error.TestError('The device type is not supported: %s',
                              device_type)

    # Get the bluetooth device object and query some important properties.
    device = SUPPORTED_DEVICE_TYPES[device_type](host)()

    # Get some properties of the kit
    # NOTE: Strings updated here must be kept in sync with Chameleon.
    device._capabilities = _retry_device_method('GetCapabilities')
    device._transports = device._capabilities["CAP_TRANSPORTS"]
    device._is_le_only = ("TRANSPORT_LE" in device._transports and
                          len(device._transports) == 1)
    device._has_pin = device._capabilities["CAP_HAS_PIN"]
    device.can_init_connection = device._capabilities["CAP_INIT_CONNECT"]

    _retry_device_method('Init')
    logging.info('device type: %s', device_type)

    device.name = _retry_device_method('GetAdvertisedName')
    logging.info('device name: %s', device.name)

    device.address = _retry_device_method('GetLocalBluetoothAddress')
    logging.info('address: %s', device.address)

    pin_falsy_values = [] if device._has_pin else [None]
    device.pin = _retry_device_method('GetPinCode', pin_falsy_values)
    logging.info('pin: %s', device.pin)

    class_falsy_values = [None] if device._is_le_only else [0]

    # Class of service is None for LE-only devices. Don't fail or parse it.
    device.class_of_service = _retry_device_method('GetClassOfService',
                                                   class_falsy_values)
    if device._is_le_only:
      parsed_class_of_service = device.class_of_service
    else:
      parsed_class_of_service = "0x%04X" % device.class_of_service
    logging.info('class of service: %s', parsed_class_of_service)

    device.class_of_device = _retry_device_method('GetClassOfDevice',
                                                  class_falsy_values)
    # Class of device is None for LE-only devices. Don't fail or parse it.
    if device._is_le_only:
      parsed_class_of_device = device.class_of_device
    else:
      parsed_class_of_device = "0x%04X" % device.class_of_device
    logging.info('class of device: %s', parsed_class_of_device)

    device.device_type = _retry_device_method('GetHIDDeviceType')
    logging.info('device type: %s', device.device_type)

    device.authenticaiton_mode = _retry_device_method('GetAuthenticationMode')
    logging.info('authentication mode: %s', device.authenticaiton_mode)

    device.port = _retry_device_method('GetPort')
    logging.info('serial port: %s\n', device.port)

    return device


def recreate_serial_device(device):
    """Create and connect to a new serial device.

    @param device: the bluetooth HID device

    @returns: True if the serial device is re-created successfully.

    """
    logging.info('Remove the old serial device and create a new one.')
    if device is not None:
        try:
            device.Close()
        except:
            logging.error('failed to close the serial device.')
            return False
    try:
        device.CreateSerialDevice()
        return True
    except:
        logging.error('failed to invoke CreateSerialDevice.')
        return False


def _reboot_chameleon(host, device):
    REBOOT_SLEEP_SECS = 40

    if not REBOOTING_CHAMELEON:
        logging.info('Skip rebooting chameleon.')
        return False

    # Close the bluetooth peripheral device and reboot the chameleon board.
    device.Close()
    logging.info('rebooting chameleon...')
    host.chameleon.reboot()

    # Every chameleon reboot would take a bit more than REBOOT_SLEEP_SECS.
    # Sleep REBOOT_SLEEP_SECS and then begin probing the chameleon board.
    time.sleep(REBOOT_SLEEP_SECS)

    # Check if the serial device could initialize, connect, and
    # enter command mode correctly.
    logging.info('Checking device status...')
    if not _run_method(device.Init, 'Init'):
        logging.info('device.Init: failed after reboot')
        return False
    if not device.CheckSerialConnection():
        logging.info('device.CheckSerialConnection: failed after reboot')
        return False
    if not _run_method(device.EnterCommandMode, 'EnterCommandMode'):
        logging.info('device.EnterCommandMode: failed after reboot')
        return False
    logging.info('The device is created successfully after reboot.')
    return True


def _is_successful(result, legal_falsy_values=[]):
    """Is the method result considered successful?

    Some method results, for example that of class_of_service, may be 0 which is
    considered a valid result. Occassionally, None is acceptable.

    The default values exist for uses of this function before the options were
    added, ideally we should change zero_ok to False.

    @param result: a method result
    @param legal_falsy_values: Values that are falsy but might be OK.

    @returns: True if bool(result) is True, or if result is 0 and zero_ok, or if
              result is None and none_ok.
    """
    truthiness_of_result = bool(result)
    return truthiness_of_result or result in legal_falsy_values


def fix_serial_device(host, device):
    """Fix the serial device.

    This function tries to fix the serial device by
    (1) re-creating a serial device, or
    (2) rebooting the chameleon board.

    @param host: the DUT, usually a chromebook
    @param device: the bluetooth HID device

    @returns: True if the serial device is fixed. False otherwise.

    """
    # Check the serial connection. Fix it if needed.
    if device.CheckSerialConnection():
        # The USB serial connection still exists.
        # Re-connection suffices to solve the problem. The problem
        # is usually caused by serial port change. For example,
        # the serial port changed from /dev/ttyUSB0 to /dev/ttyUSB1.
        logging.info('retry: creating a new serial device...')
        if not recreate_serial_device(device):
            return False

    # Check if recreate_serial_device() above fixes the problem.
    # If not, reboot the chameleon board including creation of a new
    # bluetooth device. Check if reboot fixes the problem.
    # If not, return False.
    result = _run_method(device.EnterCommandMode, 'EnterCommandMode')
    return _is_successful(result) or _reboot_chameleon(host, device)


def retry(test_method, instance, *args, **kwargs):
    """Execute the target facade test_method(). Retry if failing the first time.

    A test_method is something like self.test_xxxx() in BluetoothAdapterTests,
    e.g., BluetoothAdapterTests.test_bluetoothd_running().

    @param test_method: the test method to retry

    @returns: True if the return value of test_method() is successful.
              False otherwise.

    """
    if _is_successful(_run_method(test_method, test_method.__name__,
                                  instance, *args, **kwargs)):
        return True

    # Try to fix the serial device if applicable.
    logging.error('%s failed at the 1st time.', test_method.__name__)

    # If this test does not use any attached serial device, just re-run
    # the test.
    logging.info('%s: retry the 2nd time.', test_method.__name__)
    time.sleep(1)
    if not hasattr(instance, 'device_type'):
        return _is_successful(_run_method(test_method, test_method.__name__,
                                          instance, *args, **kwargs))

    host = instance.host
    device = instance.devices[instance.device_type]
    if not fix_serial_device(host, device):
        return False

    logging.info('%s: retry the 2nd time.', test_method.__name__)
    return _is_successful(_run_method(test_method, test_method.__name__,
                                      instance, *args, **kwargs))


def _test_retry_and_log(test_method_or_retry_flag):
    """A decorator that logs test results, collects error messages, and retries
       on request.

    @param test_method_or_retry_flag: either the test_method or a retry_flag.
        There are some possibilities of this argument:
        1. the test_method to conduct and retry: should retry the test_method.
            This occurs with
            @_test_retry_and_log
        2. the retry flag is True. Should retry the test_method.
            This occurs with
            @_test_retry_and_log(True)
        3. the retry flag is False. Do not retry the test_method.
            This occurs with
            @_test_retry_and_log(False)

    @returns: a wrapper of the test_method with test log. The retry mechanism
        would depend on the retry flag.

    """

    def decorator(test_method):
        """A decorator wrapper of the decorated test_method.

        @param test_method: the test method being decorated.

        @returns the wrapper of the test method.

        """
        @functools.wraps(test_method)
        def wrapper(instance, *args, **kwargs):
            """A wrapper of the decorated method.

            @param instance: an BluetoothAdapterTests instance

            @returns the result of the test method

            """
            instance.results = None
            if callable(test_method_or_retry_flag) or test_method_or_retry_flag:
                test_result = retry(test_method, instance, *args, **kwargs)
            else:
                test_result = test_method(instance, *args, **kwargs)

            if test_result:
                logging.info('[*** passed: %s]', test_method.__name__)
            else:
                fail_msg = '[--- failed: %s (%s)]' % (test_method.__name__,
                                                      str(instance.results))
                logging.error(fail_msg)
                instance.fails.append(fail_msg)
            return test_result
        return wrapper

    if callable(test_method_or_retry_flag):
        # If the decorator function comes with no argument like
        # @_test_retry_and_log
        return decorator(test_method_or_retry_flag)
    else:
        # If the decorator function comes with an argument like
        # @_test_retry_and_log(False)
        return decorator


def test_case_log(method):
    """A decorator for test case methods.

    The main purpose of this decorator is to display the test case name
    in the test log which looks like

        <... test_case_RA3_CD_SI200_CD_PC_CD_UA3 ...>

    @param method: the test case method to decorate.

    @returns: a wrapper function of the decorated method.

    """
    @functools.wraps(method)
    def wrapper(instance, *args, **kwargs):
        """Log the name of the wrapped method before execution"""
        logging.info('\n<... %s ...>', method.__name__)
        method(instance, *args, **kwargs)
    return wrapper


class BluetoothAdapterTests(test.test):
    """Server side bluetooth adapter tests.

    This test class tries to thoroughly verify most of the important work
    states of a bluetooth adapter.

    The various test methods are supposed to be invoked by actual autotest
    tests such as server/cros/site_tests/bluetooth_Adapter*.

    """
    version = 1
    ADAPTER_ACTION_SLEEP_SECS = 1
    ADAPTER_PAIRING_TIMEOUT_SECS = 60
    ADAPTER_CONNECTION_TIMEOUT_SECS = 30
    ADAPTER_DISCONNECTION_TIMEOUT_SECS = 30
    ADAPTER_PAIRING_POLLING_SLEEP_SECS = 3
    ADAPTER_DISCOVER_TIMEOUT_SECS = 60          # 30 seconds too short sometimes
    ADAPTER_DISCOVER_POLLING_SLEEP_SECS = 1
    ADAPTER_DISCOVER_NAME_TIMEOUT_SECS = 30
    ADAPTER_WAIT_DEFAULT_TIMEOUT_SECS = 10
    ADAPTER_POLLING_DEFAULT_SLEEP_SECS = 1

    HID_REPORT_SLEEP_SECS = 1

    # Default suspend time in seconds for suspend resume.
    SUSPEND_TIME_SECS=10

    # hci0 is the default hci device if there is no external bluetooth dongle.
    EXPECTED_HCI = 'hci0'

    CLASS_OF_SERVICE_MASK = 0xFFE000
    CLASS_OF_DEVICE_MASK = 0x001FFF

    # Constants about advertising.
    DAFAULT_MIN_ADVERTISEMENT_INTERVAL_MS = 1280
    DAFAULT_MAX_ADVERTISEMENT_INTERVAL_MS = 1280
    ADVERTISING_INTERVAL_UNIT = 0.625

    # Error messages about advertising dbus methods.
    ERROR_FAILED_TO_REGISTER_ADVERTISEMENT = (
            'org.bluez.Error.Failed: Failed to register advertisement')
    ERROR_INVALID_ADVERTISING_INTERVALS = (
            'org.bluez.Error.InvalidArguments: Invalid arguments')

    # Supported profiles by chrome os.
    SUPPORTED_UUIDS = {
            'HSP_AG_UUID': '00001112-0000-1000-8000-00805f9b34fb',
            'GATT_UUID': '00001801-0000-1000-8000-00805f9b34fb',
            'A2DP_SOURCE_UUID': '0000110a-0000-1000-8000-00805f9b34fb',
            'HFP_AG_UUID': '0000111f-0000-1000-8000-00805f9b34fb',
            'PNP_UUID': '00001200-0000-1000-8000-00805f9b34fb',
            'GAP_UUID': '00001800-0000-1000-8000-00805f9b34fb'}


    def get_device(self, device_type):
        """Get the bluetooth device object.

        @param device_type : the bluetooth HID device type, e.g., 'MOUSE'

        @returns: the bluetooth device object

        """
        self.device_type = device_type
        if self.devices[device_type] is None:
            self.devices[device_type] = get_bluetooth_emulated_device(
                    self.host, device_type)
        return self.devices[device_type]


    def suspend_resume(self, suspend_time=SUSPEND_TIME_SECS):
        """Suspend the DUT for a while and then resume.

        @param suspend_time: the suspend time in secs

        """
        logging.info('The DUT suspends for %d seconds...', suspend_time)
        try:
            self.host.suspend(suspend_time=suspend_time)
        except error.AutoservSuspendError:
            logging.error('The DUT did not suspend for %d seconds', suspend_time)
            pass
        logging.info('The DUT is waken up.')


    def _wait_for_condition(self, func, method_name,
                            timeout=ADAPTER_WAIT_DEFAULT_TIMEOUT_SECS,
                            sleep_interval=ADAPTER_POLLING_DEFAULT_SLEEP_SECS):
        """Wait for the func() to become True.

        @param fun: the function to wait for.
        @param method_name: the invoking class method.
        @param timeout: number of seconds to wait before giving up.
        @param sleep_interval: the interval in seconds to sleep between
                invoking func().

        @returns: the bluetooth device object

        """

        try:
            utils.poll_for_condition(condition=func,
                                     timeout=timeout,
                                     sleep_interval=sleep_interval,
                                     desc=('Waiting %s', method_name))
            return True
        except utils.TimeoutError as e:
            logging.error('%s: %s', method_name, e)
        except Exception as e:
            logging.error('%s: %s', method_name, e)
            err = 'bluetoothd possibly crashed. Check out /var/log/messages.'
            logging.error(err)
        except:
            logging.error('%s: unexpected error', method_name)
        return False


    # -------------------------------------------------------------------
    # Adater standalone tests
    # -------------------------------------------------------------------


    @_test_retry_and_log
    def test_bluetoothd_running(self):
        """Test that bluetoothd is running."""
        return self.bluetooth_facade.is_bluetoothd_running()


    @_test_retry_and_log
    def test_start_bluetoothd(self):
        """Test that bluetoothd could be started successfully."""
        return self.bluetooth_facade.start_bluetoothd()


    @_test_retry_and_log
    def test_stop_bluetoothd(self):
        """Test that bluetoothd could be stopped successfully."""
        return self.bluetooth_facade.stop_bluetoothd()


    @_test_retry_and_log
    def test_adapter_work_state(self):
        """Test that the bluetooth adapter is in the correct working state.

        This includes that the adapter is detectable, is powered on,
        and its hci device is hci0.
        """
        has_adapter = self.bluetooth_facade.has_adapter()
        is_powered_on = self._wait_for_condition(
                self.bluetooth_facade.is_powered_on, method_name())
        hci = self.bluetooth_facade.get_hci() == self.EXPECTED_HCI
        self.results = {
                'has_adapter': has_adapter,
                'is_powered_on': is_powered_on,
                'hci': hci}
        return all(self.results.values())


    @_test_retry_and_log
    def test_power_on_adapter(self):
        """Test that the adapter could be powered on successfully."""
        power_on = self.bluetooth_facade.set_powered(True)
        is_powered_on = self._wait_for_condition(
                self.bluetooth_facade.is_powered_on, method_name())

        self.results = {'power_on': power_on, 'is_powered_on': is_powered_on}
        return all(self.results.values())


    @_test_retry_and_log
    def test_power_off_adapter(self):
        """Test that the adapter could be powered off successfully."""
        power_off = self.bluetooth_facade.set_powered(False)
        is_powered_off = self._wait_for_condition(
                lambda: not self.bluetooth_facade.is_powered_on(),
                method_name())

        self.results = {
                'power_off': power_off,
                'is_powered_off': is_powered_off}
        return all(self.results.values())


    @_test_retry_and_log
    def test_reset_on_adapter(self):
        """Test that the adapter could be reset on successfully.

        This includes restarting bluetoothd, and removing the settings
        and cached devices.
        """
        reset_on = self.bluetooth_facade.reset_on()
        is_powered_on = self._wait_for_condition(
                self.bluetooth_facade.is_powered_on, method_name())

        self.results = {'reset_on': reset_on, 'is_powered_on': is_powered_on}
        return all(self.results.values())


    @_test_retry_and_log
    def test_reset_off_adapter(self):
        """Test that the adapter could be reset off successfully.

        This includes restarting bluetoothd, and removing the settings
        and cached devices.
        """
        reset_off = self.bluetooth_facade.reset_off()
        is_powered_off = self._wait_for_condition(
                lambda: not self.bluetooth_facade.is_powered_on(),
                method_name())

        self.results = {
                'reset_off': reset_off,
                'is_powered_off': is_powered_off}
        return all(self.results.values())


    @_test_retry_and_log
    def test_UUIDs(self):
        """Test that basic profiles are supported."""
        adapter_UUIDs = self.bluetooth_facade.get_UUIDs()
        self.results = [uuid for uuid in self.SUPPORTED_UUIDS.values()
                        if uuid not in adapter_UUIDs]
        return not bool(self.results)


    @_test_retry_and_log
    def test_start_discovery(self):
        """Test that the adapter could start discovery."""
        start_discovery = self.bluetooth_facade.start_discovery()
        is_discovering = self._wait_for_condition(
                self.bluetooth_facade.is_discovering, method_name())

        self.results = {
                'start_discovery': start_discovery,
                'is_discovering': is_discovering}
        return all(self.results.values())


    @_test_retry_and_log
    def test_stop_discovery(self):
        """Test that the adapter could stop discovery."""
        stop_discovery = self.bluetooth_facade.stop_discovery()
        is_not_discovering = self._wait_for_condition(
                lambda: not self.bluetooth_facade.is_discovering(),
                method_name())

        self.results = {
                'stop_discovery': stop_discovery,
                'is_not_discovering': is_not_discovering}
        return all(self.results.values())


    @_test_retry_and_log
    def test_discoverable(self):
        """Test that the adapter could be set discoverable."""
        set_discoverable = self.bluetooth_facade.set_discoverable(True)
        is_discoverable = self._wait_for_condition(
                self.bluetooth_facade.is_discoverable, method_name())

        self.results = {
                'set_discoverable': set_discoverable,
                'is_discoverable': is_discoverable}
        return all(self.results.values())


    @_test_retry_and_log
    def test_nondiscoverable(self):
        """Test that the adapter could be set non-discoverable."""
        set_nondiscoverable = self.bluetooth_facade.set_discoverable(False)
        is_nondiscoverable = self._wait_for_condition(
                lambda: not self.bluetooth_facade.is_discoverable(),
                method_name())

        self.results = {
                'set_nondiscoverable': set_nondiscoverable,
                'is_nondiscoverable': is_nondiscoverable}
        return all(self.results.values())


    @_test_retry_and_log
    def test_pairable(self):
        """Test that the adapter could be set pairable."""
        set_pairable = self.bluetooth_facade.set_pairable(True)
        is_pairable = self._wait_for_condition(
                self.bluetooth_facade.is_pairable, method_name())

        self.results = {
                'set_pairable': set_pairable,
                'is_pairable': is_pairable}
        return all(self.results.values())


    @_test_retry_and_log
    def test_nonpairable(self):
        """Test that the adapter could be set non-pairable."""
        set_nonpairable = self.bluetooth_facade.set_pairable(False)
        is_nonpairable = self._wait_for_condition(
                lambda: not self.bluetooth_facade.is_pairable(), method_name())

        self.results = {
                'set_nonpairable': set_nonpairable,
                'is_nonpairable': is_nonpairable}
        return all(self.results.values())


    # -------------------------------------------------------------------
    # Tests about general discovering, pairing, and connection
    # -------------------------------------------------------------------


    @_test_retry_and_log
    def test_discover_device(self, device_address):
        """Test that the adapter could discover the specified device address.

        @param device_address: Address of the device.

        @returns: True if the device is found. False otherwise.

        """
        has_device_initially = False
        start_discovery = False
        device_discovered = False
        has_device = self.bluetooth_facade.has_device

        if has_device(device_address):
            has_device_initially = True
        elif self.bluetooth_facade.start_discovery():
            start_discovery = True
            try:
                utils.poll_for_condition(
                        condition=(lambda: has_device(device_address)),
                        timeout=self.ADAPTER_DISCOVER_TIMEOUT_SECS,
                        sleep_interval=self.ADAPTER_DISCOVER_POLLING_SLEEP_SECS,
                        desc='Waiting for discovering %s' % device_address)
                device_discovered = True
            except utils.TimeoutError as e:
                logging.error('test_discover_device: %s', e)
            except Exception as e:
                logging.error('test_discover_device: %s', e)
                err = 'bluetoothd probably crashed. Check out /var/log/messages'
                logging.error(err)
            except:
                logging.error('test_discover_device: unexpected error')

        self.results = {
                'has_device_initially': has_device_initially,
                'start_discovery': start_discovery,
                'device_discovered': device_discovered}
        return has_device_initially or device_discovered


    @_test_retry_and_log
    def test_pairing(self, device_address, pin, trusted=True):
        """Test that the adapter could pair with the device successfully.

        @param device_address: Address of the device.
        @param pin: pin code to pair with the device.
        @param trusted: indicating whether to set the device trusted.

        @returns: True if pairing succeeds. False otherwise.

        """

        def _pair_device():
            """Pair to the device.

            @returns: True if it could pair with the device. False otherwise.

            """
            return self.bluetooth_facade.pair_legacy_device(
                    device_address, pin, trusted,
                    self.ADAPTER_PAIRING_TIMEOUT_SECS)


        has_device = False
        paired = False
        if self.bluetooth_facade.has_device(device_address):
            has_device = True
            try:
                utils.poll_for_condition(
                        condition=_pair_device,
                        timeout=self.ADAPTER_PAIRING_TIMEOUT_SECS,
                        sleep_interval=self.ADAPTER_PAIRING_POLLING_SLEEP_SECS,
                        desc='Waiting for pairing %s' % device_address)
                paired = True
            except utils.TimeoutError as e:
                logging.error('test_pairing: %s', e)
            except:
                logging.error('test_pairing: unexpected error')

        self.results = {'has_device': has_device, 'paired': paired}
        return all(self.results.values())


    @_test_retry_and_log
    def test_remove_pairing(self, device_address):
        """Test that the adapter could remove the paired device.

        @param device_address: Address of the device.

        @returns: True if the device is removed successfully. False otherwise.

        """
        device_is_paired_initially = self.bluetooth_facade.device_is_paired(
                device_address)
        remove_pairing = False
        pairing_removed = False

        if device_is_paired_initially:
            remove_pairing = self.bluetooth_facade.remove_device_object(
                    device_address)
            pairing_removed = not self.bluetooth_facade.device_is_paired(
                    device_address)

        self.results = {
                'device_is_paired_initially': device_is_paired_initially,
                'remove_pairing': remove_pairing,
                'pairing_removed': pairing_removed}
        return all(self.results.values())


    def test_set_trusted(self, device_address, trusted=True):
        """Test whether the device with the specified address is trusted.

        @param device_address: Address of the device.
        @param trusted : True or False indicating if trusted is expected.

        @returns: True if the device's "Trusted" property is as specified;
                  False otherwise.

        """

        set_trusted = self.bluetooth_facade.set_trusted(
                device_address, trusted)

        properties = self.bluetooth_facade.get_device_properties(
                device_address)
        actual_trusted = properties.get('Trusted')

        self.results = {
                'set_trusted': set_trusted,
                'actual trusted': actual_trusted,
                'expected trusted': trusted}
        return actual_trusted == trusted


    @_test_retry_and_log
    def test_connection_by_adapter(self, device_address):
        """Test that the adapter of dut could connect to the device successfully

        It is the caller's responsibility to pair to the device before
        doing connection.

        @param device_address: Address of the device.

        @returns: True if connection is performed. False otherwise.

        """

        def _connect_device():
            """Connect to the device.

            @returns: True if it could connect to the device. False otherwise.

            """
            return self.bluetooth_facade.connect_device(device_address)


        has_device = False
        connected = False
        if self.bluetooth_facade.has_device(device_address):
            has_device = True
            try:
                utils.poll_for_condition(
                        condition=_connect_device,
                        timeout=self.ADAPTER_PAIRING_TIMEOUT_SECS,
                        sleep_interval=self.ADAPTER_PAIRING_POLLING_SLEEP_SECS,
                        desc='Waiting for connecting to %s' % device_address)
                connected = True
            except utils.TimeoutError as e:
                logging.error('test_connection_by_adapter: %s', e)
            except:
                logging.error('test_connection_by_adapter: unexpected error')

        self.results = {'has_device': has_device, 'connected': connected}
        return all(self.results.values())


    @_test_retry_and_log
    def test_disconnection_by_adapter(self, device_address):
        """Test that the adapter of dut could disconnect the device successfully

        @param device_address: Address of the device.

        @returns: True if disconnection is performed. False otherwise.

        """
        return self.bluetooth_facade.disconnect_device(device_address)


    def _enter_command_mode(self, device):
        """Let the device enter command mode.

        Before using the device, need to call this method to make sure
        it is in the command mode.

        @param device: the bluetooth HID device

        @returns: True if successful. False otherwise.

        """
        result = _is_successful(_run_method(device.EnterCommandMode,
                                            'EnterCommandMode'))
        if not result:
            logging.error('EnterCommandMode failed')
        return result


    @_test_retry_and_log
    def test_connection_by_device(self, device):
        """Test that the device could connect to the adapter successfully.

        This emulates the behavior that a device may initiate a
        connection request after waking up from power saving mode.

        @param device: the bluetooth HID device

        @returns: True if connection is performed correctly by device and
                  the adapter also enters connection state.
                  False otherwise.

        """
        if not self._enter_command_mode(device):
            return False

        method_name = 'test_connection_by_device'
        connection_by_device = False
        adapter_address = self.bluetooth_facade.address
        try:
            device.ConnectToRemoteAddress(adapter_address)
            connection_by_device = True
        except Exception as e:
            logging.error('%s (device): %s', method_name, e)
        except:
            logging.error('%s (device): unexpected error', method_name)

        connection_seen_by_adapter = False
        device_address = device.address
        device_is_connected = self.bluetooth_facade.device_is_connected
        try:
            utils.poll_for_condition(
                    condition=lambda: device_is_connected(device_address),
                    timeout=self.ADAPTER_CONNECTION_TIMEOUT_SECS,
                    desc=('Waiting for connection from %s' % device_address))
            connection_seen_by_adapter = True
        except utils.TimeoutError as e:
            logging.error('%s (adapter): %s', method_name, e)
        except:
            logging.error('%s (adapter): unexpected error', method_name)

        self.results = {
                'connection_by_device': connection_by_device,
                'connection_seen_by_adapter': connection_seen_by_adapter}
        return all(self.results.values())


    @_test_retry_and_log
    def test_disconnection_by_device(self, device):
        """Test that the device could disconnect the adapter successfully.

        This emulates the behavior that a device may initiate a
        disconnection request before going into power saving mode.

        Note: should not try to enter command mode in this method. When
              a device is connected, there is no way to enter command mode.
              One could just issue a special disconnect command without
              entering command mode.

        @param device: the bluetooth HID device

        @returns: True if disconnection is performed correctly by device and
                  the adapter also observes the disconnection.
                  False otherwise.

        """
        method_name = 'test_disconnection_by_device'
        disconnection_by_device = False
        try:
            device.Disconnect()
            disconnection_by_device = True
        except Exception as e:
            logging.error('%s (device): %s', method_name, e)
        except:
            logging.error('%s (device): unexpected error', method_name)

        disconnection_seen_by_adapter = False
        device_address = device.address
        device_is_connected = self.bluetooth_facade.device_is_connected
        try:
            utils.poll_for_condition(
                    condition=lambda: not device_is_connected(device_address),
                    timeout=self.ADAPTER_DISCONNECTION_TIMEOUT_SECS,
                    desc=('Waiting for disconnection from %s' % device_address))
            disconnection_seen_by_adapter = True
        except utils.TimeoutError as e:
            logging.error('%s (adapter): %s', method_name, e)
        except:
            logging.error('%s (adapter): unexpected error', method_name)

        self.results = {
                'disconnection_by_device': disconnection_by_device,
                'disconnection_seen_by_adapter': disconnection_seen_by_adapter}
        return all(self.results.values())

    @_test_retry_and_log
    def test_device_is_connected(self, device_address):
        """Test that device address given is currently connected.

        @param device_address: Address of the device.

        @returns: True if the device is connected.
                  False otherwise.

        """
        def _is_connected():
            """Test if device is connected.

            @returns: True if device is connected. False otherwise.

            """
            return self.bluetooth_facade.device_is_connected(device_address)


        method_name = 'test_device_is_connected'
        has_device = False
        connected = False
        if self.bluetooth_facade.has_device(device_address):
            has_device = True
            try:
                utils.poll_for_condition(
                        condition=_is_connected,
                        timeout=self.ADAPTER_CONNECTION_TIMEOUT_SECS,
                        sleep_interval=self.ADAPTER_PAIRING_POLLING_SLEEP_SECS,
                        desc='Waiting for connection to %s' % device_address)
                connected = True
            except utils.TimeoutError as e:
                logging.error('%s: %s', method_name, e)
            except:
                logging.error('%s: unexpected error', method_name)
        self.results = {'has_device': has_device, 'connected': connected}
        return all(self.results.values())


    @_test_retry_and_log
    def test_device_is_paired(self, device_address):
        """Test that the device address given is currently paired.

        @param device_address: Address of the device.

        @returns: True if the device is paired.
                  False otherwise.

        """
        def _is_paired():
            """Test if device is paired.

            @returns: True if device is paired. False otherwise.

            """
            return self.bluetooth_facade.device_is_paired(device_address)


        method_name = 'test_device_is_paired'
        has_device = False
        paired = False
        if self.bluetooth_facade.has_device(device_address):
            has_device = True
            try:
                utils.poll_for_condition(
                        condition=_is_paired,
                        timeout=self.ADAPTER_PAIRING_TIMEOUT_SECS,
                        sleep_interval=self.ADAPTER_PAIRING_POLLING_SLEEP_SECS,
                        desc='Waiting for connection to %s' % device_address)
                paired = True
            except utils.TimeoutError as e:
                logging.error('%s: %s', method_name, e)
            except:
                logging.error('%s: unexpected error', method_name)
        self.results = {'has_device': has_device, 'paired': paired}
        return all(self.results.values())


    def _get_device_name(self, device_address):
        """Get the device name.

        @returns: True if the device name is derived. None otherwise.

        """
        properties = self.bluetooth_facade.get_device_properties(
                device_address)
        self.discovered_device_name = properties.get('Name')
        return bool(self.discovered_device_name)


    @_test_retry_and_log
    def test_device_name(self, device_address, expected_device_name):
        """Test that the device name discovered by the adapter is correct.

        @param device_address: Address of the device.
        @param expected_device_name: the bluetooth device name

        @returns: True if the discovered_device_name is expected_device_name.
                  False otherwise.

        """
        try:
            utils.poll_for_condition(
                    condition=lambda: self._get_device_name(device_address),
                    timeout=self.ADAPTER_DISCOVER_NAME_TIMEOUT_SECS,
                    sleep_interval=self.ADAPTER_DISCOVER_POLLING_SLEEP_SECS,
                    desc='Waiting for device name of %s' % device_address)
        except utils.TimeoutError as e:
            logging.error('test_device_name: %s', e)
        except:
            logging.error('test_device_name: unexpected error')

        self.results = {
                'expected_device_name': expected_device_name,
                'discovered_device_name': self.discovered_device_name}
        return self.discovered_device_name == expected_device_name


    @_test_retry_and_log
    def test_device_class_of_service(self, device_address,
                                     expected_class_of_service):
        """Test that the discovered device class of service is as expected.

        @param device_address: Address of the device.
        @param expected_class_of_service: the expected class of service

        @returns: True if the discovered class of service matches the
                  expected class of service. False otherwise.

        """
        properties = self.bluetooth_facade.get_device_properties(
                device_address)
        device_class = properties.get('Class')
        discovered_class_of_service = (device_class & self.CLASS_OF_SERVICE_MASK
                                       if device_class else None)

        self.results = {
                'device_class': device_class,
                'expected_class_of_service': expected_class_of_service,
                'discovered_class_of_service': discovered_class_of_service}
        return discovered_class_of_service == expected_class_of_service


    @_test_retry_and_log
    def test_device_class_of_device(self, device_address,
                                    expected_class_of_device):
        """Test that the discovered device class of device is as expected.

        @param device_address: Address of the device.
        @param expected_class_of_device: the expected class of device

        @returns: True if the discovered class of device matches the
                  expected class of device. False otherwise.

        """
        properties = self.bluetooth_facade.get_device_properties(
                device_address)
        device_class = properties.get('Class')
        discovered_class_of_device = (device_class & self.CLASS_OF_DEVICE_MASK
                                      if device_class else None)

        self.results = {
                'device_class': device_class,
                'expected_class_of_device': expected_class_of_device,
                'discovered_class_of_device': discovered_class_of_device}
        return discovered_class_of_device == expected_class_of_device


    def _get_btmon_log(self, method, logging_timespan=1):
        """Capture the btmon log when executing the specified method.

        @param method: the method to capture log.
                       The method would be executed only when it is not None.
                       This allows us to simply capture btmon log without
                       executing any command.
        @param logging_timespan: capture btmon log for logging_timespan seconds.

        """
        self.bluetooth_le_facade.btmon_start()
        self.advertising_msg = method() if method else ''
        time.sleep(logging_timespan)
        self.bluetooth_le_facade.btmon_stop()


    def convert_to_adv_jiffies(self, adv_interval_ms):
        """Convert adv interval in ms to jiffies, i.e., multiples of 0.625 ms.

        @param adv_interval_ms: an advertising interval

        @returns: the equivalent jiffies

        """
        return adv_interval_ms / self.ADVERTISING_INTERVAL_UNIT


    def compute_duration(self, max_adv_interval_ms):
        """Compute duration from max_adv_interval_ms.

        Advertising duration is calculated approximately as
            duration = max_adv_interval_ms / 1000.0 * 1.1

        @param max_adv_interval_ms: max advertising interval in milliseconds.

        @returns: duration in seconds.

        """
        return max_adv_interval_ms / 1000.0 * 1.1


    def compute_logging_timespan(self, max_adv_interval_ms):
        """Compute the logging timespan from max_adv_interval_ms.

        The logging timespan is the time needed to record btmon log.

        @param max_adv_interval_ms: max advertising interval in milliseconds.

        @returns: logging_timespan in seconds.

        """
        duration = self.compute_duration(max_adv_interval_ms)
        logging_timespan = max(self.count_advertisements * duration, 1)
        return logging_timespan


    @_test_retry_and_log(False)
    def test_check_duration_and_intervals(self, min_adv_interval_ms,
                                          max_adv_interval_ms,
                                          number_advertisements):
        """Verify that every advertisements are scheduled according to the
        duration and intervals.

        An advertisement would be scheduled at the time span of
             duration * number_advertisements

        @param min_adv_interval_ms: min advertising interval in milliseconds.
        @param max_adv_interval_ms: max advertising interval in milliseconds.
        @param number_advertisements: the number of existing advertisements

        @returns: True if all advertisements are scheduled based on the
                duration and intervals.

        """


        def within_tolerance(expected, actual, max_error=0.1):
            """Determine if the percent error is within specified tolerance.

            @param expected: The expected value.
            @param actual: The actual (measured) value.
            @param max_error: The maximum percent error acceptable.

            @returns: True if the percent error is less than or equal to
                      max_error.
            """
            return abs(expected - actual) / abs(expected) <= max_error


        start_str = 'Set Advertising Intervals:'
        search_strings = ['HCI Command: LE Set Advertising Data', 'Company']
        search_str = '|'.join(search_strings)

        contents = self.bluetooth_le_facade.btmon_get(search_str=search_str,
                                                      start_str=start_str)

        # Company string looks like
        #   Company: not assigned (65283)
        company_pattern = re.compile('Company:.*\((\d*)\)')

        # The string with timestamp looks like
        #   < HCI Command: LE Set Advertising Data (0x08|0x0008) [hci0] 3.799236
        set_adv_time_str = 'LE Set Advertising Data.*\[hci\d\].*(\d+\.\d+)'
        set_adv_time_pattern = re.compile(set_adv_time_str)

        adv_timestamps = {}
        timestamp = None
        manufacturer_id = None
        for line in contents:
            result = set_adv_time_pattern.search(line)
            if result:
                timestamp = float(result.group(1))

            result = company_pattern.search(line)
            if result:
                manufacturer_id = '0x%04x' % int(result.group(1))

            if timestamp and manufacturer_id:
                if manufacturer_id not in adv_timestamps:
                    adv_timestamps[manufacturer_id] = []
                adv_timestamps[manufacturer_id].append(timestamp)
                timestamp = None
                manufacturer_id = None

        duration = self.compute_duration(max_adv_interval_ms)
        expected_timespan = duration * number_advertisements

        check_duration = True
        for manufacturer_id, values in adv_timestamps.iteritems():
            logging.debug('manufacturer_id %s: %s', manufacturer_id, values)
            timespans = [values[i] - values[i - 1]
                         for i in xrange(1, len(values))]
            errors = [timespans[i] for i in xrange(len(timespans))
                      if not within_tolerance(expected_timespan, timespans[i])]
            logging.debug('timespans: %s', timespans)
            logging.debug('errors: %s', errors)
            if bool(errors):
                check_duration = False

        # Verify that the advertising intervals are also correct.
        min_adv_interval_ms_found, max_adv_interval_ms_found = (
                self._verify_advertising_intervals(min_adv_interval_ms,
                                                   max_adv_interval_ms))

        self.results = {
                'check_duration': check_duration,
                'max_adv_interval_ms_found': max_adv_interval_ms_found,
                'max_adv_interval_ms_found': max_adv_interval_ms_found,
        }
        return all(self.results.values())


    def _get_min_max_intervals_strings(self, min_adv_interval_ms,
                                       max_adv_interval_ms):
        """Get the min and max advertising intervals strings shown in btmon.

        Advertising intervals shown in the btmon log look like
            Min advertising interval: 1280.000 msec (0x0800)
            Max advertising interval: 1280.000 msec (0x0800)

        @param min_adv_interval_ms: min advertising interval in milliseconds.
        @param max_adv_interval_ms: max advertising interval in milliseconds.

        @returns: the min and max intervals strings.

        """
        min_str = ('Min advertising interval: %.3f msec (0x%04x)' %
                   (min_adv_interval_ms,
                    min_adv_interval_ms / self.ADVERTISING_INTERVAL_UNIT))
        logging.debug('min_adv_interval_ms: %s', min_str)

        max_str = ('Max advertising interval: %.3f msec (0x%04x)' %
                   (max_adv_interval_ms,
                    max_adv_interval_ms / self.ADVERTISING_INTERVAL_UNIT))
        logging.debug('max_adv_interval_ms: %s', max_str)

        return (min_str, max_str)


    def _verify_advertising_intervals(self, min_adv_interval_ms,
                                      max_adv_interval_ms):
        """Verify min and max advertising intervals.

        Advertising intervals look like
            Min advertising interval: 1280.000 msec (0x0800)
            Max advertising interval: 1280.000 msec (0x0800)

        @param min_adv_interval_ms: min advertising interval in milliseconds.
        @param max_adv_interval_ms: max advertising interval in milliseconds.

        @returns: a tuple of (True, True) if both min and max advertising
            intervals could be found. Otherwise, the corresponding element
            in the tuple if False.

        """
        min_str, max_str = self._get_min_max_intervals_strings(
                min_adv_interval_ms, max_adv_interval_ms)

        min_adv_interval_ms_found = self.bluetooth_le_facade.btmon_find(min_str)
        max_adv_interval_ms_found = self.bluetooth_le_facade.btmon_find(max_str)

        return min_adv_interval_ms_found, max_adv_interval_ms_found


    @_test_retry_and_log(False)
    def test_register_advertisement(self, advertisement_data, instance_id,
                                    min_adv_interval_ms, max_adv_interval_ms):
        """Verify that an advertisement is registered correctly.

        This test verifies the following data:
        - advertisement added
        - manufacturer data
        - service UUIDs
        - service data
        - advertising intervals
        - advertising enabled

        @param advertisement_data: the data of an advertisement to register.
        @param instance_id: the instance id which starts at 1.
        @param min_adv_interval_ms: min_adv_interval in milliseconds.
        @param max_adv_interval_ms: max_adv_interval in milliseconds.

        @returns: True if the advertisement is registered correctly.
                  False otherwise.

        """
        # When registering a new advertisement, it is possible that another
        # instance is advertising. It may need to wait for all other
        # advertisements to complete advertising once.
        self.count_advertisements += 1
        logging_timespan = self.compute_logging_timespan(max_adv_interval_ms)
        self._get_btmon_log(
                lambda: self.bluetooth_le_facade.register_advertisement(
                        advertisement_data),
                logging_timespan=logging_timespan)

        # Verify that a new advertisement is added.
        advertisement_added = (
                self.bluetooth_le_facade.btmon_find('Advertising Added') and
                self.bluetooth_le_facade.btmon_find('Instance: %d' %
                                                    instance_id))

        # Verify that the manufacturer data could be found.
        manufacturer_data = advertisement_data.get('ManufacturerData', '')
        for manufacturer_id in manufacturer_data:
            # The 'not assigned' text below means the manufacturer id
            # is not actually assigned to any real manufacturer.
            # For real 16-bit manufacturer UUIDs, refer to
            #  https://www.bluetooth.com/specifications/assigned-numbers/16-bit-UUIDs-for-Members
            manufacturer_data_found = self.bluetooth_le_facade.btmon_find(
                    'Company: not assigned (%d)' % int(manufacturer_id, 16))

        # Verify that all service UUIDs could be found.
        service_uuids_found = True
        for uuid in advertisement_data.get('ServiceUUIDs', []):
            # Service UUIDs looks like ['0x180D', '0x180F']
            #   Heart Rate (0x180D)
            #   Battery Service (0x180F)
            # For actual 16-bit service UUIDs, refer to
            #   https://www.bluetooth.com/specifications/gatt/services
            if not self.bluetooth_le_facade.btmon_find('0x%s' % uuid):
                service_uuids_found = False
                break

        # Verify service data.
        service_data_found = True
        for uuid, data in advertisement_data.get('ServiceData', {}).items():
            # A service data looks like
            #   Service Data (UUID 0x9999): 0001020304
            # while uuid is '9999' and data is [0x00, 0x01, 0x02, 0x03, 0x04]
            data_str = ''.join(map(lambda n: '%02x' % n, data))
            if not self.bluetooth_le_facade.btmon_find(
                    'Service Data (UUID 0x%s): %s' % (uuid, data_str)):
                service_data_found = False
                break

        # Verify that the advertising intervals are correct.
        min_adv_interval_ms_found, max_adv_interval_ms_found = (
                self._verify_advertising_intervals(min_adv_interval_ms,
                                                   max_adv_interval_ms))

        # Verify advertising is enabled.
        advertising_enabled = self.bluetooth_le_facade.btmon_find(
                'Advertising: Enabled (0x01)')

        self.results = {
                'advertisement_added': advertisement_added,
                'manufacturer_data_found': manufacturer_data_found,
                'service_uuids_found': service_uuids_found,
                'service_data_found': service_data_found,
                'min_adv_interval_ms_found': min_adv_interval_ms_found,
                'max_adv_interval_ms_found': max_adv_interval_ms_found,
                'advertising_enabled': advertising_enabled,
        }
        return all(self.results.values())


    @_test_retry_and_log(False)
    def test_fail_to_register_advertisement(self, advertisement_data,
                                            min_adv_interval_ms,
                                            max_adv_interval_ms):
        """Verify that failure is incurred when max advertisements are reached.

        This test verifies that a registration failure is incurred when
        max advertisements are reached. The error message looks like:

            org.bluez.Error.Failed: Maximum advertisements reached

        @param advertisement_data: the advertisement to register.
        @param min_adv_interval_ms: min_adv_interval in milliseconds.
        @param max_adv_interval_ms: max_adv_interval in milliseconds.

        @returns: True if the error message is received correctly.
                  False otherwise.

        """
        logging_timespan = self.compute_logging_timespan(max_adv_interval_ms)
        self._get_btmon_log(
                lambda: self.bluetooth_le_facade.register_advertisement(
                        advertisement_data),
                logging_timespan=logging_timespan)

        # Verify that it failed to register advertisement due to the fact
        # that max advertisements are reached.
        failed_to_register_error = (self.ERROR_FAILED_TO_REGISTER_ADVERTISEMENT
                                    in self.advertising_msg)

        # Verify that no new advertisement is added.
        advertisement_not_added = not self.bluetooth_le_facade.btmon_find(
                'Advertising Added:')

        # Verify that the advertising intervals are correct.
        min_adv_interval_ms_found, max_adv_interval_ms_found = (
                self._verify_advertising_intervals(min_adv_interval_ms,
                                                   max_adv_interval_ms))

        # Verify advertising remains enabled.
        advertising_enabled = self.bluetooth_le_facade.btmon_find(
                'Advertising: Enabled (0x01)')

        self.results = {
                'failed_to_register_error': failed_to_register_error,
                'advertisement_not_added': advertisement_not_added,
                'min_adv_interval_ms_found': min_adv_interval_ms_found,
                'max_adv_interval_ms_found': max_adv_interval_ms_found,
                'advertising_enabled': advertising_enabled,
        }
        return all(self.results.values())


    @_test_retry_and_log(False)
    def test_unregister_advertisement(self, advertisement_data, instance_id,
                                      advertising_disabled):
        """Verify that an advertisement is unregistered correctly.

        This test verifies the following data:
        - advertisement removed
        - advertising status: enabled if there are advertisements left;
                              disabled otherwise.

        @param advertisement_data: the data of an advertisement to unregister.
        @param instance_id: the instance id of the advertisement to remove.
        @param advertising_disabled: is advertising disabled? This happens
                only when all advertisements are removed.

        @returns: True if the advertisement is unregistered correctly.
                  False otherwise.

        """
        self.count_advertisements -= 1
        self._get_btmon_log(
                lambda: self.bluetooth_le_facade.unregister_advertisement(
                        advertisement_data))

        # Verify that the advertisement is removed.
        advertisement_removed = (
                self.bluetooth_le_facade.btmon_find('Advertising Removed') and
                self.bluetooth_le_facade.btmon_find('Instance: %d' %
                                                    instance_id))

        # If advertising_disabled is True, there should be no log like
        #       'Advertising: Enabled (0x01)'
        # If advertising_disabled is False, there should be log like
        #       'Advertising: Enabled (0x01)'

        # Only need to check advertising status when the last advertisement
        # is removed. For any other advertisements prior to the last one,
        # we may or may not observe 'Advertising: Enabled (0x01)' message.
        # Hence, the test would become flaky if we insist to see that message.
        # A possible workaround is to sleep for a while and then check the
        # message. The drawback is that we may need to wait up to 10 seconds
        # if the advertising duration and intervals are long.
        # In a test case, we always run test_check_duration_and_intervals()
        # to check if advertising duration and intervals are correct after
        # un-registering one or all advertisements, it is safe to do so.
        advertising_enabled_found = self.bluetooth_le_facade.btmon_find(
                'Advertising: Enabled (0x01)')
        advertising_disabled_found = self.bluetooth_le_facade.btmon_find(
                'Advertising: Disabled (0x00)')
        advertising_status_correct = not advertising_disabled or (
                advertising_disabled_found and not advertising_enabled_found)

        self.results = {
                'advertisement_removed': advertisement_removed,
                'advertising_status_correct': advertising_status_correct,
        }
        return all(self.results.values())


    @_test_retry_and_log(False)
    def test_set_advertising_intervals(self, min_adv_interval_ms,
                                       max_adv_interval_ms):
        """Verify that new advertising intervals are set correctly.

        Note that setting advertising intervals does not enable/disable
        advertising. Hence, there is no need to check the advertising
        status.

        @param min_adv_interval_ms: the min advertising interval in ms.
        @param max_adv_interval_ms: the max advertising interval in ms.

        @returns: True if the new advertising intervals are correct.
                  False otherwise.

        """
        self._get_btmon_log(
                lambda: self.bluetooth_le_facade.set_advertising_intervals(
                        min_adv_interval_ms, max_adv_interval_ms))

        # Verify the new advertising intervals.
        # With intervals of 200 ms and 200 ms, the log looks like
        #   bluetoothd: Set Advertising Intervals: 0x0140, 0x0140
        txt = 'bluetoothd: Set Advertising Intervals: 0x%04x, 0x%04x'
        adv_intervals_found = self.bluetooth_le_facade.btmon_find(
                txt % (self.convert_to_adv_jiffies(min_adv_interval_ms),
                       self.convert_to_adv_jiffies(max_adv_interval_ms)))

        self.results = {'adv_intervals_found': adv_intervals_found}
        return all(self.results.values())


    @_test_retry_and_log(False)
    def test_fail_to_set_advertising_intervals(
            self, invalid_min_adv_interval_ms, invalid_max_adv_interval_ms,
            orig_min_adv_interval_ms, orig_max_adv_interval_ms):
        """Verify that setting invalid advertising intervals results in error.

        If invalid min/max advertising intervals are given, it would incur
        the error: 'org.bluez.Error.InvalidArguments: Invalid arguments'.
        Note that valid advertising intervals fall between 20 ms and 10,240 ms.

        @param invalid_min_adv_interval_ms: the invalid min advertising interval
                in ms.
        @param invalid_max_adv_interval_ms: the invalid max advertising interval
                in ms.
        @param orig_min_adv_interval_ms: the original min advertising interval
                in ms.
        @param orig_max_adv_interval_ms: the original max advertising interval
                in ms.

        @returns: True if it fails to set invalid advertising intervals.
                  False otherwise.

        """
        self._get_btmon_log(
                lambda: self.bluetooth_le_facade.set_advertising_intervals(
                        invalid_min_adv_interval_ms,
                        invalid_max_adv_interval_ms))

        # Verify that the invalid error is observed in the dbus error callback
        # message.
        invalid_intervals_error = (self.ERROR_INVALID_ADVERTISING_INTERVALS in
                                   self.advertising_msg)

        # Verify that the min/max advertising intervals remain the same
        # after setting the invalid advertising intervals.
        #
        # In btmon log, we would see the following message first.
        #    bluetoothd: Set Advertising Intervals: 0x0010, 0x0010
        # And then, we should check if "Min advertising interval" and
        # "Max advertising interval" remain the same.
        start_str = 'bluetoothd: Set Advertising Intervals: 0x%04x, 0x%04x' % (
                self.convert_to_adv_jiffies(invalid_min_adv_interval_ms),
                self.convert_to_adv_jiffies(invalid_max_adv_interval_ms))

        search_strings = ['Min advertising interval:',
                          'Max advertising interval:']
        search_str = '|'.join(search_strings)

        contents = self.bluetooth_le_facade.btmon_get(search_str=search_str,
                                                      start_str=start_str)

        # The min/max advertising intervals of all advertisements should remain
        # the same as the previous valid ones.
        min_max_str = '[Min|Max] advertising interval: (\d*\.\d*) msec'
        min_max_pattern = re.compile(min_max_str)
        correct_orig_min_adv_interval = True
        correct_orig_max_adv_interval = True
        for line in contents:
            result = min_max_pattern.search(line)
            if result:
                interval = float(result.group(1))
                if 'Min' in line and interval != orig_min_adv_interval_ms:
                    correct_orig_min_adv_interval = False
                elif 'Max' in line and interval != orig_max_adv_interval_ms:
                    correct_orig_max_adv_interval = False

        self.results = {
                'invalid_intervals_error': invalid_intervals_error,
                'correct_orig_min_adv_interval': correct_orig_min_adv_interval,
                'correct_orig_max_adv_interval': correct_orig_max_adv_interval}

        return all(self.results.values())


    @_test_retry_and_log(False)
    def test_check_advertising_intervals(self, min_adv_interval_ms,
                                         max_adv_interval_ms):
        """Verify that the advertising intervals are as expected.

        @param min_adv_interval_ms: the min advertising interval in ms.
        @param max_adv_interval_ms: the max advertising interval in ms.

        @returns: True if the advertising intervals are correct.
                  False otherwise.

        """
        self._get_btmon_log(None)

        # Verify that the advertising intervals are correct.
        min_adv_interval_ms_found, max_adv_interval_ms_found = (
                self._verify_advertising_intervals(min_adv_interval_ms,
                                                   max_adv_interval_ms))

        self.results = {
                'min_adv_interval_ms_found': min_adv_interval_ms_found,
                'max_adv_interval_ms_found': max_adv_interval_ms_found,
        }
        return all(self.results.values())


    @_test_retry_and_log(False)
    def test_reset_advertising(self, instance_ids=[]):
        """Verify that advertising is reset correctly.

        Note that reset advertising would set advertising intervals to
        the default values. However, we would not be able to observe
        the values change until new advertisements are registered.
        Therefore, it is required that a test_register_advertisement()
        test is conducted after this test.

        If instance_ids is [], all advertisements would still be removed
        if there are any. However, no need to check 'Advertising Removed'
        in btmon log since we may or may not be able to observe the message.
        This feature is needed if this test is invoked as the first one in
        a test case to reset advertising. In this situation, this test does
        not know how many advertisements exist.

        @param instance_ids: the list of instance IDs that should be removed.

        @returns: True if advertising is reset correctly.
                  False otherwise.

        """
        self.count_advertisements = 0
        self._get_btmon_log(
                lambda: self.bluetooth_le_facade.reset_advertising())

        # Verify that every advertisement is removed. When an advertisement
        # with instance id 1 is removed, the log looks like
        #   Advertising Removed
        #       instance: 1
        if len(instance_ids) > 0:
            advertisement_removed = self.bluetooth_le_facade.btmon_find(
                    'Advertising Removed')
            if advertisement_removed:
                for instance_id in instance_ids:
                    txt = 'Instance: %d' % instance_id
                    if not self.bluetooth_le_facade.btmon_find(txt):
                        advertisement_removed = False
                        break
        else:
            advertisement_removed = True

        if not advertisement_removed:
            logging.error('Failed to remove advertisement')

        # Verify that "Reset Advertising Intervals" command has been issued.
        reset_advertising_intervals = self.bluetooth_le_facade.btmon_find(
                'bluetoothd: Reset Advertising Intervals')

        # Verify the advertising is disabled.
        advertising_disabled_observied = self.bluetooth_le_facade.btmon_find(
                'Advertising: Disabled')
        # If there are no existing advertisements, we may not observe
        # 'Advertising: Disabled'.
        advertising_disabled = (instance_ids == [] or
                                advertising_disabled_observied)

        self.results = {
                'advertisement_removed': advertisement_removed,
                'reset_advertising_intervals': reset_advertising_intervals,
                'advertising_disabled': advertising_disabled,
        }
        return all(self.results.values())


    # -------------------------------------------------------------------
    # Bluetooth mouse related tests
    # -------------------------------------------------------------------


    def _record_input_events(self, device, gesture):
        """Record the input events.

        @param device: the bluetooth HID device.
        @param gesture: the gesture method to perform.

        @returns: the input events received on the DUT.

        """
        self.input_facade.initialize_input_recorder(device.name)
        self.input_facade.start_input_recorder()
        time.sleep(self.HID_REPORT_SLEEP_SECS)
        gesture()
        time.sleep(self.HID_REPORT_SLEEP_SECS)
        self.input_facade.stop_input_recorder()
        time.sleep(self.HID_REPORT_SLEEP_SECS)
        event_values = self.input_facade.get_input_events()
        events = [Event(*ev) for ev in event_values]
        return events


    def _test_mouse_click(self, device, button):
        """Test that the mouse click events could be received correctly.

        @param device: the meta device containing a bluetooth HID device
        @param button: which button to test, 'LEFT' or 'RIGHT'

        @returns: True if the report received by the host matches the
                  expected one. False otherwise.

        """
        if button == 'LEFT':
            gesture = device.LeftClick
        elif button == 'RIGHT':
            gesture = device.RightClick
        else:
            raise error.TestError('Button (%s) is not valid.' % button)

        actual_events = self._record_input_events(device, gesture)

        linux_input_button = {'LEFT': BTN_LEFT, 'RIGHT': BTN_RIGHT}
        expected_events = [
                # Button down
                recorder.MSC_SCAN_BTN_EVENT[button],
                Event(EV_KEY, linux_input_button[button], 1),
                recorder.SYN_EVENT,
                # Button up
                recorder.MSC_SCAN_BTN_EVENT[button],
                Event(EV_KEY, linux_input_button[button], 0),
                recorder.SYN_EVENT]

        self.results = {
                'actual_events': map(str, actual_events),
                'expected_events': map(str, expected_events)}
        return actual_events == expected_events


    @_test_retry_and_log
    def test_mouse_left_click(self, device):
        """Test that the mouse left click events could be received correctly.

        @param device: the meta device containing a bluetooth HID device

        @returns: True if the report received by the host matches the
                  expected one. False otherwise.

        """
        return self._test_mouse_click(device, 'LEFT')


    @_test_retry_and_log
    def test_mouse_right_click(self, device):
        """Test that the mouse right click events could be received correctly.

        @param device: the meta device containing a bluetooth HID device

        @returns: True if the report received by the host matches the
                  expected one. False otherwise.

        """
        return self._test_mouse_click(device, 'RIGHT')


    def _test_mouse_move(self, device, delta_x=0, delta_y=0):
        """Test that the mouse move events could be received correctly.

        @param device: the meta device containing a bluetooth HID device
        @param delta_x: the distance to move cursor in x axis
        @param delta_y: the distance to move cursor in y axis

        @returns: True if the report received by the host matches the
                  expected one. False otherwise.

        """
        gesture = lambda: device.Move(delta_x, delta_y)
        actual_events = self._record_input_events(device, gesture)

        events_x = [Event(EV_REL, REL_X, delta_x)] if delta_x else []
        events_y = [Event(EV_REL, REL_Y, delta_y)] if delta_y else []
        expected_events = events_x + events_y + [recorder.SYN_EVENT]

        self.results = {
                'actual_events': map(str, actual_events),
                'expected_events': map(str, expected_events)}
        return actual_events == expected_events


    @_test_retry_and_log
    def test_mouse_move_in_x(self, device, delta_x):
        """Test that the mouse move events in x could be received correctly.

        @param device: the meta device containing a bluetooth HID device
        @param delta_x: the distance to move cursor in x axis

        @returns: True if the report received by the host matches the
                  expected one. False otherwise.

        """
        return self._test_mouse_move(device, delta_x=delta_x)


    @_test_retry_and_log
    def test_mouse_move_in_y(self, device, delta_y):
        """Test that the mouse move events in y could be received correctly.

        @param device: the meta device containing a bluetooth HID device
        @param delta_y: the distance to move cursor in y axis

        @returns: True if the report received by the host matches the
                  expected one. False otherwise.

        """
        return self._test_mouse_move(device, delta_y=delta_y)


    @_test_retry_and_log
    def test_mouse_move_in_xy(self, device, delta_x, delta_y):
        """Test that the mouse move events could be received correctly.

        @param device: the meta device containing a bluetooth HID device
        @param delta_x: the distance to move cursor in x axis
        @param delta_y: the distance to move cursor in y axis

        @returns: True if the report received by the host matches the
                  expected one. False otherwise.

        """
        return self._test_mouse_move(device, delta_x=delta_x, delta_y=delta_y)


    def _test_mouse_scroll(self, device, units):
        """Test that the mouse wheel events could be received correctly.

        @param device: the meta device containing a bluetooth HID device
        @param units: the units to scroll in y axis

        @returns: True if the report received by the host matches the
                  expected one. False otherwise.

        """
        gesture = lambda: device.Scroll(units)
        actual_events = self._record_input_events(device, gesture)
        expected_events = [Event(EV_REL, REL_WHEEL, units), recorder.SYN_EVENT]
        self.results = {
                'actual_events': map(str, actual_events),
                'expected_events': map(str, expected_events)}
        return actual_events == expected_events


    @_test_retry_and_log
    def test_mouse_scroll_down(self, device, delta_y):
        """Test that the mouse wheel events could be received correctly.

        @param device: the meta device containing a bluetooth HID device
        @param delta_y: the units to scroll down in y axis;
                        should be a postive value

        @returns: True if the report received by the host matches the
                  expected one. False otherwise.

        """
        if delta_y > 0:
            return self._test_mouse_scroll(device, delta_y)
        else:
            raise error.TestError('delta_y (%d) should be a positive value',
                                  delta_y)


    @_test_retry_and_log
    def test_mouse_scroll_up(self, device, delta_y):
        """Test that the mouse wheel events could be received correctly.

        @param device: the meta device containing a bluetooth HID device
        @param delta_y: the units to scroll up in y axis;
                        should be a postive value

        @returns: True if the report received by the host matches the
                  expected one. False otherwise.

        """
        if delta_y > 0:
            return self._test_mouse_scroll(device, -delta_y)
        else:
            raise error.TestError('delta_y (%d) should be a positive value',
                                  delta_y)


    @_test_retry_and_log
    def test_mouse_click_and_drag(self, device, delta_x, delta_y):
        """Test that the mouse click-and-drag events could be received
        correctly.

        @param device: the meta device containing a bluetooth HID device
        @param delta_x: the distance to drag in x axis
        @param delta_y: the distance to drag in y axis

        @returns: True if the report received by the host matches the
                  expected one. False otherwise.

        """
        gesture = lambda: device.ClickAndDrag(delta_x, delta_y)
        actual_events = self._record_input_events(device, gesture)

        button = 'LEFT'
        expected_events = (
                [# Button down
                 recorder.MSC_SCAN_BTN_EVENT[button],
                 Event(EV_KEY, BTN_LEFT, 1),
                 recorder.SYN_EVENT] +
                # cursor movement in x and y
                ([Event(EV_REL, REL_X, delta_x)] if delta_x else []) +
                ([Event(EV_REL, REL_Y, delta_y)] if delta_y else []) +
                [recorder.SYN_EVENT] +
                # Button up
                [recorder.MSC_SCAN_BTN_EVENT[button],
                 Event(EV_KEY, BTN_LEFT, 0),
                 recorder.SYN_EVENT])

        self.results = {
                'actual_events': map(str, actual_events),
                'expected_events': map(str, expected_events)}
        return actual_events == expected_events


    # -------------------------------------------------------------------
    # Autotest methods
    # -------------------------------------------------------------------


    def initialize(self):
        """Initialize bluetooth adapter tests."""
        # Run through every tests and collect failed tests in self.fails.
        self.fails = []

        # If a test depends on multiple conditions, write the results of
        # the conditions in self.results so that it is easy to know
        # what conditions failed by looking at the log.
        self.results = None

        # Some tests may instantiate a peripheral device for testing.
        self.devices = dict()
        for device_type in SUPPORTED_DEVICE_TYPES:
            self.devices[device_type] = None

        # The count of registered advertisements.
        self.count_advertisements = 0


    def check_chameleon(self):
        """Check the existence of chameleon_host.

        The chameleon_host is specified in --args as follows

        (cr) $ test_that --args "chameleon_host=$CHAMELEON_IP" "$DUT_IP" <test>

        """
        if self.host.chameleon is None:
            raise error.TestError('Have to specify chameleon_host IP.')


    def run_once(self, *args, **kwargs):
        """This method should be implemented by children classes.

        Typically, the run_once() method would look like:

        factory = remote_facade_factory.RemoteFacadeFactory(host)
        self.bluetooth_facade = factory.create_bluetooth_hid_facade()

        self.test_bluetoothd_running()
        # ...
        # invoke more self.test_xxx() tests.
        # ...

        if self.fails:
            raise error.TestFail(self.fails)

        """
        raise NotImplementedError


    def cleanup(self):
        """Clean up bluetooth adapter tests."""
        # Close the device properly if a device is instantiated.
        # Note: do not write something like the following statements
        #           if self.devices[device_type]:
        #       or
        #           if bool(self.devices[device_type]):
        #       Otherwise, it would try to invoke bluetooth_mouse.__nonzero__()
        #       which just does not exist.
        for device_type in SUPPORTED_DEVICE_TYPES:
            if self.devices[device_type] is not None:
                self.devices[device_type].Close()
