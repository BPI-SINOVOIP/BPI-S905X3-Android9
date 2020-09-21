/**************************************************************************
 * 
 * Copyright 2007 VMware, Inc.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

 /*
  * Authors:
  *   Keith Whitwell <keithw@vmware.com>
  *   Brian Paul
  */
 

#include "main/macros.h"
#include "main/mtypes.h"
#include "main/glformats.h"
#include "main/samplerobj.h"
#include "main/teximage.h"
#include "main/texobj.h"

#include "st_context.h"
#include "st_cb_texture.h"
#include "st_format.h"
#include "st_atom.h"
#include "st_texture.h"
#include "pipe/p_context.h"
#include "pipe/p_defines.h"

#include "cso_cache/cso_context.h"

#include "util/u_format.h"


/**
 * Convert GLenum texcoord wrap tokens to pipe tokens.
 */
static GLuint
gl_wrap_xlate(GLenum wrap)
{
   switch (wrap) {
   case GL_REPEAT:
      return PIPE_TEX_WRAP_REPEAT;
   case GL_CLAMP:
      return PIPE_TEX_WRAP_CLAMP;
   case GL_CLAMP_TO_EDGE:
      return PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   case GL_CLAMP_TO_BORDER:
      return PIPE_TEX_WRAP_CLAMP_TO_BORDER;
   case GL_MIRRORED_REPEAT:
      return PIPE_TEX_WRAP_MIRROR_REPEAT;
   case GL_MIRROR_CLAMP_EXT:
      return PIPE_TEX_WRAP_MIRROR_CLAMP;
   case GL_MIRROR_CLAMP_TO_EDGE_EXT:
      return PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE;
   case GL_MIRROR_CLAMP_TO_BORDER_EXT:
      return PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER;
   default:
      assert(0);
      return 0;
   }
}


static GLuint
gl_filter_to_mip_filter(GLenum filter)
{
   switch (filter) {
   case GL_NEAREST:
   case GL_LINEAR:
      return PIPE_TEX_MIPFILTER_NONE;

   case GL_NEAREST_MIPMAP_NEAREST:
   case GL_LINEAR_MIPMAP_NEAREST:
      return PIPE_TEX_MIPFILTER_NEAREST;

   case GL_NEAREST_MIPMAP_LINEAR:
   case GL_LINEAR_MIPMAP_LINEAR:
      return PIPE_TEX_MIPFILTER_LINEAR;

   default:
      assert(0);
      return PIPE_TEX_MIPFILTER_NONE;
   }
}


static GLuint
gl_filter_to_img_filter(GLenum filter)
{
   switch (filter) {
   case GL_NEAREST:
   case GL_NEAREST_MIPMAP_NEAREST:
   case GL_NEAREST_MIPMAP_LINEAR:
      return PIPE_TEX_FILTER_NEAREST;

   case GL_LINEAR:
   case GL_LINEAR_MIPMAP_NEAREST:
   case GL_LINEAR_MIPMAP_LINEAR:
      return PIPE_TEX_FILTER_LINEAR;

   default:
      assert(0);
      return PIPE_TEX_FILTER_NEAREST;
   }
}


