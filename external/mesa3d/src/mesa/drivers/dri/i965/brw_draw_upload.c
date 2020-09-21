/*
 * Copyright 2003 VMware, Inc.
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

#include "main/bufferobj.h"
#include "main/context.h"
#include "main/enums.h"
#include "main/macros.h"
#include "main/glformats.h"

#include "brw_draw.h"
#include "brw_defines.h"
#include "brw_context.h"
#include "brw_state.h"

#include "intel_batchbuffer.h"
#include "intel_buffer_objects.h"

static const GLuint double_types_float[5] = {
   0,
   BRW_SURFACEFORMAT_R64_FLOAT,
   BRW_SURFACEFORMAT_R64G64_FLOAT,
   BRW_SURFACEFORMAT_R64G64B64_FLOAT,
   BRW_SURFACEFORMAT_R64G64B64A64_FLOAT
};

static const GLuint double_types_passthru[5] = {
   0,
   BRW_SURFACEFORMAT_R64_PASSTHRU,
   BRW_SURFACEFORMAT_R64G64_PASSTHRU,
   BRW_SURFACEFORMAT_R64G64B64_PASSTHRU,
   BRW_SURFACEFORMAT_R64G64B64A64_PASSTHRU
};

static const GLuint float_types[5] = {
   0,
   BRW_SURFACEFORMAT_R32_FLOAT,
   BRW_SURFACEFORMAT_R32G32_FLOAT,
   BRW_SURFACEFORMAT_R32G32B32_FLOAT,
   BRW_SURFACEFORMAT_R32G32B32A32_FLOAT
};

static const GLuint half_float_types[5] = {
   0,
   BRW_SURFACEFORMAT_R16_FLOAT,
   BRW_SURFACEFORMAT_R16G16_FLOAT,
   BRW_SURFACEFORMAT_R16G16B16_FLOAT,
   BRW_SURFACEFORMAT_R16G16B16A16_FLOAT
};

static const GLuint fixed_point_types[5] = {
   0,
   BRW_SURFACEFORMAT_R32_SFIXED,
   BRW_SURFACEFORMAT_R32G32_SFIXED,
   BRW_SURFACEFORMAT_R32G32B32_SFIXED,
   BRW_SURFACEFORMAT_R32G32B32A32_SFIXED,
};

static const GLuint uint_types_direct[5] = {
   0,
   BRW_SURFACEFORMAT_R32_UINT,
   BRW_SURFACEFORMAT_R32G32_UINT,
   BRW_SURFACEFORMAT_R32G32B32_UINT,
   BRW_SURFACEFORMAT_R32G32B32A32_UINT
};

static const GLuint uint_types_norm[5] = {
   0,
   BRW_SURFACEFORMAT_R32_UNORM,
   BRW_SURFACEFORMAT_R32G32_UNORM,
   BRW_SURFACEFORMAT_R32G32B32_UNORM,
   BRW_SURFACEFORMAT_R32G32B32A32_UNORM
};

static const GLuint uint_types_scale[5] = {
   0,
   BRW_SURFACEFORMAT_R32_USCALED,
   BRW_SURFACEFORMAT_R32G32_USCALED,
   BRW_SURFACEFORMAT_R32G32B32_USCALED,
   BRW_SURFACEFORMAT_R32G32B32A32_USCALED
};

static const GLuint int_types_direct[5] = {
   0,
   BRW_SURFACEFORMAT_R32_SINT,
   BRW_SURFACEFORMAT_R32G32_SINT,
   BRW_SURFACEFORMAT_R32G32B32_SINT,
   BRW_SURFACEFORMAT_R32G32B32A32_SINT
};

static const GLuint int_types_norm[5] = {
   0,
   BRW_SURFACEFORMAT_R32_SNORM,
   BRW_SURFACEFORMAT_R32G32_SNORM,
   BRW_SURFACEFORMAT_R32G32B32_SNORM,
   BRW_SURFACEFORMAT_R32G32B32A32_SNORM
};

static const GLuint int_types_scale[5] = {
   0,
   BRW_SURFACEFORMAT_R32_SSCALED,
   BRW_SURFACEFORMAT_R32G32_SSCALED,
   BRW_SURFACEFORMAT_R32G32B32_SSCALED,
   BRW_SURFACEFORMAT_R32G32B32A32_SSCALED
};

static const GLuint ushort_types_direct[5] = {
   0,
   BRW_SURFACEFORMAT_R16_UINT,
   BRW_SURFACEFORMAT_R16G16_UINT,
   BRW_SURFACEFORMAT_R16G16B16_UINT,
   BRW_SURFACEFORMAT_R16G16B16A16_UINT
};

static const GLuint ushort_types_norm[5] = {
   0,
   BRW_SURFACEFORMAT_R16_UNORM,
   BRW_SURFACEFORMAT_R16G16_UNORM,
   BRW_SURFACEFORMAT_R16G16B16_UNORM,
   BRW_SURFACEFORMAT_R16G16B16A16_UNORM
};

static const GLuint ushort_types_scale[5] = {
   0,
   BRW_SURFACEFORMAT_R16_USCALED,
   BRW_SURFACEFORMAT_R16G16_USCALED,
   BRW_SURFACEFORMAT_R16G16B16_USCALED,
   BRW_SURFACEFORMAT_R16G16B16A16_USCALED
};

static const GLuint short_types_direct[5] = {
   0,
   BRW_SURFACEFORMAT_R16_SINT,
   BRW_SURFACEFORMAT_R16G16_SINT,
   BRW_SURFACEFORMAT_R16G16B16_SINT,
   BRW_SURFACEFORMAT_R16G16B16A16_SINT
};

static const GLuint short_types_norm[5] = {
   0,
   BRW_SURFACEFORMAT_R16_SNORM,
   BRW_SURFACEFORMAT_R16G16_SNORM,
   BRW_SURFACEFORMAT_R16G16B16_SNORM,
   BRW_SURFACEFORMAT_R16G16B16A16_SNORM
};

static const GLuint short_types_scale[5] = {
   0,
   BRW_SURFACEFORMAT_R16_SSCALED,
   BRW_SURFACEFORMAT_R16G16_SSCALED,
   BRW_SURFACEFORMAT_R16G16B16_SSCALED,
   BRW_SURFACEFORMAT_R16G16B16A16_SSCALED
};

static const GLuint ubyte_types_direct[5] = {
   0,
   BRW_SURFACEFORMAT_R8_UINT,
   BRW_SURFACEFORMAT_R8G8_UINT,
   BRW_SURFACEFORMAT_R8G8B8_UINT,
   BRW_SURFACEFORMAT_R8G8B8A8_UINT
};

static const GLuint ubyte_types_norm[5] = {
   0,
   BRW_SURFACEFORMAT_R8_UNORM,
   BRW_SURFACEFORMAT_R8G8_UNORM,
   BRW_SURFACEFORMAT_R8G8B8_UNORM,
   BRW_SURFACEFORMAT_R8G8B8A8_UNORM
};

static const GLuint ubyte_types_scale[5] = {
   0,
   BRW_SURFACEFORMAT_R8_USCALED,
   BRW_SURFACEFORMAT_R8G8_USCALED,
   BRW_SURFACEFORMAT_R8G8B8_USCALED,
   BRW_SURFACEFORMAT_R8G8B8A8_USCALED
};

static const GLuint byte_types_direct[5] = {
   0,
   BRW_SURFACEFORMAT_R8_SINT,
   BRW_SURFACEFORMAT_R8G8_SINT,
   BRW_SURFACEFORMAT_R8G8B8_SINT,
   BRW_SURFACEFORMAT_R8G8B8A8_SINT
};

static const GLuint byte_types_norm[5] = {
   0,
   BRW_SURFACEFORMAT_R8_SNORM,
   BRW_SURFACEFORMAT_R8G8_SNORM,
   BRW_SURFACEFORMAT_R8G8B8_SNORM,
   BRW_SURFACEFORMAT_R8G8B8A8_SNORM
};

static const GLuint byte_types_scale[5] = {
   0,
   BRW_SURFACEFORMAT_R8_SSCALED,
   BRW_SURFACEFORMAT_R8G8_SSCALED,
   BRW_SURFACEFORMAT_R8G8B8_SSCALED,
   BRW_SURFACEFORMAT_R8G8B8A8_SSCALED
};

static GLuint
double_types(struct brw_context *brw,
             int size,
             GLboolean doubles)
{
   /* From the BDW PRM, Volume 2d, page 588 (VERTEX_ELEMENT_STATE):
    * "When SourceElementFormat is set to one of the *64*_PASSTHRU formats,
    * 64-bit components are stored in the URB without any conversion."
    * Also included on BDW PRM, Volume 7, page 470, table "Source Element
    * Formats Supported in VF Unit"
    *
    * Previous PRMs don't include those references, so for gen7 we can't use
    * PASSTHRU formats directly. But in any case, we prefer to return passthru
    * even in that case, because that reflects what we want to achieve, even
    * if we would need to workaround on gen < 8.
    */
   return (doubles
           ? double_types_passthru[size]
           : double_types_float[size]);
}

