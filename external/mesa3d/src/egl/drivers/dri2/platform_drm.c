/*
 * Copyright © 2011 Intel Corporation
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Kristian Høgsberg <krh@bitplanet.net>
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <xf86drm.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "egl_dri2.h"
#include "egl_dri2_fallbacks.h"
#include "loader.h"

static struct gbm_bo *
lock_front_buffer(struct gbm_surface *_surf)
{
   struct gbm_dri_surface *surf = (struct gbm_dri_surface *) _surf;
   struct dri2_egl_surface *dri2_surf = surf->dri_private;
   struct gbm_dri_device *device = (struct gbm_dri_device *) _surf->gbm;
   struct gbm_bo *bo;

   if (dri2_surf->current == NULL) {
      _eglError(EGL_BAD_SURFACE, "no front buffer");
      return NULL;
   }

   bo = dri2_surf->current->bo;

   if (device->dri2) {
      dri2_surf->current->locked = 1;
      dri2_surf->current = NULL;
   }

   return bo;
}

static void
release_buffer(struct gbm_surface *_surf, struct gbm_bo *bo)
{
   struct gbm_dri_surface *surf = (struct gbm_dri_surface *) _surf;
   struct dri2_egl_surface *dri2_surf = surf->dri_private;
   unsigned i;

   for (i = 0; i < ARRAY_SIZE(dri2_surf->color_buffers); i++) {
      if (dri2_surf->color_buffers[i].bo == bo) {
	 dri2_surf->color_buffers[i].locked = 0;
      }
   }
}

static int
has_free_buffers(struct gbm_surface *_surf)
{
   struct gbm_dri_surface *surf = (struct gbm_dri_surface *) _surf;
   struct dri2_egl_surface *dri2_surf = surf->dri_private;
   unsigned i;

   for (i = 0; i < ARRAY_SIZE(dri2_surf->color_buffers); i++)
      if (!dri2_surf->color_buffers[i].locked)
	 return 1;

   return 0;
}

static _EGLSurface *
dri2_drm_create_surface(_EGLDriver *drv, _EGLDisplay *disp, EGLint type,
                        _EGLConfig *conf, void *native_window,
                        const EGLint *attrib_list)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_config *dri2_conf = dri2_egl_config(conf);
   struct dri2_egl_surface *dri2_surf;
   struct gbm_surface *window = native_window;
   struct gbm_dri_surface *surf;
   const __DRIconfig *config;

   (void) drv;

   dri2_surf = calloc(1, sizeof *dri2_surf);
   if (!dri2_surf) {
      _eglError(EGL_BAD_ALLOC, "dri2_create_surface");
      return NULL;
   }

   if (!_eglInitSurface(&dri2_surf->base, disp, type, conf, attrib_list))
      goto cleanup_surf;

   switch (type) {
   case EGL_WINDOW_BIT:
      if (!window) {
         _eglError(EGL_BAD_NATIVE_WINDOW, "dri2_create_surface");
         goto cleanup_surf;
      }

      surf = gbm_dri_surface(window);
      dri2_surf->gbm_surf = surf;
      dri2_surf->base.Width =  surf->base.width;
      dri2_surf->base.Height = surf->base.height;
      surf->dri_private = dri2_surf;
      break;
   default:
      goto cleanup_surf;
   }

   config = dri2_get_dri_config(dri2_conf, EGL_WINDOW_BIT,
                                dri2_surf->base.GLColorspace);

   if (dri2_dpy->dri2) {
      dri2_surf->dri_drawable =
         dri2_dpy->dri2->createNewDrawable(dri2_dpy->dri_screen, config,
                                           dri2_surf->gbm_surf);

   } else {
      assert(dri2_dpy->swrast != NULL);

      dri2_surf->dri_drawable =
         dri2_dpy->swrast->createNewDrawable(dri2_dpy->dri_screen, config,
                                             dri2_surf->gbm_surf);

   }
   if (dri2_surf->dri_drawable == NULL) {
      _eglError(EGL_BAD_ALLOC, "createNewDrawable()");
      goto cleanup_surf;
   }

   return &dri2_surf->base;

 cleanup_surf:
   free(dri2_surf);

   return NULL;
}

static _EGLSurface *
dri2_drm_create_window_surface(_EGLDriver *drv, _EGLDisplay *disp,
                               _EGLConfig *conf, void *native_window,
                               const EGLint *attrib_list)
{
   return dri2_drm_create_surface(drv, disp, EGL_WINDOW_BIT, conf,
                                  native_window, attrib_list);
}

static _EGLSurface *
dri2_drm_create_pixmap_surface(_EGLDriver *drv, _EGLDisplay *disp,
                               _EGLConfig *conf, void *native_window,
                               const EGLint *attrib_list)
{
   /* From the EGL_MESA_platform_gbm spec, version 5:
    *
    *  It is not valid to call eglCreatePlatformPixmapSurfaceEXT with a <dpy>
    *  that belongs to the GBM platform. Any such call fails and generates
    *  EGL_BAD_PARAMETER.
    */
   _eglError(EGL_BAD_PARAMETER, "cannot create EGL pixmap surfaces on GBM");
   return NULL;
}

