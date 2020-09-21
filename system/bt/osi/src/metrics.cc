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
#define LOG_TAG "bt_osi_metrics"

#include <unistd.h>
#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <memory>
#include <mutex>

#include <base/base64.h>
#include <base/logging.h>

#include "bluetooth/metrics/bluetooth.pb.h"
#include "osi/include/leaky_bonded_queue.h"
#include "osi/include/log.h"
#include "osi/include/osi.h"
#include "osi/include/time.h"
#include "stack/include/btm_api_types.h"

#include "osi/include/metrics.h"

namespace system_bt_osi {

using bluetooth::metrics::BluetoothMetricsProto::A2DPSession;
using bluetooth::metrics::BluetoothMetricsProto::BluetoothLog;
using bluetooth::metrics::BluetoothMetricsProto::BluetoothSession;
using bluetooth::metrics::BluetoothMetricsProto::
    BluetoothSession_ConnectionTechnologyType;
using bluetooth::metrics::BluetoothMetricsProto::
    BluetoothSession_DisconnectReasonType;
using bluetooth::metrics::BluetoothMetricsProto::DeviceInfo;
using bluetooth::metrics::BluetoothMetricsProto::DeviceInfo_DeviceType;
using bluetooth::metrics::BluetoothMetricsProto::PairEvent;
using bluetooth::metrics::BluetoothMetricsProto::ScanEvent;
using bluetooth::metrics::BluetoothMetricsProto::ScanEvent_ScanTechnologyType;
using bluetooth::metrics::BluetoothMetricsProto::ScanEvent_ScanEventType;
using bluetooth::metrics::BluetoothMetricsProto::WakeEvent;
using bluetooth::metrics::BluetoothMetricsProto::WakeEvent_WakeEventType;
using bluetooth::metrics::BluetoothMetricsProto::HeadsetProfileType;
using bluetooth::metrics::BluetoothMetricsProto::HeadsetProfileType_MIN;
using bluetooth::metrics::BluetoothMetricsProto::HeadsetProfileType_MAX;
using bluetooth::metrics::BluetoothMetricsProto::HeadsetProfileType_ARRAYSIZE;
using bluetooth::metrics::BluetoothMetricsProto::HeadsetProfileType_IsValid;
using bluetooth::metrics::BluetoothMetricsProto::HeadsetProfileConnectionStats;
/*
 * Get current OS boot time in millisecond
 */
static int64_t time_get_os_boottime_ms(void) {
  return time_get_os_boottime_us() / 1000;
}

static float combine_averages(float avg_a, int64_t ct_a, float avg_b,
                              int64_t ct_b) {
  if (ct_a > 0 && ct_b > 0) {
    return (avg_a * ct_a + avg_b * ct_b) / (ct_a + ct_b);
  } else if (ct_b > 0) {
    return avg_b;
  } else {
    return avg_a;
  }
}

static int32_t combine_averages(int32_t avg_a, int64_t ct_a, int32_t avg_b,
                                int64_t ct_b) {
  if (ct_a > 0 && ct_b > 0) {
    return (avg_a * ct_a + avg_b * ct_b) / (ct_a + ct_b);
  } else if (ct_b > 0) {
    return avg_b;
  } else {
    return avg_a;
  }
}

void A2dpSessionMetrics::Update(const A2dpSessionMetrics& metrics) {
  if (metrics.audio_duration_ms >= 0) {
    audio_duration_ms = std::max(static_cast<int64_t>(0), audio_duration_ms);
    audio_duration_ms += metrics.audio_duration_ms;
  }
  if (metrics.media_timer_min_ms >= 0) {
    if (media_timer_min_ms < 0) {
      media_timer_min_ms = metrics.media_timer_min_ms;
    } else {
      media_timer_min_ms =
          std::min(media_timer_min_ms, metrics.media_timer_min_ms);
    }
  }
  if (metrics.media_timer_max_ms >= 0) {
    media_timer_max_ms =
        std::max(media_timer_max_ms, metrics.media_timer_max_ms);
  }
  if (metrics.media_timer_avg_ms >= 0 && metrics.total_scheduling_count >= 0) {
    if (media_timer_avg_ms < 0 || total_scheduling_count < 0) {
      media_timer_avg_ms = metrics.media_timer_avg_ms;
      total_scheduling_count = metrics.total_scheduling_count;
    } else {
      media_timer_avg_ms = combine_averages(
          media_timer_avg_ms, total_scheduling_count,
          metrics.media_timer_avg_ms, metrics.total_scheduling_count);
      total_scheduling_count += metrics.total_scheduling_count;
    }
  }
  if (metrics.buffer_overruns_max_count >= 0) {
    buffer_overruns_max_count =
        std::max(buffer_overruns_max_count, metrics.buffer_overruns_max_count);
  }
  if (metrics.buffer_overruns_total >= 0) {
    buffer_overruns_total =
        std::max(static_cast<int32_t>(0), buffer_overruns_total);
    buffer_overruns_total += metrics.buffer_overruns_total;
  }
  if (metrics.buffer_underruns_average >= 0 &&
      metrics.buffer_underruns_count >= 0) {
    if (buffer_underruns_average < 0 || buffer_underruns_count < 0) {
      buffer_underruns_average = metrics.buffer_underruns_average;
      buffer_underruns_count = metrics.buffer_underruns_count;
    } else {
      buffer_underruns_average = combine_averages(
          buffer_underruns_average, buffer_underruns_count,
          metrics.buffer_underruns_average, metrics.buffer_underruns_count);
      buffer_underruns_count += metrics.buffer_underruns_count;
    }
  }
}

bool A2dpSessionMetrics::operator==(const A2dpSessionMetrics& rhs) const {
  return audio_duration_ms == rhs.audio_duration_ms &&
         media_timer_min_ms == rhs.media_timer_min_ms &&
         media_timer_max_ms == rhs.media_timer_max_ms &&
         media_timer_avg_ms == rhs.media_timer_avg_ms &&
         total_scheduling_count == rhs.total_scheduling_count &&
         buffer_overruns_max_count == rhs.buffer_overruns_max_count &&
         buffer_overruns_total == rhs.buffer_overruns_total &&
         buffer_underruns_average == rhs.buffer_underruns_average &&
         buffer_underruns_count == rhs.buffer_underruns_count;
}

static DeviceInfo_DeviceType get_device_type(device_type_t type) {
  switch (type) {
    case DEVICE_TYPE_BREDR:
      return DeviceInfo_DeviceType::DeviceInfo_DeviceType_DEVICE_TYPE_BREDR;
    case DEVICE_TYPE_LE:
      return DeviceInfo_DeviceType::DeviceInfo_DeviceType_DEVICE_TYPE_LE;
    case DEVICE_TYPE_DUMO:
      return DeviceInfo_DeviceType::DeviceInfo_DeviceType_DEVICE_TYPE_DUMO;
    case DEVICE_TYPE_UNKNOWN:
    default:
      return DeviceInfo_DeviceType::DeviceInfo_DeviceType_DEVICE_TYPE_UNKNOWN;
  }
}

static BluetoothSession_ConnectionTechnologyType get_connection_tech_type(
    connection_tech_t type) {
  switch (type) {
    case CONNECTION_TECHNOLOGY_TYPE_LE:
      return BluetoothSession_ConnectionTechnologyType::
          BluetoothSession_ConnectionTechnologyType_CONNECTION_TECHNOLOGY_TYPE_LE;
    case CONNECTION_TECHNOLOGY_TYPE_BREDR:
      return BluetoothSession_ConnectionTechnologyType::
          BluetoothSession_ConnectionTechnologyType_CONNECTION_TECHNOLOGY_TYPE_BREDR;
    case CONNECTION_TECHNOLOGY_TYPE_UNKNOWN:
    default:
      return BluetoothSession_ConnectionTechnologyType::
          BluetoothSession_ConnectionTechnologyType_CONNECTION_TECHNOLOGY_TYPE_UNKNOWN;
  }
}

static ScanEvent_ScanTechnologyType get_scan_tech_type(scan_tech_t type) {
  switch (type) {
    case SCAN_TECH_TYPE_LE:
      return ScanEvent_ScanTechnologyType::
          ScanEvent_ScanTechnologyType_SCAN_TECH_TYPE_LE;
    case SCAN_TECH_TYPE_BREDR:
      return ScanEvent_ScanTechnologyType::
          ScanEvent_ScanTechnologyType_SCAN_TECH_TYPE_BREDR;
    case SCAN_TECH_TYPE_BOTH:
      return ScanEvent_ScanTechnologyType::
          ScanEvent_ScanTechnologyType_SCAN_TECH_TYPE_BOTH;
    case SCAN_TYPE_UNKNOWN:
    default:
      return ScanEvent_ScanTechnologyType::
          ScanEvent_ScanTechnologyType_SCAN_TYPE_UNKNOWN;
  }
}

static WakeEvent_WakeEventType get_wake_event_type(wake_event_type_t type) {
  switch (type) {
    case WAKE_EVENT_ACQUIRED:
      return WakeEvent_WakeEventType::WakeEvent_WakeEventType_ACQUIRED;
    case WAKE_EVENT_RELEASED:
      return WakeEvent_WakeEventType::WakeEvent_WakeEventType_RELEASED;
    case WAKE_EVENT_UNKNOWN:
    default:
      return WakeEvent_WakeEventType::WakeEvent_WakeEventType_UNKNOWN;
  }
}

static BluetoothSession_DisconnectReasonType get_disconnect_reason_type(
    disconnect_reason_t type) {
  switch (type) {
    case DISCONNECT_REASON_METRICS_DUMP:
      return BluetoothSession_DisconnectReasonType::
          BluetoothSession_DisconnectReasonType_METRICS_DUMP;
    case DISCONNECT_REASON_NEXT_START_WITHOUT_END_PREVIOUS:
      return BluetoothSession_DisconnectReasonType::
          BluetoothSession_DisconnectReasonType_NEXT_START_WITHOUT_END_PREVIOUS;
    case DISCONNECT_REASON_UNKNOWN:
    default:
      return BluetoothSession_DisconnectReasonType::
          BluetoothSession_DisconnectReasonType_UNKNOWN;
  }
}

struct BluetoothMetricsLogger::impl {
  impl(size_t max_bluetooth_session, size_t max_pair_event,
       size_t max_wake_event, size_t max_scan_event)
      : bt_session_queue_(
            new LeakyBondedQueue<BluetoothSession>(max_bluetooth_session)),
        pair_event_queue_(new LeakyBondedQueue<PairEvent>(max_pair_event)),
        wake_event_queue_(new LeakyBondedQueue<WakeEvent>(max_wake_event)),
        scan_event_queue_(new LeakyBondedQueue<ScanEvent>(max_scan_event)) {
    bluetooth_log_ = BluetoothLog::default_instance().New();
    headset_profile_connection_counts_.fill(0);
    bluetooth_session_ = nullptr;
    bluetooth_session_start_time_ms_ = 0;
    a2dp_session_metrics_ = A2dpSessionMetrics();
  }

