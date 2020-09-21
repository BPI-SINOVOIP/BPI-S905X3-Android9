/*
 * Copyright (c) 2017 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef COMPOSITION_H
#define COMPOSITION_H

#include <BasicTypes.h>

typedef enum {
    MESON_COMPOSITION_UNDETERMINED = 0,
    /*Compostion type of composer*/
    MESON_COMPOSITION_DUMMY = 1,
    MESON_COMPOSITION_CLIENT,
    MESON_COMPOSITION_GE2D,

    /*Compostion type of plane*/
    MESON_COMPOSITION_PLANE_CURSOR,
    MESON_COMPOSITION_PLANE_OSD,
    /*Video plane*/
    MESON_COMPOSITION_PLANE_AMVIDEO,
    MESON_COMPOSITION_PLANE_AMVIDEO_SIDEBAND,
    /*New video plane.*/
    MESON_COMPOSITION_PLANE_HWCVIDEO,
} meson_compositon_t;

bool isVideoPlaneComposition(int meson_composition_type);
bool isComposerComposition(int meson_composition_type);
const char* compositionTypeToString(int meson_composition_type);

#endif/*COMPOSITION_H*/
