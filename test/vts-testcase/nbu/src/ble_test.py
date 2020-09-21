"""BLE tests."""

import time



from mobly import asserts
from mobly import test_runner
from mobly import utils
from utils import android_base_test

# Number of seconds for the target to stay BLE advertising.
ADVERTISING_TIME = 120
# The number of seconds to wait for receiving scan results.
SCAN_TIMEOUT = 5
# The number of seconds to wair for connection established.
CONNECTION_TIMEOUT = 20
# The number of seconds to wait before cancel connection.
CANCEL_CONNECTION_WAIT_TIME = 0.1
# UUID for test service.
TEST_BLE_SERVICE_UUID = '0000fe23-0000-1000-8000-00805f9b34fb'
# UUID for write characteristic.
TEST_WRITE_UUID = '0000e632-0000-1000-8000-00805f9b34fb'
# UUID for second write characteristic.
TEST_SECOND_WRITE_UUID = '0000e633-0000-1000-8000-00805f9b34fb'
# UUID for read test.
TEST_READ_UUID = '0000e631-0000-1000-8000-00805f9b34fb'
# UUID for second read characteristic.
TEST_SECOND_READ_UUID = '0000e634-0000-1000-8000-00805f9b34fb'
# UUID for third read characteristic.
TEST_THIRD_READ_UUID = '0000e635-0000-1000-8000-00805f9b34fb'
# UUID for scan response.
TEST_SCAN_RESPONSE_UUID = '0000e639-0000-1000-8000-00805f9b34fb'
# Advertise settings in json format for Ble Advertise.
ADVERTISE_SETTINGS = {
    'AdvertiseMode': 'ADVERTISE_MODE_LOW_LATENCY',
    'Timeout': ADVERTISING_TIME * 1000,
    'Connectable': True,
    'TxPowerLevel': 'ADVERTISE_TX_POWER_ULTRA_LOW'
}
# Ramdom data to represent device stored in advertise data.
DATA = utils.rand_ascii_str(16)
# Random data for scan response.
SCAN_RESPONSE_DATA = utils.rand_ascii_str(16)
# Random data for read operation.
READ_DATA = utils.rand_ascii_str(8)
# Random data for second read operation.
SECOND_READ_DATA = utils.rand_ascii_str(8)
# Random data for third read operation.
THIRD_READ_DATA = utils.rand_ascii_str(8)
# Random data for write operation.
WRITE_DATA = utils.rand_ascii_str(8)
# Random data for second write operation.
SECOND_WRITE_DATA = utils.rand_ascii_str(8)
# Advertise data in json format for BLE advertise.
ADVERTISE_DATA = {
    'IncludeDeviceName': False,
    'ServiceData': [{
        'UUID': TEST_BLE_SERVICE_UUID,
        'Data': DATA
    }]
}
# Advertise data in json format representing scan response for BLE advertise.
SCAN_RESPONSE = {
    'InlcudeDeviceName':
        False,
    'ServiceData': [{
        'UUID': TEST_SCAN_RESPONSE_UUID,
        'Data': SCAN_RESPONSE_DATA
    }]
}
# Scan filter in json format for BLE scan.
SCAN_FILTER = {'ServiceUuid': TEST_BLE_SERVICE_UUID}
# Scan settings in json format for BLE scan.
SCAN_SETTINGS = {'ScanMode': 'SCAN_MODE_LOW_LATENCY'}
# Characteristics for write in json format.
WRITE_CHARACTERISTIC = {
    'UUID': TEST_WRITE_UUID,
    'Property': 'PROPERTY_WRITE',
    'Permission': 'PERMISSION_WRITE'
}
SECOND_WRITE_CHARACTERISTIC = {
    'UUID': TEST_SECOND_WRITE_UUID,
    'Property': 'PROPERTY_WRITE',
    'Permission': 'PERMISSION_WRITE'
}
# Characteristics for read in json format.
READ_CHARACTERISTIC = {
    'UUID': TEST_READ_UUID,
    'Property': 'PROPERTY_READ',
    'Permission': 'PERMISSION_READ',
    'Data': READ_DATA
}
SECOND_READ_CHARACTERISTIC = {
    'UUID': TEST_SECOND_READ_UUID,
    'Property': 'PROPERTY_READ',
    'Permission': 'PERMISSION_READ',
    'Data': SECOND_READ_DATA
}
THIRD_READ_CHARACTERISTIC = {
    'UUID': TEST_THIRD_READ_UUID,
    'Property': 'PROPERTY_READ',
    'Permission': 'PERMISSION_READ',
    'Data': THIRD_READ_DATA
}
# Service data in json format for Ble Server.
SERVICE = {
    'UUID':
        TEST_BLE_SERVICE_UUID,
    'Type':
        'SERVICE_TYPE_PRIMARY',
    'Characteristics': [
        WRITE_CHARACTERISTIC, SECOND_WRITE_CHARACTERISTIC, READ_CHARACTERISTIC,
        SECOND_READ_CHARACTERISTIC, THIRD_READ_CHARACTERISTIC
    ]
}
# Macros for literal string.
UUID = 'UUID'
GATT_SUCCESS = 'GATT_SUCCESS'
STATE = 'newState'
STATUS = 'status'


