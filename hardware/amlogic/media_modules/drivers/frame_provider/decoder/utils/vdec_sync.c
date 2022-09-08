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
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/sync_file.h>
#include "vdec_sync.h"

#define VDEC_DBG_ENABLE_FENCE	(0x100)

extern u32 vdec_get_debug(void);

static const struct fence_ops timeline_fence_ops;
static inline struct sync_pt *fence_to_sync_pt(struct fence *fence)
{
	if (fence->ops != &timeline_fence_ops)
		return NULL;
	return container_of(fence, struct sync_pt, fence);
}

/**
 * sync_timeline_create() - creates a sync object
 * @name:	sync_timeline name
 *
 * Creates a new sync_timeline. Returns the sync_timeline object or NULL in
 * case of error.
 */
static struct sync_timeline *sync_timeline_create(const char *name)
{
	struct sync_timeline *obj;

	obj = kzalloc(sizeof(*obj), GFP_KERNEL);
	if (!obj)
		return NULL;

	kref_init(&obj->kref);
	obj->context = fence_context_alloc(1);
	obj->timestamp = local_clock();
	strlcpy(obj->name, name, sizeof(obj->name));
	INIT_LIST_HEAD(&obj->active_list_head);
	INIT_LIST_HEAD(&obj->pt_list);
	spin_lock_init(&obj->lock);

	return obj;
}

static void sync_timeline_free(struct kref *kref)
{
	struct sync_timeline *obj =
		container_of(kref, struct sync_timeline, kref);

	pr_info("[VDEC-FENCE] free timeline: %lx\n", (ulong) obj);
	kfree(obj);
}

static void sync_timeline_get(struct sync_timeline *obj)
{
	kref_get(&obj->kref);
}

static void sync_timeline_put(struct sync_timeline *obj)
{
	kref_put(&obj->kref, sync_timeline_free);
}

static const char *timeline_fence_get_driver_name(struct fence *fence)
{
	struct sync_timeline *parent = fence_parent(fence);

	return parent->name;
}

static const char *timeline_fence_get_timeline_name(struct fence *fence)
{
	struct sync_timeline *parent = fence_parent(fence);

	return parent->name;
}

static void timeline_fence_release(struct fence *fence)
{
	struct sync_pt *pt = fence_to_sync_pt(fence);
	struct sync_timeline *parent = fence_parent(fence);
	unsigned long flags;

	/*pr_info("[VDEC-FENCE] release fence: %lx\n", (ulong) fence);*/

	spin_lock_irqsave(fence->lock, flags);
	list_del(&pt->link);
	if (!list_empty(&pt->active_list))
		list_del(&pt->active_list);
	spin_unlock_irqrestore(fence->lock, flags);
	sync_timeline_put(parent);
	fence_free(fence);
}

static bool timeline_fence_signaled(struct fence *fence)
{
	struct sync_timeline *parent = fence_parent(fence);
	struct sync_pt *pt = get_sync_pt(fence);

	if (__fence_is_later(fence->seqno, parent->value))
		return false;

	if (pt->timestamp > parent->timestamp)
		return false;

	return true;
}

static bool timeline_fence_enable_signaling(struct fence *fence)
{
	struct sync_pt *pt = container_of(fence, struct sync_pt, fence);
	struct sync_timeline *parent = fence_parent(fence);

	if (timeline_fence_signaled(fence))
		return false;

	list_add_tail(&pt->active_list, &parent->active_list_head);
	return true;
}

static void timeline_fence_disable_signaling(struct fence *fence)
{
	struct sync_pt *pt = container_of(fence, struct sync_pt, fence);

	list_del_init(&pt->active_list);
}

static void timeline_fence_value_str(struct fence *fence,
				    char *str, int size)
{
	snprintf(str, size, "%d", fence->seqno);
}

static void timeline_fence_timeline_value_str(struct fence *fence,
					     char *str, int size)
{
	struct sync_timeline *parent = fence_parent(fence);

	snprintf(str, size, "%d", parent->value);
}

