/*
 * Copyright (c) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#include <Composition.h>

bool isVideoPlaneComposition(int meson_composition_type) {
    if (meson_composition_type == MESON_COMPOSITION_PLANE_AMVIDEO
        || meson_composition_type == MESON_COMPOSITION_PLANE_AMVIDEO_SIDEBAND
        || meson_composition_type == MESON_COMPOSITION_PLANE_HWCVIDEO) {
        return true;
    }

    return false;
}

bool isComposerComposition(int meson_composition_type) {
    if (meson_composition_type == MESON_COMPOSITION_CLIENT
        || meson_composition_type == MESON_COMPOSITION_GE2D
        || meson_composition_type == MESON_COMPOSITION_DUMMY)
        return true;
    else
        return false;
}

const char* compositionTypeToString(
    int meson_composition_type) {
    const char * compStr = "NONE";
    switch (meson_composition_type) {
        case MESON_COMPOSITION_UNDETERMINED:
            compStr = "NONE";
            break;
        case MESON_COMPOSITION_DUMMY:
            compStr = "DUMMY";
            break;
        case MESON_COMPOSITION_CLIENT:
            compStr = "CLIENT";
            break;
        case MESON_COMPOSITION_GE2D:
            compStr = "GE2D";
            break;
        case MESON_COMPOSITION_PLANE_AMVIDEO:
            compStr = "AMVIDEO";
            break;
        case MESON_COMPOSITION_PLANE_AMVIDEO_SIDEBAND:
            compStr = "SIDEBAND";
            break;
        case MESON_COMPOSITION_PLANE_OSD:
            compStr = "OSD";
            break;
        case MESON_COMPOSITION_PLANE_CURSOR:
            compStr = "CURSOR";
            break;
        case MESON_COMPOSITION_PLANE_HWCVIDEO:
            compStr = "HWCVIDEO";
            break;
        default:
            compStr = "UNKNOWN";
            break;
    }

    return compStr;
}


