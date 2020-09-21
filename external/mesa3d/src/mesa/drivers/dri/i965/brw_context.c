/*
 Copyright 2003 VMware, Inc.
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics to
 develop this 3D driver.

 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:

 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keithw@vmware.com>
  */


#include "compiler/nir/nir.h"
#include "main/api_exec.h"
#include "main/context.h"
#include "main/fbobject.h"
#include "main/extensions.h"
#include "main/imports.h"
#include "main/macros.h"
#include "main/points.h"
#include "main/version.h"
#include "main/vtxfmt.h"
#include "main/texobj.h"
#include "main/framebuffer.h"

#include "vbo/vbo_context.h"

#include "drivers/common/driverfuncs.h"
#include "drivers/common/meta.h"
#include "utils.h"

#include "brw_context.h"
#include "brw_defines.h"
#include "brw_blorp.h"
#include "brw_compiler.h"
#include "brw_draw.h"
#include "brw_state.h"

#include "intel_batchbuffer.h"
#include "intel_buffer_objects.h"
#include "intel_buffers.h"
#include "intel_fbo.h"
#include "intel_mipmap_tree.h"
#include "intel_pixel.h"
#include "intel_image.h"
#include "intel_tex.h"
#include "intel_tex_obj.h"

#include "swrast_setup/swrast_setup.h"
#include "tnl/tnl.h"
#include "tnl/t_pipeline.h"
#include "util/ralloc.h"
#include "util/debug.h"
#include "isl/isl.h"

/***************************************
 * Mesa's Driver Functions
 ***************************************/

const char *const brw_vendor_string = "Intel Open Source Technology Center";

static const char *
get_bsw_model(const struct intel_screen *screen)
{
   switch (screen->eu_total) {
   case 16:
      return "405";
   case 12:
      return "400";
   default:
      return "   ";
   }
}

const char *
brw_get_renderer_string(const struct intel_screen *screen)
{
   const char *chipset;
   static char buffer[128];
   char *bsw = NULL;

   switch (screen->deviceID) {
#undef CHIPSET
#define CHIPSET(id, symbol, str) case id: chipset = str; break;
#include "pci_ids/i965_pci_ids.h"
   default:
      chipset = "Unknown Intel Chipset";
      break;
   }

   /* Braswell branding is funny, so we have to fix it up here */
   if (screen->deviceID == 0x22B1) {
      bsw = strdup(chipset);
      char *needle = strstr(bsw, "XXX");
      if (needle) {
         memcpy(needle, get_bsw_model(screen), 3);
         chipset = bsw;
      }
   }

   (void) driGetRendererString(buffer, chipset, 0);
   free(bsw);
   return buffer;
}

static const GLubyte *
intel_get_string(struct gl_context * ctx, GLenum name)
{
   const struct brw_context *const brw = brw_context(ctx);

   switch (name) {
   case GL_VENDOR:
      return (GLubyte *) brw_vendor_string;

   case GL_RENDERER:
      return
         (GLubyte *) brw_get_renderer_string(brw->screen);

   default:
      return NULL;
   }
}

static void
intel_viewport(struct gl_context *ctx)
{
   struct brw_context *brw = brw_context(ctx);
   __DRIcontext *driContext = brw->driContext;

   if (_mesa_is_winsys_fbo(ctx->DrawBuffer)) {
      if (driContext->driDrawablePriv)
         dri2InvalidateDrawable(driContext->driDrawablePriv);
      if (driContext->driReadablePriv)
         dri2InvalidateDrawable(driContext->driReadablePriv);
   }
}

static void
intel_update_framebuffer(struct gl_context *ctx,
                         struct gl_framebuffer *fb)
{
   struct brw_context *brw = brw_context(ctx);

   /* Quantize the derived default number of samples
    */
   fb->DefaultGeometry._NumSamples =
      intel_quantize_num_samples(brw->screen,
                                 fb->DefaultGeometry.NumSamples);
}

static bool
intel_disable_rb_aux_buffer(struct brw_context *brw, const drm_intel_bo *bo)
{
   const struct gl_framebuffer *fb = brw->ctx.DrawBuffer;
   bool found = false;

   for (unsigned i = 0; i < fb->_NumColorDrawBuffers; i++) {
      const struct intel_renderbuffer *irb =
         intel_renderbuffer(fb->_ColorDrawBuffers[i]);

      if (irb && irb->mt->bo == bo) {
         found = brw->draw_aux_buffer_disabled[i] = true;
      }
   }

   return found;
}

/* On Gen9 color buffers may be compressed by the hardware (lossless
 * compression). There are, however, format restrictions and care needs to be
 * taken that the sampler engine is capable for re-interpreting a buffer with
 * format different the buffer was originally written with.
 *
 * For example, SRGB formats are not compressible and the sampler engine isn't
 * capable of treating RGBA_UNORM as SRGB_ALPHA. In such a case the underlying
 * color buffer needs to be resolved so that the sampling surface can be
 * sampled as non-compressed (i.e., without the auxiliary MCS buffer being
 * set).
 */
static bool
intel_texture_view_requires_resolve(struct brw_context *brw,
                                    struct intel_texture_object *intel_tex)
{
   if (brw->gen < 9 ||
       !intel_miptree_is_lossless_compressed(brw, intel_tex->mt))
     return false;

   const uint32_t brw_format = brw_format_for_mesa_format(intel_tex->_Format);

   if (isl_format_supports_lossless_compression(&brw->screen->devinfo,
                                                brw_format))
      return false;

   perf_debug("Incompatible sampling format (%s) for rbc (%s)\n",
              _mesa_get_format_name(intel_tex->_Format),
              _mesa_get_format_name(intel_tex->mt->format));

   if (intel_disable_rb_aux_buffer(brw, intel_tex->mt->bo))
      perf_debug("Sampling renderbuffer with non-compressible format - "
                 "turning off compression");

   return true;
}

