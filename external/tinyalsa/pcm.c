/* pcm.c
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

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <poll.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <limits.h>

#include <linux/ioctl.h>
#define __force
#define __bitwise
#define __user
#include <sound/asound.h>

#include <tinyalsa/asoundlib.h>

#define PARAM_MAX SNDRV_PCM_HW_PARAM_LAST_INTERVAL

/* Logs information into a string; follows snprintf() in that
 * offset may be greater than size, and though no characters are copied
 * into string, characters are still counted into offset. */
#define STRLOG(string, offset, size, ...) \
    do { int temp, clipoffset = offset > size ? size : offset; \
         temp = snprintf(string + clipoffset, size - clipoffset, __VA_ARGS__); \
         if (temp > 0) offset += temp; } while (0)

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

/* refer to SNDRV_PCM_ACCESS_##index in sound/asound.h. */
static const char * const access_lookup[] = {
        "MMAP_INTERLEAVED",
        "MMAP_NONINTERLEAVED",
        "MMAP_COMPLEX",
        "RW_INTERLEAVED",
        "RW_NONINTERLEAVED",
};

/* refer to SNDRV_PCM_FORMAT_##index in sound/asound.h. */
static const char * const format_lookup[] = {
        /*[0] =*/ "S8",
        "U8",
        "S16_LE",
        "S16_BE",
        "U16_LE",
        "U16_BE",
        "S24_LE",
        "S24_BE",
        "U24_LE",
        "U24_BE",
        "S32_LE",
        "S32_BE",
        "U32_LE",
        "U32_BE",
        "FLOAT_LE",
        "FLOAT_BE",
        "FLOAT64_LE",
        "FLOAT64_BE",
        "IEC958_SUBFRAME_LE",
        "IEC958_SUBFRAME_BE",
        "MU_LAW",
        "A_LAW",
        "IMA_ADPCM",
        "MPEG",
        /*[24] =*/ "GSM",
        /* gap */
        [31] = "SPECIAL",
        "S24_3LE",
        "S24_3BE",
        "U24_3LE",
        "U24_3BE",
        "S20_3LE",
        "S20_3BE",
        "U20_3LE",
        "U20_3BE",
        "S18_3LE",
        "S18_3BE",
        "U18_3LE",
        /*[43] =*/ "U18_3BE",
#if 0
        /* recent additions, may not be present on local asound.h */
        "G723_24",
        "G723_24_1B",
        "G723_40",
        "G723_40_1B",
        "DSD_U8",
        "DSD_U16_LE",
#endif
};

/* refer to SNDRV_PCM_SUBFORMAT_##index in sound/asound.h. */
static const char * const subformat_lookup[] = {
        "STD",
};

static inline int param_is_mask(int p)
{
    return (p >= SNDRV_PCM_HW_PARAM_FIRST_MASK) &&
        (p <= SNDRV_PCM_HW_PARAM_LAST_MASK);
}

static inline int param_is_interval(int p)
{
    return (p >= SNDRV_PCM_HW_PARAM_FIRST_INTERVAL) &&
        (p <= SNDRV_PCM_HW_PARAM_LAST_INTERVAL);
}

static inline struct snd_interval *param_to_interval(struct snd_pcm_hw_params *p, int n)
{
    return &(p->intervals[n - SNDRV_PCM_HW_PARAM_FIRST_INTERVAL]);
}

static inline struct snd_mask *param_to_mask(struct snd_pcm_hw_params *p, int n)
{
    return &(p->masks[n - SNDRV_PCM_HW_PARAM_FIRST_MASK]);
}

static void param_set_mask(struct snd_pcm_hw_params *p, int n, unsigned int bit)
{
    if (bit >= SNDRV_MASK_MAX)
        return;
    if (param_is_mask(n)) {
        struct snd_mask *m = param_to_mask(p, n);
        m->bits[0] = 0;
        m->bits[1] = 0;
        m->bits[bit >> 5] |= (1 << (bit & 31));
    }
}

static void param_set_min(struct snd_pcm_hw_params *p, int n, unsigned int val)
{
    if (param_is_interval(n)) {
        struct snd_interval *i = param_to_interval(p, n);
        i->min = val;
    }
}

static unsigned int param_get_min(struct snd_pcm_hw_params *p, int n)
{
    if (param_is_interval(n)) {
        struct snd_interval *i = param_to_interval(p, n);
        return i->min;
    }
    return 0;
}

static void param_set_max(struct snd_pcm_hw_params *p, int n, unsigned int val)
{
    if (param_is_interval(n)) {
        struct snd_interval *i = param_to_interval(p, n);
        i->max = val;
    }
}

static unsigned int param_get_max(struct snd_pcm_hw_params *p, int n)
{
    if (param_is_interval(n)) {
        struct snd_interval *i = param_to_interval(p, n);
        return i->max;
    }
    return 0;
}

static void param_set_int(struct snd_pcm_hw_params *p, int n, unsigned int val)
{
    if (param_is_interval(n)) {
        struct snd_interval *i = param_to_interval(p, n);
        i->min = val;
        i->max = val;
        i->integer = 1;
    }
}

static unsigned int param_get_int(struct snd_pcm_hw_params *p, int n)
{
    if (param_is_interval(n)) {
        struct snd_interval *i = param_to_interval(p, n);
        if (i->integer)
            return i->max;
    }
    return 0;
}

