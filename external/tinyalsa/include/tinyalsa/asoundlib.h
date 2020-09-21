/* asoundlib.h
**
** Copyright 2011, The Android Open Source Project
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**     * Redistributions of source code must retain the above copyright
**       notice, this list of conditions and the following disclaimer.
**     * Redistributions in binary form must reproduce the above copyright
**       notice, this list of conditions and the following disclaimer in the
**       documentation and/or other materials provided with the distribution.
**     * Neither the name of The Android Open Source Project nor the names of
**       its contributors may be used to endorse or promote products derived
**       from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY The Android Open Source Project ``AS IS'' AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED. IN NO EVENT SHALL The Android Open Source Project BE LIABLE
** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
** SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
** CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
** LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
** OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
** DAMAGE.
*/

#ifndef ASOUNDLIB_H
#define ASOUNDLIB_H

#include <sys/time.h>
#include <stddef.h>

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * PCM API
 */

struct pcm;

#define PCM_OUT        0x00000000
#define PCM_IN         0x10000000
#define PCM_MMAP       0x00000001
#define PCM_NOIRQ      0x00000002
#define PCM_NORESTART  0x00000004 /* PCM_NORESTART - when set, calls to
                                   * pcm_write for a playback stream will not
                                   * attempt to restart the stream in the case
                                   * of an underflow, but will return -EPIPE
                                   * instead.  After the first -EPIPE error, the
                                   * stream is considered to be stopped, and a
                                   * second call to pcm_write will attempt to
                                   * restart the stream.
                                   */
#define PCM_MONOTONIC  0x00000008 /* see pcm_get_htimestamp */

/* PCM runtime states */
#define	PCM_STATE_OPEN		0
#define	PCM_STATE_SETUP		1
#define	PCM_STATE_PREPARED	2
#define	PCM_STATE_RUNNING		3
#define	PCM_STATE_XRUN		4
#define	PCM_STATE_DRAINING	5
#define	PCM_STATE_PAUSED		6
#define	PCM_STATE_SUSPENDED	7
#define	PCM_STATE_DISCONNECTED	8

/* TLV header size*/
#define TLV_HEADER_SIZE (2 * sizeof(unsigned int))

/* Bit formats */
enum pcm_format {
    PCM_FORMAT_INVALID = -1,
    PCM_FORMAT_S16_LE = 0,  /* 16-bit signed */
    PCM_FORMAT_S32_LE,      /* 32-bit signed */
    PCM_FORMAT_S8,          /* 8-bit signed */
    PCM_FORMAT_S24_LE,      /* 24-bits in 4-bytes */
    PCM_FORMAT_S24_3LE,     /* 24-bits in 3-bytes */

    PCM_FORMAT_MAX,
};

/* Bitmask has 256 bits (32 bytes) in asound.h */
struct pcm_mask {
    unsigned int bits[32 / sizeof(unsigned int)];
};

/* Configuration for a stream */
struct pcm_config {
    unsigned int channels;
    unsigned int rate;
    unsigned int period_size;
    unsigned int period_count;
    enum pcm_format format;

    /* Values to use for the ALSA start, stop and silence thresholds, and
     * silence size.  Setting any one of these values to 0 will cause the
     * default tinyalsa values to be used instead.
     * Tinyalsa defaults are as follows.
     *
     * start_threshold   : period_count * period_size
     * stop_threshold    : period_count * period_size
     * silence_threshold : 0
     * silence_size      : 0
     */
    unsigned int start_threshold;
    unsigned int stop_threshold;
    unsigned int silence_threshold;
    unsigned int silence_size;

    /* Minimum number of frames available before pcm_mmap_write() will actually
     * write into the kernel buffer. Only used if the stream is opened in mmap mode
     * (pcm_open() called with PCM_MMAP flag set).   Use 0 for default.
     */
    int avail_min;
};

/* PCM parameters */
enum pcm_param
{
    /* mask parameters */
    PCM_PARAM_ACCESS,
    PCM_PARAM_FORMAT,
    PCM_PARAM_SUBFORMAT,
    /* interval parameters */
    PCM_PARAM_SAMPLE_BITS,
    PCM_PARAM_FRAME_BITS,
    PCM_PARAM_CHANNELS,
    PCM_PARAM_RATE,
    PCM_PARAM_PERIOD_TIME,
    PCM_PARAM_PERIOD_SIZE,
    PCM_PARAM_PERIOD_BYTES,
    PCM_PARAM_PERIODS,
    PCM_PARAM_BUFFER_TIME,
    PCM_PARAM_BUFFER_SIZE,
    PCM_PARAM_BUFFER_BYTES,
    PCM_PARAM_TICK_TIME,
};

