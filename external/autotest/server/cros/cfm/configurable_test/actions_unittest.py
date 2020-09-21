import mock
import unittest

from autotest_lib.client.common_lib.cros.cfm.usb import usb_device
from autotest_lib.client.common_lib.cros.cfm.usb import usb_device_spec
from autotest_lib.server.cros.cfm.configurable_test import actions
from autotest_lib.server.cros.cfm.configurable_test import action_context
from autotest_lib.server.cros.cfm.configurable_test import scenario

# Constants to use in case the actual values are irrelevant.
USB_DEVICE_SPEC = usb_device_spec.UsbDeviceSpec(
        'vid', 'pid', 'product', ['iface'])

USB_DEVICE = usb_device.UsbDevice('v', 'p', 'prod', ['if'], 1, 2, 1)


# Test, disable missing-docstring
# pylint: disable=missing-docstring
class TestActions(unittest.TestCase):
    """
    Tests for the available actions for configurable CFM tests to run.
    """

    def setUp(self):
        self.host_mock = mock.MagicMock()
        self.cfm_facade_mock = mock.MagicMock()
        self.usb_device_collector_mock = mock.MagicMock()
        self.usb_port_manager_mock = mock.MagicMock()
        self.crash_detector_mock = mock.MagicMock()
        self.context_with_mocks = action_context.ActionContext(
                host=self.host_mock,
                cfm_facade=self.cfm_facade_mock,
                usb_device_collector=self.usb_device_collector_mock,
                usb_port_manager=self.usb_port_manager_mock,
                crash_detector=self.crash_detector_mock)


    def test_assert_file_does_not_contain_no_match(self):
        action = actions.AssertFileDoesNotContain('/foo', ['EE', 'WW'])
        context = action_context.ActionContext(
                file_contents_collector=FakeCollector('abc\ndef'))
        action.execute(context)

    def test_assert_file_does_not_contain_match(self):
        action = actions.AssertFileDoesNotContain('/foo', ['EE', 'WW'])
        context = action_context.ActionContext(
                file_contents_collector=FakeCollector('abc\naWWd'))
        self.assertRaises(AssertionError, lambda: action.execute(context))

    def test_assert_file_does_not_contain_regex_match(self):
        action = actions.AssertFileDoesNotContain('/foo', ['EE', 'W{3}Q+'])
        context = action_context.ActionContext(
                file_contents_collector=FakeCollector('abc\naWWWQQd'))
        self.assertRaises(AssertionError, lambda: action.execute(context))

    def test_reboot_dut_no_restart(self):
        action = actions.RebootDut()
        action.execute(self.context_with_mocks)
        self.host_mock.reboot.assert_called_once_with()
        self.assertFalse(self.cfm_facade_mock.method_calls)

    def test_reboot_dut_with_restart(self):
        action = actions.RebootDut(restart_chrome_for_cfm=True)
        action.execute(self.context_with_mocks)
        self.host_mock.reboot.assert_called_once_with()
        (self.cfm_facade_mock.restart_chrome_for_cfm
                .assert_called_once_with())
        (self.cfm_facade_mock.wait_for_meetings_telemetry_commands
                .assert_called_once_with())

    def test_assert_usb_device_collector(self):
        spec = usb_device_spec.UsbDeviceSpec(
                'vid', 'pid', 'product', ['iface'])
        action = actions.AssertUsbDevices([spec], lambda x: True)
        action.execute(self.context_with_mocks)

    def test_assert_usb_device_collector_matching_predicate(self):
        spec = usb_device_spec.UsbDeviceSpec(
                'vid', 'pid', 'product', ['iface'])
        device = usb_device.UsbDevice(
                'v', 'p', 'prod', ['if'], 1, 2, 1)
        self.usb_device_collector_mock.get_devices_by_spec = mock.Mock(
                return_value=[device])
        action = actions.AssertUsbDevices(
                [spec], lambda x: x[0].product_id == 'p')
        action.execute(self.context_with_mocks)

    def test_assert_usb_device_collector_non_matching_predicate(self):
        spec = usb_device_spec.UsbDeviceSpec(
                'vid', 'pid', 'product', ['iface'])
        device = usb_device.UsbDevice(
                'v', 'p', 'prod', ['if'], 1, 2, 1)
        self.usb_device_collector_mock.get_devices_by_spec = mock.Mock(
                return_value=[device])
        action = actions.AssertUsbDevices(
                [spec], lambda x: x[0].product_id == 'r')
        self.assertRaises(AssertionError, lambda: action.execute(
                self.context_with_mocks))

    def test_assert_usb_device_collector_default_predicate(self):
        self.usb_device_collector_mock.get_devices_by_spec = mock.Mock(
                return_value=[USB_DEVICE])  # Default checks list is of size 1
        action = actions.AssertUsbDevices([USB_DEVICE_SPEC])
        action.execute(self.context_with_mocks)

    def test_select_scenario_at_random(self):
        dummy_action1 = DummyAction()
        dummy_action2 = DummyAction()
        scenarios = [scenario.Scenario(dummy_action1),
                     scenario.Scenario(dummy_action2)]
        action = actions.SelectScenarioAtRandom(scenarios, 10)
        action.execute(self.context_with_mocks)
        # Assert that our actions were executed the expected number of times.
        total_executes = (dummy_action1.executed_times
                          + dummy_action2.executed_times)
        self.assertEqual(10, total_executes)

    def test_select_scenario_at_random_str_contains_seed(self):
        action = actions.SelectScenarioAtRandom([], 10, 123)
        self.assertTrue('seed=123' in str(action))

    def test_select_scenario_at_random_same_seed_same_actions(self):
        scenario1_action1 = DummyAction()
        scenario1_action2 = DummyAction()
        scenarios1 = [scenario.Scenario(scenario1_action1),
                     scenario.Scenario(scenario1_action2)]
        scenario2_action1 = DummyAction()
        scenario2_action2 = DummyAction()
        scenarios2 = [scenario.Scenario(scenario2_action1),
                     scenario.Scenario(scenario2_action2)]
        action1 = actions.SelectScenarioAtRandom(scenarios1, 100, 0)
        action2 = actions.SelectScenarioAtRandom(scenarios2, 100, 0)
        action1.execute(self.context_with_mocks)
        action2.execute(self.context_with_mocks)
        self.assertEqual(scenario1_action1.executed_times,
                         scenario2_action1.executed_times)
        self.assertEqual(scenario1_action2.executed_times,
                         scenario2_action2.executed_times)

    def test_power_cycle_usb_port(self):
        device = usb_device.UsbDevice(
                'v', 'p', 'prod', ['if'], 1, 2, 1)
        self.usb_device_collector_mock.get_devices_by_spec = mock.Mock(
                side_effect=[[device, device], [device], [device, device]])
        action = actions.PowerCycleUsbPort(
                [USB_DEVICE_SPEC], 0, lambda x: [x[0]])
        action.execute(self.context_with_mocks)
        self.usb_port_manager_mock.set_port_power.assert_has_calls(
                [mock.call([(1, 2)], False), mock.call([(1, 2)], True)])

    def test_power_cycle_usb_port_device_does_not_turn_off(self):
        # Return the same device all the time - i.e., it does not turn off.
        self.usb_device_collector_mock.get_devices_by_spec = mock.Mock(
                return_value=[USB_DEVICE])
        action = actions.PowerCycleUsbPort([USB_DEVICE_SPEC], 0)
        self.assertRaises(
                actions.TimeoutError,
                lambda: action.execute(self.context_with_mocks))

    def test_power_cycle_usb_port_device_does_not_turn_on(self):
        self.usb_device_collector_mock.get_devices_by_spec = mock.Mock(
                side_effect=[[USB_DEVICE, USB_DEVICE], [], [USB_DEVICE]])
        action = actions.PowerCycleUsbPort([USB_DEVICE_SPEC], 0)
        self.assertRaises(
                actions.TimeoutError,
                lambda: action.execute(self.context_with_mocks))

    def test_retry_action_success_after_retry(self):
        action = actions.RetryAssertAction(RaisesFirstTimeAction(), 3, 0)
        action.execute(self.context_with_mocks)

    def test_retry_action_fail_when_no_more_retries(self):
        action = actions.RetryAssertAction(RaisesFirstTimeAction(), 1)
        self.assertRaises(
                AssertionError, lambda: action.execute(self.context_with_mocks))

    def test_assert_no_new_crashes(self):
        action = actions.AssertNoNewCrashes()
        self.crash_detector_mock.get_new_crash_files = mock.Mock(
                return_value=[])
        action.do_execute(self.context_with_mocks)

    def test_assert_no_new_crashes_crash_detected(self):
        action = actions.AssertNoNewCrashes()
        self.crash_detector_mock.get_new_crash_files = mock.Mock(
                return_value=['/a/new/crash/file'])
        self.assertRaises(
                AssertionError,
                lambda: action.do_execute(self.context_with_mocks))


class FakeCollector(object):
    def __init__(self, contents):
        self.contents = contents

    def collect_file_contents(self, path):
        return self.contents

class DummyAction(actions.Action):
    def __init__(self):
        self.executed_times = 0

    def do_execute(self, context):
        self.executed_times += 1

class RaisesFirstTimeAction(actions.Action):
    def __init__(self):
        self.executed = False

    def do_execute(self, context):
        if not self.executed:
            self.executed = True
            raise AssertionError()

