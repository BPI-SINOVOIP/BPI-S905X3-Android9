"""Bluetooth tests."""



from mobly import asserts
from mobly import test_runner
from mobly import utils
from utils import android_base_test

# Number of seconds for the target to stay discoverable on Bluetooth.
DISCOVERABLE_TIME = 180
# Random DATA to represent device bluetooth name.
DEVICE_NAME = utils.rand_ascii_str(8)
# UUID value for RfComm connection
RFCOMM_UUID = 'fa87c0d0-afac-11de-8a39-0800200c9a66'


def DiscoverBluetoothDeviceByName(device, name):
  """Start a bluetooth scan and find the device that has the bluetooth name.

  Args:
    device: AndroidDevice. Device to start bluetooth scan.
    name: the bluetooth name looking for.

  Returns:
    Dict represents a bluetooth device if found.
  """
  discovered_devices = device.android.btDiscoverAndGetResults()
  for device in discovered_devices:
    if name == device['Name']:
      device_discovered = device
      return device_discovered
  asserts.fail('Failed to discover the target device %s over Bluetooth.' % name)


class BluetoothTest(android_base_test.AndroidBaseTest):
  """Bluetooth tests."""

  def setup_class(self):
    super(BluetoothTest, self).setup_class()
    self.initiator = self.dut_a
    # Sets the tag that represents this device in logs.
    self.initiator.debug_tag = 'initiator'
    # The device that is expected to be discovered and receive messages.
    self.receiver = self.dut_b
    self.receiver.debug_tag = 'receiver'

  def setup_test(self):
    # Make sure bluetooth is on.
    self.initiator.android.btEnable()
    self.receiver.android.btEnable()
    # Set Bluetooth name on target device.
    self.receiver.android.btSetName(DEVICE_NAME)

  def test_bluetooth_process(self):
    """Test for basic bluetooth rfcomm process flow.

    Steps:
      1. Receiver becomes discoverable.
      2. Initiator discovers receiver via bluetooth.
      3. Initiator connects to receiver via rfcomm profile.
      4. Initiator sends a message to receiver and receiver receives the exact
      message.

    Verifies:
      Receiver receives the correct message.
    """
    # Name value for RfComm connection
    rfcomm_name = utils.rand_ascii_str(8)
    self.receiver.connection_callback = (
        self.receiver.android.btRfcommStartServer(rfcomm_name, RFCOMM_UUID))
    self.receiver.log.info('Start Rfcomm server with name: %s uuid: %s' %
                           (rfcomm_name, RFCOMM_UUID))
    target_name = self.receiver.android.btGetName()
    self.receiver.log.info('Become discoverable with name "%s" for %ds.',
                           target_name, DISCOVERABLE_TIME)
    self.receiver.android.btBecomeDiscoverable(DISCOVERABLE_TIME)
    self.initiator.log.info('Looking for Bluetooth devices.')
    discovered_device = DiscoverBluetoothDeviceByName(
        self.initiator, target_name)
    self.initiator.log.info('Target device is found. Device: %s'
                            % discovered_device)
    remote_address = discovered_device['Address']
    self.initiator.android.btRfcommConnect(remote_address, RFCOMM_UUID)
    self.receiver.connection_callback.waitAndGet('onAccepted', 30)
    # self.initiator.connection_callback.waitAndGet('onConnected', 30)
    self.initiator.log.info('Connection established')
    # Random data to be sent through bluetooth rfcomm.
    data = utils.rand_ascii_str(8)
    self.receiver.read_callback = self.receiver.android.btRfcommRead()
    self.initiator.android.btRfcommWrite(data)
    read_result = self.receiver.read_callback.waitAndGet('onDataAvailable', 30)
    asserts.assert_equal(read_result.data['Data'], data)
    self.receiver.log.info('Received correct message from the other side')
    self.initiator.android.btRfcommDisconnect()
    self.receiver.android.btRfcommStopServer()

  def teardown_test(self):
    # Turn Bluetooth off on both devices after test finishes.
    self.receiver.android.btDisable()
    self.initiator.android.btDisable()


if __name__ == '__main__':
  test_runner.main()
