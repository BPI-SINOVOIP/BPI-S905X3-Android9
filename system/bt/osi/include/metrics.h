/******************************************************************************
 *
 *  Copyright 2016 Google, Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

#pragma once

#include <bta/include/bta_api.h>
#include <stdint.h>
#include <memory>
#include <string>

namespace system_bt_osi {

// Typedefs to hide protobuf definition to the rest of stack

typedef enum {
  DEVICE_TYPE_UNKNOWN,
  DEVICE_TYPE_BREDR,
  DEVICE_TYPE_LE,
  DEVICE_TYPE_DUMO,
} device_type_t;

typedef enum {
  WAKE_EVENT_UNKNOWN,
  WAKE_EVENT_ACQUIRED,
  WAKE_EVENT_RELEASED,
} wake_event_type_t;

typedef enum {
  SCAN_TYPE_UNKNOWN,
  SCAN_TECH_TYPE_LE,
  SCAN_TECH_TYPE_BREDR,
  SCAN_TECH_TYPE_BOTH,
} scan_tech_t;

typedef enum {
  CONNECTION_TECHNOLOGY_TYPE_UNKNOWN,
  CONNECTION_TECHNOLOGY_TYPE_LE,
  CONNECTION_TECHNOLOGY_TYPE_BREDR,
} connection_tech_t;

typedef enum {
  DISCONNECT_REASON_UNKNOWN,
  DISCONNECT_REASON_METRICS_DUMP,
  DISCONNECT_REASON_NEXT_START_WITHOUT_END_PREVIOUS,
} disconnect_reason_t;

/* Values of A2DP metrics that we care about
 *
 *    audio_duration_ms : sum of audio duration (in milliseconds).
 *    device_class: device class of the paired device.
 *    media_timer_min_ms : minimum scheduled time (in milliseconds)
 *                         of the media timer.
 *    media_timer_max_ms: maximum scheduled time (in milliseconds)
 *                        of the media timer.
 *    media_timer_avg_ms: average scheduled time (in milliseconds)
 *                        of the media timer.
 *    buffer_overruns_max_count: TODO - not clear what this is.
 *    buffer_overruns_total : number of times the media buffer with
 *                            audio data has overrun
 *    buffer_underruns_average: TODO - not clear what this is.
 *    buffer_underruns_count: number of times there was no enough
 *                            audio data to add to the media buffer.
 * NOTE: Negative values are invalid
*/
class A2dpSessionMetrics {
 public:
  A2dpSessionMetrics() {}

  /*
   * Update the metrics value in the current metrics object using the metrics
   * objects supplied
   */
  void Update(const A2dpSessionMetrics& metrics);

  /*
   * Compare whether two metrics objects are equal
   */
  bool operator==(const A2dpSessionMetrics& rhs) const;

  /*
   * Initialize all values to -1 which is invalid in order to make a distinction
   * between 0 and invalid values
   */
  int64_t audio_duration_ms = -1;
  int32_t media_timer_min_ms = -1;
  int32_t media_timer_max_ms = -1;
  int32_t media_timer_avg_ms = -1;
  int64_t total_scheduling_count = -1;
  int32_t buffer_overruns_max_count = -1;
  int32_t buffer_overruns_total = -1;
  float buffer_underruns_average = -1;
  int32_t buffer_underruns_count = -1;
};

class BluetoothMetricsLogger {
 public:
  static BluetoothMetricsLogger* GetInstance() {
    static BluetoothMetricsLogger* instance = new BluetoothMetricsLogger();
    return instance;
  }

  /*
   * Record a pairing event
   *
   * Parameters:
   *    timestamp_ms: Unix epoch time in milliseconds
   *    device_class: class of remote device
   *    device_type: type of remote device
   *    disconnect_reason: HCI reason for pairing disconnection.
   *                       See: stack/include/hcidefs.h
   */
  void LogPairEvent(uint32_t disconnect_reason, uint64_t timestamp_ms,
                    uint32_t device_class, device_type_t device_type);

