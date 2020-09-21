/******************************************************************************
 *
 *  Copyright 2016 The Android Open Source Project
 *  Copyright 2009-2012 Broadcom Corporation
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

#define LOG_TAG "bt_btif_a2dp_source"
#define ATRACE_TAG ATRACE_TAG_AUDIO

#include <base/run_loop.h>
#ifndef OS_GENERIC
#include <cutils/trace.h>
#endif
#include <inttypes.h>
#include <limits.h>
#include <string.h>
#include <algorithm>

#include "audio_a2dp_hw/include/audio_a2dp_hw.h"
#include "bt_common.h"
#include "bta_av_ci.h"
#include "btif_a2dp.h"
#include "btif_a2dp_audio_interface.h"
#include "btif_a2dp_control.h"
#include "btif_a2dp_source.h"
#include "btif_av.h"
#include "btif_av_co.h"
#include "btif_util.h"
#include "osi/include/fixed_queue.h"
#include "osi/include/log.h"
#include "osi/include/metrics.h"
#include "osi/include/osi.h"
#include "osi/include/thread.h"
#include "osi/include/time.h"
#include "uipc.h"

#include <condition_variable>
#include <mutex>

using system_bt_osi::BluetoothMetricsLogger;
using system_bt_osi::A2dpSessionMetrics;

extern std::unique_ptr<tUIPC_STATE> a2dp_uipc;

/**
 * The typical runlevel of the tx queue size is ~1 buffer
 * but due to link flow control or thread preemption in lower
 * layers we might need to temporarily buffer up data.
 */
#define MAX_OUTPUT_A2DP_FRAME_QUEUE_SZ (MAX_PCM_FRAME_NUM_PER_TICK * 2)

class SchedulingStats {
 public:
  SchedulingStats() { Reset(); }
  void Reset() {
    total_updates = 0;
    last_update_us = 0;
    overdue_scheduling_count = 0;
    total_overdue_scheduling_delta_us = 0;
    max_overdue_scheduling_delta_us = 0;
    premature_scheduling_count = 0;
    total_premature_scheduling_delta_us = 0;
    max_premature_scheduling_delta_us = 0;
    exact_scheduling_count = 0;
    total_scheduling_time_us = 0;
  }

  // Counter for total updates
  size_t total_updates;

  // Last update timestamp (in us)
  uint64_t last_update_us;

  // Counter for overdue scheduling
  size_t overdue_scheduling_count;

  // Accumulated overdue scheduling deviations (in us)
  uint64_t total_overdue_scheduling_delta_us;

  // Max. overdue scheduling delta time (in us)
  uint64_t max_overdue_scheduling_delta_us;

  // Counter for premature scheduling
  size_t premature_scheduling_count;

  // Accumulated premature scheduling deviations (in us)
  uint64_t total_premature_scheduling_delta_us;

  // Max. premature scheduling delta time (in us)
  uint64_t max_premature_scheduling_delta_us;

  // Counter for exact scheduling
  size_t exact_scheduling_count;

  // Accumulated and counted scheduling time (in us)
  uint64_t total_scheduling_time_us;
};

class BtifMediaStats {
 public:
  BtifMediaStats() { Reset(); }
  void Reset() {
    session_start_us = 0;
    session_end_us = 0;
    tx_queue_enqueue_stats.Reset();
    tx_queue_dequeue_stats.Reset();
    tx_queue_total_frames = 0;
    tx_queue_max_frames_per_packet = 0;
    tx_queue_total_queueing_time_us = 0;
    tx_queue_max_queueing_time_us = 0;
    tx_queue_total_readbuf_calls = 0;
    tx_queue_last_readbuf_us = 0;
    tx_queue_total_flushed_messages = 0;
    tx_queue_last_flushed_us = 0;
    tx_queue_total_dropped_messages = 0;
    tx_queue_max_dropped_messages = 0;
    tx_queue_dropouts = 0;
    tx_queue_last_dropouts_us = 0;
    media_read_total_underflow_bytes = 0;
    media_read_total_underflow_count = 0;
    media_read_last_underflow_us = 0;
  }

  uint64_t session_start_us;
  uint64_t session_end_us;

  SchedulingStats tx_queue_enqueue_stats;
  SchedulingStats tx_queue_dequeue_stats;

  size_t tx_queue_total_frames;
  size_t tx_queue_max_frames_per_packet;

  uint64_t tx_queue_total_queueing_time_us;
  uint64_t tx_queue_max_queueing_time_us;

  size_t tx_queue_total_readbuf_calls;
  uint64_t tx_queue_last_readbuf_us;

  size_t tx_queue_total_flushed_messages;
  uint64_t tx_queue_last_flushed_us;

  size_t tx_queue_total_dropped_messages;
  size_t tx_queue_max_dropped_messages;
  size_t tx_queue_dropouts;
  uint64_t tx_queue_last_dropouts_us;

  size_t media_read_total_underflow_bytes;
  size_t media_read_total_underflow_count;
  uint64_t media_read_last_underflow_us;
};

class BtWorkerThread {
 public:
  BtWorkerThread(const std::string& thread_name)
      : thread_name_(thread_name),
        message_loop_(nullptr),
        run_loop_(nullptr),
        message_loop_thread_(nullptr),
        started_(false) {}

  void StartUp() {
    if (message_loop_thread_ != nullptr) {
      return;  // Already started up
    }
    message_loop_thread_ = thread_new(thread_name_.c_str());
    CHECK(message_loop_thread_ != nullptr);
    started_ = false;
    thread_post(message_loop_thread_, &BtWorkerThread::RunThread, this);
    {
      // Block until run_loop_ is allocated and ready to run
      std::unique_lock<std::mutex> start_lock(start_up_mutex_);
      while (!started_) {
        start_up_cv_.wait(start_lock);
      }
    }
  }

  bool DoInThread(const tracked_objects::Location& from_here,
                  const base::Closure& task) {
    if ((message_loop_ == nullptr) || !message_loop_->task_runner().get()) {
      LOG_ERROR(
          LOG_TAG,
          "%s: Dropping message for thread %s: message loop is not initialized",
          __func__, thread_name_.c_str());
      return false;
    }
    if (!message_loop_->task_runner()->PostTask(from_here, task)) {
      LOG_ERROR(LOG_TAG,
                "%s: Posting task to message loop for thread %s failed",
                __func__, thread_name_.c_str());
      return false;
    }
    return true;
  }

  void ShutDown() {
    if ((run_loop_ != nullptr) && (message_loop_ != nullptr)) {
      message_loop_->task_runner()->PostTask(FROM_HERE,
                                             run_loop_->QuitClosure());
    }
    thread_free(message_loop_thread_);
    message_loop_thread_ = nullptr;
  }

 private:
  static void RunThread(void* context) {
    auto wt = static_cast<BtWorkerThread*>(context);
    wt->Run();
  }