static bool
is_passthru_format(uint32_t format)
{
   switch (format) {
   case BRW_SURFACEFORMAT_R64_PASSTHRU:
   case BRW_SURFACEFORMAT_R64G64_PASSTHRU:
   case BRW_SURFACEFORMAT_R64G64B64_PASSTHRU:
   case BRW_SURFACEFORMAT_R64G64B64A64_PASSTHRU:
      return true;
   default:
      return false;
   }
}

static int
uploads_needed(uint32_t format)
{
   if (!is_passthru_format(format))
      return 1;

   switch (format) {
   case BRW_SURFACEFORMAT_R64_PASSTHRU:
   case BRW_SURFACEFORMAT_R64G64_PASSTHRU:
      return 1;
   case BRW_SURFACEFORMAT_R64G64B64_PASSTHRU:
   case BRW_SURFACEFORMAT_R64G64B64A64_PASSTHRU:
      return 2;
   default:
      unreachable("not reached");
   }
}

/*
 * Returns the number of componentes associated with a format that is used on
 * a 64 to 32 format split. See downsize_format()
 */
static int
upload_format_size(uint32_t upload_format)
{
   switch (upload_format) {
   case BRW_SURFACEFORMAT_R32G32_FLOAT:
      return 2;
   case BRW_SURFACEFORMAT_R32G32B32A32_FLOAT:
      return 4;
   default:
      unreachable("not reached");
   }
}

/*
 * Returns the format that we are finally going to use when upload a vertex
 * element. It will only change if we are using *64*PASSTHRU formats, as for
 * gen < 8 they need to be splitted on two *32*FLOAT formats.
 *
 * @upload points in which upload we are. Valid values are [0,1]
 */
static uint32_t
downsize_format_if_needed(uint32_t format,
                          int upload)
{
   assert(upload == 0 || upload == 1);

   if (!is_passthru_format(format))
      return format;

   switch (format) {
   case BRW_SURFACEFORMAT_R64_PASSTHRU:
      return BRW_SURFACEFORMAT_R32G32_FLOAT;
   case BRW_SURFACEFORMAT_R64G64_PASSTHRU:
      return BRW_SURFACEFORMAT_R32G32B32A32_FLOAT;
   case BRW_SURFACEFORMAT_R64G64B64_PASSTHRU:
      return !upload ? BRW_SURFACEFORMAT_R32G32B32A32_FLOAT
                     : BRW_SURFACEFORMAT_R32G32_FLOAT;
   case BRW_SURFACEFORMAT_R64G64B64A64_PASSTHRU:
      return BRW_SURFACEFORMAT_R32G32B32A32_FLOAT;
   default:
      unreachable("not reached");
   }
}

/**
 * Given vertex array type/size/format/normalized info, return
 * the appopriate hardware surface type.
 * Format will be GL_RGBA or possibly GL_BGRA for GLubyte[4] color arrays.
 */