static void param_init(struct snd_pcm_hw_params *p)
{
    int n;

    memset(p, 0, sizeof(*p));
    for (n = SNDRV_PCM_HW_PARAM_FIRST_MASK;
         n <= SNDRV_PCM_HW_PARAM_LAST_MASK; n++) {
            struct snd_mask *m = param_to_mask(p, n);
            m->bits[0] = ~0;
            m->bits[1] = ~0;
    }
    for (n = SNDRV_PCM_HW_PARAM_FIRST_INTERVAL;
         n <= SNDRV_PCM_HW_PARAM_LAST_INTERVAL; n++) {
            struct snd_interval *i = param_to_interval(p, n);
            i->min = 0;
            i->max = ~0;
    }
    p->rmask = ~0U;
    p->cmask = 0;
    p->info = ~0U;
}

#define PCM_ERROR_MAX 128

struct pcm {
    int fd;
    unsigned int flags;
    int running:1;
    int prepared:1;
    int underruns;
    unsigned int buffer_size;
    unsigned int boundary;
    char error[PCM_ERROR_MAX];
    struct pcm_config config;
    struct snd_pcm_mmap_status *mmap_status;
    struct snd_pcm_mmap_control *mmap_control;
    struct snd_pcm_sync_ptr *sync_ptr;
    void *mmap_buffer;
    unsigned int noirq_frames_per_msec;
    int wait_for_avail_min;
    unsigned int subdevice;
};

unsigned int pcm_get_buffer_size(struct pcm *pcm)
{
    return pcm->buffer_size;
}

const char* pcm_get_error(struct pcm *pcm)
{
    return pcm->error;
}

unsigned int pcm_get_subdevice(struct pcm *pcm)
{
    return pcm->subdevice;
}

static int oops(struct pcm *pcm, int e, const char *fmt, ...)
{
    va_list ap;
    int sz;

    va_start(ap, fmt);
    vsnprintf(pcm->error, PCM_ERROR_MAX, fmt, ap);
    va_end(ap);
    sz = strlen(pcm->error);

    if (e)
        snprintf(pcm->error + sz, PCM_ERROR_MAX - sz,
                 ": %s", strerror(e));
    return -1;
}

static unsigned int pcm_format_to_alsa(enum pcm_format format)
{
    switch (format) {
    case PCM_FORMAT_S32_LE:
        return SNDRV_PCM_FORMAT_S32_LE;
    case PCM_FORMAT_S8:
        return SNDRV_PCM_FORMAT_S8;
    case PCM_FORMAT_S24_3LE:
        return SNDRV_PCM_FORMAT_S24_3LE;
    case PCM_FORMAT_S24_LE:
        return SNDRV_PCM_FORMAT_S24_LE;
    default:
    case PCM_FORMAT_S16_LE:
        return SNDRV_PCM_FORMAT_S16_LE;
    };
}

unsigned int pcm_format_to_bits(enum pcm_format format)
{
    switch (format) {
    case PCM_FORMAT_S32_LE:
    case PCM_FORMAT_S24_LE:
        return 32;
    case PCM_FORMAT_S24_3LE:
        return 24;
    default:
    case PCM_FORMAT_S16_LE:
        return 16;
    };
}

unsigned int pcm_bytes_to_frames(struct pcm *pcm, unsigned int bytes)
{
    return bytes / (pcm->config.channels *
        (pcm_format_to_bits(pcm->config.format) >> 3));
}

unsigned int pcm_frames_to_bytes(struct pcm *pcm, unsigned int frames)
{
    return frames * pcm->config.channels *
        (pcm_format_to_bits(pcm->config.format) >> 3);
}

static int pcm_sync_ptr(struct pcm *pcm, int flags) {
    if (pcm->sync_ptr) {
        pcm->sync_ptr->flags = flags;
        if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_SYNC_PTR, pcm->sync_ptr) < 0)
            return -1;
    }
    return 0;
}

static int pcm_hw_mmap_status(struct pcm *pcm) {

    if (pcm->sync_ptr)
        return 0;

    int page_size = sysconf(_SC_PAGE_SIZE);
    pcm->mmap_status = mmap(NULL, page_size, PROT_READ, MAP_FILE | MAP_SHARED,
                            pcm->fd, SNDRV_PCM_MMAP_OFFSET_STATUS);
    if (pcm->mmap_status == MAP_FAILED)
        pcm->mmap_status = NULL;
    if (!pcm->mmap_status)
        goto mmap_error;

    pcm->mmap_control = mmap(NULL, page_size, PROT_READ | PROT_WRITE,
                             MAP_FILE | MAP_SHARED, pcm->fd, SNDRV_PCM_MMAP_OFFSET_CONTROL);
    if (pcm->mmap_control == MAP_FAILED)
        pcm->mmap_control = NULL;
    if (!pcm->mmap_control) {
        munmap(pcm->mmap_status, page_size);
        pcm->mmap_status = NULL;
        goto mmap_error;
    }
    if (pcm->flags & PCM_MMAP)
        pcm->mmap_control->avail_min = pcm->config.avail_min;
    else
        pcm->mmap_control->avail_min = 1;

    return 0;

mmap_error:

    pcm->sync_ptr = calloc(1, sizeof(*pcm->sync_ptr));
    if (!pcm->sync_ptr)
        return -ENOMEM;
    pcm->mmap_status = &pcm->sync_ptr->s.status;
    pcm->mmap_control = &pcm->sync_ptr->c.control;
    if (pcm->flags & PCM_MMAP)
        pcm->mmap_control->avail_min = pcm->config.avail_min;
    else
        pcm->mmap_control->avail_min = 1;

    pcm_sync_ptr(pcm, 0);

    return 0;
}