static void
intel_update_state(struct gl_context * ctx, GLuint new_state)
{
   struct brw_context *brw = brw_context(ctx);
   struct intel_texture_object *tex_obj;
   struct intel_renderbuffer *depth_irb;

   if (ctx->swrast_context)
      _swrast_InvalidateState(ctx, new_state);
   _vbo_InvalidateState(ctx, new_state);

   brw->NewGLState |= new_state;

   _mesa_unlock_context_textures(ctx);

   /* Resolve the depth buffer's HiZ buffer. */
   depth_irb = intel_get_renderbuffer(ctx->DrawBuffer, BUFFER_DEPTH);
   if (depth_irb)
      intel_renderbuffer_resolve_hiz(brw, depth_irb);

   memset(brw->draw_aux_buffer_disabled, 0,
          sizeof(brw->draw_aux_buffer_disabled));

   /* Resolve depth buffer and render cache of each enabled texture. */
   int maxEnabledUnit = ctx->Texture._MaxEnabledTexImageUnit;
   for (int i = 0; i <= maxEnabledUnit; i++) {
      if (!ctx->Texture.Unit[i]._Current)
	 continue;
      tex_obj = intel_texture_object(ctx->Texture.Unit[i]._Current);
      if (!tex_obj || !tex_obj->mt)
	 continue;
      if (intel_miptree_sample_with_hiz(brw, tex_obj->mt))
         intel_miptree_all_slices_resolve_hiz(brw, tex_obj->mt);
      else
         intel_miptree_all_slices_resolve_depth(brw, tex_obj->mt);
      /* Sampling engine understands lossless compression and resolving
       * those surfaces should be skipped for performance reasons.
       */
      const int flags = intel_texture_view_requires_resolve(brw, tex_obj) ?
                           0 : INTEL_MIPTREE_IGNORE_CCS_E;
      intel_miptree_all_slices_resolve_color(brw, tex_obj->mt, flags);
      brw_render_cache_set_check_flush(brw, tex_obj->mt->bo);

      if (tex_obj->base.StencilSampling ||
          tex_obj->mt->format == MESA_FORMAT_S_UINT8) {
         intel_update_r8stencil(brw, tex_obj->mt);
      }
   }

   /* Resolve color for each active shader image. */
   for (unsigned i = 0; i < MESA_SHADER_STAGES; i++) {
      const struct gl_linked_shader *shader =
         ctx->_Shader->CurrentProgram[i] ?
            ctx->_Shader->CurrentProgram[i]->_LinkedShaders[i] : NULL;

      if (unlikely(shader && shader->Program->info.num_images)) {
         for (unsigned j = 0; j < shader->Program->info.num_images; j++) {
            struct gl_image_unit *u =
               &ctx->ImageUnits[shader->Program->sh.ImageUnits[j]];
            tex_obj = intel_texture_object(u->TexObj);

            if (tex_obj && tex_obj->mt) {
               /* Access to images is implemented using indirect messages
                * against data port. Normal render target write understands
                * lossless compression but unfortunately the typed/untyped
                * read/write interface doesn't. Therefore even lossless
                * compressed surfaces need to be resolved prior to accessing
                * them. Hence skip setting INTEL_MIPTREE_IGNORE_CCS_E.
                */
               intel_miptree_all_slices_resolve_color(brw, tex_obj->mt, 0);

               if (intel_miptree_is_lossless_compressed(brw, tex_obj->mt) &&
                   intel_disable_rb_aux_buffer(brw, tex_obj->mt->bo)) {
                  perf_debug("Using renderbuffer as shader image - turning "
                             "off lossless compression");
               }

               brw_render_cache_set_check_flush(brw, tex_obj->mt->bo);
            }
         }
      }
   }

   /* Resolve color buffers for non-coherent framebuffer fetch. */
   if (!ctx->Extensions.MESA_shader_framebuffer_fetch &&
       ctx->FragmentProgram._Current &&
       ctx->FragmentProgram._Current->info.outputs_read) {
      const struct gl_framebuffer *fb = ctx->DrawBuffer;

      for (unsigned i = 0; i < fb->_NumColorDrawBuffers; i++) {
         const struct intel_renderbuffer *irb =
            intel_renderbuffer(fb->_ColorDrawBuffers[i]);

         if (irb &&
             intel_miptree_resolve_color(
                brw, irb->mt, irb->mt_level, irb->mt_layer, irb->layer_count,
                INTEL_MIPTREE_IGNORE_CCS_E))
            brw_render_cache_set_check_flush(brw, irb->mt->bo);
      }
   }

   /* If FRAMEBUFFER_SRGB is used on Gen9+ then we need to resolve any of the
    * single-sampled color renderbuffers because the CCS buffer isn't
    * supported for SRGB formats. This only matters if FRAMEBUFFER_SRGB is
    * enabled because otherwise the surface state will be programmed with the
    * linear equivalent format anyway.
    */
   if (brw->gen >= 9 && ctx->Color.sRGBEnabled) {
      struct gl_framebuffer *fb = ctx->DrawBuffer;
      for (int i = 0; i < fb->_NumColorDrawBuffers; i++) {
         struct gl_renderbuffer *rb = fb->_ColorDrawBuffers[i];

         if (rb == NULL)
            continue;

         struct intel_renderbuffer *irb = intel_renderbuffer(rb);
         struct intel_mipmap_tree *mt = irb->mt;

         if (mt == NULL ||
             mt->num_samples > 1 ||
             _mesa_get_srgb_format_linear(mt->format) == mt->format)
               continue;

         /* Lossless compression is not supported for SRGB formats, it
          * should be impossible to get here with such surfaces.
          */
         assert(!intel_miptree_is_lossless_compressed(brw, mt));
         intel_miptree_all_slices_resolve_color(brw, mt, 0);
         brw_render_cache_set_check_flush(brw, mt->bo);
      }
   }

   _mesa_lock_context_textures(ctx);

   if (new_state & _NEW_BUFFERS) {
      intel_update_framebuffer(ctx, ctx->DrawBuffer);
      if (ctx->DrawBuffer != ctx->ReadBuffer)
         intel_update_framebuffer(ctx, ctx->ReadBuffer);
   }
}

#define flushFront(screen)      ((screen)->image.loader ? (screen)->image.loader->flushFrontBuffer : (screen)->dri2.loader->flushFrontBuffer)

static void
intel_flush_front(struct gl_context *ctx)
{
   struct brw_context *brw = brw_context(ctx);
   __DRIcontext *driContext = brw->driContext;
   __DRIdrawable *driDrawable = driContext->driDrawablePriv;
   __DRIscreen *const dri_screen = brw->screen->driScrnPriv;

   if (brw->front_buffer_dirty && _mesa_is_winsys_fbo(ctx->DrawBuffer)) {
      if (flushFront(dri_screen) && driDrawable &&
          driDrawable->loaderPrivate) {

         /* Resolve before flushing FAKE_FRONT_LEFT to FRONT_LEFT.
          *
          * This potentially resolves both front and back buffer. It
          * is unnecessary to resolve the back, but harms nothing except
          * performance. And no one cares about front-buffer render
          * performance.
          */
         intel_resolve_for_dri2_flush(brw, driDrawable);
         intel_batchbuffer_flush(brw);

         flushFront(dri_screen)(driDrawable, driDrawable->loaderPrivate);

         /* We set the dirty bit in intel_prepare_render() if we're
          * front buffer rendering once we get there.
          */
         brw->front_buffer_dirty = false;
      }
   }
}

static void
intel_glFlush(struct gl_context *ctx)
{
   struct brw_context *brw = brw_context(ctx);

   intel_batchbuffer_flush(brw);
   intel_flush_front(ctx);

   brw->need_flush_throttle = true;
}

static void
intel_finish(struct gl_context * ctx)
{
   struct brw_context *brw = brw_context(ctx);

   intel_glFlush(ctx);

   if (brw->batch.last_bo)
      drm_intel_bo_wait_rendering(brw->batch.last_bo);
}

static void
brw_init_driver_functions(struct brw_context *brw,
                          struct dd_function_table *functions)
{
   _mesa_init_driver_functions(functions);

   /* GLX uses DRI2 invalidate events to handle window resizing.
    * Unfortunately, EGL does not - libEGL is written in XCB (not Xlib),
    * which doesn't provide a mechanism for snooping the event queues.
    *
    * So EGL still relies on viewport hacks to handle window resizing.
    * This should go away with DRI3000.
    */
   if (!brw->driContext->driScreenPriv->dri2.useInvalidate)
      functions->Viewport = intel_viewport;

   functions->Flush = intel_glFlush;
   functions->Finish = intel_finish;
   functions->GetString = intel_get_string;
   functions->UpdateState = intel_update_state;

   intelInitTextureFuncs(functions);
   intelInitTextureImageFuncs(functions);
   intelInitTextureSubImageFuncs(functions);
   intelInitTextureCopyImageFuncs(functions);
   intelInitCopyImageFuncs(functions);
   intelInitClearFuncs(functions);
   intelInitBufferFuncs(functions);
   intelInitPixelFuncs(functions);
   intelInitBufferObjectFuncs(functions);
   brw_init_syncobj_functions(functions);
   brw_init_object_purgeable_functions(functions);

   brwInitFragProgFuncs( functions );
   brw_init_common_queryobj_functions(functions);
   if (brw->gen >= 8 || brw->is_haswell)
      hsw_init_queryobj_functions(functions);
   else if (brw->gen >= 6)
      gen6_init_queryobj_functions(functions);
   else
      gen4_init_queryobj_functions(functions);
   brw_init_compute_functions(functions);
   if (brw->gen >= 7)
      brw_init_conditional_render_functions(functions);

   functions->QueryInternalFormat = brw_query_internal_format;

   functions->NewTransformFeedback = brw_new_transform_feedback;
   functions->DeleteTransformFeedback = brw_delete_transform_feedback;
   if (can_do_mi_math_and_lrr(brw->screen)) {
      functions->BeginTransformFeedback = hsw_begin_transform_feedback;
      functions->EndTransformFeedback = hsw_end_transform_feedback;
      functions->PauseTransformFeedback = hsw_pause_transform_feedback;
      functions->ResumeTransformFeedback = hsw_resume_transform_feedback;
   } else if (brw->gen >= 7) {
      functions->BeginTransformFeedback = gen7_begin_transform_feedback;
      functions->EndTransformFeedback = gen7_end_transform_feedback;
      functions->PauseTransformFeedback = gen7_pause_transform_feedback;
      functions->ResumeTransformFeedback = gen7_resume_transform_feedback;
      functions->GetTransformFeedbackVertexCount =
         brw_get_transform_feedback_vertex_count;
   } else {
      functions->BeginTransformFeedback = brw_begin_transform_feedback;
      functions->EndTransformFeedback = brw_end_transform_feedback;
   }

   if (brw->gen >= 6)
      functions->GetSamplePosition = gen6_get_sample_position;
}