static const struct fence_ops timeline_fence_ops = {
	.get_driver_name	= timeline_fence_get_driver_name,
	.get_timeline_name	= timeline_fence_get_timeline_name,
	.enable_signaling	= timeline_fence_enable_signaling,
	.disable_signaling	= timeline_fence_disable_signaling,
	.signaled		= timeline_fence_signaled,
	.wait			= fence_default_wait,
	.release		= timeline_fence_release,
	.fence_value_str	= timeline_fence_value_str,
	.timeline_value_str	= timeline_fence_timeline_value_str,
};

/**
 * sync_timeline_signal() - signal a status change on a sync_timeline
 * @obj:	sync_timeline to signal
 * @inc:	num to increment on timeline->value
 *
 * A sync implementation should call this any time one of it's fences
 * has signaled or has an error condition.
 */
static void sync_timeline_signal(struct sync_timeline *obj, unsigned int inc)
{
	struct sync_pt *pt, *next;
	unsigned long flags;

	spin_lock_irqsave(&obj->lock, flags);
	obj->value += inc;
	list_for_each_entry_safe(pt, next, &obj->active_list_head,
				 active_list) {
		if (fence_is_signaled(&pt->fence))
			list_del_init(&pt->active_list);
	}
	spin_unlock_irqrestore(&obj->lock, flags);
}

/**
 * sync_pt_create() - creates a sync pt
 * @parent:	fence's parent sync_timeline
 * @inc:	value of the fence
 *
 * Creates a new sync_pt as a child of @parent.  @size bytes will be
 * allocated allowing for implementation specific data to be kept after
 * the generic sync_timeline struct. Returns the sync_pt object or
 * NULL in case of error.
 */
static struct sync_pt *sync_pt_create(struct sync_timeline *obj,
				      unsigned int value)
{
	struct sync_pt *pt;
	unsigned long flags;

	pt = kzalloc(sizeof(*pt), GFP_KERNEL);
	if (!pt)
		return NULL;
	spin_lock_irqsave(&obj->lock, flags);
	sync_timeline_get(obj);
	fence_init(&pt->fence, &timeline_fence_ops, &obj->lock,
		   obj->context, value);
	list_add_tail(&pt->link, &obj->pt_list);
	INIT_LIST_HEAD(&pt->active_list);
	spin_unlock_irqrestore(&obj->lock, flags);

	return pt;
}

static void sync_pt_free(struct sync_timeline *obj,
			 struct sync_pt *pt)
{
	unsigned long flags;

	spin_lock_irqsave(&obj->lock, flags);
	list_del(&pt->link);
	sync_timeline_put(obj);
	spin_unlock_irqrestore(&obj->lock, flags);
	kfree(pt);
	pt = NULL;
}

static int timeline_create_fence(struct vdec_sync *sync, int usage,
				    struct fence **fence, int *fd, u32 value)
{
	int ret;
	struct sync_pt *pt;
	struct sync_file *sync_file;
	struct sync_timeline *obj = sync->timeline;

	if (obj == NULL)
		return -EPERM;

	pt = sync_pt_create(obj, value);
	if (!pt) {
		return -ENOMEM;
	}

	if (usage == FENCE_USE_FOR_APP) {
		*fd =  get_unused_fd_flags(O_CLOEXEC);
		if (*fd < 0) {
			return -EBADF;
			goto err;
		}

		sync_file = sync_file_create(&pt->fence);
		if (!sync_file) {
			ret = -ENOMEM;
			goto err;
		}

		fd_install(*fd, sync_file->file);

		/* decreases refcnt. */
		fence_put(&pt->fence);
	}

	*fence = &pt->fence;

	pt->timestamp = local_clock();

	if (vdec_get_debug() & VDEC_DBG_ENABLE_FENCE)
		pr_info("[VDEC-FENCE]: create fence: %lx, fd: %d, ref: %d, usage: %d\n",
			(ulong) &pt->fence, *fd, atomic_read(&pt->fence.refcount.refcount), usage);
	return 0;
err:
	put_unused_fd(*fd);
	if (pt)
		sync_pt_free(obj, pt);

	return ret;
}

struct fence *vdec_fence_get(int fd)
{
	return sync_file_get_fence(fd);
}
EXPORT_SYMBOL(vdec_fence_get);