unsigned
brw_get_vertex_surface_type(struct brw_context *brw,
                            const struct gl_vertex_array *glarray)
{
   int size = glarray->Size;
   const bool is_ivybridge_or_older =
      brw->gen <= 7 && !brw->is_baytrail && !brw->is_haswell;

   if (unlikely(INTEL_DEBUG & DEBUG_VERTS))
      fprintf(stderr, "type %s size %d normalized %d\n",
              _mesa_enum_to_string(glarray->Type),
              glarray->Size, glarray->Normalized);

   if (glarray->Integer) {
      assert(glarray->Format == GL_RGBA); /* sanity check */
      switch (glarray->Type) {
      case GL_INT: return int_types_direct[size];
      case GL_SHORT:
         if (is_ivybridge_or_older && size == 3)
            return short_types_direct[4];
         else
            return short_types_direct[size];
      case GL_BYTE:
         if (is_ivybridge_or_older && size == 3)
            return byte_types_direct[4];
         else
            return byte_types_direct[size];
      case GL_UNSIGNED_INT: return uint_types_direct[size];
      case GL_UNSIGNED_SHORT:
         if (is_ivybridge_or_older && size == 3)
            return ushort_types_direct[4];
         else
            return ushort_types_direct[size];
      case GL_UNSIGNED_BYTE:
         if (is_ivybridge_or_older && size == 3)
            return ubyte_types_direct[4];
         else
            return ubyte_types_direct[size];
      default: unreachable("not reached");
      }
   } else if (glarray->Type == GL_UNSIGNED_INT_10F_11F_11F_REV) {
      return BRW_SURFACEFORMAT_R11G11B10_FLOAT;
   } else if (glarray->Normalized) {
      switch (glarray->Type) {
      case GL_DOUBLE: return double_types(brw, size, glarray->Doubles);
      case GL_FLOAT: return float_types[size];
      case GL_HALF_FLOAT:
      case GL_HALF_FLOAT_OES:
         if (brw->gen < 6 && size == 3)
            return half_float_types[4];
         else
            return half_float_types[size];
      case GL_INT: return int_types_norm[size];
      case GL_SHORT: return short_types_norm[size];
      case GL_BYTE: return byte_types_norm[size];
      case GL_UNSIGNED_INT: return uint_types_norm[size];
      case GL_UNSIGNED_SHORT: return ushort_types_norm[size];
      case GL_UNSIGNED_BYTE:
         if (glarray->Format == GL_BGRA) {
            /* See GL_EXT_vertex_array_bgra */
            assert(size == 4);
            return BRW_SURFACEFORMAT_B8G8R8A8_UNORM;
         }
         else {
            return ubyte_types_norm[size];
         }
      case GL_FIXED:
         if (brw->gen >= 8 || brw->is_haswell)
            return fixed_point_types[size];

         /* This produces GL_FIXED inputs as values between INT32_MIN and
          * INT32_MAX, which will be scaled down by 1/65536 by the VS.
          */
         return int_types_scale[size];
      /* See GL_ARB_vertex_type_2_10_10_10_rev.
       * W/A: Pre-Haswell, the hardware doesn't really support the formats we'd
       * like to use here, so upload everything as UINT and fix
       * it in the shader
       */
      case GL_INT_2_10_10_10_REV:
         assert(size == 4);
         if (brw->gen >= 8 || brw->is_haswell) {
            return glarray->Format == GL_BGRA
               ? BRW_SURFACEFORMAT_B10G10R10A2_SNORM
               : BRW_SURFACEFORMAT_R10G10B10A2_SNORM;
         }
         return BRW_SURFACEFORMAT_R10G10B10A2_UINT;
      case GL_UNSIGNED_INT_2_10_10_10_REV:
         assert(size == 4);
         if (brw->gen >= 8 || brw->is_haswell) {
            return glarray->Format == GL_BGRA
               ? BRW_SURFACEFORMAT_B10G10R10A2_UNORM
               : BRW_SURFACEFORMAT_R10G10B10A2_UNORM;
         }
         return BRW_SURFACEFORMAT_R10G10B10A2_UINT;
      default: unreachable("not reached");
      }
   }
   else {
      /* See GL_ARB_vertex_type_2_10_10_10_rev.
       * W/A: the hardware doesn't really support the formats we'd
       * like to use here, so upload everything as UINT and fix
       * it in the shader
       */
      if (glarray->Type == GL_INT_2_10_10_10_REV) {
         assert(size == 4);
         if (brw->gen >= 8 || brw->is_haswell) {
            return glarray->Format == GL_BGRA
               ? BRW_SURFACEFORMAT_B10G10R10A2_SSCALED
               : BRW_SURFACEFORMAT_R10G10B10A2_SSCALED;
         }
         return BRW_SURFACEFORMAT_R10G10B10A2_UINT;
      } else if (glarray->Type == GL_UNSIGNED_INT_2_10_10_10_REV) {
         assert(size == 4);
         if (brw->gen >= 8 || brw->is_haswell) {
            return glarray->Format == GL_BGRA
               ? BRW_SURFACEFORMAT_B10G10R10A2_USCALED
               : BRW_SURFACEFORMAT_R10G10B10A2_USCALED;
         }
         return BRW_SURFACEFORMAT_R10G10B10A2_UINT;
      }
      assert(glarray->Format == GL_RGBA); /* sanity check */
      switch (glarray->Type) {
      case GL_DOUBLE: return double_types(brw, size, glarray->Doubles);
      case GL_FLOAT: return float_types[size];
      case GL_HALF_FLOAT:
      case GL_HALF_FLOAT_OES:
         if (brw->gen < 6 && size == 3)
            return half_float_types[4];
         else
            return half_float_types[size];
      case GL_INT: return int_types_scale[size];
      case GL_SHORT: return short_types_scale[size];
      case GL_BYTE: return byte_types_scale[size];
      case GL_UNSIGNED_INT: return uint_types_scale[size];
      case GL_UNSIGNED_SHORT: return ushort_types_scale[size];
      case GL_UNSIGNED_BYTE: return ubyte_types_scale[size];
      case GL_FIXED:
         if (brw->gen >= 8 || brw->is_haswell)
            return fixed_point_types[size];

         /* This produces GL_FIXED inputs as values between INT32_MIN and
          * INT32_MAX, which will be scaled down by 1/65536 by the VS.
          */
         return int_types_scale[size];
      default: unreachable("not reached");
      }
   }
}

static void
copy_array_to_vbo_array(struct brw_context *brw,
			struct brw_vertex_element *element,
			int min, int max,
			struct brw_vertex_buffer *buffer,
			GLuint dst_stride)
{
   const int src_stride = element->glarray->StrideB;

   /* If the source stride is zero, we just want to upload the current
    * attribute once and set the buffer's stride to 0.  There's no need
    * to replicate it out.
    */
   if (src_stride == 0) {
      intel_upload_data(brw, element->glarray->Ptr,
                        element->glarray->_ElementSize,
                        element->glarray->_ElementSize,
			&buffer->bo, &buffer->offset);

      buffer->stride = 0;
      buffer->size = element->glarray->_ElementSize;
      return;
   }

   const unsigned char *src = element->glarray->Ptr + min * src_stride;
   int count = max - min + 1;
   GLuint size = count * dst_stride;
   uint8_t *dst = intel_upload_space(brw, size, dst_stride,
                                     &buffer->bo, &buffer->offset);

   /* The GL 4.5 spec says:
    *      "If any enabled array’s buffer binding is zero when DrawArrays or
    *      one of the other drawing commands defined in section 10.4 is called,
    *      the result is undefined."
    *
    * In this case, let's the dst with undefined values
    */
   if (src != NULL) {
      if (dst_stride == src_stride) {
         memcpy(dst, src, size);
      } else {
         while (count--) {
            memcpy(dst, src, dst_stride);
            src += src_stride;
            dst += dst_stride;
         }
      }
   }
   buffer->stride = dst_stride;
   buffer->size = size;
}