static EGLBoolean
dri2_drm_destroy_surface(_EGLDriver *drv, _EGLDisplay *disp, _EGLSurface *surf)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_surface *dri2_surf = dri2_egl_surface(surf);
   unsigned i;

   dri2_dpy->core->destroyDrawable(dri2_surf->dri_drawable);

   for (i = 0; i < ARRAY_SIZE(dri2_surf->color_buffers); i++) {
      if (dri2_surf->color_buffers[i].bo)
	 gbm_bo_destroy(dri2_surf->color_buffers[i].bo);
   }

   for (i = 0; i < __DRI_BUFFER_COUNT; i++) {
      if (dri2_surf->dri_buffers[i])
         dri2_dpy->dri2->releaseBuffer(dri2_dpy->dri_screen,
                                       dri2_surf->dri_buffers[i]);
   }

   free(surf);

   return EGL_TRUE;
}

static int
get_back_bo(struct dri2_egl_surface *dri2_surf)
{
   struct dri2_egl_display *dri2_dpy =
      dri2_egl_display(dri2_surf->base.Resource.Display);
   struct gbm_dri_surface *surf = dri2_surf->gbm_surf;
   int age = 0;
   unsigned i;

   if (dri2_surf->back == NULL) {
      for (i = 0; i < ARRAY_SIZE(dri2_surf->color_buffers); i++) {
	 if (!dri2_surf->color_buffers[i].locked &&
	      dri2_surf->color_buffers[i].age >= age) {
	    dri2_surf->back = &dri2_surf->color_buffers[i];
	    age = dri2_surf->color_buffers[i].age;
	 }
      }
   }

   if (dri2_surf->back == NULL)
      return -1;
   if (dri2_surf->back->bo == NULL)
      dri2_surf->back->bo = gbm_bo_create(&dri2_dpy->gbm_dri->base.base,
					  surf->base.width, surf->base.height,
					  surf->base.format, surf->base.flags);
   if (dri2_surf->back->bo == NULL)
      return -1;

   return 0;
}

static int
get_swrast_front_bo(struct dri2_egl_surface *dri2_surf)
{
   struct dri2_egl_display *dri2_dpy =
      dri2_egl_display(dri2_surf->base.Resource.Display);
   struct gbm_dri_surface *surf = dri2_surf->gbm_surf;

   if (dri2_surf->current == NULL) {
      assert(!dri2_surf->color_buffers[0].locked);
      dri2_surf->current = &dri2_surf->color_buffers[0];
   }

   if (dri2_surf->current->bo == NULL)
      dri2_surf->current->bo = gbm_bo_create(&dri2_dpy->gbm_dri->base.base,
                                             surf->base.width, surf->base.height,
                                             surf->base.format, surf->base.flags);
   if (dri2_surf->current->bo == NULL)
      return -1;

   return 0;
}

static void
back_bo_to_dri_buffer(struct dri2_egl_surface *dri2_surf, __DRIbuffer *buffer)
{
   struct dri2_egl_display *dri2_dpy =
      dri2_egl_display(dri2_surf->base.Resource.Display);
   struct gbm_dri_bo *bo;
   int name, pitch;

   bo = (struct gbm_dri_bo *) dri2_surf->back->bo;

   dri2_dpy->image->queryImage(bo->image, __DRI_IMAGE_ATTRIB_NAME, &name);
   dri2_dpy->image->queryImage(bo->image, __DRI_IMAGE_ATTRIB_STRIDE, &pitch);

   buffer->attachment = __DRI_BUFFER_BACK_LEFT;
   buffer->name = name;
   buffer->pitch = pitch;
   buffer->cpp = 4;
   buffer->flags = 0;
}

