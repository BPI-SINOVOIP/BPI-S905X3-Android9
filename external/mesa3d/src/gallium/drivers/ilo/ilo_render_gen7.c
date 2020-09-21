/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2013 LunarG, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Chia-I Wu <olv@lunarg.com>
 */

#include "genhw/genhw.h"
#include "core/ilo_builder_3d.h"
#include "core/ilo_builder_render.h"

#include "ilo_blitter.h"
#include "ilo_resource.h"
#include "ilo_shader.h"
#include "ilo_state.h"
#include "ilo_render_gen.h"

static void
gen7_wa_post_3dstate_push_constant_alloc_ps(struct ilo_render *r)
{
   /*
    * From the Ivy Bridge PRM, volume 2 part 1, page 292:
    *
    *     "A PIPE_CONTOL command with the CS Stall bit set must be programmed
    *      in the ring after this instruction
    *      (3DSTATE_PUSH_CONSTANT_ALLOC_PS)."
    */
   const uint32_t dw1 = GEN6_PIPE_CONTROL_CS_STALL;

   ILO_DEV_ASSERT(r->dev, 7, 7);

   r->state.deferred_pipe_control_dw1 |= dw1;
}

static void
gen7_wa_pre_vs(struct ilo_render *r)
{
   /*
    * From the Ivy Bridge PRM, volume 2 part 1, page 106:
    *
    *     "A PIPE_CONTROL with Post-Sync Operation set to 1h and a depth stall
    *      needs to be sent just prior to any 3DSTATE_VS, 3DSTATE_URB_VS,
    *      3DSTATE_CONSTANT_VS, 3DSTATE_BINDING_TABLE_POINTER_VS,
    *      3DSTATE_SAMPLER_STATE_POINTER_VS command.  Only one PIPE_CONTROL
    *      needs to be sent before any combination of VS associated 3DSTATE."
    */
   const uint32_t dw1 = GEN6_PIPE_CONTROL_DEPTH_STALL |
                        GEN6_PIPE_CONTROL_WRITE_IMM;

   ILO_DEV_ASSERT(r->dev, 7, 7);

   if ((r->state.current_pipe_control_dw1 & dw1) != dw1)
      ilo_render_pipe_control(r, dw1);
}

static void
gen7_wa_pre_3dstate_sf_depth_bias(struct ilo_render *r)
{
   /*
    * From the Ivy Bridge PRM, volume 2 part 1, page 258:
    *
    *     "Due to an HW issue driver needs to send a pipe control with stall
    *      when ever there is state change in depth bias related state (in
    *      3DSTATE_SF)"
    */
   const uint32_t dw1 = GEN6_PIPE_CONTROL_CS_STALL;

   ILO_DEV_ASSERT(r->dev, 7, 7);

   if ((r->state.current_pipe_control_dw1 & dw1) != dw1)
      ilo_render_pipe_control(r, dw1);
}

static void
gen7_wa_pre_3dstate_multisample(struct ilo_render *r)
{
   /*
    * From the Ivy Bridge PRM, volume 2 part 1, page 304:
    *
    *     "Driver must ierarchi that all the caches in the depth pipe are
    *      flushed before this command (3DSTATE_MULTISAMPLE) is parsed. This
    *      requires driver to send a PIPE_CONTROL with a CS stall along with a
    *      Depth Flush prior to this command.
    */
   const uint32_t dw1 = GEN6_PIPE_CONTROL_DEPTH_CACHE_FLUSH |
                        GEN6_PIPE_CONTROL_CS_STALL;

   ILO_DEV_ASSERT(r->dev, 7, 7.5);

   if ((r->state.current_pipe_control_dw1 & dw1) != dw1)
      ilo_render_pipe_control(r, dw1);
}