void
brw_prepare_vertices(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   /* BRW_NEW_VS_PROG_DATA */
   const struct brw_vs_prog_data *vs_prog_data =
      brw_vs_prog_data(brw->vs.base.prog_data);
   GLbitfield64 vs_inputs = vs_prog_data->inputs_read;
   const unsigned char *ptr = NULL;
   GLuint interleaved = 0;
   unsigned int min_index = brw->vb.min_index + brw->basevertex;
   unsigned int max_index = brw->vb.max_index + brw->basevertex;
   unsigned i;
   int delta, j;

   struct brw_vertex_element *upload[VERT_ATTRIB_MAX];
   GLuint nr_uploads = 0;

   /* _NEW_POLYGON
    *
    * On gen6+, edge flags don't end up in the VUE (either in or out of the
    * VS).  Instead, they're uploaded as the last vertex element, and the data
    * is passed sideband through the fixed function units.  So, we need to
    * prepare the vertex buffer for it, but it's not present in inputs_read.
    */
   if (brw->gen >= 6 && (ctx->Polygon.FrontMode != GL_FILL ||
                           ctx->Polygon.BackMode != GL_FILL)) {
      vs_inputs |= VERT_BIT_EDGEFLAG;
   }

   if (0)
      fprintf(stderr, "%s %d..%d\n", __func__, min_index, max_index);

   /* Accumulate the list of enabled arrays. */
   brw->vb.nr_enabled = 0;
   while (vs_inputs) {
      GLuint first = ffsll(vs_inputs) - 1;
      assert (first < 64);
      GLuint index =
         first - DIV_ROUND_UP(_mesa_bitcount_64(vs_prog_data->double_inputs_read &
                                                BITFIELD64_MASK(first)), 2);
      struct brw_vertex_element *input = &brw->vb.inputs[index];
      input->is_dual_slot = (vs_prog_data->double_inputs_read & BITFIELD64_BIT(first)) != 0;
      vs_inputs &= ~BITFIELD64_BIT(first);
      if (input->is_dual_slot)
         vs_inputs &= ~BITFIELD64_BIT(first + 1);
      brw->vb.enabled[brw->vb.nr_enabled++] = input;
   }

   if (brw->vb.nr_enabled == 0)
      return;

   if (brw->vb.nr_buffers)
      return;

   /* The range of data in a given buffer represented as [min, max) */
   struct intel_buffer_object *enabled_buffer[VERT_ATTRIB_MAX];
   uint32_t buffer_range_start[VERT_ATTRIB_MAX];
   uint32_t buffer_range_end[VERT_ATTRIB_MAX];

   for (i = j = 0; i < brw->vb.nr_enabled; i++) {
      struct brw_vertex_element *input = brw->vb.enabled[i];
      const struct gl_vertex_array *glarray = input->glarray;

      if (_mesa_is_bufferobj(glarray->BufferObj)) {
	 struct intel_buffer_object *intel_buffer =
	    intel_buffer_object(glarray->BufferObj);

         const uint32_t offset = (uintptr_t)glarray->Ptr;

         /* Start with the worst case */
         uint32_t start = 0;
         uint32_t range = intel_buffer->Base.Size;
         if (glarray->InstanceDivisor) {
            if (brw->num_instances) {
               start = offset + glarray->StrideB * brw->baseinstance;
               range = (glarray->StrideB * ((brw->num_instances - 1) /
                                            glarray->InstanceDivisor) +
                        glarray->_ElementSize);
            }
         } else {
            if (brw->vb.index_bounds_valid) {
               start = offset + min_index * glarray->StrideB;
               range = (glarray->StrideB * (max_index - min_index) +
                        glarray->_ElementSize);
            }
         }

	 /* If we have a VB set to be uploaded for this buffer object
	  * already, reuse that VB state so that we emit fewer
	  * relocations.
	  */
	 unsigned k;
	 for (k = 0; k < i; k++) {
	    const struct gl_vertex_array *other = brw->vb.enabled[k]->glarray;
	    if (glarray->BufferObj == other->BufferObj &&
		glarray->StrideB == other->StrideB &&
		glarray->InstanceDivisor == other->InstanceDivisor &&
		(uintptr_t)(glarray->Ptr - other->Ptr) < glarray->StrideB)
	    {
	       input->buffer = brw->vb.enabled[k]->buffer;
	       input->offset = glarray->Ptr - other->Ptr;

               buffer_range_start[input->buffer] =
                  MIN2(buffer_range_start[input->buffer], start);
               buffer_range_end[input->buffer] =
                  MAX2(buffer_range_end[input->buffer], start + range);
	       break;
	    }
	 }
	 if (k == i) {
	    struct brw_vertex_buffer *buffer = &brw->vb.buffers[j];

	    /* Named buffer object: Just reference its contents directly. */
	    buffer->offset = offset;
	    buffer->stride = glarray->StrideB;
	    buffer->step_rate = glarray->InstanceDivisor;
            buffer->size = glarray->BufferObj->Size - offset;

            enabled_buffer[j] = intel_buffer;
            buffer_range_start[j] = start;
            buffer_range_end[j] = start + range;

	    input->buffer = j++;
	    input->offset = 0;
	 }
      } else {
	 /* Queue the buffer object up to be uploaded in the next pass,
	  * when we've decided if we're doing interleaved or not.
	  */
	 if (nr_uploads == 0) {
	    interleaved = glarray->StrideB;
	    ptr = glarray->Ptr;
	 }
	 else if (interleaved != glarray->StrideB ||
                  glarray->Ptr < ptr ||
                  (uintptr_t)(glarray->Ptr - ptr) + glarray->_ElementSize > interleaved)
	 {
            /* If our stride is different from the first attribute's stride,
             * or if the first attribute's stride didn't cover our element,
             * disable the interleaved upload optimization.  The second case
             * can most commonly occur in cases where there is a single vertex
             * and, for example, the data is stored on the application's
             * stack.
             *
             * NOTE: This will also disable the optimization in cases where
             * the data is in a different order than the array indices.
             * Something like:
             *
             *     float data[...];
             *     glVertexAttribPointer(0, 4, GL_FLOAT, 32, &data[4]);
             *     glVertexAttribPointer(1, 4, GL_FLOAT, 32, &data[0]);
             */
	    interleaved = 0;
	 }

	 upload[nr_uploads++] = input;
      }
   }

   /* Now that we've set up all of the buffers, we walk through and reference
    * each of them.  We do this late so that we get the right size in each
    * buffer and don't reference too little data.
    */
   for (i = 0; i < j; i++) {
      struct brw_vertex_buffer *buffer = &brw->vb.buffers[i];
      if (buffer->bo)
         continue;

      const uint32_t start = buffer_range_start[i];
      const uint32_t range = buffer_range_end[i] - buffer_range_start[i];

      buffer->bo = intel_bufferobj_buffer(brw, enabled_buffer[i], start, range);
      drm_intel_bo_reference(buffer->bo);
   }

   /* If we need to upload all the arrays, then we can trim those arrays to
    * only the used elements [min_index, max_index] so long as we adjust all
    * the values used in the 3DPRIMITIVE i.e. by setting the vertex bias.
    */
   brw->vb.start_vertex_bias = 0;
   delta = min_index;
   if (nr_uploads == brw->vb.nr_enabled) {
      brw->vb.start_vertex_bias = -delta;
      delta = 0;
   }

   /* Handle any arrays to be uploaded. */
   if (nr_uploads > 1) {
      if (interleaved) {
	 struct brw_vertex_buffer *buffer = &brw->vb.buffers[j];
	 /* All uploads are interleaved, so upload the arrays together as
	  * interleaved.  First, upload the contents and set up upload[0].
	  */
	 copy_array_to_vbo_array(brw, upload[0], min_index, max_index,
				 buffer, interleaved);
	 buffer->offset -= delta * interleaved;
         buffer->size += delta * interleaved;

	 for (i = 0; i < nr_uploads; i++) {
	    /* Then, just point upload[i] at upload[0]'s buffer. */
	    upload[i]->offset =
	       ((const unsigned char *)upload[i]->glarray->Ptr - ptr);
	    upload[i]->buffer = j;
	 }
	 j++;

	 nr_uploads = 0;
      }
   }
   /* Upload non-interleaved arrays */
   for (i = 0; i < nr_uploads; i++) {
      struct brw_vertex_buffer *buffer = &brw->vb.buffers[j];
      if (upload[i]->glarray->InstanceDivisor == 0) {
         copy_array_to_vbo_array(brw, upload[i], min_index, max_index,
                                 buffer, upload[i]->glarray->_ElementSize);
      } else {
         /* This is an instanced attribute, since its InstanceDivisor
          * is not zero. Therefore, its data will be stepped after the
          * instanced draw has been run InstanceDivisor times.
          */
         uint32_t instanced_attr_max_index =
            (brw->num_instances - 1) / upload[i]->glarray->InstanceDivisor;
         copy_array_to_vbo_array(brw, upload[i], 0, instanced_attr_max_index,
                                 buffer, upload[i]->glarray->_ElementSize);
      }
      buffer->offset -= delta * buffer->stride;
      buffer->size += delta * buffer->stride;
      buffer->step_rate = upload[i]->glarray->InstanceDivisor;
      upload[i]->buffer = j++;
      upload[i]->offset = 0;
   }

   brw->vb.nr_buffers = j;
}