static void
brw_initialize_context_constants(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   const struct brw_compiler *compiler = brw->screen->compiler;

   const bool stage_exists[MESA_SHADER_STAGES] = {
      [MESA_SHADER_VERTEX] = true,
      [MESA_SHADER_TESS_CTRL] = brw->gen >= 7,
      [MESA_SHADER_TESS_EVAL] = brw->gen >= 7,
      [MESA_SHADER_GEOMETRY] = brw->gen >= 6,
      [MESA_SHADER_FRAGMENT] = true,
      [MESA_SHADER_COMPUTE] =
         ((ctx->API == API_OPENGL_COMPAT || ctx->API == API_OPENGL_CORE) &&
          ctx->Const.MaxComputeWorkGroupSize[0] >= 1024) ||
         (ctx->API == API_OPENGLES2 &&
          ctx->Const.MaxComputeWorkGroupSize[0] >= 128) ||
         _mesa_extension_override_enables.ARB_compute_shader,
   };

   unsigned num_stages = 0;
   for (int i = 0; i < MESA_SHADER_STAGES; i++) {
      if (stage_exists[i])
         num_stages++;
   }

   unsigned max_samplers =
      brw->gen >= 8 || brw->is_haswell ? BRW_MAX_TEX_UNIT : 16;

   ctx->Const.MaxDualSourceDrawBuffers = 1;
   ctx->Const.MaxDrawBuffers = BRW_MAX_DRAW_BUFFERS;
   ctx->Const.MaxCombinedShaderOutputResources =
      MAX_IMAGE_UNITS + BRW_MAX_DRAW_BUFFERS;

   ctx->Const.QueryCounterBits.Timestamp = 36;

   ctx->Const.MaxTextureCoordUnits = 8; /* Mesa limit */
   ctx->Const.MaxImageUnits = MAX_IMAGE_UNITS;
   if (brw->gen >= 7) {
      ctx->Const.MaxRenderbufferSize = 16384;
      ctx->Const.MaxTextureLevels = MIN2(15 /* 16384 */, MAX_TEXTURE_LEVELS);
      ctx->Const.MaxCubeTextureLevels = 15; /* 16384 */
   } else {
      ctx->Const.MaxRenderbufferSize = 8192;
      ctx->Const.MaxTextureLevels = MIN2(14 /* 8192 */, MAX_TEXTURE_LEVELS);
      ctx->Const.MaxCubeTextureLevels = 14; /* 8192 */
   }
   ctx->Const.Max3DTextureLevels = 12; /* 2048 */
   ctx->Const.MaxArrayTextureLayers = brw->gen >= 7 ? 2048 : 512;
   ctx->Const.MaxTextureMbytes = 1536;
   ctx->Const.MaxTextureRectSize = 1 << 12;
   ctx->Const.MaxTextureMaxAnisotropy = 16.0;
   ctx->Const.MaxTextureLodBias = 15.0;
   ctx->Const.StripTextureBorder = true;
   if (brw->gen >= 7) {
      ctx->Const.MaxProgramTextureGatherComponents = 4;
      ctx->Const.MinProgramTextureGatherOffset = -32;
      ctx->Const.MaxProgramTextureGatherOffset = 31;
   } else if (brw->gen == 6) {
      ctx->Const.MaxProgramTextureGatherComponents = 1;
      ctx->Const.MinProgramTextureGatherOffset = -8;
      ctx->Const.MaxProgramTextureGatherOffset = 7;
   }

   ctx->Const.MaxUniformBlockSize = 65536;

   for (int i = 0; i < MESA_SHADER_STAGES; i++) {
      struct gl_program_constants *prog = &ctx->Const.Program[i];

      if (!stage_exists[i])
         continue;

      prog->MaxTextureImageUnits = max_samplers;

      prog->MaxUniformBlocks = BRW_MAX_UBO;
      prog->MaxCombinedUniformComponents =
         prog->MaxUniformComponents +
         ctx->Const.MaxUniformBlockSize / 4 * prog->MaxUniformBlocks;

      prog->MaxAtomicCounters = MAX_ATOMIC_COUNTERS;
      prog->MaxAtomicBuffers = BRW_MAX_ABO;
      prog->MaxImageUniforms = compiler->scalar_stage[i] ? BRW_MAX_IMAGES : 0;
      prog->MaxShaderStorageBlocks = BRW_MAX_SSBO;
   }

   ctx->Const.MaxTextureUnits =
      MIN2(ctx->Const.MaxTextureCoordUnits,
           ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxTextureImageUnits);

   ctx->Const.MaxUniformBufferBindings = num_stages * BRW_MAX_UBO;
   ctx->Const.MaxCombinedUniformBlocks = num_stages * BRW_MAX_UBO;
   ctx->Const.MaxCombinedAtomicBuffers = num_stages * BRW_MAX_ABO;
   ctx->Const.MaxCombinedShaderStorageBlocks = num_stages * BRW_MAX_SSBO;
   ctx->Const.MaxShaderStorageBufferBindings = num_stages * BRW_MAX_SSBO;
   ctx->Const.MaxCombinedTextureImageUnits = num_stages * max_samplers;
   ctx->Const.MaxCombinedImageUniforms = num_stages * BRW_MAX_IMAGES;


   /* Hardware only supports a limited number of transform feedback buffers.
    * So we need to override the Mesa default (which is based only on software
    * limits).
    */
   ctx->Const.MaxTransformFeedbackBuffers = BRW_MAX_SOL_BUFFERS;

   /* On Gen6, in the worst case, we use up one binding table entry per
    * transform feedback component (see comments above the definition of
    * BRW_MAX_SOL_BINDINGS, in brw_context.h), so we need to advertise a value
    * for MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS equal to
    * BRW_MAX_SOL_BINDINGS.
    *
    * In "separate components" mode, we need to divide this value by
    * BRW_MAX_SOL_BUFFERS, so that the total number of binding table entries
    * used up by all buffers will not exceed BRW_MAX_SOL_BINDINGS.
    */
   ctx->Const.MaxTransformFeedbackInterleavedComponents = BRW_MAX_SOL_BINDINGS;
   ctx->Const.MaxTransformFeedbackSeparateComponents =
      BRW_MAX_SOL_BINDINGS / BRW_MAX_SOL_BUFFERS;

   ctx->Const.AlwaysUseGetTransformFeedbackVertexCount =
      !can_do_mi_math_and_lrr(brw->screen);

   int max_samples;
   const int *msaa_modes = intel_supported_msaa_modes(brw->screen);
   const int clamp_max_samples =
      driQueryOptioni(&brw->optionCache, "clamp_max_samples");

   if (clamp_max_samples < 0) {
      max_samples = msaa_modes[0];
   } else {
      /* Select the largest supported MSAA mode that does not exceed
       * clamp_max_samples.
       */
      max_samples = 0;
      for (int i = 0; msaa_modes[i] != 0; ++i) {
         if (msaa_modes[i] <= clamp_max_samples) {
            max_samples = msaa_modes[i];
            break;
         }
      }
   }

   ctx->Const.MaxSamples = max_samples;
   ctx->Const.MaxColorTextureSamples = max_samples;
   ctx->Const.MaxDepthTextureSamples = max_samples;
   ctx->Const.MaxIntegerSamples = max_samples;
   ctx->Const.MaxImageSamples = 0;

   /* gen6_set_sample_maps() sets SampleMap{2,4,8}x variables which are used
    * to map indices of rectangular grid to sample numbers within a pixel.
    * These variables are used by GL_EXT_framebuffer_multisample_blit_scaled
    * extension implementation. For more details see the comment above
    * gen6_set_sample_maps() definition.
    */
   gen6_set_sample_maps(ctx);

   ctx->Const.MinLineWidth = 1.0;
   ctx->Const.MinLineWidthAA = 1.0;
   if (brw->gen >= 6) {
      ctx->Const.MaxLineWidth = 7.375;
      ctx->Const.MaxLineWidthAA = 7.375;
      ctx->Const.LineWidthGranularity = 0.125;
   } else {
      ctx->Const.MaxLineWidth = 7.0;
      ctx->Const.MaxLineWidthAA = 7.0;
      ctx->Const.LineWidthGranularity = 0.5;
   }

   /* For non-antialiased lines, we have to round the line width to the
    * nearest whole number. Make sure that we don't advertise a line
    * width that, when rounded, will be beyond the actual hardware
    * maximum.
    */
   assert(roundf(ctx->Const.MaxLineWidth) <= ctx->Const.MaxLineWidth);

   ctx->Const.MinPointSize = 1.0;
   ctx->Const.MinPointSizeAA = 1.0;
   ctx->Const.MaxPointSize = 255.0;
   ctx->Const.MaxPointSizeAA = 255.0;
   ctx->Const.PointSizeGranularity = 1.0;

   if (brw->gen >= 5 || brw->is_g4x)
      ctx->Const.MaxClipPlanes = 8;

   ctx->Const.GLSLTessLevelsAsInputs = true;
   ctx->Const.LowerTCSPatchVerticesIn = brw->gen >= 8;
   ctx->Const.LowerTESPatchVerticesIn = true;
   ctx->Const.PrimitiveRestartForPatches = true;

   ctx->Const.Program[MESA_SHADER_VERTEX].MaxNativeInstructions = 16 * 1024;
   ctx->Const.Program[MESA_SHADER_VERTEX].MaxAluInstructions = 0;
   ctx->Const.Program[MESA_SHADER_VERTEX].MaxTexInstructions = 0;
   ctx->Const.Program[MESA_SHADER_VERTEX].MaxTexIndirections = 0;
   ctx->Const.Program[MESA_SHADER_VERTEX].MaxNativeAluInstructions = 0;
   ctx->Const.Program[MESA_SHADER_VERTEX].MaxNativeTexInstructions = 0;
   ctx->Const.Program[MESA_SHADER_VERTEX].MaxNativeTexIndirections = 0;
   ctx->Const.Program[MESA_SHADER_VERTEX].MaxNativeAttribs = 16;
   ctx->Const.Program[MESA_SHADER_VERTEX].MaxNativeTemps = 256;
   ctx->Const.Program[MESA_SHADER_VERTEX].MaxNativeAddressRegs = 1;
   ctx->Const.Program[MESA_SHADER_VERTEX].MaxNativeParameters = 1024;
   ctx->Const.Program[MESA_SHADER_VERTEX].MaxEnvParams =
      MIN2(ctx->Const.Program[MESA_SHADER_VERTEX].MaxNativeParameters,
	   ctx->Const.Program[MESA_SHADER_VERTEX].MaxEnvParams);

   ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxNativeInstructions = 1024;
   ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxNativeAluInstructions = 1024;
   ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxNativeTexInstructions = 1024;
   ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxNativeTexIndirections = 1024;
   ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxNativeAttribs = 12;
   ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxNativeTemps = 256;
   ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxNativeAddressRegs = 0;
   ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxNativeParameters = 1024;
   ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxEnvParams =
      MIN2(ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxNativeParameters,
	   ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxEnvParams);

   /* Fragment shaders use real, 32-bit twos-complement integers for all
    * integer types.
    */
   ctx->Const.Program[MESA_SHADER_FRAGMENT].LowInt.RangeMin = 31;
   ctx->Const.Program[MESA_SHADER_FRAGMENT].LowInt.RangeMax = 30;
   ctx->Const.Program[MESA_SHADER_FRAGMENT].LowInt.Precision = 0;
   ctx->Const.Program[MESA_SHADER_FRAGMENT].HighInt = ctx->Const.Program[MESA_SHADER_FRAGMENT].LowInt;
   ctx->Const.Program[MESA_SHADER_FRAGMENT].MediumInt = ctx->Const.Program[MESA_SHADER_FRAGMENT].LowInt;

   ctx->Const.Program[MESA_SHADER_VERTEX].LowInt.RangeMin = 31;
   ctx->Const.Program[MESA_SHADER_VERTEX].LowInt.RangeMax = 30;
   ctx->Const.Program[MESA_SHADER_VERTEX].LowInt.Precision = 0;
   ctx->Const.Program[MESA_SHADER_VERTEX].HighInt = ctx->Const.Program[MESA_SHADER_VERTEX].LowInt;
   ctx->Const.Program[MESA_SHADER_VERTEX].MediumInt = ctx->Const.Program[MESA_SHADER_VERTEX].LowInt;

   /* Gen6 converts quads to polygon in beginning of 3D pipeline,
    * but we're not sure how it's actually done for vertex order,
    * that affect provoking vertex decision. Always use last vertex
    * convention for quad primitive which works as expected for now.
    */
   if (brw->gen >= 6)
      ctx->Const.QuadsFollowProvokingVertexConvention = false;

   ctx->Const.NativeIntegers = true;
   ctx->Const.VertexID_is_zero_based = true;

   /* Regarding the CMP instruction, the Ivybridge PRM says:
    *
    *   "For each enabled channel 0b or 1b is assigned to the appropriate flag
    *    bit and 0/all zeros or all ones (e.g, byte 0xFF, word 0xFFFF, DWord
    *    0xFFFFFFFF) is assigned to dst."
    *
    * but PRMs for earlier generations say
    *
    *   "In dword format, one GRF may store up to 8 results. When the register
    *    is used later as a vector of Booleans, as only LSB at each channel
    *    contains meaning [sic] data, software should make sure all higher bits
    *    are masked out (e.g. by 'and-ing' an [sic] 0x01 constant)."
    *
    * We select the representation of a true boolean uniform to be ~0, and fix
    * the results of Gen <= 5 CMP instruction's with -(result & 1).
    */
   ctx->Const.UniformBooleanTrue = ~0;

   /* From the gen4 PRM, volume 4 page 127:
    *
    *     "For SURFTYPE_BUFFER non-rendertarget surfaces, this field specifies
    *      the base address of the first element of the surface, computed in
    *      software by adding the surface base address to the byte offset of
    *      the element in the buffer."
    *
    * However, unaligned accesses are slower, so enforce buffer alignment.
    */
   ctx->Const.UniformBufferOffsetAlignment = 16;

   /* ShaderStorageBufferOffsetAlignment should be a cacheline (64 bytes) so
    * that we can safely have the CPU and GPU writing the same SSBO on
    * non-cachecoherent systems (our Atom CPUs). With UBOs, the GPU never
    * writes, so there's no problem. For an SSBO, the GPU and the CPU can
    * be updating disjoint regions of the buffer simultaneously and that will
    * break if the regions overlap the same cacheline.
    */
   ctx->Const.ShaderStorageBufferOffsetAlignment = 64;
   ctx->Const.TextureBufferOffsetAlignment = 16;
   ctx->Const.MaxTextureBufferSize = 128 * 1024 * 1024;

   if (brw->gen >= 6) {
      ctx->Const.MaxVarying = 32;
      ctx->Const.Program[MESA_SHADER_VERTEX].MaxOutputComponents = 128;
      ctx->Const.Program[MESA_SHADER_GEOMETRY].MaxInputComponents = 64;
      ctx->Const.Program[MESA_SHADER_GEOMETRY].MaxOutputComponents = 128;
      ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxInputComponents = 128;
      ctx->Const.Program[MESA_SHADER_TESS_CTRL].MaxInputComponents = 128;
      ctx->Const.Program[MESA_SHADER_TESS_CTRL].MaxOutputComponents = 128;
      ctx->Const.Program[MESA_SHADER_TESS_EVAL].MaxInputComponents = 128;
      ctx->Const.Program[MESA_SHADER_TESS_EVAL].MaxOutputComponents = 128;
   }

   /* We want the GLSL compiler to emit code that uses condition codes */
   for (int i = 0; i < MESA_SHADER_STAGES; i++) {
      ctx->Const.ShaderCompilerOptions[i] =
         brw->screen->compiler->glsl_compiler_options[i];
   }

   if (brw->gen >= 7) {
      ctx->Const.MaxViewportWidth = 32768;
      ctx->Const.MaxViewportHeight = 32768;
   }

   /* ARB_viewport_array, OES_viewport_array */
   if (brw->gen >= 6) {
      ctx->Const.MaxViewports = GEN6_NUM_VIEWPORTS;
      ctx->Const.ViewportSubpixelBits = 0;

      /* Cast to float before negating because MaxViewportWidth is unsigned.
       */
      ctx->Const.ViewportBounds.Min = -(float)ctx->Const.MaxViewportWidth;
      ctx->Const.ViewportBounds.Max = ctx->Const.MaxViewportWidth;
   }

   /* ARB_gpu_shader5 */
   if (brw->gen >= 7)
      ctx->Const.MaxVertexStreams = MIN2(4, MAX_VERTEX_STREAMS);

   /* ARB_framebuffer_no_attachments */
   ctx->Const.MaxFramebufferWidth = 16384;
   ctx->Const.MaxFramebufferHeight = 16384;
   ctx->Const.MaxFramebufferLayers = ctx->Const.MaxArrayTextureLayers;
   ctx->Const.MaxFramebufferSamples = max_samples;

   /* OES_primitive_bounding_box */
   ctx->Const.NoPrimitiveBoundingBoxOutput = true;
}

