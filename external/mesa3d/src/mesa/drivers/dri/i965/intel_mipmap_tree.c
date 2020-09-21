/*
 * Copyright 2006 VMware, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <GL/gl.h>
#include <GL/internal/dri_interface.h>

#include "intel_batchbuffer.h"
#include "intel_mipmap_tree.h"
#include "intel_resolve_map.h"
#include "intel_tex.h"
#include "intel_blit.h"
#include "intel_fbo.h"

#include "brw_blorp.h"
#include "brw_context.h"
#include "brw_state.h"

#include "main/enums.h"
#include "main/fbobject.h"
#include "main/formats.h"
#include "main/glformats.h"
#include "main/texcompress_etc.h"
#include "main/teximage.h"
#include "main/streaming-load-memcpy.h"
#include "x86/common_x86_asm.h"

#define FILE_DEBUG_FLAG DEBUG_MIPTREE

static void *intel_miptree_map_raw(struct brw_context *brw,
                                   struct intel_mipmap_tree *mt);

static void intel_miptree_unmap_raw(struct intel_mipmap_tree *mt);

static bool
intel_miptree_alloc_mcs(struct brw_context *brw,
                        struct intel_mipmap_tree *mt,
                        GLuint num_samples);

/**
 * Determine which MSAA layout should be used by the MSAA surface being
 * created, based on the chip generation and the surface type.
 */
static enum intel_msaa_layout
compute_msaa_layout(struct brw_context *brw, mesa_format format,
                    enum intel_aux_disable aux_disable)
{
   /* Prior to Gen7, all MSAA surfaces used IMS layout. */
   if (brw->gen < 7)
      return INTEL_MSAA_LAYOUT_IMS;

   /* In Gen7, IMS layout is only used for depth and stencil buffers. */
   switch (_mesa_get_format_base_format(format)) {
   case GL_DEPTH_COMPONENT:
   case GL_STENCIL_INDEX:
   case GL_DEPTH_STENCIL:
      return INTEL_MSAA_LAYOUT_IMS;
   default:
      /* From the Ivy Bridge PRM, Vol4 Part1 p77 ("MCS Enable"):
       *
       *   This field must be set to 0 for all SINT MSRTs when all RT channels
       *   are not written
       *
       * In practice this means that we have to disable MCS for all signed
       * integer MSAA buffers.  The alternative, to disable MCS only when one
       * of the render target channels is disabled, is impractical because it
       * would require converting between CMS and UMS MSAA layouts on the fly,
       * which is expensive.
       */
      if (brw->gen == 7 && _mesa_get_format_datatype(format) == GL_INT) {
         return INTEL_MSAA_LAYOUT_UMS;
      } else if (aux_disable & INTEL_AUX_DISABLE_MCS) {
         /* We can't use the CMS layout because it uses an aux buffer, the MCS
          * buffer. So fallback to UMS, which is identical to CMS without the
          * MCS. */
         return INTEL_MSAA_LAYOUT_UMS;
      } else {
         return INTEL_MSAA_LAYOUT_CMS;
      }
   }
}

bool
intel_tiling_supports_non_msrt_mcs(const struct brw_context *brw,
                                   unsigned tiling)
{
   /* From the Ivy Bridge PRM, Vol2 Part1 11.7 "MCS Buffer for Render
    * Target(s)", beneath the "Fast Color Clear" bullet (p326):
    *
    *     - Support is limited to tiled render targets.
    *
    * Gen9 changes the restriction to Y-tile only.
    */
   if (brw->gen >= 9)
      return tiling == I915_TILING_Y;
   else if (brw->gen >= 7)
      return tiling != I915_TILING_NONE;
   else
      return false;
}

/**
 * For a single-sampled render target ("non-MSRT"), determine if an MCS buffer
 * can be used. This doesn't (and should not) inspect any of the properties of
 * the miptree's BO.
 *
 * From the Ivy Bridge PRM, Vol2 Part1 11.7 "MCS Buffer for Render Target(s)",
 * beneath the "Fast Color Clear" bullet (p326):
 *
 *     - Support is for non-mip-mapped and non-array surface types only.
 *
 * And then later, on p327:
 *
 *     - MCS buffer for non-MSRT is supported only for RT formats 32bpp,
 *       64bpp, and 128bpp.
 *
 * From the Skylake documentation, it is made clear that X-tiling is no longer
 * supported:
 *
 *     - MCS and Lossless compression is supported for TiledY/TileYs/TileYf
 *     non-MSRTs only.
 */
bool
intel_miptree_supports_non_msrt_fast_clear(struct brw_context *brw,
                                           const struct intel_mipmap_tree *mt)
{
   /* MCS support does not exist prior to Gen7 */
   if (brw->gen < 7)
      return false;

   if (mt->aux_disable & INTEL_AUX_DISABLE_MCS)
      return false;

   /* This function applies only to non-multisampled render targets. */
   if (mt->num_samples > 1)
      return false;

   /* MCS is only supported for color buffers */
   switch (_mesa_get_format_base_format(mt->format)) {
   case GL_DEPTH_COMPONENT:
   case GL_DEPTH_STENCIL:
   case GL_STENCIL_INDEX:
      return false;
   }

   if (mt->cpp != 4 && mt->cpp != 8 && mt->cpp != 16)
      return false;

   const bool mip_mapped = mt->first_level != 0 || mt->last_level != 0;
   const bool arrayed = mt->physical_depth0 != 1;

   if (arrayed) {
       /* Multisample surfaces with the CMS layout are not layered surfaces,
        * yet still have physical_depth0 > 1. Assert that we don't
        * accidentally reject a multisampled surface here. We should have
        * rejected it earlier by explicitly checking the sample count.
        */
      assert(mt->num_samples <= 1);
   }

   /* Handle the hardware restrictions...
    *
    * All GENs have the following restriction: "MCS buffer for non-MSRT is
    * supported only for RT formats 32bpp, 64bpp, and 128bpp."
    *
    * From the HSW PRM Volume 7: 3D-Media-GPGPU, page 652: (Color Clear of
    * Non-MultiSampler Render Target Restrictions) Support is for
    * non-mip-mapped and non-array surface types only.
    *
    * From the BDW PRM Volume 7: 3D-Media-GPGPU, page 649: (Color Clear of
    * Non-MultiSampler Render Target Restriction). Mip-mapped and arrayed
    * surfaces are supported with MCS buffer layout with these alignments in
    * the RT space: Horizontal Alignment = 256 and Vertical Alignment = 128.
    *
    * From the SKL PRM Volume 7: 3D-Media-GPGPU, page 632: (Color Clear of
    * Non-MultiSampler Render Target Restriction). Mip-mapped and arrayed
    * surfaces are supported with MCS buffer layout with these alignments in
    * the RT space: Horizontal Alignment = 128 and Vertical Alignment = 64.
    */
   if (brw->gen < 8 && (mip_mapped || arrayed))
      return false;

   /* There's no point in using an MCS buffer if the surface isn't in a
    * renderable format.
    */
   if (!brw->format_supported_as_render_target[mt->format])
      return false;

   if (brw->gen >= 9) {
      mesa_format linear_format = _mesa_get_srgb_format_linear(mt->format);
      const uint32_t brw_format = brw_format_for_mesa_format(linear_format);
      return isl_format_supports_lossless_compression(&brw->screen->devinfo,
                                                      brw_format);
   } else
      return true;
}

/* On Gen9 support for color buffer compression was extended to single
 * sampled surfaces. This is a helper considering both auxiliary buffer
 * type and number of samples telling if the given miptree represents
 * the new single sampled case - also called lossless compression.
 */
bool
intel_miptree_is_lossless_compressed(const struct brw_context *brw,
                                     const struct intel_mipmap_tree *mt)
{
   /* Only available from Gen9 onwards. */
   if (brw->gen < 9)
      return false;

   /* Compression always requires auxiliary buffer. */
   if (!mt->mcs_buf)
      return false;

   /* Single sample compression is represented re-using msaa compression
    * layout type: "Compressed Multisampled Surfaces".
    */
   if (mt->msaa_layout != INTEL_MSAA_LAYOUT_CMS)
      return false;

   /* And finally distinguish between msaa and single sample case. */
   return mt->num_samples <= 1;
}

bool
intel_miptree_supports_lossless_compressed(struct brw_context *brw,
                                           const struct intel_mipmap_tree *mt)
{
   /* For now compression is only enabled for integer formats even though
    * there exist supported floating point formats also. This is a heuristic
    * decision based on current public benchmarks. In none of the cases these
    * formats provided any improvement but a few cases were seen to regress.
    * Hence these are left to to be enabled in the future when they are known
    * to improve things.
    */
   if (_mesa_get_format_datatype(mt->format) == GL_FLOAT)
      return false;

   /* Fast clear mechanism and lossless compression go hand in hand. */
   if (!intel_miptree_supports_non_msrt_fast_clear(brw, mt))
      return false;

   /* Fast clear can be also used to clear srgb surfaces by using equivalent
    * linear format. This trick, however, can't be extended to be used with
    * lossless compression and therefore a check is needed to see if the format
    * really is linear.
    */
   return _mesa_get_srgb_format_linear(mt->format) == mt->format;
}

/**
 * Determine depth format corresponding to a depth+stencil format,
 * for separate stencil.
 */
mesa_format
intel_depth_format_for_depthstencil_format(mesa_format format) {
   switch (format) {
   case MESA_FORMAT_Z24_UNORM_S8_UINT:
      return MESA_FORMAT_Z24_UNORM_X8_UINT;
   case MESA_FORMAT_Z32_FLOAT_S8X24_UINT:
      return MESA_FORMAT_Z_FLOAT32;
   default:
      return format;
   }
}


/**
 * @param for_bo Indicates that the caller is
 *        intel_miptree_create_for_bo(). If true, then do not create
 *        \c stencil_mt.
 */
static struct intel_mipmap_tree *
intel_miptree_create_layout(struct brw_context *brw,
                            GLenum target,
                            mesa_format format,
                            GLuint first_level,
                            GLuint last_level,
                            GLuint width0,
                            GLuint height0,
                            GLuint depth0,
                            GLuint num_samples,
                            uint32_t layout_flags)
{
   struct intel_mipmap_tree *mt = calloc(sizeof(*mt), 1);
   if (!mt)
      return NULL;

   DBG("%s target %s format %s level %d..%d slices %d <-- %p\n", __func__,
       _mesa_enum_to_string(target),
       _mesa_get_format_name(format),
       first_level, last_level, depth0, mt);

   if (target == GL_TEXTURE_1D_ARRAY)
      assert(height0 == 1);

   mt->target = target;
   mt->format = format;
   mt->first_level = first_level;
   mt->last_level = last_level;
   mt->logical_width0 = width0;
   mt->logical_height0 = height0;
   mt->logical_depth0 = depth0;
   mt->aux_disable = (layout_flags & MIPTREE_LAYOUT_DISABLE_AUX) != 0 ?
      INTEL_AUX_DISABLE_ALL : INTEL_AUX_DISABLE_NONE;
   mt->aux_disable |= INTEL_AUX_DISABLE_CCS;
   mt->is_scanout = (layout_flags & MIPTREE_LAYOUT_FOR_SCANOUT) != 0;
   exec_list_make_empty(&mt->hiz_map);
   exec_list_make_empty(&mt->color_resolve_map);
   mt->cpp = _mesa_get_format_bytes(format);
   mt->num_samples = num_samples;
   mt->compressed = _mesa_is_format_compressed(format);
   mt->msaa_layout = INTEL_MSAA_LAYOUT_NONE;
   mt->refcount = 1;

   int depth_multiply = 1;
   if (num_samples > 1) {
      /* Adjust width/height/depth for MSAA */
      mt->msaa_layout = compute_msaa_layout(brw, format, mt->aux_disable);
      if (mt->msaa_layout == INTEL_MSAA_LAYOUT_IMS) {
         /* From the Ivybridge PRM, Volume 1, Part 1, page 108:
          * "If the surface is multisampled and it is a depth or stencil
          *  surface or Multisampled Surface StorageFormat in SURFACE_STATE is
          *  MSFMT_DEPTH_STENCIL, WL and HL must be adjusted as follows before
          *  proceeding:
          *
          *  +----------------------------------------------------------------+
          *  | Num Multisamples |        W_l =         |        H_l =         |
          *  +----------------------------------------------------------------+
          *  |         2        | ceiling(W_l / 2) * 4 | H_l (no adjustment)  |
          *  |         4        | ceiling(W_l / 2) * 4 | ceiling(H_l / 2) * 4 |
          *  |         8        | ceiling(W_l / 2) * 8 | ceiling(H_l / 2) * 4 |
          *  |        16        | ceiling(W_l / 2) * 8 | ceiling(H_l / 2) * 8 |
          *  +----------------------------------------------------------------+
          * "
          *
          * Note that MSFMT_DEPTH_STENCIL just means the IMS (interleaved)
          * format rather than UMS/CMS (array slices).  The Sandybridge PRM,
          * Volume 1, Part 1, Page 111 has the same formula for 4x MSAA.
          *
          * Another more complicated explanation for these adjustments comes
          * from the Sandybridge PRM, volume 4, part 1, page 31:
          *
          *     "Any of the other messages (sample*, LOD, load4) used with a
          *      (4x) multisampled surface will in-effect sample a surface with
          *      double the height and width as that indicated in the surface
          *      state. Each pixel position on the original-sized surface is
          *      replaced with a 2x2 of samples with the following arrangement:
          *
          *         sample 0 sample 2
          *         sample 1 sample 3"
          *
          * Thus, when sampling from a multisampled texture, it behaves as
          * though the layout in memory for (x,y,sample) is:
          *
          *      (0,0,0) (0,0,2)   (1,0,0) (1,0,2)
          *      (0,0,1) (0,0,3)   (1,0,1) (1,0,3)
          *
          *      (0,1,0) (0,1,2)   (1,1,0) (1,1,2)
          *      (0,1,1) (0,1,3)   (1,1,1) (1,1,3)
          *
          * However, the actual layout of multisampled data in memory is:
          *
          *      (0,0,0) (1,0,0)   (0,0,1) (1,0,1)
          *      (0,1,0) (1,1,0)   (0,1,1) (1,1,1)
          *
          *      (0,0,2) (1,0,2)   (0,0,3) (1,0,3)
          *      (0,1,2) (1,1,2)   (0,1,3) (1,1,3)
          *
          * This pattern repeats for each 2x2 pixel block.
          *
          * As a result, when calculating the size of our 4-sample buffer for
          * an odd width or height, we have to align before scaling up because
          * sample 3 is in that bottom right 2x2 block.
          */
         switch (num_samples) {
         case 2:
            assert(brw->gen >= 8);
            width0 = ALIGN(width0, 2) * 2;
            height0 = ALIGN(height0, 2);
            break;
         case 4:
            width0 = ALIGN(width0, 2) * 2;
            height0 = ALIGN(height0, 2) * 2;
            break;
         case 8:
            width0 = ALIGN(width0, 2) * 4;
            height0 = ALIGN(height0, 2) * 2;
            break;
         case 16:
            width0 = ALIGN(width0, 2) * 4;
            height0 = ALIGN(height0, 2) * 4;
            break;
         default:
            /* num_samples should already have been quantized to 0, 1, 2, 4, 8
             * or 16.
             */
            unreachable("not reached");
         }
      } else {
         /* Non-interleaved */
         depth_multiply = num_samples;
         depth0 *= depth_multiply;
      }
   }

   /* Set array_layout to ALL_SLICES_AT_EACH_LOD when array_spacing_lod0 can
    * be used. array_spacing_lod0 is only used for non-IMS MSAA surfaces on
    * Gen 7 and 8. On Gen 8 and 9 this layout is not available but it is still
    * used on Gen8 to make it pick a qpitch value which doesn't include space
    * for the mipmaps. On Gen9 this is not necessary because it will
    * automatically pick a packed qpitch value whenever mt->first_level ==
    * mt->last_level.
    * TODO: can we use it elsewhere?
    * TODO: also disable this on Gen8 and pick the qpitch value like Gen9
    */
   if (brw->gen >= 9) {
      mt->array_layout = ALL_LOD_IN_EACH_SLICE;
   } else {
      switch (mt->msaa_layout) {
      case INTEL_MSAA_LAYOUT_NONE:
      case INTEL_MSAA_LAYOUT_IMS:
         mt->array_layout = ALL_LOD_IN_EACH_SLICE;
         break;
      case INTEL_MSAA_LAYOUT_UMS:
      case INTEL_MSAA_LAYOUT_CMS:
         mt->array_layout = ALL_SLICES_AT_EACH_LOD;
         break;
      }
   }

   if (target == GL_TEXTURE_CUBE_MAP)
      assert(depth0 == 6 * depth_multiply);

   mt->physical_width0 = width0;
   mt->physical_height0 = height0;
   mt->physical_depth0 = depth0;

   if (!(layout_flags & MIPTREE_LAYOUT_FOR_BO) &&
       _mesa_get_format_base_format(format) == GL_DEPTH_STENCIL &&
       (brw->must_use_separate_stencil ||
	(brw->has_separate_stencil &&
         intel_miptree_wants_hiz_buffer(brw, mt)))) {
      uint32_t stencil_flags = MIPTREE_LAYOUT_ACCELERATED_UPLOAD;
      if (brw->gen == 6) {
         stencil_flags |= MIPTREE_LAYOUT_FORCE_ALL_SLICE_AT_LOD |
                          MIPTREE_LAYOUT_TILING_ANY;
      }

      mt->stencil_mt = intel_miptree_create(brw,
                                            mt->target,
                                            MESA_FORMAT_S_UINT8,
                                            mt->first_level,
                                            mt->last_level,
                                            mt->logical_width0,
                                            mt->logical_height0,
                                            mt->logical_depth0,
                                            num_samples,
                                            stencil_flags);

      if (!mt->stencil_mt) {
	 intel_miptree_release(&mt);
	 return NULL;
      }
      mt->stencil_mt->r8stencil_needs_update = true;

      /* Fix up the Z miptree format for how we're splitting out separate
       * stencil.  Gen7 expects there to be no stencil bits in its depth buffer.
       */
      mt->format = intel_depth_format_for_depthstencil_format(mt->format);
      mt->cpp = 4;

      if (format == mt->format) {
         _mesa_problem(NULL, "Unknown format %s in separate stencil mt\n",
                       _mesa_get_format_name(mt->format));
      }
   }

   if (layout_flags & MIPTREE_LAYOUT_FORCE_ALL_SLICE_AT_LOD)
      mt->array_layout = ALL_SLICES_AT_EACH_LOD;

   /*
    * Obey HALIGN_16 constraints for Gen8 and Gen9 buffers which are
    * multisampled or have an AUX buffer attached to it.
    *
    * GEN  |    MSRT        | AUX_CCS_* or AUX_MCS
    *  -------------------------------------------
    *  9   |  HALIGN_16     |    HALIGN_16
    *  8   |  HALIGN_ANY    |    HALIGN_16
    *  7   |      ?         |        ?
    *  6   |      ?         |        ?
    */
   if (intel_miptree_supports_non_msrt_fast_clear(brw, mt)) {
      if (brw->gen >= 9 || (brw->gen == 8 && num_samples <= 1))
         layout_flags |= MIPTREE_LAYOUT_FORCE_HALIGN16;
   } else if (brw->gen >= 9 && num_samples > 1) {
      layout_flags |= MIPTREE_LAYOUT_FORCE_HALIGN16;
   } else {
      const UNUSED bool is_lossless_compressed_aux =
         brw->gen >= 9 && num_samples == 1 &&
         mt->format == MESA_FORMAT_R_UINT32;

      /* For now, nothing else has this requirement */
      assert(is_lossless_compressed_aux ||
             (layout_flags & MIPTREE_LAYOUT_FORCE_HALIGN16) == 0);
   }

   brw_miptree_layout(brw, mt, layout_flags);

   if (mt->aux_disable & INTEL_AUX_DISABLE_MCS)
      assert(mt->msaa_layout != INTEL_MSAA_LAYOUT_CMS);

   return mt;
}