void
brw_prepare_shader_draw_parameters(struct brw_context *brw)
{
   const struct brw_vs_prog_data *vs_prog_data =
      brw_vs_prog_data(brw->vs.base.prog_data);

   /* For non-indirect draws, upload gl_BaseVertex. */
   if ((vs_prog_data->uses_basevertex || vs_prog_data->uses_baseinstance) &&
       brw->draw.draw_params_bo == NULL) {
      intel_upload_data(brw, &brw->draw.params, sizeof(brw->draw.params), 4,
			&brw->draw.draw_params_bo,
                        &brw->draw.draw_params_offset);
   }

   if (vs_prog_data->uses_drawid) {
      intel_upload_data(brw, &brw->draw.gl_drawid, sizeof(brw->draw.gl_drawid), 4,
                        &brw->draw.draw_id_bo,
                        &brw->draw.draw_id_offset);
   }
}

/**
 * Emit a VERTEX_BUFFER_STATE entry (part of 3DSTATE_VERTEX_BUFFERS).
 */
uint32_t *
brw_emit_vertex_buffer_state(struct brw_context *brw,
                             unsigned buffer_nr,
                             drm_intel_bo *bo,
                             unsigned start_offset,
                             unsigned end_offset,
                             unsigned stride,
                             unsigned step_rate,
                             uint32_t *__map)
{
   struct gl_context *ctx = &brw->ctx;
   uint32_t dw0;

   if (brw->gen >= 8) {
      dw0 = buffer_nr << GEN6_VB0_INDEX_SHIFT;
   } else if (brw->gen >= 6) {
      dw0 = (buffer_nr << GEN6_VB0_INDEX_SHIFT) |
            (step_rate ? GEN6_VB0_ACCESS_INSTANCEDATA
                       : GEN6_VB0_ACCESS_VERTEXDATA);
   } else {
      dw0 = (buffer_nr << BRW_VB0_INDEX_SHIFT) |
            (step_rate ? BRW_VB0_ACCESS_INSTANCEDATA
                       : BRW_VB0_ACCESS_VERTEXDATA);
   }

   if (brw->gen >= 7)
      dw0 |= GEN7_VB0_ADDRESS_MODIFYENABLE;

   switch (brw->gen) {
   case 7:
      dw0 |= GEN7_MOCS_L3 << 16;
      break;
   case 8:
      dw0 |= BDW_MOCS_WB << 16;
      break;
   case 9:
      dw0 |= SKL_MOCS_WB << 16;
      break;
   }

   WARN_ONCE(stride >= (brw->gen >= 5 ? 2048 : 2047),
             "VBO stride %d too large, bad rendering may occur\n",
             stride);
   OUT_BATCH(dw0 | (stride << BRW_VB0_PITCH_SHIFT));
   if (brw->gen >= 8) {
      OUT_RELOC64(bo, I915_GEM_DOMAIN_VERTEX, 0, start_offset);
      /* From the BSpec: 3D Pipeline Stages - 3D Pipeline Geometry -
       *                 Vertex Fetch (VF) Stage - State
       *
       * Instead of "VBState.StartingBufferAddress + VBState.MaxIndex x
       * VBState.BufferPitch", the address of the byte immediately beyond the
       * last valid byte of the buffer is determined by
       * "VBState.StartingBufferAddress + VBState.BufferSize".
       */
      OUT_BATCH(end_offset - start_offset);
   } else if (brw->gen >= 5) {
      OUT_RELOC(bo, I915_GEM_DOMAIN_VERTEX, 0, start_offset);
      /* From the BSpec: 3D Pipeline Stages - 3D Pipeline Geometry -
       *                 Vertex Fetch (VF) Stage - State
       *
       *  Instead of "VBState.StartingBufferAddress + VBState.MaxIndex x
       *  VBState.BufferPitch", the address of the byte immediately beyond the
       *  last valid byte of the buffer is determined by
       *  "VBState.EndAddress + 1".
       */
      OUT_RELOC(bo, I915_GEM_DOMAIN_VERTEX, 0, end_offset - 1);
      OUT_BATCH(step_rate);
   } else {
      OUT_RELOC(bo, I915_GEM_DOMAIN_VERTEX, 0, start_offset);
      OUT_BATCH(0);
      OUT_BATCH(step_rate);
   }

   return __map;
}

