/*
 * Copyright 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "a2dp_vendor_aptx_hd_encoder"

#include "a2dp_vendor_aptx_hd_encoder.h"

#include <dlfcn.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "a2dp_vendor.h"
#include "a2dp_vendor_aptx_hd.h"
#include "bt_common.h"
#include "osi/include/log.h"
#include "osi/include/osi.h"

//
// Encoder for aptX-HD Source Codec
//

//
// The aptX-HD encoder shared library, and the functions to use
//
static const char* APTX_HD_ENCODER_LIB_NAME = "libaptXHD_encoder.so";
static void* aptx_hd_encoder_lib_handle = NULL;

static const char* APTX_HD_ENCODER_INIT_NAME = "aptxhdbtenc_init";
typedef int (*tAPTX_HD_ENCODER_INIT)(void* state, short endian);

static const char* APTX_HD_ENCODER_ENCODE_STEREO_NAME =
    "aptxhdbtenc_encodestereo";
typedef int (*tAPTX_HD_ENCODER_ENCODE_STEREO)(void* state, void* pcmL,
                                              void* pcmR, void* buffer);

static const char* APTX_HD_ENCODER_SIZEOF_PARAMS_NAME = "SizeofAptxhdbtenc";
typedef int (*tAPTX_HD_ENCODER_SIZEOF_PARAMS)(void);

static tAPTX_HD_ENCODER_INIT aptx_hd_encoder_init_func;
static tAPTX_HD_ENCODER_ENCODE_STEREO aptx_hd_encoder_encode_stereo_func;
static tAPTX_HD_ENCODER_SIZEOF_PARAMS aptx_hd_encoder_sizeof_params_func;

// offset
#if (BTA_AV_CO_CP_SCMS_T == TRUE)
#define A2DP_APTX_HD_OFFSET (AVDT_MEDIA_OFFSET + 1)
#else
#define A2DP_APTX_HD_OFFSET AVDT_MEDIA_OFFSET
#endif

#define A2DP_APTX_HD_MAX_PCM_BYTES_PER_READ 4096

typedef struct {
  uint64_t sleep_time_ns;
  uint32_t pcm_reads;
  uint32_t pcm_bytes_per_read;
  uint32_t aptx_hd_bytes;
  uint32_t frame_size_counter;
} tAPTX_HD_FRAMING_PARAMS;

typedef struct {
  uint64_t session_start_us;

  size_t media_read_total_expected_packets;
  size_t media_read_total_expected_reads_count;
  size_t media_read_total_expected_read_bytes;

  size_t media_read_total_dropped_packets;
  size_t media_read_total_actual_reads_count;
  size_t media_read_total_actual_read_bytes;
} a2dp_aptx_hd_encoder_stats_t;

typedef struct {
  a2dp_source_read_callback_t read_callback;
  a2dp_source_enqueue_callback_t enqueue_callback;

  bool use_SCMS_T;
  bool is_peer_edr;          // True if the peer device supports EDR
  bool peer_supports_3mbps;  // True if the peer device supports 3Mbps EDR
  uint16_t peer_mtu;         // MTU of the A2DP peer
  uint32_t timestamp;        // Timestamp for the A2DP frames

  tA2DP_FEEDING_PARAMS feeding_params;
  tAPTX_HD_FRAMING_PARAMS framing_params;
  void* aptx_hd_encoder_state;
  a2dp_aptx_hd_encoder_stats_t stats;
} tA2DP_APTX_HD_ENCODER_CB;

static tA2DP_APTX_HD_ENCODER_CB a2dp_aptx_hd_encoder_cb;

static void a2dp_vendor_aptx_hd_encoder_update(
    uint16_t peer_mtu, A2dpCodecConfig* a2dp_codec_config,
    bool* p_restart_input, bool* p_restart_output, bool* p_config_updated);
static void aptx_hd_init_framing_params(
    tAPTX_HD_FRAMING_PARAMS* framing_params);
static void aptx_hd_update_framing_params(
    tAPTX_HD_FRAMING_PARAMS* framing_params);
static size_t aptx_hd_encode_24bit(tAPTX_HD_FRAMING_PARAMS* framing_params,
                                   size_t* data_out_index, uint32_t* data32_in,
                                   uint8_t* data_out);

bool A2DP_VendorLoadEncoderAptxHd(void) {
  if (aptx_hd_encoder_lib_handle != NULL) return true;  // Already loaded

  // Open the encoder library
  aptx_hd_encoder_lib_handle = dlopen(APTX_HD_ENCODER_LIB_NAME, RTLD_NOW);
  if (aptx_hd_encoder_lib_handle == NULL) {
    LOG_ERROR(LOG_TAG, "%s: cannot open aptX-HD encoder library %s: %s",
              __func__, APTX_HD_ENCODER_LIB_NAME, dlerror());
    return false;
  }

  aptx_hd_encoder_init_func = (tAPTX_HD_ENCODER_INIT)dlsym(
      aptx_hd_encoder_lib_handle, APTX_HD_ENCODER_INIT_NAME);
  if (aptx_hd_encoder_init_func == NULL) {
    LOG_ERROR(LOG_TAG,
              "%s: cannot find function '%s' in the encoder library: %s",
              __func__, APTX_HD_ENCODER_INIT_NAME, dlerror());
    A2DP_VendorUnloadEncoderAptxHd();
    return false;
  }

  aptx_hd_encoder_encode_stereo_func = (tAPTX_HD_ENCODER_ENCODE_STEREO)dlsym(
      aptx_hd_encoder_lib_handle, APTX_HD_ENCODER_ENCODE_STEREO_NAME);
  if (aptx_hd_encoder_encode_stereo_func == NULL) {
    LOG_ERROR(LOG_TAG,
              "%s: cannot find function '%s' in the encoder library: %s",
              __func__, APTX_HD_ENCODER_ENCODE_STEREO_NAME, dlerror());
    A2DP_VendorUnloadEncoderAptxHd();
    return false;
  }

  aptx_hd_encoder_sizeof_params_func = (tAPTX_HD_ENCODER_SIZEOF_PARAMS)dlsym(
      aptx_hd_encoder_lib_handle, APTX_HD_ENCODER_SIZEOF_PARAMS_NAME);
  if (aptx_hd_encoder_sizeof_params_func == NULL) {
    LOG_ERROR(LOG_TAG,
              "%s: cannot find function '%s' in the encoder library: %s",
              __func__, APTX_HD_ENCODER_SIZEOF_PARAMS_NAME, dlerror());
    A2DP_VendorUnloadEncoderAptxHd();
    return false;
  }

  return true;
}

void A2DP_VendorUnloadEncoderAptxHd(void) {
  aptx_hd_encoder_init_func = NULL;
  aptx_hd_encoder_encode_stereo_func = NULL;
  aptx_hd_encoder_sizeof_params_func = NULL;

  if (aptx_hd_encoder_lib_handle != NULL) {
    dlclose(aptx_hd_encoder_lib_handle);
    aptx_hd_encoder_lib_handle = NULL;
  }
}

void a2dp_vendor_aptx_hd_encoder_init(
    const tA2DP_ENCODER_INIT_PEER_PARAMS* p_peer_params,
    A2dpCodecConfig* a2dp_codec_config,
    a2dp_source_read_callback_t read_callback,
    a2dp_source_enqueue_callback_t enqueue_callback) {
  memset(&a2dp_aptx_hd_encoder_cb, 0, sizeof(a2dp_aptx_hd_encoder_cb));

  a2dp_aptx_hd_encoder_cb.stats.session_start_us = time_get_os_boottime_us();

  a2dp_aptx_hd_encoder_cb.read_callback = read_callback;
  a2dp_aptx_hd_encoder_cb.enqueue_callback = enqueue_callback;
  a2dp_aptx_hd_encoder_cb.is_peer_edr = p_peer_params->is_peer_edr;
  a2dp_aptx_hd_encoder_cb.peer_supports_3mbps =
      p_peer_params->peer_supports_3mbps;
  a2dp_aptx_hd_encoder_cb.peer_mtu = p_peer_params->peer_mtu;
  a2dp_aptx_hd_encoder_cb.timestamp = 0;

  /* aptX-HD encoder config */
  a2dp_aptx_hd_encoder_cb.use_SCMS_T = false;  // TODO: should be a parameter