static void
convert_sampler(struct st_context *st,
                struct pipe_sampler_state *sampler,
                GLuint texUnit)
{
   const struct gl_texture_object *texobj;
   struct gl_context *ctx = st->ctx;
   const struct gl_sampler_object *msamp;
   GLenum texBaseFormat;

   texobj = ctx->Texture.Unit[texUnit]._Current;
   if (!texobj) {
      texobj = _mesa_get_fallback_texture(ctx, TEXTURE_2D_INDEX);
      msamp = &texobj->Sampler;
   } else {
      msamp = _mesa_get_samplerobj(ctx, texUnit);
   }

   texBaseFormat = _mesa_texture_base_format(texobj);

   memset(sampler, 0, sizeof(*sampler));
   sampler->wrap_s = gl_wrap_xlate(msamp->WrapS);
   sampler->wrap_t = gl_wrap_xlate(msamp->WrapT);
   sampler->wrap_r = gl_wrap_xlate(msamp->WrapR);

   sampler->min_img_filter = gl_filter_to_img_filter(msamp->MinFilter);
   sampler->min_mip_filter = gl_filter_to_mip_filter(msamp->MinFilter);
   sampler->mag_img_filter = gl_filter_to_img_filter(msamp->MagFilter);

   if (texobj->Target != GL_TEXTURE_RECTANGLE_ARB)
      sampler->normalized_coords = 1;

   sampler->lod_bias = ctx->Texture.Unit[texUnit].LodBias + msamp->LodBias;
   /* Reduce the number of states by allowing only the values that AMD GCN
    * can represent. Apps use lod_bias for smooth transitions to bigger mipmap
    * levels.
    */
   sampler->lod_bias = CLAMP(sampler->lod_bias, -16, 16);
   sampler->lod_bias = floorf(sampler->lod_bias * 256) / 256;

   sampler->min_lod = MAX2(msamp->MinLod, 0.0f);
   sampler->max_lod = msamp->MaxLod;
   if (sampler->max_lod < sampler->min_lod) {
      /* The GL spec doesn't seem to specify what to do in this case.
       * Swap the values.
       */
      float tmp = sampler->max_lod;
      sampler->max_lod = sampler->min_lod;
      sampler->min_lod = tmp;
      assert(sampler->min_lod <= sampler->max_lod);
   }

   /* For non-black borders... */
   if (msamp->BorderColor.ui[0] ||
       msamp->BorderColor.ui[1] ||
       msamp->BorderColor.ui[2] ||
       msamp->BorderColor.ui[3]) {
      const struct st_texture_object *stobj = st_texture_object_const(texobj);
      const GLboolean is_integer = texobj->_IsIntegerFormat;
      const struct pipe_sampler_view *sv = NULL;
      union pipe_color_union border_color;
      GLuint i;

      /* Just search for the first used view. We can do this because the
         swizzle is per-texture, not per context. */
      /* XXX: clean that up to not use the sampler view at all */
      for (i = 0; i < stobj->num_sampler_views; ++i) {
         if (stobj->sampler_views[i]) {
            sv = stobj->sampler_views[i];
            break;
         }
      }

      if (st->apply_texture_swizzle_to_border_color && sv) {
         const unsigned char swz[4] =
         {
            sv->swizzle_r,
            sv->swizzle_g,
            sv->swizzle_b,
            sv->swizzle_a,
         };

         st_translate_color(&msamp->BorderColor,
                            &border_color,
                            texBaseFormat, is_integer);

         util_format_apply_color_swizzle(&sampler->border_color,
                                         &border_color, swz, is_integer);
      } else {
         st_translate_color(&msamp->BorderColor,
                            &sampler->border_color,
                            texBaseFormat, is_integer);
      }
   }

   sampler->max_anisotropy = (msamp->MaxAnisotropy == 1.0 ?
                              0 : (GLuint) msamp->MaxAnisotropy);

   /* If sampling a depth texture and using shadow comparison */
   if ((texBaseFormat == GL_DEPTH_COMPONENT ||
        (texBaseFormat == GL_DEPTH_STENCIL && !texobj->StencilSampling)) &&
       msamp->CompareMode == GL_COMPARE_R_TO_TEXTURE) {
      sampler->compare_mode = PIPE_TEX_COMPARE_R_TO_TEXTURE;
      sampler->compare_func = st_compare_func_to_pipe(msamp->CompareFunc);
   }

   sampler->seamless_cube_map =
      ctx->Texture.CubeMapSeamless || msamp->CubeMapSeamless;
}


/**
 * Update the gallium driver's sampler state for fragment, vertex or
 * geometry shader stage.
 */
