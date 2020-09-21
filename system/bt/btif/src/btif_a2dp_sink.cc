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

#define LOG_TAG "bt_btif_a2dp_sink"

#include <atomic>
#include <cstring>
#include <mutex>

#include "bt_common.h"
#include "btif_a2dp.h"
#include "btif_a2dp_sink.h"
#include "btif_av.h"
#include "btif_av_co.h"
#include "btif_avrcp_audio_track.h"
#include "btif_util.h"
#include "osi/include/fixed_queue.h"
#include "osi/include/log.h"
#include "osi/include/osi.h"
#include "osi/include/thread.h"

using LockGuard = std::lock_guard<std::mutex>;

/**
 * The receiving queue buffer size.
 */
#define MAX_INPUT_A2DP_FRAME_QUEUE_SZ (MAX_PCM_FRAME_NUM_PER_TICK * 2)

#define BTIF_SINK_MEDIA_TIME_TICK_MS 20

/* In case of A2DP Sink, we will delay start by 5 AVDTP Packets */
#define MAX_A2DP_DELAYED_START_FRAME_COUNT 5

enum {
  BTIF_A2DP_SINK_STATE_OFF,
  BTIF_A2DP_SINK_STATE_STARTING_UP,
  BTIF_A2DP_SINK_STATE_RUNNING,
  BTIF_A2DP_SINK_STATE_SHUTTING_DOWN
};

/* BTIF Media Sink command event definition */
enum {
  BTIF_MEDIA_SINK_DECODER_UPDATE = 1,
  BTIF_MEDIA_SINK_CLEAR_TRACK,
  BTIF_MEDIA_SINK_SET_FOCUS_STATE,
  BTIF_MEDIA_SINK_AUDIO_RX_FLUSH
};

typedef struct {
  BT_HDR hdr;
  uint8_t codec_info[AVDT_CODEC_SIZE];
} tBTIF_MEDIA_SINK_DECODER_UPDATE;

typedef struct {
  BT_HDR hdr;
  btif_a2dp_sink_focus_state_t focus_state;
} tBTIF_MEDIA_SINK_FOCUS_UPDATE;

/* BTIF A2DP Sink control block */
typedef struct {
  thread_t* worker_thread;
  fixed_queue_t* cmd_msg_queue;
  fixed_queue_t* rx_audio_queue;
  bool rx_flush; /* discards any incoming data when true */
  alarm_t* decode_alarm;
  tA2DP_SAMPLE_RATE sample_rate;
  tA2DP_CHANNEL_COUNT channel_count;
  btif_a2dp_sink_focus_state_t rx_focus_state; /* audio focus state */
  void* audio_track;
  const tA2DP_DECODER_INTERFACE* decoder_interface;
} tBTIF_A2DP_SINK_CB;

// Mutex for below data structures.
static std::mutex g_mutex;

static tBTIF_A2DP_SINK_CB btif_a2dp_sink_cb;

static std::atomic<int> btif_a2dp_sink_state{BTIF_A2DP_SINK_STATE_OFF};

static void btif_a2dp_sink_init_delayed(void* context);
static void btif_a2dp_sink_startup_delayed(void* context);
static void btif_a2dp_sink_start_session_delayed(void* context);
static void btif_a2dp_sink_end_session_delayed(void* context);
static void btif_a2dp_sink_shutdown_delayed(void* context);
static void btif_a2dp_sink_cleanup_delayed(void* context);
static void btif_a2dp_sink_command_ready(fixed_queue_t* queue, void* context);
static void btif_a2dp_sink_audio_handle_stop_decoding(void);
static void btif_decode_alarm_cb(void* context);
static void btif_a2dp_sink_audio_handle_start_decoding(void);
static void btif_a2dp_sink_avk_handle_timer(UNUSED_ATTR void* context);
static void btif_a2dp_sink_audio_rx_flush_req(void);
/* Handle incoming media packets A2DP SINK streaming */
static void btif_a2dp_sink_handle_inc_media(BT_HDR* p_msg);
static void btif_a2dp_sink_decoder_update_event(
    tBTIF_MEDIA_SINK_DECODER_UPDATE* p_buf);
static void btif_a2dp_sink_clear_track_event(void);
static void btif_a2dp_sink_set_focus_state_event(
    btif_a2dp_sink_focus_state_t state);