def IsRequiredScanResult(scan_result):
  result = scan_result.data['result']
  for service in result['ScanRecord']['Services']:
    if service[UUID] == TEST_BLE_SERVICE_UUID and service['Data'] == DATA:
      return True
  return False


def Discover(scanner, advertiser):
  """Logic for BLE scan and advertise.

  Args:
    scanner: AndroidDevice. The device that starts ble scan to find target.
    advertiser: AndroidDevice. The device that keeps advertising so other
    devices acknowledge it.

  Steps:
    1. Advertiser starts advertising and gets a startSuccess callback.
    2. Scanner starts scanning and finds advertiser from scan results.

  Verifies:
    Advertiser is discovered within 5s by scanner.
  """
  advertiser.advertise_callback = advertiser.android.bleStartAdvertising(
      ADVERTISE_SETTINGS, ADVERTISE_DATA, SCAN_RESPONSE)
  scanner.scan_callback = scanner.android.bleStartScan([SCAN_FILTER],
                                                       SCAN_SETTINGS)
  advertiser.advertise_callback.waitAndGet('onStartSuccess', 30)
  advertiser.log.info('BLE advertising started')
  time.sleep(SCAN_TIMEOUT)
  scan_result = scanner.scan_callback.waitForEvent(
      'onScanResult', IsRequiredScanResult, SCAN_TIMEOUT)
  scan_success = False
  scan_response_found = False
  result = scan_result.data['result']
  for service in result['ScanRecord']['Services']:
    if service[UUID] == TEST_BLE_SERVICE_UUID and service['Data'] == DATA:
      scanner.connect_to_address = result['Device']['Address']
      scan_success = True
    if (service[UUID] == TEST_SCAN_RESPONSE_UUID and
        service['Data'] == SCAN_RESPONSE_DATA):
      scan_response_found = True
  asserts.assert_true(
      scan_success, 'Advertiser is not found inside %d seconds' % SCAN_TIMEOUT)
  asserts.assert_true(scan_response_found, 'Scan response is not found')
  scanner.log.info('Advertiser is found')


def StopDiscover(scanner, advertiser):
  """Logic for stopping BLE scan and advertise.

  Args:
    scanner: AndroidDevice. The device that starts ble scan to find target.
    advertiser: AndroidDevice. The device that keeps advertising so other
    devices acknowledge it.

  Steps:
    1. Scanner stops scanning.
    2. Advertiser stops advertising.
  """
  scanner.android.bleStopScan(scanner.scan_callback.callback_id)
  scanner.log.info('BLE scanning stopped')
  advertiser.android.bleStopAdvertising(
      advertiser.advertise_callback.callback_id)
  advertiser.log.info('BLE advertising stopped')