#if (BTA_AV_CO_CP_SCMS_T == TRUE)
  a2dp_aptx_hd_encoder_cb.use_SCMS_T = true;
#endif

  a2dp_aptx_hd_encoder_cb.aptx_hd_encoder_state =
      osi_malloc(aptx_hd_encoder_sizeof_params_func());
  if (a2dp_aptx_hd_encoder_cb.aptx_hd_encoder_state != NULL) {
    aptx_hd_encoder_init_func(a2dp_aptx_hd_encoder_cb.aptx_hd_encoder_state, 0);
  } else {
    LOG_ERROR(LOG_TAG, "%s: Cannot allocate aptX-HD encoder state", __func__);
    // TODO: Return an error?
  }

  // NOTE: Ignore the restart_input / restart_output flags - this initization
  // happens when the connection is (re)started.
  bool restart_input = false;
  bool restart_output = false;
  bool config_updated = false;
  a2dp_vendor_aptx_hd_encoder_update(a2dp_aptx_hd_encoder_cb.peer_mtu,
                                     a2dp_codec_config, &restart_input,
                                     &restart_output, &config_updated);
}

bool A2dpCodecConfigAptxHd::updateEncoderUserConfig(
    const tA2DP_ENCODER_INIT_PEER_PARAMS* p_peer_params, bool* p_restart_input,
    bool* p_restart_output, bool* p_config_updated) {
  a2dp_aptx_hd_encoder_cb.is_peer_edr = p_peer_params->is_peer_edr;
  a2dp_aptx_hd_encoder_cb.peer_supports_3mbps =
      p_peer_params->peer_supports_3mbps;
  a2dp_aptx_hd_encoder_cb.peer_mtu = p_peer_params->peer_mtu;
  a2dp_aptx_hd_encoder_cb.timestamp = 0;

  if (a2dp_aptx_hd_encoder_cb.peer_mtu == 0) {
    LOG_ERROR(LOG_TAG,
              "%s: Cannot update the codec encoder for %s: "
              "invalid peer MTU",
              __func__, name().c_str());
    return false;
  }

  a2dp_vendor_aptx_hd_encoder_update(a2dp_aptx_hd_encoder_cb.peer_mtu, this,
                                     p_restart_input, p_restart_output,
                                     p_config_updated);
  return true;
}