static void btif_a2dp_sink_audio_rx_flush_event(void);
static void btif_a2dp_sink_clear_track_event_req(void);

UNUSED_ATTR static const char* dump_media_event(uint16_t event) {
  switch (event) {
    CASE_RETURN_STR(BTIF_MEDIA_SINK_DECODER_UPDATE)
    CASE_RETURN_STR(BTIF_MEDIA_SINK_CLEAR_TRACK)
    CASE_RETURN_STR(BTIF_MEDIA_SINK_SET_FOCUS_STATE)
    CASE_RETURN_STR(BTIF_MEDIA_SINK_AUDIO_RX_FLUSH)
    default:
      break;
  }
  return "UNKNOWN A2DP SINK EVENT";
}

bool btif_a2dp_sink_init(void) {
  LOG_INFO(LOG_TAG, "%s", __func__);
  LockGuard lock(g_mutex);

  if (btif_a2dp_sink_state != BTIF_A2DP_SINK_STATE_OFF) {
    LOG_ERROR(LOG_TAG, "%s: A2DP Sink media task already running", __func__);
    return false;
  }

  memset(&btif_a2dp_sink_cb, 0, sizeof(btif_a2dp_sink_cb));
  btif_a2dp_sink_state = BTIF_A2DP_SINK_STATE_STARTING_UP;

  /* Start A2DP Sink media task */
  btif_a2dp_sink_cb.worker_thread = thread_new("btif_a2dp_sink_worker_thread");
  if (btif_a2dp_sink_cb.worker_thread == NULL) {
    LOG_ERROR(LOG_TAG, "%s: unable to start up media thread", __func__);
    btif_a2dp_sink_state = BTIF_A2DP_SINK_STATE_OFF;
    return false;
  }

  btif_a2dp_sink_cb.rx_focus_state = BTIF_A2DP_SINK_FOCUS_NOT_GRANTED;
  btif_a2dp_sink_cb.audio_track = NULL;
  btif_a2dp_sink_cb.rx_audio_queue = fixed_queue_new(SIZE_MAX);

  btif_a2dp_sink_cb.cmd_msg_queue = fixed_queue_new(SIZE_MAX);
  fixed_queue_register_dequeue(
      btif_a2dp_sink_cb.cmd_msg_queue,
      thread_get_reactor(btif_a2dp_sink_cb.worker_thread),
      btif_a2dp_sink_command_ready, NULL);

  /* Schedule the rest of the operations */
  thread_post(btif_a2dp_sink_cb.worker_thread, btif_a2dp_sink_init_delayed,
              NULL);

  return true;
}

static void btif_a2dp_sink_init_delayed(UNUSED_ATTR void* context) {
  LOG_INFO(LOG_TAG, "%s", __func__);
  raise_priority_a2dp(TASK_HIGH_MEDIA);
  btif_a2dp_sink_state = BTIF_A2DP_SINK_STATE_RUNNING;
}

bool btif_a2dp_sink_startup(void) {
  LOG_INFO(LOG_TAG, "%s", __func__);
  thread_post(btif_a2dp_sink_cb.worker_thread, btif_a2dp_sink_startup_delayed,
              NULL);
  return true;
}

static void btif_a2dp_sink_startup_delayed(UNUSED_ATTR void* context) {
  LOG_INFO(LOG_TAG, "%s", __func__);
  LockGuard lock(g_mutex);
  // Nothing to do
}

bool btif_a2dp_sink_start_session(const RawAddress& peer_address) {
  LOG_INFO(LOG_TAG, "%s: peer_address=%s", __func__,
           peer_address.ToString().c_str());
  thread_post(btif_a2dp_sink_cb.worker_thread,
              btif_a2dp_sink_start_session_delayed, NULL);
  return true;
}

static void btif_a2dp_sink_start_session_delayed(UNUSED_ATTR void* context) {
  LOG_INFO(LOG_TAG, "%s", __func__);
  LockGuard lock(g_mutex);
  // Nothing to do
}

