/*
 * Copyright © 2015 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include "anv_private.h"

static int
anv_ioctl(int fd, unsigned long request, void *arg)
{
   int ret;

   do {
      ret = ioctl(fd, request, arg);
   } while (ret == -1 && (errno == EINTR || errno == EAGAIN));

   return ret;
}

/**
 * Wrapper around DRM_IOCTL_I915_GEM_CREATE.
 *
 * Return gem handle, or 0 on failure. Gem handles are never 0.
 */
uint32_t
anv_gem_create(struct anv_device *device, size_t size)
{
   struct drm_i915_gem_create gem_create = {
      .size = size,
   };

   int ret = anv_ioctl(device->fd, DRM_IOCTL_I915_GEM_CREATE, &gem_create);
   if (ret != 0) {
      /* FIXME: What do we do if this fails? */
      return 0;
   }

   return gem_create.handle;
}

void
anv_gem_close(struct anv_device *device, uint32_t gem_handle)
{
   struct drm_gem_close close = {
      .handle = gem_handle,
   };

   anv_ioctl(device->fd, DRM_IOCTL_GEM_CLOSE, &close);
}

/**
 * Wrapper around DRM_IOCTL_I915_GEM_MMAP.
 */
void*
anv_gem_mmap(struct anv_device *device, uint32_t gem_handle,
             uint64_t offset, uint64_t size, uint32_t flags)
{
   struct drm_i915_gem_mmap gem_mmap = {
      .handle = gem_handle,
      .offset = offset,
      .size = size,
      .flags = flags,
   };

   int ret = anv_ioctl(device->fd, DRM_IOCTL_I915_GEM_MMAP, &gem_mmap);
   if (ret != 0)
      return MAP_FAILED;

   VG(VALGRIND_MALLOCLIKE_BLOCK(gem_mmap.addr_ptr, gem_mmap.size, 0, 1));
   return (void *)(uintptr_t) gem_mmap.addr_ptr;
}

/* This is just a wrapper around munmap, but it also notifies valgrind that
 * this map is no longer valid.  Pair this with anv_gem_mmap().
 */
void
anv_gem_munmap(void *p, uint64_t size)
{
   VG(VALGRIND_FREELIKE_BLOCK(p, 0));
   munmap(p, size);
}

uint32_t
anv_gem_userptr(struct anv_device *device, void *mem, size_t size)
{
   struct drm_i915_gem_userptr userptr = {
      .user_ptr = (__u64)((unsigned long) mem),
      .user_size = size,
      .flags = 0,
   };

   int ret = anv_ioctl(device->fd, DRM_IOCTL_I915_GEM_USERPTR, &userptr);
   if (ret == -1)
      return 0;

   return userptr.handle;
}

int
anv_gem_set_caching(struct anv_device *device,
                    uint32_t gem_handle, uint32_t caching)
{
   struct drm_i915_gem_caching gem_caching = {
      .handle = gem_handle,
      .caching = caching,
   };

   return anv_ioctl(device->fd, DRM_IOCTL_I915_GEM_SET_CACHING, &gem_caching);
}

int
anv_gem_set_domain(struct anv_device *device, uint32_t gem_handle,
                   uint32_t read_domains, uint32_t write_domain)
{
   struct drm_i915_gem_set_domain gem_set_domain = {
      .handle = gem_handle,
      .read_domains = read_domains,
      .write_domain = write_domain,
   };

   return anv_ioctl(device->fd, DRM_IOCTL_I915_GEM_SET_DOMAIN, &gem_set_domain);
}

/**
 * On error, \a timeout_ns holds the remaining time.
 */
int
anv_gem_wait(struct anv_device *device, uint32_t gem_handle, int64_t *timeout_ns)
{
   struct drm_i915_gem_wait wait = {
      .bo_handle = gem_handle,
      .timeout_ns = *timeout_ns,
      .flags = 0,
   };

   int ret = anv_ioctl(device->fd, DRM_IOCTL_I915_GEM_WAIT, &wait);
   *timeout_ns = wait.timeout_ns;

   return ret;
}

int
anv_gem_execbuffer(struct anv_device *device,
                   struct drm_i915_gem_execbuffer2 *execbuf)
{
   return anv_ioctl(device->fd, DRM_IOCTL_I915_GEM_EXECBUFFER2, execbuf);
}