static void
update_shader_samplers(struct st_context *st,
                       enum pipe_shader_type shader_stage,
                       const struct gl_program *prog,
                       unsigned max_units,
                       struct pipe_sampler_state *samplers,
                       unsigned *num_samplers)
{
   GLbitfield samplers_used = prog->SamplersUsed;
   GLbitfield free_slots = ~prog->SamplersUsed;
   GLbitfield external_samplers_used = prog->ExternalSamplersUsed;
   GLuint unit;
   const GLuint old_max = *num_samplers;
   const struct pipe_sampler_state *states[PIPE_MAX_SAMPLERS];

   if (*num_samplers == 0 && samplers_used == 0x0)
      return;

   *num_samplers = 0;

   /* loop over sampler units (aka tex image units) */
   for (unit = 0; unit < max_units; unit++, samplers_used >>= 1) {
      struct pipe_sampler_state *sampler = samplers + unit;

      if (samplers_used & 1) {
         const GLuint texUnit = prog->SamplerUnits[unit];

         convert_sampler(st, sampler, texUnit);
         states[unit] = sampler;
         *num_samplers = unit + 1;
      }
      else if (samplers_used != 0 || unit < old_max) {
         states[unit] = NULL;
      }
      else {
         /* if we've reset all the old samplers and we have no more new ones */
         break;
      }
   }

   /* For any external samplers with multiplaner YUV, stuff the additional
    * sampler states we need at the end.
    *
    * Just re-use the existing sampler-state from the primary slot.
    */
   while (unlikely(external_samplers_used)) {
      GLuint unit = u_bit_scan(&external_samplers_used);
      GLuint extra = 0;
      struct st_texture_object *stObj =
            st_get_texture_object(st->ctx, prog, unit);
      struct pipe_sampler_state *sampler = samplers + unit;

      if (!stObj)
         continue;

      switch (st_get_view_format(stObj)) {
      case PIPE_FORMAT_NV12:
         /* we need one additional sampler: */
         extra = u_bit_scan(&free_slots);
         states[extra] = sampler;
         break;
      case PIPE_FORMAT_IYUV:
         /* we need two additional samplers: */
         extra = u_bit_scan(&free_slots);
         states[extra] = sampler;
         extra = u_bit_scan(&free_slots);
         states[extra] = sampler;
         break;
      default:
         break;
      }

      *num_samplers = MAX2(*num_samplers, extra + 1);
   }

   cso_set_samplers(st->cso_context, shader_stage, *num_samplers, states);
}


static void
update_samplers(struct st_context *st)
{
   const struct gl_context *ctx = st->ctx;

   update_shader_samplers(st,
                          PIPE_SHADER_FRAGMENT,
                          ctx->FragmentProgram._Current,
                          ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxTextureImageUnits,
                          st->state.samplers[PIPE_SHADER_FRAGMENT],
                          &st->state.num_samplers[PIPE_SHADER_FRAGMENT]);

   update_shader_samplers(st,
                          PIPE_SHADER_VERTEX,
                          ctx->VertexProgram._Current,
                          ctx->Const.Program[MESA_SHADER_VERTEX].MaxTextureImageUnits,
                          st->state.samplers[PIPE_SHADER_VERTEX],
                          &st->state.num_samplers[PIPE_SHADER_VERTEX]);

   if (ctx->GeometryProgram._Current) {
      update_shader_samplers(st,
                             PIPE_SHADER_GEOMETRY,
                             ctx->GeometryProgram._Current,
                             ctx->Const.Program[MESA_SHADER_GEOMETRY].MaxTextureImageUnits,
                             st->state.samplers[PIPE_SHADER_GEOMETRY],
                             &st->state.num_samplers[PIPE_SHADER_GEOMETRY]);
   }
   if (ctx->TessCtrlProgram._Current) {
      update_shader_samplers(st,
                             PIPE_SHADER_TESS_CTRL,
                             ctx->TessCtrlProgram._Current,
                             ctx->Const.Program[MESA_SHADER_TESS_CTRL].MaxTextureImageUnits,
                             st->state.samplers[PIPE_SHADER_TESS_CTRL],
                             &st->state.num_samplers[PIPE_SHADER_TESS_CTRL]);
   }
   if (ctx->TessEvalProgram._Current) {
      update_shader_samplers(st,
                             PIPE_SHADER_TESS_EVAL,
                             ctx->TessEvalProgram._Current,
                             ctx->Const.Program[MESA_SHADER_TESS_EVAL].MaxTextureImageUnits,
                             st->state.samplers[PIPE_SHADER_TESS_EVAL],
                             &st->state.num_samplers[PIPE_SHADER_TESS_EVAL]);
   }
   if (ctx->ComputeProgram._Current) {
      update_shader_samplers(st,
                             PIPE_SHADER_COMPUTE,
                             ctx->ComputeProgram._Current,
                             ctx->Const.Program[MESA_SHADER_COMPUTE].MaxTextureImageUnits,
                             st->state.samplers[PIPE_SHADER_COMPUTE],
                             &st->state.num_samplers[PIPE_SHADER_COMPUTE]);
   }
}


const struct st_tracked_state st_update_sampler = {
   update_samplers					/* update */
};