bool btif_a2dp_sink_restart_session(const RawAddress& old_peer_address,
                                    const RawAddress& new_peer_address) {
  LOG_INFO(LOG_TAG, "%s: old_peer_address=%s new_peer_address=%s", __func__,
           old_peer_address.ToString().c_str(),
           new_peer_address.ToString().c_str());

  CHECK(!new_peer_address.IsEmpty());

  if (!old_peer_address.IsEmpty()) {
    btif_a2dp_sink_end_session(old_peer_address);
  }

  if (!bta_av_co_set_active_peer(new_peer_address)) {
    LOG_ERROR(LOG_TAG, "%s: Cannot stream audio: cannot set active peer to %s",
              __func__, new_peer_address.ToString().c_str());
    return false;
  }

  if (old_peer_address.IsEmpty()) {
    btif_a2dp_sink_startup();
  }
  btif_a2dp_sink_start_session(new_peer_address);

  return true;
}

bool btif_a2dp_sink_end_session(const RawAddress& peer_address) {
  LOG_INFO(LOG_TAG, "%s: peer_address=%s", __func__,
           peer_address.ToString().c_str());
  thread_post(btif_a2dp_sink_cb.worker_thread,
              btif_a2dp_sink_end_session_delayed, NULL);
  return true;
}

static void btif_a2dp_sink_end_session_delayed(UNUSED_ATTR void* context) {
  LOG_INFO(LOG_TAG, "%s", __func__);
  LockGuard lock(g_mutex);
  // Nothing to do
}

void btif_a2dp_sink_shutdown(void) {
  LOG_INFO(LOG_TAG, "%s", __func__);
  thread_post(btif_a2dp_sink_cb.worker_thread, btif_a2dp_sink_shutdown_delayed,
              NULL);
}

static void btif_a2dp_sink_shutdown_delayed(UNUSED_ATTR void* context) {
  LOG_INFO(LOG_TAG, "%s", __func__);
  LockGuard lock(g_mutex);
  // Nothing to do
}

void btif_a2dp_sink_cleanup(void) {
  LOG_INFO(LOG_TAG, "%s", __func__);

  alarm_t* decode_alarm;
  fixed_queue_t* cmd_msg_queue;
  thread_t* worker_thread;

  // Make sure the sink is shutdown
  btif_a2dp_sink_shutdown();

  {
    LockGuard lock(g_mutex);
    if ((btif_a2dp_sink_state == BTIF_A2DP_SINK_STATE_OFF) ||
        (btif_a2dp_sink_state == BTIF_A2DP_SINK_STATE_SHUTTING_DOWN)) {
      return;
    }
    // Make sure no channels are restarted while shutting down
    btif_a2dp_sink_state = BTIF_A2DP_SINK_STATE_SHUTTING_DOWN;

    decode_alarm = btif_a2dp_sink_cb.decode_alarm;
    btif_a2dp_sink_cb.decode_alarm = NULL;

    cmd_msg_queue = btif_a2dp_sink_cb.cmd_msg_queue;
    btif_a2dp_sink_cb.cmd_msg_queue = NULL;

    worker_thread = btif_a2dp_sink_cb.worker_thread;
    btif_a2dp_sink_cb.worker_thread = NULL;
  }

  // Stop the timer
  alarm_free(decode_alarm);

  // Exit the thread
  fixed_queue_free(cmd_msg_queue, NULL);
  thread_post(worker_thread, btif_a2dp_sink_cleanup_delayed, NULL);
  thread_free(worker_thread);
}

static void btif_a2dp_sink_cleanup_delayed(UNUSED_ATTR void* context) {
  LOG_INFO(LOG_TAG, "%s", __func__);
  LockGuard lock(g_mutex);

  fixed_queue_free(btif_a2dp_sink_cb.rx_audio_queue, NULL);
  btif_a2dp_sink_cb.rx_audio_queue = NULL;
  btif_a2dp_sink_state = BTIF_A2DP_SINK_STATE_OFF;
}

tA2DP_SAMPLE_RATE btif_a2dp_sink_get_sample_rate(void) {
  LockGuard lock(g_mutex);
  return btif_a2dp_sink_cb.sample_rate;
}

tA2DP_CHANNEL_COUNT btif_a2dp_sink_get_channel_count(void) {
  LockGuard lock(g_mutex);
  return btif_a2dp_sink_cb.channel_count;
}