def Connect(client, server):
  """Logic for starting BLE client and server.

  Args:
    client: AndroidDevice. The device that behaves as GATT client.
    server: AndroidDevice. The device that behaves as GATT server.

  Steps:
    1. Server starts and service added properly.
    2. Client connects to server via Gatt, connection completes with
    GATT_SUCCESS within some TIMEOUT, onConnectionStateChange/STATE_CONNECTED
    called EXACTLY once.

  Verifies:
    Client gets corresponding callback.
  """
  server.server_callback = server.android.bleStartServer([SERVICE])
  start_server_result = server.server_callback.waitAndGet('onServiceAdded', 30)
  asserts.assert_equal(start_server_result.data[STATUS], GATT_SUCCESS)
  uuids = [
      characteristic[UUID]
      for characteristic in start_server_result.data['Service'][
          'Characteristics']
  ]
  for uuid in [
      characteristic[UUID] for characteristic in SERVICE['Characteristics']
  ]:
    asserts.assert_true(uuid in uuids, 'Failed to find uuid %s.' % uuid)
  server.log.info('BLE server started')
  client.client_callback = client.android.bleConnectGatt(
      client.connect_to_address)
  time.sleep(CONNECTION_TIMEOUT)
  start_client_results = client.client_callback.getAll(
      'onConnectionStateChange')
  asserts.assert_equal(len(start_client_results), 1)
  for start_client_result in start_client_results:
    asserts.assert_equal(start_client_result.data[STATUS], GATT_SUCCESS)
    asserts.assert_equal(start_client_result.data[STATE], 'STATE_CONNECTED')
  client.log.info('BLE client connected')


def CancelOpen(client, server):
  """Logic for BLE client cancel open and reconnect.

  Args:
    client: AndroidDevice. The device that behaves as GATT client.
    server: AndroidDevice. The device that behaves as GATT server.

  Steps:
    1. Server starts and service added properly.
    2. Client stars to connect to server via Gatt, but the connection has not
    been established.
    3. Client calls disconnect to cancel the connection.
    4. Client connects to server, connection completes with GATT_SUCCESS within
    some TIMEOUT, onConnectionStateChange/STATE_CONNECTEDcalled EXACTLY once.

  Verifies:
    Client gets corresponding callback.
  """
  server.server_callback = server.android.bleStartServer([SERVICE])
  start_server_result = server.server_callback.waitAndGet('onServiceAdded', 30)
  asserts.assert_equal(start_server_result.data[STATUS], GATT_SUCCESS)
  uuids = [
      characteristic[UUID]
      for characteristic in start_server_result.data['Service'][
          'Characteristics']
  ]
  for uuid in [
      characteristic[UUID] for characteristic in SERVICE['Characteristics']
  ]:
    asserts.assert_true(uuid in uuids, 'Failed to find uuid %s.' % uuid)
  server.log.info('BLE server started')
  client.client_callback = client.android.bleConnectGatt(
      client.connect_to_address)
  time.sleep(CANCEL_CONNECTION_WAIT_TIME)
  start_client_results = client.client_callback.getAll(
      'onConnectionStateChange')
  if not start_client_results:
    client.android.bleDisconnect()
    client.log.info('BLE client cancel open')
    time.sleep(CANCEL_CONNECTION_WAIT_TIME)
    client.client_callback = client.android.bleConnectGatt(
        client.connect_to_address)
    time.sleep(CONNECTION_TIMEOUT)
    start_client_results = client.client_callback.getAll(
        'onConnectionStateChange')
    asserts.assert_equal(len(start_client_results), 1)
    for start_client_result in start_client_results:
      asserts.assert_equal(start_client_result.data[STATUS], GATT_SUCCESS)
      asserts.assert_equal(start_client_result.data[STATE], 'STATE_CONNECTED')
      client.log.info('BLE client connected')


def Disconnect(client, server):
  """Logic for stopping BLE client and server.

  Args:
    client: AndroidDevice. The device that behaves as GATT client.
    server: AndroidDevice. The device that behaves as GATT server.

  Steps:
    1. Client calls disconnect, gets a callback with STATE_DISCONNECTED
    and GATT_SUCCESS.
    2. Server closes.

  Verifies:
    Client gets corresponding callback.
  """
  client.android.bleDisconnect()
  stop_client_result = client.client_callback.waitAndGet(
      'onConnectionStateChange', 30)
  asserts.assert_equal(stop_client_result.data[STATUS], GATT_SUCCESS)
  asserts.assert_equal(stop_client_result.data[STATE], 'STATE_DISCONNECTED')
  client.log.info('BLE client disconnected')
  server.android.bleStopServer()
  server.log.info('BLE server stopped')