static void
brw_initialize_cs_context_constants(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   const struct intel_screen *screen = brw->screen;
   struct gen_device_info *devinfo = &brw->screen->devinfo;

   /* FINISHME: Do this for all platforms that the kernel supports */
   if (brw->is_cherryview &&
       screen->subslice_total > 0 && screen->eu_total > 0) {
      /* Logical CS threads = EUs per subslice * 7 threads per EU */
      uint32_t max_cs_threads = screen->eu_total / screen->subslice_total * 7;

      /* Fuse configurations may give more threads than expected, never less. */
      if (max_cs_threads > devinfo->max_cs_threads)
         devinfo->max_cs_threads = max_cs_threads;
   }

   /* Maximum number of scalar compute shader invocations that can be run in
    * parallel in the same subslice assuming SIMD32 dispatch.
    *
    * We don't advertise more than 64 threads, because we are limited to 64 by
    * our usage of thread_width_max in the gpgpu walker command. This only
    * currently impacts Haswell, which otherwise might be able to advertise 70
    * threads. With SIMD32 and 64 threads, Haswell still provides twice the
    * required the number of invocation needed for ARB_compute_shader.
    */
   const unsigned max_threads = MIN2(64, devinfo->max_cs_threads);
   const uint32_t max_invocations = 32 * max_threads;
   ctx->Const.MaxComputeWorkGroupSize[0] = max_invocations;
   ctx->Const.MaxComputeWorkGroupSize[1] = max_invocations;
   ctx->Const.MaxComputeWorkGroupSize[2] = max_invocations;
   ctx->Const.MaxComputeWorkGroupInvocations = max_invocations;
   ctx->Const.MaxComputeSharedMemorySize = 64 * 1024;
}

