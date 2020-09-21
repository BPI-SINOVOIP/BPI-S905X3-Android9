import mock
import unittest

from autotest_lib.client.common_lib.cros.cfm.usb import usb_port_manager

# gpio for the port on bus 1, port 2. Specified in
# usb_port_manager._PORT_ID_TO_GPIO_INDEX_DICT
GPIO = 218
GPIO_PATH = '/sys/class/gpio/gpio%s' % GPIO

# pylint: disable=missing-docstring
class UsbPortManagerTest(unittest.TestCase):

    def test_power_off_gpio_unexported(self):
        host = mock.Mock()
        host.get_board = mock.Mock(return_value='board: guado')
        host.run = mock.Mock()
        host.path_exists = mock.Mock(return_value=False)
        port_manager = usb_port_manager.UsbPortManager(host)
        port_manager.set_port_power([(1, 2)], False)
        expected_runs = [
                mock.call('echo %s > /sys/class/gpio/export' %  GPIO),
                mock.call('echo out > %s/direction' % GPIO_PATH),
                mock.call('echo 0 > %s/value' % GPIO_PATH),
                mock.call('echo %s > /sys/class/gpio/unexport' %  GPIO),
        ]
        host.run.assert_has_calls(expected_runs)

    def test_power_on_gpio_exported(self):
        host = mock.Mock()
        host.get_board = mock.Mock(return_value='board: guado')
        host.run = mock.Mock()
        host.path_exists = mock.Mock(return_value=True)
        port_manager = usb_port_manager.UsbPortManager(host)
        port_manager.set_port_power([(1, 2)], True)
        expected_runs = [
                mock.call('echo out > %s/direction' % GPIO_PATH),
                mock.call('echo 1 > %s/value' % GPIO_PATH),
        ]
        host.run.assert_has_calls(expected_runs)
