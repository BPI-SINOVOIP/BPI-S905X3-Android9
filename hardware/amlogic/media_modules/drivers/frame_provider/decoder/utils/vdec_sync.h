/*
* Copyright (C) 2017 Amlogic, Inc. All rights reserved.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
* more details.
*
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*
* Description:
*/
#include <linux/list.h>
#include <linux/rbtree.h>
#include <linux/spinlock.h>
#include <linux/fence.h>
#include <linux/sync_file.h>
#include <uapi/linux/sync_file.h>

#define FENCE_USE_FOR_DRIVER	(0)
#define FENCE_USE_FOR_APP	(1)

struct sync_timeline {
	struct kref		kref;
	char			name[32];

	/* protected by lock */
	u64			context;
	u32			value;

	struct list_head	active_list_head;
	struct list_head	pt_list;
	spinlock_t		lock;

	u64			timestamp;
};

struct sync_pt {
	struct fence		fence;
	struct list_head	link;
	struct list_head	active_list;
	u64			timestamp;
};

struct vdec_sync {
	u8			name[32];
	void			*timeline;
	int			usage;
	int			fd;
	struct fence		*fence;
};

static inline struct sync_timeline *fence_parent(struct fence *fence)
{
	return container_of(fence->lock, struct sync_timeline, lock);
}

static inline struct sync_pt *get_sync_pt(struct fence *fence)
{
	return container_of(fence, struct sync_pt, fence);
}

struct fence *vdec_fence_get(int fd);

void vdec_fence_put(struct fence *fence);

int vdec_fence_wait(struct fence *fence, long timeout);

void vdec_timeline_create(struct vdec_sync *sync, u8 *name);

int vdec_timeline_create_fence(struct vdec_sync *sync);

void vdec_timeline_increase(struct vdec_sync *sync, u32 value);

void vdec_timeline_put(struct vdec_sync *sync);

int vdec_fence_status_get(struct fence *fence);

void vdec_fence_status_set(struct fence *fence, int status);

bool check_objs_all_signaled(struct vdec_sync *sync);

int vdec_clean_all_fence(struct vdec_sync *sync);