  void Run() {
    LOG_INFO(LOG_TAG, "%s: message loop for thread %s started", __func__,
             thread_name_.c_str());
    message_loop_ = new base::MessageLoop();
    run_loop_ = new base::RunLoop();
    {
      std::unique_lock<std::mutex> start_lock(start_up_mutex_);
      started_ = true;
      start_up_cv_.notify_all();
    }
    // Blocking util ShutDown() is called
    run_loop_->Run();
    delete message_loop_;
    message_loop_ = nullptr;
    delete run_loop_;
    run_loop_ = nullptr;
    LOG_INFO(LOG_TAG, "%s: message loop for thread %s finished", __func__,
             thread_name_.c_str());
  }

  std::string thread_name_;
  base::MessageLoop* message_loop_;
  base::RunLoop* run_loop_;
  thread_t* message_loop_thread_;
  // For start-up
  bool started_;
  std::mutex start_up_mutex_;
  std::condition_variable start_up_cv_;
};

class BtifA2dpSource {
 public:
  enum RunState {
    kStateOff,
    kStateStartingUp,
    kStateRunning,
    kStateShuttingDown
  };

  BtifA2dpSource()
      : tx_audio_queue(nullptr),
        tx_flush(false),
        media_alarm(nullptr),
        encoder_interface(nullptr),
        encoder_interval_ms(0),
        state_(kStateOff) {}

  void Reset() {
    fixed_queue_free(tx_audio_queue, nullptr);
    tx_audio_queue = nullptr;
    tx_flush = false;
    alarm_free(media_alarm);
    media_alarm = nullptr;
    encoder_interface = nullptr;
    encoder_interval_ms = 0;
    stats.Reset();
    accumulated_stats.Reset();
    state_ = kStateOff;
  }

  BtifA2dpSource::RunState State() const { return state_; }
  void SetState(BtifA2dpSource::RunState state) { state_ = state; }

  fixed_queue_t* tx_audio_queue;
  bool tx_flush; /* Discards any outgoing data when true */
  alarm_t* media_alarm;
  const tA2DP_ENCODER_INTERFACE* encoder_interface;
  period_ms_t encoder_interval_ms; /* Local copy of the encoder interval */
  BtifMediaStats stats;
  BtifMediaStats accumulated_stats;

 private:
  BtifA2dpSource::RunState state_;
};

static BtWorkerThread btif_a2dp_source_thread("btif_a2dp_source_thread");
static BtifA2dpSource btif_a2dp_source_cb;

static void btif_a2dp_source_init_delayed(void);
static void btif_a2dp_source_startup_delayed(void);
static void btif_a2dp_source_start_session_delayed(
    const RawAddress& peer_address);
static void btif_a2dp_source_end_session_delayed(
    const RawAddress& peer_address);
static void btif_a2dp_source_shutdown_delayed(void);
static void btif_a2dp_source_cleanup_delayed(void);
static void btif_a2dp_source_audio_tx_start_event(void);
static void btif_a2dp_source_audio_tx_stop_event(void);
static void btif_a2dp_source_audio_tx_flush_event(void);
// Set up the A2DP Source codec, and prepare the encoder.
// The peer address is |peer_addr|.
// This function should be called prior to starting A2DP streaming.
static void btif_a2dp_source_setup_codec(const RawAddress& peer_addr);
static void btif_a2dp_source_setup_codec_delayed(
    const RawAddress& peer_address);
static void btif_a2dp_source_encoder_user_config_update_event(
    const RawAddress& peer_address,
    const btav_a2dp_codec_config_t& codec_user_config);
static void btif_a2dp_source_audio_feeding_update_event(
    const btav_a2dp_codec_config_t& codec_audio_config);
static bool btif_a2dp_source_audio_tx_flush_req(void);
static void btif_a2dp_source_alarm_cb(void* context);
static void btif_a2dp_source_audio_handle_timer(void);
static uint32_t btif_a2dp_source_read_callback(uint8_t* p_buf, uint32_t len);
static bool btif_a2dp_source_enqueue_callback(BT_HDR* p_buf, size_t frames_n,
                                              uint32_t bytes_read);
static void log_tstamps_us(const char* comment, uint64_t timestamp_us);
static void update_scheduling_stats(SchedulingStats* stats, uint64_t now_us,
                                    uint64_t expected_delta);
// Update the A2DP Source related metrics.
// This function should be called before collecting the metrics.
static void btif_a2dp_source_update_metrics(void);
static void btm_read_rssi_cb(void* data);
static void btm_read_failed_contact_counter_cb(void* data);
static void btm_read_automatic_flush_timeout_cb(void* data);
static void btm_read_tx_power_cb(void* data);

void btif_a2dp_source_accumulate_scheduling_stats(SchedulingStats* src,
                                                  SchedulingStats* dst) {
  dst->total_updates += src->total_updates;
  dst->last_update_us = src->last_update_us;
  dst->overdue_scheduling_count += src->overdue_scheduling_count;
  dst->total_overdue_scheduling_delta_us +=
      src->total_overdue_scheduling_delta_us;
  dst->max_overdue_scheduling_delta_us =
      std::max(dst->max_overdue_scheduling_delta_us,
               src->max_overdue_scheduling_delta_us);
  dst->premature_scheduling_count += src->premature_scheduling_count;
  dst->total_premature_scheduling_delta_us +=
      src->total_premature_scheduling_delta_us;
  dst->max_premature_scheduling_delta_us =
      std::max(dst->max_premature_scheduling_delta_us,
               src->max_premature_scheduling_delta_us);
  dst->exact_scheduling_count += src->exact_scheduling_count;
  dst->total_scheduling_time_us += src->total_scheduling_time_us;
}

void btif_a2dp_source_accumulate_stats(BtifMediaStats* src,
                                       BtifMediaStats* dst) {
  dst->tx_queue_total_frames += src->tx_queue_total_frames;
  dst->tx_queue_max_frames_per_packet = std::max(
      dst->tx_queue_max_frames_per_packet, src->tx_queue_max_frames_per_packet);
  dst->tx_queue_total_queueing_time_us += src->tx_queue_total_queueing_time_us;
  dst->tx_queue_max_queueing_time_us = std::max(
      dst->tx_queue_max_queueing_time_us, src->tx_queue_max_queueing_time_us);
  dst->tx_queue_total_readbuf_calls += src->tx_queue_total_readbuf_calls;
  dst->tx_queue_last_readbuf_us = src->tx_queue_last_readbuf_us;
  dst->tx_queue_total_flushed_messages += src->tx_queue_total_flushed_messages;
  dst->tx_queue_last_flushed_us = src->tx_queue_last_flushed_us;
  dst->tx_queue_total_dropped_messages += src->tx_queue_total_dropped_messages;
  dst->tx_queue_max_dropped_messages = std::max(
      dst->tx_queue_max_dropped_messages, src->tx_queue_max_dropped_messages);
  dst->tx_queue_dropouts += src->tx_queue_dropouts;
  dst->tx_queue_last_dropouts_us = src->tx_queue_last_dropouts_us;
  dst->media_read_total_underflow_bytes +=
      src->media_read_total_underflow_bytes;
  dst->media_read_total_underflow_count +=
      src->media_read_total_underflow_count;
  dst->media_read_last_underflow_us = src->media_read_last_underflow_us;
  btif_a2dp_source_accumulate_scheduling_stats(&src->tx_queue_enqueue_stats,
                                               &dst->tx_queue_enqueue_stats);
  btif_a2dp_source_accumulate_scheduling_stats(&src->tx_queue_dequeue_stats,
                                               &dst->tx_queue_dequeue_stats);
  src->Reset();
}