static void
gen7_wa_pre_depth(struct ilo_render *r)
{
   ILO_DEV_ASSERT(r->dev, 7, 7.5);

   if (ilo_dev_gen(r->dev) == ILO_GEN(7)) {
      /*
       * From the Ivy Bridge PRM, volume 2 part 1, page 315:
       *
       *     "Driver must send a least one PIPE_CONTROL command with CS Stall
       *      and a post sync operation prior to the group of depth
       *      commands(3DSTATE_DEPTH_BUFFER, 3DSTATE_CLEAR_PARAMS,
       *      3DSTATE_STENCIL_BUFFER, and 3DSTATE_HIER_DEPTH_BUFFER)."
       */
      const uint32_t dw1 = GEN6_PIPE_CONTROL_CS_STALL |
                           GEN6_PIPE_CONTROL_WRITE_IMM;

      if ((r->state.current_pipe_control_dw1 & dw1) != dw1)
         ilo_render_pipe_control(r, dw1);
   }

   /*
    * From the Ivy Bridge PRM, volume 2 part 1, page 315:
    *
    *     "Restriction: Prior to changing Depth/Stencil Buffer state (i.e.,
    *      any combination of 3DSTATE_DEPTH_BUFFER, 3DSTATE_CLEAR_PARAMS,
    *      3DSTATE_STENCIL_BUFFER, 3DSTATE_HIER_DEPTH_BUFFER) SW must first
    *      issue a pipelined depth stall (PIPE_CONTROL with Depth Stall bit
    *      set), followed by a pipelined depth cache flush (PIPE_CONTROL with
    *      Depth Flush Bit set, followed by another pipelined depth stall
    *      (PIPE_CONTROL with Depth Stall Bit set), unless SW can otherwise
    *      guarantee that the pipeline from WM onwards is already flushed
    *      (e.g., via a preceding MI_FLUSH)."
    */
   ilo_render_pipe_control(r, GEN6_PIPE_CONTROL_DEPTH_STALL);
   ilo_render_pipe_control(r, GEN6_PIPE_CONTROL_DEPTH_CACHE_FLUSH);
   ilo_render_pipe_control(r, GEN6_PIPE_CONTROL_DEPTH_STALL);
}

static void
gen7_wa_pre_3dstate_ps_max_threads(struct ilo_render *r)
{
   /*
    * From the Ivy Bridge PRM, volume 2 part 1, page 286:
    *
    *     "If this field (Maximum Number of Threads in 3DSTATE_PS) is changed
    *      between 3DPRIMITIVE commands, a PIPE_CONTROL command with Stall at
    *      Pixel Scoreboard set is required to be issued."
    */
   const uint32_t dw1 = GEN6_PIPE_CONTROL_PIXEL_SCOREBOARD_STALL;

   ILO_DEV_ASSERT(r->dev, 7, 7.5);

   if ((r->state.current_pipe_control_dw1 & dw1) != dw1)
      ilo_render_pipe_control(r, dw1);
}

static void
gen7_wa_post_ps_and_later(struct ilo_render *r)
{
   /*
    * From the Ivy Bridge PRM, volume 2 part 1, page 276:
    *
    *     "The driver must make sure a PIPE_CONTROL with the Depth Stall
    *      Enable bit set after all the following states are programmed:
    *
    *       - 3DSTATE_PS
    *       - 3DSTATE_VIEWPORT_STATE_POINTERS_CC
    *       - 3DSTATE_CONSTANT_PS
    *       - 3DSTATE_BINDING_TABLE_POINTERS_PS
    *       - 3DSTATE_SAMPLER_STATE_POINTERS_PS
    *       - 3DSTATE_CC_STATE_POINTERS
    *       - 3DSTATE_BLEND_STATE_POINTERS
    *       - 3DSTATE_DEPTH_STENCIL_STATE_POINTERS"
    */
   const uint32_t dw1 = GEN6_PIPE_CONTROL_DEPTH_STALL;

   ILO_DEV_ASSERT(r->dev, 7, 7);

   r->state.deferred_pipe_control_dw1 |= dw1;
}

#define DIRTY(state) (session->pipe_dirty & ILO_DIRTY_ ## state)

void
gen7_draw_common_urb(struct ilo_render *r,
                     const struct ilo_state_vector *vec,
                     struct ilo_render_draw_session *session)
{
   /* 3DSTATE_URB_{VS,GS,HS,DS} */
   if (session->urb_delta.dirty & (ILO_STATE_URB_3DSTATE_URB_VS |
                                   ILO_STATE_URB_3DSTATE_URB_HS |
                                   ILO_STATE_URB_3DSTATE_URB_DS |
                                   ILO_STATE_URB_3DSTATE_URB_GS)) {
      if (ilo_dev_gen(r->dev) == ILO_GEN(7))
         gen7_wa_pre_vs(r);

      gen7_3DSTATE_URB_VS(r->builder, &vec->urb);
      gen7_3DSTATE_URB_GS(r->builder, &vec->urb);
      gen7_3DSTATE_URB_HS(r->builder, &vec->urb);
      gen7_3DSTATE_URB_DS(r->builder, &vec->urb);
   }
}