static void
brw_emit_vertices(struct brw_context *brw)
{
   GLuint i;

   brw_prepare_vertices(brw);
   brw_prepare_shader_draw_parameters(brw);

   brw_emit_query_begin(brw);

   const struct brw_vs_prog_data *vs_prog_data =
      brw_vs_prog_data(brw->vs.base.prog_data);

   unsigned nr_elements = brw->vb.nr_enabled;
   if (vs_prog_data->uses_vertexid || vs_prog_data->uses_instanceid ||
       vs_prog_data->uses_basevertex || vs_prog_data->uses_baseinstance)
      ++nr_elements;
   if (vs_prog_data->uses_drawid)
      nr_elements++;

   /* If any of the formats of vb.enabled needs more that one upload, we need
    * to add it to nr_elements */
   unsigned extra_uploads = 0;
   for (unsigned i = 0; i < brw->vb.nr_enabled; i++) {
      struct brw_vertex_element *input = brw->vb.enabled[i];
      uint32_t format = brw_get_vertex_surface_type(brw, input->glarray);

      if (uploads_needed(format) > 1)
         extra_uploads++;
   }
   nr_elements += extra_uploads;

   /* If the VS doesn't read any inputs (calculating vertex position from
    * a state variable for some reason, for example), emit a single pad
    * VERTEX_ELEMENT struct and bail.
    *
    * The stale VB state stays in place, but they don't do anything unless
    * a VE loads from them.
    */
   if (nr_elements == 0) {
      BEGIN_BATCH(3);
      OUT_BATCH((_3DSTATE_VERTEX_ELEMENTS << 16) | 1);
      if (brw->gen >= 6) {
	 OUT_BATCH((0 << GEN6_VE0_INDEX_SHIFT) |
		   GEN6_VE0_VALID |
		   (BRW_SURFACEFORMAT_R32G32B32A32_FLOAT << BRW_VE0_FORMAT_SHIFT) |
		   (0 << BRW_VE0_SRC_OFFSET_SHIFT));
      } else {
	 OUT_BATCH((0 << BRW_VE0_INDEX_SHIFT) |
		   BRW_VE0_VALID |
		   (BRW_SURFACEFORMAT_R32G32B32A32_FLOAT << BRW_VE0_FORMAT_SHIFT) |
		   (0 << BRW_VE0_SRC_OFFSET_SHIFT));
      }
      OUT_BATCH((BRW_VE1_COMPONENT_STORE_0 << BRW_VE1_COMPONENT_0_SHIFT) |
		(BRW_VE1_COMPONENT_STORE_0 << BRW_VE1_COMPONENT_1_SHIFT) |
		(BRW_VE1_COMPONENT_STORE_0 << BRW_VE1_COMPONENT_2_SHIFT) |
		(BRW_VE1_COMPONENT_STORE_1_FLT << BRW_VE1_COMPONENT_3_SHIFT));
      ADVANCE_BATCH();
      return;
   }

   /* Now emit VB and VEP state packets.
    */

   const bool uses_draw_params =
      vs_prog_data->uses_basevertex ||
      vs_prog_data->uses_baseinstance;
   const unsigned nr_buffers = brw->vb.nr_buffers +
      uses_draw_params + vs_prog_data->uses_drawid;

   if (nr_buffers) {
      if (brw->gen >= 6) {
	 assert(nr_buffers <= 33);
      } else {
	 assert(nr_buffers <= 17);
      }

      BEGIN_BATCH(1 + 4 * nr_buffers);
      OUT_BATCH((_3DSTATE_VERTEX_BUFFERS << 16) | (4 * nr_buffers - 1));
      for (i = 0; i < brw->vb.nr_buffers; i++) {
	 struct brw_vertex_buffer *buffer = &brw->vb.buffers[i];
         /* Prior to Haswell and Bay Trail we have to use 4-component formats
          * to fake 3-component ones.  In particular, we do this for
          * half-float and 8 and 16-bit integer formats.  This means that the
          * vertex element may poke over the end of the buffer by 2 bytes.
          */
         unsigned padding =
            (brw->gen <= 7 && !brw->is_baytrail && !brw->is_haswell) * 2;
         EMIT_VERTEX_BUFFER_STATE(brw, i, buffer->bo, buffer->offset,
                                  buffer->offset + buffer->size + padding,
                                  buffer->stride, buffer->step_rate);

      }

      if (uses_draw_params) {
         EMIT_VERTEX_BUFFER_STATE(brw, brw->vb.nr_buffers,
                                  brw->draw.draw_params_bo,
                                  brw->draw.draw_params_offset,
                                  brw->draw.draw_params_bo->size,
                                  0,  /* stride */
                                  0); /* step rate */
      }

      if (vs_prog_data->uses_drawid) {
         EMIT_VERTEX_BUFFER_STATE(brw, brw->vb.nr_buffers + 1,
                                  brw->draw.draw_id_bo,
                                  brw->draw.draw_id_offset,
                                  brw->draw.draw_id_bo->size,
                                  0,  /* stride */
                                  0); /* step rate */
      }

      ADVANCE_BATCH();
   }

   /* The hardware allows one more VERTEX_ELEMENTS than VERTEX_BUFFERS, presumably
    * for VertexID/InstanceID.
    */
   if (brw->gen >= 6) {
      assert(nr_elements <= 34);
   } else {
      assert(nr_elements <= 18);
   }

   struct brw_vertex_element *gen6_edgeflag_input = NULL;

   BEGIN_BATCH(1 + nr_elements * 2);
   OUT_BATCH((_3DSTATE_VERTEX_ELEMENTS << 16) | (2 * nr_elements - 1));
   for (i = 0; i < brw->vb.nr_enabled; i++) {
      struct brw_vertex_element *input = brw->vb.enabled[i];
      uint32_t format = brw_get_vertex_surface_type(brw, input->glarray);
      uint32_t comp0 = BRW_VE1_COMPONENT_STORE_SRC;
      uint32_t comp1 = BRW_VE1_COMPONENT_STORE_SRC;
      uint32_t comp2 = BRW_VE1_COMPONENT_STORE_SRC;
      uint32_t comp3 = BRW_VE1_COMPONENT_STORE_SRC;
      unsigned num_uploads = 1;
      unsigned c;

      num_uploads = uploads_needed(format);

      if (input == &brw->vb.inputs[VERT_ATTRIB_EDGEFLAG]) {
         /* Gen6+ passes edgeflag as sideband along with the vertex, instead
          * of in the VUE.  We have to upload it sideband as the last vertex
          * element according to the B-Spec.
          */
         if (brw->gen >= 6) {
            gen6_edgeflag_input = input;
            continue;
         }
      }

      for (c = 0; c < num_uploads; c++) {
         uint32_t upload_format = downsize_format_if_needed(format, c);
         /* If we need more that one upload, the offset stride would be 128
          * bits (16 bytes), as for previous uploads we are using the full
          * entry. */
         unsigned int offset = input->offset + c * 16;
         int size = input->glarray->Size;

         if (is_passthru_format(format))
            size = upload_format_size(upload_format);

         switch (size) {
         case 0: comp0 = BRW_VE1_COMPONENT_STORE_0;
         case 1: comp1 = BRW_VE1_COMPONENT_STORE_0;
         case 2: comp2 = BRW_VE1_COMPONENT_STORE_0;
         case 3: comp3 = input->glarray->Integer
                         ? BRW_VE1_COMPONENT_STORE_1_INT
                         : BRW_VE1_COMPONENT_STORE_1_FLT;
            break;
         }

         if (brw->gen >= 6) {
            OUT_BATCH((input->buffer << GEN6_VE0_INDEX_SHIFT) |
                      GEN6_VE0_VALID |
                      (upload_format << BRW_VE0_FORMAT_SHIFT) |
                      (offset << BRW_VE0_SRC_OFFSET_SHIFT));
         } else {
            OUT_BATCH((input->buffer << BRW_VE0_INDEX_SHIFT) |
                      BRW_VE0_VALID |
                      (upload_format << BRW_VE0_FORMAT_SHIFT) |
                      (offset << BRW_VE0_SRC_OFFSET_SHIFT));
         }

         if (brw->gen >= 5)
            OUT_BATCH((comp0 << BRW_VE1_COMPONENT_0_SHIFT) |
                      (comp1 << BRW_VE1_COMPONENT_1_SHIFT) |
                      (comp2 << BRW_VE1_COMPONENT_2_SHIFT) |
                      (comp3 << BRW_VE1_COMPONENT_3_SHIFT));
         else
            OUT_BATCH((comp0 << BRW_VE1_COMPONENT_0_SHIFT) |
                      (comp1 << BRW_VE1_COMPONENT_1_SHIFT) |
                      (comp2 << BRW_VE1_COMPONENT_2_SHIFT) |
                      (comp3 << BRW_VE1_COMPONENT_3_SHIFT) |
                      ((i * 4) << BRW_VE1_DST_OFFSET_SHIFT));
      }
   }

   if (vs_prog_data->uses_vertexid || vs_prog_data->uses_instanceid ||
       vs_prog_data->uses_basevertex || vs_prog_data->uses_baseinstance) {
      uint32_t dw0 = 0, dw1 = 0;
      uint32_t comp0 = BRW_VE1_COMPONENT_STORE_0;
      uint32_t comp1 = BRW_VE1_COMPONENT_STORE_0;
      uint32_t comp2 = BRW_VE1_COMPONENT_STORE_0;
      uint32_t comp3 = BRW_VE1_COMPONENT_STORE_0;

      if (vs_prog_data->uses_basevertex)
         comp0 = BRW_VE1_COMPONENT_STORE_SRC;

      if (vs_prog_data->uses_baseinstance)
         comp1 = BRW_VE1_COMPONENT_STORE_SRC;

      if (vs_prog_data->uses_vertexid)
         comp2 = BRW_VE1_COMPONENT_STORE_VID;

      if (vs_prog_data->uses_instanceid)
         comp3 = BRW_VE1_COMPONENT_STORE_IID;

      dw1 = (comp0 << BRW_VE1_COMPONENT_0_SHIFT) |
            (comp1 << BRW_VE1_COMPONENT_1_SHIFT) |
            (comp2 << BRW_VE1_COMPONENT_2_SHIFT) |
            (comp3 << BRW_VE1_COMPONENT_3_SHIFT);

      if (brw->gen >= 6) {
         dw0 |= GEN6_VE0_VALID |
                brw->vb.nr_buffers << GEN6_VE0_INDEX_SHIFT |
                BRW_SURFACEFORMAT_R32G32_UINT << BRW_VE0_FORMAT_SHIFT;
      } else {
         dw0 |= BRW_VE0_VALID |
                brw->vb.nr_buffers << BRW_VE0_INDEX_SHIFT |
                BRW_SURFACEFORMAT_R32G32_UINT << BRW_VE0_FORMAT_SHIFT;
	 dw1 |= (i * 4) << BRW_VE1_DST_OFFSET_SHIFT;
      }

      /* Note that for gl_VertexID, gl_InstanceID, and gl_PrimitiveID values,
       * the format is ignored and the value is always int.
       */

      OUT_BATCH(dw0);
      OUT_BATCH(dw1);
   }

   if (vs_prog_data->uses_drawid) {
      uint32_t dw0 = 0, dw1 = 0;

      dw1 = (BRW_VE1_COMPONENT_STORE_SRC << BRW_VE1_COMPONENT_0_SHIFT) |
            (BRW_VE1_COMPONENT_STORE_0   << BRW_VE1_COMPONENT_1_SHIFT) |
            (BRW_VE1_COMPONENT_STORE_0   << BRW_VE1_COMPONENT_2_SHIFT) |
            (BRW_VE1_COMPONENT_STORE_0   << BRW_VE1_COMPONENT_3_SHIFT);

      if (brw->gen >= 6) {
         dw0 |= GEN6_VE0_VALID |
                ((brw->vb.nr_buffers + 1) << GEN6_VE0_INDEX_SHIFT) |
                (BRW_SURFACEFORMAT_R32_UINT << BRW_VE0_FORMAT_SHIFT);
      } else {
         dw0 |= BRW_VE0_VALID |
                ((brw->vb.nr_buffers + 1) << BRW_VE0_INDEX_SHIFT) |
                (BRW_SURFACEFORMAT_R32_UINT << BRW_VE0_FORMAT_SHIFT);

	 dw1 |= (i * 4) << BRW_VE1_DST_OFFSET_SHIFT;
      }

      OUT_BATCH(dw0);
      OUT_BATCH(dw1);
   }

   if (brw->gen >= 6 && gen6_edgeflag_input) {
      uint32_t format =
         brw_get_vertex_surface_type(brw, gen6_edgeflag_input->glarray);

      OUT_BATCH((gen6_edgeflag_input->buffer << GEN6_VE0_INDEX_SHIFT) |
                GEN6_VE0_VALID |
                GEN6_VE0_EDGE_FLAG_ENABLE |
                (format << BRW_VE0_FORMAT_SHIFT) |
                (gen6_edgeflag_input->offset << BRW_VE0_SRC_OFFSET_SHIFT));
      OUT_BATCH((BRW_VE1_COMPONENT_STORE_SRC << BRW_VE1_COMPONENT_0_SHIFT) |
                (BRW_VE1_COMPONENT_STORE_0 << BRW_VE1_COMPONENT_1_SHIFT) |
                (BRW_VE1_COMPONENT_STORE_0 << BRW_VE1_COMPONENT_2_SHIFT) |
                (BRW_VE1_COMPONENT_STORE_0 << BRW_VE1_COMPONENT_3_SHIFT));
   }

   ADVANCE_BATCH();
}