/**
 * Choose an appropriate uncompressed format for a requested
 * compressed format, if unsupported.
 */
mesa_format
intel_lower_compressed_format(struct brw_context *brw, mesa_format format)
{
   /* No need to lower ETC formats on these platforms,
    * they are supported natively.
    */
   if (brw->gen >= 8 || brw->is_baytrail)
      return format;

   switch (format) {
   case MESA_FORMAT_ETC1_RGB8:
      return MESA_FORMAT_R8G8B8X8_UNORM;
   case MESA_FORMAT_ETC2_RGB8:
      return MESA_FORMAT_R8G8B8X8_UNORM;
   case MESA_FORMAT_ETC2_SRGB8:
   case MESA_FORMAT_ETC2_SRGB8_ALPHA8_EAC:
   case MESA_FORMAT_ETC2_SRGB8_PUNCHTHROUGH_ALPHA1:
      return MESA_FORMAT_B8G8R8A8_SRGB;
   case MESA_FORMAT_ETC2_RGBA8_EAC:
   case MESA_FORMAT_ETC2_RGB8_PUNCHTHROUGH_ALPHA1:
      return MESA_FORMAT_R8G8B8A8_UNORM;
   case MESA_FORMAT_ETC2_R11_EAC:
      return MESA_FORMAT_R_UNORM16;
   case MESA_FORMAT_ETC2_SIGNED_R11_EAC:
      return MESA_FORMAT_R_SNORM16;
   case MESA_FORMAT_ETC2_RG11_EAC:
      return MESA_FORMAT_R16G16_UNORM;
   case MESA_FORMAT_ETC2_SIGNED_RG11_EAC:
      return MESA_FORMAT_R16G16_SNORM;
   default:
      /* Non ETC1 / ETC2 format */
      return format;
   }
}

/* This function computes Yf/Ys tiled bo size, alignment and pitch. */
static unsigned long
intel_get_yf_ys_bo_size(struct intel_mipmap_tree *mt, unsigned *alignment,
                        unsigned long *pitch)
{
   uint32_t tile_width, tile_height;
   unsigned long stride, size, aligned_y;

   assert(mt->tr_mode != INTEL_MIPTREE_TRMODE_NONE);
   intel_get_tile_dims(mt->tiling, mt->tr_mode, mt->cpp,
                       &tile_width, &tile_height);

   aligned_y = ALIGN(mt->total_height, tile_height);
   stride = mt->total_width * mt->cpp;
   stride = ALIGN(stride, tile_width);
   size = stride * aligned_y;

   if (mt->tr_mode == INTEL_MIPTREE_TRMODE_YF) {
      assert(size % 4096 == 0);
      *alignment = 4096;
   } else {
      assert(size % (64 * 1024) == 0);
      *alignment = 64 * 1024;
   }
   *pitch = stride;
   return size;
}

static struct intel_mipmap_tree *
miptree_create(struct brw_context *brw,
               GLenum target,
               mesa_format format,
               GLuint first_level,
               GLuint last_level,
               GLuint width0,
               GLuint height0,
               GLuint depth0,
               GLuint num_samples,
               uint32_t layout_flags)
{
   struct intel_mipmap_tree *mt;
   mesa_format tex_format = format;
   mesa_format etc_format = MESA_FORMAT_NONE;
   uint32_t alloc_flags = 0;

   format = intel_lower_compressed_format(brw, format);

   etc_format = (format != tex_format) ? tex_format : MESA_FORMAT_NONE;

   assert((layout_flags & MIPTREE_LAYOUT_FOR_BO) == 0);
   mt = intel_miptree_create_layout(brw, target, format,
                                    first_level, last_level, width0,
                                    height0, depth0, num_samples,
                                    layout_flags);
   /*
    * pitch == 0 || height == 0  indicates the null texture
    */
   if (!mt || !mt->total_width || !mt->total_height) {
      intel_miptree_release(&mt);
      return NULL;
   }

   if (mt->tiling == (I915_TILING_Y | I915_TILING_X))
      mt->tiling = I915_TILING_Y;

   if (layout_flags & MIPTREE_LAYOUT_ACCELERATED_UPLOAD)
      alloc_flags |= BO_ALLOC_FOR_RENDER;

   unsigned long pitch;
   mt->etc_format = etc_format;

   if (mt->tr_mode != INTEL_MIPTREE_TRMODE_NONE) {
      unsigned alignment = 0;
      unsigned long size;
      size = intel_get_yf_ys_bo_size(mt, &alignment, &pitch);
      assert(size);
      mt->bo = drm_intel_bo_alloc_for_render(brw->bufmgr, "miptree",
                                             size, alignment);
   } else {
      if (format == MESA_FORMAT_S_UINT8) {
         /* Align to size of W tile, 64x64. */
         mt->bo = drm_intel_bo_alloc_tiled(brw->bufmgr, "miptree",
                                           ALIGN(mt->total_width, 64),
                                           ALIGN(mt->total_height, 64),
                                           mt->cpp, &mt->tiling, &pitch,
                                           alloc_flags);
      } else {
         mt->bo = drm_intel_bo_alloc_tiled(brw->bufmgr, "miptree",
                                           mt->total_width, mt->total_height,
                                           mt->cpp, &mt->tiling, &pitch,
                                           alloc_flags);
      }
   }

   mt->pitch = pitch;

   return mt;
}

struct intel_mipmap_tree *
intel_miptree_create(struct brw_context *brw,
                     GLenum target,
                     mesa_format format,
                     GLuint first_level,
                     GLuint last_level,
                     GLuint width0,
                     GLuint height0,
                     GLuint depth0,
                     GLuint num_samples,
                     uint32_t layout_flags)
{
   struct intel_mipmap_tree *mt = miptree_create(
                                     brw, target, format,
                                     first_level, last_level,
                                     width0, height0, depth0, num_samples,
                                     layout_flags);

   /* If the BO is too large to fit in the aperture, we need to use the
    * BLT engine to support it.  Prior to Sandybridge, the BLT paths can't
    * handle Y-tiling, so we need to fall back to X.
    */
   if (brw->gen < 6 && mt->bo->size >= brw->max_gtt_map_object_size &&
       mt->tiling == I915_TILING_Y) {
      unsigned long pitch = mt->pitch;
      const uint32_t alloc_flags =
         (layout_flags & MIPTREE_LAYOUT_ACCELERATED_UPLOAD) ?
         BO_ALLOC_FOR_RENDER : 0;
      perf_debug("%dx%d miptree larger than aperture; falling back to X-tiled\n",
                 mt->total_width, mt->total_height);

      mt->tiling = I915_TILING_X;
      drm_intel_bo_unreference(mt->bo);
      mt->bo = drm_intel_bo_alloc_tiled(brw->bufmgr, "miptree",
                                  mt->total_width, mt->total_height, mt->cpp,
                                  &mt->tiling, &pitch, alloc_flags);
      mt->pitch = pitch;
   }

   mt->offset = 0;

   if (!mt->bo) {
       intel_miptree_release(&mt);
       return NULL;
   }


   if (mt->msaa_layout == INTEL_MSAA_LAYOUT_CMS) {
      assert(mt->num_samples > 1);
      if (!intel_miptree_alloc_mcs(brw, mt, num_samples)) {
         intel_miptree_release(&mt);
         return NULL;
      }
   }

   /* If this miptree is capable of supporting fast color clears, set
    * fast_clear_state appropriately to ensure that fast clears will occur.
    * Allocation of the MCS miptree will be deferred until the first fast
    * clear actually occurs or when compressed single sampled buffer is
    * written by the GPU for the first time.
    */
   if (intel_tiling_supports_non_msrt_mcs(brw, mt->tiling) &&
       intel_miptree_supports_non_msrt_fast_clear(brw, mt)) {
      mt->aux_disable &= ~INTEL_AUX_DISABLE_CCS;
      assert(brw->gen < 8 || mt->halign == 16 || num_samples <= 1);

      /* On Gen9+ clients are not currently capable of consuming compressed
       * single-sampled buffers. Disabling compression allows us to skip
       * resolves.
       */
      const bool lossless_compression_disabled = INTEL_DEBUG & DEBUG_NO_RBC;
      const bool is_lossless_compressed =
         unlikely(!lossless_compression_disabled) &&
         brw->gen >= 9 && !mt->is_scanout &&
         intel_miptree_supports_lossless_compressed(brw, mt);

      if (is_lossless_compressed) {
         intel_miptree_alloc_non_msrt_mcs(brw, mt, is_lossless_compressed);
      }
   }

   return mt;
}

struct intel_mipmap_tree *
intel_miptree_create_for_bo(struct brw_context *brw,
                            drm_intel_bo *bo,
                            mesa_format format,
                            uint32_t offset,
                            uint32_t width,
                            uint32_t height,
                            uint32_t depth,
                            int pitch,
                            uint32_t layout_flags)
{
   struct intel_mipmap_tree *mt;
   uint32_t tiling, swizzle;
   GLenum target;

   drm_intel_bo_get_tiling(bo, &tiling, &swizzle);

   /* Nothing will be able to use this miptree with the BO if the offset isn't
    * aligned.
    */
   if (tiling != I915_TILING_NONE)
      assert(offset % 4096 == 0);

   /* miptrees can't handle negative pitch.  If you need flipping of images,
    * that's outside of the scope of the mt.
    */
   assert(pitch >= 0);

   target = depth > 1 ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D;

   /* The BO already has a tiling format and we shouldn't confuse the lower
    * layers by making it try to find a tiling format again.
    */
   assert((layout_flags & MIPTREE_LAYOUT_TILING_ANY) == 0);
   assert((layout_flags & MIPTREE_LAYOUT_TILING_NONE) == 0);

   layout_flags |= MIPTREE_LAYOUT_FOR_BO;
   mt = intel_miptree_create_layout(brw, target, format,
                                    0, 0,
                                    width, height, depth, 0,
                                    layout_flags);
   if (!mt)
      return NULL;

   drm_intel_bo_reference(bo);
   mt->bo = bo;
   mt->pitch = pitch;
   mt->offset = offset;
   mt->tiling = tiling;

   return mt;
}

/**
 * For a singlesample renderbuffer, this simply wraps the given BO with a
 * miptree.
 *
 * For a multisample renderbuffer, this wraps the window system's
 * (singlesample) BO with a singlesample miptree attached to the
 * intel_renderbuffer, then creates a multisample miptree attached to irb->mt
 * that will contain the actual rendering (which is lazily resolved to
 * irb->singlesample_mt).
 */