// Update the A2DP aptX-HD encoder.
// |peer_mtu| is the peer MTU.
// |a2dp_codec_config| is the A2DP codec to use for the update.
static void a2dp_vendor_aptx_hd_encoder_update(
    uint16_t peer_mtu, A2dpCodecConfig* a2dp_codec_config,
    bool* p_restart_input, bool* p_restart_output, bool* p_config_updated) {
  uint8_t codec_info[AVDT_CODEC_SIZE];

  *p_restart_input = false;
  *p_restart_output = false;
  *p_config_updated = false;
  if (!a2dp_codec_config->copyOutOtaCodecConfig(codec_info)) {
    LOG_ERROR(LOG_TAG,
              "%s: Cannot update the codec encoder for %s: "
              "invalid codec config",
              __func__, a2dp_codec_config->name().c_str());
    return;
  }
  const uint8_t* p_codec_info = codec_info;

  // The feeding parameters
  tA2DP_FEEDING_PARAMS* p_feeding_params =
      &a2dp_aptx_hd_encoder_cb.feeding_params;
  p_feeding_params->sample_rate =
      A2DP_VendorGetTrackSampleRateAptxHd(p_codec_info);
  p_feeding_params->bits_per_sample =
      a2dp_codec_config->getAudioBitsPerSample();
  p_feeding_params->channel_count =
      A2DP_VendorGetTrackChannelCountAptxHd(p_codec_info);
  LOG_DEBUG(LOG_TAG, "%s: sample_rate=%u bits_per_sample=%u channel_count=%u",
            __func__, p_feeding_params->sample_rate,
            p_feeding_params->bits_per_sample, p_feeding_params->channel_count);
  a2dp_vendor_aptx_hd_feeding_reset();
}