static int
get_aux_bo(struct dri2_egl_surface *dri2_surf,
	   unsigned int attachment, unsigned int format, __DRIbuffer *buffer)
{
   struct dri2_egl_display *dri2_dpy =
      dri2_egl_display(dri2_surf->base.Resource.Display);
   __DRIbuffer *b = dri2_surf->dri_buffers[attachment];

   if (b == NULL) {
      b = dri2_dpy->dri2->allocateBuffer(dri2_dpy->dri_screen,
					 attachment, format,
					 dri2_surf->base.Width,
					 dri2_surf->base.Height);
      dri2_surf->dri_buffers[attachment] = b;
   }
   if (b == NULL)
      return -1;

   memcpy(buffer, b, sizeof *buffer);

   return 0;
}

static __DRIbuffer *
dri2_drm_get_buffers_with_format(__DRIdrawable *driDrawable,
			     int *width, int *height,
			     unsigned int *attachments, int count,
			     int *out_count, void *loaderPrivate)
{
   struct dri2_egl_surface *dri2_surf = loaderPrivate;
   int i, j;

   dri2_surf->buffer_count = 0;
   for (i = 0, j = 0; i < 2 * count; i += 2, j++) {
      assert(attachments[i] < __DRI_BUFFER_COUNT);
      assert(dri2_surf->buffer_count < 5);

      switch (attachments[i]) {
      case __DRI_BUFFER_BACK_LEFT:
	 if (get_back_bo(dri2_surf) < 0) {
	    _eglError(EGL_BAD_ALLOC, "failed to allocate color buffer");
	    return NULL;
	 }
         back_bo_to_dri_buffer(dri2_surf, &dri2_surf->buffers[j]);
	 break;
      default:
	 if (get_aux_bo(dri2_surf, attachments[i], attachments[i + 1],
			&dri2_surf->buffers[j]) < 0) {
	    _eglError(EGL_BAD_ALLOC, "failed to allocate aux buffer");
	    return NULL;
	 }
	 break;
      }
   }

   *out_count = j;
   if (j == 0)
      return NULL;

   *width = dri2_surf->base.Width;
   *height = dri2_surf->base.Height;

   return dri2_surf->buffers;
}

static __DRIbuffer *
dri2_drm_get_buffers(__DRIdrawable * driDrawable,
                     int *width, int *height,
                     unsigned int *attachments, int count,
                     int *out_count, void *loaderPrivate)
{
   unsigned int *attachments_with_format;
   __DRIbuffer *buffer;
   const unsigned int format = 32;
   int i;

   attachments_with_format = calloc(count, 2 * sizeof(unsigned int));
   if (!attachments_with_format) {
      *out_count = 0;
      return NULL;
   }

   for (i = 0; i < count; ++i) {
      attachments_with_format[2*i] = attachments[i];
      attachments_with_format[2*i + 1] = format;
   }

   buffer =
      dri2_drm_get_buffers_with_format(driDrawable,
                                       width, height,
                                       attachments_with_format, count,
                                       out_count, loaderPrivate);

   free(attachments_with_format);

   return buffer;
}

static int
dri2_drm_image_get_buffers(__DRIdrawable *driDrawable,
                           unsigned int format,
                           uint32_t *stamp,
                           void *loaderPrivate,
                           uint32_t buffer_mask,
                           struct __DRIimageList *buffers)
{
   struct dri2_egl_surface *dri2_surf = loaderPrivate;
   struct gbm_dri_bo *bo;

   if (get_back_bo(dri2_surf) < 0)
      return 0;

   bo = (struct gbm_dri_bo *) dri2_surf->back->bo;
   buffers->image_mask = __DRI_IMAGE_BUFFER_BACK;
   buffers->back = bo->image;

   return 1;
}

static void
dri2_drm_flush_front_buffer(__DRIdrawable * driDrawable, void *loaderPrivate)
{
   (void) driDrawable;
   (void) loaderPrivate;
}