/**
 * Process driconf (drirc) options, setting appropriate context flags.
 *
 * intelInitExtensions still pokes at optionCache directly, in order to
 * avoid advertising various extensions.  No flags are set, so it makes
 * sense to continue doing that there.
 */
static void
brw_process_driconf_options(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;

   driOptionCache *options = &brw->optionCache;
   driParseConfigFiles(options, &brw->screen->optionCache,
                       brw->driContext->driScreenPriv->myNum, "i965");

   int bo_reuse_mode = driQueryOptioni(options, "bo_reuse");
   switch (bo_reuse_mode) {
   case DRI_CONF_BO_REUSE_DISABLED:
      break;
   case DRI_CONF_BO_REUSE_ALL:
      intel_bufmgr_gem_enable_reuse(brw->bufmgr);
      break;
   }

   if (!driQueryOptionb(options, "hiz")) {
       brw->has_hiz = false;
       /* On gen6, you can only do separate stencil with HIZ. */
       if (brw->gen == 6)
          brw->has_separate_stencil = false;
   }

   if (driQueryOptionb(options, "always_flush_batch")) {
      fprintf(stderr, "flushing batchbuffer before/after each draw call\n");
      brw->always_flush_batch = true;
   }

   if (driQueryOptionb(options, "always_flush_cache")) {
      fprintf(stderr, "flushing GPU caches before/after each draw call\n");
      brw->always_flush_cache = true;
   }

   if (driQueryOptionb(options, "disable_throttling")) {
      fprintf(stderr, "disabling flush throttling\n");
      brw->disable_throttling = true;
   }

   brw->precompile = driQueryOptionb(&brw->optionCache, "shader_precompile");

   if (driQueryOptionb(&brw->optionCache, "precise_trig"))
      brw->screen->compiler->precise_trig = true;

   ctx->Const.ForceGLSLExtensionsWarn =
      driQueryOptionb(options, "force_glsl_extensions_warn");

   ctx->Const.ForceGLSLVersion =
      driQueryOptioni(options, "force_glsl_version");

   ctx->Const.DisableGLSLLineContinuations =
      driQueryOptionb(options, "disable_glsl_line_continuations");

   ctx->Const.AllowGLSLExtensionDirectiveMidShader =
      driQueryOptionb(options, "allow_glsl_extension_directive_midshader");

   ctx->Const.GLSLZeroInit = driQueryOptionb(options, "glsl_zero_init");

   brw->dual_color_blend_by_location =
      driQueryOptionb(options, "dual_color_blend_by_location");
}

GLboolean
brwCreateContext(gl_api api,
	         const struct gl_config *mesaVis,
		 __DRIcontext *driContextPriv,
                 unsigned major_version,
                 unsigned minor_version,
                 uint32_t flags,
                 bool notify_reset,
                 unsigned *dri_ctx_error,
	         void *sharedContextPrivate)
{
   struct gl_context *shareCtx = (struct gl_context *) sharedContextPrivate;
   struct intel_screen *screen = driContextPriv->driScreenPriv->driverPrivate;
   const struct gen_device_info *devinfo = &screen->devinfo;
   struct dd_function_table functions;

   /* Only allow the __DRI_CTX_FLAG_ROBUST_BUFFER_ACCESS flag if the kernel
    * provides us with context reset notifications.
    */
   uint32_t allowed_flags = __DRI_CTX_FLAG_DEBUG
      | __DRI_CTX_FLAG_FORWARD_COMPATIBLE;

   if (screen->has_context_reset_notification)
      allowed_flags |= __DRI_CTX_FLAG_ROBUST_BUFFER_ACCESS;

   if (flags & ~allowed_flags) {
      *dri_ctx_error = __DRI_CTX_ERROR_UNKNOWN_FLAG;
      return false;
   }

   struct brw_context *brw = rzalloc(NULL, struct brw_context);
   if (!brw) {
      fprintf(stderr, "%s: failed to alloc context\n", __func__);
      *dri_ctx_error = __DRI_CTX_ERROR_NO_MEMORY;
      return false;
   }

   driContextPriv->driverPrivate = brw;
   brw->driContext = driContextPriv;
   brw->screen = screen;
   brw->bufmgr = screen->bufmgr;

   brw->gen = devinfo->gen;
   brw->gt = devinfo->gt;
   brw->is_g4x = devinfo->is_g4x;
   brw->is_baytrail = devinfo->is_baytrail;
   brw->is_haswell = devinfo->is_haswell;
   brw->is_cherryview = devinfo->is_cherryview;
   brw->is_broxton = devinfo->is_broxton;
   brw->has_llc = devinfo->has_llc;
   brw->has_hiz = devinfo->has_hiz_and_separate_stencil;
   brw->has_separate_stencil = devinfo->has_hiz_and_separate_stencil;
   brw->has_pln = devinfo->has_pln;
   brw->has_compr4 = devinfo->has_compr4;
   brw->has_surface_tile_offset = devinfo->has_surface_tile_offset;
   brw->has_negative_rhw_bug = devinfo->has_negative_rhw_bug;
   brw->needs_unlit_centroid_workaround =
      devinfo->needs_unlit_centroid_workaround;

   brw->must_use_separate_stencil = devinfo->must_use_separate_stencil;
   brw->has_swizzling = screen->hw_has_swizzling;

   isl_device_init(&brw->isl_dev, devinfo, screen->hw_has_swizzling);

   brw->vs.base.stage = MESA_SHADER_VERTEX;
   brw->tcs.base.stage = MESA_SHADER_TESS_CTRL;
   brw->tes.base.stage = MESA_SHADER_TESS_EVAL;
   brw->gs.base.stage = MESA_SHADER_GEOMETRY;
   brw->wm.base.stage = MESA_SHADER_FRAGMENT;
   if (brw->gen >= 8) {
      gen8_init_vtable_surface_functions(brw);
      brw->vtbl.emit_depth_stencil_hiz = gen8_emit_depth_stencil_hiz;
   } else if (brw->gen >= 7) {
      gen7_init_vtable_surface_functions(brw);
      brw->vtbl.emit_depth_stencil_hiz = gen7_emit_depth_stencil_hiz;
   } else if (brw->gen >= 6) {
      gen6_init_vtable_surface_functions(brw);
      brw->vtbl.emit_depth_stencil_hiz = gen6_emit_depth_stencil_hiz;
   } else {
      gen4_init_vtable_surface_functions(brw);
      brw->vtbl.emit_depth_stencil_hiz = brw_emit_depth_stencil_hiz;
   }

   brw_init_driver_functions(brw, &functions);

   if (notify_reset)
      functions.GetGraphicsResetStatus = brw_get_graphics_reset_status;

   struct gl_context *ctx = &brw->ctx;

   if (!_mesa_initialize_context(ctx, api, mesaVis, shareCtx, &functions)) {
      *dri_ctx_error = __DRI_CTX_ERROR_NO_MEMORY;
      fprintf(stderr, "%s: failed to init mesa context\n", __func__);
      intelDestroyContext(driContextPriv);
      return false;
   }

   driContextSetFlags(ctx, flags);

   /* Initialize the software rasterizer and helper modules.
    *
    * As of GL 3.1 core, the gen4+ driver doesn't need the swrast context for
    * software fallbacks (which we have to support on legacy GL to do weird
    * glDrawPixels(), glBitmap(), and other functions).
    */
   if (api != API_OPENGL_CORE && api != API_OPENGLES2) {
      _swrast_CreateContext(ctx);
   }

   _vbo_CreateContext(ctx);
   if (ctx->swrast_context) {
      _tnl_CreateContext(ctx);
      TNL_CONTEXT(ctx)->Driver.RunPipeline = _tnl_run_pipeline;
      _swsetup_CreateContext(ctx);

      /* Configure swrast to match hardware characteristics: */
      _swrast_allow_pixel_fog(ctx, false);
      _swrast_allow_vertex_fog(ctx, true);
   }

   _mesa_meta_init(ctx);

   brw_process_driconf_options(brw);

   if (INTEL_DEBUG & DEBUG_PERF)
      brw->perf_debug = true;

   brw_initialize_cs_context_constants(brw);
   brw_initialize_context_constants(brw);

   ctx->Const.ResetStrategy = notify_reset
      ? GL_LOSE_CONTEXT_ON_RESET_ARB : GL_NO_RESET_NOTIFICATION_ARB;

   /* Reinitialize the context point state.  It depends on ctx->Const values. */
   _mesa_init_point(ctx);

   intel_fbo_init(brw);

   intel_batchbuffer_init(&brw->batch, brw->bufmgr, brw->has_llc);

   if (brw->gen >= 6) {
      /* Create a new hardware context.  Using a hardware context means that
       * our GPU state will be saved/restored on context switch, allowing us
       * to assume that the GPU is in the same state we left it in.
       *
       * This is required for transform feedback buffer offsets, query objects,
       * and also allows us to reduce how much state we have to emit.
       */
      brw->hw_ctx = drm_intel_gem_context_create(brw->bufmgr);

      if (!brw->hw_ctx) {
         fprintf(stderr, "Gen6+ requires Kernel 3.6 or later.\n");
         intelDestroyContext(driContextPriv);
         return false;
      }
   }

   if (brw_init_pipe_control(brw, devinfo)) {
      *dri_ctx_error = __DRI_CTX_ERROR_NO_MEMORY;
      intelDestroyContext(driContextPriv);
      return false;
   }

   brw_init_state(brw);

   intelInitExtensions(ctx);

   brw_init_surface_formats(brw);

   if (brw->gen >= 6)
      brw_blorp_init(brw);

   brw->urb.size = devinfo->urb.size;

   if (brw->gen == 6)
      brw->urb.gs_present = false;

   brw->prim_restart.in_progress = false;
   brw->prim_restart.enable_cut_index = false;
   brw->gs.enabled = false;
   brw->sf.viewport_transform_enable = true;
   brw->clip.viewport_count = 1;

   brw->predicate.state = BRW_PREDICATE_STATE_RENDER;

   brw->max_gtt_map_object_size = screen->max_gtt_map_object_size;

   brw->use_resource_streamer = screen->has_resource_streamer &&
      (env_var_as_boolean("INTEL_USE_HW_BT", false) ||
       env_var_as_boolean("INTEL_USE_GATHER", false));

   ctx->VertexProgram._MaintainTnlProgram = true;
   ctx->FragmentProgram._MaintainTexEnvProgram = true;

   brw_draw_init( brw );

   if ((flags & __DRI_CTX_FLAG_DEBUG) != 0) {
      /* Turn on some extra GL_ARB_debug_output generation. */
      brw->perf_debug = true;
   }

   if ((flags & __DRI_CTX_FLAG_ROBUST_BUFFER_ACCESS) != 0) {
      ctx->Const.ContextFlags |= GL_CONTEXT_FLAG_ROBUST_ACCESS_BIT_ARB;
      ctx->Const.RobustAccess = GL_TRUE;
   }

   if (INTEL_DEBUG & DEBUG_SHADER_TIME)
      brw_init_shader_time(brw);

   _mesa_compute_version(ctx);

   _mesa_initialize_dispatch_tables(ctx);
   _mesa_initialize_vbo_vtxfmt(ctx);

   vbo_use_buffer_objects(ctx);
   vbo_always_unmap_buffers(ctx);

   return true;
}