int
anv_gem_set_tiling(struct anv_device *device,
                   uint32_t gem_handle, uint32_t stride, uint32_t tiling)
{
   int ret;

   /* set_tiling overwrites the input on the error path, so we have to open
    * code anv_ioctl.
    */
   do {
      struct drm_i915_gem_set_tiling set_tiling = {
         .handle = gem_handle,
         .tiling_mode = tiling,
         .stride = stride,
      };

      ret = ioctl(device->fd, DRM_IOCTL_I915_GEM_SET_TILING, &set_tiling);
   } while (ret == -1 && (errno == EINTR || errno == EAGAIN));

   return ret;
}

int
anv_gem_get_param(int fd, uint32_t param)
{
   int tmp;

   drm_i915_getparam_t gp = {
      .param = param,
      .value = &tmp,
   };

   int ret = anv_ioctl(fd, DRM_IOCTL_I915_GETPARAM, &gp);
   if (ret == 0)
      return tmp;

   return 0;
}

bool
anv_gem_get_bit6_swizzle(int fd, uint32_t tiling)
{
   struct drm_gem_close close;
   int ret;

   struct drm_i915_gem_create gem_create = {
      .size = 4096,
   };

   if (anv_ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &gem_create)) {
      assert(!"Failed to create GEM BO");
      return false;
   }

   bool swizzled = false;

   /* set_tiling overwrites the input on the error path, so we have to open
    * code anv_ioctl.
    */
   do {
      struct drm_i915_gem_set_tiling set_tiling = {
         .handle = gem_create.handle,
         .tiling_mode = tiling,
         .stride = tiling == I915_TILING_X ? 512 : 128,
      };

      ret = ioctl(fd, DRM_IOCTL_I915_GEM_SET_TILING, &set_tiling);
   } while (ret == -1 && (errno == EINTR || errno == EAGAIN));

   if (ret != 0) {
      assert(!"Failed to set BO tiling");
      goto close_and_return;
   }

   struct drm_i915_gem_get_tiling get_tiling = {
      .handle = gem_create.handle,
   };

   if (anv_ioctl(fd, DRM_IOCTL_I915_GEM_GET_TILING, &get_tiling)) {
      assert(!"Failed to get BO tiling");
      goto close_and_return;
   }

   swizzled = get_tiling.swizzle_mode != I915_BIT_6_SWIZZLE_NONE;

close_and_return:

   memset(&close, 0, sizeof(close));
   close.handle = gem_create.handle;
   anv_ioctl(fd, DRM_IOCTL_GEM_CLOSE, &close);

   return swizzled;
}

int
anv_gem_create_context(struct anv_device *device)
{
   struct drm_i915_gem_context_create create = { 0 };

   int ret = anv_ioctl(device->fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &create);
   if (ret == -1)
      return -1;

   return create.ctx_id;
}

int
anv_gem_destroy_context(struct anv_device *device, int context)
{
   struct drm_i915_gem_context_destroy destroy = {
      .ctx_id = context,
   };

   return anv_ioctl(device->fd, DRM_IOCTL_I915_GEM_CONTEXT_DESTROY, &destroy);
}

int
anv_gem_get_aperture(int fd, uint64_t *size)
{
   struct drm_i915_gem_get_aperture aperture = { 0 };

   int ret = anv_ioctl(fd, DRM_IOCTL_I915_GEM_GET_APERTURE, &aperture);
   if (ret == -1)
      return -1;

   *size = aperture.aper_available_size;

   return 0;
}

int
anv_gem_handle_to_fd(struct anv_device *device, uint32_t gem_handle)
{
   struct drm_prime_handle args = {
      .handle = gem_handle,
      .flags = DRM_CLOEXEC,
   };

   int ret = anv_ioctl(device->fd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &args);
   if (ret == -1)
      return -1;

   return args.fd;
}

uint32_t
anv_gem_fd_to_handle(struct anv_device *device, int fd)
{
   struct drm_prime_handle args = {
      .fd = fd,
   };

   int ret = anv_ioctl(device->fd, DRM_IOCTL_PRIME_FD_TO_HANDLE, &args);
   if (ret == -1)
      return 0;

   return args.handle;
}