void
gen7_draw_common_pcb_alloc(struct ilo_render *r,
                           const struct ilo_state_vector *vec,
                           struct ilo_render_draw_session *session)
{
   /* 3DSTATE_PUSH_CONSTANT_ALLOC_{VS,PS} */
   if (session->urb_delta.dirty &
         (ILO_STATE_URB_3DSTATE_PUSH_CONSTANT_ALLOC_VS |
          ILO_STATE_URB_3DSTATE_PUSH_CONSTANT_ALLOC_HS |
          ILO_STATE_URB_3DSTATE_PUSH_CONSTANT_ALLOC_DS |
          ILO_STATE_URB_3DSTATE_PUSH_CONSTANT_ALLOC_GS |
          ILO_STATE_URB_3DSTATE_PUSH_CONSTANT_ALLOC_PS)) {
      gen7_3DSTATE_PUSH_CONSTANT_ALLOC_VS(r->builder, &vec->urb);
      gen7_3DSTATE_PUSH_CONSTANT_ALLOC_GS(r->builder, &vec->urb);
      gen7_3DSTATE_PUSH_CONSTANT_ALLOC_PS(r->builder, &vec->urb);

      if (ilo_dev_gen(r->dev) == ILO_GEN(7))
         gen7_wa_post_3dstate_push_constant_alloc_ps(r);
   }
}

void
gen7_draw_common_pointers_1(struct ilo_render *r,
                            const struct ilo_state_vector *vec,
                            struct ilo_render_draw_session *session)
{
   /* 3DSTATE_VIEWPORT_STATE_POINTERS_{CC,SF_CLIP} */
   if (session->viewport_changed) {
      gen7_3DSTATE_VIEWPORT_STATE_POINTERS_CC(r->builder,
            r->state.CC_VIEWPORT);

      gen7_3DSTATE_VIEWPORT_STATE_POINTERS_SF_CLIP(r->builder,
            r->state.SF_CLIP_VIEWPORT);
   }
}

void
gen7_draw_common_pointers_2(struct ilo_render *r,
                            const struct ilo_state_vector *vec,
                            struct ilo_render_draw_session *session)
{
   /* 3DSTATE_BLEND_STATE_POINTERS */
   if (session->blend_changed) {
      gen7_3DSTATE_BLEND_STATE_POINTERS(r->builder,
            r->state.BLEND_STATE);
   }

   /* 3DSTATE_CC_STATE_POINTERS */
   if (session->cc_changed) {
      gen7_3DSTATE_CC_STATE_POINTERS(r->builder,
            r->state.COLOR_CALC_STATE);
   }

   /* 3DSTATE_DEPTH_STENCIL_STATE_POINTERS */
   if (ilo_dev_gen(r->dev) < ILO_GEN(8) && session->dsa_changed) {
      gen7_3DSTATE_DEPTH_STENCIL_STATE_POINTERS(r->builder,
            r->state.DEPTH_STENCIL_STATE);
   }
}

void
gen7_draw_vs(struct ilo_render *r,
             const struct ilo_state_vector *vec,
             struct ilo_render_draw_session *session)
{
   const bool emit_3dstate_binding_table = session->binding_table_vs_changed;
   const bool emit_3dstate_sampler_state = session->sampler_vs_changed;
   /* see gen6_draw_vs() */
   const bool emit_3dstate_constant_vs = session->pcb_vs_changed;
   const bool emit_3dstate_vs = (DIRTY(VS) || r->instruction_bo_changed);

   /* emit depth stall before any of the VS commands */
   if (ilo_dev_gen(r->dev) == ILO_GEN(7)) {
      if (emit_3dstate_binding_table || emit_3dstate_sampler_state ||
          emit_3dstate_constant_vs || emit_3dstate_vs)
         gen7_wa_pre_vs(r);
   }

   /* 3DSTATE_BINDING_TABLE_POINTERS_VS */
   if (emit_3dstate_binding_table) {
      gen7_3DSTATE_BINDING_TABLE_POINTERS_VS(r->builder,
              r->state.vs.BINDING_TABLE_STATE);
   }

   /* 3DSTATE_SAMPLER_STATE_POINTERS_VS */
   if (emit_3dstate_sampler_state) {
      gen7_3DSTATE_SAMPLER_STATE_POINTERS_VS(r->builder,
              r->state.vs.SAMPLER_STATE);
   }

   /* 3DSTATE_CONSTANT_VS */
   if (emit_3dstate_constant_vs) {
      gen7_3DSTATE_CONSTANT_VS(r->builder,
              &r->state.vs.PUSH_CONSTANT_BUFFER,
              &r->state.vs.PUSH_CONSTANT_BUFFER_size,
              1);
   }

   /* 3DSTATE_VS */
   if (emit_3dstate_vs) {
      const union ilo_shader_cso *cso = ilo_shader_get_kernel_cso(vec->vs);
      const uint32_t kernel_offset = ilo_shader_get_kernel_offset(vec->vs);

      if (ilo_dev_gen(r->dev) >= ILO_GEN(8)) {
         gen8_3DSTATE_VS(r->builder, &cso->vs,
               kernel_offset, r->vs_scratch.bo);
      } else {
         gen6_3DSTATE_VS(r->builder, &cso->vs,
               kernel_offset, r->vs_scratch.bo);
      }
   }
}