def DiscoverServices(client):
  """Logic for BLE services discovery.

  Args:
    client: AndroidDevice. The device that behaves as GATT client.

  Steps:
    1. Client successfully completes service discovery & gets
    onServicesDiscovered callback within some TIMEOUT, onServicesDiscovered/
    GATT_SUCCESS called EXACTLY once.
    2. Client discovers the readable and writable characteristics.

  Verifies:
    Client gets corresponding callback.
  """
  client.android.bleDiscoverServices()
  time.sleep(CONNECTION_TIMEOUT)
  discover_services_results = client.client_callback.getAll(
      'onServiceDiscovered')
  asserts.assert_equal(len(discover_services_results), 1)
  service_discovered = False
  asserts.assert_equal(discover_services_results[0].data[STATUS],
                       GATT_SUCCESS)
  for service in discover_services_results[0].data['Services']:
    if service['UUID'] == TEST_BLE_SERVICE_UUID:
      service_discovered = True
      uuids = [
          characteristic[UUID]
          for characteristic in service['Characteristics']
      ]
      for uuid in [
          characteristic[UUID]
          for characteristic in SERVICE['Characteristics']
      ]:
        asserts.assert_true(uuid in uuids, 'Failed to find uuid %s.' % uuid)
  asserts.assert_true(service_discovered,
                      'Failed to discover the customize service')
  client.log.info('BLE discover services finished')


def ReadCharacteristic(client):
  """Logic for BLE characteristic read.

  Args:
    client: AndroidDevice. The device that behaves as GATT client.

  Steps:
    1. Client reads a characteristic from server & gets true.
    2. Server calls sendResponse & client gets onCharacteristicRead.

  Verifies:
    Client gets corresponding callback.
  """
  read_operation_result = client.android.bleReadOperation(
      TEST_BLE_SERVICE_UUID, TEST_READ_UUID)
  asserts.assert_true(read_operation_result,
                      'BLE read operation failed to start')
  read_operation_result = client.client_callback.waitAndGet(
      'onCharacteristicRead', 30)
  asserts.assert_equal(read_operation_result.data[STATUS], GATT_SUCCESS)
  asserts.assert_equal(read_operation_result.data['Data'], READ_DATA)
  client.log.info('Read operation finished')
  read_operation_result = client.android.bleReadOperation(
      TEST_BLE_SERVICE_UUID, TEST_SECOND_READ_UUID)
  asserts.assert_true(read_operation_result,
                      'BLE read operation failed to start')
  read_operation_result = client.client_callback.waitAndGet(
      'onCharacteristicRead', 30)
  asserts.assert_equal(read_operation_result.data[STATUS], GATT_SUCCESS)
  asserts.assert_equal(read_operation_result.data['Data'], SECOND_READ_DATA)
  client.log.info('Second read operation finished')
  read_operation_result = client.android.bleReadOperation(
      TEST_BLE_SERVICE_UUID, TEST_THIRD_READ_UUID)
  asserts.assert_true(read_operation_result,
                      'BLE read operation failed to start')
  read_operation_result = client.client_callback.waitAndGet(
      'onCharacteristicRead', 30)
  asserts.assert_equal(read_operation_result.data[STATUS], GATT_SUCCESS)
  asserts.assert_equal(read_operation_result.data['Data'], THIRD_READ_DATA)
  client.log.info('Third read operation finished')


def WriteCharacteristic(client, server):
  """Logic for BLE characteristic write.

  Args:
    client: AndroidDevice. The device that behaves as GATT client.
    server: AndroidDevice. The device that behaves as GATT server.

  Steps:
    1. Client writes a characteristic to server & gets true.
    2. Server calls sendResponse & client gets onCharacteristicWrite.

  Verifies:
    Client gets corresponding callback.
  """
  write_operation_result = client.android.bleWriteOperation(
      TEST_BLE_SERVICE_UUID, TEST_WRITE_UUID, WRITE_DATA)
  asserts.assert_true(write_operation_result,
                      'BLE write operation failed to start')
  server_write_operation_result = server.server_callback.waitAndGet(
      'onCharacteristicWriteRequest', 30)
  asserts.assert_equal(server_write_operation_result.data['Data'], WRITE_DATA)
  client.client_callback.waitAndGet('onCharacteristicWrite', 30)
  client.log.info('Write operation finished')
  write_operation_result = client.android.bleWriteOperation(
      TEST_BLE_SERVICE_UUID, TEST_SECOND_WRITE_UUID, SECOND_WRITE_DATA)
  asserts.assert_true(write_operation_result,
                      'BLE write operation failed to start')
  server_write_operation_result = server.server_callback.waitAndGet(
      'onCharacteristicWriteRequest', 30)
  asserts.assert_equal(server_write_operation_result.data['Data'],
                       SECOND_WRITE_DATA)
  client.client_callback.waitAndGet('onCharacteristicWrite', 30)
  client.log.info('Second write operation finished')