static void btif_a2dp_sink_command_ready(fixed_queue_t* queue,
                                         UNUSED_ATTR void* context) {
  BT_HDR* p_msg = (BT_HDR*)fixed_queue_dequeue(queue);

  LOG_VERBOSE(LOG_TAG, "%s: event %d %s", __func__, p_msg->event,
              dump_media_event(p_msg->event));

  switch (p_msg->event) {
    case BTIF_MEDIA_SINK_DECODER_UPDATE:
      btif_a2dp_sink_decoder_update_event(
          (tBTIF_MEDIA_SINK_DECODER_UPDATE*)p_msg);
      break;
    case BTIF_MEDIA_SINK_CLEAR_TRACK:
      btif_a2dp_sink_clear_track_event();
      break;
    case BTIF_MEDIA_SINK_SET_FOCUS_STATE: {
      btif_a2dp_sink_focus_state_t state =
          ((tBTIF_MEDIA_SINK_FOCUS_UPDATE*)p_msg)->focus_state;
      btif_a2dp_sink_set_focus_state_event(state);
      break;
    }
    case BTIF_MEDIA_SINK_AUDIO_RX_FLUSH:
      btif_a2dp_sink_audio_rx_flush_event();
      break;
    default:
      LOG_ERROR(LOG_TAG, "%s: unknown event %d", __func__, p_msg->event);
      break;
  }

  osi_free(p_msg);
  LOG_VERBOSE(LOG_TAG, "%s: %s DONE", __func__, dump_media_event(p_msg->event));
}

void btif_a2dp_sink_update_decoder(const uint8_t* p_codec_info) {
  LOG_INFO(LOG_TAG, "%s", __func__);
  tBTIF_MEDIA_SINK_DECODER_UPDATE* p_buf =
      reinterpret_cast<tBTIF_MEDIA_SINK_DECODER_UPDATE*>(
          osi_malloc(sizeof(tBTIF_MEDIA_SINK_DECODER_UPDATE)));

  APPL_TRACE_EVENT("%s: p_codec_info[%x:%x:%x:%x:%x:%x]", __func__,
                   p_codec_info[1], p_codec_info[2], p_codec_info[3],
                   p_codec_info[4], p_codec_info[5], p_codec_info[6]);

  memcpy(p_buf->codec_info, p_codec_info, AVDT_CODEC_SIZE);
  p_buf->hdr.event = BTIF_MEDIA_SINK_DECODER_UPDATE;

  fixed_queue_enqueue(btif_a2dp_sink_cb.cmd_msg_queue, p_buf);
}

void btif_a2dp_sink_on_idle(void) {
  LOG_INFO(LOG_TAG, "%s", __func__);
  if (btif_a2dp_sink_state == BTIF_A2DP_SINK_STATE_OFF) return;
  btif_a2dp_sink_audio_handle_stop_decoding();
  btif_a2dp_sink_clear_track_event_req();
}

void btif_a2dp_sink_on_stopped(UNUSED_ATTR tBTA_AV_SUSPEND* p_av_suspend) {
  LOG_INFO(LOG_TAG, "%s", __func__);
  if (btif_a2dp_sink_state == BTIF_A2DP_SINK_STATE_OFF) return;
  btif_a2dp_sink_audio_handle_stop_decoding();
}

void btif_a2dp_sink_on_suspended(UNUSED_ATTR tBTA_AV_SUSPEND* p_av_suspend) {
  LOG_INFO(LOG_TAG, "%s", __func__);
  if (btif_a2dp_sink_state == BTIF_A2DP_SINK_STATE_OFF) return;
  btif_a2dp_sink_audio_handle_stop_decoding();
}

static void btif_a2dp_sink_audio_handle_stop_decoding(void) {
  LOG_INFO(LOG_TAG, "%s", __func__);
  alarm_t* old_alarm;
  {
    LockGuard lock(g_mutex);
    btif_a2dp_sink_cb.rx_flush = true;
    btif_a2dp_sink_audio_rx_flush_req();
    old_alarm = btif_a2dp_sink_cb.decode_alarm;
    btif_a2dp_sink_cb.decode_alarm = NULL;
  }

  // Drop the lock here, btif_decode_alarm_cb may in the process of being called
  // while we alarm free leading to deadlock.
  //
  // alarm_free waits for btif_decode_alarm_cb which is waiting for g_mutex.
  alarm_free(old_alarm);

  {
    LockGuard lock(g_mutex);
#ifndef OS_GENERIC
    BtifAvrcpAudioTrackPause(btif_a2dp_sink_cb.audio_track);
#endif
  }
}

