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

#ifndef EGL_DRI2_INCLUDED
#define EGL_DRI2_INCLUDED

#include <stdint.h>

#ifdef HAVE_X11_PLATFORM
#include <xcb/xcb.h>
#include <xcb/dri2.h>
#include <xcb/xfixes.h>
#include <X11/Xlib-xcb.h>

#ifdef HAVE_DRI3
#include "loader_dri3_helper.h"
#endif
#endif

#ifdef HAVE_WAYLAND_PLATFORM
#include <wayland-client.h>
#include "wayland-egl-priv.h"
#endif

#include <GL/gl.h>
#include <GL/internal/dri_interface.h>

#ifdef HAVE_DRM_PLATFORM
#include <gbm_driint.h>
#endif

#ifdef HAVE_ANDROID_PLATFORM
#define LOG_TAG "EGL-DRI2"

#include <system/window.h>

#include "platform_android_gralloc_drm.h"
#include <hardware/gralloc.h>
#include <cutils/log.h>

#endif /* HAVE_ANDROID_PLATFORM */

#include "eglconfig.h"
#include "eglcontext.h"
#include "egldisplay.h"
#include "egldriver.h"
#include "eglcurrent.h"
#include "egllog.h"
#include "eglsurface.h"
#include "eglimage.h"
#include "eglsync.h"

struct wl_buffer;

struct dri2_egl_driver
{
   _EGLDriver base;

   void *handle;
   _EGLProc (*get_proc_address)(const char *procname);
   void (*glFlush)(void);
};

struct dri2_egl_display_vtbl {
   int (*authenticate)(_EGLDisplay *disp, uint32_t id);

   _EGLSurface* (*create_window_surface)(_EGLDriver *drv, _EGLDisplay *dpy,
                                         _EGLConfig *config,
                                         void *native_window,
                                         const EGLint *attrib_list);

   _EGLSurface* (*create_pixmap_surface)(_EGLDriver *drv, _EGLDisplay *dpy,
                                         _EGLConfig *config,
                                         void *native_pixmap,
                                         const EGLint *attrib_list);

   _EGLSurface* (*create_pbuffer_surface)(_EGLDriver *drv, _EGLDisplay *dpy,
                                          _EGLConfig *config,
                                          const EGLint *attrib_list);

   EGLBoolean (*destroy_surface)(_EGLDriver *drv, _EGLDisplay *dpy,
                                 _EGLSurface *surface);

   EGLBoolean (*swap_interval)(_EGLDriver *drv, _EGLDisplay *dpy,
                               _EGLSurface *surf, EGLint interval);

   _EGLImage* (*create_image)(_EGLDriver *drv, _EGLDisplay *dpy,
                              _EGLContext *ctx, EGLenum target,
                              EGLClientBuffer buffer,
                              const EGLint *attr_list);

   EGLBoolean (*swap_buffers)(_EGLDriver *drv, _EGLDisplay *dpy,
                              _EGLSurface *surf);

   EGLBoolean (*swap_buffers_with_damage)(_EGLDriver *drv, _EGLDisplay *dpy,
                                          _EGLSurface *surface,
                                          const EGLint *rects, EGLint n_rects);

   EGLBoolean (*swap_buffers_region)(_EGLDriver *drv, _EGLDisplay *dpy,
                                     _EGLSurface *surf, EGLint numRects,
                                     const EGLint *rects);

   EGLBoolean (*post_sub_buffer)(_EGLDriver *drv, _EGLDisplay *dpy,
                                 _EGLSurface *surf,
                                 EGLint x, EGLint y,
                                 EGLint width, EGLint height);

   EGLBoolean (*copy_buffers)(_EGLDriver *drv, _EGLDisplay *dpy,
                              _EGLSurface *surf, void *native_pixmap_target);

   EGLint (*query_buffer_age)(_EGLDriver *drv, _EGLDisplay *dpy,
                              _EGLSurface *surf);

   EGLBoolean (*query_surface)(_EGLDriver *drv, _EGLDisplay *dpy,
                               _EGLSurface *surf, EGLint attribute,
                               EGLint *value);

   struct wl_buffer* (*create_wayland_buffer_from_image)(
                        _EGLDriver *drv, _EGLDisplay *dpy, _EGLImage *img);

   EGLBoolean (*get_sync_values)(_EGLDisplay *display, _EGLSurface *surface,
                                 EGLuint64KHR *ust, EGLuint64KHR *msc,
                                 EGLuint64KHR *sbc);