def ReliableWrite(client, server):
  """Logic for BLE reliable write.

  Args:
    client: AndroidDevice. The device that behaves as GATT client.
    server: AndroidDevice. The device that behaves as GATT server.

  Steps:
    1. Client calls beginReliableWrite & gets true.
    2. Client writes a characteristic to server & gets true.
    3. Server calls sendResponse & client gets onCharacteristicWrite.
    4. Client calls executeReliableWrite & gets true
    5. Server calls sendResponse & client gets onReliableWriteCompleted.

  Verifies:
    Client get corresponding callbacks.
  """
  begin_reliable_write_result = client.android.bleBeginReliableWrite()
  asserts.assert_true(begin_reliable_write_result,
                      'BLE reliable write failed to start')
  client.log.info('BLE reliable write started')
  WriteCharacteristic(client, server)
  execute_reliable_write_result = client.android.bleExecuteReliableWrite()
  asserts.assert_true(execute_reliable_write_result,
                      'BLE reliable write failed to execute')
  client.log.info('BLE reliable write execute started')
  client.client_callback.waitAndGet('onReliableWriteCompleted', 30)
  client.log.info('BLE reliable write finished')


class BleTest(android_base_test.AndroidBaseTest):
  """BLE tests."""

  def setup_class(self):
    super(BleTest, self).setup_class()
    self.initiator = self.dut_a
    self.receiver = self.dut_b
    # Sets the tag that represents this device in logs.
    # The device used to scan BLE devices and behave as a GATT client.
    self.initiator.debug_tag = 'initiator'
    # The device used to BLE advertise and behave as a GATT server.
    self.receiver.debug_tag = 'receiver'

  def setup_test(self):
    # Make sure bluetooth is on.
    self.initiator.android.btEnable()
    self.receiver.android.btEnable()

  def test_basic_ble_process(self):
    """Test for basic BLE process flow.

    Steps:
      1. Initiator discovers receiver.
      2. Initiator connects to receiver.
      3. Initiator discovers the BLE service receiver provided.
      4. Initiator reads a message from receiver.
      5. Initiator sends a message to receiver.
      6. Initiator disconnects from receiver.
      7. BLE scan and advertise stopped.

    Verifies:
      In each step, initiator and receiver get corresponding callbacks.
    """
    Discover(self.initiator, self.receiver)
    Connect(self.initiator, self.receiver)
    DiscoverServices(self.initiator)
    ReadCharacteristic(self.initiator)
    WriteCharacteristic(self.initiator, self.receiver)
    Disconnect(self.initiator, self.receiver)
    StopDiscover(self.initiator, self.receiver)

  def test_cancel_open_ble_process(self):
    """Test for BLE process flow involving cancel open.

    Steps:
      1. Initiator discovers receiver.
      2. Initiator initials connection to receiver, cancels connection and
      reconnects.
      3. Initiator discovers the BLE service receiver provided.
      6. Initiator disconnects from receiver.
      7. BLE scan and advertise stopped.

    Verifies:
      In each step, initiator and receiver get corresponding callbacks.
    """
    Discover(self.initiator, self.receiver)
    CancelOpen(self.initiator, self.receiver)
    DiscoverServices(self.initiator)
    Disconnect(self.initiator, self.receiver)
    StopDiscover(self.initiator, self.receiver)

  def test_reliable_write_ble_process(self):
    """Test for BLE process flow involving reliable write.

    Steps:
      1. Initiator discovers receiver.
      2. Initiator connects to receiver.
      3. Initiator discovers the BLE service receiver provided.
      4. Initiator starts reliable write to receiver and finishes successfully.
      5. Initiator disconnects from receiver.
      6. BLE scan and advertise stopped.

    Verifies:
      In each step, initiator and receiver get corresponding callbacks.
    """
    Discover(self.initiator, self.receiver)
    Connect(self.initiator, self.receiver)
    DiscoverServices(self.initiator)
    ReliableWrite(self.initiator, self.receiver)
    Disconnect(self.initiator, self.receiver)
    StopDiscover(self.initiator, self.receiver)

  def teardown_test(self):
    # Turn Bluetooth off on both devices after test finishes.
    self.initiator.android.btDisable()
    self.receiver.android.btDisable()


if __name__ == '__main__':
  test_runner.main()
