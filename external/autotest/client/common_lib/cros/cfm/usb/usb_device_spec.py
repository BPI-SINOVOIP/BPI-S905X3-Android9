"""Utility class representing the spec for a USB device.

The file cfm_usb_devices.py lists all known USB device specs.
"""

class UsbDeviceSpec(object):
  """Utility class representing the spec for a CfM USB device."""

  # Dictionary of all UsbDeviceSpec instance that have been created.
  # Mapping from vid_pid to UsbDeviceSpec instance.
  _all_specs = {}

  def __init__(self, vid, pid, product, interfaces):
      """
      Constructor.

      @param vid: Vendor ID. String.
      @param pid: Product ID. String.
      @param product: Product description. String
      @param interfaces: List of strings
      """
      self._vid = vid
      self._pid = pid
      self._product = product
      self._interfaces = interfaces
      self.__class__._all_specs[self.vid_pid] = self

  @classmethod
  def get_usb_device_spec(cls, vid_pid):
      """Looks up UsbDeviceSpec by vid_pid."""
      return cls._all_specs.get(vid_pid)

  @property
  def vendor_id(self):
      """Returns the vendor id for this USB device."""
      return self._vid

  @property
  def product_id(self):
      """Returns the product id for this USB device."""
      return self._pid

  @property
  def vid_pid(self):
      """Return the <vendor_id>:<product_id> as a string."""
      return '%s:%s' % (self._vid, self._pid)

  @property
  def product(self):
      """Returns the product name."""
      return self._product

  @property
  def interfaces(self):
      """Returns the list of interfaces."""
      return self._interfaces

  def __str__(self):
      return self.__repr__()

  def __repr__(self):
      return "%s (%s)" % (self._product, self.vid_pid)
