//
//  Copyright 2015 Google, Inc.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at:
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//

#pragma once

#include <mutex>
#include <unordered_set>
#include <vector>

#include <base/macros.h>
#include <hardware/bluetooth.h>
#include <hardware/bt_gatt.h>

namespace bluetooth {
namespace hal {

// This class represents the standard BT-GATT interface. This class combines
// GATT profile server and client role operations with general GAP profile
// operations of various roles (central, scanner, peripheral, advertiser),
// wrapping around the underlying bt_gatt_interface_t structure. A single
// instance of this class exists per application and it allows multiple classes
// to interface with the global HAL interface by multiplexing callbacks among
// registered clients.
//
// This is declared as an abstract interface so that a fake implementation can
// be injected for testing the upper layer.
class BluetoothGattInterface {
 public:
  // The HAL interface doesn't allow registering "user data" that carries
  // context beyond the callback parameters, forcing implementations to deal
  // with global variables. The *Observer interface is to redirect these events
  // to interested parties in an object-oriented manner.

  // The standard LE scanner callback interface.
  class ScannerObserver {
   public:
    virtual ~ScannerObserver() = default;

    // All of the events below correspond to callbacks defined in
    // "btgatt_scanner_callbacks_t" in the HAL API definitions.

    virtual void ScanResultCallback(
        BluetoothGattInterface* gatt_iface, const RawAddress& bda, int rssi,
        std::vector<uint8_t> adv_data);  // NOLINT(pass-by-value)
  };

  // The standard BT-GATT client callback interface.
  class ClientObserver {
   public:
    virtual ~ClientObserver() = default;

    // All of the events below correspond to callbacks defined in
    // "bt_gatt_client_callbacks_t" in the HAL API definitions.

    virtual void RegisterClientCallback(BluetoothGattInterface* gatt_iface,
                                        int status, int client_if,
                                        const bluetooth::Uuid& app_uuid);

    virtual void ConnectCallback(BluetoothGattInterface* gatt_iface,
                                 int conn_id, int status, int client_if,
                                 const RawAddress& bda);

    virtual void DisconnectCallback(BluetoothGattInterface* gatt_iface,
                                    int conn_id, int status, int client_if,
                                    const RawAddress& bda);

    virtual void SearchCompleteCallback(BluetoothGattInterface* gatt_iface,
                                        int conn_id, int status);

    virtual void RegisterForNotificationCallback(
        BluetoothGattInterface* gatt_iface, int conn_id, int status,
        int registered, uint16_t handle);

    virtual void NotifyCallback(BluetoothGattInterface* gatt_iface, int conn_id,
                                const btgatt_notify_params_t& p_data);

    virtual void WriteCharacteristicCallback(BluetoothGattInterface* gatt_iface,
                                             int conn_id, int status,
                                             uint16_t handle);

    virtual void WriteDescriptorCallback(BluetoothGattInterface* gatt_iface,
                                         int conn_id, int status,
                                         uint16_t handle);

    virtual void MtuChangedCallback(BluetoothGattInterface* gatt_iface,
                                    int conn_id, int status, int mtu);

    virtual void GetGattDbCallback(BluetoothGattInterface* gatt_iface,
                                   int conn_id,
                                   const btgatt_db_element_t* gatt_db,
                                   int size);

    virtual void ServicesRemovedCallback(BluetoothGattInterface* gatt_iface,
                                         int conn_id, uint16_t start_handle,
                                         uint16_t end_handle);

    virtual void ServicesAddedCallback(BluetoothGattInterface* gatt_iface,
                                       int conn_id,
                                       const btgatt_db_element_t& added,
                                       int added_count);
  };

  // The standard BT-GATT server callback interface.
  class ServerObserver {
   public:
    virtual ~ServerObserver() = default;

    virtual void RegisterServerCallback(BluetoothGattInterface* gatt_iface,
                                        int status, int server_if,
                                        const bluetooth::Uuid& app_uuid);

    virtual void ConnectionCallback(BluetoothGattInterface* gatt_iface,
                                    int conn_id, int server_if, int connected,
                                    const RawAddress& bda);

    virtual void ServiceAddedCallback(
        BluetoothGattInterface* gatt_iface, int status, int server_if,
        std::vector<btgatt_db_element_t> service);  // NOLINT(pass-by-value)

    virtual void ServiceStoppedCallback(BluetoothGattInterface* gatt_iface,
                                        int status, int server_if,
                                        int srvc_handle);

    virtual void ServiceDeletedCallback(BluetoothGattInterface* gatt_iface,
                                        int status, int server_if,
                                        int srvc_handle);

    virtual void RequestReadCharacteristicCallback(
        BluetoothGattInterface* gatt_iface, int conn_id, int trans_id,
        const RawAddress& bda, int attr_handle, int offset, bool is_long);

    virtual void RequestReadDescriptorCallback(
        BluetoothGattInterface* gatt_iface, int conn_id, int trans_id,
        const RawAddress& bda, int attr_handle, int offset, bool is_long);

