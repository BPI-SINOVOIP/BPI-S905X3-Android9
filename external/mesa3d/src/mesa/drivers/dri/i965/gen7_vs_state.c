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

#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"
#include "brw_util.h"
#include "program/prog_parameter.h"
#include "program/prog_statevars.h"
#include "intel_batchbuffer.h"

static void
upload_vs_state(struct brw_context *brw)
{
   const struct gen_device_info *devinfo = &brw->screen->devinfo;
   const struct brw_stage_state *stage_state = &brw->vs.base;
   const struct brw_stage_prog_data *prog_data = stage_state->prog_data;
   const struct brw_vue_prog_data *vue_prog_data =
      brw_vue_prog_data(stage_state->prog_data);
   uint32_t floating_point_mode = 0;
   const int max_threads_shift = brw->is_haswell ?
      HSW_VS_MAX_THREADS_SHIFT : GEN6_VS_MAX_THREADS_SHIFT;

   if (!brw->is_haswell && !brw->is_baytrail)
      gen7_emit_vs_workaround_flush(brw);

   if (prog_data->use_alt_mode)
      floating_point_mode = GEN6_VS_FLOATING_POINT_MODE_ALT;

   BEGIN_BATCH(6);
   OUT_BATCH(_3DSTATE_VS << 16 | (6 - 2));
   OUT_BATCH(stage_state->prog_offset);
   OUT_BATCH(floating_point_mode |
	     ((ALIGN(stage_state->sampler_count, 4)/4) <<
              GEN6_VS_SAMPLER_COUNT_SHIFT) |
             ((prog_data->binding_table.size_bytes / 4) <<
              GEN6_VS_BINDING_TABLE_ENTRY_COUNT_SHIFT));

   if (prog_data->total_scratch) {
      OUT_RELOC(stage_state->scratch_bo,
		I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER,
		ffs(stage_state->per_thread_scratch) - 11);
   } else {
      OUT_BATCH(0);
   }

   OUT_BATCH((prog_data->dispatch_grf_start_reg <<
              GEN6_VS_DISPATCH_START_GRF_SHIFT) |
	     (vue_prog_data->urb_read_length << GEN6_VS_URB_READ_LENGTH_SHIFT) |
	     (0 << GEN6_VS_URB_ENTRY_READ_OFFSET_SHIFT));

   OUT_BATCH(((devinfo->max_vs_threads - 1) << max_threads_shift) |
	     GEN6_VS_STATISTICS_ENABLE |
	     GEN6_VS_ENABLE);
   ADVANCE_BATCH();
}

const struct brw_tracked_state gen7_vs_state = {
   .dirty = {
      .mesa  = 0,
      .brw   = BRW_NEW_BATCH |
               BRW_NEW_BLORP |
               BRW_NEW_CONTEXT |
               BRW_NEW_VS_PROG_DATA,
   },
   .emit = upload_vs_state,
};
