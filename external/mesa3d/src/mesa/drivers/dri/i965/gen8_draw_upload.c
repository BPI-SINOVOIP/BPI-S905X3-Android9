/*
 * Copyright © 2012 Intel Corporation
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

#include "main/bufferobj.h"
#include "main/context.h"
#include "main/enums.h"
#include "main/macros.h"

#include "brw_draw.h"
#include "brw_defines.h"
#include "brw_context.h"
#include "brw_state.h"

#include "intel_batchbuffer.h"
#include "intel_buffer_objects.h"

#ifndef NDEBUG
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
#endif

static void
gen8_emit_vertices(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   bool uses_edge_flag;

   brw_prepare_vertices(brw);
   brw_prepare_shader_draw_parameters(brw);

   uses_edge_flag = (ctx->Polygon.FrontMode != GL_FILL ||
                     ctx->Polygon.BackMode != GL_FILL);

   const struct brw_vs_prog_data *vs_prog_data =
      brw_vs_prog_data(brw->vs.base.prog_data);

   if (vs_prog_data->uses_vertexid || vs_prog_data->uses_instanceid) {
      unsigned vue = brw->vb.nr_enabled;

      /* The element for the edge flags must always be last, so we have to
       * insert the SGVS before it in that case.
       */
      if (uses_edge_flag) {
         assert(vue > 0);
         vue--;
      }

      WARN_ONCE(vue >= 33,
                "Trying to insert VID/IID past 33rd vertex element, "
                "need to reorder the vertex attrbutes.");

      unsigned dw1 = 0;
      if (vs_prog_data->uses_vertexid) {
         dw1 |= GEN8_SGVS_ENABLE_VERTEX_ID |
                (2 << GEN8_SGVS_VERTEX_ID_COMPONENT_SHIFT) |  /* .z channel */
                (vue << GEN8_SGVS_VERTEX_ID_ELEMENT_OFFSET_SHIFT);
      }

      if (vs_prog_data->uses_instanceid) {
         dw1 |= GEN8_SGVS_ENABLE_INSTANCE_ID |
                (3 << GEN8_SGVS_INSTANCE_ID_COMPONENT_SHIFT) | /* .w channel */
                (vue << GEN8_SGVS_INSTANCE_ID_ELEMENT_OFFSET_SHIFT);
      }

      BEGIN_BATCH(2);
      OUT_BATCH(_3DSTATE_VF_SGVS << 16 | (2 - 2));
      OUT_BATCH(dw1);
      ADVANCE_BATCH();

      BEGIN_BATCH(3);
      OUT_BATCH(_3DSTATE_VF_INSTANCING << 16 | (3 - 2));
      OUT_BATCH(vue | GEN8_VF_INSTANCING_ENABLE);
      OUT_BATCH(0);
      ADVANCE_BATCH();
   } else {
      BEGIN_BATCH(2);
      OUT_BATCH(_3DSTATE_VF_SGVS << 16 | (2 - 2));
      OUT_BATCH(0);
      ADVANCE_BATCH();
   }

   /* Normally we don't need an element for the SGVS attribute because the
    * 3DSTATE_VF_SGVS instruction lets you store the generated attribute in an
    * element that is past the list in 3DSTATE_VERTEX_ELEMENTS. However if
    * we're using draw parameters then we need an element for the those
    * values.  Additionally if there is an edge flag element then the SGVS
    * can't be inserted past that so we need a dummy element to ensure that
    * the edge flag is the last one.
    */
   const bool needs_sgvs_element = (vs_prog_data->uses_basevertex ||
                                    vs_prog_data->uses_baseinstance ||
                                    ((vs_prog_data->uses_instanceid ||
                                      vs_prog_data->uses_vertexid) &&
                                     uses_edge_flag));
   const unsigned nr_elements =
      brw->vb.nr_enabled + needs_sgvs_element + vs_prog_data->uses_drawid;

   /* If the VS doesn't read any inputs (calculating vertex position from
    * a state variable for some reason, for example), emit a single pad
    * VERTEX_ELEMENT struct and bail.
    *
    * The stale VB state stays in place, but they don't do anything unless
    * a VE loads from them.
    */
   if (nr_elements == 0) {
      BEGIN_BATCH(3);
      OUT_BATCH((_3DSTATE_VERTEX_ELEMENTS << 16) | (3 - 2));
      OUT_BATCH((0 << GEN6_VE0_INDEX_SHIFT) |
                GEN6_VE0_VALID |
                (BRW_SURFACEFORMAT_R32G32B32A32_FLOAT << BRW_VE0_FORMAT_SHIFT) |
                (0 << BRW_VE0_SRC_OFFSET_SHIFT));
      OUT_BATCH((BRW_VE1_COMPONENT_STORE_0 << BRW_VE1_COMPONENT_0_SHIFT) |
                (BRW_VE1_COMPONENT_STORE_0 << BRW_VE1_COMPONENT_1_SHIFT) |
                (BRW_VE1_COMPONENT_STORE_0 << BRW_VE1_COMPONENT_2_SHIFT) |
                (BRW_VE1_COMPONENT_STORE_1_FLT << BRW_VE1_COMPONENT_3_SHIFT));
      ADVANCE_BATCH();
      return;
   }

   /* Now emit 3DSTATE_VERTEX_BUFFERS and 3DSTATE_VERTEX_ELEMENTS packets. */
   const bool uses_draw_params =
      vs_prog_data->uses_basevertex ||
      vs_prog_data->uses_baseinstance;
   const unsigned nr_buffers = brw->vb.nr_buffers +
      uses_draw_params + vs_prog_data->uses_drawid;

   if (nr_buffers) {
      assert(nr_buffers <= 33);

      BEGIN_BATCH(1 + 4 * nr_buffers);
      OUT_BATCH((_3DSTATE_VERTEX_BUFFERS << 16) | (4 * nr_buffers - 1));
      for (unsigned i = 0; i < brw->vb.nr_buffers; i++) {
         const struct brw_vertex_buffer *buffer = &brw->vb.buffers[i];
         EMIT_VERTEX_BUFFER_STATE(brw, i, buffer->bo,
                                  buffer->offset,
                                  buffer->offset + buffer->size,
                                  buffer->stride, 0 /* unused */);
      }

      if (uses_draw_params) {
         EMIT_VERTEX_BUFFER_STATE(brw, brw->vb.nr_buffers,
                                  brw->draw.draw_params_bo,
                                  brw->draw.draw_params_offset,
                                  brw->draw.draw_params_bo->size,
                                  0 /* stride */,
                                  0 /* unused */);
      }

      if (vs_prog_data->uses_drawid) {
         EMIT_VERTEX_BUFFER_STATE(brw, brw->vb.nr_buffers + 1,
                                  brw->draw.draw_id_bo,
                                  brw->draw.draw_id_offset,
                                  brw->draw.draw_id_bo->size,
                                  0 /* stride */,
                                  0 /* unused */);
      }
      ADVANCE_BATCH();
   }

   /* The hardware allows one more VERTEX_ELEMENTS than VERTEX_BUFFERS,
    * presumably for VertexID/InstanceID.
    */
   assert(nr_elements <= 34);

   struct brw_vertex_element *gen6_edgeflag_input = NULL;

   BEGIN_BATCH(1 + nr_elements * 2);
   OUT_BATCH((_3DSTATE_VERTEX_ELEMENTS << 16) | (2 * nr_elements - 1));
   for (unsigned i = 0; i < brw->vb.nr_enabled; i++) {
      struct brw_vertex_element *input = brw->vb.enabled[i];
      uint32_t format = brw_get_vertex_surface_type(brw, input->glarray);
      uint32_t comp0 = BRW_VE1_COMPONENT_STORE_SRC;
      uint32_t comp1 = BRW_VE1_COMPONENT_STORE_SRC;
      uint32_t comp2 = BRW_VE1_COMPONENT_STORE_SRC;
      uint32_t comp3 = BRW_VE1_COMPONENT_STORE_SRC;

      /* From the BDW PRM, Volume 2d, page 588 (VERTEX_ELEMENT_STATE):
       * "Any SourceElementFormat of *64*_PASSTHRU cannot be used with an
       * element which has edge flag enabled."
       */
      assert(!(is_passthru_format(format) && uses_edge_flag));

      /* The gen4 driver expects edgeflag to come in as a float, and passes
       * that float on to the tests in the clipper.  Mesa's current vertex
       * attribute value for EdgeFlag is stored as a float, which works out.
       * glEdgeFlagPointer, on the other hand, gives us an unnormalized
       * integer ubyte.  Just rewrite that to convert to a float.
       */
      if (input == &brw->vb.inputs[VERT_ATTRIB_EDGEFLAG]) {
         /* Gen6+ passes edgeflag as sideband along with the vertex, instead
          * of in the VUE.  We have to upload it sideband as the last vertex
          * element according to the B-Spec.
          */
         gen6_edgeflag_input = input;
         continue;
      }

      switch (input->glarray->Size) {
      case 0: comp0 = BRW_VE1_COMPONENT_STORE_0;
      case 1: comp1 = BRW_VE1_COMPONENT_STORE_0;
      case 2: comp2 = BRW_VE1_COMPONENT_STORE_0;
      case 3:
         if (input->glarray->Doubles) {
            comp3 = BRW_VE1_COMPONENT_STORE_0;
         } else if (input->glarray->Integer) {
            comp3 = BRW_VE1_COMPONENT_STORE_1_INT;
         } else {
            comp3 = BRW_VE1_COMPONENT_STORE_1_FLT;
         }

         break;
      }

      /* From the BDW PRM, Volume 2d, page 586 (VERTEX_ELEMENT_STATE):
       *
       *     "When SourceElementFormat is set to one of the *64*_PASSTHRU
       *     formats, 64-bit components are stored in the URB without any
       *     conversion. In this case, vertex elements must be written as 128
       *     or 256 bits, with VFCOMP_STORE_0 being used to pad the output
       *     as required. E.g., if R64_PASSTHRU is used to copy a 64-bit Red
       *     component into the URB, Component 1 must be specified as
       *     VFCOMP_STORE_0 (with Components 2,3 set to VFCOMP_NOSTORE)
       *     in order to output a 128-bit vertex element, or Components 1-3 must
       *     be specified as VFCOMP_STORE_0 in order to output a 256-bit vertex
       *     element. Likewise, use of R64G64B64_PASSTHRU requires Component 3
       *     to be specified as VFCOMP_STORE_0 in order to output a 256-bit vertex
       *     element."
       */
      if (input->glarray->Doubles && !input->is_dual_slot) {
         /* Store vertex elements which correspond to double and dvec2 vertex
          * shader inputs as 128-bit vertex elements, instead of 256-bits.
          */
         comp2 = BRW_VE1_COMPONENT_NOSTORE;
         comp3 = BRW_VE1_COMPONENT_NOSTORE;
      }

      OUT_BATCH((input->buffer << GEN6_VE0_INDEX_SHIFT) |
                GEN6_VE0_VALID |
                (format << BRW_VE0_FORMAT_SHIFT) |
                (input->offset << BRW_VE0_SRC_OFFSET_SHIFT));

      OUT_BATCH((comp0 << BRW_VE1_COMPONENT_0_SHIFT) |
                (comp1 << BRW_VE1_COMPONENT_1_SHIFT) |
                (comp2 << BRW_VE1_COMPONENT_2_SHIFT) |
                (comp3 << BRW_VE1_COMPONENT_3_SHIFT));
   }

   if (needs_sgvs_element) {
      if (vs_prog_data->uses_basevertex ||
          vs_prog_data->uses_baseinstance) {
         OUT_BATCH(GEN6_VE0_VALID |
                   brw->vb.nr_buffers << GEN6_VE0_INDEX_SHIFT |
                   BRW_SURFACEFORMAT_R32G32_UINT << BRW_VE0_FORMAT_SHIFT);
         OUT_BATCH((BRW_VE1_COMPONENT_STORE_SRC << BRW_VE1_COMPONENT_0_SHIFT) |
                   (BRW_VE1_COMPONENT_STORE_SRC << BRW_VE1_COMPONENT_1_SHIFT) |
                   (BRW_VE1_COMPONENT_STORE_0 << BRW_VE1_COMPONENT_2_SHIFT) |
                   (BRW_VE1_COMPONENT_STORE_0 << BRW_VE1_COMPONENT_3_SHIFT));
      } else {
         OUT_BATCH(GEN6_VE0_VALID);
         OUT_BATCH((BRW_VE1_COMPONENT_STORE_0 << BRW_VE1_COMPONENT_0_SHIFT) |
                   (BRW_VE1_COMPONENT_STORE_0 << BRW_VE1_COMPONENT_1_SHIFT) |
                   (BRW_VE1_COMPONENT_STORE_0 << BRW_VE1_COMPONENT_2_SHIFT) |
                   (BRW_VE1_COMPONENT_STORE_0 << BRW_VE1_COMPONENT_3_SHIFT));
      }
   }

   if (vs_prog_data->uses_drawid) {
      OUT_BATCH(GEN6_VE0_VALID |
                ((brw->vb.nr_buffers + 1) << GEN6_VE0_INDEX_SHIFT) |
                (BRW_SURFACEFORMAT_R32_UINT << BRW_VE0_FORMAT_SHIFT));
      OUT_BATCH((BRW_VE1_COMPONENT_STORE_SRC << BRW_VE1_COMPONENT_0_SHIFT) |
                   (BRW_VE1_COMPONENT_STORE_0 << BRW_VE1_COMPONENT_1_SHIFT) |
                   (BRW_VE1_COMPONENT_STORE_0 << BRW_VE1_COMPONENT_2_SHIFT) |
                   (BRW_VE1_COMPONENT_STORE_0 << BRW_VE1_COMPONENT_3_SHIFT));
   }

   if (gen6_edgeflag_input) {
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

   for (unsigned i = 0, j = 0; i < brw->vb.nr_enabled; i++) {
      const struct brw_vertex_element *input = brw->vb.enabled[i];
      const struct brw_vertex_buffer *buffer = &brw->vb.buffers[input->buffer];
      unsigned element_index;

      /* The edge flag element is reordered to be the last one in the code
       * above so we need to compensate for that in the element indices used
       * below.
       */
      if (input == gen6_edgeflag_input)
         element_index = nr_elements - 1;
      else
         element_index = j++;

      BEGIN_BATCH(3);
      OUT_BATCH(_3DSTATE_VF_INSTANCING << 16 | (3 - 2));
      OUT_BATCH(element_index |
                (buffer->step_rate ? GEN8_VF_INSTANCING_ENABLE : 0));
      OUT_BATCH(buffer->step_rate);
      ADVANCE_BATCH();
   }

   if (vs_prog_data->uses_drawid) {
      const unsigned element = brw->vb.nr_enabled + needs_sgvs_element;
      BEGIN_BATCH(3);
      OUT_BATCH(_3DSTATE_VF_INSTANCING << 16 | (3 - 2));
      OUT_BATCH(element);
      OUT_BATCH(0);
      ADVANCE_BATCH();
   }
}