static EGLBoolean
dri2_drm_swap_buffers(_EGLDriver *drv, _EGLDisplay *disp, _EGLSurface *draw)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_surface *dri2_surf = dri2_egl_surface(draw);
   unsigned i;

   if (dri2_dpy->swrast) {
      dri2_dpy->core->swapBuffers(dri2_surf->dri_drawable);
   } else {
      if (dri2_surf->base.Type == EGL_WINDOW_BIT) {
         if (dri2_surf->current)
            _eglError(EGL_BAD_SURFACE, "dri2_swap_buffers");
         for (i = 0; i < ARRAY_SIZE(dri2_surf->color_buffers); i++)
            if (dri2_surf->color_buffers[i].age > 0)
               dri2_surf->color_buffers[i].age++;

         /* Make sure we have a back buffer in case we're swapping without
          * ever rendering. */
         if (get_back_bo(dri2_surf) < 0) {
            _eglError(EGL_BAD_ALLOC, "dri2_swap_buffers");
            return EGL_FALSE;
         }

         dri2_surf->current = dri2_surf->back;
         dri2_surf->current->age = 1;
         dri2_surf->back = NULL;
      }

      dri2_flush_drawable_for_swapbuffers(disp, draw);
      dri2_dpy->flush->invalidate(dri2_surf->dri_drawable);
   }

   return EGL_TRUE;
}

static EGLint
dri2_drm_query_buffer_age(_EGLDriver *drv,
                          _EGLDisplay *disp, _EGLSurface *surface)
{
   struct dri2_egl_surface *dri2_surf = dri2_egl_surface(surface);

   if (get_back_bo(dri2_surf) < 0) {
      _eglError(EGL_BAD_ALLOC, "dri2_query_buffer_age");
      return 0;
   }

   return dri2_surf->back->age;
}

static _EGLImage *
dri2_drm_create_image_khr_pixmap(_EGLDisplay *disp, _EGLContext *ctx,
                                 EGLClientBuffer buffer, const EGLint *attr_list)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct gbm_dri_bo *dri_bo = gbm_dri_bo((struct gbm_bo *) buffer);
   struct dri2_egl_image *dri2_img;

   dri2_img = malloc(sizeof *dri2_img);
   if (!dri2_img) {
      _eglError(EGL_BAD_ALLOC, "dri2_create_image_khr_pixmap");
      return NULL;
   }

   if (!_eglInitImage(&dri2_img->base, disp)) {
      free(dri2_img);
      return NULL;
   }

   dri2_img->dri_image = dri2_dpy->image->dupImage(dri_bo->image, dri2_img);
   if (dri2_img->dri_image == NULL) {
      free(dri2_img);
      _eglError(EGL_BAD_ALLOC, "dri2_create_image_khr_pixmap");
      return NULL;
   }

   return &dri2_img->base;
}

static _EGLImage *
dri2_drm_create_image_khr(_EGLDriver *drv, _EGLDisplay *disp,
                          _EGLContext *ctx, EGLenum target,
                          EGLClientBuffer buffer, const EGLint *attr_list)
{
   (void) drv;

   switch (target) {
   case EGL_NATIVE_PIXMAP_KHR:
      return dri2_drm_create_image_khr_pixmap(disp, ctx, buffer, attr_list);
   default:
      return dri2_create_image_khr(drv, disp, ctx, target, buffer, attr_list);
   }
}

static int
dri2_drm_authenticate(_EGLDisplay *disp, uint32_t id)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);

   return drmAuthMagic(dri2_dpy->fd, id);
}

static void
swrast_put_image2(__DRIdrawable *driDrawable,
                  int            op,
                  int            x,
                  int            y,
                  int            width,
                  int            height,
                  int            stride,
                  char          *data,
                  void          *loaderPrivate)
{
   struct dri2_egl_surface *dri2_surf = loaderPrivate;
   int internal_stride, i;
   struct gbm_dri_bo *bo;

   if (op != __DRI_SWRAST_IMAGE_OP_DRAW &&
       op != __DRI_SWRAST_IMAGE_OP_SWAP)
      return;

   if (get_swrast_front_bo(dri2_surf) < 0)
      return;

   bo = gbm_dri_bo(dri2_surf->current->bo);
   if (gbm_dri_bo_map_dumb(bo) == NULL)
      return;

   internal_stride = bo->base.base.stride;

   for (i = 0; i < height; i++) {
      memcpy(bo->map + (x + i) * internal_stride + y,
             data + i * stride, stride);
   }

   gbm_dri_bo_unmap_dumb(bo);
}