static void pcm_hw_munmap_status(struct pcm *pcm) {
    if (pcm->sync_ptr) {
        free(pcm->sync_ptr);
        pcm->sync_ptr = NULL;
    } else {
        int page_size = sysconf(_SC_PAGE_SIZE);
        if (pcm->mmap_status)
            munmap(pcm->mmap_status, page_size);
        if (pcm->mmap_control)
            munmap(pcm->mmap_control, page_size);
    }
    pcm->mmap_status = NULL;
    pcm->mmap_control = NULL;
}

static int pcm_areas_copy(struct pcm *pcm, unsigned int pcm_offset,
                          char *buf, unsigned int src_offset,
                          unsigned int frames)
{
    int size_bytes = pcm_frames_to_bytes(pcm, frames);
    int pcm_offset_bytes = pcm_frames_to_bytes(pcm, pcm_offset);
    int src_offset_bytes = pcm_frames_to_bytes(pcm, src_offset);

    /* interleaved only atm */
    if (pcm->flags & PCM_IN)
        memcpy(buf + src_offset_bytes,
               (char*)pcm->mmap_buffer + pcm_offset_bytes,
               size_bytes);
    else
        memcpy((char*)pcm->mmap_buffer + pcm_offset_bytes,
               buf + src_offset_bytes,
               size_bytes);
    return 0;
}

static int pcm_mmap_transfer_areas(struct pcm *pcm, char *buf,
                                unsigned int offset, unsigned int size)
{
    void *pcm_areas;
    int commit;
    unsigned int pcm_offset, frames, count = 0;

    while (size > 0) {
        frames = size;
        pcm_mmap_begin(pcm, &pcm_areas, &pcm_offset, &frames);
        pcm_areas_copy(pcm, pcm_offset, buf, offset, frames);
        commit = pcm_mmap_commit(pcm, pcm_offset, frames);
        if (commit < 0) {
            oops(pcm, errno, "failed to commit %d frames\n", frames);
            return commit;
        }

        offset += commit;
        count += commit;
        size -= commit;
    }
    return count;
}

int pcm_get_htimestamp(struct pcm *pcm, unsigned int *avail,
                       struct timespec *tstamp)
{
    int frames;
    int rc;
    snd_pcm_uframes_t hw_ptr;

    if (!pcm_is_ready(pcm))
        return -1;

    rc = pcm_sync_ptr(pcm, SNDRV_PCM_SYNC_PTR_APPL|SNDRV_PCM_SYNC_PTR_HWSYNC);
    if (rc < 0)
        return -1;

    if ((pcm->mmap_status->state != PCM_STATE_RUNNING) &&
            (pcm->mmap_status->state != PCM_STATE_DRAINING))
        return -1;

    *tstamp = pcm->mmap_status->tstamp;
    if (tstamp->tv_sec == 0 && tstamp->tv_nsec == 0)
        return -1;

    hw_ptr = pcm->mmap_status->hw_ptr;
    if (pcm->flags & PCM_IN)
        frames = hw_ptr - pcm->mmap_control->appl_ptr;
    else
        frames = hw_ptr + pcm->buffer_size - pcm->mmap_control->appl_ptr;

    if (frames < 0)
        frames += pcm->boundary;
    else if (frames > (int)pcm->boundary)
        frames -= pcm->boundary;

    *avail = (unsigned int)frames;

    return 0;
}

int pcm_mmap_get_hw_ptr(struct pcm* pcm, unsigned int *hw_ptr, struct timespec *tstamp)
{
    int frames;
    int rc;

    if (pcm == NULL || hw_ptr == NULL || tstamp == NULL)
        return oops(pcm, EINVAL, "pcm %p, hw_ptr %p, tstamp %p", pcm, hw_ptr, tstamp);

    if (!pcm_is_ready(pcm))
        return oops(pcm, errno, "pcm_is_ready failed");

    rc = pcm_sync_ptr(pcm, SNDRV_PCM_SYNC_PTR_HWSYNC);
    if (rc < 0)
        return oops(pcm, errno, "pcm_sync_ptr failed");

    if (pcm->mmap_status == NULL)
        return oops(pcm, EINVAL, "pcm %p, mmap_status is NULL", pcm);

    if ((pcm->mmap_status->state != PCM_STATE_RUNNING) &&
            (pcm->mmap_status->state != PCM_STATE_DRAINING))
        return oops(pcm, ENOSYS, "invalid stream state %d", pcm->mmap_status->state);

    *tstamp = pcm->mmap_status->tstamp;
    if (tstamp->tv_sec == 0 && tstamp->tv_nsec == 0)
        return oops(pcm, errno, "invalid time stamp");

    *hw_ptr = pcm->mmap_status->hw_ptr;

    return 0;
}

int pcm_write(struct pcm *pcm, const void *data, unsigned int count)
{
    struct snd_xferi x;

    if (pcm->flags & PCM_IN)
        return -EINVAL;

    x.buf = (void*)data;
    x.frames = count / (pcm->config.channels *
                        pcm_format_to_bits(pcm->config.format) / 8);

    for (;;) {
        if (!pcm->running) {
            int prepare_error = pcm_prepare(pcm);
            if (prepare_error)
                return prepare_error;
            if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_WRITEI_FRAMES, &x))
                return oops(pcm, errno, "cannot write initial data");
            pcm->running = 1;
            return 0;
        }
        if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_WRITEI_FRAMES, &x)) {
            pcm->prepared = 0;
            pcm->running = 0;
            if (errno == EPIPE) {
                /* we failed to make our window -- try to restart if we are
                 * allowed to do so.  Otherwise, simply allow the EPIPE error to
                 * propagate up to the app level */
                pcm->underruns++;
                if (pcm->flags & PCM_NORESTART)
                    return -EPIPE;
                continue;
            }
            return oops(pcm, errno, "cannot write stream data");
        }
        return 0;
    }
}