  /* Bluetooth log lock protected */
  BluetoothLog* bluetooth_log_;
  std::array<int, HeadsetProfileType_ARRAYSIZE>
      headset_profile_connection_counts_;
  std::recursive_mutex bluetooth_log_lock_;
  /* End Bluetooth log lock protected */
  /* Bluetooth session lock protected */
  BluetoothSession* bluetooth_session_;
  uint64_t bluetooth_session_start_time_ms_;
  A2dpSessionMetrics a2dp_session_metrics_;
  std::recursive_mutex bluetooth_session_lock_;
  /* End bluetooth session lock protected */
  std::unique_ptr<LeakyBondedQueue<BluetoothSession>> bt_session_queue_;
  std::unique_ptr<LeakyBondedQueue<PairEvent>> pair_event_queue_;
  std::unique_ptr<LeakyBondedQueue<WakeEvent>> wake_event_queue_;
  std::unique_ptr<LeakyBondedQueue<ScanEvent>> scan_event_queue_;
};

BluetoothMetricsLogger::BluetoothMetricsLogger()
    : pimpl_(new impl(kMaxNumBluetoothSession, kMaxNumPairEvent,
                      kMaxNumWakeEvent, kMaxNumScanEvent)) {}

void BluetoothMetricsLogger::LogPairEvent(uint32_t disconnect_reason,
                                          uint64_t timestamp_ms,
                                          uint32_t device_class,
                                          device_type_t device_type) {
  PairEvent* event = new PairEvent();
  DeviceInfo* info = event->mutable_device_paired_with();
  info->set_device_class(device_class);
  info->set_device_type(get_device_type(device_type));
  event->set_disconnect_reason(disconnect_reason);
  event->set_event_time_millis(timestamp_ms);
  pimpl_->pair_event_queue_->Enqueue(event);
  {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->bluetooth_log_lock_);
    pimpl_->bluetooth_log_->set_num_pair_event(
        pimpl_->bluetooth_log_->num_pair_event() + 1);
  }
}