static void btif_decode_alarm_cb(UNUSED_ATTR void* context) {
  LockGuard lock(g_mutex);
  if (btif_a2dp_sink_cb.worker_thread != NULL) {
    thread_post(btif_a2dp_sink_cb.worker_thread,
                btif_a2dp_sink_avk_handle_timer, NULL);
  }
}

static void btif_a2dp_sink_clear_track_event(void) {
  LOG_INFO(LOG_TAG, "%s", __func__);
  LockGuard lock(g_mutex);

#ifndef OS_GENERIC
  BtifAvrcpAudioTrackStop(btif_a2dp_sink_cb.audio_track);
  BtifAvrcpAudioTrackDelete(btif_a2dp_sink_cb.audio_track);
#endif
  btif_a2dp_sink_cb.audio_track = NULL;
}

// Must be called while locked.
static void btif_a2dp_sink_audio_handle_start_decoding(void) {
  LOG_INFO(LOG_TAG, "%s", __func__);
  if (btif_a2dp_sink_cb.decode_alarm != NULL)
    return;  // Already started decoding

#ifndef OS_GENERIC
  BtifAvrcpAudioTrackStart(btif_a2dp_sink_cb.audio_track);
#endif

  btif_a2dp_sink_cb.decode_alarm = alarm_new_periodic("btif.a2dp_sink_decode");
  if (btif_a2dp_sink_cb.decode_alarm == NULL) {
    LOG_ERROR(LOG_TAG, "%s: unable to allocate decode alarm", __func__);
    return;
  }
  alarm_set(btif_a2dp_sink_cb.decode_alarm, BTIF_SINK_MEDIA_TIME_TICK_MS,
            btif_decode_alarm_cb, NULL);
}

static void btif_a2dp_sink_on_decode_complete(uint8_t* data, uint32_t len) {
#ifndef OS_GENERIC
  BtifAvrcpAudioTrackWriteData(btif_a2dp_sink_cb.audio_track,
                               reinterpret_cast<void*>(data), len);
#endif
}

// Must be called while locked.
static void btif_a2dp_sink_handle_inc_media(BT_HDR* p_msg) {
  if ((btif_av_get_peer_sep() == AVDT_TSEP_SNK) ||
      (btif_a2dp_sink_cb.rx_flush)) {
    APPL_TRACE_DEBUG("%s: state changed happened in this tick", __func__);
    return;
  }

  CHECK(btif_a2dp_sink_cb.decoder_interface);
  if (!btif_a2dp_sink_cb.decoder_interface->decode_packet(p_msg)) {
    LOG_ERROR(LOG_TAG, "%s: decoding failed", __func__);
  }
}

static void btif_a2dp_sink_avk_handle_timer(UNUSED_ATTR void* context) {
  LockGuard lock(g_mutex);

  BT_HDR* p_msg;
  if (fixed_queue_is_empty(btif_a2dp_sink_cb.rx_audio_queue)) {
    APPL_TRACE_DEBUG("%s: empty queue", __func__);
    return;
  }

  /* Don't do anything in case of focus not granted */
  if (btif_a2dp_sink_cb.rx_focus_state == BTIF_A2DP_SINK_FOCUS_NOT_GRANTED) {
    APPL_TRACE_DEBUG("%s: skipping frames since focus is not present",
                     __func__);
    return;
  }
  /* Play only in BTIF_A2DP_SINK_FOCUS_GRANTED case */
  if (btif_a2dp_sink_cb.rx_flush) {
    fixed_queue_flush(btif_a2dp_sink_cb.rx_audio_queue, osi_free);
    return;
  }

  APPL_TRACE_DEBUG("%s: process frames begin", __func__);
  while (true) {
    p_msg = (BT_HDR*)fixed_queue_try_dequeue(btif_a2dp_sink_cb.rx_audio_queue);
    if (p_msg == NULL) {
      break;
    }
    APPL_TRACE_DEBUG("%s: number of packets in queue %zu", __func__,
                     fixed_queue_length(btif_a2dp_sink_cb.rx_audio_queue));

    /* Queue packet has less frames */
    btif_a2dp_sink_handle_inc_media(p_msg);
    osi_free(p_msg);
  }
  APPL_TRACE_DEBUG("%s: process frames end", __func__);
}