int pcm_read(struct pcm *pcm, void *data, unsigned int count)
{
    struct snd_xferi x;

    if (!(pcm->flags & PCM_IN))
        return -EINVAL;

    x.buf = data;
    x.frames = count / (pcm->config.channels *
                        pcm_format_to_bits(pcm->config.format) / 8);

    for (;;) {
        if (!pcm->running) {
            if (pcm_start(pcm) < 0) {
                fprintf(stderr, "start error");
                return -errno;
            }
        }
        if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_READI_FRAMES, &x)) {
            pcm->prepared = 0;
            pcm->running = 0;
            if (errno == EPIPE) {
                    /* we failed to make our window -- try to restart */
                pcm->underruns++;
                continue;
            }
            return oops(pcm, errno, "cannot read stream data");
        }
        return 0;
    }
}

static struct pcm bad_pcm = {
    .fd = -1,
};

struct pcm_params *pcm_params_get(unsigned int card, unsigned int device,
                                  unsigned int flags)
{
    struct snd_pcm_hw_params *params;
    char fn[256];
    int fd;

    snprintf(fn, sizeof(fn), "/dev/snd/pcmC%uD%u%c", card, device,
             flags & PCM_IN ? 'c' : 'p');

    fd = open(fn, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "cannot open device '%s'\n", fn);
        goto err_open;
    }

    params = calloc(1, sizeof(struct snd_pcm_hw_params));
    if (!params)
        goto err_calloc;

    param_init(params);
    if (ioctl(fd, SNDRV_PCM_IOCTL_HW_REFINE, params)) {
        fprintf(stderr, "SNDRV_PCM_IOCTL_HW_REFINE error (%d)\n", errno);
        goto err_hw_refine;
    }

    close(fd);

    return (struct pcm_params *)params;

err_hw_refine:
    free(params);
err_calloc:
    close(fd);
err_open:
    return NULL;
}

void pcm_params_free(struct pcm_params *pcm_params)
{
    struct snd_pcm_hw_params *params = (struct snd_pcm_hw_params *)pcm_params;

    if (params)
        free(params);
}

static int pcm_param_to_alsa(enum pcm_param param)
{
    switch (param) {
    case PCM_PARAM_ACCESS:
        return SNDRV_PCM_HW_PARAM_ACCESS;
    case PCM_PARAM_FORMAT:
        return SNDRV_PCM_HW_PARAM_FORMAT;
    case PCM_PARAM_SUBFORMAT:
        return SNDRV_PCM_HW_PARAM_SUBFORMAT;
    case PCM_PARAM_SAMPLE_BITS:
        return SNDRV_PCM_HW_PARAM_SAMPLE_BITS;
        break;
    case PCM_PARAM_FRAME_BITS:
        return SNDRV_PCM_HW_PARAM_FRAME_BITS;
        break;
    case PCM_PARAM_CHANNELS:
        return SNDRV_PCM_HW_PARAM_CHANNELS;
        break;
    case PCM_PARAM_RATE:
        return SNDRV_PCM_HW_PARAM_RATE;
        break;
    case PCM_PARAM_PERIOD_TIME:
        return SNDRV_PCM_HW_PARAM_PERIOD_TIME;
        break;
    case PCM_PARAM_PERIOD_SIZE:
        return SNDRV_PCM_HW_PARAM_PERIOD_SIZE;
        break;
    case PCM_PARAM_PERIOD_BYTES:
        return SNDRV_PCM_HW_PARAM_PERIOD_BYTES;
        break;
    case PCM_PARAM_PERIODS:
        return SNDRV_PCM_HW_PARAM_PERIODS;
        break;
    case PCM_PARAM_BUFFER_TIME:
        return SNDRV_PCM_HW_PARAM_BUFFER_TIME;
        break;
    case PCM_PARAM_BUFFER_SIZE:
        return SNDRV_PCM_HW_PARAM_BUFFER_SIZE;
        break;
    case PCM_PARAM_BUFFER_BYTES:
        return SNDRV_PCM_HW_PARAM_BUFFER_BYTES;
        break;
    case PCM_PARAM_TICK_TIME:
        return SNDRV_PCM_HW_PARAM_TICK_TIME;
        break;

    default:
        return -1;
    }
}

struct pcm_mask *pcm_params_get_mask(struct pcm_params *pcm_params,
                                     enum pcm_param param)
{
    int p;
    struct snd_pcm_hw_params *params = (struct snd_pcm_hw_params *)pcm_params;
    if (params == NULL) {
        return NULL;
    }

    p = pcm_param_to_alsa(param);
    if (p < 0 || !param_is_mask(p)) {
        return NULL;
    }

    return (struct pcm_mask *)param_to_mask(params, p);
}

unsigned int pcm_params_get_min(struct pcm_params *pcm_params,
                                enum pcm_param param)
{
    struct snd_pcm_hw_params *params = (struct snd_pcm_hw_params *)pcm_params;
    int p;