/* Mixer control types */
enum mixer_ctl_type {
    MIXER_CTL_TYPE_BOOL,
    MIXER_CTL_TYPE_INT,
    MIXER_CTL_TYPE_ENUM,
    MIXER_CTL_TYPE_BYTE,
    MIXER_CTL_TYPE_IEC958,
    MIXER_CTL_TYPE_INT64,
    MIXER_CTL_TYPE_UNKNOWN,

    MIXER_CTL_TYPE_MAX,
};

/* Open and close a stream */
struct pcm *pcm_open(unsigned int card, unsigned int device,
                     unsigned int flags, struct pcm_config *config);
int pcm_close(struct pcm *pcm);
int pcm_is_ready(struct pcm *pcm);

/* Obtain the parameters for a PCM */
struct pcm_params *pcm_params_get(unsigned int card, unsigned int device,
                                  unsigned int flags);
void pcm_params_free(struct pcm_params *pcm_params);

struct pcm_mask *pcm_params_get_mask(struct pcm_params *pcm_params,
                                     enum pcm_param param);
unsigned int pcm_params_get_min(struct pcm_params *pcm_params,
                                enum pcm_param param);
void pcm_params_set_min(struct pcm_params *pcm_params,
                                enum pcm_param param, unsigned int val);
unsigned int pcm_params_get_max(struct pcm_params *pcm_params,
                                enum pcm_param param);
void pcm_params_set_max(struct pcm_params *pcm_params,
                                enum pcm_param param, unsigned int val);

/* Converts the pcm parameters to a human readable string.
 * The string parameter is a caller allocated buffer of size bytes,
 * which is then filled up to size - 1 and null terminated,
 * if size is greater than zero.
 * The return value is the number of bytes copied to string
 * (not including null termination) if less than size; otherwise,
 * the number of bytes required for the buffer.
 */
int pcm_params_to_string(struct pcm_params *params, char *string, unsigned int size);

/* Returns 1 if the pcm_format is present (format bit set) in
 * the pcm_params structure; 0 otherwise, or upon unrecognized format.
 */
int pcm_params_format_test(struct pcm_params *params, enum pcm_format format);

/* Set and get config */
int pcm_get_config(struct pcm *pcm, struct pcm_config *config);
int pcm_set_config(struct pcm *pcm, struct pcm_config *config);

/* Returns a human readable reason for the last error */
const char *pcm_get_error(struct pcm *pcm);

/* Returns the sample size in bits for a PCM format.
 * As with ALSA formats, this is the storage size for the format, whereas the
 * format represents the number of significant bits. For example,
 * PCM_FORMAT_S24_LE uses 32 bits of storage.
 */
unsigned int pcm_format_to_bits(enum pcm_format format);

/* Returns the buffer size (int frames) that should be used for pcm_write. */
unsigned int pcm_get_buffer_size(struct pcm *pcm);
unsigned int pcm_frames_to_bytes(struct pcm *pcm, unsigned int frames);
unsigned int pcm_bytes_to_frames(struct pcm *pcm, unsigned int bytes);

/* Returns the pcm latency in ms */
unsigned int pcm_get_latency(struct pcm *pcm);

/* Returns available frames in pcm buffer and corresponding time stamp.
 * The clock is CLOCK_MONOTONIC if flag PCM_MONOTONIC was specified in pcm_open,
 * otherwise the clock is CLOCK_REALTIME.
 * For an input stream, frames available are frames ready for the
 * application to read.
 * For an output stream, frames available are the number of empty frames available
 * for the application to write.
 */
int pcm_get_htimestamp(struct pcm *pcm, unsigned int *avail,
                       struct timespec *tstamp);

/* Returns the subdevice on which the pcm has been opened */
unsigned int pcm_get_subdevice(struct pcm *pcm);

/* Write data to the fifo.
 * Will start playback on the first write or on a write that
 * occurs after a fifo underrun.
 */
