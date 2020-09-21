import unittest

from autotest_lib.client.common_lib.cros.cfm.usb import usb_device_spec

# pylint: disable=missing-docstring

class UsbDeviceSpecTest(unittest.TestCase):
  """Unit tests for UsbDeviceSpec."""

  def setUp(self):
      self._spec = usb_device_spec.UsbDeviceSpec(
          vid='vid',
          pid='pid',
          product='product',
          interfaces=['a', 'b'])

  def test_vendor_id(self):
      self.assertEqual(self._spec.vendor_id, 'vid')

  def test_product_id(self):
      self.assertEqual(self._spec.product_id, 'pid')

  def test_product(self):
      self.assertEqual(self._spec.product, 'product')

  def test_vid_pid(self):
      self.assertEqual(self._spec.vid_pid, 'vid:pid')

  def test_get_usb_device_spec(self):
      self.assertEqual(self._spec,
                       usb_device_spec.UsbDeviceSpec.get_usb_device_spec(
                           self._spec.vid_pid))