    if (!params)
        return 0;

    p = pcm_param_to_alsa(param);
    if (p < 0)
        return 0;

    return param_get_min(params, p);
}

void pcm_params_set_min(struct pcm_params *pcm_params,
                                enum pcm_param param, unsigned int val)
{
    struct snd_pcm_hw_params *params = (struct snd_pcm_hw_params *)pcm_params;
    int p;

    if (!params)
        return;

    p = pcm_param_to_alsa(param);
    if (p < 0)
        return;

    param_set_min(params, p, val);
}

unsigned int pcm_params_get_max(struct pcm_params *pcm_params,
                                enum pcm_param param)
{
    struct snd_pcm_hw_params *params = (struct snd_pcm_hw_params *)pcm_params;
    int p;

    if (!params)
        return 0;

    p = pcm_param_to_alsa(param);
    if (p < 0)
        return 0;

    return param_get_max(params, p);
}

void pcm_params_set_max(struct pcm_params *pcm_params,
                                enum pcm_param param, unsigned int val)
{
    struct snd_pcm_hw_params *params = (struct snd_pcm_hw_params *)pcm_params;
    int p;

    if (!params)
        return;

    p = pcm_param_to_alsa(param);
    if (p < 0)
        return;

    param_set_max(params, p, val);
}

static int pcm_mask_test(struct pcm_mask *m, unsigned int index)
{
    const unsigned int bitshift = 5; /* for 32 bit integer */
    const unsigned int bitmask = (1 << bitshift) - 1;
    unsigned int element;

    element = index >> bitshift;
    if (element >= ARRAY_SIZE(m->bits))
        return 0; /* for safety, but should never occur */
    return (m->bits[element] >> (index & bitmask)) & 1;
}

static int pcm_mask_to_string(struct pcm_mask *m, char *string, unsigned int size,
                              char *mask_name,
                              const char * const *bit_array_name, size_t bit_array_size)
{
    unsigned int i;
    unsigned int offset = 0;

    if (m == NULL)
        return 0;
    if (bit_array_size < 32) {
        STRLOG(string, offset, size, "%12s:\t%#08x\n", mask_name, m->bits[0]);
    } else { /* spans two or more bitfields, print with an array index */
        for (i = 0; i < (bit_array_size + 31) >> 5; ++i) {
            STRLOG(string, offset, size, "%9s[%d]:\t%#08x\n",
                   mask_name, i, m->bits[i]);
        }
    }
    for (i = 0; i < bit_array_size; ++i) {
        if (pcm_mask_test(m, i)) {
            STRLOG(string, offset, size, "%12s \t%s\n", "", bit_array_name[i]);
        }
    }
    return offset;
}

int pcm_params_to_string(struct pcm_params *params, char *string, unsigned int size)
{
    struct pcm_mask *m;
    unsigned int min, max;
    unsigned int clipoffset, offset;

    m = pcm_params_get_mask(params, PCM_PARAM_ACCESS);
    offset = pcm_mask_to_string(m, string, size,
                                 "Access", access_lookup, ARRAY_SIZE(access_lookup));
    m = pcm_params_get_mask(params, PCM_PARAM_FORMAT);
    clipoffset = offset > size ? size : offset;
    offset += pcm_mask_to_string(m, string + clipoffset, size - clipoffset,
                                 "Format", format_lookup, ARRAY_SIZE(format_lookup));
    m = pcm_params_get_mask(params, PCM_PARAM_SUBFORMAT);
    clipoffset = offset > size ? size : offset;
    offset += pcm_mask_to_string(m, string + clipoffset, size - clipoffset,
                                 "Subformat", subformat_lookup, ARRAY_SIZE(subformat_lookup));
    min = pcm_params_get_min(params, PCM_PARAM_RATE);
    max = pcm_params_get_max(params, PCM_PARAM_RATE);
    STRLOG(string, offset, size, "        Rate:\tmin=%uHz\tmax=%uHz\n", min, max);
    min = pcm_params_get_min(params, PCM_PARAM_CHANNELS);
    max = pcm_params_get_max(params, PCM_PARAM_CHANNELS);
    STRLOG(string, offset, size, "    Channels:\tmin=%u\t\tmax=%u\n", min, max);
    min = pcm_params_get_min(params, PCM_PARAM_SAMPLE_BITS);
    max = pcm_params_get_max(params, PCM_PARAM_SAMPLE_BITS);
    STRLOG(string, offset, size, " Sample bits:\tmin=%u\t\tmax=%u\n", min, max);
    min = pcm_params_get_min(params, PCM_PARAM_PERIOD_SIZE);
    max = pcm_params_get_max(params, PCM_PARAM_PERIOD_SIZE);
    STRLOG(string, offset, size, " Period size:\tmin=%u\t\tmax=%u\n", min, max);
    min = pcm_params_get_min(params, PCM_PARAM_PERIODS);
    max = pcm_params_get_max(params, PCM_PARAM_PERIODS);
    STRLOG(string, offset, size, "Period count:\tmin=%u\t\tmax=%u\n", min, max);
    return offset;
}

int pcm_params_format_test(struct pcm_params *params, enum pcm_format format)
{
    unsigned int alsa_format = pcm_format_to_alsa(format);

    if (alsa_format == SNDRV_PCM_FORMAT_S16_LE && format != PCM_FORMAT_S16_LE)
        return 0; /* caution: format not recognized is equivalent to S16_LE */
    return pcm_mask_test(pcm_params_get_mask(params, PCM_PARAM_FORMAT), alsa_format);
}

