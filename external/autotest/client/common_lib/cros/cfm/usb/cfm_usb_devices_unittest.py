import unittest

from autotest_lib.client.common_lib.cros.cfm.usb import cfm_usb_devices

# pylint: disable=missing-docstring

class CfmUsbDevicesTest(unittest.TestCase):
  """Unit tests for cfm_usb_devices."""

  def test_get_usb_device(self):
      device_spec = cfm_usb_devices.HUDDLY_GO
      self.assertEqual(cfm_usb_devices.get_usb_device_spec(device_spec.vid_pid),
          device_spec)