void
gen7_draw_hs(struct ilo_render *r,
             const struct ilo_state_vector *vec,
             struct ilo_render_draw_session *session)
{
   /* 3DSTATE_CONSTANT_HS and 3DSTATE_HS */
   if (r->hw_ctx_changed) {
      const struct ilo_state_hs *hs = &vec->disabled_hs;
      const uint32_t kernel_offset = 0;

      gen7_3DSTATE_CONSTANT_HS(r->builder, 0, 0, 0);

      if (ilo_dev_gen(r->dev) >= ILO_GEN(8))
         gen8_3DSTATE_HS(r->builder, hs, kernel_offset, NULL);
      else
         gen7_3DSTATE_HS(r->builder, hs, kernel_offset, NULL);
   }

   /* 3DSTATE_BINDING_TABLE_POINTERS_HS */
   if (r->hw_ctx_changed)
      gen7_3DSTATE_BINDING_TABLE_POINTERS_HS(r->builder, 0);
}

void
gen7_draw_te(struct ilo_render *r,
             const struct ilo_state_vector *vec,
             struct ilo_render_draw_session *session)
{
   /* 3DSTATE_TE */
   if (r->hw_ctx_changed) {
      const struct ilo_state_ds *ds = &vec->disabled_ds;
      gen7_3DSTATE_TE(r->builder, ds);
   }
}

void
gen7_draw_ds(struct ilo_render *r,
             const struct ilo_state_vector *vec,
             struct ilo_render_draw_session *session)
{
   /* 3DSTATE_CONSTANT_DS and 3DSTATE_DS */
   if (r->hw_ctx_changed) {
      const struct ilo_state_ds *ds = &vec->disabled_ds;
      const uint32_t kernel_offset = 0;

      gen7_3DSTATE_CONSTANT_DS(r->builder, 0, 0, 0);

      if (ilo_dev_gen(r->dev) >= ILO_GEN(8))
         gen8_3DSTATE_DS(r->builder, ds, kernel_offset, NULL);
      else
         gen7_3DSTATE_DS(r->builder, ds, kernel_offset, NULL);
   }

   /* 3DSTATE_BINDING_TABLE_POINTERS_DS */
   if (r->hw_ctx_changed)
      gen7_3DSTATE_BINDING_TABLE_POINTERS_DS(r->builder, 0);

}

void
gen7_draw_gs(struct ilo_render *r,
             const struct ilo_state_vector *vec,
             struct ilo_render_draw_session *session)
{
   /* 3DSTATE_CONSTANT_GS and 3DSTATE_GS */
   if (r->hw_ctx_changed) {
      const struct ilo_state_gs *gs = &vec->disabled_gs;
      const uint32_t kernel_offset = 0;

      gen7_3DSTATE_CONSTANT_GS(r->builder, 0, 0, 0);

      if (ilo_dev_gen(r->dev) >= ILO_GEN(8))
         gen8_3DSTATE_GS(r->builder, gs, kernel_offset, NULL);
      else
         gen7_3DSTATE_GS(r->builder, gs, kernel_offset, NULL);
   }

   /* 3DSTATE_BINDING_TABLE_POINTERS_GS */
   if (session->binding_table_gs_changed) {
      gen7_3DSTATE_BINDING_TABLE_POINTERS_GS(r->builder,
            r->state.gs.BINDING_TABLE_STATE);
   }
}

void
gen7_draw_sol(struct ilo_render *r,
              const struct ilo_state_vector *vec,
              struct ilo_render_draw_session *session)
{
   const struct ilo_state_sol *sol;
   const struct ilo_shader_state *shader;
   bool dirty_sh = false;

   if (vec->gs) {
      shader = vec->gs;
      dirty_sh = DIRTY(GS);
   }
   else {
      shader = vec->vs;
      dirty_sh = DIRTY(VS);
   }

   sol = ilo_shader_get_kernel_sol(shader);

   /* 3DSTATE_SO_BUFFER */
   if ((DIRTY(SO) || dirty_sh || r->batch_bo_changed) &&
       vec->so.enabled) {
      int i;

      for (i = 0; i < ILO_STATE_SOL_MAX_BUFFER_COUNT; i++) {
         const struct pipe_stream_output_target *target =
            (i < vec->so.count && vec->so.states[i]) ?
            vec->so.states[i] : NULL;
         const struct ilo_state_sol_buffer *sb = (target) ?
            &((const struct ilo_stream_output_target *) target)->sb :
            &vec->so.dummy_sb;

         if (ilo_dev_gen(r->dev) >= ILO_GEN(8))
            gen8_3DSTATE_SO_BUFFER(r->builder, sol, sb, i);
         else
            gen7_3DSTATE_SO_BUFFER(r->builder, sol, sb, i);
      }
   }