void BluetoothMetricsLogger::LogWakeEvent(wake_event_type_t type,
                                          const std::string& requestor,
                                          const std::string& name,
                                          uint64_t timestamp_ms) {
  WakeEvent* event = new WakeEvent();
  event->set_wake_event_type(get_wake_event_type(type));
  event->set_requestor(requestor);
  event->set_name(name);
  event->set_event_time_millis(timestamp_ms);
  pimpl_->wake_event_queue_->Enqueue(event);
  {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->bluetooth_log_lock_);
    pimpl_->bluetooth_log_->set_num_wake_event(
        pimpl_->bluetooth_log_->num_wake_event() + 1);
  }
}

void BluetoothMetricsLogger::LogScanEvent(bool start,
                                          const std::string& initator,
                                          scan_tech_t type, uint32_t results,
                                          uint64_t timestamp_ms) {
  ScanEvent* event = new ScanEvent();
  if (start) {
    event->set_scan_event_type(ScanEvent::SCAN_EVENT_START);
  } else {
    event->set_scan_event_type(ScanEvent::SCAN_EVENT_STOP);
  }
  event->set_initiator(initator);
  event->set_scan_technology_type(get_scan_tech_type(type));
  event->set_number_results(results);
  event->set_event_time_millis(timestamp_ms);
  pimpl_->scan_event_queue_->Enqueue(event);
  {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->bluetooth_log_lock_);
    pimpl_->bluetooth_log_->set_num_scan_event(
        pimpl_->bluetooth_log_->num_scan_event() + 1);
  }
}