bool btif_a2dp_source_init(void) {
  LOG_INFO(LOG_TAG, "%s", __func__);

  // Start A2DP Source media task
  btif_a2dp_source_thread.StartUp();
  btif_a2dp_source_thread.DoInThread(
      FROM_HERE, base::Bind(&btif_a2dp_source_init_delayed));
  return true;
}

static void btif_a2dp_source_init_delayed(void) {
  LOG_INFO(LOG_TAG, "%s", __func__);
  // Nothing to do
}

bool btif_a2dp_source_startup(void) {
  LOG_INFO(LOG_TAG, "%s", __func__);

  if (btif_a2dp_source_cb.State() != BtifA2dpSource::kStateOff) {
    LOG_ERROR(LOG_TAG, "%s: A2DP Source media task already running", __func__);
    return false;
  }

  btif_a2dp_source_cb.Reset();
  btif_a2dp_source_cb.SetState(BtifA2dpSource::kStateStartingUp);
  btif_a2dp_source_cb.tx_audio_queue = fixed_queue_new(SIZE_MAX);

  // Schedule the rest of the operations
  btif_a2dp_source_thread.DoInThread(
      FROM_HERE, base::Bind(&btif_a2dp_source_startup_delayed));

  return true;
}

static void btif_a2dp_source_startup_delayed(void) {
  LOG_INFO(LOG_TAG, "%s", __func__);

  raise_priority_a2dp(TASK_HIGH_MEDIA);
  btif_a2dp_control_init();
  btif_a2dp_source_cb.SetState(BtifA2dpSource::kStateRunning);
  BluetoothMetricsLogger::GetInstance()->LogBluetoothSessionStart(
      system_bt_osi::CONNECTION_TECHNOLOGY_TYPE_BREDR, 0);
}

bool btif_a2dp_source_start_session(const RawAddress& peer_address) {
  LOG_INFO(LOG_TAG, "%s: peer_address=%s", __func__,
           peer_address.ToString().c_str());
  btif_a2dp_source_setup_codec(peer_address);
  btif_a2dp_source_thread.DoInThread(
      FROM_HERE,
      base::Bind(&btif_a2dp_source_start_session_delayed, peer_address));
  return true;
}

static void btif_a2dp_source_start_session_delayed(
    const RawAddress& peer_address) {
  LOG_INFO(LOG_TAG, "%s: peer_address=%s", __func__,
           peer_address.ToString().c_str());
  if (btif_a2dp_source_cb.State() != BtifA2dpSource::kStateRunning) {
    LOG_ERROR(LOG_TAG, "%s: A2DP Source media task is not running", __func__);
    return;
  }
  if (btif_av_is_a2dp_offload_enabled()) {
    btif_a2dp_audio_interface_start_session();
  }
}

bool btif_a2dp_source_restart_session(const RawAddress& old_peer_address,
                                      const RawAddress& new_peer_address) {
  bool is_streaming = alarm_is_scheduled(btif_a2dp_source_cb.media_alarm);
  LOG_INFO(LOG_TAG,
           "%s: old_peer_address=%s new_peer_address=%s is_streaming=%s",
           __func__, old_peer_address.ToString().c_str(),
           new_peer_address.ToString().c_str(), logbool(is_streaming).c_str());

  CHECK(!new_peer_address.IsEmpty());

  // Must stop first the audio streaming
  if (is_streaming) {
    btif_a2dp_source_stop_audio_req();
  }

  // If the old active peer was valid, end the old session.
  // Otherwise, time to startup the A2DP Source processing.
  if (!old_peer_address.IsEmpty()) {
    btif_a2dp_source_end_session(old_peer_address);
  } else {
    btif_a2dp_source_startup();
  }

  // Start the session.
  // If audio was streaming before, start audio streaming as well.
  btif_a2dp_source_start_session(new_peer_address);
  if (is_streaming) {
    btif_a2dp_source_start_audio_req();
  }
  return true;
}

bool btif_a2dp_source_end_session(const RawAddress& peer_address) {
  LOG_INFO(LOG_TAG, "%s: peer_address=%s", __func__,
           peer_address.ToString().c_str());
  btif_a2dp_source_thread.DoInThread(
      FROM_HERE,
      base::Bind(&btif_a2dp_source_end_session_delayed, peer_address));
  return true;
}

static void btif_a2dp_source_end_session_delayed(
    const RawAddress& peer_address) {
  LOG_INFO(LOG_TAG, "%s: peer_address=%s", __func__,
           peer_address.ToString().c_str());
  if (btif_a2dp_source_cb.State() != BtifA2dpSource::kStateRunning) {
    LOG_ERROR(LOG_TAG, "%s: A2DP Source media task is not running", __func__);
    return;
  }
  btif_av_stream_stop(peer_address);
  if (btif_av_is_a2dp_offload_enabled()) {
    btif_a2dp_audio_interface_end_session();
  }
}

void btif_a2dp_source_shutdown(void) {
  LOG_INFO(LOG_TAG, "%s", __func__);

  if ((btif_a2dp_source_cb.State() == BtifA2dpSource::kStateOff) ||
      (btif_a2dp_source_cb.State() == BtifA2dpSource::kStateShuttingDown)) {
    return;
  }

  /* Make sure no channels are restarted while shutting down */
  btif_a2dp_source_cb.SetState(BtifA2dpSource::kStateShuttingDown);

  btif_a2dp_source_thread.DoInThread(
      FROM_HERE, base::Bind(&btif_a2dp_source_shutdown_delayed));
}

static void btif_a2dp_source_shutdown_delayed(void) {
  LOG_INFO(LOG_TAG, "%s", __func__);

  // Stop the timer
  alarm_free(btif_a2dp_source_cb.media_alarm);
  btif_a2dp_source_cb.media_alarm = nullptr;

  btif_a2dp_control_cleanup();
  if (btif_av_is_a2dp_offload_enabled())
    btif_a2dp_audio_interface_end_session();
  fixed_queue_free(btif_a2dp_source_cb.tx_audio_queue, nullptr);
  btif_a2dp_source_cb.tx_audio_queue = nullptr;

  btif_a2dp_source_cb.SetState(BtifA2dpSource::kStateOff);
  BluetoothMetricsLogger::GetInstance()->LogBluetoothSessionEnd(
      system_bt_osi::DISCONNECT_REASON_UNKNOWN, 0);
}