   __DRIdrawable *(*get_dri_drawable)(_EGLSurface *surf);
};

struct dri2_egl_display
{
   const struct dri2_egl_display_vtbl *vtbl;

   int                       dri2_major;
   int                       dri2_minor;
   __DRIscreen              *dri_screen;
   int                       own_dri_screen;
   const __DRIconfig       **driver_configs;
   void                     *driver;
   const __DRIcoreExtension       *core;
   const __DRIimageDriverExtension *image_driver;
   const __DRIdri2Extension       *dri2;
   const __DRIswrastExtension     *swrast;
   const __DRI2flushExtension     *flush;
   const __DRItexBufferExtension  *tex_buffer;
   const __DRIimageExtension      *image;
   const __DRIrobustnessExtension *robustness;
   const __DRI2configQueryExtension *config;
   const __DRI2fenceExtension *fence;
   const __DRI2rendererQueryExtension *rendererQuery;
   const __DRI2interopExtension *interop;
   int                       fd;

   /* dri2_initialize/dri2_terminate increment/decrement this count, so does
    * dri2_make_current (tracks if there are active contexts/surfaces). */
   int                       ref_count;

   int                       own_device;
   int                       invalidate_available;
   int                       min_swap_interval;
   int                       max_swap_interval;
   int                       default_swap_interval;
#ifdef HAVE_DRM_PLATFORM
   struct gbm_dri_device    *gbm_dri;
#endif

   char                     *driver_name;

   const __DRIextension    **loader_extensions;
   const __DRIextension    **driver_extensions;

#ifdef HAVE_X11_PLATFORM
   xcb_connection_t         *conn;
   xcb_screen_t             *screen;
   int                      swap_available;
#ifdef HAVE_DRI3
   struct loader_dri3_extensions loader_dri3_ext;
#endif
#endif

#ifdef HAVE_WAYLAND_PLATFORM
   struct wl_display        *wl_dpy;
   struct wl_display        *wl_dpy_wrapper;
   struct wl_registry       *wl_registry;
   struct wl_drm            *wl_server_drm;
   struct wl_drm            *wl_drm;
   struct wl_shm            *wl_shm;
   struct wl_event_queue    *wl_queue;
   int                       authenticated;
   int                       formats;
   uint32_t                  capabilities;
   char                     *device_name;
#endif

#ifdef HAVE_ANDROID_PLATFORM
   const gralloc_module_t *gralloc;
#endif

   int                       is_render_node;
   int                       is_different_gpu;
};

struct dri2_egl_context
{
   _EGLContext   base;
   __DRIcontext *dri_context;
};

#ifdef HAVE_WAYLAND_PLATFORM
enum wayland_buffer_type {
   WL_BUFFER_FRONT,
   WL_BUFFER_BACK,
   WL_BUFFER_THIRD,
   WL_BUFFER_COUNT
};
#endif

struct dri2_egl_surface
{
   _EGLSurface          base;
   __DRIdrawable       *dri_drawable;
   __DRIbuffer          buffers[5];
   int                  buffer_count;
   int                  have_fake_front;

#ifdef HAVE_X11_PLATFORM
   xcb_drawable_t       drawable;
   xcb_xfixes_region_t  region;
   int                  depth;
   int                  bytes_per_pixel;
   xcb_gcontext_t       gc;
   xcb_gcontext_t       swapgc;
#endif

#ifdef HAVE_WAYLAND_PLATFORM
   struct wl_egl_window  *wl_win;
   int                    dx;
   int                    dy;
   struct wl_callback    *throttle_callback;
   int                    format;
#endif

#ifdef HAVE_DRM_PLATFORM
   struct gbm_dri_surface *gbm_surf;
#endif

#if defined(HAVE_WAYLAND_PLATFORM) || defined(HAVE_DRM_PLATFORM)
   __DRIbuffer           *dri_buffers[__DRI_BUFFER_COUNT];
   struct {
#ifdef HAVE_WAYLAND_PLATFORM
      struct wl_buffer   *wl_buffer;
      __DRIimage         *dri_image;
      /* for is_different_gpu case. NULL else */
      __DRIimage         *linear_copy;
      /* for swrast */
      void *data;
      int data_size;
#endif
#ifdef HAVE_DRM_PLATFORM
      struct gbm_bo       *bo;
#endif
      int                 locked;
      int                 age;
   } color_buffers[4], *back, *current;
#endif

#ifdef HAVE_ANDROID_PLATFORM
   struct ANativeWindow *window;
   struct ANativeWindowBuffer *buffer;
   __DRIimage *dri_image_back;
   __DRIimage *dri_image_front;