int pcm_close(struct pcm *pcm)
{
    if (pcm == &bad_pcm)
        return 0;

    pcm_hw_munmap_status(pcm);

    if (pcm->flags & PCM_MMAP) {
        pcm_stop(pcm);
        munmap(pcm->mmap_buffer, pcm_frames_to_bytes(pcm, pcm->buffer_size));
    }

    if (pcm->fd >= 0)
        close(pcm->fd);
    pcm->prepared = 0;
    pcm->running = 0;
    pcm->buffer_size = 0;
    pcm->fd = -1;
    free(pcm);
    return 0;
}

struct pcm *pcm_open(unsigned int card, unsigned int device,
                     unsigned int flags, struct pcm_config *config)
{
    struct pcm *pcm;
    struct snd_pcm_info info;
    struct snd_pcm_hw_params params;
    struct snd_pcm_sw_params sparams;
    char fn[256];
    int rc;

    if (!config) {
        return &bad_pcm; /* TODO: could support default config here */
    }
    pcm = calloc(1, sizeof(struct pcm));
    if (!pcm)
        return &bad_pcm; /* TODO: could support default config here */

    pcm->config = *config;

    snprintf(fn, sizeof(fn), "/dev/snd/pcmC%uD%u%c", card, device,
             flags & PCM_IN ? 'c' : 'p');

    pcm->flags = flags;
    pcm->fd = open(fn, O_RDWR|O_NONBLOCK);
    if (pcm->fd < 0) {
        oops(pcm, errno, "cannot open device '%s'", fn);
        return pcm;
    }

    if (fcntl(pcm->fd, F_SETFL, fcntl(pcm->fd, F_GETFL) &
              ~O_NONBLOCK) < 0) {
        oops(pcm, errno, "failed to reset blocking mode '%s'", fn);
        goto fail_close;
    }

    if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_INFO, &info)) {
        oops(pcm, errno, "cannot get info");
        goto fail_close;
    }
    pcm->subdevice = info.subdevice;

    param_init(&params);
    param_set_mask(&params, SNDRV_PCM_HW_PARAM_FORMAT,
                   pcm_format_to_alsa(config->format));
    param_set_mask(&params, SNDRV_PCM_HW_PARAM_SUBFORMAT,
                   SNDRV_PCM_SUBFORMAT_STD);
    param_set_min(&params, SNDRV_PCM_HW_PARAM_PERIOD_SIZE, config->period_size);
    param_set_int(&params, SNDRV_PCM_HW_PARAM_SAMPLE_BITS,
                  pcm_format_to_bits(config->format));
    param_set_int(&params, SNDRV_PCM_HW_PARAM_FRAME_BITS,
                  pcm_format_to_bits(config->format) * config->channels);
    param_set_int(&params, SNDRV_PCM_HW_PARAM_CHANNELS,
                  config->channels);
    param_set_int(&params, SNDRV_PCM_HW_PARAM_PERIODS, config->period_count);
    param_set_int(&params, SNDRV_PCM_HW_PARAM_RATE, config->rate);

    if (flags & PCM_NOIRQ) {
        if (!(flags & PCM_MMAP)) {
            oops(pcm, EINVAL, "noirq only currently supported with mmap().");
            goto fail_close;
        }

        params.flags |= SNDRV_PCM_HW_PARAMS_NO_PERIOD_WAKEUP;
        pcm->noirq_frames_per_msec = config->rate / 1000;
    }

    if (flags & PCM_MMAP)
        param_set_mask(&params, SNDRV_PCM_HW_PARAM_ACCESS,
                       SNDRV_PCM_ACCESS_MMAP_INTERLEAVED);
    else
        param_set_mask(&params, SNDRV_PCM_HW_PARAM_ACCESS,
                       SNDRV_PCM_ACCESS_RW_INTERLEAVED);

    if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_HW_PARAMS, &params)) {
        oops(pcm, errno, "cannot set hw params");
        goto fail_close;
    }

    /* get our refined hw_params */
    config->period_size = param_get_int(&params, SNDRV_PCM_HW_PARAM_PERIOD_SIZE);
    config->period_count = param_get_int(&params, SNDRV_PCM_HW_PARAM_PERIODS);
    pcm->buffer_size = config->period_count * config->period_size;

    if (flags & PCM_MMAP) {
        pcm->mmap_buffer = mmap(NULL, pcm_frames_to_bytes(pcm, pcm->buffer_size),
                                PROT_READ | PROT_WRITE, MAP_FILE | MAP_SHARED, pcm->fd, 0);
        if (pcm->mmap_buffer == MAP_FAILED) {
            oops(pcm, errno, "failed to mmap buffer %d bytes\n",
                 pcm_frames_to_bytes(pcm, pcm->buffer_size));
            goto fail_close;
        }
    }

    memset(&sparams, 0, sizeof(sparams));
    sparams.tstamp_mode = SNDRV_PCM_TSTAMP_ENABLE;
    sparams.period_step = 1;

    if (!config->start_threshold) {
        if (pcm->flags & PCM_IN)
            pcm->config.start_threshold = sparams.start_threshold = 1;
        else
            pcm->config.start_threshold = sparams.start_threshold =
                config->period_count * config->period_size / 2;
    } else
        sparams.start_threshold = config->start_threshold;

    /* pick a high stop threshold - todo: does this need further tuning */
    if (!config->stop_threshold) {
        if (pcm->flags & PCM_IN)
            pcm->config.stop_threshold = sparams.stop_threshold =
                config->period_count * config->period_size * 10;
        else
            pcm->config.stop_threshold = sparams.stop_threshold =
                config->period_count * config->period_size;
    }
    else
        sparams.stop_threshold = config->stop_threshold;

    if (!pcm->config.avail_min) {
        if (pcm->flags & PCM_MMAP)
            pcm->config.avail_min = sparams.avail_min = pcm->config.period_size;
        else
            pcm->config.avail_min = sparams.avail_min = 1;
    } else
        sparams.avail_min = config->avail_min;

    sparams.xfer_align = config->period_size / 2; /* needed for old kernels */
    sparams.silence_threshold = config->silence_threshold;
    sparams.silence_size = config->silence_size;
    pcm->boundary = sparams.boundary = pcm->buffer_size;

    while (pcm->boundary * 2 <= INT_MAX - pcm->buffer_size)
        pcm->boundary *= 2;

    if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_SW_PARAMS, &sparams)) {
        oops(pcm, errno, "cannot set sw params");
        goto fail;
    }

    rc = pcm_hw_mmap_status(pcm);
    if (rc < 0) {
        oops(pcm, errno, "mmap status failed");
        goto fail;
    }