const struct brw_tracked_state brw_vertices = {
   .dirty = {
      .mesa = _NEW_POLYGON,
      .brw = BRW_NEW_BATCH |
             BRW_NEW_BLORP |
             BRW_NEW_VERTICES |
             BRW_NEW_VS_PROG_DATA,
   },
   .emit = brw_emit_vertices,
};

static void
brw_upload_indices(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   const struct _mesa_index_buffer *index_buffer = brw->ib.ib;
   GLuint ib_size;
   drm_intel_bo *old_bo = brw->ib.bo;
   struct gl_buffer_object *bufferobj;
   GLuint offset;
   GLuint ib_type_size;

   if (index_buffer == NULL)
      return;

   ib_type_size = _mesa_sizeof_type(index_buffer->type);
   ib_size = index_buffer->count ? ib_type_size * index_buffer->count :
                                   index_buffer->obj->Size;
   bufferobj = index_buffer->obj;

   /* Turn into a proper VBO:
    */
   if (!_mesa_is_bufferobj(bufferobj)) {
      /* Get new bufferobj, offset:
       */
      intel_upload_data(brw, index_buffer->ptr, ib_size, ib_type_size,
			&brw->ib.bo, &offset);
      brw->ib.size = brw->ib.bo->size;
   } else {
      offset = (GLuint) (unsigned long) index_buffer->ptr;

      /* If the index buffer isn't aligned to its element size, we have to
       * rebase it into a temporary.
       */
      if ((ib_type_size - 1) & offset) {
         perf_debug("copying index buffer to a temporary to work around "
                    "misaligned offset %d\n", offset);

         GLubyte *map = ctx->Driver.MapBufferRange(ctx,
                                                   offset,
                                                   ib_size,
                                                   GL_MAP_READ_BIT,
                                                   bufferobj,
                                                   MAP_INTERNAL);

         intel_upload_data(brw, map, ib_size, ib_type_size,
                           &brw->ib.bo, &offset);
         brw->ib.size = brw->ib.bo->size;

         ctx->Driver.UnmapBuffer(ctx, bufferobj, MAP_INTERNAL);
      } else {
         drm_intel_bo *bo =
            intel_bufferobj_buffer(brw, intel_buffer_object(bufferobj),
                                   offset, ib_size);
         if (bo != brw->ib.bo) {
            drm_intel_bo_unreference(brw->ib.bo);
            brw->ib.bo = bo;
            brw->ib.size = bufferobj->Size;
            drm_intel_bo_reference(bo);
         }
      }
   }

   /* Use 3DPRIMITIVE's start_vertex_offset to avoid re-uploading
    * the index buffer state when we're just moving the start index
    * of our drawing.
    */
   brw->ib.start_vertex_offset = offset / ib_type_size;

   if (brw->ib.bo != old_bo)
      brw->ctx.NewDriverState |= BRW_NEW_INDEX_BUFFER;

   if (index_buffer->type != brw->ib.type) {
      brw->ib.type = index_buffer->type;
      brw->ctx.NewDriverState |= BRW_NEW_INDEX_BUFFER;
   }
}