const struct brw_tracked_state gen8_vertices = {
   .dirty = {
      .mesa = _NEW_POLYGON,
      .brw = BRW_NEW_BATCH |
             BRW_NEW_BLORP |
             BRW_NEW_VERTICES |
             BRW_NEW_VS_PROG_DATA,
   },
   .emit = gen8_emit_vertices,
};

static void
gen8_emit_index_buffer(struct brw_context *brw)
{
   const struct _mesa_index_buffer *index_buffer = brw->ib.ib;
   uint32_t mocs_wb = brw->gen >= 9 ? SKL_MOCS_WB : BDW_MOCS_WB;

   if (index_buffer == NULL)
      return;

   BEGIN_BATCH(5);
   OUT_BATCH(CMD_INDEX_BUFFER << 16 | (5 - 2));
   OUT_BATCH(brw_get_index_type(index_buffer->type) | mocs_wb);
   OUT_RELOC64(brw->ib.bo, I915_GEM_DOMAIN_VERTEX, 0, 0);
   OUT_BATCH(brw->ib.size);
   ADVANCE_BATCH();
}

const struct brw_tracked_state gen8_index_buffer = {
   .dirty = {
      .mesa = 0,
      .brw = BRW_NEW_BATCH |
             BRW_NEW_BLORP |
             BRW_NEW_INDEX_BUFFER,
   },
   .emit = gen8_emit_index_buffer,
};

static void
gen8_emit_vf_topology(struct brw_context *brw)
{
   BEGIN_BATCH(2);
   OUT_BATCH(_3DSTATE_VF_TOPOLOGY << 16 | (2 - 2));
   OUT_BATCH(brw->primitive);
   ADVANCE_BATCH();
}

const struct brw_tracked_state gen8_vf_topology = {
   .dirty = {
      .mesa = 0,
      .brw = BRW_NEW_BLORP |
             BRW_NEW_PRIMITIVE,
   },
   .emit = gen8_emit_vf_topology,
};