void a2dp_vendor_aptx_hd_encoder_cleanup(void) {
  osi_free(a2dp_aptx_hd_encoder_cb.aptx_hd_encoder_state);
  memset(&a2dp_aptx_hd_encoder_cb, 0, sizeof(a2dp_aptx_hd_encoder_cb));
}

//
// Initialize the framing parameters, and set those that don't change
// while streaming (e.g., 'sleep_time_ns').
//
static void aptx_hd_init_framing_params(
    tAPTX_HD_FRAMING_PARAMS* framing_params) {
  framing_params->sleep_time_ns = 0;
  framing_params->pcm_reads = 0;
  framing_params->pcm_bytes_per_read = 0;
  framing_params->aptx_hd_bytes = 0;
  framing_params->frame_size_counter = 0;

  framing_params->sleep_time_ns = 9000000;

  LOG_DEBUG(LOG_TAG, "%s: sleep_time_ns = %" PRIu64, __func__,
            framing_params->sleep_time_ns);
}

//
// Set frame size and transmission interval needed to stream the required
// sample rate using 2-DH5 packets for aptX and 2-DH3 packets for aptX-LL.
// With SCMS-T enabled we need to reserve room for extra headers added later.
// Packets are always sent at equals time intervals but to achieve the
// required sample rate, the frame size needs to change on occasion.
//
// Also need to specify how many of the required PCM samples are read at a
// time:
//     aptx_bytes = pcm_reads * pcm_bytes_per_read / 4
// and
//     number of aptX samples produced = pcm_bytes_per_read / 16
//
static void aptx_hd_update_framing_params(
    tAPTX_HD_FRAMING_PARAMS* framing_params) {
  if (a2dp_aptx_hd_encoder_cb.feeding_params.sample_rate == 48000) {
    framing_params->aptx_hd_bytes = 648;
    framing_params->pcm_bytes_per_read = 24;
    framing_params->pcm_reads = 108;
  } else {
    // Assume the sample rate is 44100

    //
    // Total of 80 iterations:
    // - Iteration 80: packet size 648, with 108 reads of 24 PCM bytes
    // - Iterations 20, 40, 60: packet size 612, with 102 reads of 24 PCM bytes
    // - All other iterations: packet size 594, with 99 reads of 24 PCM bytes
    //
    if (framing_params->frame_size_counter + 1 == 80) {
      framing_params->aptx_hd_bytes = 648;
      framing_params->pcm_bytes_per_read = 24;
      framing_params->pcm_reads = 108;
    } else if (((framing_params->frame_size_counter + 1) % 20) == 0) {
      framing_params->aptx_hd_bytes = 612;
      framing_params->pcm_bytes_per_read = 24;
      framing_params->pcm_reads = 102;
    } else {
      framing_params->aptx_hd_bytes = 594;
      framing_params->pcm_bytes_per_read = 24;
      framing_params->pcm_reads = 99;
    }
    framing_params->frame_size_counter++;
    if (framing_params->frame_size_counter == 80)
      framing_params->frame_size_counter = 0;
  }

  LOG_VERBOSE(LOG_TAG,
              "%s: sleep_time_ns = %" PRIu64
              " aptx_hd_bytes = %u "
              "pcm_bytes_per_read = %u pcm_reads = %u frame_size_counter = %u",
              __func__, framing_params->sleep_time_ns,
              framing_params->aptx_hd_bytes, framing_params->pcm_bytes_per_read,
              framing_params->pcm_reads, framing_params->frame_size_counter);
}

