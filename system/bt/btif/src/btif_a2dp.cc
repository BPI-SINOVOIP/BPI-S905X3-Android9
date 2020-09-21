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

#define LOG_TAG "bt_btif_a2dp"

#include <stdbool.h>

#include "audio_a2dp_hw/include/audio_a2dp_hw.h"
#include "bt_common.h"
#include "bta_av_api.h"
#include "btif_a2dp.h"
#include "btif_a2dp_audio_interface.h"
#include "btif_a2dp_control.h"
#include "btif_a2dp_sink.h"
#include "btif_a2dp_source.h"
#include "btif_av.h"
#include "btif_av_co.h"
#include "btif_hf.h"
#include "btif_util.h"
#include "osi/include/log.h"

void btif_a2dp_on_idle(void) {
  LOG_INFO(LOG_TAG, "%s: ## ON A2DP IDLE ## peer_sep = %d", __func__,
           btif_av_get_peer_sep());
  if (btif_av_get_peer_sep() == AVDT_TSEP_SNK) {
    btif_a2dp_source_on_idle();
  } else if (btif_av_get_peer_sep() == AVDT_TSEP_SRC) {
    btif_a2dp_sink_on_idle();
  }
}

bool btif_a2dp_on_started(const RawAddress& peer_addr,
                          tBTA_AV_START* p_av_start, bool pending_start) {
  bool ack = false;

  LOG_INFO(LOG_TAG,
           "%s: ## ON A2DP STARTED ## peer %s pending_start:%s p_av_start:%p",
           __func__, peer_addr.ToString().c_str(),
           logbool(pending_start).c_str(), p_av_start);

  if (p_av_start == NULL) {
    /* ack back a local start request */

    if (!btif_av_is_a2dp_offload_enabled()) {
      btif_a2dp_command_ack(A2DP_CTRL_ACK_SUCCESS);
      return true;
    } else if (bluetooth::headset::IsCallIdle()) {
      btif_av_stream_start_offload();
    } else {
      LOG_ERROR(LOG_TAG, "%s: peer %s call in progress, do not start offload",
                __func__, peer_addr.ToString().c_str());
      btif_a2dp_audio_on_started(A2DP_CTRL_ACK_INCALL_FAILURE);
    }
    return true;
  }

  LOG_INFO(LOG_TAG,
           "%s: peer %s pending_start:%s status:%d suspending:%s initiator:%s",
           __func__, peer_addr.ToString().c_str(),
           logbool(pending_start).c_str(), p_av_start->status,
           logbool(p_av_start->suspending).c_str(),
           logbool(p_av_start->initiator).c_str());

  if (p_av_start->status == BTA_AV_SUCCESS) {
    if (!p_av_start->suspending) {
      if (p_av_start->initiator) {
        if (pending_start) {
          if (btif_av_is_a2dp_offload_enabled()) {
            btif_av_stream_start_offload();
          } else {
            btif_a2dp_command_ack(A2DP_CTRL_ACK_SUCCESS);
          }
          ack = true;
        }
      }

      /* media task is autostarted upon a2dp audiopath connection */
    }
  } else if (pending_start) {
    LOG_ERROR(LOG_TAG, "%s: peer %s A2DP start request failed: status = %d",
              __func__, peer_addr.ToString().c_str(), p_av_start->status);
    btif_a2dp_command_ack(A2DP_CTRL_ACK_FAILURE);
    ack = true;
  }
  return ack;
}

void btif_a2dp_on_stopped(tBTA_AV_SUSPEND* p_av_suspend) {
  LOG_INFO(LOG_TAG, "%s: ## ON A2DP STOPPED ## p_av_suspend=%p", __func__,
           p_av_suspend);

  if (btif_av_get_peer_sep() == AVDT_TSEP_SRC) {
    btif_a2dp_sink_on_stopped(p_av_suspend);
    return;
  }
  if (!btif_av_is_a2dp_offload_enabled()) {
    btif_a2dp_source_on_stopped(p_av_suspend);
  } else if (p_av_suspend != NULL) {
    btif_a2dp_audio_on_stopped(p_av_suspend->status);
  }
}

void btif_a2dp_on_suspended(tBTA_AV_SUSPEND* p_av_suspend) {
  LOG_INFO(LOG_TAG, "%s: ## ON A2DP SUSPENDED ## p_av_suspend=%p", __func__,
           p_av_suspend);
  if (!btif_av_is_a2dp_offload_enabled()) {
    if (btif_av_get_peer_sep() == AVDT_TSEP_SRC) {
      btif_a2dp_sink_on_suspended(p_av_suspend);
    } else {
      btif_a2dp_source_on_suspended(p_av_suspend);
    }
  } else {
    btif_a2dp_audio_on_suspended(p_av_suspend->status);
  }
}

void btif_a2dp_on_offload_started(const RawAddress& peer_addr,
                                  tBTA_AV_STATUS status) {
  tA2DP_CTRL_ACK ack;
  LOG_INFO(LOG_TAG, "%s: peer %s status %d", __func__,
           peer_addr.ToString().c_str(), status);

  switch (status) {
    case BTA_AV_SUCCESS:
      ack = A2DP_CTRL_ACK_SUCCESS;
      break;
    case BTA_AV_FAIL_RESOURCES:
      LOG_ERROR(LOG_TAG, "%s: peer %s FAILED UNSUPPORTED", __func__,
                peer_addr.ToString().c_str());
      ack = A2DP_CTRL_ACK_UNSUPPORTED;
      break;
    default:
      LOG_ERROR(LOG_TAG, "%s: peer %s FAILED: status = %d", __func__,
                peer_addr.ToString().c_str(), status);
      ack = A2DP_CTRL_ACK_FAILURE;
      break;
  }
  if (btif_av_is_a2dp_offload_enabled()) {
    btif_a2dp_audio_on_started(status);
    if (ack != BTA_AV_SUCCESS && btif_av_stream_started_ready()) {
      // Offload request will return with failure from btif_av sm if
      // suspend is triggered for remote start. Disconnect only if SoC
      // returned failure for offload VSC
      LOG_ERROR(LOG_TAG, "%s: peer %s offload start failed", __func__,
                peer_addr.ToString().c_str());
      btif_av_src_disconnect_sink(peer_addr);
    }
  } else {
    btif_a2dp_command_ack(ack);
  }
}

void btif_debug_a2dp_dump(int fd) {
  btif_a2dp_source_debug_dump(fd);
  btif_a2dp_sink_debug_dump(fd);
  btif_a2dp_codec_debug_dump(fd);
}