void
intel_update_winsys_renderbuffer_miptree(struct brw_context *intel,
                                         struct intel_renderbuffer *irb,
                                         drm_intel_bo *bo,
                                         uint32_t width, uint32_t height,
                                         uint32_t pitch)
{
   struct intel_mipmap_tree *singlesample_mt = NULL;
   struct intel_mipmap_tree *multisample_mt = NULL;
   struct gl_renderbuffer *rb = &irb->Base.Base;
   mesa_format format = rb->Format;
   int num_samples = rb->NumSamples;

   /* Only the front and back buffers, which are color buffers, are allocated
    * through the image loader.
    */
   assert(_mesa_get_format_base_format(format) == GL_RGB ||
          _mesa_get_format_base_format(format) == GL_RGBA);

   singlesample_mt = intel_miptree_create_for_bo(intel,
                                                 bo,
                                                 format,
                                                 0,
                                                 width,
                                                 height,
                                                 1,
                                                 pitch,
                                                 MIPTREE_LAYOUT_FOR_SCANOUT);
   if (!singlesample_mt)
      goto fail;

   /* If this miptree is capable of supporting fast color clears, set
    * mcs_state appropriately to ensure that fast clears will occur.
    * Allocation of the MCS miptree will be deferred until the first fast
    * clear actually occurs.
    */
   if (intel_tiling_supports_non_msrt_mcs(intel, singlesample_mt->tiling) &&
       intel_miptree_supports_non_msrt_fast_clear(intel, singlesample_mt)) {
      singlesample_mt->aux_disable &= ~INTEL_AUX_DISABLE_CCS;
   }

   if (num_samples == 0) {
      intel_miptree_release(&irb->mt);
      irb->mt = singlesample_mt;

      assert(!irb->singlesample_mt);
   } else {
      intel_miptree_release(&irb->singlesample_mt);
      irb->singlesample_mt = singlesample_mt;

      if (!irb->mt ||
          irb->mt->logical_width0 != width ||
          irb->mt->logical_height0 != height) {
         multisample_mt = intel_miptree_create_for_renderbuffer(intel,
                                                                format,
                                                                width,
                                                                height,
                                                                num_samples);
         if (!multisample_mt)
            goto fail;

         irb->need_downsample = false;
         intel_miptree_release(&irb->mt);
         irb->mt = multisample_mt;
      }
   }
   return;

fail:
   intel_miptree_release(&irb->singlesample_mt);
   intel_miptree_release(&irb->mt);
   return;
}

struct intel_mipmap_tree*
intel_miptree_create_for_renderbuffer(struct brw_context *brw,
                                      mesa_format format,
                                      uint32_t width,
                                      uint32_t height,
                                      uint32_t num_samples)
{
   struct intel_mipmap_tree *mt;
   uint32_t depth = 1;
   bool ok;
   GLenum target = num_samples > 1 ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
   const uint32_t layout_flags = MIPTREE_LAYOUT_ACCELERATED_UPLOAD |
                                 MIPTREE_LAYOUT_TILING_ANY |
                                 MIPTREE_LAYOUT_FOR_SCANOUT;

   mt = intel_miptree_create(brw, target, format, 0, 0,
                             width, height, depth, num_samples,
                             layout_flags);
   if (!mt)
      goto fail;

   if (intel_miptree_wants_hiz_buffer(brw, mt)) {
      ok = intel_miptree_alloc_hiz(brw, mt);
      if (!ok)
         goto fail;
   }

   return mt;

fail:
   intel_miptree_release(&mt);
   return NULL;
}

void
intel_miptree_reference(struct intel_mipmap_tree **dst,
                        struct intel_mipmap_tree *src)
{
   if (*dst == src)
      return;

   intel_miptree_release(dst);

   if (src) {
      src->refcount++;
      DBG("%s %p refcount now %d\n", __func__, src, src->refcount);
   }

   *dst = src;
}

static void
intel_miptree_hiz_buffer_free(struct intel_miptree_hiz_buffer *hiz_buf)
{
   if (hiz_buf == NULL)
      return;

   if (hiz_buf->mt)
      intel_miptree_release(&hiz_buf->mt);
   else
      drm_intel_bo_unreference(hiz_buf->aux_base.bo);

   free(hiz_buf);
}

void
intel_miptree_release(struct intel_mipmap_tree **mt)
{
   if (!*mt)
      return;

   DBG("%s %p refcount will be %d\n", __func__, *mt, (*mt)->refcount - 1);
   if (--(*mt)->refcount <= 0) {
      GLuint i;

      DBG("%s deleting %p\n", __func__, *mt);

      drm_intel_bo_unreference((*mt)->bo);
      intel_miptree_release(&(*mt)->stencil_mt);
      intel_miptree_release(&(*mt)->r8stencil_mt);
      intel_miptree_hiz_buffer_free((*mt)->hiz_buf);
      if ((*mt)->mcs_buf) {
         drm_intel_bo_unreference((*mt)->mcs_buf->bo);
         free((*mt)->mcs_buf);
      }
      intel_resolve_map_clear(&(*mt)->hiz_map);
      intel_resolve_map_clear(&(*mt)->color_resolve_map);

      intel_miptree_release(&(*mt)->plane[0]);
      intel_miptree_release(&(*mt)->plane[1]);

      for (i = 0; i < MAX_TEXTURE_LEVELS; i++) {
	 free((*mt)->level[i].slice);
      }

      free(*mt);
   }
   *mt = NULL;
}


void
intel_get_image_dims(struct gl_texture_image *image,
                     int *width, int *height, int *depth)
{
   switch (image->TexObject->Target) {
   case GL_TEXTURE_1D_ARRAY:
      /* For a 1D Array texture the OpenGL API will treat the image height as
       * the number of array slices. For Intel hardware, we treat the 1D array
       * as a 2D Array with a height of 1. So, here we want to swap image
       * height and depth.
       */
      assert(image->Depth == 1);
      *width = image->Width;
      *height = 1;
      *depth = image->Height;
      break;
   case GL_TEXTURE_CUBE_MAP:
      /* For Cube maps, the mesa/main api layer gives us a depth of 1 even
       * though we really have 6 slices.
       */
      assert(image->Depth == 1);
      *width = image->Width;
      *height = image->Height;
      *depth = 6;
      break;
   default:
      *width = image->Width;
      *height = image->Height;
      *depth = image->Depth;
      break;
   }
}

/**
 * Can the image be pulled into a unified mipmap tree?  This mirrors
 * the completeness test in a lot of ways.
 *
 * Not sure whether I want to pass gl_texture_image here.
 */
bool
intel_miptree_match_image(struct intel_mipmap_tree *mt,
                          struct gl_texture_image *image)
{
   struct intel_texture_image *intelImage = intel_texture_image(image);
   GLuint level = intelImage->base.Base.Level;
   int width, height, depth;

   /* glTexImage* choose the texture object based on the target passed in, and
    * objects can't change targets over their lifetimes, so this should be
    * true.
    */
   assert(image->TexObject->Target == mt->target);

   mesa_format mt_format = mt->format;
   if (mt->format == MESA_FORMAT_Z24_UNORM_X8_UINT && mt->stencil_mt)
      mt_format = MESA_FORMAT_Z24_UNORM_S8_UINT;
   if (mt->format == MESA_FORMAT_Z_FLOAT32 && mt->stencil_mt)
      mt_format = MESA_FORMAT_Z32_FLOAT_S8X24_UINT;
   if (mt->etc_format != MESA_FORMAT_NONE)
      mt_format = mt->etc_format;

   if (image->TexFormat != mt_format)
      return false;

   intel_get_image_dims(image, &width, &height, &depth);

   if (mt->target == GL_TEXTURE_CUBE_MAP)
      depth = 6;

   int level_depth = mt->level[level].depth;
   if (mt->num_samples > 1) {
      switch (mt->msaa_layout) {
      case INTEL_MSAA_LAYOUT_NONE:
      case INTEL_MSAA_LAYOUT_IMS:
         break;
      case INTEL_MSAA_LAYOUT_UMS:
      case INTEL_MSAA_LAYOUT_CMS:
         level_depth /= mt->num_samples;
         break;
      }
   }

   /* Test image dimensions against the base level image adjusted for
    * minification.  This will also catch images not present in the
    * tree, changed targets, etc.
    */
   if (width != minify(mt->logical_width0, level - mt->first_level) ||
       height != minify(mt->logical_height0, level - mt->first_level) ||
       depth != level_depth) {
      return false;
   }

   if (image->NumSamples != mt->num_samples)
      return false;

   return true;
}


void
intel_miptree_set_level_info(struct intel_mipmap_tree *mt,
			     GLuint level,
			     GLuint x, GLuint y, GLuint d)
{
   mt->level[level].depth = d;
   mt->level[level].level_x = x;
   mt->level[level].level_y = y;

   DBG("%s level %d, depth %d, offset %d,%d\n", __func__,
       level, d, x, y);

   assert(mt->level[level].slice == NULL);

   mt->level[level].slice = calloc(d, sizeof(*mt->level[0].slice));
   mt->level[level].slice[0].x_offset = mt->level[level].level_x;
   mt->level[level].slice[0].y_offset = mt->level[level].level_y;
}


void
intel_miptree_set_image_offset(struct intel_mipmap_tree *mt,
			       GLuint level, GLuint img,
			       GLuint x, GLuint y)
{
   if (img == 0 && level == 0)
      assert(x == 0 && y == 0);

   assert(img < mt->level[level].depth);

   mt->level[level].slice[img].x_offset = mt->level[level].level_x + x;
   mt->level[level].slice[img].y_offset = mt->level[level].level_y + y;

   DBG("%s level %d img %d pos %d,%d\n",
       __func__, level, img,
       mt->level[level].slice[img].x_offset,
       mt->level[level].slice[img].y_offset);
}

void
intel_miptree_get_image_offset(const struct intel_mipmap_tree *mt,
			       GLuint level, GLuint slice,
			       GLuint *x, GLuint *y)
{
   assert(slice < mt->level[level].depth);

   *x = mt->level[level].slice[slice].x_offset;
   *y = mt->level[level].slice[slice].y_offset;
}


/**
 * This function computes the tile_w (in bytes) and tile_h (in rows) of
 * different tiling patterns. If the BO is untiled, tile_w is set to cpp
 * and tile_h is set to 1.
 */
void
intel_get_tile_dims(uint32_t tiling, uint32_t tr_mode, uint32_t cpp,
                    uint32_t *tile_w, uint32_t *tile_h)
{
   if (tr_mode == INTEL_MIPTREE_TRMODE_NONE) {
      switch (tiling) {
      case I915_TILING_X:
         *tile_w = 512;
         *tile_h = 8;
         break;
      case I915_TILING_Y:
         *tile_w = 128;
         *tile_h = 32;
         break;
      case I915_TILING_NONE:
         *tile_w = cpp;
         *tile_h = 1;
         break;
      default:
         unreachable("not reached");
      }
   } else {
      uint32_t aspect_ratio = 1;
      assert(_mesa_is_pow_two(cpp));

      switch (cpp) {
      case 1:
         *tile_h = 64;
         break;
      case 2:
      case 4:
         *tile_h = 32;
         break;
      case 8:
      case 16:
         *tile_h = 16;
         break;
      default:
         unreachable("not reached");
      }

      if (cpp == 2 || cpp == 8)
         aspect_ratio = 2;

      if (tr_mode == INTEL_MIPTREE_TRMODE_YS)
         *tile_h *= 4;

      *tile_w = *tile_h * aspect_ratio * cpp;
   }
}


/**
 * This function computes masks that may be used to select the bits of the X
 * and Y coordinates that indicate the offset within a tile.  If the BO is
 * untiled, the masks are set to 0.
 */
void
intel_get_tile_masks(uint32_t tiling, uint32_t tr_mode, uint32_t cpp,
                     uint32_t *mask_x, uint32_t *mask_y)
{
   uint32_t tile_w_bytes, tile_h;

   intel_get_tile_dims(tiling, tr_mode, cpp, &tile_w_bytes, &tile_h);

   *mask_x = tile_w_bytes / cpp - 1;
   *mask_y = tile_h - 1;
}

/**
 * Compute the offset (in bytes) from the start of the BO to the given x
 * and y coordinate.  For tiled BOs, caller must ensure that x and y are
 * multiples of the tile size.
 */
uint32_t
intel_miptree_get_aligned_offset(const struct intel_mipmap_tree *mt,
                                 uint32_t x, uint32_t y)
{
   int cpp = mt->cpp;
   uint32_t pitch = mt->pitch;
   uint32_t tiling = mt->tiling;

   switch (tiling) {
   default:
      unreachable("not reached");
   case I915_TILING_NONE:
      return y * pitch + x * cpp;
   case I915_TILING_X:
      assert((x % (512 / cpp)) == 0);
      assert((y % 8) == 0);
      return y * pitch + x / (512 / cpp) * 4096;
   case I915_TILING_Y:
      assert((x % (128 / cpp)) == 0);
      assert((y % 32) == 0);
      return y * pitch + x / (128 / cpp) * 4096;
   }
}

/**
 * Rendering with tiled buffers requires that the base address of the buffer
 * be aligned to a page boundary.  For renderbuffers, and sometimes with
 * textures, we may want the surface to point at a texture image level that
 * isn't at a page boundary.
 *
 * This function returns an appropriately-aligned base offset
 * according to the tiling restrictions, plus any required x/y offset
 * from there.
 */
uint32_t
intel_miptree_get_tile_offsets(const struct intel_mipmap_tree *mt,
                               GLuint level, GLuint slice,
                               uint32_t *tile_x,
                               uint32_t *tile_y)
{
   uint32_t x, y;
   uint32_t mask_x, mask_y;

   intel_get_tile_masks(mt->tiling, mt->tr_mode, mt->cpp, &mask_x, &mask_y);
   intel_miptree_get_image_offset(mt, level, slice, &x, &y);

   *tile_x = x & mask_x;
   *tile_y = y & mask_y;

   return intel_miptree_get_aligned_offset(mt, x & ~mask_x, y & ~mask_y);
}

static void
intel_miptree_copy_slice_sw(struct brw_context *brw,
                            struct intel_mipmap_tree *dst_mt,
                            struct intel_mipmap_tree *src_mt,
                            int level,
                            int slice,
                            int width,
                            int height)
{
   void *src, *dst;
   ptrdiff_t src_stride, dst_stride;
   int cpp = dst_mt->cpp;

   intel_miptree_map(brw, src_mt,
                     level, slice,
                     0, 0,
                     width, height,
                     GL_MAP_READ_BIT | BRW_MAP_DIRECT_BIT,
                     &src, &src_stride);

   intel_miptree_map(brw, dst_mt,
                     level, slice,
                     0, 0,
                     width, height,
                     GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT |
                     BRW_MAP_DIRECT_BIT,
                     &dst, &dst_stride);

   DBG("sw blit %s mt %p %p/%"PRIdPTR" -> %s mt %p %p/%"PRIdPTR" (%dx%d)\n",
       _mesa_get_format_name(src_mt->format),
       src_mt, src, src_stride,
       _mesa_get_format_name(dst_mt->format),
       dst_mt, dst, dst_stride,
       width, height);

   int row_size = cpp * width;
   if (src_stride == row_size &&
       dst_stride == row_size) {
      memcpy(dst, src, row_size * height);
   } else {
      for (int i = 0; i < height; i++) {
         memcpy(dst, src, row_size);
         dst += dst_stride;
         src += src_stride;
      }
   }

   intel_miptree_unmap(brw, dst_mt, level, slice);
   intel_miptree_unmap(brw, src_mt, level, slice);

   /* Don't forget to copy the stencil data over, too.  We could have skipped
    * passing BRW_MAP_DIRECT_BIT, but that would have meant intel_miptree_map
    * shuffling the two data sources in/out of temporary storage instead of
    * the direct mapping we get this way.
    */
   if (dst_mt->stencil_mt) {
      assert(src_mt->stencil_mt);
      intel_miptree_copy_slice_sw(brw, dst_mt->stencil_mt, src_mt->stencil_mt,
                                  level, slice, width, height);
   }
}

static void
intel_miptree_copy_slice(struct brw_context *brw,
			 struct intel_mipmap_tree *dst_mt,
			 struct intel_mipmap_tree *src_mt,
			 int level,
			 int face,
			 int depth)

