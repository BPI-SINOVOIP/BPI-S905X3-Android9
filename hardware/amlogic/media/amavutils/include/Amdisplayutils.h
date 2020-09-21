/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */



#ifndef AMDISPLAY_UTILS_H
#define AMDISPLAY_UTILS_H

#ifdef  __cplusplus
extern "C" {
#endif
    int amdisplay_utils_get_size(int *width, int *height);
    int amdisplay_utils_get_size_fb2(int *width, int *height);

    /*scale osd mode ,only support x1 x2*/
    int amdisplay_utils_set_scale_mode(int scale_wx, int scale_hx);
    int amdisplay_utils_get_osd_rotation();
#ifdef  __cplusplus
}
#endif


#endif