void BluetoothMetricsLogger::LogBluetoothSessionStart(
    connection_tech_t connection_tech_type, uint64_t timestamp_ms) {
  std::lock_guard<std::recursive_mutex> lock(pimpl_->bluetooth_session_lock_);
  if (pimpl_->bluetooth_session_ != nullptr) {
    LogBluetoothSessionEnd(DISCONNECT_REASON_NEXT_START_WITHOUT_END_PREVIOUS,
                           0);
  }
  if (timestamp_ms == 0) {
    timestamp_ms = time_get_os_boottime_ms();
  }
  pimpl_->bluetooth_session_start_time_ms_ = timestamp_ms;
  pimpl_->bluetooth_session_ = new BluetoothSession();
  pimpl_->bluetooth_session_->set_connection_technology_type(
      get_connection_tech_type(connection_tech_type));
}

void BluetoothMetricsLogger::LogBluetoothSessionEnd(
    disconnect_reason_t disconnect_reason, uint64_t timestamp_ms) {
  std::lock_guard<std::recursive_mutex> lock(pimpl_->bluetooth_session_lock_);
  if (pimpl_->bluetooth_session_ == nullptr) {
    return;
  }
  if (timestamp_ms == 0) {
    timestamp_ms = time_get_os_boottime_ms();
  }
  int64_t session_duration_sec =
      (timestamp_ms - pimpl_->bluetooth_session_start_time_ms_) / 1000;
  pimpl_->bluetooth_session_->set_session_duration_sec(session_duration_sec);
  pimpl_->bluetooth_session_->set_disconnect_reason_type(
      get_disconnect_reason_type(disconnect_reason));
  pimpl_->bt_session_queue_->Enqueue(pimpl_->bluetooth_session_);
  pimpl_->bluetooth_session_ = nullptr;
  {
    std::lock_guard<std::recursive_mutex> log_lock(pimpl_->bluetooth_log_lock_);
    pimpl_->bluetooth_log_->set_num_bluetooth_session(
        pimpl_->bluetooth_log_->num_bluetooth_session() + 1);
  }
}

void BluetoothMetricsLogger::LogBluetoothSessionDeviceInfo(
    uint32_t device_class, device_type_t device_type) {
  std::lock_guard<std::recursive_mutex> lock(pimpl_->bluetooth_session_lock_);
  if (pimpl_->bluetooth_session_ == nullptr) {
    LogBluetoothSessionStart(CONNECTION_TECHNOLOGY_TYPE_UNKNOWN, 0);
  }
  DeviceInfo* info = pimpl_->bluetooth_session_->mutable_device_connected_to();
  info->set_device_class(device_class);
  info->set_device_type(DeviceInfo::DEVICE_TYPE_BREDR);
}