   /* 3DSTATE_SO_DECL_LIST */
   if (dirty_sh && vec->so.enabled)
      gen7_3DSTATE_SO_DECL_LIST(r->builder, sol);

   /*
    * From the Ivy Bridge PRM, volume 2 part 1, page 196-197:
    *
    *     "Anytime the SOL unit MMIO registers or non-pipeline state are
    *      written, the SOL unit needs to receive a pipeline state update with
    *      SOL unit dirty state for information programmed in MMIO/NP to get
    *      loaded into the SOL unit.
    *
    *      The SOL unit incorrectly double buffers MMIO/NP registers and only
    *      moves them into the design for usage when control topology is
    *      received with the SOL unit dirty state.
    *
    *      If the state does not change, need to resend the same state.
    *
    *      Because of corruption, software must flush the whole fixed function
    *      pipeline when 3DSTATE_STREAMOUT changes state."
    *
    * The first and fourth paragraphs are gone on Gen7.5+.
    */

   /* 3DSTATE_STREAMOUT */
   gen7_3DSTATE_STREAMOUT(r->builder, sol);
}

static void
gen7_draw_sf(struct ilo_render *r,
             const struct ilo_state_vector *vec,
             struct ilo_render_draw_session *session)
{
   /* 3DSTATE_SBE */
   if (DIRTY(FS)) {
      const struct ilo_state_sbe *sbe = ilo_shader_get_kernel_sbe(vec->fs);
      gen7_3DSTATE_SBE(r->builder, sbe);
   }

   /* 3DSTATE_SF */
   if (session->rs_delta.dirty & ILO_STATE_RASTER_3DSTATE_SF) {
      if (ilo_dev_gen(r->dev) == ILO_GEN(7))
         gen7_wa_pre_3dstate_sf_depth_bias(r);

      gen7_3DSTATE_SF(r->builder, &vec->rasterizer->rs);
   }
}

static void
gen7_draw_wm(struct ilo_render *r,
             const struct ilo_state_vector *vec,
             struct ilo_render_draw_session *session)
{
   const union ilo_shader_cso *cso = ilo_shader_get_kernel_cso(vec->fs);
   const uint32_t kernel_offset = ilo_shader_get_kernel_offset(vec->fs);

   /* 3DSTATE_WM */
   if (DIRTY(FS) || (session->rs_delta.dirty & ILO_STATE_RASTER_3DSTATE_WM))
      gen7_3DSTATE_WM(r->builder, &vec->rasterizer->rs, &cso->ps);

   /* 3DSTATE_BINDING_TABLE_POINTERS_PS */
   if (session->binding_table_fs_changed) {
      gen7_3DSTATE_BINDING_TABLE_POINTERS_PS(r->builder,
            r->state.wm.BINDING_TABLE_STATE);
   }

   /* 3DSTATE_SAMPLER_STATE_POINTERS_PS */
   if (session->sampler_fs_changed) {
      gen7_3DSTATE_SAMPLER_STATE_POINTERS_PS(r->builder,
            r->state.wm.SAMPLER_STATE);
   }

   /* 3DSTATE_CONSTANT_PS */
   if (session->pcb_fs_changed) {
      gen7_3DSTATE_CONSTANT_PS(r->builder,
            &r->state.wm.PUSH_CONSTANT_BUFFER,
            &r->state.wm.PUSH_CONSTANT_BUFFER_size,
            1);
   }

   /* 3DSTATE_PS */
   if (DIRTY(FS) || r->instruction_bo_changed) {
      if (r->hw_ctx_changed)
         gen7_wa_pre_3dstate_ps_max_threads(r);

      gen7_3DSTATE_PS(r->builder, &cso->ps, kernel_offset, r->fs_scratch.bo);
   }

   /* 3DSTATE_SCISSOR_STATE_POINTERS */
   if (session->scissor_changed) {
      gen6_3DSTATE_SCISSOR_STATE_POINTERS(r->builder,
            r->state.SCISSOR_RECT);
   }

