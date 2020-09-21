/*
 * include/linux/amlogic/media/video_sink/ionvideo_ext.h
 *
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#ifndef IONVIDEO_EXT_H
#define IONVIDEO_EXT_H

extern int ionvideo_assign_map(char **receiver_name, int *inst);

extern int ionvideo_alloc_map(int *inst);

extern void ionvideo_release_map(int inst);

#endif /* IONVIDEO_EXT_H */