#ifdef SNDRV_PCM_IOCTL_TTSTAMP
    if (pcm->flags & PCM_MONOTONIC) {
        int arg = SNDRV_PCM_TSTAMP_TYPE_MONOTONIC;
        rc = ioctl(pcm->fd, SNDRV_PCM_IOCTL_TTSTAMP, &arg);
        if (rc < 0) {
            oops(pcm, errno, "cannot set timestamp type");
            goto fail;
        }
    }
#endif

    pcm->underruns = 0;
    return pcm;

fail:
    if (flags & PCM_MMAP)
        munmap(pcm->mmap_buffer, pcm_frames_to_bytes(pcm, pcm->buffer_size));
fail_close:
    close(pcm->fd);
    pcm->fd = -1;
    return pcm;
}

int pcm_is_ready(struct pcm *pcm)
{
    return pcm->fd >= 0;
}

int pcm_prepare(struct pcm *pcm)
{
    if (pcm->prepared)
        return 0;

    if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_PREPARE) < 0)
        return oops(pcm, errno, "cannot prepare channel");

    pcm->prepared = 1;
    return 0;
}

int pcm_start(struct pcm *pcm)
{
    int prepare_error = pcm_prepare(pcm);
    if (prepare_error)
        return prepare_error;

    if (pcm->flags & PCM_MMAP)
	    pcm_sync_ptr(pcm, 0);

    if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_START) < 0)
        return oops(pcm, errno, "cannot start channel");

    pcm->running = 1;
    return 0;
}

int pcm_stop(struct pcm *pcm)
{
    if (ioctl(pcm->fd, SNDRV_PCM_IOCTL_DROP) < 0)
        return oops(pcm, errno, "cannot stop channel");

    pcm->prepared = 0;
    pcm->running = 0;
    return 0;
}

static inline int pcm_mmap_playback_avail(struct pcm *pcm)
{
    int avail;

    avail = pcm->mmap_status->hw_ptr + pcm->buffer_size - pcm->mmap_control->appl_ptr;

    if (avail < 0)
        avail += pcm->boundary;
    else if (avail > (int)pcm->boundary)
        avail -= pcm->boundary;

    return avail;
}

static inline int pcm_mmap_capture_avail(struct pcm *pcm)
{
    int avail = pcm->mmap_status->hw_ptr - pcm->mmap_control->appl_ptr;
    if (avail < 0)
        avail += pcm->boundary;
    return avail;
}

int pcm_mmap_avail(struct pcm *pcm)
{
    pcm_sync_ptr(pcm, SNDRV_PCM_SYNC_PTR_HWSYNC);
    if (pcm->flags & PCM_IN)
        return pcm_mmap_capture_avail(pcm);
    else
        return pcm_mmap_playback_avail(pcm);
}

static void pcm_mmap_appl_forward(struct pcm *pcm, int frames)
{
    unsigned int appl_ptr = pcm->mmap_control->appl_ptr;
    appl_ptr += frames;

    /* check for boundary wrap */
    if (appl_ptr > pcm->boundary)
         appl_ptr -= pcm->boundary;
    pcm->mmap_control->appl_ptr = appl_ptr;
}

int pcm_mmap_begin(struct pcm *pcm, void **areas, unsigned int *offset,
                   unsigned int *frames)
{
    unsigned int continuous, copy_frames, avail;

    /* return the mmap buffer */
    *areas = pcm->mmap_buffer;

    /* and the application offset in frames */
    *offset = pcm->mmap_control->appl_ptr % pcm->buffer_size;

    avail = pcm_mmap_avail(pcm);
    if (avail > pcm->buffer_size)
        avail = pcm->buffer_size;
    continuous = pcm->buffer_size - *offset;

    /* we can only copy frames if the are availabale and continuos */
    copy_frames = *frames;
    if (copy_frames > avail)
        copy_frames = avail;
    if (copy_frames > continuous)
        copy_frames = continuous;
    *frames = copy_frames;

    return 0;
}

int pcm_mmap_commit(struct pcm *pcm, unsigned int offset __attribute__((unused)), unsigned int frames)
{
    /* update the application pointer in userspace and kernel */
    pcm_mmap_appl_forward(pcm, frames);
    pcm_sync_ptr(pcm, 0);

    return frames;
}