   {
      const bool emit_3dstate_ps = (DIRTY(FS) || DIRTY(BLEND));
      const bool emit_3dstate_depth_buffer =
         (DIRTY(FB) || DIRTY(DSA) || r->state_bo_changed);

      if (ilo_dev_gen(r->dev) == ILO_GEN(7)) {
         /* XXX what is the best way to know if this workaround is needed? */
         if (emit_3dstate_ps ||
             session->pcb_fs_changed ||
             session->viewport_changed ||
             session->binding_table_fs_changed ||
             session->sampler_fs_changed ||
             session->cc_changed ||
             session->blend_changed ||
             session->dsa_changed)
            gen7_wa_post_ps_and_later(r);
      }

      if (emit_3dstate_depth_buffer)
         gen7_wa_pre_depth(r);
   }

   /* 3DSTATE_DEPTH_BUFFER and 3DSTATE_CLEAR_PARAMS */
   if (DIRTY(FB) || r->batch_bo_changed) {
      const struct ilo_state_zs *zs;
      uint32_t clear_params;

      if (vec->fb.state.zsbuf) {
         const struct ilo_surface_cso *surface =
            (const struct ilo_surface_cso *) vec->fb.state.zsbuf;
         const struct ilo_texture_slice *slice =
            ilo_texture_get_slice(ilo_texture(surface->base.texture),
                  surface->base.u.tex.level, surface->base.u.tex.first_layer);

         assert(!surface->is_rt);
         zs = &surface->u.zs;
         clear_params = slice->clear_value;
      }
      else {
         zs = &vec->fb.null_zs;
         clear_params = 0;
      }

      gen6_3DSTATE_DEPTH_BUFFER(r->builder, zs);
      gen6_3DSTATE_HIER_DEPTH_BUFFER(r->builder, zs);
      gen6_3DSTATE_STENCIL_BUFFER(r->builder, zs);
      gen7_3DSTATE_CLEAR_PARAMS(r->builder, clear_params);
   }
}

static void
gen7_draw_wm_multisample(struct ilo_render *r,
                         const struct ilo_state_vector *vec,
                         struct ilo_render_draw_session *session)
{
   /* 3DSTATE_MULTISAMPLE */
   if (DIRTY(FB) || (session->rs_delta.dirty &
            ILO_STATE_RASTER_3DSTATE_MULTISAMPLE)) {
      const uint8_t sample_count = (vec->fb.num_samples > 4) ? 8 :
                                   (vec->fb.num_samples > 1) ? 4 : 1;

      gen7_wa_pre_3dstate_multisample(r);

      gen6_3DSTATE_MULTISAMPLE(r->builder, &vec->rasterizer->rs,
            &r->sample_pattern, sample_count);
   }

   /* 3DSTATE_SAMPLE_MASK */
   if (session->rs_delta.dirty & ILO_STATE_RASTER_3DSTATE_SAMPLE_MASK)
      gen6_3DSTATE_SAMPLE_MASK(r->builder, &vec->rasterizer->rs);
}

void
ilo_render_emit_draw_commands_gen7(struct ilo_render *render,
                                   const struct ilo_state_vector *vec,
                                   struct ilo_render_draw_session *session)
{
   ILO_DEV_ASSERT(render->dev, 7, 7.5);

   /*
    * We try to keep the order of the commands match, as closely as possible,
    * that of the classic i965 driver.  It allows us to compare the command
    * streams easily.
    */
   gen6_draw_common_select(render, vec, session);
   gen6_draw_common_sip(render, vec, session);
   gen6_draw_vf_statistics(render, vec, session);
   gen7_draw_common_pcb_alloc(render, vec, session);
   gen6_draw_common_base_address(render, vec, session);
   gen7_draw_common_pointers_1(render, vec, session);
   gen7_draw_common_urb(render, vec, session);
   gen7_draw_common_pointers_2(render, vec, session);
   gen7_draw_wm_multisample(render, vec, session);
   gen7_draw_gs(render, vec, session);
   gen7_draw_hs(render, vec, session);
   gen7_draw_te(render, vec, session);
   gen7_draw_ds(render, vec, session);
   gen7_draw_vs(render, vec, session);
   gen7_draw_sol(render, vec, session);
   gen6_draw_clip(render, vec, session);
   gen7_draw_sf(render, vec, session);
   gen7_draw_wm(render, vec, session);
   gen6_draw_wm_raster(render, vec, session);
   gen6_draw_sf_rect(render, vec, session);
   gen6_draw_vf(render, vec, session);

   ilo_render_3dprimitive(render, &vec->draw_info);
}

static void
gen7_rectlist_pcb_alloc(struct ilo_render *r,
                        const struct ilo_blitter *blitter)
{
   gen7_3DSTATE_PUSH_CONSTANT_ALLOC_VS(r->builder, &blitter->urb);
   gen7_3DSTATE_PUSH_CONSTANT_ALLOC_PS(r->builder, &blitter->urb);

   if (ilo_dev_gen(r->dev) == ILO_GEN(7))
      gen7_wa_post_3dstate_push_constant_alloc_ps(r);
}