void
intelDestroyContext(__DRIcontext * driContextPriv)
{
   struct brw_context *brw =
      (struct brw_context *) driContextPriv->driverPrivate;
   struct gl_context *ctx = &brw->ctx;

   /* Dump a final BMP in case the application doesn't call SwapBuffers */
   if (INTEL_DEBUG & DEBUG_AUB) {
      intel_batchbuffer_flush(brw);
      aub_dump_bmp(&brw->ctx);
   }

   _mesa_meta_free(&brw->ctx);

   if (INTEL_DEBUG & DEBUG_SHADER_TIME) {
      /* Force a report. */
      brw->shader_time.report_time = 0;

      brw_collect_and_report_shader_time(brw);
      brw_destroy_shader_time(brw);
   }

   if (brw->gen >= 6)
      blorp_finish(&brw->blorp);

   brw_destroy_state(brw);
   brw_draw_destroy(brw);

   drm_intel_bo_unreference(brw->curbe.curbe_bo);
   if (brw->vs.base.scratch_bo)
      drm_intel_bo_unreference(brw->vs.base.scratch_bo);
   if (brw->tcs.base.scratch_bo)
      drm_intel_bo_unreference(brw->tcs.base.scratch_bo);
   if (brw->tes.base.scratch_bo)
      drm_intel_bo_unreference(brw->tes.base.scratch_bo);
   if (brw->gs.base.scratch_bo)
      drm_intel_bo_unreference(brw->gs.base.scratch_bo);
   if (brw->wm.base.scratch_bo)
      drm_intel_bo_unreference(brw->wm.base.scratch_bo);

   gen7_reset_hw_bt_pool_offsets(brw);
   drm_intel_bo_unreference(brw->hw_bt_pool.bo);
   brw->hw_bt_pool.bo = NULL;

   drm_intel_gem_context_destroy(brw->hw_ctx);

   if (ctx->swrast_context) {
      _swsetup_DestroyContext(&brw->ctx);
      _tnl_DestroyContext(&brw->ctx);
   }
   _vbo_DestroyContext(&brw->ctx);

   if (ctx->swrast_context)
      _swrast_DestroyContext(&brw->ctx);

   brw_fini_pipe_control(brw);
   intel_batchbuffer_free(&brw->batch);

   drm_intel_bo_unreference(brw->throttle_batch[1]);
   drm_intel_bo_unreference(brw->throttle_batch[0]);
   brw->throttle_batch[1] = NULL;
   brw->throttle_batch[0] = NULL;

   driDestroyOptionCache(&brw->optionCache);

   /* free the Mesa context */
   _mesa_free_context_data(&brw->ctx);

   ralloc_free(brw);
   driContextPriv->driverPrivate = NULL;
}

GLboolean
intelUnbindContext(__DRIcontext * driContextPriv)
{
   /* Unset current context and dispath table */
   _mesa_make_current(NULL, NULL, NULL);

   return true;
}

/**
 * Fixes up the context for GLES23 with our default-to-sRGB-capable behavior
 * on window system framebuffers.
 *
 * Desktop GL is fairly reasonable in its handling of sRGB: You can ask if
 * your renderbuffer can do sRGB encode, and you can flip a switch that does
 * sRGB encode if the renderbuffer can handle it.  You can ask specifically
 * for a visual where you're guaranteed to be capable, but it turns out that
 * everyone just makes all their ARGB8888 visuals capable and doesn't offer
 * incapable ones, because there's no difference between the two in resources
 * used.  Applications thus get built that accidentally rely on the default
 * visual choice being sRGB, so we make ours sRGB capable.  Everything sounds
 * great...
 *
 * But for GLES2/3, they decided that it was silly to not turn on sRGB encode
 * for sRGB renderbuffers you made with the GL_EXT_texture_sRGB equivalent.
 * So they removed the enable knob and made it "if the renderbuffer is sRGB
 * capable, do sRGB encode".  Then, for your window system renderbuffers, you
 * can ask for sRGB visuals and get sRGB encode, or not ask for sRGB visuals
 * and get no sRGB encode (assuming that both kinds of visual are available).
 * Thus our choice to support sRGB by default on our visuals for desktop would
 * result in broken rendering of GLES apps that aren't expecting sRGB encode.
 *
 * Unfortunately, renderbuffer setup happens before a context is created.  So
 * in intel_screen.c we always set up sRGB, and here, if you're a GLES2/3
 * context (without an sRGB visual, though we don't have sRGB visuals exposed
 * yet), we go turn that back off before anyone finds out.
 */
static void
intel_gles3_srgb_workaround(struct brw_context *brw,
                            struct gl_framebuffer *fb)
{
   struct gl_context *ctx = &brw->ctx;

   if (_mesa_is_desktop_gl(ctx) || !fb->Visual.sRGBCapable)
      return;

   /* Some day when we support the sRGB capable bit on visuals available for
    * GLES, we'll need to respect that and not disable things here.
    */
   fb->Visual.sRGBCapable = false;
   for (int i = 0; i < BUFFER_COUNT; i++) {
      struct gl_renderbuffer *rb = fb->Attachment[i].Renderbuffer;
      if (rb)
         rb->Format = _mesa_get_srgb_format_linear(rb->Format);
   }
}