/* when true media task discards any rx frames */
void btif_a2dp_sink_set_rx_flush(bool enable) {
  LOG_INFO(LOG_TAG, "%s: enable=%s", __func__, (enable) ? "true" : "false");
  LockGuard lock(g_mutex);

  btif_a2dp_sink_cb.rx_flush = enable;
}

static void btif_a2dp_sink_audio_rx_flush_event(void) {
  LOG_INFO(LOG_TAG, "%s", __func__);
  LockGuard lock(g_mutex);
  // Flush all received encoded audio buffers
  fixed_queue_flush(btif_a2dp_sink_cb.rx_audio_queue, osi_free);
}

static void btif_a2dp_sink_decoder_update_event(
    tBTIF_MEDIA_SINK_DECODER_UPDATE* p_buf) {
  LOG_INFO(LOG_TAG, "%s", __func__);
  LockGuard lock(g_mutex);
  APPL_TRACE_DEBUG("%s: p_codec_info[%x:%x:%x:%x:%x:%x]", __func__,
                   p_buf->codec_info[1], p_buf->codec_info[2],
                   p_buf->codec_info[3], p_buf->codec_info[4],
                   p_buf->codec_info[5], p_buf->codec_info[6]);

  int sample_rate = A2DP_GetTrackSampleRate(p_buf->codec_info);
  if (sample_rate == -1) {
    LOG_ERROR(LOG_TAG, "%s: cannot get the track frequency", __func__);
    return;
  }
  int channel_count = A2DP_GetTrackChannelCount(p_buf->codec_info);
  if (channel_count == -1) {
    LOG_ERROR(LOG_TAG, "%s: cannot get the channel count", __func__);
    return;
  }
  int channel_type = A2DP_GetSinkTrackChannelType(p_buf->codec_info);
  if (channel_type == -1) {
    LOG_ERROR(LOG_TAG, "%s: cannot get the Sink channel type", __func__);
    return;
  }
  btif_a2dp_sink_cb.sample_rate = sample_rate;
  btif_a2dp_sink_cb.channel_count = channel_count;

  btif_a2dp_sink_cb.rx_flush = false;
  APPL_TRACE_DEBUG("%s: reset to Sink role", __func__);

  btif_a2dp_sink_cb.decoder_interface = bta_av_co_get_decoder_interface();
  if (btif_a2dp_sink_cb.decoder_interface == NULL) {
    LOG_ERROR(LOG_TAG, "%s: cannot stream audio: no source decoder interface",
              __func__);
    return;
  }

  if (!btif_a2dp_sink_cb.decoder_interface->decoder_init(
          btif_a2dp_sink_on_decode_complete)) {
    LOG_ERROR(LOG_TAG, "%s: failed to initialize decoder", __func__);
    return;
  }

  APPL_TRACE_DEBUG("%s: create audio track", __func__);
  btif_a2dp_sink_cb.audio_track =
#ifndef OS_GENERIC
      BtifAvrcpAudioTrackCreate(sample_rate, channel_type);
#else
      NULL;
#endif
  if (btif_a2dp_sink_cb.audio_track == NULL) {
    LOG_ERROR(LOG_TAG, "%s: track creation failed", __func__);
    return;
  }
}

