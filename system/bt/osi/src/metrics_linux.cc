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
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <memory>
#include <mutex>

#include <base/base64.h>
#include <base/logging.h>

#include "osi/include/leaky_bonded_queue.h"
#include "osi/include/log.h"
#include "osi/include/osi.h"
#include "osi/include/time.h"

#include "osi/include/metrics.h"

namespace system_bt_osi {

// Maximum number of log entries for each repeated field
#define MAX_NUM_BLUETOOTH_SESSION 50
#define MAX_NUM_PAIR_EVENT 50
#define MAX_NUM_WAKE_EVENT 50
#define MAX_NUM_SCAN_EVENT 50

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
  if (metrics.audio_duration_ms > 0) {
    audio_duration_ms = std::max(static_cast<int64_t>(0), audio_duration_ms);
    audio_duration_ms += metrics.audio_duration_ms;
  }
  if (metrics.media_timer_min_ms > 0) {
    if (media_timer_min_ms < 0) {
      media_timer_min_ms = metrics.media_timer_min_ms;
    } else {
      media_timer_min_ms =
          std::min(media_timer_min_ms, metrics.media_timer_min_ms);
    }
  }
  if (metrics.media_timer_max_ms > 0) {
    media_timer_max_ms =
        std::max(media_timer_max_ms, metrics.media_timer_max_ms);
  }
  if (metrics.media_timer_avg_ms > 0 && metrics.total_scheduling_count > 0) {
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
  if (metrics.buffer_overruns_max_count > 0) {
    buffer_overruns_max_count =
        std::max(buffer_overruns_max_count, metrics.buffer_overruns_max_count);
  }
  if (metrics.buffer_overruns_total > 0) {
    buffer_overruns_total =
        std::max(static_cast<int32_t>(0), buffer_overruns_total);
    buffer_overruns_total += metrics.buffer_overruns_total;
  }
  if (metrics.buffer_underruns_average > 0 &&
      metrics.buffer_underruns_count > 0) {
    if (buffer_underruns_average < 0 || buffer_underruns_count < 0) {
      buffer_underruns_average = metrics.buffer_underruns_average;
      buffer_underruns_count = metrics.buffer_underruns_count;
    } else {
      buffer_underruns_average = combine_averages(
          metrics.buffer_underruns_average, metrics.buffer_underruns_count,
          buffer_underruns_average, buffer_underruns_count);
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

struct BluetoothMetricsLogger::impl {
  // TODO(siyuanh): Implement for linux
};

BluetoothMetricsLogger::BluetoothMetricsLogger() : pimpl_(new impl) {}

void BluetoothMetricsLogger::LogPairEvent(uint32_t disconnect_reason,
                                          uint64_t timestamp_ms,
                                          uint32_t device_class,
                                          device_type_t device_type) {
  // TODO(siyuanh): Implement for linux
}

void BluetoothMetricsLogger::LogWakeEvent(wake_event_type_t type,
                                          const std::string& requestor,
                                          const std::string& name,
                                          uint64_t timestamp_ms) {
  // TODO(siyuanh): Implement for linux
}

void BluetoothMetricsLogger::LogScanEvent(bool start,
                                          const std::string& initator,
                                          scan_tech_t type, uint32_t results,
                                          uint64_t timestamp_ms) {
  // TODO(siyuanh): Implement for linux
}

void BluetoothMetricsLogger::LogBluetoothSessionStart(
    connection_tech_t connection_tech_type, uint64_t timestamp_ms) {
  // TODO(siyuanh): Implement for linux
}

void BluetoothMetricsLogger::LogBluetoothSessionEnd(
    disconnect_reason_t disconnect_reason, uint64_t timestamp_ms) {
  // TODO(siyuanh): Implement for linux
}

void BluetoothMetricsLogger::LogBluetoothSessionDeviceInfo(
    uint32_t device_class, device_type_t device_type) {
  // TODO(siyuanh): Implement for linux
}

void BluetoothMetricsLogger::LogA2dpSession(
    const A2dpSessionMetrics& a2dp_session_metrics) {
  // TODO(siyuanh): Implement for linux
}

void BluetoothMetricsLogger::WriteString(std::string* serialized, bool clear) {
  // TODO(siyuanh): Implement for linux
}

void BluetoothMetricsLogger::WriteBase64String(std::string* serialized,
                                               bool clear) {
  // TODO(siyuanh): Implement for linux
}

void BluetoothMetricsLogger::WriteBase64(int fd, bool clear) {
  // TODO(siyuanh): Implement for linux
}

void BluetoothMetricsLogger::CutoffSession() {
  // TODO(siyuanh): Implement for linux
}

void BluetoothMetricsLogger::Build() {
  // TODO(siyuanh): Implement for linux
}

void BluetoothMetricsLogger::Reset() {
  // TODO(siyuanh): Implement for linux
}

}  // namespace system_bt_osi