{
   mesa_format format = src_mt->format;
   uint32_t width = minify(src_mt->physical_width0, level - src_mt->first_level);
   uint32_t height = minify(src_mt->physical_height0, level - src_mt->first_level);
   int slice;

   if (face > 0)
      slice = face;
   else
      slice = depth;

   assert(depth < src_mt->level[level].depth);
   assert(src_mt->format == dst_mt->format);

   if (dst_mt->compressed) {
      unsigned int i, j;
      _mesa_get_format_block_size(dst_mt->format, &i, &j);
      height = ALIGN_NPOT(height, j) / j;
      width = ALIGN_NPOT(width, i) / i;
   }

   /* If it's a packed depth/stencil buffer with separate stencil, the blit
    * below won't apply since we can't do the depth's Y tiling or the
    * stencil's W tiling in the blitter.
    */
   if (src_mt->stencil_mt) {
      intel_miptree_copy_slice_sw(brw,
                                  dst_mt, src_mt,
                                  level, slice,
                                  width, height);
      return;
   }

   uint32_t dst_x, dst_y, src_x, src_y;
   intel_miptree_get_image_offset(dst_mt, level, slice, &dst_x, &dst_y);
   intel_miptree_get_image_offset(src_mt, level, slice, &src_x, &src_y);

   DBG("validate blit mt %s %p %d,%d/%d -> mt %s %p %d,%d/%d (%dx%d)\n",
       _mesa_get_format_name(src_mt->format),
       src_mt, src_x, src_y, src_mt->pitch,
       _mesa_get_format_name(dst_mt->format),
       dst_mt, dst_x, dst_y, dst_mt->pitch,
       width, height);

   if (!intel_miptree_blit(brw,
                           src_mt, level, slice, 0, 0, false,
                           dst_mt, level, slice, 0, 0, false,
                           width, height, GL_COPY)) {
      perf_debug("miptree validate blit for %s failed\n",
                 _mesa_get_format_name(format));

      intel_miptree_copy_slice_sw(brw, dst_mt, src_mt, level, slice,
                                  width, height);
   }
}

/**
 * Copies the image's current data to the given miptree, and associates that
 * miptree with the image.
 *
 * If \c invalidate is true, then the actual image data does not need to be
 * copied, but the image still needs to be associated to the new miptree (this
 * is set to true if we're about to clear the image).
 */
void
intel_miptree_copy_teximage(struct brw_context *brw,
			    struct intel_texture_image *intelImage,
			    struct intel_mipmap_tree *dst_mt,
                            bool invalidate)
{
   struct intel_mipmap_tree *src_mt = intelImage->mt;
   struct intel_texture_object *intel_obj =
      intel_texture_object(intelImage->base.Base.TexObject);
   int level = intelImage->base.Base.Level;
   int face = intelImage->base.Base.Face;

   GLuint depth;
   if (intel_obj->base.Target == GL_TEXTURE_1D_ARRAY)
      depth = intelImage->base.Base.Height;
   else
      depth = intelImage->base.Base.Depth;

   if (!invalidate) {
      for (int slice = 0; slice < depth; slice++) {
         intel_miptree_copy_slice(brw, dst_mt, src_mt, level, face, slice);
      }
   }

   intel_miptree_reference(&intelImage->mt, dst_mt);
   intel_obj->needs_validate = true;
}

static void
intel_miptree_init_mcs(struct brw_context *brw,
                       struct intel_mipmap_tree *mt,
                       int init_value)
{
   assert(mt->mcs_buf != NULL);

   /* From the Ivy Bridge PRM, Vol 2 Part 1 p326:
    *
    *     When MCS buffer is enabled and bound to MSRT, it is required that it
    *     is cleared prior to any rendering.
    *
    * Since we don't use the MCS buffer for any purpose other than rendering,
    * it makes sense to just clear it immediately upon allocation.
    *
    * Note: the clear value for MCS buffers is all 1's, so we memset to 0xff.
    */
   const int ret = brw_bo_map_gtt(brw, mt->mcs_buf->bo, "miptree");
   if (unlikely(ret)) {
      fprintf(stderr, "Failed to map mcs buffer into GTT\n");
      drm_intel_bo_unreference(mt->mcs_buf->bo);
      free(mt->mcs_buf);
      return;
   }
   void *data = mt->mcs_buf->bo->virtual;
   memset(data, init_value, mt->mcs_buf->size);
   drm_intel_bo_unmap(mt->mcs_buf->bo);
}

static struct intel_miptree_aux_buffer *
intel_mcs_miptree_buf_create(struct brw_context *brw,
                             struct intel_mipmap_tree *mt,
                             mesa_format format,
                             unsigned mcs_width,
                             unsigned mcs_height,
                             uint32_t layout_flags)
{
   struct intel_miptree_aux_buffer *buf = calloc(sizeof(*buf), 1);
   struct intel_mipmap_tree *temp_mt;

   if (!buf)
      return NULL;

   /* From the Ivy Bridge PRM, Vol4 Part1 p76, "MCS Base Address":
    *
    *     "The MCS surface must be stored as Tile Y."
    */
   layout_flags |= MIPTREE_LAYOUT_TILING_Y;
   temp_mt = miptree_create(brw,
                            mt->target,
                            format,
                            mt->first_level,
                            mt->last_level,
                            mcs_width,
                            mcs_height,
                            mt->logical_depth0,
                            0 /* num_samples */,
                            layout_flags);
   if (!temp_mt) {
      free(buf);
      return NULL;
   }

   buf->bo = temp_mt->bo;
   buf->offset = temp_mt->offset;
   buf->size = temp_mt->total_height * temp_mt->pitch;
   buf->pitch = temp_mt->pitch;
   buf->qpitch = temp_mt->qpitch;

   /* Just hang on to the BO which backs the AUX buffer; the rest of the miptree
    * structure should go away. We use miptree create simply as a means to make
    * sure all the constraints for the buffer are satisfied.
    */
   drm_intel_bo_reference(temp_mt->bo);
   intel_miptree_release(&temp_mt);

   return buf;
}

static bool
intel_miptree_alloc_mcs(struct brw_context *brw,
                        struct intel_mipmap_tree *mt,
                        GLuint num_samples)
{
   assert(brw->gen >= 7); /* MCS only used on Gen7+ */
   assert(mt->mcs_buf == NULL);
   assert((mt->aux_disable & INTEL_AUX_DISABLE_MCS) == 0);

   /* Choose the correct format for the MCS buffer.  All that really matters
    * is that we allocate the right buffer size, since we'll always be
    * accessing this miptree using MCS-specific hardware mechanisms, which
    * infer the correct format based on num_samples.
    */
   mesa_format format;
   switch (num_samples) {
   case 2:
   case 4:
      /* 8 bits/pixel are required for MCS data when using 4x MSAA (2 bits for
       * each sample).
       */
      format = MESA_FORMAT_R_UNORM8;
      break;
   case 8:
      /* 32 bits/pixel are required for MCS data when using 8x MSAA (3 bits
       * for each sample, plus 8 padding bits).
       */
      format = MESA_FORMAT_R_UINT32;
      break;
   case 16:
      /* 64 bits/pixel are required for MCS data when using 16x MSAA (4 bits
       * for each sample).
       */
      format = MESA_FORMAT_RG_UINT32;
      break;
   default:
      unreachable("Unrecognized sample count in intel_miptree_alloc_mcs");
   };

   mt->mcs_buf =
      intel_mcs_miptree_buf_create(brw, mt,
                                   format,
                                   mt->logical_width0,
                                   mt->logical_height0,
                                   MIPTREE_LAYOUT_ACCELERATED_UPLOAD);
   if (!mt->mcs_buf)
      return false;

   intel_miptree_init_mcs(brw, mt, 0xFF);

   /* Multisampled miptrees are only supported for single level. */
   assert(mt->first_level == 0);
   intel_miptree_set_fast_clear_state(brw, mt, mt->first_level, 0,
                                      mt->logical_depth0,
                                      INTEL_FAST_CLEAR_STATE_CLEAR);

   return true;
}


bool
intel_miptree_alloc_non_msrt_mcs(struct brw_context *brw,
                                 struct intel_mipmap_tree *mt,
                                 bool is_lossless_compressed)
{
   assert(mt->mcs_buf == NULL);
   assert(!(mt->aux_disable & (INTEL_AUX_DISABLE_MCS | INTEL_AUX_DISABLE_CCS)));

   struct isl_surf temp_main_surf;
   struct isl_surf temp_ccs_surf;

   /* Create first an ISL presentation for the main color surface and let ISL
    * calculate equivalent CCS surface against it.
    */
   intel_miptree_get_isl_surf(brw, mt, &temp_main_surf);
   if (!isl_surf_get_ccs_surf(&brw->isl_dev, &temp_main_surf, &temp_ccs_surf))
      return false;

   assert(temp_ccs_surf.size &&
          (temp_ccs_surf.size % temp_ccs_surf.row_pitch == 0));

   struct intel_miptree_aux_buffer *buf = calloc(sizeof(*buf), 1);
   if (!buf)
      return false;

   buf->size = temp_ccs_surf.size;
   buf->pitch = temp_ccs_surf.row_pitch;
   buf->qpitch = isl_surf_get_array_pitch_sa_rows(&temp_ccs_surf);

   /* In case of compression mcs buffer needs to be initialised requiring the
    * buffer to be immediately mapped to cpu space for writing. Therefore do
    * not use the gpu access flag which can cause an unnecessary delay if the
    * backing pages happened to be just used by the GPU.
    */
   const uint32_t alloc_flags =
      is_lossless_compressed ? 0 : BO_ALLOC_FOR_RENDER;
   uint32_t tiling = I915_TILING_Y;
   unsigned long pitch;

   /* ISL has stricter set of alignment rules then the drm allocator.
    * Therefore one can pass the ISL dimensions in terms of bytes instead of
    * trying to recalculate based on different format block sizes.
    */
   buf->bo = drm_intel_bo_alloc_tiled(brw->bufmgr, "ccs-miptree",
                                      buf->pitch, buf->size / buf->pitch,
                                      1, &tiling, &pitch, alloc_flags);
   if (buf->bo) {
      assert(pitch == buf->pitch);
      assert(tiling == I915_TILING_Y);
   } else {
      free(buf);
      return false;
   }

   mt->mcs_buf = buf;

   /* From Gen9 onwards single-sampled (non-msrt) auxiliary buffers are
    * used for lossless compression which requires similar initialisation
    * as multi-sample compression.
    */
   if (is_lossless_compressed) {
      /* Hardware sets the auxiliary buffer to all zeroes when it does full
       * resolve. Initialize it accordingly in case the first renderer is
       * cpu (or other none compression aware party).
       *
       * This is also explicitly stated in the spec (MCS Buffer for Render
       * Target(s)):
       *   "If Software wants to enable Color Compression without Fast clear,
       *    Software needs to initialize MCS with zeros."
       */
      intel_miptree_init_mcs(brw, mt, 0);
      mt->msaa_layout = INTEL_MSAA_LAYOUT_CMS;
   }

   return true;
}

/**
 * Helper for intel_miptree_alloc_hiz() that sets
 * \c mt->level[level].has_hiz. Return true if and only if
 * \c has_hiz was set.
 */
static bool
intel_miptree_level_enable_hiz(struct brw_context *brw,
                               struct intel_mipmap_tree *mt,
                               uint32_t level)
{
   assert(mt->hiz_buf);

   if (brw->gen >= 8 || brw->is_haswell) {
      uint32_t width = minify(mt->physical_width0, level);
      uint32_t height = minify(mt->physical_height0, level);

      /* Disable HiZ for LOD > 0 unless the width is 8 aligned
       * and the height is 4 aligned. This allows our HiZ support
       * to fulfill Haswell restrictions for HiZ ops. For LOD == 0,
       * we can grow the width & height to allow the HiZ op to
       * force the proper size alignments.
       */
      if (level > 0 && ((width & 7) || (height & 3))) {
         DBG("mt %p level %d: HiZ DISABLED\n", mt, level);
         return false;
      }
   }

   DBG("mt %p level %d: HiZ enabled\n", mt, level);
   mt->level[level].has_hiz = true;
   return true;
}


/**
 * Helper for intel_miptree_alloc_hiz() that determines the required hiz
 * buffer dimensions and allocates a bo for the hiz buffer.
 */
static struct intel_miptree_hiz_buffer *
intel_gen7_hiz_buf_create(struct brw_context *brw,
                          struct intel_mipmap_tree *mt)
{
   unsigned z_width = mt->logical_width0;
   unsigned z_height = mt->logical_height0;
   const unsigned z_depth = MAX2(mt->logical_depth0, 1);
   unsigned hz_width, hz_height;
   struct intel_miptree_hiz_buffer *buf = calloc(sizeof(*buf), 1);

   if (!buf)
      return NULL;

   /* Gen7 PRM Volume 2, Part 1, 11.5.3 "Hierarchical Depth Buffer" documents
    * adjustments required for Z_Height and Z_Width based on multisampling.
    */
   switch (mt->num_samples) {
   case 0:
   case 1:
      break;
   case 2:
   case 4:
      z_width *= 2;
      z_height *= 2;
      break;
   case 8:
      z_width *= 4;
      z_height *= 2;
      break;
   default:
      unreachable("unsupported sample count");
   }

   const unsigned vertical_align = 8; /* 'j' in the docs */
   const unsigned H0 = z_height;
   const unsigned h0 = ALIGN(H0, vertical_align);
   const unsigned h1 = ALIGN(minify(H0, 1), vertical_align);
   const unsigned Z0 = z_depth;

   /* HZ_Width (bytes) = ceiling(Z_Width / 16) * 16 */
   hz_width = ALIGN(z_width, 16);

   if (mt->target == GL_TEXTURE_3D) {
      unsigned H_i = H0;
      unsigned Z_i = Z0;
      hz_height = 0;
      for (unsigned level = mt->first_level; level <= mt->last_level; ++level) {
         unsigned h_i = ALIGN(H_i, vertical_align);
         /* sum(i=0 to m; h_i * max(1, floor(Z_Depth/2**i))) */
         hz_height += h_i * Z_i;
         H_i = minify(H_i, 1);
         Z_i = minify(Z_i, 1);
      }
      /* HZ_Height =
       *    (1/2) * sum(i=0 to m; h_i * max(1, floor(Z_Depth/2**i)))
       */
      hz_height = DIV_ROUND_UP(hz_height, 2);
   } else {
      const unsigned hz_qpitch = h0 + h1 + (12 * vertical_align);
      /* HZ_Height (rows) = Ceiling ( ( Q_pitch * Z_depth/2) /8 ) * 8 */
      hz_height = DIV_ROUND_UP(hz_qpitch * Z0, 2 * 8) * 8;
   }

   unsigned long pitch;
   uint32_t tiling = I915_TILING_Y;
   buf->aux_base.bo = drm_intel_bo_alloc_tiled(brw->bufmgr, "hiz",
                                               hz_width, hz_height, 1,
                                               &tiling, &pitch,
                                               BO_ALLOC_FOR_RENDER);
   if (!buf->aux_base.bo) {
      free(buf);
      return NULL;
   } else if (tiling != I915_TILING_Y) {
      drm_intel_bo_unreference(buf->aux_base.bo);
      free(buf);
      return NULL;
   }

   buf->aux_base.size = hz_width * hz_height;
   buf->aux_base.pitch = pitch;

   return buf;
}


/**
 * Helper for intel_miptree_alloc_hiz() that determines the required hiz
 * buffer dimensions and allocates a bo for the hiz buffer.
 */
static struct intel_miptree_hiz_buffer *
intel_gen8_hiz_buf_create(struct brw_context *brw,
                          struct intel_mipmap_tree *mt)
{
   unsigned z_width = mt->logical_width0;
   unsigned z_height = mt->logical_height0;
   const unsigned z_depth = MAX2(mt->logical_depth0, 1);
   unsigned hz_width, hz_height;
   struct intel_miptree_hiz_buffer *buf = calloc(sizeof(*buf), 1);

   if (!buf)
      return NULL;