uint8_t btif_a2dp_sink_enqueue_buf(BT_HDR* p_pkt) {
  LockGuard lock(g_mutex);
  if (btif_a2dp_sink_cb.rx_flush) /* Flush enabled, do not enqueue */
    return fixed_queue_length(btif_a2dp_sink_cb.rx_audio_queue);

  if (fixed_queue_length(btif_a2dp_sink_cb.rx_audio_queue) ==
      MAX_INPUT_A2DP_FRAME_QUEUE_SZ) {
    uint8_t ret = fixed_queue_length(btif_a2dp_sink_cb.rx_audio_queue);
    osi_free(fixed_queue_try_dequeue(btif_a2dp_sink_cb.rx_audio_queue));
    return ret;
  }

  BTIF_TRACE_VERBOSE("%s +", __func__);
  /* Allocate and queue this buffer */
  BT_HDR* p_msg =
      reinterpret_cast<BT_HDR*>(osi_malloc(sizeof(*p_msg) + p_pkt->len));
  memcpy(p_msg, p_pkt, sizeof(*p_msg));
  p_msg->offset = 0;
  memcpy(p_msg->data, p_pkt->data + p_pkt->offset, p_pkt->len);
  fixed_queue_enqueue(btif_a2dp_sink_cb.rx_audio_queue, p_msg);
  if (fixed_queue_length(btif_a2dp_sink_cb.rx_audio_queue) ==
      MAX_A2DP_DELAYED_START_FRAME_COUNT) {
    BTIF_TRACE_DEBUG("%s: Initiate decoding", __func__);
    btif_a2dp_sink_audio_handle_start_decoding();
  }

  return fixed_queue_length(btif_a2dp_sink_cb.rx_audio_queue);
}

void btif_a2dp_sink_audio_rx_flush_req(void) {
  LOG_INFO(LOG_TAG, "%s", __func__);
  if (fixed_queue_is_empty(btif_a2dp_sink_cb.rx_audio_queue)) {
    /* Queue is already empty */
    return;
  }

  BT_HDR* p_buf = reinterpret_cast<BT_HDR*>(osi_malloc(sizeof(BT_HDR)));
  p_buf->event = BTIF_MEDIA_SINK_AUDIO_RX_FLUSH;
  fixed_queue_enqueue(btif_a2dp_sink_cb.cmd_msg_queue, p_buf);
}

void btif_a2dp_sink_debug_dump(UNUSED_ATTR int fd) {
  // Nothing to do
}

void btif_a2dp_sink_set_focus_state_req(btif_a2dp_sink_focus_state_t state) {
  LOG_INFO(LOG_TAG, "%s", __func__);
  tBTIF_MEDIA_SINK_FOCUS_UPDATE* p_buf =
      reinterpret_cast<tBTIF_MEDIA_SINK_FOCUS_UPDATE*>(
          osi_malloc(sizeof(tBTIF_MEDIA_SINK_FOCUS_UPDATE)));
  p_buf->focus_state = state;
  p_buf->hdr.event = BTIF_MEDIA_SINK_SET_FOCUS_STATE;
  fixed_queue_enqueue(btif_a2dp_sink_cb.cmd_msg_queue, p_buf);
}

static void btif_a2dp_sink_set_focus_state_event(
    btif_a2dp_sink_focus_state_t state) {
  LOG_INFO(LOG_TAG, "%s: state=%d", __func__, state);
  LockGuard lock(g_mutex);

  if (!btif_av_is_connected()) return;
  APPL_TRACE_DEBUG("%s: setting focus state to %d", __func__, state);
  btif_a2dp_sink_cb.rx_focus_state = state;
  if (btif_a2dp_sink_cb.rx_focus_state == BTIF_A2DP_SINK_FOCUS_NOT_GRANTED) {
    fixed_queue_flush(btif_a2dp_sink_cb.rx_audio_queue, osi_free);
    btif_a2dp_sink_cb.rx_flush = true;
  } else if (btif_a2dp_sink_cb.rx_focus_state == BTIF_A2DP_SINK_FOCUS_GRANTED) {
    btif_a2dp_sink_cb.rx_flush = false;
  }
}

void btif_a2dp_sink_set_audio_track_gain(float gain) {
  LOG_INFO(LOG_TAG, "%s: set gain to %f", __func__, gain);
  LockGuard lock(g_mutex);

#ifndef OS_GENERIC
  BtifAvrcpSetAudioTrackGain(btif_a2dp_sink_cb.audio_track, gain);
#endif
}

static void btif_a2dp_sink_clear_track_event_req(void) {
  LOG_INFO(LOG_TAG, "%s", __func__);
  BT_HDR* p_buf = reinterpret_cast<BT_HDR*>(osi_malloc(sizeof(BT_HDR)));

  p_buf->event = BTIF_MEDIA_SINK_CLEAR_TRACK;
  fixed_queue_enqueue(btif_a2dp_sink_cb.cmd_msg_queue, p_buf);
}