void btif_a2dp_source_cleanup(void) {
  LOG_INFO(LOG_TAG, "%s", __func__);

  // Make sure the source is shutdown
  btif_a2dp_source_shutdown();

  btif_a2dp_source_thread.DoInThread(
      FROM_HERE, base::Bind(&btif_a2dp_source_cleanup_delayed));

  // Exit the thread
  btif_a2dp_source_thread.ShutDown();
}

static void btif_a2dp_source_cleanup_delayed(void) {
  LOG_INFO(LOG_TAG, "%s", __func__);
  // Nothing to do
}

bool btif_a2dp_source_media_task_is_running(void) {
  return (btif_a2dp_source_cb.State() == BtifA2dpSource::kStateRunning);
}

bool btif_a2dp_source_media_task_is_shutting_down(void) {
  return (btif_a2dp_source_cb.State() == BtifA2dpSource::kStateShuttingDown);
}

bool btif_a2dp_source_is_streaming(void) {
  return alarm_is_scheduled(btif_a2dp_source_cb.media_alarm);
}

static void btif_a2dp_source_setup_codec(const RawAddress& peer_address) {
  LOG_INFO(LOG_TAG, "%s: peer_address=%s", __func__,
           peer_address.ToString().c_str());

  // Check to make sure the platform has 8 bits/byte since
  // we're using that in frame size calculations now.
  CHECK(CHAR_BIT == 8);

  btif_a2dp_source_audio_tx_flush_req();
  btif_a2dp_source_thread.DoInThread(
      FROM_HERE,
      base::Bind(&btif_a2dp_source_setup_codec_delayed, peer_address));
}

static void btif_a2dp_source_setup_codec_delayed(
    const RawAddress& peer_address) {
  LOG_INFO(LOG_TAG, "%s: peer_address=%s", __func__,
           peer_address.ToString().c_str());

  tA2DP_ENCODER_INIT_PEER_PARAMS peer_params;
  bta_av_co_get_peer_params(peer_address, &peer_params);

  if (!bta_av_co_set_active_peer(peer_address)) {
    LOG_ERROR(LOG_TAG, "%s: Cannot stream audio: cannot set active peer to %s",
              __func__, peer_address.ToString().c_str());
    return;
  }
  btif_a2dp_source_cb.encoder_interface = bta_av_co_get_encoder_interface();
  if (btif_a2dp_source_cb.encoder_interface == nullptr) {
    LOG_ERROR(LOG_TAG, "%s: Cannot stream audio: no source encoder interface",
              __func__);
    return;
  }

  A2dpCodecConfig* a2dp_codec_config = bta_av_get_a2dp_current_codec();
  if (a2dp_codec_config == nullptr) {
    LOG_ERROR(LOG_TAG, "%s: Cannot stream audio: current codec is not set",
              __func__);
    return;
  }

  btif_a2dp_source_cb.encoder_interface->encoder_init(
      &peer_params, a2dp_codec_config, btif_a2dp_source_read_callback,
      btif_a2dp_source_enqueue_callback);

  // Save a local copy of the encoder_interval_ms
  btif_a2dp_source_cb.encoder_interval_ms =
      btif_a2dp_source_cb.encoder_interface->get_encoder_interval_ms();
}

void btif_a2dp_source_start_audio_req(void) {
  LOG_INFO(LOG_TAG, "%s", __func__);

  btif_a2dp_source_thread.DoInThread(
      FROM_HERE, base::Bind(&btif_a2dp_source_audio_tx_start_event));
  btif_a2dp_source_cb.stats.Reset();
  // Assign session_start_us to 1 when time_get_os_boottime_us() is 0 to
  // indicate btif_a2dp_source_start_audio_req() has been called
  btif_a2dp_source_cb.stats.session_start_us = time_get_os_boottime_us();
  if (btif_a2dp_source_cb.stats.session_start_us == 0) {
    btif_a2dp_source_cb.stats.session_start_us = 1;
  }
  btif_a2dp_source_cb.stats.session_end_us = 0;
}

void btif_a2dp_source_stop_audio_req(void) {
  LOG_INFO(LOG_TAG, "%s", __func__);

  btif_a2dp_source_thread.DoInThread(
      FROM_HERE, base::Bind(&btif_a2dp_source_audio_tx_stop_event));

  btif_a2dp_source_cb.stats.session_end_us = time_get_os_boottime_us();
  btif_a2dp_source_update_metrics();
  btif_a2dp_source_accumulate_stats(&btif_a2dp_source_cb.stats,
                                    &btif_a2dp_source_cb.accumulated_stats);
}

void btif_a2dp_source_encoder_user_config_update_req(
    const RawAddress& peer_address,
    const btav_a2dp_codec_config_t& codec_user_config) {
  LOG_INFO(LOG_TAG, "%s: peer_address=%s", __func__,
           peer_address.ToString().c_str());
  btif_a2dp_source_thread.DoInThread(
      FROM_HERE, base::Bind(&btif_a2dp_source_encoder_user_config_update_event,
                            peer_address, codec_user_config));
}

static void btif_a2dp_source_encoder_user_config_update_event(
    const RawAddress& peer_address,
    const btav_a2dp_codec_config_t& codec_user_config) {
  LOG_INFO(LOG_TAG, "%s: peer_address=%s", __func__,
           peer_address.ToString().c_str());
  if (!bta_av_co_set_codec_user_config(peer_address, codec_user_config)) {
    LOG_ERROR(LOG_TAG, "%s: cannot update codec user configuration", __func__);
  }
}

void btif_a2dp_source_feeding_update_req(
    const btav_a2dp_codec_config_t& codec_audio_config) {
  LOG_INFO(LOG_TAG, "%s", __func__);
  btif_a2dp_source_thread.DoInThread(
      FROM_HERE, base::Bind(&btif_a2dp_source_audio_feeding_update_event,
                            codec_audio_config));
}

static void btif_a2dp_source_audio_feeding_update_event(
    const btav_a2dp_codec_config_t& codec_audio_config) {
  LOG_INFO(LOG_TAG, "%s", __func__);
  if (!bta_av_co_set_codec_audio_config(codec_audio_config)) {
    LOG_ERROR(LOG_TAG, "%s: cannot update codec audio feeding parameters",
              __func__);
  }
}

void btif_a2dp_source_on_idle(void) {
  LOG_INFO(LOG_TAG, "%s", __func__);
  if (btif_a2dp_source_cb.State() == BtifA2dpSource::kStateOff) return;

  /* Make sure media task is stopped */
  btif_a2dp_source_stop_audio_req();
}