void a2dp_vendor_aptx_hd_feeding_reset(void) {
  aptx_hd_init_framing_params(&a2dp_aptx_hd_encoder_cb.framing_params);
}

void a2dp_vendor_aptx_hd_feeding_flush(void) {
  aptx_hd_init_framing_params(&a2dp_aptx_hd_encoder_cb.framing_params);
}

period_ms_t a2dp_vendor_aptx_hd_get_encoder_interval_ms(void) {
  return a2dp_aptx_hd_encoder_cb.framing_params.sleep_time_ns / (1000 * 1000);
}

void a2dp_vendor_aptx_hd_send_frames(uint64_t timestamp_us) {
  tAPTX_HD_FRAMING_PARAMS* framing_params =
      &a2dp_aptx_hd_encoder_cb.framing_params;

  // Prepare the packet to send
  BT_HDR* p_buf = (BT_HDR*)osi_malloc(BT_DEFAULT_BUFFER_SIZE);
  p_buf->offset = A2DP_APTX_HD_OFFSET;
  p_buf->len = 0;
  p_buf->layer_specific = 0;

  uint8_t* encoded_ptr = (uint8_t*)(p_buf + 1);
  encoded_ptr += p_buf->offset;

  aptx_hd_update_framing_params(framing_params);

  //
  // Read the PCM data and encode it
  //
  uint32_t
      read_buffer32[A2DP_APTX_HD_MAX_PCM_BYTES_PER_READ / sizeof(uint32_t)];
  uint32_t expected_read_bytes =
      framing_params->pcm_reads * framing_params->pcm_bytes_per_read;
  size_t encoded_ptr_index = 0;
  size_t pcm_bytes_encoded = 0;
  uint32_t bytes_read = 0;

  a2dp_aptx_hd_encoder_cb.stats.media_read_total_expected_packets++;
  a2dp_aptx_hd_encoder_cb.stats.media_read_total_expected_reads_count++;
  a2dp_aptx_hd_encoder_cb.stats.media_read_total_expected_read_bytes +=
      expected_read_bytes;

  LOG_VERBOSE(LOG_TAG, "%s: PCM read of size %u", __func__,
              expected_read_bytes);
  bytes_read = a2dp_aptx_hd_encoder_cb.read_callback((uint8_t*)read_buffer32,
                                                     expected_read_bytes);
  a2dp_aptx_hd_encoder_cb.stats.media_read_total_actual_read_bytes +=
      bytes_read;
  if (bytes_read < expected_read_bytes) {
    LOG_WARN(LOG_TAG,
             "%s: underflow at PCM reading: read %u bytes instead of %u",
             __func__, bytes_read, expected_read_bytes);
    a2dp_aptx_hd_encoder_cb.stats.media_read_total_dropped_packets++;
    osi_free(p_buf);
    return;
  }
  a2dp_aptx_hd_encoder_cb.stats.media_read_total_actual_reads_count++;

  for (uint32_t reads = 0, offset = 0; reads < framing_params->pcm_reads;
       reads++, offset +=
                framing_params->pcm_bytes_per_read / sizeof(uint32_t)) {
    pcm_bytes_encoded +=
        aptx_hd_encode_24bit(framing_params, &encoded_ptr_index,
                             read_buffer32 + offset, encoded_ptr);
  }

  // Compute the number of encoded bytes
  const int COMPRESSION_RATIO = 4;
  size_t encoded_bytes = pcm_bytes_encoded / COMPRESSION_RATIO;
  p_buf->len += encoded_bytes;
  LOG_VERBOSE(LOG_TAG, "%s: encoded %zu PCM bytes to %zu", __func__,
              pcm_bytes_encoded, encoded_bytes);

  // Update the RTP timestamp
  *((uint32_t*)(p_buf + 1)) = a2dp_aptx_hd_encoder_cb.timestamp;
  const uint8_t BYTES_PER_FRAME = 3;
  uint32_t rtp_timestamp =
      (pcm_bytes_encoded /
       a2dp_aptx_hd_encoder_cb.feeding_params.channel_count) /
      BYTES_PER_FRAME;
  a2dp_aptx_hd_encoder_cb.timestamp += rtp_timestamp;

  if (p_buf->len > 0) {
    a2dp_aptx_hd_encoder_cb.enqueue_callback(p_buf, 1, bytes_read);
  } else {
    a2dp_aptx_hd_encoder_cb.stats.media_read_total_dropped_packets++;
    osi_free(p_buf);
  }
}