GLboolean
intelMakeCurrent(__DRIcontext * driContextPriv,
                 __DRIdrawable * driDrawPriv,
                 __DRIdrawable * driReadPriv)
{
   struct brw_context *brw;
   GET_CURRENT_CONTEXT(curCtx);

   if (driContextPriv)
      brw = (struct brw_context *) driContextPriv->driverPrivate;
   else
      brw = NULL;

   /* According to the glXMakeCurrent() man page: "Pending commands to
    * the previous context, if any, are flushed before it is released."
    * But only flush if we're actually changing contexts.
    */
   if (brw_context(curCtx) && brw_context(curCtx) != brw) {
      _mesa_flush(curCtx);
   }

   if (driContextPriv) {
      struct gl_context *ctx = &brw->ctx;
      struct gl_framebuffer *fb, *readFb;

      if (driDrawPriv == NULL) {
         fb = _mesa_get_incomplete_framebuffer();
      } else {
         fb = driDrawPriv->driverPrivate;
         driContextPriv->dri2.draw_stamp = driDrawPriv->dri2.stamp - 1;
      }

      if (driReadPriv == NULL) {
         readFb = _mesa_get_incomplete_framebuffer();
      } else {
         readFb = driReadPriv->driverPrivate;
         driContextPriv->dri2.read_stamp = driReadPriv->dri2.stamp - 1;
      }

      /* The sRGB workaround changes the renderbuffer's format. We must change
       * the format before the renderbuffer's miptree get's allocated, otherwise
       * the formats of the renderbuffer and its miptree will differ.
       */
      intel_gles3_srgb_workaround(brw, fb);
      intel_gles3_srgb_workaround(brw, readFb);

      /* If the context viewport hasn't been initialized, force a call out to
       * the loader to get buffers so we have a drawable size for the initial
       * viewport. */
      if (!brw->ctx.ViewportInitialized)
         intel_prepare_render(brw);

      _mesa_make_current(ctx, fb, readFb);
   } else {
      _mesa_make_current(NULL, NULL, NULL);
   }

   return true;
}

void
intel_resolve_for_dri2_flush(struct brw_context *brw,
                             __DRIdrawable *drawable)
{
   if (brw->gen < 6) {
      /* MSAA and fast color clear are not supported, so don't waste time
       * checking whether a resolve is needed.
       */
      return;
   }

   struct gl_framebuffer *fb = drawable->driverPrivate;
   struct intel_renderbuffer *rb;

   /* Usually, only the back buffer will need to be downsampled. However,
    * the front buffer will also need it if the user has rendered into it.
    */
   static const gl_buffer_index buffers[2] = {
         BUFFER_BACK_LEFT,
         BUFFER_FRONT_LEFT,
   };

   for (int i = 0; i < 2; ++i) {
      rb = intel_get_renderbuffer(fb, buffers[i]);
      if (rb == NULL || rb->mt == NULL)
         continue;
      if (rb->mt->num_samples <= 1) {
         assert(rb->mt_layer == 0 && rb->mt_level == 0 &&
                rb->layer_count == 1);
         intel_miptree_resolve_color(brw, rb->mt, 0, 0, 1, 0);
      } else {
         intel_renderbuffer_downsample(brw, rb);
      }
   }
}

static unsigned
intel_bits_per_pixel(const struct intel_renderbuffer *rb)
{
   return _mesa_get_format_bytes(intel_rb_format(rb)) * 8;
}

static void
intel_query_dri2_buffers(struct brw_context *brw,
                         __DRIdrawable *drawable,
                         __DRIbuffer **buffers,
                         int *count);

static void
intel_process_dri2_buffer(struct brw_context *brw,
                          __DRIdrawable *drawable,
                          __DRIbuffer *buffer,
                          struct intel_renderbuffer *rb,
                          const char *buffer_name);

static void
intel_update_image_buffers(struct brw_context *brw, __DRIdrawable *drawable);

static void
intel_update_dri2_buffers(struct brw_context *brw, __DRIdrawable *drawable)
{
   struct gl_framebuffer *fb = drawable->driverPrivate;
   struct intel_renderbuffer *rb;
   __DRIbuffer *buffers = NULL;
   int i, count;
   const char *region_name;

   /* Set this up front, so that in case our buffers get invalidated
    * while we're getting new buffers, we don't clobber the stamp and
    * thus ignore the invalidate. */
   drawable->lastStamp = drawable->dri2.stamp;

   if (unlikely(INTEL_DEBUG & DEBUG_DRI))
      fprintf(stderr, "enter %s, drawable %p\n", __func__, drawable);

   intel_query_dri2_buffers(brw, drawable, &buffers, &count);

   if (buffers == NULL)
      return;

   for (i = 0; i < count; i++) {
       switch (buffers[i].attachment) {
       case __DRI_BUFFER_FRONT_LEFT:
           rb = intel_get_renderbuffer(fb, BUFFER_FRONT_LEFT);
           region_name = "dri2 front buffer";
           break;

       case __DRI_BUFFER_FAKE_FRONT_LEFT:
           rb = intel_get_renderbuffer(fb, BUFFER_FRONT_LEFT);
           region_name = "dri2 fake front buffer";
           break;

       case __DRI_BUFFER_BACK_LEFT:
           rb = intel_get_renderbuffer(fb, BUFFER_BACK_LEFT);
           region_name = "dri2 back buffer";
           break;

       case __DRI_BUFFER_DEPTH:
       case __DRI_BUFFER_HIZ:
       case __DRI_BUFFER_DEPTH_STENCIL:
       case __DRI_BUFFER_STENCIL:
       case __DRI_BUFFER_ACCUM:
       default:
           fprintf(stderr,
                   "unhandled buffer attach event, attachment type %d\n",
                   buffers[i].attachment);
           return;
       }

       intel_process_dri2_buffer(brw, drawable, &buffers[i], rb, region_name);
   }

}

void
intel_update_renderbuffers(__DRIcontext *context, __DRIdrawable *drawable)
{
   struct brw_context *brw = context->driverPrivate;
   __DRIscreen *dri_screen = brw->screen->driScrnPriv;

   /* Set this up front, so that in case our buffers get invalidated
    * while we're getting new buffers, we don't clobber the stamp and
    * thus ignore the invalidate. */
   drawable->lastStamp = drawable->dri2.stamp;

   if (unlikely(INTEL_DEBUG & DEBUG_DRI))
      fprintf(stderr, "enter %s, drawable %p\n", __func__, drawable);

   if (dri_screen->image.loader)
      intel_update_image_buffers(brw, drawable);
   else
      intel_update_dri2_buffers(brw, drawable);

   driUpdateFramebufferSize(&brw->ctx, drawable);
}

/**
 * intel_prepare_render should be called anywhere that curent read/drawbuffer
 * state is required.
 */
void
intel_prepare_render(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   __DRIcontext *driContext = brw->driContext;
   __DRIdrawable *drawable;

   drawable = driContext->driDrawablePriv;
   if (drawable && drawable->dri2.stamp != driContext->dri2.draw_stamp) {
      if (drawable->lastStamp != drawable->dri2.stamp)
         intel_update_renderbuffers(driContext, drawable);
      driContext->dri2.draw_stamp = drawable->dri2.stamp;
   }

   drawable = driContext->driReadablePriv;
   if (drawable && drawable->dri2.stamp != driContext->dri2.read_stamp) {
      if (drawable->lastStamp != drawable->dri2.stamp)
         intel_update_renderbuffers(driContext, drawable);
      driContext->dri2.read_stamp = drawable->dri2.stamp;
   }

   /* If we're currently rendering to the front buffer, the rendering
    * that will happen next will probably dirty the front buffer.  So
    * mark it as dirty here.
    */
   if (_mesa_is_front_buffer_drawing(ctx->DrawBuffer))
      brw->front_buffer_dirty = true;
}

/**
 * \brief Query DRI2 to obtain a DRIdrawable's buffers.
 *
 * To determine which DRI buffers to request, examine the renderbuffers
 * attached to the drawable's framebuffer. Then request the buffers with
 * DRI2GetBuffers() or DRI2GetBuffersWithFormat().
 *
 * This is called from intel_update_renderbuffers().
 *
 * \param drawable      Drawable whose buffers are queried.
 * \param buffers       [out] List of buffers returned by DRI2 query.
 * \param buffer_count  [out] Number of buffers returned.
 *
 * \see intel_update_renderbuffers()
 * \see DRI2GetBuffers()
 * \see DRI2GetBuffersWithFormat()
 */
