import unittest

from autotest_lib.client.common_lib.cros.cfm.usb import usb_device
from autotest_lib.client.common_lib.cros.cfm.usb import usb_device_spec

# pylint: disable=missing-docstring

class UsbDeviceTest(unittest.TestCase):
  """Unit tests for UsbDevice."""

  def setUp(self):
      self._usb_device = usb_device.UsbDevice(
          vid='vid',
          pid='pid',
          product='product',
          interfaces=['a', 'b'],
          bus=1,
          level=1,
          port=2)

  def test_vendor_id(self):
      self.assertEqual(self._usb_device.vendor_id, 'vid')

  def test_product_id(self):
      self.assertEqual(self._usb_device.product_id, 'pid')

  def test_product(self):
      self.assertEqual(self._usb_device.product, 'product')

  def test_vid_pid(self):
      self.assertEqual(self._usb_device.vid_pid, 'vid:pid')

  def test_bus(self):
      self.assertEqual(self._usb_device.bus, 1)

  def test_port(self):
      self.assertEqual(self._usb_device.port, 2)

  def test_level(self):
     self.assertEqual(self._usb_device.level, 1)

  def test_get_parent(self):
      child = usb_device.UsbDevice(
          vid='vid1',
          pid='pid1',
          product='product',
          interfaces=['a', 'b'],
          bus=1,
          level=2,
          port=2,
          parent=self._usb_device)
      self.assertEquals(child.get_parent(1).vid_pid, 'vid:pid')

  def test_get_parent_error(self):
      self.assertRaises(ValueError, lambda: self._usb_device.get_parent(2))

  def test_interfaces_matches_spec(self):
      device_spec = usb_device_spec.UsbDeviceSpec(
          vid='vid',
          pid='pid',
          product='product',
          interfaces=['a', 'b'])
      self.assertTrue(self._usb_device.interfaces_match_spec(device_spec))
