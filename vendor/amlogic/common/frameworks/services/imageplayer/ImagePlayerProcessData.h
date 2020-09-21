/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: C++ file
 */
#ifndef ANDROID_GUI_IIMAGE_PLAYER_PROCESS_DATA_H
#define ANDROID_GUI_IIMAGE_PLAYER_PROCESS_DATA_H

extern void copy_data_to_mmap_buf(int fd, char *user_data,
                                  int width, int height,
                                  int format);

#endif