void vdec_fence_put(struct fence *fence)
{
	if (vdec_get_debug() & VDEC_DBG_ENABLE_FENCE)
		pr_info("[VDEC-FENCE]: the fence (%px) cost time: %lld ns\n",
			fence, local_clock() - get_sync_pt(fence)->timestamp);
	fence_put(fence);
}
EXPORT_SYMBOL(vdec_fence_put);

int vdec_fence_wait(struct fence *fence, long timeout)
{
	if (vdec_get_debug() & VDEC_DBG_ENABLE_FENCE)
		pr_info("[VDEC-FENCE]: wait fence %lx.\n", (ulong) fence);

	return fence_wait_timeout(fence, false, timeout);
}
EXPORT_SYMBOL(vdec_fence_wait);

void vdec_timeline_create(struct vdec_sync *sync, u8 *name)
{
	snprintf(sync->name, sizeof(sync->name), "%s", name);

	sync->timeline = (void *)sync_timeline_create(sync->name);
	if (sync->timeline)
		pr_info("[VDEC-FENCE]: create timeline %lx, name: %s\n",
			(ulong) sync->timeline, sync->name);
	else
		pr_err("[VDEC-FENCE]: create timeline faild.\n");
}
EXPORT_SYMBOL(vdec_timeline_create);

int vdec_timeline_create_fence(struct vdec_sync *sync)
{
	struct sync_timeline *obj = sync->timeline;
	struct sync_pt *pt = NULL;
	ulong flags;
	u32 value = 0;

	if (obj == NULL)
		return -EPERM;

	spin_lock_irqsave(&obj->lock, flags);

	pt = list_last_entry(&obj->pt_list, struct sync_pt, link);
	value = obj->value + 1;

	if (!list_empty(&obj->pt_list)) {
		pt = list_last_entry(&obj->pt_list, struct sync_pt, link);
		if (value == pt->fence.seqno) {
			value++;
		}
	}
	spin_unlock_irqrestore(&obj->lock, flags);

	return timeline_create_fence(sync,
				     sync->usage,
				     &sync->fence,
				     &sync->fd,
				     value);
}
EXPORT_SYMBOL(vdec_timeline_create_fence);

void vdec_timeline_increase(struct vdec_sync *sync, u32 value)
{
	struct sync_timeline *obj = sync->timeline;

	if (obj == NULL)
		return;

	obj->timestamp = local_clock();

	if (vdec_get_debug() & VDEC_DBG_ENABLE_FENCE)
		pr_info("[VDEC-FENCE]: update timeline %d.\n",
			obj->value + value);

	sync_timeline_signal(obj, value);
}
EXPORT_SYMBOL(vdec_timeline_increase);

void vdec_timeline_put(struct vdec_sync *sync)
{
	struct sync_timeline *obj = sync->timeline;

	sync_timeline_put(obj);
}
EXPORT_SYMBOL(vdec_timeline_put);

void vdec_fence_status_set(struct fence *fence, int status)
{
	fence->error = status;
}
EXPORT_SYMBOL(vdec_fence_status_set);

int vdec_fence_status_get(struct fence *fence)
{
	return fence_get_status(fence);
}
EXPORT_SYMBOL(vdec_fence_status_get);

bool check_objs_all_signaled(struct vdec_sync *sync)
{
	struct sync_timeline *obj = sync->timeline;
	bool ret = false;
	ulong flags;

	spin_lock_irqsave(&obj->lock, flags);
	ret = list_empty(&obj->active_list_head);
	spin_unlock_irqrestore(&obj->lock, flags);

	return ret;
}
EXPORT_SYMBOL(check_objs_all_signaled);

int vdec_clean_all_fence(struct vdec_sync *sync)
{
	struct sync_timeline *obj = sync->timeline;
	struct sync_pt *pt, *next;

	spin_lock_irq(&obj->lock);

	list_for_each_entry_safe(pt, next, &obj->pt_list, link) {
		fence_set_error(&pt->fence, -ENOENT);
		fence_signal_locked(&pt->fence);
	}

	spin_unlock_irq(&obj->lock);

	return 0;
}
EXPORT_SYMBOL(vdec_clean_all_fence);

