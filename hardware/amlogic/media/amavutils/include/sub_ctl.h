/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */


#ifndef SUB_CTL_H
#define SUB_CTL_H

#ifdef  __cplusplus
extern "C" {
#endif

int media_set_subtitle_fps(int fps);
int media_set_subtitle_total(int total);
int media_set_subtitle_enable(int enable);
int media_set_subtitle_index(int index);
int media_set_subtitle_width(int width);
int media_set_subtitle_height(int height);
int media_set_subtitle_type(int type);
int media_set_subtitle_curr(int num);
int media_set_subtitle_startpts(int startpts);
int media_set_subtitle_reset(int res);
int media_set_subtitle_size(int size);
int media_set_subtitle_subtype(int type);
#ifdef  __cplusplus
}
#endif

#endif