int pcm_write(struct pcm *pcm, const void *data, unsigned int count);
int pcm_read(struct pcm *pcm, void *data, unsigned int count);

/*
 * mmap() support.
 */
int pcm_mmap_write(struct pcm *pcm, const void *data, unsigned int count);
int pcm_mmap_read(struct pcm *pcm, void *data, unsigned int count);
int pcm_mmap_begin(struct pcm *pcm, void **areas, unsigned int *offset,
                   unsigned int *frames);
int pcm_mmap_commit(struct pcm *pcm, unsigned int offset, unsigned int frames);
int pcm_mmap_avail(struct pcm *pcm);

/* Returns current read/write position in the mmap buffer with associated time stamp.
 */
int pcm_mmap_get_hw_ptr(struct pcm* pcm, unsigned int *hw_ptr, struct timespec *tstamp);

/* Prepare the PCM substream to be triggerable */
int pcm_prepare(struct pcm *pcm);
/* Start and stop a PCM channel that doesn't transfer data */
int pcm_start(struct pcm *pcm);
int pcm_stop(struct pcm *pcm);

/* ioctl function for PCM driver */
int pcm_ioctl(struct pcm *pcm, int request, ...);

/* Interrupt driven API */
int pcm_wait(struct pcm *pcm, int timeout);
int pcm_get_poll_fd(struct pcm *pcm);

/* Change avail_min after the stream has been opened with no need to stop the stream.
 * Only accepted if opened with PCM_MMAP and PCM_NOIRQ flags
 */
int pcm_set_avail_min(struct pcm *pcm, int avail_min);

/*
 * MIXER API
 */

struct mixer;
struct mixer_ctl;

/* Open and close a mixer */
struct mixer *mixer_open(unsigned int card);
void mixer_close(struct mixer *mixer);

/* Get info about a mixer */
const char *mixer_get_name(struct mixer *mixer);

/* Obtain mixer controls */
unsigned int mixer_get_num_ctls(struct mixer *mixer);
struct mixer_ctl *mixer_get_ctl(struct mixer *mixer, unsigned int id);
struct mixer_ctl *mixer_get_ctl_by_name(struct mixer *mixer, const char *name);

/* Get info about mixer controls */
const char *mixer_ctl_get_name(struct mixer_ctl *ctl);
enum mixer_ctl_type mixer_ctl_get_type(struct mixer_ctl *ctl);
const char *mixer_ctl_get_type_string(struct mixer_ctl *ctl);
unsigned int mixer_ctl_get_num_values(struct mixer_ctl *ctl);
unsigned int mixer_ctl_get_num_enums(struct mixer_ctl *ctl);
const char *mixer_ctl_get_enum_string(struct mixer_ctl *ctl,
                                      unsigned int enum_id);

/* Some sound cards update their controls due to external events,
 * such as HDMI EDID byte data changing when an HDMI cable is
 * connected. This API allows the count of elements to be updated.
 */
void mixer_ctl_update(struct mixer_ctl *ctl);

/* Set and get mixer controls */
int mixer_ctl_get_percent(struct mixer_ctl *ctl, unsigned int id);
int mixer_ctl_set_percent(struct mixer_ctl *ctl, unsigned int id, int percent);

int mixer_ctl_get_value(struct mixer_ctl *ctl, unsigned int id);
int mixer_ctl_is_access_tlv_rw(struct mixer_ctl *ctl);
int mixer_ctl_get_array(struct mixer_ctl *ctl, void *array, size_t count);
int mixer_ctl_set_value(struct mixer_ctl *ctl, unsigned int id, int value);
int mixer_ctl_set_array(struct mixer_ctl *ctl, const void *array, size_t count);
int mixer_ctl_set_enum_by_string(struct mixer_ctl *ctl, const char *string);

/* Determine range of integer mixer controls */
int mixer_ctl_get_range_min(struct mixer_ctl *ctl);
int mixer_ctl_get_range_max(struct mixer_ctl *ctl);

int mixer_subscribe_events(struct mixer *mixer, int subscribe);
int mixer_wait_event(struct mixer *mixer, int timeout);
int mixer_consume_event(struct mixer *mixer);

#if defined(__cplusplus)
}  /* extern "C" */
#endif

#endif