void BluetoothMetricsLogger::LogA2dpSession(
    const A2dpSessionMetrics& a2dp_session_metrics) {
  std::lock_guard<std::recursive_mutex> lock(pimpl_->bluetooth_session_lock_);
  if (pimpl_->bluetooth_session_ == nullptr) {
    // When no bluetooth session exist, create one on system's behalf
    // Set connection type: for A2DP it is always BR/EDR
    LogBluetoothSessionStart(CONNECTION_TECHNOLOGY_TYPE_BREDR, 0);
    LogBluetoothSessionDeviceInfo(BTM_COD_MAJOR_AUDIO, DEVICE_TYPE_BREDR);
  }
  // Accumulate metrics
  pimpl_->a2dp_session_metrics_.Update(a2dp_session_metrics);
  // Get or allocate new A2DP session object
  A2DPSession* a2dp_session =
      pimpl_->bluetooth_session_->mutable_a2dp_session();
  a2dp_session->set_audio_duration_millis(
      pimpl_->a2dp_session_metrics_.audio_duration_ms);
  a2dp_session->set_media_timer_min_millis(
      pimpl_->a2dp_session_metrics_.media_timer_min_ms);
  a2dp_session->set_media_timer_max_millis(
      pimpl_->a2dp_session_metrics_.media_timer_max_ms);
  a2dp_session->set_media_timer_avg_millis(
      pimpl_->a2dp_session_metrics_.media_timer_avg_ms);
  a2dp_session->set_buffer_overruns_max_count(
      pimpl_->a2dp_session_metrics_.buffer_overruns_max_count);
  a2dp_session->set_buffer_overruns_total(
      pimpl_->a2dp_session_metrics_.buffer_overruns_total);
  a2dp_session->set_buffer_underruns_average(
      pimpl_->a2dp_session_metrics_.buffer_underruns_average);
  a2dp_session->set_buffer_underruns_count(
      pimpl_->a2dp_session_metrics_.buffer_underruns_count);
}

void BluetoothMetricsLogger::LogHeadsetProfileRfcConnection(
    tBTA_SERVICE_ID service_id) {
  std::lock_guard<std::recursive_mutex> lock(pimpl_->bluetooth_log_lock_);
  switch (service_id) {
    case BTA_HSP_SERVICE_ID:
      pimpl_->headset_profile_connection_counts_[HeadsetProfileType::HSP]++;
      break;
    case BTA_HFP_SERVICE_ID:
      pimpl_->headset_profile_connection_counts_[HeadsetProfileType::HFP]++;
      break;
    default:
      pimpl_->headset_profile_connection_counts_
          [HeadsetProfileType::HEADSET_PROFILE_UNKNOWN]++;
      break;
  }
  return;
}

void BluetoothMetricsLogger::WriteString(std::string* serialized) {
  std::lock_guard<std::recursive_mutex> lock(pimpl_->bluetooth_log_lock_);
  LOG_DEBUG(LOG_TAG, "%s building metrics", __func__);
  Build();
  LOG_DEBUG(LOG_TAG, "%s serializing metrics", __func__);
  if (!pimpl_->bluetooth_log_->SerializeToString(serialized)) {
    LOG_ERROR(LOG_TAG, "%s: error serializing metrics", __func__);
  }
  // Always clean up log objects
  pimpl_->bluetooth_log_->Clear();
}

void BluetoothMetricsLogger::WriteBase64String(std::string* serialized) {
  this->WriteString(serialized);
  base::Base64Encode(*serialized, serialized);
}

void BluetoothMetricsLogger::WriteBase64(int fd) {
  std::string protoBase64;
  this->WriteBase64String(&protoBase64);
  ssize_t ret;
  OSI_NO_INTR(ret = write(fd, protoBase64.c_str(), protoBase64.size()));
  if (ret == -1) {
    LOG_ERROR(LOG_TAG, "%s: error writing to dumpsys fd: %s (%d)", __func__,
              strerror(errno), errno);
  }
}