   /* Gen7 PRM Volume 2, Part 1, 11.5.3 "Hierarchical Depth Buffer" documents
    * adjustments required for Z_Height and Z_Width based on multisampling.
    */
   if (brw->gen < 9) {
      switch (mt->num_samples) {
      case 0:
      case 1:
         break;
      case 2:
      case 4:
         z_width *= 2;
         z_height *= 2;
         break;
      case 8:
         z_width *= 4;
         z_height *= 2;
         break;
      default:
         unreachable("unsupported sample count");
      }
   }

   const unsigned vertical_align = 8; /* 'j' in the docs */
   const unsigned H0 = z_height;
   const unsigned h0 = ALIGN(H0, vertical_align);
   const unsigned h1 = ALIGN(minify(H0, 1), vertical_align);
   const unsigned Z0 = z_depth;

   /* HZ_Width (bytes) = ceiling(Z_Width / 16) * 16 */
   hz_width = ALIGN(z_width, 16);

   unsigned H_i = H0;
   unsigned Z_i = Z0;
   unsigned sum_h_i = 0;
   unsigned hz_height_3d_sum = 0;
   for (unsigned level = mt->first_level; level <= mt->last_level; ++level) {
      unsigned i = level - mt->first_level;
      unsigned h_i = ALIGN(H_i, vertical_align);
      /* sum(i=2 to m; h_i) */
      if (i >= 2) {
         sum_h_i += h_i;
      }
      /* sum(i=0 to m; h_i * max(1, floor(Z_Depth/2**i))) */
      hz_height_3d_sum += h_i * Z_i;
      H_i = minify(H_i, 1);
      Z_i = minify(Z_i, 1);
   }
   /* HZ_QPitch = h0 + max(h1, sum(i=2 to m; h_i)) */
   buf->aux_base.qpitch = h0 + MAX2(h1, sum_h_i);

   if (mt->target == GL_TEXTURE_3D) {
      /* (1/2) * sum(i=0 to m; h_i * max(1, floor(Z_Depth/2**i))) */
      hz_height = DIV_ROUND_UP(hz_height_3d_sum, 2);
   } else {
      /* HZ_Height (rows) = ceiling( (HZ_QPitch/2)/8) *8 * Z_Depth */
      hz_height = DIV_ROUND_UP(buf->aux_base.qpitch, 2 * 8) * 8 * Z0;
   }

   unsigned long pitch;
   uint32_t tiling = I915_TILING_Y;
   buf->aux_base.bo = drm_intel_bo_alloc_tiled(brw->bufmgr, "hiz",
                                               hz_width, hz_height, 1,
                                               &tiling, &pitch,
                                               BO_ALLOC_FOR_RENDER);
   if (!buf->aux_base.bo) {
      free(buf);
      return NULL;
   } else if (tiling != I915_TILING_Y) {
      drm_intel_bo_unreference(buf->aux_base.bo);
      free(buf);
      return NULL;
   }

   buf->aux_base.size = hz_width * hz_height;
   buf->aux_base.pitch = pitch;

   return buf;
}


static struct intel_miptree_hiz_buffer *
intel_hiz_miptree_buf_create(struct brw_context *brw,
                             struct intel_mipmap_tree *mt)
{
   struct intel_miptree_hiz_buffer *buf = calloc(sizeof(*buf), 1);
   uint32_t layout_flags = MIPTREE_LAYOUT_ACCELERATED_UPLOAD;

   if (brw->gen == 6)
      layout_flags |= MIPTREE_LAYOUT_FORCE_ALL_SLICE_AT_LOD;

   if (!buf)
      return NULL;

   layout_flags |= MIPTREE_LAYOUT_TILING_ANY;
   buf->mt = intel_miptree_create(brw,
                                  mt->target,
                                  mt->format,
                                  mt->first_level,
                                  mt->last_level,
                                  mt->logical_width0,
                                  mt->logical_height0,
                                  mt->logical_depth0,
                                  mt->num_samples,
                                  layout_flags);
   if (!buf->mt) {
      free(buf);
      return NULL;
   }

   buf->aux_base.bo = buf->mt->bo;
   buf->aux_base.size = buf->mt->total_height * buf->mt->pitch;
   buf->aux_base.pitch = buf->mt->pitch;
   buf->aux_base.qpitch = buf->mt->qpitch;

   return buf;
}

bool
intel_miptree_wants_hiz_buffer(struct brw_context *brw,
                               struct intel_mipmap_tree *mt)
{
   if (!brw->has_hiz)
      return false;

   if (mt->hiz_buf != NULL)
      return false;

   if (mt->aux_disable & INTEL_AUX_DISABLE_HIZ)
      return false;

   switch (mt->format) {
   case MESA_FORMAT_Z_FLOAT32:
   case MESA_FORMAT_Z32_FLOAT_S8X24_UINT:
   case MESA_FORMAT_Z24_UNORM_X8_UINT:
   case MESA_FORMAT_Z24_UNORM_S8_UINT:
   case MESA_FORMAT_Z_UNORM16:
      return true;
   default:
      return false;
   }
}

bool
intel_miptree_alloc_hiz(struct brw_context *brw,
			struct intel_mipmap_tree *mt)
{
   assert(mt->hiz_buf == NULL);
   assert((mt->aux_disable & INTEL_AUX_DISABLE_HIZ) == 0);

   if (brw->gen == 7) {
      mt->hiz_buf = intel_gen7_hiz_buf_create(brw, mt);
   } else if (brw->gen >= 8) {
      mt->hiz_buf = intel_gen8_hiz_buf_create(brw, mt);
   } else {
      mt->hiz_buf = intel_hiz_miptree_buf_create(brw, mt);
   }

   if (!mt->hiz_buf)
      return false;

   /* Mark that all slices need a HiZ resolve. */
   for (unsigned level = mt->first_level; level <= mt->last_level; ++level) {
      if (!intel_miptree_level_enable_hiz(brw, mt, level))
         continue;

      for (unsigned layer = 0; layer < mt->level[level].depth; ++layer) {
         struct intel_resolve_map *m = malloc(sizeof(struct intel_resolve_map));
         exec_node_init(&m->link);
         m->level = level;
         m->layer = layer;
         m->need = BLORP_HIZ_OP_HIZ_RESOLVE;

         exec_list_push_tail(&mt->hiz_map, &m->link);
      }
   }

   return true;
}

/**
 * Can the miptree sample using the hiz buffer?
 */
bool
intel_miptree_sample_with_hiz(struct brw_context *brw,
                              struct intel_mipmap_tree *mt)
{
   /* It's unclear how well supported sampling from the hiz buffer is on GEN8,
    * so keep things conservative for now and never enable it unless we're SKL+.
    */
   if (brw->gen < 9) {
      return false;
   }

   if (!mt->hiz_buf) {
      return false;
   }

   /* It seems the hardware won't fallback to the depth buffer if some of the
    * mipmap levels aren't available in the HiZ buffer. So we need all levels
    * of the texture to be HiZ enabled.
    */
   for (unsigned level = mt->first_level; level <= mt->last_level; ++level) {
      if (!intel_miptree_level_has_hiz(mt, level))
         return false;
   }

   /* If compressed multisampling is enabled, then we use it for the auxiliary
    * buffer instead.
    *
    * From the BDW PRM (Volume 2d: Command Reference: Structures
    *                   RENDER_SURFACE_STATE.AuxiliarySurfaceMode):
    *
    *  "If this field is set to AUX_HIZ, Number of Multisamples must be
    *   MULTISAMPLECOUNT_1, and Surface Type cannot be SURFTYPE_3D.
    *
    * There is no such blurb for 1D textures, but there is sufficient evidence
    * that this is broken on SKL+.
    */
   return (mt->num_samples <= 1 &&
           mt->target != GL_TEXTURE_3D &&
           mt->target != GL_TEXTURE_1D /* gen9+ restriction */);
}

/**
 * Does the miptree slice have hiz enabled?
 */
bool
intel_miptree_level_has_hiz(struct intel_mipmap_tree *mt, uint32_t level)
{
   intel_miptree_check_level_layer(mt, level, 0);
   return mt->level[level].has_hiz;
}

void
intel_miptree_slice_set_needs_hiz_resolve(struct intel_mipmap_tree *mt,
					  uint32_t level,
					  uint32_t layer)
{
   if (!intel_miptree_level_has_hiz(mt, level))
      return;

   intel_resolve_map_set(&mt->hiz_map,
			 level, layer, BLORP_HIZ_OP_HIZ_RESOLVE);
}


void
intel_miptree_slice_set_needs_depth_resolve(struct intel_mipmap_tree *mt,
                                            uint32_t level,
                                            uint32_t layer)
{
   if (!intel_miptree_level_has_hiz(mt, level))
      return;

   intel_resolve_map_set(&mt->hiz_map,
			 level, layer, BLORP_HIZ_OP_DEPTH_RESOLVE);
}

void
intel_miptree_set_all_slices_need_depth_resolve(struct intel_mipmap_tree *mt,
                                                uint32_t level)
{
   uint32_t layer;
   uint32_t end_layer = mt->level[level].depth;

   for (layer = 0; layer < end_layer; layer++) {
      intel_miptree_slice_set_needs_depth_resolve(mt, level, layer);
   }
}

static bool
intel_miptree_slice_resolve(struct brw_context *brw,
			    struct intel_mipmap_tree *mt,
			    uint32_t level,
			    uint32_t layer,
			    enum blorp_hiz_op need)
{
   intel_miptree_check_level_layer(mt, level, layer);

   struct intel_resolve_map *item =
	 intel_resolve_map_get(&mt->hiz_map, level, layer);

   if (!item || item->need != need)
      return false;

   intel_hiz_exec(brw, mt, level, layer, need);
   intel_resolve_map_remove(item);
   return true;
}

bool
intel_miptree_slice_resolve_hiz(struct brw_context *brw,
				struct intel_mipmap_tree *mt,
				uint32_t level,
				uint32_t layer)
{
   return intel_miptree_slice_resolve(brw, mt, level, layer,
				      BLORP_HIZ_OP_HIZ_RESOLVE);
}

bool
intel_miptree_slice_resolve_depth(struct brw_context *brw,
				  struct intel_mipmap_tree *mt,
				  uint32_t level,
				  uint32_t layer)
{
   return intel_miptree_slice_resolve(brw, mt, level, layer,
				      BLORP_HIZ_OP_DEPTH_RESOLVE);
}

static bool
intel_miptree_all_slices_resolve(struct brw_context *brw,
				 struct intel_mipmap_tree *mt,
				 enum blorp_hiz_op need)
{
   bool did_resolve = false;

   foreach_list_typed_safe(struct intel_resolve_map, map, link, &mt->hiz_map) {
      if (map->need != need)
	 continue;

      intel_hiz_exec(brw, mt, map->level, map->layer, need);
      intel_resolve_map_remove(map);
      did_resolve = true;
   }

   return did_resolve;
}

bool
intel_miptree_all_slices_resolve_hiz(struct brw_context *brw,
				     struct intel_mipmap_tree *mt)
{
   return intel_miptree_all_slices_resolve(brw, mt,
					   BLORP_HIZ_OP_HIZ_RESOLVE);
}

bool
intel_miptree_all_slices_resolve_depth(struct brw_context *brw,
				       struct intel_mipmap_tree *mt)
{
   return intel_miptree_all_slices_resolve(brw, mt,
					   BLORP_HIZ_OP_DEPTH_RESOLVE);
}

enum intel_fast_clear_state
intel_miptree_get_fast_clear_state(const struct intel_mipmap_tree *mt,
                                   unsigned level, unsigned layer)
{
   intel_miptree_check_level_layer(mt, level, layer);

   const struct intel_resolve_map *item =
      intel_resolve_map_const_get(&mt->color_resolve_map, level, layer);

   if (!item)
      return INTEL_FAST_CLEAR_STATE_RESOLVED;

   return item->fast_clear_state;
}

static void
intel_miptree_check_color_resolve(const struct brw_context *brw,
                                  const struct intel_mipmap_tree *mt,
                                  unsigned level, unsigned layer)
{

   if ((mt->aux_disable & INTEL_AUX_DISABLE_CCS) || !mt->mcs_buf)
      return;

   /* Fast color clear is supported for mipmapped surfaces only on Gen8+. */
   assert(brw->gen >= 8 ||
          (level == 0 && mt->first_level == 0 && mt->last_level == 0));

   /* Compression of arrayed msaa surfaces is supported. */
   if (mt->num_samples > 1)
      return;

   /* Fast color clear is supported for non-msaa arrays only on Gen8+. */
   assert(brw->gen >= 8 || (layer == 0 && mt->logical_depth0 == 1));

   (void)level;
   (void)layer;
}

void
intel_miptree_set_fast_clear_state(const struct brw_context *brw,
                                   struct intel_mipmap_tree *mt,
                                   unsigned level,
                                   unsigned first_layer,
                                   unsigned num_layers,
                                   enum intel_fast_clear_state new_state)
{
   /* Setting the state to resolved means removing the item from the list
    * altogether.
    */
   assert(new_state != INTEL_FAST_CLEAR_STATE_RESOLVED);

   intel_miptree_check_color_resolve(brw, mt, level, first_layer);

   assert(first_layer + num_layers <= mt->physical_depth0);

   for (unsigned i = 0; i < num_layers; i++)
      intel_resolve_map_set(&mt->color_resolve_map, level,
                            first_layer + i, new_state);
}

bool
intel_miptree_has_color_unresolved(const struct intel_mipmap_tree *mt,
                                   unsigned start_level, unsigned num_levels,
                                   unsigned start_layer, unsigned num_layers)
{
   return intel_resolve_map_find_any(&mt->color_resolve_map,
                                     start_level, num_levels,
                                     start_layer, num_layers) != NULL;
}

void
intel_miptree_used_for_rendering(const struct brw_context *brw,
                                 struct intel_mipmap_tree *mt, unsigned level,
                                 unsigned start_layer, unsigned num_layers)
{
   const bool is_lossless_compressed =
      intel_miptree_is_lossless_compressed(brw, mt);

   for (unsigned i = 0; i < num_layers; ++i) {
      const enum intel_fast_clear_state fast_clear_state =
         intel_miptree_get_fast_clear_state(mt, level, start_layer + i);

      /* If the buffer was previously in fast clear state, change it to
       * unresolved state, since it won't be guaranteed to be clear after
       * rendering occurs.
       */
      if (is_lossless_compressed ||
          fast_clear_state == INTEL_FAST_CLEAR_STATE_CLEAR) {
         intel_miptree_set_fast_clear_state(
            brw, mt, level, start_layer + i, 1,
            INTEL_FAST_CLEAR_STATE_UNRESOLVED);
      }
   }
}

static bool
intel_miptree_needs_color_resolve(const struct brw_context *brw,
                                  const struct intel_mipmap_tree *mt,
                                  int flags)
{
   if (mt->aux_disable & INTEL_AUX_DISABLE_CCS)
      return false;

   const bool is_lossless_compressed =
      intel_miptree_is_lossless_compressed(brw, mt);

   /* From gen9 onwards there is new compression scheme for single sampled
    * surfaces called "lossless compressed". These don't need to be always
    * resolved.
    */
   if ((flags & INTEL_MIPTREE_IGNORE_CCS_E) && is_lossless_compressed)
      return false;

   /* Fast color clear resolves only make sense for non-MSAA buffers. */
   if (mt->msaa_layout != INTEL_MSAA_LAYOUT_NONE && !is_lossless_compressed)
      return false;

   return true;
}