const struct brw_tracked_state brw_indices = {
   .dirty = {
      .mesa = 0,
      .brw = BRW_NEW_BLORP |
             BRW_NEW_INDICES,
   },
   .emit = brw_upload_indices,
};

static void
brw_emit_index_buffer(struct brw_context *brw)
{
   const struct _mesa_index_buffer *index_buffer = brw->ib.ib;
   GLuint cut_index_setting;

   if (index_buffer == NULL)
      return;

   if (brw->prim_restart.enable_cut_index && !brw->is_haswell) {
      cut_index_setting = BRW_CUT_INDEX_ENABLE;
   } else {
      cut_index_setting = 0;
   }

   BEGIN_BATCH(3);
   OUT_BATCH(CMD_INDEX_BUFFER << 16 |
             cut_index_setting |
             brw_get_index_type(index_buffer->type) |
             1);
   OUT_RELOC(brw->ib.bo,
             I915_GEM_DOMAIN_VERTEX, 0,
             0);
   OUT_RELOC(brw->ib.bo,
             I915_GEM_DOMAIN_VERTEX, 0,
	     brw->ib.size - 1);
   ADVANCE_BATCH();
}

const struct brw_tracked_state brw_index_buffer = {
   .dirty = {
      .mesa = 0,
      .brw = BRW_NEW_BATCH |
             BRW_NEW_BLORP |
             BRW_NEW_INDEX_BUFFER,
   },
   .emit = brw_emit_index_buffer,
};