static void
gen7_rectlist_urb(struct ilo_render *r,
                  const struct ilo_blitter *blitter)
{
   gen7_3DSTATE_URB_VS(r->builder, &blitter->urb);
   gen7_3DSTATE_URB_GS(r->builder, &blitter->urb);
   gen7_3DSTATE_URB_HS(r->builder, &blitter->urb);
   gen7_3DSTATE_URB_DS(r->builder, &blitter->urb);
}

static void
gen7_rectlist_vs_to_sf(struct ilo_render *r,
                       const struct ilo_blitter *blitter)
{
   gen7_3DSTATE_CONSTANT_VS(r->builder, NULL, NULL, 0);
   gen6_3DSTATE_VS(r->builder, &blitter->vs, 0, NULL);

   gen7_3DSTATE_CONSTANT_HS(r->builder, NULL, NULL, 0);
   gen7_3DSTATE_HS(r->builder, &blitter->hs, 0, NULL);

   gen7_3DSTATE_TE(r->builder, &blitter->ds);

   gen7_3DSTATE_CONSTANT_DS(r->builder, NULL, NULL, 0);
   gen7_3DSTATE_DS(r->builder, &blitter->ds, 0, NULL);

   gen7_3DSTATE_CONSTANT_GS(r->builder, NULL, NULL, 0);
   gen7_3DSTATE_GS(r->builder, &blitter->gs, 0, NULL);

   gen7_3DSTATE_STREAMOUT(r->builder, &blitter->sol);

   gen6_3DSTATE_CLIP(r->builder, &blitter->fb.rs);

   if (ilo_dev_gen(r->dev) == ILO_GEN(7))
      gen7_wa_pre_3dstate_sf_depth_bias(r);

   gen7_3DSTATE_SF(r->builder, &blitter->fb.rs);
   gen7_3DSTATE_SBE(r->builder, &blitter->sbe);
}

static void
gen7_rectlist_wm(struct ilo_render *r,
                 const struct ilo_blitter *blitter)
{
   gen7_3DSTATE_WM(r->builder, &blitter->fb.rs, &blitter->ps);

   gen7_3DSTATE_CONSTANT_PS(r->builder, NULL, NULL, 0);

   gen7_wa_pre_3dstate_ps_max_threads(r);
   gen7_3DSTATE_PS(r->builder, &blitter->ps, 0, NULL);
}

static void
gen7_rectlist_wm_depth(struct ilo_render *r,
                       const struct ilo_blitter *blitter)
{
   gen7_wa_pre_depth(r);

   if (blitter->uses & (ILO_BLITTER_USE_FB_DEPTH |
                        ILO_BLITTER_USE_FB_STENCIL))
      gen6_3DSTATE_DEPTH_BUFFER(r->builder, &blitter->fb.dst.u.zs);

   if (blitter->uses & ILO_BLITTER_USE_FB_DEPTH) {
      gen6_3DSTATE_HIER_DEPTH_BUFFER(r->builder,
            &blitter->fb.dst.u.zs);
   }

   if (blitter->uses & ILO_BLITTER_USE_FB_STENCIL) {
      gen6_3DSTATE_STENCIL_BUFFER(r->builder,
            &blitter->fb.dst.u.zs);
   }

   gen7_3DSTATE_CLEAR_PARAMS(r->builder,
         blitter->depth_clear_value);
}

static void
gen7_rectlist_wm_multisample(struct ilo_render *r,
                             const struct ilo_blitter *blitter)
{
   const uint8_t sample_count = (blitter->fb.num_samples > 4) ? 8 :
                                (blitter->fb.num_samples > 1) ? 4 : 1;

   gen7_wa_pre_3dstate_multisample(r);

   gen6_3DSTATE_MULTISAMPLE(r->builder, &blitter->fb.rs,
         &r->sample_pattern, sample_count);

   gen6_3DSTATE_SAMPLE_MASK(r->builder, &blitter->fb.rs);
}