void btif_a2dp_source_on_stopped(tBTA_AV_SUSPEND* p_av_suspend) {
  LOG_INFO(LOG_TAG, "%s", __func__);

  if (btif_a2dp_source_cb.State() == BtifA2dpSource::kStateOff) return;

  /* allow using this api for other than suspend */
  if (p_av_suspend != nullptr) {
    if (p_av_suspend->status != BTA_AV_SUCCESS) {
      LOG_ERROR(LOG_TAG, "%s: A2DP stop request failed: status=%d", __func__,
                p_av_suspend->status);
      if (p_av_suspend->initiator) {
        LOG_WARN(LOG_TAG, "%s: A2DP stop request failed: status=%d", __func__,
                 p_av_suspend->status);
        btif_a2dp_command_ack(A2DP_CTRL_ACK_FAILURE);
      }
      return;
    }
  }

  /* ensure tx frames are immediately suspended */
  btif_a2dp_source_cb.tx_flush = true;

  /* request to stop media task */
  btif_a2dp_source_audio_tx_flush_req();
  btif_a2dp_source_stop_audio_req();

  /* once stream is fully stopped we will ack back */
}

void btif_a2dp_source_on_suspended(tBTA_AV_SUSPEND* p_av_suspend) {
  LOG_INFO(LOG_TAG, "%s", __func__);

  if (btif_a2dp_source_cb.State() == BtifA2dpSource::kStateOff) return;

  /* check for status failures */
  if (p_av_suspend->status != BTA_AV_SUCCESS) {
    if (p_av_suspend->initiator) {
      LOG_WARN(LOG_TAG, "%s: A2DP suspend request failed: status=%d", __func__,
               p_av_suspend->status);
      btif_a2dp_command_ack(A2DP_CTRL_ACK_FAILURE);
    }
  }

  /* once stream is fully stopped we will ack back */

  /* ensure tx frames are immediately flushed */
  btif_a2dp_source_cb.tx_flush = true;

  /* stop timer tick */
  btif_a2dp_source_stop_audio_req();
}

/* when true media task discards any tx frames */
void btif_a2dp_source_set_tx_flush(bool enable) {
  LOG_INFO(LOG_TAG, "%s: enable=%s", __func__, (enable) ? "true" : "false");
  btif_a2dp_source_cb.tx_flush = enable;
}

static void btif_a2dp_source_audio_tx_start_event(void) {
  LOG_INFO(LOG_TAG, "%s: media_alarm is %srunning, streaming %s", __func__,
           alarm_is_scheduled(btif_a2dp_source_cb.media_alarm) ? "" : "not ",
           btif_a2dp_source_is_streaming() ? "true" : "false");

  if (btif_av_is_a2dp_offload_enabled()) return;

  /* Reset the media feeding state */
  CHECK(btif_a2dp_source_cb.encoder_interface != nullptr);
  btif_a2dp_source_cb.encoder_interface->feeding_reset();

  APPL_TRACE_EVENT(
      "%s: starting timer %" PRIu64 " ms", __func__,
      btif_a2dp_source_cb.encoder_interface->get_encoder_interval_ms());
  alarm_free(btif_a2dp_source_cb.media_alarm);
  btif_a2dp_source_cb.media_alarm =
      alarm_new_periodic("btif.a2dp_source_media_alarm");
  if (btif_a2dp_source_cb.media_alarm == nullptr) {
    LOG_ERROR(LOG_TAG, "%s: unable to allocate media alarm", __func__);
    return;
  }

  alarm_set(btif_a2dp_source_cb.media_alarm,
            btif_a2dp_source_cb.encoder_interface->get_encoder_interval_ms(),
            btif_a2dp_source_alarm_cb, nullptr);
}

static void btif_a2dp_source_audio_tx_stop_event(void) {
  LOG_INFO(LOG_TAG, "%s: media_alarm is %srunning, streaming %s", __func__,
           alarm_is_scheduled(btif_a2dp_source_cb.media_alarm) ? "" : "not ",
           btif_a2dp_source_is_streaming() ? "true" : "false");

  if (btif_av_is_a2dp_offload_enabled()) return;

  uint8_t p_buf[AUDIO_STREAM_OUTPUT_BUFFER_SZ * 2];
  uint16_t event;

  // Keep track of audio data still left in the pipe
  btif_a2dp_control_log_bytes_read(
      UIPC_Read(*a2dp_uipc, UIPC_CH_ID_AV_AUDIO, &event, p_buf, sizeof(p_buf)));

  /* Stop the timer first */
  alarm_free(btif_a2dp_source_cb.media_alarm);
  btif_a2dp_source_cb.media_alarm = nullptr;

  UIPC_Close(*a2dp_uipc, UIPC_CH_ID_AV_AUDIO);

  /*
   * Try to send acknowldegment once the media stream is
   * stopped. This will make sure that the A2DP HAL layer is
   * un-blocked on wait for acknowledgment for the sent command.
   * This resolves a corner cases AVDTP SUSPEND collision
   * when the DUT and the remote device issue SUSPEND simultaneously
   * and due to the processing of the SUSPEND request from the remote,
   * the media path is torn down. If the A2DP HAL happens to wait
   * for ACK for the initiated SUSPEND, it would never receive it casuing
   * a block/wait. Due to this acknowledgement, the A2DP HAL is guranteed
   * to get the ACK for any pending command in such cases.
   */

  btif_a2dp_command_ack(A2DP_CTRL_ACK_SUCCESS);

  /* audio engine stopped, reset tx suspended flag */
  btif_a2dp_source_cb.tx_flush = false;

  /* Reset the media feeding state */
  if (btif_a2dp_source_cb.encoder_interface != nullptr)
    btif_a2dp_source_cb.encoder_interface->feeding_reset();
}

static void btif_a2dp_source_alarm_cb(UNUSED_ATTR void* context) {
  btif_a2dp_source_thread.DoInThread(
      FROM_HERE, base::Bind(&btif_a2dp_source_audio_handle_timer));
}

static void btif_a2dp_source_audio_handle_timer(void) {
  if (btif_av_is_a2dp_offload_enabled()) return;

  uint64_t timestamp_us = time_get_os_boottime_us();
  log_tstamps_us("A2DP Source tx timer", timestamp_us);

  if (!alarm_is_scheduled(btif_a2dp_source_cb.media_alarm)) {
    LOG_ERROR(LOG_TAG, "%s: ERROR Media task Scheduled after Suspend",
              __func__);
    return;
  }
  CHECK(btif_a2dp_source_cb.encoder_interface != nullptr);
  size_t transmit_queue_length =
      fixed_queue_length(btif_a2dp_source_cb.tx_audio_queue);
#ifndef OS_GENERIC
  ATRACE_INT("btif TX queue", transmit_queue_length);
#endif
  if (btif_a2dp_source_cb.encoder_interface->set_transmit_queue_length !=
      nullptr) {
    btif_a2dp_source_cb.encoder_interface->set_transmit_queue_length(
        transmit_queue_length);
  }
  btif_a2dp_source_cb.encoder_interface->send_frames(timestamp_us);
  bta_av_ci_src_data_ready(BTA_AV_CHNL_AUDIO);
  update_scheduling_stats(&btif_a2dp_source_cb.stats.tx_queue_enqueue_stats,
                          timestamp_us,
                          btif_a2dp_source_cb.encoder_interval_ms * 1000);
}