    virtual void RequestWriteCharacteristicCallback(
        BluetoothGattInterface* gatt_iface, int conn_id, int trans_id,
        const RawAddress& bda, int attr_handle, int offset, bool need_rsp,
        bool is_prep,
        std::vector<uint8_t> value);  // NOLINT(pass-by-value)

    virtual void RequestWriteDescriptorCallback(
        BluetoothGattInterface* gatt_iface, int conn_id, int trans_id,
        const RawAddress& bda, int attr_handle, int offset, bool need_rsp,
        bool is_prep,
        std::vector<uint8_t> value);  // NOLINT(pass-by-alue)

    virtual void RequestExecWriteCallback(BluetoothGattInterface* gatt_iface,
                                          int conn_id, int trans_id,
                                          const RawAddress& bda,
                                          int exec_write);

    virtual void ResponseConfirmationCallback(
        BluetoothGattInterface* gatt_iface, int status, int handle);

    virtual void IndicationSentCallback(BluetoothGattInterface* gatt_iface,
                                        int conn_id, int status);

    virtual void MtuChangedCallback(BluetoothGattInterface* gatt_iface,
                                    int conn_id, int mtu);
  };

  // Initialize and clean up the BluetoothInterface singleton. Returns false if
  // the underlying HAL interface failed to initialize, and true on success.
  static bool Initialize();

  // Shuts down and cleans up the interface. CleanUp must be called on the same
  // thread that called Initialize.
  static void CleanUp();

  // Returns true if the interface was initialized and a global singleton has
  // been created.
  static bool IsInitialized();

  // Initialize for testing. Use this to inject a test version of
  // BluetoothGattInterface. To be used from unit tests only.
  static void InitializeForTesting(BluetoothGattInterface* test_instance);

  // Returns the BluetoothGattInterface singleton. If the interface has
  // not been initialized, returns nullptr. This method is thread-safe, in that
  // it will block if the internal lock is being held by another thread. Don't
  // call this re-entrantly from an observer event as this may cause a deadlock.
  static BluetoothGattInterface* Get();

  // Add or remove an observer that is interested in LE scanner interface
  // notifications from us. Thread-safety is guaranteed by ObserverList.
  virtual void AddScannerObserver(ScannerObserver* observer) = 0;
  virtual void RemoveScannerObserver(ScannerObserver* observer) = 0;

  // Add or remove an observer that is interested in GATT client interface
  // notifications from us. Thread-safety is guaranteed by ObserverList.
  virtual void AddClientObserver(ClientObserver* observer) = 0;
  virtual void RemoveClientObserver(ClientObserver* observer) = 0;

  // Add or remove an observer that is interested in GATT server interface
  // notifications from us. Thread-safety is guaranteed by ObserverList.
  virtual void AddServerObserver(ServerObserver* observer) = 0;
  virtual void RemoveServerObserver(ServerObserver* observer) = 0;

  // The HAL module pointer that represents the standard BT LE advertiser
  // interface. This is implemented in and provided by the shared Bluetooth
  // library, so this isn't owned by us.
  //
  // Upper layers can make ble_advertiser_interface_t API calls through this
  // structure.
  virtual BleAdvertiserInterface* GetAdvertiserHALInterface() const = 0;

  // The HAL module pointer that represents the standard BT LE scanner
  // interface. This is implemented in and provided by the shared Bluetooth
  // library, so this isn't owned by us.
  //
  // Upper layers can make ble_scanner_interface_t API calls through this
  // structure.
  virtual BleScannerInterface* GetScannerHALInterface() const = 0;

  // The HAL module pointer that represents the standard BT-GATT client
  // interface. This is implemented in and provided by the shared Bluetooth
  // library, so this isn't owned by us.
  //
  // Upper layers can make btgatt_client_interface_t API calls through this
  // structure.
  virtual const btgatt_client_interface_t* GetClientHALInterface() const = 0;

  // The HAL module pointer that represents the standard BT-GATT server
  // interface. This is implemented in and provided by the shared Bluetooth
  // library, so this isn't owned by us.
  //
  // Upper layers can make btgatt_server_interface_t API calls through this
  // structure.
  virtual const btgatt_server_interface_t* GetServerHALInterface() const = 0;

  // Initiates a regular BLE device scan. This is called internally from each
  // LowEnergyClient. This function synchronizes the scan requests and maintains
  // an internal reference count for each scan client that is interested.
  bt_status_t StartScan(int client_id);
  bt_status_t StopScan(int client_id);

 protected:
  BluetoothGattInterface() = default;
  virtual ~BluetoothGattInterface() = default;

 private:
  // Used to keep a reference count for the different BLE scan clients.
  std::mutex scan_clients_lock_;
  std::unordered_set<int> scan_client_set_;

  DISALLOW_COPY_AND_ASSIGN(BluetoothGattInterface);
};

}  // namespace hal
}  // namespace bluetooth