bool
intel_miptree_resolve_color(struct brw_context *brw,
                            struct intel_mipmap_tree *mt, unsigned level,
                            unsigned start_layer, unsigned num_layers,
                            int flags)
{
   intel_miptree_check_color_resolve(brw, mt, level, start_layer);

   if (!intel_miptree_needs_color_resolve(brw, mt, flags))
      return false;

   /* Arrayed fast clear is only supported for gen8+. */
   assert(brw->gen >= 8 || num_layers == 1);

   bool resolved = false;
   for (unsigned i = 0; i < num_layers; ++i) {
      intel_miptree_check_level_layer(mt, level, start_layer + i);

      struct intel_resolve_map *item =
         intel_resolve_map_get(&mt->color_resolve_map, level,
                               start_layer + i);

      if (item) {
         assert(item->fast_clear_state != INTEL_FAST_CLEAR_STATE_RESOLVED);

         brw_blorp_resolve_color(brw, mt, level, start_layer);
         intel_resolve_map_remove(item);
         resolved = true;
      }
   }

   return resolved;
}

void
intel_miptree_all_slices_resolve_color(struct brw_context *brw,
                                       struct intel_mipmap_tree *mt,
                                       int flags)
{
   if (!intel_miptree_needs_color_resolve(brw, mt, flags))
      return;
      
   foreach_list_typed_safe(struct intel_resolve_map, map, link,
                           &mt->color_resolve_map) {
      assert(map->fast_clear_state != INTEL_FAST_CLEAR_STATE_RESOLVED);

      brw_blorp_resolve_color(brw, mt, map->level, map->layer);
      intel_resolve_map_remove(map);
   }
}

/**
 * Make it possible to share the BO backing the given miptree with another
 * process or another miptree.
 *
 * Fast color clears are unsafe with shared buffers, so we need to resolve and
 * then discard the MCS buffer, if present.  We also set the no_ccs flag to
 * ensure that no MCS buffer gets allocated in the future.
 *
 * HiZ is similarly unsafe with shared buffers.
 */
void
intel_miptree_make_shareable(struct brw_context *brw,
                             struct intel_mipmap_tree *mt)
{
   /* MCS buffers are also used for multisample buffers, but we can't resolve
    * away a multisample MCS buffer because it's an integral part of how the
    * pixel data is stored.  Fortunately this code path should never be
    * reached for multisample buffers.
    */
   assert(mt->msaa_layout == INTEL_MSAA_LAYOUT_NONE || mt->num_samples <= 1);

   if (mt->mcs_buf) {
      intel_miptree_all_slices_resolve_color(brw, mt, 0);
      mt->aux_disable |= (INTEL_AUX_DISABLE_CCS | INTEL_AUX_DISABLE_MCS);
      drm_intel_bo_unreference(mt->mcs_buf->bo);
      free(mt->mcs_buf);
      mt->mcs_buf = NULL;
   }

   if (mt->hiz_buf) {
      mt->aux_disable |= INTEL_AUX_DISABLE_HIZ;
      intel_miptree_all_slices_resolve_depth(brw, mt);
      intel_miptree_hiz_buffer_free(mt->hiz_buf);
      mt->hiz_buf = NULL;
   }
}


/**
 * \brief Get pointer offset into stencil buffer.
 *
 * The stencil buffer is W tiled. Since the GTT is incapable of W fencing, we
 * must decode the tile's layout in software.
 *
 * See
 *   - PRM, 2011 Sandy Bridge, Volume 1, Part 2, Section 4.5.2.1 W-Major Tile
 *     Format.
 *   - PRM, 2011 Sandy Bridge, Volume 1, Part 2, Section 4.5.3 Tiling Algorithm
 *
 * Even though the returned offset is always positive, the return type is
 * signed due to
 *    commit e8b1c6d6f55f5be3bef25084fdd8b6127517e137
 *    mesa: Fix return type of  _mesa_get_format_bytes() (#37351)
 */
static intptr_t
intel_offset_S8(uint32_t stride, uint32_t x, uint32_t y, bool swizzled)
{
   uint32_t tile_size = 4096;
   uint32_t tile_width = 64;
   uint32_t tile_height = 64;
   uint32_t row_size = 64 * stride;

   uint32_t tile_x = x / tile_width;
   uint32_t tile_y = y / tile_height;

   /* The byte's address relative to the tile's base addres. */
   uint32_t byte_x = x % tile_width;
   uint32_t byte_y = y % tile_height;

   uintptr_t u = tile_y * row_size
               + tile_x * tile_size
               + 512 * (byte_x / 8)
               +  64 * (byte_y / 8)
               +  32 * ((byte_y / 4) % 2)
               +  16 * ((byte_x / 4) % 2)
               +   8 * ((byte_y / 2) % 2)
               +   4 * ((byte_x / 2) % 2)
               +   2 * (byte_y % 2)
               +   1 * (byte_x % 2);

   if (swizzled) {
      /* adjust for bit6 swizzling */
      if (((byte_x / 8) % 2) == 1) {
	 if (((byte_y / 8) % 2) == 0) {
	    u += 64;
	 } else {
	    u -= 64;
	 }
      }
   }

   return u;
}

void
intel_miptree_updownsample(struct brw_context *brw,
                           struct intel_mipmap_tree *src,
                           struct intel_mipmap_tree *dst)
{
   brw_blorp_blit_miptrees(brw,
                           src, 0 /* level */, 0 /* layer */,
                           src->format, SWIZZLE_XYZW,
                           dst, 0 /* level */, 0 /* layer */, dst->format,
                           0, 0,
                           src->logical_width0, src->logical_height0,
                           0, 0,
                           dst->logical_width0, dst->logical_height0,
                           GL_NEAREST, false, false /*mirror x, y*/,
                           false, false);

   if (src->stencil_mt) {
      brw_blorp_blit_miptrees(brw,
                              src->stencil_mt, 0 /* level */, 0 /* layer */,
                              src->stencil_mt->format, SWIZZLE_XYZW,
                              dst->stencil_mt, 0 /* level */, 0 /* layer */,
                              dst->stencil_mt->format,
                              0, 0,
                              src->logical_width0, src->logical_height0,
                              0, 0,
                              dst->logical_width0, dst->logical_height0,
                              GL_NEAREST, false, false /*mirror x, y*/,
                              false, false /* decode/encode srgb */);
   }
}

void
intel_update_r8stencil(struct brw_context *brw,
                       struct intel_mipmap_tree *mt)
{
   assert(brw->gen >= 7);
   struct intel_mipmap_tree *src =
      mt->format == MESA_FORMAT_S_UINT8 ? mt : mt->stencil_mt;
   if (!src || brw->gen >= 8 || !src->r8stencil_needs_update)
      return;

   if (!mt->r8stencil_mt) {
      const uint32_t r8stencil_flags =
         MIPTREE_LAYOUT_ACCELERATED_UPLOAD | MIPTREE_LAYOUT_TILING_Y |
         MIPTREE_LAYOUT_DISABLE_AUX;
      assert(brw->gen > 6); /* Handle MIPTREE_LAYOUT_FORCE_ALL_SLICE_AT_LOD */
      mt->r8stencil_mt = intel_miptree_create(brw,
                                              src->target,
                                              MESA_FORMAT_R_UINT8,
                                              src->first_level,
                                              src->last_level,
                                              src->logical_width0,
                                              src->logical_height0,
                                              src->logical_depth0,
                                              src->num_samples,
                                              r8stencil_flags);
      assert(mt->r8stencil_mt);
   }

   struct intel_mipmap_tree *dst = mt->r8stencil_mt;

   for (int level = src->first_level; level <= src->last_level; level++) {
      const unsigned depth = src->level[level].depth;
      const int layers_per_blit =
         (dst->msaa_layout == INTEL_MSAA_LAYOUT_UMS ||
          dst->msaa_layout == INTEL_MSAA_LAYOUT_CMS) ?
         dst->num_samples : 1;

      for (unsigned layer = 0; layer < depth; layer++) {
         brw_blorp_blit_miptrees(brw,
                                 src, level, layer,
                                 src->format, SWIZZLE_X,
                                 dst, level, layers_per_blit * layer,
                                 MESA_FORMAT_R_UNORM8,
                                 0, 0,
                                 minify(src->logical_width0, level),
                                 minify(src->logical_height0, level),
                                 0, 0,
                                 minify(dst->logical_width0, level),
                                 minify(dst->logical_height0, level),
                                 GL_NEAREST, false, false /*mirror x, y*/,
                                 false, false /* decode/encode srgb */);
      }
   }

   brw_render_cache_set_check_flush(brw, dst->bo);
   src->r8stencil_needs_update = false;
}

static void *
intel_miptree_map_raw(struct brw_context *brw, struct intel_mipmap_tree *mt)
{
   /* CPU accesses to color buffers don't understand fast color clears, so
    * resolve any pending fast color clears before we map.
    */
   intel_miptree_all_slices_resolve_color(brw, mt, 0);

   drm_intel_bo *bo = mt->bo;

   if (drm_intel_bo_references(brw->batch.bo, bo))
      intel_batchbuffer_flush(brw);

   if (mt->tiling != I915_TILING_NONE)
      brw_bo_map_gtt(brw, bo, "miptree");
   else
      brw_bo_map(brw, bo, true, "miptree");

   return bo->virtual;
}

static void
intel_miptree_unmap_raw(struct intel_mipmap_tree *mt)
{
   drm_intel_bo_unmap(mt->bo);
}

static void
intel_miptree_map_gtt(struct brw_context *brw,
		      struct intel_mipmap_tree *mt,
		      struct intel_miptree_map *map,
		      unsigned int level, unsigned int slice)
{
   unsigned int bw, bh;
   void *base;
   unsigned int image_x, image_y;
   intptr_t x = map->x;
   intptr_t y = map->y;

   /* For compressed formats, the stride is the number of bytes per
    * row of blocks.  intel_miptree_get_image_offset() already does
    * the divide.
    */
   _mesa_get_format_block_size(mt->format, &bw, &bh);
   assert(y % bh == 0);
   assert(x % bw == 0);
   y /= bh;
   x /= bw;

   base = intel_miptree_map_raw(brw, mt) + mt->offset;

   if (base == NULL)
      map->ptr = NULL;
   else {
      /* Note that in the case of cube maps, the caller must have passed the
       * slice number referencing the face.
      */
      intel_miptree_get_image_offset(mt, level, slice, &image_x, &image_y);
      x += image_x;
      y += image_y;

      map->stride = mt->pitch;
      map->ptr = base + y * map->stride + x * mt->cpp;
   }

   DBG("%s: %d,%d %dx%d from mt %p (%s) "
       "%"PRIiPTR",%"PRIiPTR" = %p/%d\n", __func__,
       map->x, map->y, map->w, map->h,
       mt, _mesa_get_format_name(mt->format),
       x, y, map->ptr, map->stride);
}

static void
intel_miptree_unmap_gtt(struct intel_mipmap_tree *mt)
{
   intel_miptree_unmap_raw(mt);
}

static void
intel_miptree_map_blit(struct brw_context *brw,
		       struct intel_mipmap_tree *mt,
		       struct intel_miptree_map *map,
		       unsigned int level, unsigned int slice)
{
   map->linear_mt = intel_miptree_create(brw, GL_TEXTURE_2D, mt->format,
                                         /* first_level */ 0,
                                         /* last_level */ 0,
                                         map->w, map->h, 1,
                                         /* samples */ 0,
                                         MIPTREE_LAYOUT_TILING_NONE);

   if (!map->linear_mt) {
      fprintf(stderr, "Failed to allocate blit temporary\n");
      goto fail;
   }
   map->stride = map->linear_mt->pitch;

   /* One of either READ_BIT or WRITE_BIT or both is set.  READ_BIT implies no
    * INVALIDATE_RANGE_BIT.  WRITE_BIT needs the original values read in unless
    * invalidate is set, since we'll be writing the whole rectangle from our
    * temporary buffer back out.
    */
   if (!(map->mode & GL_MAP_INVALIDATE_RANGE_BIT)) {
      if (!intel_miptree_copy(brw,
                              mt, level, slice, map->x, map->y,
                              map->linear_mt, 0, 0, 0, 0,
                              map->w, map->h)) {
         fprintf(stderr, "Failed to blit\n");
         goto fail;
      }
   }

   map->ptr = intel_miptree_map_raw(brw, map->linear_mt);

   DBG("%s: %d,%d %dx%d from mt %p (%s) %d,%d = %p/%d\n", __func__,
       map->x, map->y, map->w, map->h,
       mt, _mesa_get_format_name(mt->format),
       level, slice, map->ptr, map->stride);

   return;

fail:
   intel_miptree_release(&map->linear_mt);
   map->ptr = NULL;
   map->stride = 0;
}

static void
intel_miptree_unmap_blit(struct brw_context *brw,
			 struct intel_mipmap_tree *mt,
			 struct intel_miptree_map *map,
			 unsigned int level,
			 unsigned int slice)
{
   struct gl_context *ctx = &brw->ctx;

   intel_miptree_unmap_raw(map->linear_mt);

   if (map->mode & GL_MAP_WRITE_BIT) {
      bool ok = intel_miptree_copy(brw,
                                   map->linear_mt, 0, 0, 0, 0,
                                   mt, level, slice, map->x, map->y,
                                   map->w, map->h);
      WARN_ONCE(!ok, "Failed to blit from linear temporary mapping");
   }

   intel_miptree_release(&map->linear_mt);
}

/**
 * "Map" a buffer by copying it to an untiled temporary using MOVNTDQA.
 */
#if defined(USE_SSE41)
static void
intel_miptree_map_movntdqa(struct brw_context *brw,
                           struct intel_mipmap_tree *mt,
                           struct intel_miptree_map *map,
                           unsigned int level, unsigned int slice)
{
   assert(map->mode & GL_MAP_READ_BIT);
   assert(!(map->mode & GL_MAP_WRITE_BIT));

   DBG("%s: %d,%d %dx%d from mt %p (%s) %d,%d = %p/%d\n", __func__,
       map->x, map->y, map->w, map->h,
       mt, _mesa_get_format_name(mt->format),
       level, slice, map->ptr, map->stride);

   /* Map the original image */
   uint32_t image_x;
   uint32_t image_y;
   intel_miptree_get_image_offset(mt, level, slice, &image_x, &image_y);
   image_x += map->x;
   image_y += map->y;

   void *src = intel_miptree_map_raw(brw, mt);
   if (!src)
      return;

   src += mt->offset;

   src += image_y * mt->pitch;
   src += image_x * mt->cpp;

   /* Due to the pixel offsets for the particular image being mapped, our
    * src pointer may not be 16-byte aligned.  However, if the pitch is
    * divisible by 16, then the amount by which it's misaligned will remain
    * consistent from row to row.
    */
   assert((mt->pitch % 16) == 0);
   const int misalignment = ((uintptr_t) src) & 15;

   /* Create an untiled temporary buffer for the mapping. */
   const unsigned width_bytes = _mesa_format_row_stride(mt->format, map->w);

   map->stride = ALIGN(misalignment + width_bytes, 16);

   map->buffer = _mesa_align_malloc(map->stride * map->h, 16);
   /* Offset the destination so it has the same misalignment as src. */
   map->ptr = map->buffer + misalignment;

   assert((((uintptr_t) map->ptr) & 15) == misalignment);

   for (uint32_t y = 0; y < map->h; y++) {
      void *dst_ptr = map->ptr + y * map->stride;
      void *src_ptr = src + y * mt->pitch;

      _mesa_streaming_load_memcpy(dst_ptr, src_ptr, width_bytes);
   }

   intel_miptree_unmap_raw(mt);
}

static void
intel_miptree_unmap_movntdqa(struct brw_context *brw,
                             struct intel_mipmap_tree *mt,
                             struct intel_miptree_map *map,
                             unsigned int level,
                             unsigned int slice)
{
   _mesa_align_free(map->buffer);
   map->buffer = NULL;
   map->ptr = NULL;
}
#endif

