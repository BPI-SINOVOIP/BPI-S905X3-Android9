"""Utility class representing a CfM USB device.

This class represents actual data found by running the usb-device command.
"""

class UsbDevice(object):
  """Utility class representing a CfM USB device."""

  def __init__(self,
               vid,
               pid,
               product,
               interfaces,
               bus,
               port,
               level,
               parent=None):
      """
      Constructor.

      @param vid: Vendor ID. String.
      @param pid: Product ID. String.
      @param product: Product description. String
      @param interfaces: List of strings.
      @param bus: The bus this device is connected to. Number.
      @param port: The port number as specified in /sys/bus/usb/devices/usb*.
          Number.
      @param level: The level in the device hierarchy this device is connected
          at. A device connected directly to a port is typically at level 1.
      @param parent: Optional parent UsbDevice. A parent device is a device that
          this device is connected to, typically a USB Hub.
      """
      self._vid = vid
      self._pid = pid
      self._product = product
      self._interfaces = interfaces
      self._bus = bus
      self._port = port
      self._level = level
      self._parent = parent

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

  @property
  def port(self):
      """Returns the port this USB device is connected to."""
      return self._port

  @property
  def bus(self):
      """Returns the bus this USB device is connected to."""
      return self._bus

  @property
  def level(self):
      """Returns the level of this USB Device."""
      return self._level

  @property
  def parent(self):
      """
      Returns the parent device of this device.

      Or None if this is the top level device.
      @returns the parent or None.
      """
      return self._parent

  @parent.setter
  def parent(self, value):
      """
      Sets the parent device of this device.

      We allow setting parents to make it easier to create the device tree.
      @param value the new parent.
      """
      self._parent = value

  def interfaces_match_spec(self, usb_device_spec):
      """
      Checks that the interfaces of this device matches those of the given spec.

      @param usb_device_spec an instance of UsbDeviceSpec
      @returns True or False
      """
      # List of expected interfaces. This might be a sublist of the actual
      # list of interfaces. Note: we have to use lists and not sets since
      # the list of interfaces might contain duplicates.
      expected_interfaces = sorted(usb_device_spec.interfaces)
      length = len(expected_interfaces)
      actual_interfaces = sorted(self.interfaces)
      return actual_interfaces[0:length] == expected_interfaces

  def get_parent(self, level):
      """
      Gets the parent device at the specified level.

      Devices are connected in a hierarchy. Typically like this:
      Level 0: Machine's internal USB hub
       |
       +--+ Level 1: Device connected to the machine's physical ports.
         |
         +--+ Level 2: If level 1 is a Hub, this is a device connected to
                       that Hub.

      A typical application of this method is when power cycling. Then we get a
      device's parent at level 1 to locate the port that should be power cycled.

      @param level the level of the parent to return.
      @returns A UsbDevice instance of the parent at the specified level.
      @raises ValueError if we did not find a device at the specified level.
      """
      device = self
      while device != None:
          if device.level < level:
              raise ValueError(
                  'Reached lower level without finding level %d', level)
          if device.level == level:
              return device
          device = device.parent

  def __str__(self):
      return "%s (%s)" % (self._product, self.vid_pid)

  def __repr__(self):
      return "%s (%s), bus=%s, port=%s, parent=%s" % (
              self._product, self.vid_pid, self._bus, self._port, self.parent)