int pcm_avail_update(struct pcm *pcm)
{
    pcm_sync_ptr(pcm, 0);
    return pcm_mmap_avail(pcm);
}

int pcm_state(struct pcm *pcm)
{
    int err = pcm_sync_ptr(pcm, 0);
    if (err < 0)
        return err;

    return pcm->mmap_status->state;
}

int pcm_set_avail_min(struct pcm *pcm, int avail_min)
{
    if ((~pcm->flags) & (PCM_MMAP | PCM_NOIRQ))
        return -ENOSYS;

    pcm->config.avail_min = avail_min;
    return 0;
}

int pcm_wait(struct pcm *pcm, int timeout)
{
    struct pollfd pfd;
    int err;

    pfd.fd = pcm->fd;
    pfd.events = POLLOUT | POLLERR | POLLNVAL;

    do {
        /* let's wait for avail or timeout */
        err = poll(&pfd, 1, timeout);
        if (err < 0)
            return -errno;

        /* timeout ? */
        if (err == 0)
            return 0;

        /* have we been interrupted ? */
        if (errno == -EINTR)
            continue;

        /* check for any errors */
        if (pfd.revents & (POLLERR | POLLNVAL)) {
            switch (pcm_state(pcm)) {
            case PCM_STATE_XRUN:
                return -EPIPE;
            case PCM_STATE_SUSPENDED:
                return -ESTRPIPE;
            case PCM_STATE_DISCONNECTED:
                return -ENODEV;
            default:
                return -EIO;
            }
        }
    /* poll again if fd not ready for IO */
    } while (!(pfd.revents & (POLLIN | POLLOUT)));

    return 1;
}

int pcm_get_poll_fd(struct pcm *pcm)
{
    return pcm->fd;
}

int pcm_mmap_transfer(struct pcm *pcm, const void *buffer, unsigned int bytes)
{
    int err = 0, frames, avail;
    unsigned int offset = 0, count;

    if (bytes == 0)
        return 0;

    count = pcm_bytes_to_frames(pcm, bytes);

    while (count > 0) {

        /* get the available space for writing new frames */
        avail = pcm_avail_update(pcm);
        if (avail < 0) {
            fprintf(stderr, "cannot determine available mmap frames");
            return err;
        }

        /* start the audio if we reach the threshold */
	    if (!pcm->running &&
            (pcm->buffer_size - avail) >= pcm->config.start_threshold) {
            if (pcm_start(pcm) < 0) {
               fprintf(stderr, "start error: hw 0x%x app 0x%x avail 0x%x\n",
                    (unsigned int)pcm->mmap_status->hw_ptr,
                    (unsigned int)pcm->mmap_control->appl_ptr,
                    avail);
                return -errno;
            }
            pcm->wait_for_avail_min = 0;
        }

        /* sleep until we have space to write new frames */
        if (pcm->running) {
            /* enable waiting for avail_min threshold when less frames than we have to write
             * are available. */
            if (!pcm->wait_for_avail_min && (count > (unsigned int)avail))
                pcm->wait_for_avail_min = 1;

            if (pcm->wait_for_avail_min && (avail < pcm->config.avail_min)) {
                int time = -1;

                /* disable waiting for avail_min threshold to allow small amounts of data to be
                 * written without waiting as long as there is enough room in buffer. */
                pcm->wait_for_avail_min = 0;

                if (pcm->flags & PCM_NOIRQ)
                    time = (pcm->config.avail_min - avail) / pcm->noirq_frames_per_msec;

                err = pcm_wait(pcm, time);
                if (err < 0) {
                    pcm->prepared = 0;
                    pcm->running = 0;
                    oops(pcm, errno, "wait error: hw 0x%x app 0x%x avail 0x%x\n",
                        (unsigned int)pcm->mmap_status->hw_ptr,
                        (unsigned int)pcm->mmap_control->appl_ptr,
                        avail);
                    pcm->mmap_control->appl_ptr = 0;
                    return err;
                }
                continue;
            }
        }

        frames = count;
        if (frames > avail)
            frames = avail;

        if (!frames)
            break;

        /* copy frames from buffer */
        frames = pcm_mmap_transfer_areas(pcm, (void *)buffer, offset, frames);
        if (frames < 0) {
            fprintf(stderr, "write error: hw 0x%x app 0x%x avail 0x%x\n",
                    (unsigned int)pcm->mmap_status->hw_ptr,
                    (unsigned int)pcm->mmap_control->appl_ptr,
                    avail);
            return frames;
        }

        offset += frames;
        count -= frames;
    }

    return 0;
}

int pcm_mmap_write(struct pcm *pcm, const void *data, unsigned int count)
{
    if ((~pcm->flags) & (PCM_OUT | PCM_MMAP))
        return -ENOSYS;

    return pcm_mmap_transfer(pcm, (void *)data, count);
}

int pcm_mmap_read(struct pcm *pcm, void *data, unsigned int count)
{
    if ((~pcm->flags) & (PCM_IN | PCM_MMAP))
        return -ENOSYS;

    return pcm_mmap_transfer(pcm, data, count);
}

int pcm_ioctl(struct pcm *pcm, int request, ...)
{
    va_list ap;
    void * arg;

    if (!pcm_is_ready(pcm))
        return -1;

    va_start(ap, request);
    arg = va_arg(ap, void *);
    va_end(ap);

    return ioctl(pcm->fd, request, arg);
}