static void
intel_miptree_map_s8(struct brw_context *brw,
		     struct intel_mipmap_tree *mt,
		     struct intel_miptree_map *map,
		     unsigned int level, unsigned int slice)
{
   map->stride = map->w;
   map->buffer = map->ptr = malloc(map->stride * map->h);
   if (!map->buffer)
      return;

   /* One of either READ_BIT or WRITE_BIT or both is set.  READ_BIT implies no
    * INVALIDATE_RANGE_BIT.  WRITE_BIT needs the original values read in unless
    * invalidate is set, since we'll be writing the whole rectangle from our
    * temporary buffer back out.
    */
   if (!(map->mode & GL_MAP_INVALIDATE_RANGE_BIT)) {
      uint8_t *untiled_s8_map = map->ptr;
      uint8_t *tiled_s8_map = intel_miptree_map_raw(brw, mt);
      unsigned int image_x, image_y;

      intel_miptree_get_image_offset(mt, level, slice, &image_x, &image_y);

      for (uint32_t y = 0; y < map->h; y++) {
	 for (uint32_t x = 0; x < map->w; x++) {
	    ptrdiff_t offset = intel_offset_S8(mt->pitch,
	                                       x + image_x + map->x,
	                                       y + image_y + map->y,
					       brw->has_swizzling);
	    untiled_s8_map[y * map->w + x] = tiled_s8_map[offset];
	 }
      }

      intel_miptree_unmap_raw(mt);

      DBG("%s: %d,%d %dx%d from mt %p %d,%d = %p/%d\n", __func__,
	  map->x, map->y, map->w, map->h,
	  mt, map->x + image_x, map->y + image_y, map->ptr, map->stride);
   } else {
      DBG("%s: %d,%d %dx%d from mt %p = %p/%d\n", __func__,
	  map->x, map->y, map->w, map->h,
	  mt, map->ptr, map->stride);
   }
}

static void
intel_miptree_unmap_s8(struct brw_context *brw,
		       struct intel_mipmap_tree *mt,
		       struct intel_miptree_map *map,
		       unsigned int level,
		       unsigned int slice)
{
   if (map->mode & GL_MAP_WRITE_BIT) {
      unsigned int image_x, image_y;
      uint8_t *untiled_s8_map = map->ptr;
      uint8_t *tiled_s8_map = intel_miptree_map_raw(brw, mt);

      intel_miptree_get_image_offset(mt, level, slice, &image_x, &image_y);

      for (uint32_t y = 0; y < map->h; y++) {
	 for (uint32_t x = 0; x < map->w; x++) {
	    ptrdiff_t offset = intel_offset_S8(mt->pitch,
	                                       image_x + x + map->x,
	                                       image_y + y + map->y,
					       brw->has_swizzling);
	    tiled_s8_map[offset] = untiled_s8_map[y * map->w + x];
	 }
      }

      intel_miptree_unmap_raw(mt);
   }

   free(map->buffer);
}

static void
intel_miptree_map_etc(struct brw_context *brw,
                      struct intel_mipmap_tree *mt,
                      struct intel_miptree_map *map,
                      unsigned int level,
                      unsigned int slice)
{
   assert(mt->etc_format != MESA_FORMAT_NONE);
   if (mt->etc_format == MESA_FORMAT_ETC1_RGB8) {
      assert(mt->format == MESA_FORMAT_R8G8B8X8_UNORM);
   }

   assert(map->mode & GL_MAP_WRITE_BIT);
   assert(map->mode & GL_MAP_INVALIDATE_RANGE_BIT);

   map->stride = _mesa_format_row_stride(mt->etc_format, map->w);
   map->buffer = malloc(_mesa_format_image_size(mt->etc_format,
                                                map->w, map->h, 1));
   map->ptr = map->buffer;
}

static void
intel_miptree_unmap_etc(struct brw_context *brw,
                        struct intel_mipmap_tree *mt,
                        struct intel_miptree_map *map,
                        unsigned int level,
                        unsigned int slice)
{
   uint32_t image_x;
   uint32_t image_y;
   intel_miptree_get_image_offset(mt, level, slice, &image_x, &image_y);

   image_x += map->x;
   image_y += map->y;

   uint8_t *dst = intel_miptree_map_raw(brw, mt)
                + image_y * mt->pitch
                + image_x * mt->cpp;

   if (mt->etc_format == MESA_FORMAT_ETC1_RGB8)
      _mesa_etc1_unpack_rgba8888(dst, mt->pitch,
                                 map->ptr, map->stride,
                                 map->w, map->h);
   else
      _mesa_unpack_etc2_format(dst, mt->pitch,
                               map->ptr, map->stride,
                               map->w, map->h, mt->etc_format);

   intel_miptree_unmap_raw(mt);
   free(map->buffer);
}

/**
 * Mapping function for packed depth/stencil miptrees backed by real separate
 * miptrees for depth and stencil.
 *
 * On gen7, and to support HiZ pre-gen7, we have to have the stencil buffer
 * separate from the depth buffer.  Yet at the GL API level, we have to expose
 * packed depth/stencil textures and FBO attachments, and Mesa core expects to
 * be able to map that memory for texture storage and glReadPixels-type
 * operations.  We give Mesa core that access by mallocing a temporary and
 * copying the data between the actual backing store and the temporary.
 */
static void
intel_miptree_map_depthstencil(struct brw_context *brw,
			       struct intel_mipmap_tree *mt,
			       struct intel_miptree_map *map,
			       unsigned int level, unsigned int slice)
{
   struct intel_mipmap_tree *z_mt = mt;
   struct intel_mipmap_tree *s_mt = mt->stencil_mt;
   bool map_z32f_x24s8 = mt->format == MESA_FORMAT_Z_FLOAT32;
   int packed_bpp = map_z32f_x24s8 ? 8 : 4;

   map->stride = map->w * packed_bpp;
   map->buffer = map->ptr = malloc(map->stride * map->h);
   if (!map->buffer)
      return;

   /* One of either READ_BIT or WRITE_BIT or both is set.  READ_BIT implies no
    * INVALIDATE_RANGE_BIT.  WRITE_BIT needs the original values read in unless
    * invalidate is set, since we'll be writing the whole rectangle from our
    * temporary buffer back out.
    */
   if (!(map->mode & GL_MAP_INVALIDATE_RANGE_BIT)) {
      uint32_t *packed_map = map->ptr;
      uint8_t *s_map = intel_miptree_map_raw(brw, s_mt);
      uint32_t *z_map = intel_miptree_map_raw(brw, z_mt);
      unsigned int s_image_x, s_image_y;
      unsigned int z_image_x, z_image_y;

      intel_miptree_get_image_offset(s_mt, level, slice,
				     &s_image_x, &s_image_y);
      intel_miptree_get_image_offset(z_mt, level, slice,
				     &z_image_x, &z_image_y);

      for (uint32_t y = 0; y < map->h; y++) {
	 for (uint32_t x = 0; x < map->w; x++) {
	    int map_x = map->x + x, map_y = map->y + y;
	    ptrdiff_t s_offset = intel_offset_S8(s_mt->pitch,
						 map_x + s_image_x,
						 map_y + s_image_y,
						 brw->has_swizzling);
	    ptrdiff_t z_offset = ((map_y + z_image_y) *
                                  (z_mt->pitch / 4) +
				  (map_x + z_image_x));
	    uint8_t s = s_map[s_offset];
	    uint32_t z = z_map[z_offset];

	    if (map_z32f_x24s8) {
	       packed_map[(y * map->w + x) * 2 + 0] = z;
	       packed_map[(y * map->w + x) * 2 + 1] = s;
	    } else {
	       packed_map[y * map->w + x] = (s << 24) | (z & 0x00ffffff);
	    }
	 }
      }

      intel_miptree_unmap_raw(s_mt);
      intel_miptree_unmap_raw(z_mt);

      DBG("%s: %d,%d %dx%d from z mt %p %d,%d, s mt %p %d,%d = %p/%d\n",
	  __func__,
	  map->x, map->y, map->w, map->h,
	  z_mt, map->x + z_image_x, map->y + z_image_y,
	  s_mt, map->x + s_image_x, map->y + s_image_y,
	  map->ptr, map->stride);
   } else {
      DBG("%s: %d,%d %dx%d from mt %p = %p/%d\n", __func__,
	  map->x, map->y, map->w, map->h,
	  mt, map->ptr, map->stride);
   }
}

static void
intel_miptree_unmap_depthstencil(struct brw_context *brw,
				 struct intel_mipmap_tree *mt,
				 struct intel_miptree_map *map,
				 unsigned int level,
				 unsigned int slice)
{
   struct intel_mipmap_tree *z_mt = mt;
   struct intel_mipmap_tree *s_mt = mt->stencil_mt;
   bool map_z32f_x24s8 = mt->format == MESA_FORMAT_Z_FLOAT32;

   if (map->mode & GL_MAP_WRITE_BIT) {
      uint32_t *packed_map = map->ptr;
      uint8_t *s_map = intel_miptree_map_raw(brw, s_mt);
      uint32_t *z_map = intel_miptree_map_raw(brw, z_mt);
      unsigned int s_image_x, s_image_y;
      unsigned int z_image_x, z_image_y;

      intel_miptree_get_image_offset(s_mt, level, slice,
				     &s_image_x, &s_image_y);
      intel_miptree_get_image_offset(z_mt, level, slice,
				     &z_image_x, &z_image_y);

      for (uint32_t y = 0; y < map->h; y++) {
	 for (uint32_t x = 0; x < map->w; x++) {
	    ptrdiff_t s_offset = intel_offset_S8(s_mt->pitch,
						 x + s_image_x + map->x,
						 y + s_image_y + map->y,
						 brw->has_swizzling);
	    ptrdiff_t z_offset = ((y + z_image_y + map->y) *
                                  (z_mt->pitch / 4) +
				  (x + z_image_x + map->x));

	    if (map_z32f_x24s8) {
	       z_map[z_offset] = packed_map[(y * map->w + x) * 2 + 0];
	       s_map[s_offset] = packed_map[(y * map->w + x) * 2 + 1];
	    } else {
	       uint32_t packed = packed_map[y * map->w + x];
	       s_map[s_offset] = packed >> 24;
	       z_map[z_offset] = packed;
	    }
	 }
      }

      intel_miptree_unmap_raw(s_mt);
      intel_miptree_unmap_raw(z_mt);

      DBG("%s: %d,%d %dx%d from z mt %p (%s) %d,%d, s mt %p %d,%d = %p/%d\n",
	  __func__,
	  map->x, map->y, map->w, map->h,
	  z_mt, _mesa_get_format_name(z_mt->format),
	  map->x + z_image_x, map->y + z_image_y,
	  s_mt, map->x + s_image_x, map->y + s_image_y,
	  map->ptr, map->stride);
   }

   free(map->buffer);
}

/**
 * Create and attach a map to the miptree at (level, slice). Return the
 * attached map.
 */
static struct intel_miptree_map*
intel_miptree_attach_map(struct intel_mipmap_tree *mt,
                         unsigned int level,
                         unsigned int slice,
                         unsigned int x,
                         unsigned int y,
                         unsigned int w,
                         unsigned int h,
                         GLbitfield mode)
{
   struct intel_miptree_map *map = calloc(1, sizeof(*map));

   if (!map)
      return NULL;

   assert(mt->level[level].slice[slice].map == NULL);
   mt->level[level].slice[slice].map = map;

   map->mode = mode;
   map->x = x;
   map->y = y;
   map->w = w;
   map->h = h;

   return map;
}

/**
 * Release the map at (level, slice).
 */
static void
intel_miptree_release_map(struct intel_mipmap_tree *mt,
                         unsigned int level,
                         unsigned int slice)
{
   struct intel_miptree_map **map;

   map = &mt->level[level].slice[slice].map;
   free(*map);
   *map = NULL;
}

static bool
can_blit_slice(struct intel_mipmap_tree *mt,
               unsigned int level, unsigned int slice)
{
   /* See intel_miptree_blit() for details on the 32k pitch limit. */
   if (mt->pitch >= 32768)
      return false;

   return true;
}

static bool
use_intel_mipree_map_blit(struct brw_context *brw,
                          struct intel_mipmap_tree *mt,
                          GLbitfield mode,
                          unsigned int level,
                          unsigned int slice)
{
   if (brw->has_llc &&
      /* It's probably not worth swapping to the blit ring because of
       * all the overhead involved. But, we must use blitter for the
       * surfaces with INTEL_MIPTREE_TRMODE_{YF,YS}.
       */
       (!(mode & GL_MAP_WRITE_BIT) ||
        mt->tr_mode != INTEL_MIPTREE_TRMODE_NONE) &&
       !mt->compressed &&
       (mt->tiling == I915_TILING_X ||
        /* Prior to Sandybridge, the blitter can't handle Y tiling */
        (brw->gen >= 6 && mt->tiling == I915_TILING_Y) ||
        /* Fast copy blit on skl+ supports all tiling formats. */
        brw->gen >= 9) &&
       can_blit_slice(mt, level, slice))
      return true;

   if (mt->tiling != I915_TILING_NONE &&
       mt->bo->size >= brw->max_gtt_map_object_size) {
      assert(can_blit_slice(mt, level, slice));
      return true;
   }

   return false;
}

/**
 * Parameter \a out_stride has type ptrdiff_t not because the buffer stride may
 * exceed 32 bits but to diminish the likelihood subtle bugs in pointer
 * arithmetic overflow.
 *
 * If you call this function and use \a out_stride, then you're doing pointer
 * arithmetic on \a out_ptr. The type of \a out_stride doesn't prevent all
 * bugs.  The caller must still take care to avoid 32-bit overflow errors in
 * all arithmetic expressions that contain buffer offsets and pixel sizes,
 * which usually have type uint32_t or GLuint.
 */
void
intel_miptree_map(struct brw_context *brw,
                  struct intel_mipmap_tree *mt,
                  unsigned int level,
                  unsigned int slice,
                  unsigned int x,
                  unsigned int y,
                  unsigned int w,
                  unsigned int h,
                  GLbitfield mode,
                  void **out_ptr,
                  ptrdiff_t *out_stride)
{
   struct intel_miptree_map *map;

   assert(mt->num_samples <= 1);

   map = intel_miptree_attach_map(mt, level, slice, x, y, w, h, mode);
   if (!map){
      *out_ptr = NULL;
      *out_stride = 0;
      return;
   }

   intel_miptree_slice_resolve_depth(brw, mt, level, slice);
   if (map->mode & GL_MAP_WRITE_BIT) {
      intel_miptree_slice_set_needs_hiz_resolve(mt, level, slice);
   }

   if (mt->format == MESA_FORMAT_S_UINT8) {
      intel_miptree_map_s8(brw, mt, map, level, slice);
   } else if (mt->etc_format != MESA_FORMAT_NONE &&
              !(mode & BRW_MAP_DIRECT_BIT)) {
      intel_miptree_map_etc(brw, mt, map, level, slice);
   } else if (mt->stencil_mt && !(mode & BRW_MAP_DIRECT_BIT)) {
      intel_miptree_map_depthstencil(brw, mt, map, level, slice);
   } else if (use_intel_mipree_map_blit(brw, mt, mode, level, slice)) {
      intel_miptree_map_blit(brw, mt, map, level, slice);
#if defined(USE_SSE41)
   } else if (!(mode & GL_MAP_WRITE_BIT) &&
              !mt->compressed && cpu_has_sse4_1 &&
              (mt->pitch % 16 == 0)) {
      intel_miptree_map_movntdqa(brw, mt, map, level, slice);
#endif
   } else {
      /* intel_miptree_map_gtt() doesn't support surfaces with Yf/Ys tiling. */
      assert(mt->tr_mode == INTEL_MIPTREE_TRMODE_NONE);
      intel_miptree_map_gtt(brw, mt, map, level, slice);
   }

   *out_ptr = map->ptr;
   *out_stride = map->stride;

   if (map->ptr == NULL)
      intel_miptree_release_map(mt, level, slice);
}

