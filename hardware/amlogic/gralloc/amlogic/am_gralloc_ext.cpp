/*
 * Copyright (c) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include "am_gralloc_ext.h"
#if USE_BUFFER_USAGE == 1
#include <hardware/gralloc1.h>
#else
#include <hardware/gralloc.h>
#include <gralloc_usage_ext.h>
#endif
#include <gralloc_priv.h>
#include "gralloc_buffer_priv.h"

bool am_gralloc_is_valid_graphic_buffer(
    const native_handle_t * hnd) {
    private_handle_t const* buffer = hnd ? private_handle_t::dynamicCast(hnd) : NULL;
    if (buffer)
        return true;
    return false;
}

int am_gralloc_ext_get_ext_attr(struct private_handle_t * hnd,
    buf_attr attr, int * val) {
    if (hnd->attr_base == MAP_FAILED) {
        if (gralloc_buffer_attr_map(hnd, 1) < 0) {
            return GRALLOC1_ERROR_BAD_HANDLE;
        }
    }

    if (gralloc_buffer_attr_read(hnd, attr, val) < 0) {
        gralloc_buffer_attr_unmap(hnd);
        return GRALLOC1_ERROR_BAD_HANDLE;
    }

    return GRALLOC1_ERROR_NONE;
}

int am_gralloc_ext_set_ext_attr(struct private_handle_t * hnd,
    buf_attr attr, int val) {
    if (hnd->attr_base == MAP_FAILED) {
        if (gralloc_buffer_attr_map(hnd, 1) < 0) {
            return GRALLOC1_ERROR_BAD_HANDLE;
        }
    }

    if (gralloc_buffer_attr_write(hnd, attr, &val) < 0) {
        gralloc_buffer_attr_unmap(hnd);
        return GRALLOC1_ERROR_BAD_HANDLE;
    }

    return GRALLOC1_ERROR_NONE;
}

#if USE_BUFFER_USAGE
#include <android/hardware/graphics/common/1.0/types.h>

using android::hardware::graphics::common::V1_0::BufferUsage;

uint64_t am_gralloc_get_video_overlay_producer_usage() {
    return static_cast<uint64_t>(BufferUsage::VIDEO_DECODER);
}

uint64_t am_gralloc_get_omx_metadata_producer_usage() {
    return static_cast<uint64_t>(BufferUsage::VIDEO_DECODER
        | BufferUsage::CPU_READ_OFTEN
        | BufferUsage::CPU_WRITE_OFTEN);
}

uint64_t am_gralloc_get_omx_osd_producer_usage() {
    return static_cast<uint64_t>(BufferUsage::VIDEO_DECODER
        | BufferUsage::GPU_RENDER_TARGET);
}

bool am_gralloc_is_omx_metadata_producer(uint64_t usage) {
    if (!am_gralloc_is_omx_osd_producer(usage)) {
        uint64_t omx_metadata_usage = am_gralloc_get_omx_metadata_producer_usage();
        if (((usage & omx_metadata_usage) == omx_metadata_usage)
            && !(usage & BufferUsage::GPU_RENDER_TARGET)) {
            return true;
        }
    }

    return false;
}
#else
uint64_t am_gralloc_get_video_overlay_producer_usage() {
#if USE_BUFFER_USAGE == 1
    return GRALLOC1_PRODUCER_USAGE_VIDEO_DECODER;
#else
    return GRALLOC_USAGE_AML_VIDEO_OVERLAY;
#endif
}

uint64_t am_gralloc_get_omx_metadata_producer_usage() {
#if USE_BUFFER_USAGE == 1
    return (GRALLOC1_PRODUCER_USAGE_VIDEO_DECODER ||
            GRALLOC1_PRODUCER_USAGE_CPU_READ_OFTEN ||
            GRALLOC1_PRODUCER_USAGE_CPU_WRITE_OFTEN);
#else
    return (GRALLOC_USAGE_AML_VIDEO_OVERLAY ||
            GRALLOC_USAGE_SW_READ_OFTEN ||
            GRALLOC_USAGE_SW_WRITE_OFTEN);
#endif
}

uint64_t am_gralloc_get_omx_osd_producer_usage() {
#if USE_BUFFER_USAGE == 1
    return (GRALLOC1_PRODUCER_USAGE_VIDEO_DECODER ||
            GRALLOC1_PRODUCER_USAGE_GPU_RENDER_TARGET);
#else
    return (GRALLOC_USAGE_AML_VIDEO_OVERLAY ||
            GRALLOC_USAGE_HW_RENDER);
#endif
}

bool am_gralloc_is_omx_metadata_producer(uint64_t usage) {
    if (!am_gralloc_is_omx_osd_producer(usage)) {
        uint64_t omx_metadata_usage = am_gralloc_get_omx_metadata_producer_usage();
        if (((usage & omx_metadata_usage) == omx_metadata_usage)
#if GRALLOC_USE_GRALLOC1_API == 1
            && !(usage & GRALLOC1_PRODUCER_USAGE_GPU_RENDER_TARGET)) {
#else
            && !(usage & GRALLOC_USAGE_HW_RENDER)) {
#endif
            return true;
        }
    }

    return false;
}
#endif

bool am_gralloc_is_omx_osd_producer(uint64_t usage) {
    uint64_t omx_osd_usage = am_gralloc_get_omx_osd_producer_usage();
    if ((usage & omx_osd_usage) == omx_osd_usage) {
        return true;
    }

    return false;
}

int am_gralloc_get_format(const native_handle_t * hnd) {
    private_handle_t const* buffer = hnd ? private_handle_t::dynamicCast(hnd) : NULL;
    if (buffer)
        return buffer->format;

    return -ENOMEM;
}

int am_gralloc_get_buffer_fd(const native_handle_t * hnd) {
    private_handle_t const* buffer = hnd ? private_handle_t::dynamicCast(hnd) : NULL;
    if (buffer)
        return buffer->share_fd;

    return -1;
}

int am_gralloc_get_stride_in_byte(const native_handle_t * hnd) {
    private_handle_t const* buffer = hnd ? private_handle_t::dynamicCast(hnd) : NULL;
    if (buffer)
        return buffer->byte_stride;

    return 0;
}

int am_gralloc_get_stride_in_pixel(const native_handle_t * hnd) {
    private_handle_t const* buffer = hnd ? private_handle_t::dynamicCast(hnd) : NULL;
    if (buffer)
        return buffer->stride;

    return 0;
}

int am_gralloc_get_width(const native_handle_t * hnd) {
    private_handle_t const* buffer = hnd ? private_handle_t::dynamicCast(hnd) : NULL;
    if (buffer)
        return buffer->req_width;

    return 0;
}

int am_gralloc_get_height(const native_handle_t * hnd) {
    private_handle_t const* buffer = hnd ? private_handle_t::dynamicCast(hnd) : NULL;
    if (buffer)
        return buffer->req_height;

    return 0;
}

uint64_t am_gralloc_get_producer_usage(
    const native_handle_t * hnd) {
    private_handle_t const* buffer = hnd ? private_handle_t::dynamicCast(hnd) : NULL;
    if (buffer)
        return buffer->producer_usage;

    return 0;
}

uint64_t am_gralloc_get_consumer_usage(
    const native_handle_t * hnd) {
    private_handle_t const* buffer = hnd ? private_handle_t::dynamicCast(hnd) : NULL;
    if (buffer)
        return buffer->consumer_usage;

    return 0;
}

bool am_gralloc_is_secure_buffer(const native_handle_t *hnd) {
    private_handle_t const* buffer = hnd ? private_handle_t::dynamicCast(hnd) : NULL;
     if (NULL == buffer)
        return true;

     if (buffer->flags & private_handle_t::PRIV_FLAGS_SECURE_PROTECTED)
         return true;

     return false;
}

bool am_gralloc_is_coherent_buffer(const native_handle_t * hnd) {
    private_handle_t const* buffer = hnd ? private_handle_t::dynamicCast(hnd) : NULL;
    if (NULL == buffer)
        return false;

    if ((buffer->flags & private_handle_t::PRIV_FLAGS_CONTINUOUS_BUF)
            || (buffer->flags & private_handle_t::PRIV_FLAGS_USES_ION_DMA_HEAP)) {
        return true;
    }

    return false;
}

bool am_gralloc_is_overlay_buffer(const native_handle_t * hnd) {
     private_handle_t const* buffer = hnd ? private_handle_t::dynamicCast(hnd) : NULL;
     if (buffer && (buffer->flags & private_handle_t::PRIV_FLAGS_VIDEO_OVERLAY))
         return true;

     return false;
}

bool am_gralloc_is_omx_metadata_buffer(
    const native_handle_t * hnd) {
    private_handle_t const* buffer = hnd ? private_handle_t::dynamicCast(hnd) : NULL;
    if (buffer && (buffer->flags & private_handle_t::PRIV_FLAGS_VIDEO_OMX))
        return true;

    return false;
 }

 int am_gralloc_get_omx_metadata_tunnel(
    const native_handle_t * hnd, int * tunnel) {
    private_handle_t * buffer = hnd ? private_handle_t::dynamicCast(hnd) : NULL;
    int ret = GRALLOC1_ERROR_NONE;
    if (buffer) {
        int val;
        ret = am_gralloc_ext_get_ext_attr(buffer,
            GRALLOC_ARM_BUFFER_ATTR_AM_OMX_TUNNEL, &val);
        if (ret == GRALLOC1_ERROR_NONE) {
            if (val == 1)
                *tunnel = 1;
            else
                *tunnel = 0;
        }
    } else {
        ret = GRALLOC1_ERROR_BAD_HANDLE;
    }

    return ret;
}

 int am_gralloc_set_omx_metadata_tunnel(
    const native_handle_t * hnd, int tunnel) {
     private_handle_t * buffer = hnd ? private_handle_t::dynamicCast(hnd) : NULL;
     int ret = GRALLOC1_ERROR_NONE;

    if (buffer) {
        ret = am_gralloc_ext_set_ext_attr(buffer,
            GRALLOC_ARM_BUFFER_ATTR_AM_OMX_TUNNEL, tunnel);
    } else {
        ret = GRALLOC1_ERROR_BAD_HANDLE;
    }

    return ret;
}

typedef struct am_sideband_handle {
   native_handle_t base;
   unsigned int id;
   int flags;
   int channel;
} am_sideband_handle_t;

#define AM_SIDEBAND_HANDLE_NUM_INT (3)
#define AM_SIDEBAND_HANDLE_NUM_FD (0)
#define AM_SIDEBAND_IDENTIFIER (0xabcdcdef)

native_handle_t * am_gralloc_create_sideband_handle(int type, int channel) {
    am_sideband_handle_t * pHnd = (am_sideband_handle_t *)
        native_handle_create(AM_SIDEBAND_HANDLE_NUM_FD,
        AM_SIDEBAND_HANDLE_NUM_INT);
    pHnd->id = AM_SIDEBAND_IDENTIFIER;

    if (type == AM_TV_SIDEBAND) {
        pHnd->flags = private_handle_t::PRIV_FLAGS_VIDEO_OVERLAY;
    } else if (type == AM_OMX_SIDEBAND) {
        pHnd->flags = private_handle_t::PRIV_FLAGS_VIDEO_OMX;
    } else if (type == AM_AMCODEX_SIDEBAND) {
        pHnd->flags = private_handle_t::PRIV_FLAGS_VIDEO_AMCODEX;
    }
    pHnd->channel = channel;

    return (native_handle_t *)pHnd;
}

int am_gralloc_destroy_sideband_handle(native_handle_t * hnd) {
    if (hnd) {
        native_handle_delete(hnd);
    }

    return 0;
}

 int am_gralloc_get_sideband_channel(
    const native_handle_t * hnd, int * channel) {
    if (!hnd || hnd->version != sizeof(native_handle_t)
        || hnd->numInts != AM_SIDEBAND_HANDLE_NUM_INT || hnd->numFds != AM_SIDEBAND_HANDLE_NUM_FD) {
        return GRALLOC1_ERROR_BAD_HANDLE;
    }

    am_sideband_handle_t * buffer = (am_sideband_handle_t *)(hnd);
    if (buffer->id != AM_SIDEBAND_IDENTIFIER)
        return GRALLOC1_ERROR_BAD_HANDLE;

    int ret = GRALLOC1_ERROR_NONE;
    if (buffer) {
        if (buffer->channel == AM_VIDEO_DEFAULT) {
            *channel = AM_VIDEO_DEFAULT;
        } else {
            *channel = AM_VIDEO_EXTERNAL;
        }
    } else {
        ret = GRALLOC1_ERROR_BAD_HANDLE;
    }

    return ret;
}

int am_gralloc_get_vpu_afbc_mask(const native_handle_t * hnd) {
    private_handle_t const* buffer = hnd ? private_handle_t::dynamicCast(hnd) : NULL;

    if (buffer) {
        uint64_t internalFormat = buffer->internal_format;
        int afbcFormat = 0;

        if (internalFormat & MALI_GRALLOC_INTFMT_AFBCENABLE_MASK) {
            afbcFormat |=
                (VPU_AFBC_EN | VPU_AFBC_YUV_TRANSFORM |VPU_AFBC_BLOCK_SPLIT);

            if (internalFormat & MALI_GRALLOC_INTFMT_AFBC_WIDEBLK) {
                afbcFormat |= VPU_AFBC_SUPER_BLOCK_ASPECT;
            }

            #if 0
            if (internalFormat & MALI_GRALLOC_INTFMT_AFBC_SPLITBLK) {
                afbcFormat |= VPU_AFBC_BLOCK_SPLIT;
            }
            #endif

            if (internalFormat & MALI_GRALLOC_INTFMT_AFBC_TILED_HEADERS) {
                afbcFormat |= VPU_AFBC_TILED_HEADER_EN;
            }

            #if 0
            if (gralloc_buffer_attr_map(buffer, 0) >= 0) {
                int tmp=0;
                if (gralloc_buffer_attr_read(buffer, GRALLOC_ARM_BUFFER_ATTR_AFBC_YUV_TRANS, &tmp) >= 0) {
                    if (tmp != 0) {
                        afbcFormat |= VPU_AFBC_YUV_TRANSFORM;
                    }
                }
            }
            #endif
        }
        return afbcFormat;
    }

    return 0;
}