static uint32_t btif_a2dp_source_read_callback(uint8_t* p_buf, uint32_t len) {
  uint16_t event;
  uint32_t bytes_read =
      UIPC_Read(*a2dp_uipc, UIPC_CH_ID_AV_AUDIO, &event, p_buf, len);

  if (bytes_read < len) {
    LOG_WARN(LOG_TAG, "%s: UNDERFLOW: ONLY READ %d BYTES OUT OF %d", __func__,
             bytes_read, len);
    btif_a2dp_source_cb.stats.media_read_total_underflow_bytes +=
        (len - bytes_read);
    btif_a2dp_source_cb.stats.media_read_total_underflow_count++;
    btif_a2dp_source_cb.stats.media_read_last_underflow_us =
        time_get_os_boottime_us();
  }

  return bytes_read;
}

static bool btif_a2dp_source_enqueue_callback(BT_HDR* p_buf, size_t frames_n,
                                              uint32_t bytes_read) {
  uint64_t now_us = time_get_os_boottime_us();
  btif_a2dp_control_log_bytes_read(bytes_read);

  /* Check if timer was stopped (media task stopped) */
  if (!alarm_is_scheduled(btif_a2dp_source_cb.media_alarm)) {
    osi_free(p_buf);
    return false;
  }

  /* Check if the transmission queue has been flushed */
  if (btif_a2dp_source_cb.tx_flush) {
    LOG_VERBOSE(LOG_TAG, "%s: tx suspended, discarded frame", __func__);

    btif_a2dp_source_cb.stats.tx_queue_total_flushed_messages +=
        fixed_queue_length(btif_a2dp_source_cb.tx_audio_queue);
    btif_a2dp_source_cb.stats.tx_queue_last_flushed_us = now_us;
    fixed_queue_flush(btif_a2dp_source_cb.tx_audio_queue, osi_free);

    osi_free(p_buf);
    return false;
  }

  // Check for TX queue overflow
  // TODO: Using frames_n here is probably wrong: should be "+ 1" instead.
  if (fixed_queue_length(btif_a2dp_source_cb.tx_audio_queue) + frames_n >
      MAX_OUTPUT_A2DP_FRAME_QUEUE_SZ) {
    LOG_WARN(LOG_TAG, "%s: TX queue buffer size now=%u adding=%u max=%d",
             __func__,
             (uint32_t)fixed_queue_length(btif_a2dp_source_cb.tx_audio_queue),
             (uint32_t)frames_n, MAX_OUTPUT_A2DP_FRAME_QUEUE_SZ);
    // Keep track of drop-outs
    btif_a2dp_source_cb.stats.tx_queue_dropouts++;
    btif_a2dp_source_cb.stats.tx_queue_last_dropouts_us = now_us;

    // Flush all queued buffers
    size_t drop_n = fixed_queue_length(btif_a2dp_source_cb.tx_audio_queue);
    btif_a2dp_source_cb.stats.tx_queue_max_dropped_messages = std::max(
        drop_n, btif_a2dp_source_cb.stats.tx_queue_max_dropped_messages);
    while (fixed_queue_length(btif_a2dp_source_cb.tx_audio_queue)) {
      btif_a2dp_source_cb.stats.tx_queue_total_dropped_messages++;
      osi_free(fixed_queue_try_dequeue(btif_a2dp_source_cb.tx_audio_queue));
    }

    // Request additional debug info if we had to flush buffers
    RawAddress peer_bda = btif_av_source_active_peer();
    tBTM_STATUS status = BTM_ReadRSSI(peer_bda, btm_read_rssi_cb);
    if (status != BTM_CMD_STARTED) {
      LOG_WARN(LOG_TAG, "%s: Cannot read RSSI: status %d", __func__, status);
    }
    status = BTM_ReadFailedContactCounter(peer_bda,
                                          btm_read_failed_contact_counter_cb);
    if (status != BTM_CMD_STARTED) {
      LOG_WARN(LOG_TAG, "%s: Cannot read Failed Contact Counter: status %d",
               __func__, status);
    }
    status = BTM_ReadAutomaticFlushTimeout(peer_bda,
                                           btm_read_automatic_flush_timeout_cb);
    if (status != BTM_CMD_STARTED) {
      LOG_WARN(LOG_TAG, "%s: Cannot read Automatic Flush Timeout: status %d",
               __func__, status);
    }
    status =
        BTM_ReadTxPower(peer_bda, BT_TRANSPORT_BR_EDR, btm_read_tx_power_cb);
    if (status != BTM_CMD_STARTED) {
      LOG_WARN(LOG_TAG, "%s: Cannot read Tx Power: status %d", __func__,
               status);
    }
  }

  /* Update the statistics */
  btif_a2dp_source_cb.stats.tx_queue_total_frames += frames_n;
  btif_a2dp_source_cb.stats.tx_queue_max_frames_per_packet = std::max(
      frames_n, btif_a2dp_source_cb.stats.tx_queue_max_frames_per_packet);
  CHECK(btif_a2dp_source_cb.encoder_interface != nullptr);

  fixed_queue_enqueue(btif_a2dp_source_cb.tx_audio_queue, p_buf);

  return true;
}

static void btif_a2dp_source_audio_tx_flush_event(void) {
  /* Flush all enqueued audio buffers (encoded) */
  LOG_INFO(LOG_TAG, "%s", __func__);
  if (btif_av_is_a2dp_offload_enabled()) return;

  if (btif_a2dp_source_cb.encoder_interface != nullptr)
    btif_a2dp_source_cb.encoder_interface->feeding_flush();

  btif_a2dp_source_cb.stats.tx_queue_total_flushed_messages +=
      fixed_queue_length(btif_a2dp_source_cb.tx_audio_queue);
  btif_a2dp_source_cb.stats.tx_queue_last_flushed_us =
      time_get_os_boottime_us();
  fixed_queue_flush(btif_a2dp_source_cb.tx_audio_queue, osi_free);

  UIPC_Ioctl(*a2dp_uipc, UIPC_CH_ID_AV_AUDIO, UIPC_REQ_RX_FLUSH, nullptr);
}

static bool btif_a2dp_source_audio_tx_flush_req(void) {
  LOG_INFO(LOG_TAG, "%s", __func__);

  btif_a2dp_source_thread.DoInThread(
      FROM_HERE, base::Bind(&btif_a2dp_source_audio_tx_flush_event));
  return true;
}

BT_HDR* btif_a2dp_source_audio_readbuf(void) {
  uint64_t now_us = time_get_os_boottime_us();
  BT_HDR* p_buf =
      (BT_HDR*)fixed_queue_try_dequeue(btif_a2dp_source_cb.tx_audio_queue);

  btif_a2dp_source_cb.stats.tx_queue_total_readbuf_calls++;
  btif_a2dp_source_cb.stats.tx_queue_last_readbuf_us = now_us;
  if (p_buf != nullptr) {
    // Update the statistics
    update_scheduling_stats(&btif_a2dp_source_cb.stats.tx_queue_dequeue_stats,
                            now_us,
                            btif_a2dp_source_cb.encoder_interval_ms * 1000);
  }

  return p_buf;
}