void
intel_miptree_unmap(struct brw_context *brw,
                    struct intel_mipmap_tree *mt,
                    unsigned int level,
                    unsigned int slice)
{
   struct intel_miptree_map *map = mt->level[level].slice[slice].map;

   assert(mt->num_samples <= 1);

   if (!map)
      return;

   DBG("%s: mt %p (%s) level %d slice %d\n", __func__,
       mt, _mesa_get_format_name(mt->format), level, slice);

   if (mt->format == MESA_FORMAT_S_UINT8) {
      intel_miptree_unmap_s8(brw, mt, map, level, slice);
   } else if (mt->etc_format != MESA_FORMAT_NONE &&
              !(map->mode & BRW_MAP_DIRECT_BIT)) {
      intel_miptree_unmap_etc(brw, mt, map, level, slice);
   } else if (mt->stencil_mt && !(map->mode & BRW_MAP_DIRECT_BIT)) {
      intel_miptree_unmap_depthstencil(brw, mt, map, level, slice);
   } else if (map->linear_mt) {
      intel_miptree_unmap_blit(brw, mt, map, level, slice);
#if defined(USE_SSE41)
   } else if (map->buffer && cpu_has_sse4_1) {
      intel_miptree_unmap_movntdqa(brw, mt, map, level, slice);
#endif
   } else {
      intel_miptree_unmap_gtt(mt);
   }

   intel_miptree_release_map(mt, level, slice);
}

enum isl_surf_dim
get_isl_surf_dim(GLenum target)
{
   switch (target) {
   case GL_TEXTURE_1D:
   case GL_TEXTURE_1D_ARRAY:
      return ISL_SURF_DIM_1D;

   case GL_TEXTURE_2D:
   case GL_TEXTURE_2D_ARRAY:
   case GL_TEXTURE_RECTANGLE:
   case GL_TEXTURE_CUBE_MAP:
   case GL_TEXTURE_CUBE_MAP_ARRAY:
   case GL_TEXTURE_2D_MULTISAMPLE:
   case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
   case GL_TEXTURE_EXTERNAL_OES:
      return ISL_SURF_DIM_2D;

   case GL_TEXTURE_3D:
      return ISL_SURF_DIM_3D;
   }

   unreachable("Invalid texture target");
}

enum isl_dim_layout
get_isl_dim_layout(const struct gen_device_info *devinfo, uint32_t tiling,
                   GLenum target)
{
   switch (target) {
   case GL_TEXTURE_1D:
   case GL_TEXTURE_1D_ARRAY:
      return (devinfo->gen >= 9 && tiling == I915_TILING_NONE ?
              ISL_DIM_LAYOUT_GEN9_1D : ISL_DIM_LAYOUT_GEN4_2D);

   case GL_TEXTURE_2D:
   case GL_TEXTURE_2D_ARRAY:
   case GL_TEXTURE_RECTANGLE:
   case GL_TEXTURE_2D_MULTISAMPLE:
   case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
   case GL_TEXTURE_EXTERNAL_OES:
      return ISL_DIM_LAYOUT_GEN4_2D;

   case GL_TEXTURE_CUBE_MAP:
   case GL_TEXTURE_CUBE_MAP_ARRAY:
      return (devinfo->gen == 4 ? ISL_DIM_LAYOUT_GEN4_3D :
              ISL_DIM_LAYOUT_GEN4_2D);

   case GL_TEXTURE_3D:
      return (devinfo->gen >= 9 ?
              ISL_DIM_LAYOUT_GEN4_2D : ISL_DIM_LAYOUT_GEN4_3D);
   }

   unreachable("Invalid texture target");
}

enum isl_tiling
intel_miptree_get_isl_tiling(const struct intel_mipmap_tree *mt)
{
   if (mt->format == MESA_FORMAT_S_UINT8) {
      return ISL_TILING_W;
   } else {
      switch (mt->tiling) {
      case I915_TILING_NONE:
         return ISL_TILING_LINEAR;
      case I915_TILING_X:
         return ISL_TILING_X;
      case I915_TILING_Y:
         switch (mt->tr_mode) {
         case INTEL_MIPTREE_TRMODE_NONE:
            return ISL_TILING_Y0;
         case INTEL_MIPTREE_TRMODE_YF:
            return ISL_TILING_Yf;
         case INTEL_MIPTREE_TRMODE_YS:
            return ISL_TILING_Ys;
         default:
            unreachable("Invalid tiled resource mode");
         }
      default:
         unreachable("Invalid tiling mode");
      }
   }
}

void
intel_miptree_get_isl_surf(struct brw_context *brw,
                           const struct intel_mipmap_tree *mt,
                           struct isl_surf *surf)
{
   surf->dim = get_isl_surf_dim(mt->target);
   surf->dim_layout = get_isl_dim_layout(&brw->screen->devinfo,
                                         mt->tiling, mt->target);

   if (mt->num_samples > 1) {
      switch (mt->msaa_layout) {
      case INTEL_MSAA_LAYOUT_IMS:
         surf->msaa_layout = ISL_MSAA_LAYOUT_INTERLEAVED;
         break;
      case INTEL_MSAA_LAYOUT_UMS:
      case INTEL_MSAA_LAYOUT_CMS:
         surf->msaa_layout = ISL_MSAA_LAYOUT_ARRAY;
         break;
      default:
         unreachable("Invalid MSAA layout");
      }
   } else {
      surf->msaa_layout = ISL_MSAA_LAYOUT_NONE;
   }

   surf->tiling = intel_miptree_get_isl_tiling(mt);

   if (mt->format == MESA_FORMAT_S_UINT8) {
      /* The ISL definition of row_pitch matches the surface state pitch field
       * a bit better than intel_mipmap_tree.  In particular, ISL incorporates
       * the factor of 2 for W-tiling in row_pitch.
       */
      surf->row_pitch = 2 * mt->pitch;
   } else {
      surf->row_pitch = mt->pitch;
   }

   surf->format = translate_tex_format(brw, mt->format, false);

   if (brw->gen >= 9) {
      if (surf->dim == ISL_SURF_DIM_1D && surf->tiling == ISL_TILING_LINEAR) {
         /* For gen9 1-D surfaces, intel_mipmap_tree has a bogus alignment. */
         surf->image_alignment_el = isl_extent3d(64, 1, 1);
      } else {
         /* On gen9+, intel_mipmap_tree stores the horizontal and vertical
          * alignment in terms of surface elements like we want.
          */
         surf->image_alignment_el = isl_extent3d(mt->halign, mt->valign, 1);
      }
   } else {
      /* On earlier gens it's stored in pixels. */
      unsigned bw, bh;
      _mesa_get_format_block_size(mt->format, &bw, &bh);
      surf->image_alignment_el =
         isl_extent3d(mt->halign / bw, mt->valign / bh, 1);
   }

   surf->logical_level0_px.width = mt->logical_width0;
   surf->logical_level0_px.height = mt->logical_height0;
   if (surf->dim == ISL_SURF_DIM_3D) {
      surf->logical_level0_px.depth = mt->logical_depth0;
      surf->logical_level0_px.array_len = 1;
   } else {
      surf->logical_level0_px.depth = 1;
      surf->logical_level0_px.array_len = mt->logical_depth0;
   }

   surf->phys_level0_sa.width = mt->physical_width0;
   surf->phys_level0_sa.height = mt->physical_height0;
   if (surf->dim == ISL_SURF_DIM_3D) {
      surf->phys_level0_sa.depth = mt->physical_depth0;
      surf->phys_level0_sa.array_len = 1;
   } else {
      surf->phys_level0_sa.depth = 1;
      surf->phys_level0_sa.array_len = mt->physical_depth0;
   }

   surf->levels = mt->last_level + 1;
   surf->samples = MAX2(mt->num_samples, 1);

   surf->size = 0; /* TODO */
   surf->alignment = 0; /* TODO */

   switch (surf->dim_layout) {
   case ISL_DIM_LAYOUT_GEN4_2D:
   case ISL_DIM_LAYOUT_GEN4_3D:
      if (brw->gen >= 9) {
         surf->array_pitch_el_rows = mt->qpitch;
      } else {
         unsigned bw, bh;
         _mesa_get_format_block_size(mt->format, &bw, &bh);
         assert(mt->qpitch % bh == 0);
         surf->array_pitch_el_rows = mt->qpitch / bh;
      }
      break;
   case ISL_DIM_LAYOUT_GEN9_1D:
      surf->array_pitch_el_rows = 1;
      break;
   }

   switch (mt->array_layout) {
   case ALL_LOD_IN_EACH_SLICE:
      surf->array_pitch_span = ISL_ARRAY_PITCH_SPAN_FULL;
      break;
   case ALL_SLICES_AT_EACH_LOD:
      surf->array_pitch_span = ISL_ARRAY_PITCH_SPAN_COMPACT;
      break;
   default:
      unreachable("Invalid array layout");
   }

   GLenum base_format = _mesa_get_format_base_format(mt->format);
   switch (base_format) {
   case GL_DEPTH_COMPONENT:
      surf->usage = ISL_SURF_USAGE_DEPTH_BIT | ISL_SURF_USAGE_TEXTURE_BIT;
      break;
   case GL_STENCIL_INDEX:
      surf->usage = ISL_SURF_USAGE_STENCIL_BIT;
      if (brw->gen >= 8)
         surf->usage |= ISL_SURF_USAGE_TEXTURE_BIT;
      break;
   case GL_DEPTH_STENCIL:
      /* In this case we only texture from the depth part */
      surf->usage = ISL_SURF_USAGE_DEPTH_BIT | ISL_SURF_USAGE_STENCIL_BIT |
                    ISL_SURF_USAGE_TEXTURE_BIT;
      break;
   default:
      surf->usage = ISL_SURF_USAGE_TEXTURE_BIT;
      if (brw->format_supported_as_render_target[mt->format])
         surf->usage = ISL_SURF_USAGE_RENDER_TARGET_BIT;
      break;
   }

   if (_mesa_is_cube_map_texture(mt->target))
      surf->usage |= ISL_SURF_USAGE_CUBE_BIT;
}

/* WARNING: THE SURFACE CREATED BY THIS FUNCTION IS NOT COMPLETE AND CANNOT BE
 * USED FOR ANY REAL CALCULATIONS.  THE ONLY VALID USE OF SUCH A SURFACE IS TO
 * PASS IT INTO isl_surf_fill_state.
 */
void
intel_miptree_get_aux_isl_surf(struct brw_context *brw,
                               const struct intel_mipmap_tree *mt,
                               struct isl_surf *surf,
                               enum isl_aux_usage *usage)
{
   uint32_t aux_pitch, aux_qpitch;
   if (mt->mcs_buf) {
      aux_pitch = mt->mcs_buf->pitch;
      aux_qpitch = mt->mcs_buf->qpitch;

      if (mt->num_samples > 1) {
         assert(mt->msaa_layout == INTEL_MSAA_LAYOUT_CMS);
         *usage = ISL_AUX_USAGE_MCS;
      } else if (intel_miptree_is_lossless_compressed(brw, mt)) {
         assert(brw->gen >= 9);
         *usage = ISL_AUX_USAGE_CCS_E;
      } else if ((mt->aux_disable & INTEL_AUX_DISABLE_CCS) == 0) {
         *usage = ISL_AUX_USAGE_CCS_D;
      } else {
         unreachable("Invalid MCS miptree");
      }
   } else if (mt->hiz_buf) {
      if (mt->hiz_buf->mt) {
         aux_pitch = mt->hiz_buf->mt->pitch;
         aux_qpitch = mt->hiz_buf->mt->qpitch;
      } else {
         aux_pitch = mt->hiz_buf->aux_base.pitch;
         aux_qpitch = mt->hiz_buf->aux_base.qpitch;
      }

      *usage = ISL_AUX_USAGE_HIZ;
   } else {
      *usage = ISL_AUX_USAGE_NONE;
      return;
   }

   /* Start with a copy of the original surface. */
   intel_miptree_get_isl_surf(brw, mt, surf);

   /* Figure out the format and tiling of the auxiliary surface */
   switch (*usage) {
   case ISL_AUX_USAGE_NONE:
      unreachable("Invalid auxiliary usage");

   case ISL_AUX_USAGE_HIZ:
      isl_surf_get_hiz_surf(&brw->isl_dev, surf, surf);
      break;

   case ISL_AUX_USAGE_MCS:
      /*
       * From the SKL PRM:
       *    "When Auxiliary Surface Mode is set to AUX_CCS_D or AUX_CCS_E,
       *    HALIGN 16 must be used."
       */
      if (brw->gen >= 9)
         assert(mt->halign == 16);

      isl_surf_get_mcs_surf(&brw->isl_dev, surf, surf);
      break;

   case ISL_AUX_USAGE_CCS_D:
   case ISL_AUX_USAGE_CCS_E:
      /*
       * From the BDW PRM, Volume 2d, page 260 (RENDER_SURFACE_STATE):
       *
       *    "When MCS is enabled for non-MSRT, HALIGN_16 must be used"
       *
       * From the hardware spec for GEN9:
       *
       *    "When Auxiliary Surface Mode is set to AUX_CCS_D or AUX_CCS_E,
       *    HALIGN 16 must be used."
       */
      assert(mt->num_samples <= 1);
      if (brw->gen >= 8)
         assert(mt->halign == 16);

      isl_surf_get_ccs_surf(&brw->isl_dev, surf, surf);
      break;
   }

   /* We want the pitch of the actual aux buffer. */
   surf->row_pitch = aux_pitch;

   /* Auxiliary surfaces in ISL have compressed formats and array_pitch_el_rows
    * is in elements.  This doesn't match intel_mipmap_tree::qpitch which is
    * in elements of the primary color surface so we have to divide by the
    * compression block height.
    */
   surf->array_pitch_el_rows =
      aux_qpitch / isl_format_get_layout(surf->format)->bh;
}

union isl_color_value
intel_miptree_get_isl_clear_color(struct brw_context *brw,
                                  const struct intel_mipmap_tree *mt)
{
   union isl_color_value clear_color;

   if (_mesa_get_format_base_format(mt->format) == GL_DEPTH_COMPONENT) {
      clear_color.i32[0] = mt->depth_clear_value;
      clear_color.i32[1] = 0;
      clear_color.i32[2] = 0;
      clear_color.i32[3] = 0;
   } else if (brw->gen >= 9) {
      clear_color.i32[0] = mt->gen9_fast_clear_color.i[0];
      clear_color.i32[1] = mt->gen9_fast_clear_color.i[1];
      clear_color.i32[2] = mt->gen9_fast_clear_color.i[2];
      clear_color.i32[3] = mt->gen9_fast_clear_color.i[3];
   } else if (_mesa_is_format_integer(mt->format)) {
      clear_color.i32[0] = (mt->fast_clear_color_value & (1u << 31)) != 0;
      clear_color.i32[1] = (mt->fast_clear_color_value & (1u << 30)) != 0;
      clear_color.i32[2] = (mt->fast_clear_color_value & (1u << 29)) != 0;
      clear_color.i32[3] = (mt->fast_clear_color_value & (1u << 28)) != 0;
   } else {
      clear_color.f32[0] = (mt->fast_clear_color_value & (1u << 31)) != 0;
      clear_color.f32[1] = (mt->fast_clear_color_value & (1u << 30)) != 0;
      clear_color.f32[2] = (mt->fast_clear_color_value & (1u << 29)) != 0;
      clear_color.f32[3] = (mt->fast_clear_color_value & (1u << 28)) != 0;
   }

   return clear_color;
}
