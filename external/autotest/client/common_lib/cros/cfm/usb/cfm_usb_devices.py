"""CfM USB device constants.

This module contains constants for known USB device specs.

A UsbDeviceSpec instance represents a known USB device and its spec;
 - VendorID
 - ProdID
 - interfaces

This is different from a UsbDevice instance which represents a device actually
connected to the CfM and found by the usb-device command.

A UsbDevice instance found connected to a CfM is expected to match a known
UsbDeviceSpec (mapping is done using vid:pid), but due to bugs that might
not be the case (list of interfaces might be different for example).
"""

from autotest_lib.client.common_lib.cros.cfm.usb import usb_device_spec

# Cameras
HUDDLY_GO = usb_device_spec.UsbDeviceSpec(
    vid='2bd9',
    pid='0011',
    product='Huddly GO',
    interfaces=['uvcvideo', 'uvcvideo', 'uvcvideo', 'uvcvideo'],
)

LOGITECH_WEBCAM_C930E = usb_device_spec.UsbDeviceSpec(
    vid='046d',
    pid='0843',
    product='Logitech Webcam C930e',
    interfaces=['uvcvideo', 'uvcvideo', 'snd-usb-audio', 'snd-usb-audio']
)

HD_PRO_WEBCAM_C920 = usb_device_spec.UsbDeviceSpec(
    vid='046d',
    pid='082d',
    product='HD Pro Webcam C920',
    interfaces=['uvcvideo', 'uvcvideo', 'snd-usb-audio', 'snd-usb-audio'],
)

PTZ_PRO_CAMERA = usb_device_spec.UsbDeviceSpec(
    vid='046d',
    pid='0853',
    product='PTZ Pro Camera',
    interfaces=['uvcvideo', 'uvcvideo', 'usbhid'],
)

PTZ_PRO_2_CAMERA = usb_device_spec.UsbDeviceSpec(
    vid='046d',
    pid='085f',
    product='PTZ Pro 2 Camera',
    interfaces=['uvcvideo', 'uvcvideo', 'usbhid'],
)

# Audio peripherals
ATRUS = usb_device_spec.UsbDeviceSpec(
    vid='18d1',
    pid='8001',
    product='Hangouts Meet speakermic',
    interfaces=['snd-usb-audio', 'snd-usb-audio', 'snd-usb-audio', 'usbhid'],
)

JABRA_SPEAK_410 = usb_device_spec.UsbDeviceSpec(
    vid='0b0e',
    pid='0412',
    product='Jabra SPEAK 410',
    interfaces=['snd-usb-audio', 'snd-usb-audio', 'snd-usb-audio'],
)

# MiMOs
MIMO_VUE_HD_DISPLAY = usb_device_spec.UsbDeviceSpec(
    vid='17e9',
    pid='016b',
    product='MIMO VUE HD',
    interfaces=['udl'],
)

# The MiMO's firmware is tied to the Chrome OS version. The firmware was updated
# in Chrome OS 65.0.3319.0. This resulted in the PID being changed from 016b to
# 416d. The following device is the device with the new PID. We need to support
# both versions since we want to support tests at the ToT revision running
# against older Chrome OS versions.
MIMO_VUE_HD_DISPLAY_PLANKTON = usb_device_spec.UsbDeviceSpec(
    vid='17e9',
    pid='416d',
    product='MIMO VUE HD',
    interfaces=['udl'],
)

# Tuple with all known MIMO display specs that we support.
ALL_MIMO_DISPLAYS = (MIMO_VUE_HD_DISPLAY, MIMO_VUE_HD_DISPLAY_PLANKTON)

MIMO_VUE_HID_TOUCH_CONTROLLER = usb_device_spec.UsbDeviceSpec(
    vid='266e',
    pid='0110',
    product='SiS HID Touch Controller',
    interfaces=['usbhid'],
)

# Utility methods
def get_usb_device_spec(vid_pid):
  """
  Look up UsbDeviceSpec based on vid_pid.
  @return UsbDeviceSpec with matching vid_pid or None if no match.
  """
  return usb_device_spec.UsbDeviceSpec.get_usb_device_spec(vid_pid)