void
ilo_render_emit_rectlist_commands_gen7(struct ilo_render *r,
                                       const struct ilo_blitter *blitter,
                                       const struct ilo_render_rectlist_session *session)
{
   ILO_DEV_ASSERT(r->dev, 7, 7.5);

   gen7_rectlist_wm_multisample(r, blitter);

   gen6_state_base_address(r->builder, true);

   gen6_user_3DSTATE_VERTEX_BUFFERS(r->builder,
         session->vb_start, session->vb_end,
         sizeof(blitter->vertices[0]));

   gen6_3DSTATE_VERTEX_ELEMENTS(r->builder, &blitter->vf);

   gen7_rectlist_pcb_alloc(r, blitter);

   /* needed for any VS-related commands */
   if (ilo_dev_gen(r->dev) == ILO_GEN(7))
      gen7_wa_pre_vs(r);

   gen7_rectlist_urb(r, blitter);

   if (blitter->uses & ILO_BLITTER_USE_DSA) {
      gen7_3DSTATE_DEPTH_STENCIL_STATE_POINTERS(r->builder,
            r->state.DEPTH_STENCIL_STATE);
   }

   if (blitter->uses & ILO_BLITTER_USE_CC) {
      gen7_3DSTATE_CC_STATE_POINTERS(r->builder,
            r->state.COLOR_CALC_STATE);
   }

   gen7_rectlist_vs_to_sf(r, blitter);
   gen7_rectlist_wm(r, blitter);

   if (blitter->uses & ILO_BLITTER_USE_VIEWPORT) {
      gen7_3DSTATE_VIEWPORT_STATE_POINTERS_CC(r->builder,
            r->state.CC_VIEWPORT);
   }

   gen7_rectlist_wm_depth(r, blitter);

   gen6_3DSTATE_DRAWING_RECTANGLE(r->builder, 0, 0,
         blitter->fb.width, blitter->fb.height);

   if (ilo_dev_gen(r->dev) == ILO_GEN(7))
      gen7_wa_post_ps_and_later(r);

   ilo_render_3dprimitive(r, &blitter->draw_info);
}

int
ilo_render_get_draw_commands_len_gen7(const struct ilo_render *render,
                                      const struct ilo_state_vector *vec)
{
   static int len;

   ILO_DEV_ASSERT(render->dev, 7, 7.5);

   if (!len) {
      len += GEN7_3DSTATE_URB_ANY__SIZE * 4;
      len += GEN7_3DSTATE_PUSH_CONSTANT_ALLOC_ANY__SIZE * 5;
      len += GEN6_3DSTATE_CONSTANT_ANY__SIZE * 5;
      len += GEN7_3DSTATE_POINTERS_ANY__SIZE * (5 + 5 + 4);
      len += GEN7_3DSTATE_SO_BUFFER__SIZE * 4;
      len += GEN6_PIPE_CONTROL__SIZE * 5;

      len +=
         GEN6_STATE_BASE_ADDRESS__SIZE +
         GEN6_STATE_SIP__SIZE +
         GEN6_3DSTATE_VF_STATISTICS__SIZE +
         GEN6_PIPELINE_SELECT__SIZE +
         GEN6_3DSTATE_CLEAR_PARAMS__SIZE +
         GEN6_3DSTATE_DEPTH_BUFFER__SIZE +
         GEN6_3DSTATE_STENCIL_BUFFER__SIZE +
         GEN6_3DSTATE_HIER_DEPTH_BUFFER__SIZE +
         GEN6_3DSTATE_VERTEX_BUFFERS__SIZE +
         GEN6_3DSTATE_VERTEX_ELEMENTS__SIZE +
         GEN6_3DSTATE_INDEX_BUFFER__SIZE +
         GEN75_3DSTATE_VF__SIZE +
         GEN6_3DSTATE_VS__SIZE +
         GEN6_3DSTATE_GS__SIZE +
         GEN6_3DSTATE_CLIP__SIZE +
         GEN6_3DSTATE_SF__SIZE +
         GEN6_3DSTATE_WM__SIZE +
         GEN6_3DSTATE_SAMPLE_MASK__SIZE +
         GEN7_3DSTATE_HS__SIZE +
         GEN7_3DSTATE_TE__SIZE +
         GEN7_3DSTATE_DS__SIZE +
         GEN7_3DSTATE_STREAMOUT__SIZE +
         GEN7_3DSTATE_SBE__SIZE +
         GEN7_3DSTATE_PS__SIZE +
         GEN6_3DSTATE_DRAWING_RECTANGLE__SIZE +
         GEN6_3DSTATE_POLY_STIPPLE_OFFSET__SIZE +
         GEN6_3DSTATE_POLY_STIPPLE_PATTERN__SIZE +
         GEN6_3DSTATE_LINE_STIPPLE__SIZE +
         GEN6_3DSTATE_AA_LINE_PARAMETERS__SIZE +
         GEN6_3DSTATE_MULTISAMPLE__SIZE +
         GEN7_3DSTATE_SO_DECL_LIST__SIZE +
         GEN6_3DPRIMITIVE__SIZE;
   }

   return len;
}