static void log_tstamps_us(const char* comment, uint64_t timestamp_us) {
  static uint64_t prev_us = 0;
  APPL_TRACE_DEBUG("%s: [%s] ts %08" PRIu64 ", diff : %08" PRIu64
                   ", queue sz %zu",
                   __func__, comment, timestamp_us, timestamp_us - prev_us,
                   fixed_queue_length(btif_a2dp_source_cb.tx_audio_queue));
  prev_us = timestamp_us;
}

static void update_scheduling_stats(SchedulingStats* stats, uint64_t now_us,
                                    uint64_t expected_delta) {
  uint64_t last_us = stats->last_update_us;

  stats->total_updates++;
  stats->last_update_us = now_us;

  if (last_us == 0) return;  // First update: expected delta doesn't apply

  uint64_t deadline_us = last_us + expected_delta;
  if (deadline_us < now_us) {
    // Overdue scheduling
    uint64_t delta_us = now_us - deadline_us;
    // Ignore extreme outliers
    if (delta_us < 10 * expected_delta) {
      stats->max_overdue_scheduling_delta_us =
          std::max(delta_us, stats->max_overdue_scheduling_delta_us);
      stats->total_overdue_scheduling_delta_us += delta_us;
      stats->overdue_scheduling_count++;
      stats->total_scheduling_time_us += now_us - last_us;
    }
  } else if (deadline_us > now_us) {
    // Premature scheduling
    uint64_t delta_us = deadline_us - now_us;
    // Ignore extreme outliers
    if (delta_us < 10 * expected_delta) {
      stats->max_premature_scheduling_delta_us =
          std::max(delta_us, stats->max_premature_scheduling_delta_us);
      stats->total_premature_scheduling_delta_us += delta_us;
      stats->premature_scheduling_count++;
      stats->total_scheduling_time_us += now_us - last_us;
    }
  } else {
    // On-time scheduling
    stats->exact_scheduling_count++;
    stats->total_scheduling_time_us += now_us - last_us;
  }
}

void btif_a2dp_source_debug_dump(int fd) {
  btif_a2dp_source_accumulate_stats(&btif_a2dp_source_cb.stats,
                                    &btif_a2dp_source_cb.accumulated_stats);
  uint64_t now_us = time_get_os_boottime_us();
  BtifMediaStats* accumulated_stats = &btif_a2dp_source_cb.accumulated_stats;
  SchedulingStats* enqueue_stats = &accumulated_stats->tx_queue_enqueue_stats;
  SchedulingStats* dequeue_stats = &accumulated_stats->tx_queue_dequeue_stats;
  size_t ave_size;
  uint64_t ave_time_us;

  dprintf(fd, "\nA2DP State:\n");
  dprintf(fd, "  TxQueue:\n");

  dprintf(fd,
          "  Counts (enqueue/dequeue/readbuf)                        : %zu / "
          "%zu / %zu\n",
          enqueue_stats->total_updates, dequeue_stats->total_updates,
          accumulated_stats->tx_queue_total_readbuf_calls);

  dprintf(
      fd,
      "  Last update time ago in ms (enqueue/dequeue/readbuf)    : %llu / %llu "
      "/ %llu\n",
      (enqueue_stats->last_update_us > 0)
          ? (unsigned long long)(now_us - enqueue_stats->last_update_us) / 1000
          : 0,
      (dequeue_stats->last_update_us > 0)
          ? (unsigned long long)(now_us - dequeue_stats->last_update_us) / 1000
          : 0,
      (accumulated_stats->tx_queue_last_readbuf_us > 0)
          ? (unsigned long long)(now_us -
                                 accumulated_stats->tx_queue_last_readbuf_us) /
                1000
          : 0);

  ave_size = 0;
  if (enqueue_stats->total_updates != 0)
    ave_size =
        accumulated_stats->tx_queue_total_frames / enqueue_stats->total_updates;
  dprintf(fd,
          "  Frames per packet (total/max/ave)                       : %zu / "
          "%zu / %zu\n",
          accumulated_stats->tx_queue_total_frames,
          accumulated_stats->tx_queue_max_frames_per_packet, ave_size);

  dprintf(fd,
          "  Counts (flushed/dropped/dropouts)                       : %zu / "
          "%zu / %zu\n",
          accumulated_stats->tx_queue_total_flushed_messages,
          accumulated_stats->tx_queue_total_dropped_messages,
          accumulated_stats->tx_queue_dropouts);

  dprintf(fd,
          "  Counts (max dropped)                                    : %zu\n",
          accumulated_stats->tx_queue_max_dropped_messages);

  dprintf(
      fd,
      "  Last update time ago in ms (flushed/dropped)            : %llu / "
      "%llu\n",
      (accumulated_stats->tx_queue_last_flushed_us > 0)
          ? (unsigned long long)(now_us -
                                 accumulated_stats->tx_queue_last_flushed_us) /
                1000
          : 0,
      (accumulated_stats->tx_queue_last_dropouts_us > 0)
          ? (unsigned long long)(now_us -
                                 accumulated_stats->tx_queue_last_dropouts_us) /
                1000
          : 0);

  dprintf(fd,
          "  Counts (underflow)                                      : %zu\n",
          accumulated_stats->media_read_total_underflow_count);

  dprintf(fd,
          "  Bytes (underflow)                                       : %zu\n",
          accumulated_stats->media_read_total_underflow_bytes);

  dprintf(fd,
          "  Last update time ago in ms (underflow)                  : %llu\n",
          (accumulated_stats->media_read_last_underflow_us > 0)
              ? (unsigned long long)(now_us -
                                     accumulated_stats
                                         ->media_read_last_underflow_us) /
                    1000
              : 0);

  //
  // TxQueue enqueue stats
  //
  dprintf(
      fd,
      "  Enqueue deviation counts (overdue/premature)            : %zu / %zu\n",
      enqueue_stats->overdue_scheduling_count,
      enqueue_stats->premature_scheduling_count);

  ave_time_us = 0;
  if (enqueue_stats->overdue_scheduling_count != 0) {
    ave_time_us = enqueue_stats->total_overdue_scheduling_delta_us /
                  enqueue_stats->overdue_scheduling_count;
  }
  dprintf(
      fd,
      "  Enqueue overdue scheduling time in ms (total/max/ave)   : %llu / %llu "
      "/ %llu\n",
      (unsigned long long)enqueue_stats->total_overdue_scheduling_delta_us /
          1000,
      (unsigned long long)enqueue_stats->max_overdue_scheduling_delta_us / 1000,
      (unsigned long long)ave_time_us / 1000);

  ave_time_us = 0;
  if (enqueue_stats->premature_scheduling_count != 0) {
    ave_time_us = enqueue_stats->total_premature_scheduling_delta_us /
                  enqueue_stats->premature_scheduling_count;
  }
  dprintf(
      fd,
      "  Enqueue premature scheduling time in ms (total/max/ave) : %llu / %llu "
      "/ %llu\n",
      (unsigned long long)enqueue_stats->total_premature_scheduling_delta_us /
          1000,
      (unsigned long long)enqueue_stats->max_premature_scheduling_delta_us /
          1000,
      (unsigned long long)ave_time_us / 1000);

  //
  // TxQueue dequeue stats
  //
  dprintf(
      fd,
      "  Dequeue deviation counts (overdue/premature)            : %zu / %zu\n",
      dequeue_stats->overdue_scheduling_count,
      dequeue_stats->premature_scheduling_count);

  ave_time_us = 0;
  if (dequeue_stats->overdue_scheduling_count != 0) {
    ave_time_us = dequeue_stats->total_overdue_scheduling_delta_us /
                  dequeue_stats->overdue_scheduling_count;
  }
  dprintf(
      fd,
      "  Dequeue overdue scheduling time in ms (total/max/ave)   : %llu / %llu "
      "/ %llu\n",
      (unsigned long long)dequeue_stats->total_overdue_scheduling_delta_us /
          1000,
      (unsigned long long)dequeue_stats->max_overdue_scheduling_delta_us / 1000,
      (unsigned long long)ave_time_us / 1000);

  ave_time_us = 0;
  if (dequeue_stats->premature_scheduling_count != 0) {
    ave_time_us = dequeue_stats->total_premature_scheduling_delta_us /
                  dequeue_stats->premature_scheduling_count;
  }
  dprintf(
      fd,
      "  Dequeue premature scheduling time in ms (total/max/ave) : %llu / %llu "
      "/ %llu\n",
      (unsigned long long)dequeue_stats->total_premature_scheduling_delta_us /
          1000,
      (unsigned long long)dequeue_stats->max_premature_scheduling_delta_us /
          1000,
      (unsigned long long)ave_time_us / 1000);
}

