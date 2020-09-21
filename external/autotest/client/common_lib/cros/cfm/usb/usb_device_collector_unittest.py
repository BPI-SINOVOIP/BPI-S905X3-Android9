import mock
import unittest

from autotest_lib.client.common_lib.cros.cfm.usb import usb_device_collector
from autotest_lib.client.common_lib.cros.cfm.usb import usb_device_spec


# pylint: disable=missing-docstring
class UsbDeviceCollectorTest(unittest.TestCase):
    """Unit test for the class UsbDeviceCollector."""

    def setUp(self):
        # Sample stdout from `usb-devices`.
        usb_devices = (
            '\n'
            'T:  Bus=01 Lev=01 Prnt=01 Port=01 Cnt=01 Dev#=  8 Spd=12  '
                'MxCh= 0\n'
            'D:  Ver= 2.00 Cls=00(>ifc ) Sub=00 Prot=00 MxPS=64 #Cfgs=  1\n'
            'P:  Vendor=0000 ProdID=aaaa Rev=01.01\n'
            'S:  Manufacturer=Google Inc.\n'
            'S:  Product=FOO\n'
            'S:  SerialNumber=GATRW17340078\n'
            'C:  #Ifs= 5 Cfg#= 1 Atr=80 MxPwr=500mA\n'
            'I:  If#= 0 Alt= 0 #EPs= 0 Cls=0e(video) Sub=01 Prot=00 '
                'Driver=int-a\n'
            'I:  If#= 1 Alt= 0 #EPs= 0 Cls=fe(app. ) Sub=01 Prot=01 '
                'Driver=(none)\n'
            '\n'
            'T:  Bus=01 Lev=02 Prnt=08 Port=03 Cnt=01 Dev#=  4 Spd=5000 '
                'MxCh= 0\n'
            'D:  Ver= 3.10 Cls=ef(misc ) Sub=02 Prot=01 MxPS= 9 #Cfgs=  1\n'
            'P:  Vendor=0a0a ProdID=9f9f Rev=06.00\n'
            'S:  Manufacturer=Huddly\n'
            'S:  Product=BAR\n'
            'S:  SerialNumber=43F00190\n'
            'C:  #Ifs= 4 Cfg#= 1 Atr=80 MxPwr=752mA\n'
            'I:  If#= 0 Alt= 0 #EPs= 0 Cls=0e(video) Sub=01 Prot=00 '
                'Driver=int-a\n'
            'I:  If#= 1 Alt= 0 #EPs= 1 Cls=0e(video) Sub=02 Prot=00 '
                'Driver=int-a\n'
            'I:  If#= 2 Alt= 0 #EPs= 0 Cls=0e(video) Sub=01 Prot=00 '
                'Driver=int-b\n')

        mock_host = mock.Mock()
        mock_host.run.return_value.stdout = usb_devices
        self.collector = usb_device_collector.UsbDeviceCollector(mock_host)

    def test_get_usb_devices(self):
        """Unit test for get_usb_devices."""
        usb_data = self.collector.get_usb_devices()
        self.assertEqual(2, len(usb_data))

        foo_device = usb_data[0]
        self.assertEqual(foo_device.vendor_id, '0000')
        self.assertEqual(foo_device.product_id, 'aaaa')
        self.assertEqual(foo_device.product, 'FOO')
        self.assertEqual(foo_device.interfaces, ['int-a', '(none)'])
        self.assertEqual(foo_device.bus, 1)
        self.assertEqual(foo_device.port, 2)
        self.assertIsNone(foo_device.parent)

        bar_device = usb_data[1]
        self.assertEqual(bar_device.vendor_id, '0a0a')
        self.assertEqual(bar_device.product_id, '9f9f')
        self.assertEqual(bar_device.product, 'BAR')
        self.assertEqual(bar_device.interfaces, ['int-a', 'int-a', 'int-b'])
        self.assertEqual(bar_device.bus, 1)
        self.assertEqual(bar_device.port, 4)

        self.assertEqual(bar_device.parent, foo_device)

    def test_get_devices_by_spec(self):
        spec = usb_device_spec.UsbDeviceSpec('0a0a', '9f9f', 'PRODUCT', [])
        devices = self.collector.get_devices_by_spec(spec)
        self.assertEquals(len(devices), 1)

    def test_get_devices_by_specs(self):
        specs = (usb_device_spec.UsbDeviceSpec('0a0a', '9f9f', 'PRODUCT', []),
                 usb_device_spec.UsbDeviceSpec('0000', 'aaaa', 'PRODUCT', []))
        devices = self.collector.get_devices_by_spec(*specs)
        self.assertEquals(len(devices), 2)

    def test_get_devices_by_not_matching_specs(self):
        specs = (usb_device_spec.UsbDeviceSpec('no', 'match', 'PRODUCT', []),
                 usb_device_spec.UsbDeviceSpec('nono', 'match', 'PRODUCT', []))
        devices = self.collector.get_devices_by_spec(*specs)
        self.assertEquals(devices, [])

if __name__ == "__main__":
    unittest.main()