static void
swrast_get_image(__DRIdrawable *driDrawable,
                 int            x,
                 int            y,
                 int            width,
                 int            height,
                 char          *data,
                 void          *loaderPrivate)
{
   struct dri2_egl_surface *dri2_surf = loaderPrivate;
   int internal_stride, stride, i;
   struct gbm_dri_bo *bo;

   if (get_swrast_front_bo(dri2_surf) < 0)
      return;

   bo = gbm_dri_bo(dri2_surf->current->bo);
   if (gbm_dri_bo_map_dumb(bo) == NULL)
      return;

   internal_stride = bo->base.base.stride;
   stride = width * 4;

   for (i = 0; i < height; i++) {
      memcpy(data + i * stride,
             bo->map + (x + i) * internal_stride + y, stride);
   }

   gbm_dri_bo_unmap_dumb(bo);
}

static EGLBoolean
drm_add_configs_for_visuals(_EGLDriver *drv, _EGLDisplay *disp)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   static const struct {
      int format;
      unsigned int red_mask;
      unsigned int alpha_mask;
   } visuals[] = {
      { GBM_FORMAT_XRGB2101010, 0x3ff00000, 0x00000000 },
      { GBM_FORMAT_ARGB2101010, 0x3ff00000, 0xc0000000 },
      { GBM_FORMAT_XRGB8888,    0x00ff0000, 0x00000000 },
      { GBM_FORMAT_ARGB8888,    0x00ff0000, 0xff000000 },
      { GBM_FORMAT_RGB565,      0x0000f800, 0x00000000 },
   };
   EGLint attr_list[] = {
      EGL_NATIVE_VISUAL_ID, 0,
      EGL_NONE,
   };
   unsigned int format_count[ARRAY_SIZE(visuals)] = { 0 };
   unsigned int count, i, j;

   count = 0;
   for (i = 0; dri2_dpy->driver_configs[i]; i++) {
      unsigned int red, alpha;

      dri2_dpy->core->getConfigAttrib(dri2_dpy->driver_configs[i],
                                      __DRI_ATTRIB_RED_MASK, &red);
      dri2_dpy->core->getConfigAttrib(dri2_dpy->driver_configs[i],
                                      __DRI_ATTRIB_ALPHA_MASK, &alpha);

      for (j = 0; j < ARRAY_SIZE(visuals); j++) {
         struct dri2_egl_config *dri2_conf;

         if (visuals[j].red_mask != red || visuals[j].alpha_mask != alpha)
            continue;

         attr_list[1] = visuals[j].format;

         dri2_conf = dri2_add_config(disp, dri2_dpy->driver_configs[i],
               count + 1, EGL_WINDOW_BIT, attr_list, NULL);
         if (dri2_conf) {
            count++;
            format_count[j]++;
         }
      }
   }

   for (i = 0; i < ARRAY_SIZE(format_count); i++) {
      if (!format_count[i]) {
         _eglLog(_EGL_DEBUG, "No DRI config supports native format 0x%x",
                 visuals[i].format);
      }
   }

   return (count != 0);
}

static struct dri2_egl_display_vtbl dri2_drm_display_vtbl = {
   .authenticate = dri2_drm_authenticate,
   .create_window_surface = dri2_drm_create_window_surface,
   .create_pixmap_surface = dri2_drm_create_pixmap_surface,
   .create_pbuffer_surface = dri2_fallback_create_pbuffer_surface,
   .destroy_surface = dri2_drm_destroy_surface,
   .create_image = dri2_drm_create_image_khr,
   .swap_interval = dri2_fallback_swap_interval,
   .swap_buffers = dri2_drm_swap_buffers,
   .swap_buffers_with_damage = dri2_fallback_swap_buffers_with_damage,
   .swap_buffers_region = dri2_fallback_swap_buffers_region,
   .post_sub_buffer = dri2_fallback_post_sub_buffer,
   .copy_buffers = dri2_fallback_copy_buffers,
   .query_buffer_age = dri2_drm_query_buffer_age,
   .create_wayland_buffer_from_image = dri2_fallback_create_wayland_buffer_from_image,
   .get_sync_values = dri2_fallback_get_sync_values,
   .get_dri_drawable = dri2_surface_get_dri_drawable,
};