static size_t aptx_hd_encode_24bit(tAPTX_HD_FRAMING_PARAMS* framing_params,
                                   size_t* data_out_index, uint32_t* data32_in,
                                   uint8_t* data_out) {
  size_t pcm_bytes_encoded = 0;
  const uint8_t* p = (const uint8_t*)(data32_in);

  for (size_t aptx_hd_samples = 0;
       aptx_hd_samples < framing_params->pcm_bytes_per_read / 24;
       aptx_hd_samples++) {
    uint32_t pcmL[4];
    uint32_t pcmR[4];
    uint32_t encoded_sample[2];

    // Expand from AUDIO_FORMAT_PCM_24_BIT_PACKED data (3 bytes per sample)
    // into AUDIO_FORMAT_PCM_8_24_BIT (4 bytes per sample).
    for (size_t i = 0; i < 4; i++) {
      pcmL[i] = ((p[0] << 0) | (p[1] << 8) | (((int8_t)p[2]) << 16));
      p += 3;
      pcmR[i] = ((p[0] << 0) | (p[1] << 8) | (((int8_t)p[2]) << 16));
      p += 3;
    }

    aptx_hd_encoder_encode_stereo_func(
        a2dp_aptx_hd_encoder_cb.aptx_hd_encoder_state, &pcmL, &pcmR,
        &encoded_sample);

    uint8_t* encoded_ptr = (uint8_t*)&encoded_sample[0];
    data_out[*data_out_index + 0] = *(encoded_ptr + 2);
    data_out[*data_out_index + 1] = *(encoded_ptr + 1);
    data_out[*data_out_index + 2] = *(encoded_ptr + 0);
    data_out[*data_out_index + 3] = *(encoded_ptr + 6);
    data_out[*data_out_index + 4] = *(encoded_ptr + 5);
    data_out[*data_out_index + 5] = *(encoded_ptr + 4);

    pcm_bytes_encoded += 24;
    *data_out_index += 6;
  }

  return pcm_bytes_encoded;
}

period_ms_t A2dpCodecConfigAptxHd::encoderIntervalMs() const {
  return a2dp_vendor_aptx_hd_get_encoder_interval_ms();
}

int A2dpCodecConfigAptxHd::getEffectiveMtu() const {
  return a2dp_aptx_hd_encoder_cb.peer_mtu;
}

void A2dpCodecConfigAptxHd::debug_codec_dump(int fd) {
  a2dp_aptx_hd_encoder_stats_t* stats = &a2dp_aptx_hd_encoder_cb.stats;

  A2dpCodecConfig::debug_codec_dump(fd);

  dprintf(fd,
          "  Packet counts (expected/dropped)                        : %zu / "
          "%zu\n",
          stats->media_read_total_expected_packets,
          stats->media_read_total_dropped_packets);

  dprintf(fd,
          "  PCM read counts (expected/actual)                       : %zu / "
          "%zu\n",
          stats->media_read_total_expected_reads_count,
          stats->media_read_total_actual_reads_count);

  dprintf(fd,
          "  PCM read bytes (expected/actual)                        : %zu / "
          "%zu\n",
          stats->media_read_total_expected_read_bytes,
          stats->media_read_total_actual_read_bytes);
}