static void
intel_query_dri2_buffers(struct brw_context *brw,
                         __DRIdrawable *drawable,
                         __DRIbuffer **buffers,
                         int *buffer_count)
{
   __DRIscreen *dri_screen = brw->screen->driScrnPriv;
   struct gl_framebuffer *fb = drawable->driverPrivate;
   int i = 0;
   unsigned attachments[8];

   struct intel_renderbuffer *front_rb;
   struct intel_renderbuffer *back_rb;

   front_rb = intel_get_renderbuffer(fb, BUFFER_FRONT_LEFT);
   back_rb = intel_get_renderbuffer(fb, BUFFER_BACK_LEFT);

   memset(attachments, 0, sizeof(attachments));
   if ((_mesa_is_front_buffer_drawing(fb) ||
        _mesa_is_front_buffer_reading(fb) ||
        !back_rb) && front_rb) {
      /* If a fake front buffer is in use, then querying for
       * __DRI_BUFFER_FRONT_LEFT will cause the server to copy the image from
       * the real front buffer to the fake front buffer.  So before doing the
       * query, we need to make sure all the pending drawing has landed in the
       * real front buffer.
       */
      intel_batchbuffer_flush(brw);
      intel_flush_front(&brw->ctx);

      attachments[i++] = __DRI_BUFFER_FRONT_LEFT;
      attachments[i++] = intel_bits_per_pixel(front_rb);
   } else if (front_rb && brw->front_buffer_dirty) {
      /* We have pending front buffer rendering, but we aren't querying for a
       * front buffer.  If the front buffer we have is a fake front buffer,
       * the X server is going to throw it away when it processes the query.
       * So before doing the query, make sure all the pending drawing has
       * landed in the real front buffer.
       */
      intel_batchbuffer_flush(brw);
      intel_flush_front(&brw->ctx);
   }

   if (back_rb) {
      attachments[i++] = __DRI_BUFFER_BACK_LEFT;
      attachments[i++] = intel_bits_per_pixel(back_rb);
   }

   assert(i <= ARRAY_SIZE(attachments));

   *buffers =
      dri_screen->dri2.loader->getBuffersWithFormat(drawable,
                                                    &drawable->w,
                                                    &drawable->h,
                                                    attachments, i / 2,
                                                    buffer_count,
                                                    drawable->loaderPrivate);
}

/**
 * \brief Assign a DRI buffer's DRM region to a renderbuffer.
 *
 * This is called from intel_update_renderbuffers().
 *
 * \par Note:
 *    DRI buffers whose attachment point is DRI2BufferStencil or
 *    DRI2BufferDepthStencil are handled as special cases.
 *
 * \param buffer_name is a human readable name, such as "dri2 front buffer",
 *        that is passed to drm_intel_bo_gem_create_from_name().
 *
 * \see intel_update_renderbuffers()
 */
static void
intel_process_dri2_buffer(struct brw_context *brw,
                          __DRIdrawable *drawable,
                          __DRIbuffer *buffer,
                          struct intel_renderbuffer *rb,
                          const char *buffer_name)
{
   struct gl_framebuffer *fb = drawable->driverPrivate;
   drm_intel_bo *bo;

   if (!rb)
      return;

   unsigned num_samples = rb->Base.Base.NumSamples;

   /* We try to avoid closing and reopening the same BO name, because the first
    * use of a mapping of the buffer involves a bunch of page faulting which is
    * moderately expensive.
    */
   struct intel_mipmap_tree *last_mt;
   if (num_samples == 0)
      last_mt = rb->mt;
   else
      last_mt = rb->singlesample_mt;

   uint32_t old_name = 0;
   if (last_mt) {
       /* The bo already has a name because the miptree was created by a
	* previous call to intel_process_dri2_buffer(). If a bo already has a
	* name, then drm_intel_bo_flink() is a low-cost getter.  It does not
	* create a new name.
	*/
      drm_intel_bo_flink(last_mt->bo, &old_name);
   }

   if (old_name == buffer->name)
      return;

   if (unlikely(INTEL_DEBUG & DEBUG_DRI)) {
      fprintf(stderr,
              "attaching buffer %d, at %d, cpp %d, pitch %d\n",
              buffer->name, buffer->attachment,
              buffer->cpp, buffer->pitch);
   }

   bo = drm_intel_bo_gem_create_from_name(brw->bufmgr, buffer_name,
                                          buffer->name);
   if (!bo) {
      fprintf(stderr,
              "Failed to open BO for returned DRI2 buffer "
              "(%dx%d, %s, named %d).\n"
              "This is likely a bug in the X Server that will lead to a "
              "crash soon.\n",
              drawable->w, drawable->h, buffer_name, buffer->name);
      return;
   }

   intel_update_winsys_renderbuffer_miptree(brw, rb, bo,
                                            drawable->w, drawable->h,
                                            buffer->pitch);

   if (_mesa_is_front_buffer_drawing(fb) &&
       (buffer->attachment == __DRI_BUFFER_FRONT_LEFT ||
        buffer->attachment == __DRI_BUFFER_FAKE_FRONT_LEFT) &&
       rb->Base.Base.NumSamples > 1) {
      intel_renderbuffer_upsample(brw, rb);
   }

   assert(rb->mt);

   drm_intel_bo_unreference(bo);
}

/**
 * \brief Query DRI image loader to obtain a DRIdrawable's buffers.
 *
 * To determine which DRI buffers to request, examine the renderbuffers
 * attached to the drawable's framebuffer. Then request the buffers from
 * the image loader
 *
 * This is called from intel_update_renderbuffers().
 *
 * \param drawable      Drawable whose buffers are queried.
 * \param buffers       [out] List of buffers returned by DRI2 query.
 * \param buffer_count  [out] Number of buffers returned.
 *
 * \see intel_update_renderbuffers()
 */

static void
intel_update_image_buffer(struct brw_context *intel,
                          __DRIdrawable *drawable,
                          struct intel_renderbuffer *rb,
                          __DRIimage *buffer,
                          enum __DRIimageBufferMask buffer_type)
{
   struct gl_framebuffer *fb = drawable->driverPrivate;

   if (!rb || !buffer->bo)
      return;

   unsigned num_samples = rb->Base.Base.NumSamples;

   /* Check and see if we're already bound to the right
    * buffer object
    */
   struct intel_mipmap_tree *last_mt;
   if (num_samples == 0)
      last_mt = rb->mt;
   else
      last_mt = rb->singlesample_mt;

   if (last_mt && last_mt->bo == buffer->bo)
      return;

   intel_update_winsys_renderbuffer_miptree(intel, rb, buffer->bo,
                                            buffer->width, buffer->height,
                                            buffer->pitch);

   if (_mesa_is_front_buffer_drawing(fb) &&
       buffer_type == __DRI_IMAGE_BUFFER_FRONT &&
       rb->Base.Base.NumSamples > 1) {
      intel_renderbuffer_upsample(intel, rb);
   }
}

static void
intel_update_image_buffers(struct brw_context *brw, __DRIdrawable *drawable)
{
   struct gl_framebuffer *fb = drawable->driverPrivate;
   __DRIscreen *dri_screen = brw->screen->driScrnPriv;
   struct intel_renderbuffer *front_rb;
   struct intel_renderbuffer *back_rb;
   struct __DRIimageList images;
   unsigned int format;
   uint32_t buffer_mask = 0;
   int ret;

   front_rb = intel_get_renderbuffer(fb, BUFFER_FRONT_LEFT);
   back_rb = intel_get_renderbuffer(fb, BUFFER_BACK_LEFT);

   if (back_rb)
      format = intel_rb_format(back_rb);
   else if (front_rb)
      format = intel_rb_format(front_rb);
   else
      return;

   if (front_rb && (_mesa_is_front_buffer_drawing(fb) ||
                    _mesa_is_front_buffer_reading(fb) || !back_rb)) {
      buffer_mask |= __DRI_IMAGE_BUFFER_FRONT;
   }

   if (back_rb)
      buffer_mask |= __DRI_IMAGE_BUFFER_BACK;

   ret = dri_screen->image.loader->getBuffers(drawable,
                                              driGLFormatToImageFormat(format),
                                              &drawable->dri2.stamp,
                                              drawable->loaderPrivate,
                                              buffer_mask,
                                              &images);
   if (!ret)
      return;

   if (images.image_mask & __DRI_IMAGE_BUFFER_FRONT) {
      drawable->w = images.front->width;
      drawable->h = images.front->height;
      intel_update_image_buffer(brw,
                                drawable,
                                front_rb,
                                images.front,
                                __DRI_IMAGE_BUFFER_FRONT);
   }
   if (images.image_mask & __DRI_IMAGE_BUFFER_BACK) {
      drawable->w = images.back->width;
      drawable->h = images.back->height;
      intel_update_image_buffer(brw,
                                drawable,
                                back_rb,
                                images.back,
                                __DRI_IMAGE_BUFFER_BACK);
   }
}