static void btif_a2dp_source_update_metrics(void) {
  const BtifMediaStats& stats = btif_a2dp_source_cb.stats;
  const SchedulingStats& enqueue_stats = stats.tx_queue_enqueue_stats;
  A2dpSessionMetrics metrics;
  // session_start_us is 0 when btif_a2dp_source_start_audio_req() is not called
  // mark the metric duration as invalid (-1) in this case
  if (stats.session_start_us != 0) {
    int64_t session_end_us = stats.session_end_us == 0
                                 ? time_get_os_boottime_us()
                                 : stats.session_end_us;
    metrics.audio_duration_ms =
        (session_end_us - stats.session_start_us) / 1000;
  }

  if (enqueue_stats.total_updates > 1) {
    metrics.media_timer_min_ms =
        btif_a2dp_source_cb.encoder_interval_ms -
        (enqueue_stats.max_premature_scheduling_delta_us / 1000);
    metrics.media_timer_max_ms =
        btif_a2dp_source_cb.encoder_interval_ms +
        (enqueue_stats.max_overdue_scheduling_delta_us / 1000);

    metrics.total_scheduling_count = enqueue_stats.overdue_scheduling_count +
                                     enqueue_stats.premature_scheduling_count +
                                     enqueue_stats.exact_scheduling_count;
    if (metrics.total_scheduling_count > 0) {
      metrics.media_timer_avg_ms = enqueue_stats.total_scheduling_time_us /
                                   (1000 * metrics.total_scheduling_count);
    }

    metrics.buffer_overruns_max_count = stats.tx_queue_max_dropped_messages;
    metrics.buffer_overruns_total = stats.tx_queue_total_dropped_messages;
    metrics.buffer_underruns_count = stats.media_read_total_underflow_count;
    metrics.buffer_underruns_average = 0;
    if (metrics.buffer_underruns_count > 0) {
      metrics.buffer_underruns_average =
          stats.media_read_total_underflow_bytes /
          metrics.buffer_underruns_count;
    }
  }
  BluetoothMetricsLogger::GetInstance()->LogA2dpSession(metrics);
}

static void btm_read_rssi_cb(void* data) {
  if (data == nullptr) {
    LOG_ERROR(LOG_TAG, "%s: Read RSSI request timed out", __func__);
    return;
  }

  tBTM_RSSI_RESULT* result = (tBTM_RSSI_RESULT*)data;
  if (result->status != BTM_SUCCESS) {
    LOG_ERROR(LOG_TAG, "%s: unable to read remote RSSI (status %d)", __func__,
              result->status);
    return;
  }

  LOG_WARN(LOG_TAG, "%s: device: %s, rssi: %d", __func__,
           result->rem_bda.ToString().c_str(), result->rssi);
}

static void btm_read_failed_contact_counter_cb(void* data) {
  if (data == nullptr) {
    LOG_ERROR(LOG_TAG, "%s: Read Failed Contact Counter request timed out",
              __func__);
    return;
  }

  tBTM_FAILED_CONTACT_COUNTER_RESULT* result =
      (tBTM_FAILED_CONTACT_COUNTER_RESULT*)data;
  if (result->status != BTM_SUCCESS) {
    LOG_ERROR(LOG_TAG, "%s: unable to read Failed Contact Counter (status %d)",
              __func__, result->status);
    return;
  }

  LOG_WARN(LOG_TAG, "%s: device: %s, Failed Contact Counter: %u", __func__,
           result->rem_bda.ToString().c_str(), result->failed_contact_counter);
}

static void btm_read_automatic_flush_timeout_cb(void* data) {
  if (data == nullptr) {
    LOG_ERROR(LOG_TAG, "%s: Read Automatic Flush Timeout request timed out",
              __func__);
    return;
  }

  tBTM_AUTOMATIC_FLUSH_TIMEOUT_RESULT* result =
      (tBTM_AUTOMATIC_FLUSH_TIMEOUT_RESULT*)data;
  if (result->status != BTM_SUCCESS) {
    LOG_ERROR(LOG_TAG, "%s: unable to read Automatic Flush Timeout (status %d)",
              __func__, result->status);
    return;
  }

  LOG_WARN(LOG_TAG, "%s: device: %s, Automatic Flush Timeout: %u", __func__,
           result->rem_bda.ToString().c_str(), result->automatic_flush_timeout);
}

static void btm_read_tx_power_cb(void* data) {
  if (data == nullptr) {
    LOG_ERROR(LOG_TAG, "%s: Read Tx Power request timed out", __func__);
    return;
  }

  tBTM_TX_POWER_RESULT* result = (tBTM_TX_POWER_RESULT*)data;
  if (result->status != BTM_SUCCESS) {
    LOG_ERROR(LOG_TAG, "%s: unable to read Tx Power (status %d)", __func__,
              result->status);
    return;
  }

  LOG_WARN(LOG_TAG, "%s: device: %s, Tx Power: %d", __func__,
           result->rem_bda.ToString().c_str(), result->tx_power);
}