EGLBoolean
dri2_initialize_drm(_EGLDriver *drv, _EGLDisplay *disp)
{
   struct dri2_egl_display *dri2_dpy;
   struct gbm_device *gbm;
   const char *err;
   int fd = -1;

   loader_set_logger(_eglLog);

   dri2_dpy = calloc(1, sizeof *dri2_dpy);
   if (!dri2_dpy)
      return _eglError(EGL_BAD_ALLOC, "eglInitialize");

   disp->DriverData = (void *) dri2_dpy;

   gbm = disp->PlatformDisplay;
   if (gbm == NULL) {
      char buf[64];
      int n = snprintf(buf, sizeof(buf), DRM_DEV_NAME, DRM_DIR_NAME, 0);
      if (n != -1 && n < sizeof(buf))
         fd = loader_open_device(buf);
      if (fd < 0)
         fd = loader_open_device("/dev/dri/card0");
      dri2_dpy->own_device = 1;
      gbm = gbm_create_device(fd);
      if (gbm == NULL) {
         err = "DRI2: failed to create gbm device";
         goto cleanup;
      }
   } else {
      fd = fcntl(gbm_device_get_fd(gbm), F_DUPFD_CLOEXEC, 3);
      if (fd < 0) {
         err = "DRI2: failed to fcntl() existing gbm device";
         goto cleanup;
      }
   }

   if (strcmp(gbm_device_get_backend_name(gbm), "drm") != 0) {
      err = "DRI2: gbm device using incorrect/incompatible backend";
      goto cleanup;
   }

   dri2_dpy->gbm_dri = gbm_dri_device(gbm);
   if (dri2_dpy->gbm_dri->base.type != GBM_DRM_DRIVER_TYPE_DRI) {
      err = "DRI2: gbm device using incorrect/incompatible type";
      goto cleanup;
   }

   dri2_dpy->fd = fd;
   dri2_dpy->driver_name = strdup(dri2_dpy->gbm_dri->base.driver_name);

   dri2_dpy->dri_screen = dri2_dpy->gbm_dri->screen;
   dri2_dpy->core = dri2_dpy->gbm_dri->core;
   dri2_dpy->dri2 = dri2_dpy->gbm_dri->dri2;
   dri2_dpy->fence = dri2_dpy->gbm_dri->fence;
   dri2_dpy->image = dri2_dpy->gbm_dri->image;
   dri2_dpy->flush = dri2_dpy->gbm_dri->flush;
   dri2_dpy->swrast = dri2_dpy->gbm_dri->swrast;
   dri2_dpy->driver_configs = dri2_dpy->gbm_dri->driver_configs;
   dri2_dpy->interop = dri2_dpy->gbm_dri->interop;

   dri2_dpy->gbm_dri->lookup_image = dri2_lookup_egl_image;
   dri2_dpy->gbm_dri->lookup_user_data = disp;

   dri2_dpy->gbm_dri->get_buffers = dri2_drm_get_buffers;
   dri2_dpy->gbm_dri->flush_front_buffer = dri2_drm_flush_front_buffer;
   dri2_dpy->gbm_dri->get_buffers_with_format = dri2_drm_get_buffers_with_format;
   dri2_dpy->gbm_dri->image_get_buffers = dri2_drm_image_get_buffers;
   dri2_dpy->gbm_dri->swrast_put_image2 = swrast_put_image2;
   dri2_dpy->gbm_dri->swrast_get_image = swrast_get_image;

   dri2_dpy->gbm_dri->base.base.surface_lock_front_buffer = lock_front_buffer;
   dri2_dpy->gbm_dri->base.base.surface_release_buffer = release_buffer;
   dri2_dpy->gbm_dri->base.base.surface_has_free_buffers = has_free_buffers;

   dri2_setup_screen(disp);

   if (!drm_add_configs_for_visuals(drv, disp)) {
      err = "DRI2: failed to add configs";
      goto cleanup;
   }

   disp->Extensions.KHR_image_pixmap = EGL_TRUE;
   if (dri2_dpy->dri2)
      disp->Extensions.EXT_buffer_age = EGL_TRUE;

#ifdef HAVE_WAYLAND_PLATFORM
   dri2_dpy->device_name = loader_get_device_name_for_fd(dri2_dpy->fd);
#endif
   dri2_set_WL_bind_wayland_display(drv, disp);

   /* Fill vtbl last to prevent accidentally calling virtual function during
    * initialization.
    */
   dri2_dpy->vtbl = &dri2_drm_display_vtbl;

   return EGL_TRUE;

cleanup:
   if (fd >= 0)
      close(fd);

   free(dri2_dpy);
   disp->DriverData = NULL;
   return _eglError(EGL_NOT_INITIALIZED, err);
}
