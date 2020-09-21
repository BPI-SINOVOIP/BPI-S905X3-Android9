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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <stdbool.h>
#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"
#include "brw_util.h"
#include "brw_wm.h"
#include "program/program.h"
#include "program/prog_parameter.h"
#include "program/prog_statevars.h"
#include "main/framebuffer.h"
#include "intel_batchbuffer.h"

static void
upload_wm_state(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   /* BRW_NEW_FS_PROG_DATA */
   const struct brw_wm_prog_data *prog_data =
      brw_wm_prog_data(brw->wm.base.prog_data);
   bool writes_depth = prog_data->computed_depth_mode != BRW_PSCDEPTH_OFF;
   uint32_t dw1, dw2;

   /* _NEW_BUFFERS */
   const bool multisampled_fbo = _mesa_geometric_samples(ctx->DrawBuffer) > 1;

   dw1 = dw2 = 0;
   dw1 |= GEN7_WM_STATISTICS_ENABLE;
   dw1 |= GEN7_WM_LINE_AA_WIDTH_1_0;
   dw1 |= GEN7_WM_LINE_END_CAP_AA_WIDTH_0_5;

   /* _NEW_LINE */
   if (ctx->Line.StippleFlag)
      dw1 |= GEN7_WM_LINE_STIPPLE_ENABLE;

   /* _NEW_POLYGON */
   if (ctx->Polygon.StippleFlag)
      dw1 |= GEN7_WM_POLYGON_STIPPLE_ENABLE;

   if (prog_data->uses_src_depth)
      dw1 |= GEN7_WM_USES_SOURCE_DEPTH;

   if (prog_data->uses_src_w)
      dw1 |= GEN7_WM_USES_SOURCE_W;

   dw1 |= prog_data->computed_depth_mode << GEN7_WM_COMPUTED_DEPTH_MODE_SHIFT;
   dw1 |= prog_data->barycentric_interp_modes <<
      GEN7_WM_BARYCENTRIC_INTERPOLATION_MODE_SHIFT;

   /* _NEW_COLOR, _NEW_MULTISAMPLE _NEW_BUFFERS */
   /* Enable if the pixel shader kernel generates and outputs oMask.
    */
   if (prog_data->uses_kill ||
       _mesa_is_alpha_test_enabled(ctx) ||
       _mesa_is_alpha_to_coverage_enabled(ctx) ||
       prog_data->uses_omask) {
      dw1 |= GEN7_WM_KILL_ENABLE;
   }

   /* _NEW_BUFFERS | _NEW_COLOR */
   if (brw_color_buffer_write_enabled(brw) || writes_depth ||
       prog_data->has_side_effects || dw1 & GEN7_WM_KILL_ENABLE) {
      dw1 |= GEN7_WM_DISPATCH_ENABLE;
   }
   if (multisampled_fbo) {
      /* _NEW_MULTISAMPLE */
      if (ctx->Multisample.Enabled)
         dw1 |= GEN7_WM_MSRAST_ON_PATTERN;
      else
         dw1 |= GEN7_WM_MSRAST_OFF_PIXEL;

      if (prog_data->persample_dispatch)
         dw2 |= GEN7_WM_MSDISPMODE_PERSAMPLE;
      else
         dw2 |= GEN7_WM_MSDISPMODE_PERPIXEL;
   } else {
      dw1 |= GEN7_WM_MSRAST_OFF_PIXEL;
      dw2 |= GEN7_WM_MSDISPMODE_PERSAMPLE;
   }

   if (prog_data->uses_sample_mask) {
      dw1 |= GEN7_WM_USES_INPUT_COVERAGE_MASK;
   }

   /* BRW_NEW_FS_PROG_DATA */
   if (prog_data->early_fragment_tests)
      dw1 |= GEN7_WM_EARLY_DS_CONTROL_PREPS;
   else if (prog_data->has_side_effects)
      dw1 |= GEN7_WM_EARLY_DS_CONTROL_PSEXEC;

   /* The "UAV access enable" bits are unnecessary on HSW because they only
    * seem to have an effect on the HW-assisted coherency mechanism which we
    * don't need, and the rasterization-related UAV_ONLY flag and the
    * DISPATCH_ENABLE bit can be set independently from it.
    * C.f. gen8_upload_ps_extra().
    *
    * BRW_NEW_FRAGMENT_PROGRAM | BRW_NEW_FS_PROG_DATA | _NEW_BUFFERS | _NEW_COLOR
    */
   if (brw->is_haswell &&
       !(brw_color_buffer_write_enabled(brw) || writes_depth) &&
       prog_data->has_side_effects)
      dw2 |= HSW_WM_UAV_ONLY;

   BEGIN_BATCH(3);
   OUT_BATCH(_3DSTATE_WM << 16 | (3 - 2));
   OUT_BATCH(dw1);
   OUT_BATCH(dw2);
   ADVANCE_BATCH();
}

const struct brw_tracked_state gen7_wm_state = {
   .dirty = {
      .mesa  = _NEW_BUFFERS |
               _NEW_COLOR |
               _NEW_LINE |
               _NEW_MULTISAMPLE |
               _NEW_POLYGON,
      .brw   = BRW_NEW_BATCH |
               BRW_NEW_BLORP |
               BRW_NEW_FS_PROG_DATA,
   },
   .emit = upload_wm_state,
};