   /* EGL-owned buffers */
   __DRIbuffer           *local_buffers[__DRI_BUFFER_COUNT];
#endif

#if defined(HAVE_SURFACELESS_PLATFORM)
      __DRIimage           *front;
      unsigned int         visual;
#endif
};

struct dri2_egl_config
{
   _EGLConfig         base;
   const __DRIconfig *dri_single_config[2];
   const __DRIconfig *dri_double_config[2];
};

struct dri2_egl_image
{
   _EGLImage   base;
   __DRIimage *dri_image;
};

struct dri2_egl_sync {
   _EGLSync base;
   mtx_t mutex;
   cnd_t cond;
   int refcount;
   void *fence;
};

/* From xmlpool/options.h, user exposed so should be stable */
#define DRI_CONF_VBLANK_NEVER 0
#define DRI_CONF_VBLANK_DEF_INTERVAL_0 1
#define DRI_CONF_VBLANK_DEF_INTERVAL_1 2
#define DRI_CONF_VBLANK_ALWAYS_SYNC 3

/* standard typecasts */
_EGL_DRIVER_STANDARD_TYPECASTS(dri2_egl)
_EGL_DRIVER_TYPECAST(dri2_egl_image, _EGLImage, obj)
_EGL_DRIVER_TYPECAST(dri2_egl_sync, _EGLSync, obj)

extern const __DRIimageLookupExtension image_lookup_extension;
extern const __DRIuseInvalidateExtension use_invalidate;

EGLBoolean
dri2_load_driver(_EGLDisplay *disp);

/* Helper for platforms not using dri2_create_screen */
void
dri2_setup_screen(_EGLDisplay *disp);

EGLBoolean
dri2_load_driver_swrast(_EGLDisplay *disp);

EGLBoolean
dri2_load_driver_dri3(_EGLDisplay *disp);

EGLBoolean
dri2_create_screen(_EGLDisplay *disp);

__DRIdrawable *
dri2_surface_get_dri_drawable(_EGLSurface *surf);

__DRIimage *
dri2_lookup_egl_image(__DRIscreen *screen, void *image, void *data);

struct dri2_egl_config *
dri2_add_config(_EGLDisplay *disp, const __DRIconfig *dri_config, int id,
                EGLint surface_type, const EGLint *attr_list,
                const unsigned int *rgba_masks);

_EGLImage *
dri2_create_image_khr(_EGLDriver *drv, _EGLDisplay *disp,
                      _EGLContext *ctx, EGLenum target,
                      EGLClientBuffer buffer, const EGLint *attr_list);

_EGLImage *
dri2_create_image_dma_buf(_EGLDisplay *disp, _EGLContext *ctx,
                          EGLClientBuffer buffer, const EGLint *attr_list);

EGLBoolean
dri2_initialize_x11(_EGLDriver *drv, _EGLDisplay *disp);

EGLBoolean
dri2_initialize_drm(_EGLDriver *drv, _EGLDisplay *disp);

EGLBoolean
dri2_initialize_wayland(_EGLDriver *drv, _EGLDisplay *disp);

EGLBoolean
dri2_initialize_android(_EGLDriver *drv, _EGLDisplay *disp);

EGLBoolean
dri2_initialize_surfaceless(_EGLDriver *drv, _EGLDisplay *disp);

void
dri2_flush_drawable_for_swapbuffers(_EGLDisplay *disp, _EGLSurface *draw);

const __DRIconfig *
dri2_get_dri_config(struct dri2_egl_config *conf, EGLint surface_type,
                    EGLenum colorspace);

static inline void
dri2_set_WL_bind_wayland_display(_EGLDriver *drv, _EGLDisplay *disp)
{
#ifdef HAVE_WAYLAND_PLATFORM
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);

   (void) drv;

   if (dri2_dpy->device_name && dri2_dpy->image) {
       if (dri2_dpy->image->base.version >= 10 &&
           dri2_dpy->image->getCapabilities != NULL) {
           int capabilities;

           capabilities =
               dri2_dpy->image->getCapabilities(dri2_dpy->dri_screen);
           disp->Extensions.WL_bind_wayland_display =
               (capabilities & __DRI_IMAGE_CAP_GLOBAL_NAMES) != 0;
       } else {
           disp->Extensions.WL_bind_wayland_display = EGL_TRUE;
       }
   }
#endif
}

#endif /* EGL_DRI2_INCLUDED */