  /*
   * Record a wake event
   *
   * Parameters:
   *    timestamp_ms: Unix epoch time in milliseconds
   *    type: whether it was acquired or released
   *    requestor: if provided is the service requesting the wake lock
   *    name: the name of the wake lock held
   */
  void LogWakeEvent(wake_event_type_t type, const std::string& requestor,
                    const std::string& name, uint64_t timestamp_ms);

  /*
   * Record a scan event
   *
   * Parameters
   *    timestamp_ms : Unix epoch time in milliseconds
   *    start : true if this is the beginning of the scan
   *    initiator: a unique ID identifying the app starting the scan
   *    type: whether the scan reports BR/EDR, LE, or both.
   *    results: number of results to be reported.
   */
  void LogScanEvent(bool start, const std::string& initator, scan_tech_t type,
                    uint32_t results, uint64_t timestamp_ms);

  /*
   * Start logging a Bluetooth session
   *
   * A Bluetooth session is defined a a connection between this device and
   * another remote device which may include multiple profiles and protocols
   *
   * Only one Bluetooth session can exist at one time. Calling this method twice
   * without LogBluetoothSessionEnd will result in logging a premature end of
   * current Bluetooth session
   *
   * Parameters:
   *    connection_tech_type : type of connection technology
   *    timestamp_ms : the timestamp for session start, 0 means now
   *
   */
  void LogBluetoothSessionStart(connection_tech_t connection_tech_type,
                                uint64_t timestamp_ms);

  /*
   * Stop logging a Bluetooth session and pushes it to the log queue
   *
   * If no Bluetooth session exist, this method exits immediately
   *
   * Parameters:
   *    disconnect_reason : A string representation of disconnect reason
   *    timestamp_ms : the timestamp of session end, 0 means now
   *
   */
  void LogBluetoothSessionEnd(disconnect_reason_t disconnect_reason,
                              uint64_t timestamp_ms);

  /*
   * Log information about remote device in a current Bluetooth session
   *
   * If a Bluetooth session does not exist, create one with default parameter
   * and timestamp now
   *
   * Parameters:
   *    device_class : device_class defined in btm_api_types.h
   *    device_type : type of remote device
   */
  void LogBluetoothSessionDeviceInfo(uint32_t device_class,
                                     device_type_t device_type);

  /*
   * Log A2DP Audio Session Information
   *
   * - Repeated calls to this method will override previous metrics if in the
   *   same Bluetooth connection
   * - If a Bluetooth session does not exist, create one with default parameter
   *   and timestamp now
   *
   * Parameters:
   *    a2dp_session_metrics - pointer to struct holding a2dp stats
   *
   */
  void LogA2dpSession(const A2dpSessionMetrics& a2dp_session_metrics);

  /**
   * Log Headset profile RFCOMM connection event
   *
   * @param service_id the BTA service ID for this headset connection
   */
  void LogHeadsetProfileRfcConnection(tBTA_SERVICE_ID service_id);

  /*
   * Writes the metrics, in base64 protobuf format, into the descriptor FD,
   * metrics events are always cleared after dump
   */
  void WriteBase64(int fd);
  void WriteBase64String(std::string* serialized);
  void WriteString(std::string* serialized);

  /*
   * Reset the metrics logger by cleaning up its staging queues and existing
   * protobuf objects.
   */
  void Reset();

  /*
   * Maximum number of log entries for each session or event
   */
  static const size_t kMaxNumBluetoothSession = 50;
  static const size_t kMaxNumPairEvent = 50;
  static const size_t kMaxNumWakeEvent = 1000;
  static const size_t kMaxNumScanEvent = 50;

 private:
  BluetoothMetricsLogger();

  /*
   * When a Bluetooth session is on and the user initiates a metrics dump, we
   * need to be able to upload whatever we have first. This method breaks the
   * ongoing Bluetooth session into two sessions with the previous one labeled
   * as "METRICS_DUMP" for the disconnect reason.
   */
  void CutoffSession();

  /*
   * Build the internal metrics object using information gathered
   */
  void Build();

  /*
   * Reset objects related to current Bluetooth session
   */
  void ResetSession();

  /*
   * Reset the underlining BluetoothLog object
   */
  void ResetLog();

  /*
   * PIMPL style implementation to hide internal dependencies
   */
  struct impl;
  std::unique_ptr<impl> const pimpl_;
};
}