void BluetoothMetricsLogger::CutoffSession() {
  std::lock_guard<std::recursive_mutex> lock(pimpl_->bluetooth_session_lock_);
  if (pimpl_->bluetooth_session_ != nullptr) {
    BluetoothSession* new_bt_session =
        new BluetoothSession(*pimpl_->bluetooth_session_);
    new_bt_session->clear_a2dp_session();
    new_bt_session->clear_rfcomm_session();
    LogBluetoothSessionEnd(DISCONNECT_REASON_METRICS_DUMP, 0);
    pimpl_->bluetooth_session_ = new_bt_session;
    pimpl_->bluetooth_session_start_time_ms_ = time_get_os_boottime_ms();
    pimpl_->a2dp_session_metrics_ = A2dpSessionMetrics();
  }
}

void BluetoothMetricsLogger::Build() {
  std::lock_guard<std::recursive_mutex> lock(pimpl_->bluetooth_log_lock_);
  CutoffSession();
  BluetoothLog* bluetooth_log = pimpl_->bluetooth_log_;
  while (!pimpl_->bt_session_queue_->Empty() &&
         static_cast<size_t>(bluetooth_log->session_size()) <=
             pimpl_->bt_session_queue_->Capacity()) {
    bluetooth_log->mutable_session()->AddAllocated(
        pimpl_->bt_session_queue_->Dequeue());
  }
  while (!pimpl_->pair_event_queue_->Empty() &&
         static_cast<size_t>(bluetooth_log->pair_event_size()) <=
             pimpl_->pair_event_queue_->Capacity()) {
    bluetooth_log->mutable_pair_event()->AddAllocated(
        pimpl_->pair_event_queue_->Dequeue());
  }
  while (!pimpl_->scan_event_queue_->Empty() &&
         static_cast<size_t>(bluetooth_log->scan_event_size()) <=
             pimpl_->scan_event_queue_->Capacity()) {
    bluetooth_log->mutable_scan_event()->AddAllocated(
        pimpl_->scan_event_queue_->Dequeue());
  }
  while (!pimpl_->wake_event_queue_->Empty() &&
         static_cast<size_t>(bluetooth_log->wake_event_size()) <=
             pimpl_->wake_event_queue_->Capacity()) {
    bluetooth_log->mutable_wake_event()->AddAllocated(
        pimpl_->wake_event_queue_->Dequeue());
  }
  while (!pimpl_->bt_session_queue_->Empty() &&
         static_cast<size_t>(bluetooth_log->wake_event_size()) <=
             pimpl_->wake_event_queue_->Capacity()) {
    bluetooth_log->mutable_wake_event()->AddAllocated(
        pimpl_->wake_event_queue_->Dequeue());
  }
  for (size_t i = 0; i < HeadsetProfileType_ARRAYSIZE; ++i) {
    int num_times_connected = pimpl_->headset_profile_connection_counts_[i];
    if (HeadsetProfileType_IsValid(i) && num_times_connected > 0) {
      HeadsetProfileConnectionStats* headset_profile_connection_stats =
          bluetooth_log->add_headset_profile_connection_stats();
      // Able to static_cast because HeadsetProfileType_IsValid(i) is true
      headset_profile_connection_stats->set_headset_profile_type(
          static_cast<HeadsetProfileType>(i));
      headset_profile_connection_stats->set_num_times_connected(
          num_times_connected);
    }
  }
  pimpl_->headset_profile_connection_counts_.fill(0);
}

void BluetoothMetricsLogger::ResetSession() {
  std::lock_guard<std::recursive_mutex> lock(pimpl_->bluetooth_session_lock_);
  if (pimpl_->bluetooth_session_ != nullptr) {
    delete pimpl_->bluetooth_session_;
    pimpl_->bluetooth_session_ = nullptr;
  }
  pimpl_->bluetooth_session_start_time_ms_ = 0;
  pimpl_->a2dp_session_metrics_ = A2dpSessionMetrics();
}

void BluetoothMetricsLogger::ResetLog() {
  std::lock_guard<std::recursive_mutex> lock(pimpl_->bluetooth_log_lock_);
  pimpl_->bluetooth_log_->Clear();
}

void BluetoothMetricsLogger::Reset() {
  ResetSession();
  ResetLog();
  pimpl_->bt_session_queue_->Clear();
  pimpl_->pair_event_queue_->Clear();
  pimpl_->wake_event_queue_->Clear();
  pimpl_->scan_event_queue_->Clear();
}

}  // namespace system_bt_osi
