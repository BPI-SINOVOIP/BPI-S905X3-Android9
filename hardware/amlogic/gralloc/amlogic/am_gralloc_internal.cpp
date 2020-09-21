/*
 * Copyright (c) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include <gralloc_priv.h>

#include "am_gralloc_internal.h"
#include "gralloc_usage_ext.h"


#if USE_BUFFER_USAGE
#include <hardware/gralloc1.h>
#else
#include <hardware/gralloc.h>
#endif


#define UNUSED(x) (void)x

bool am_gralloc_is_omx_metadata_extend_usage(
    uint64_t usage) {
#if USE_BUFFER_USAGE
    uint64_t omx_metadata_usage = GRALLOC1_PRODUCER_USAGE_VIDEO_DECODER
        | GRALLOC1_PRODUCER_USAGE_CPU_READ
        | GRALLOC1_PRODUCER_USAGE_CPU_WRITE;
    if (((usage & omx_metadata_usage) == omx_metadata_usage)
        && !(usage & GRALLOC1_PRODUCER_USAGE_GPU_RENDER_TARGET)) {
        return true;
    }
#else
    if (usage & GRALLOC_USAGE_AML_OMX_OVERLAY) {
        return true;
    }
#endif

    return false;
}

bool am_gralloc_is_omx_osd_extend_usage(uint64_t usage) {
#if USE_BUFFER_USAGE
    uint64_t omx_osd_usage = GRALLOC1_PRODUCER_USAGE_VIDEO_DECODER
        |GRALLOC1_PRODUCER_USAGE_GPU_RENDER_TARGET;
    if ((usage & omx_osd_usage) == omx_osd_usage) {
        return true;
    }
#else
    if (usage & GRALLOC_USAGE_AML_DMA_BUFFER) {
        return true;
    }
#endif
    return false;
}

bool am_gralloc_is_video_overlay_extend_usage(
    uint64_t usage) {
#if USE_BUFFER_USAGE
    uint64_t video_overlay_usage = GRALLOC1_PRODUCER_USAGE_VIDEO_DECODER;
    if (!am_gralloc_is_omx_metadata_extend_usage(usage)
        && !am_gralloc_is_omx_osd_extend_usage(usage)
        && ((usage & video_overlay_usage) == video_overlay_usage)) {
        return true;
    }
#else
    if (usage & GRALLOC_USAGE_AML_VIDEO_OVERLAY) {
        return true;
    }
#endif
    return false;
}

bool am_gralloc_is_secure_extend_usage(
    uint64_t usage) {
#if USE_BUFFER_USAGE
    if (usage & GRALLOC_USAGE_AML_SECURE || usage & GRALLOC1_PRODUCER_USAGE_PROTECTED) {
        return true;
    }
#else
    if (usage & GRALLOC_USAGE_AML_SECURE || usage & GRALLOC_USAGE_PROTECTED) {
        return true;
    }
#endif

    return false;
}

int am_gralloc_get_omx_metadata_extend_flag() {
    return private_handle_t::PRIV_FLAGS_VIDEO_OVERLAY
        | private_handle_t::PRIV_FLAGS_VIDEO_OMX;
}

int am_gralloc_get_coherent_extend_flag() {
    return private_handle_t::PRIV_FLAGS_USES_ION_DMA_HEAP
        | private_handle_t::PRIV_FLAGS_CONTINUOUS_BUF;
}

int am_gralloc_get_video_overlay_extend_flag() {
    return private_handle_t::PRIV_FLAGS_VIDEO_OVERLAY;
}

int am_gralloc_get_secure_extend_flag() {
    return private_handle_t::PRIV_FLAGS_SECURE_PROTECTED;
}