static void
gen7_upload_ps_state(struct brw_context *brw,
                     const struct brw_stage_state *stage_state,
                     const struct brw_wm_prog_data *prog_data,
                     bool enable_dual_src_blend, unsigned sample_mask,
                     unsigned fast_clear_op)
{
   const struct gen_device_info *devinfo = &brw->screen->devinfo;
   uint32_t dw2, dw4, dw5, ksp0, ksp2;
   const int max_threads_shift = brw->is_haswell ?
      HSW_PS_MAX_THREADS_SHIFT : IVB_PS_MAX_THREADS_SHIFT;

   dw2 = dw4 = dw5 = ksp2 = 0;

   const unsigned sampler_count =
      DIV_ROUND_UP(CLAMP(stage_state->sampler_count, 0, 16), 4);
   dw2 |= SET_FIELD(sampler_count, GEN7_PS_SAMPLER_COUNT);

   dw2 |= ((prog_data->base.binding_table.size_bytes / 4) <<
           GEN7_PS_BINDING_TABLE_ENTRY_COUNT_SHIFT);

   if (prog_data->base.use_alt_mode)
      dw2 |= GEN7_PS_FLOATING_POINT_MODE_ALT;

   /* Haswell requires the sample mask to be set in this packet as well as
    * in 3DSTATE_SAMPLE_MASK; the values should match. */
   /* _NEW_BUFFERS, _NEW_MULTISAMPLE */
   if (brw->is_haswell)
      dw4 |= SET_FIELD(sample_mask, HSW_PS_SAMPLE_MASK);

   dw4 |= (devinfo->max_wm_threads - 1) << max_threads_shift;

   if (prog_data->base.nr_params > 0)
      dw4 |= GEN7_PS_PUSH_CONSTANT_ENABLE;

   /* From the IVB PRM, volume 2 part 1, page 287:
    * "This bit is inserted in the PS payload header and made available to
    * the DataPort (either via the message header or via header bypass) to
    * indicate that oMask data (one or two phases) is included in Render
    * Target Write messages. If present, the oMask data is used to mask off
    * samples."
    */
   if (prog_data->uses_omask)
      dw4 |= GEN7_PS_OMASK_TO_RENDER_TARGET;

   /* From the IVB PRM, volume 2 part 1, page 287:
    * "If the PS kernel does not need the Position XY Offsets to
    * compute a Position Value, then this field should be programmed
    * to POSOFFSET_NONE."
    * "SW Recommendation: If the PS kernel needs the Position Offsets
    * to compute a Position XY value, this field should match Position
    * ZW Interpolation Mode to ensure a consistent position.xyzw
    * computation."
    * We only require XY sample offsets. So, this recommendation doesn't
    * look useful at the moment. We might need this in future.
    */
   if (prog_data->uses_pos_offset)
      dw4 |= GEN7_PS_POSOFFSET_SAMPLE;
   else
      dw4 |= GEN7_PS_POSOFFSET_NONE;

   /* The hardware wedges if you have this bit set but don't turn on any dual
    * source blend factors.
    */
   if (enable_dual_src_blend)
      dw4 |= GEN7_PS_DUAL_SOURCE_BLEND_ENABLE;

   /* BRW_NEW_FS_PROG_DATA */
   if (prog_data->num_varying_inputs != 0)
      dw4 |= GEN7_PS_ATTRIBUTE_ENABLE;

   dw4 |= fast_clear_op;

   if (prog_data->dispatch_16)
      dw4 |= GEN7_PS_16_DISPATCH_ENABLE;

   if (prog_data->dispatch_8)
      dw4 |= GEN7_PS_8_DISPATCH_ENABLE;

   dw5 |= prog_data->base.dispatch_grf_start_reg <<
          GEN7_PS_DISPATCH_START_GRF_SHIFT_0;
   dw5 |= prog_data->dispatch_grf_start_reg_2 <<
          GEN7_PS_DISPATCH_START_GRF_SHIFT_2;

   ksp0 = stage_state->prog_offset;
   ksp2 = stage_state->prog_offset + prog_data->prog_offset_2;

   BEGIN_BATCH(8);
   OUT_BATCH(_3DSTATE_PS << 16 | (8 - 2));
   OUT_BATCH(ksp0);
   OUT_BATCH(dw2);
   if (prog_data->base.total_scratch) {
      OUT_RELOC(brw->wm.base.scratch_bo,
		I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER,
		ffs(stage_state->per_thread_scratch) - 11);
   } else {
      OUT_BATCH(0);
   }
   OUT_BATCH(dw4);
   OUT_BATCH(dw5);
   OUT_BATCH(0); /* kernel 1 pointer */
   OUT_BATCH(ksp2);
   ADVANCE_BATCH();
}

static void
upload_ps_state(struct brw_context *brw)
{
   /* BRW_NEW_FS_PROG_DATA */
   const struct brw_wm_prog_data *prog_data =
      brw_wm_prog_data(brw->wm.base.prog_data);
   const struct gl_context *ctx = &brw->ctx;
   /* BRW_NEW_FS_PROG_DATA | _NEW_COLOR */
   const bool enable_dual_src_blend = prog_data->dual_src_blend &&
                                      (ctx->Color.BlendEnabled & 1) &&
                                      ctx->Color.Blend[0]._UsesDualSrc;
   /* _NEW_BUFFERS, _NEW_MULTISAMPLE */
   const unsigned sample_mask =
      brw->is_haswell ? gen6_determine_sample_mask(brw) : 0;

   gen7_upload_ps_state(brw, &brw->wm.base, prog_data,
                        enable_dual_src_blend, sample_mask,
                        brw->wm.fast_clear_op);
}

const struct brw_tracked_state gen7_ps_state = {
   .dirty = {
      .mesa  = _NEW_BUFFERS |
               _NEW_COLOR |
               _NEW_MULTISAMPLE,
      .brw   = BRW_NEW_BATCH |
               BRW_NEW_BLORP |
               BRW_NEW_FS_PROG_DATA,
   },
   .emit = upload_ps_state,
};
