/*
 * Copyright 2013 Advanced Micro Devices, Inc.
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
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors: Marek Olšák <maraeo@gmail.com>
 *
 */

/**
 * This file contains common screen and context structures and functions
 * for r600g and radeonsi.
 */

#ifndef R600_PIPE_COMMON_H
#define R600_PIPE_COMMON_H

#include <stdio.h>

#include "radeon/radeon_winsys.h"

#include "util/u_blitter.h"
#include "util/list.h"
#include "util/u_range.h"
#include "util/slab.h"
#include "util/u_suballoc.h"
#include "util/u_transfer.h"

#define ATI_VENDOR_ID 0x1002

#define R600_RESOURCE_FLAG_TRANSFER		(PIPE_RESOURCE_FLAG_DRV_PRIV << 0)
#define R600_RESOURCE_FLAG_FLUSHED_DEPTH	(PIPE_RESOURCE_FLAG_DRV_PRIV << 1)
#define R600_RESOURCE_FLAG_FORCE_TILING		(PIPE_RESOURCE_FLAG_DRV_PRIV << 2)
#define R600_RESOURCE_FLAG_DISABLE_DCC		(PIPE_RESOURCE_FLAG_DRV_PRIV << 3)

#define R600_CONTEXT_STREAMOUT_FLUSH		(1u << 0)
/* Pipeline & streamout query controls. */
#define R600_CONTEXT_START_PIPELINE_STATS	(1u << 1)
#define R600_CONTEXT_STOP_PIPELINE_STATS	(1u << 2)
#define R600_CONTEXT_PRIVATE_FLAG		(1u << 3)

/* special primitive types */
#define R600_PRIM_RECTANGLE_LIST	PIPE_PRIM_MAX

/* Debug flags. */
/* logging */
#define DBG_TEX			(1 << 0)
/* gap - reuse */
#define DBG_COMPUTE		(1 << 2)
#define DBG_VM			(1 << 3)
/* gap - reuse */
/* shader logging */
#define DBG_FS			(1 << 5)
#define DBG_VS			(1 << 6)
#define DBG_GS			(1 << 7)
#define DBG_PS			(1 << 8)
#define DBG_CS			(1 << 9)
#define DBG_TCS			(1 << 10)
#define DBG_TES			(1 << 11)
#define DBG_NO_IR		(1 << 12)
#define DBG_NO_TGSI		(1 << 13)
#define DBG_NO_ASM		(1 << 14)
#define DBG_PREOPT_IR		(1 << 15)
#define DBG_CHECK_IR		(1 << 16)
#define DBG_NO_OPT_VARIANT	(1 << 17)
/* gaps */
#define DBG_TEST_DMA		(1 << 20)
/* Bits 21-31 are reserved for the r600g driver. */
/* features */
#define DBG_NO_ASYNC_DMA	(1llu << 32)
#define DBG_NO_HYPERZ		(1llu << 33)
#define DBG_NO_DISCARD_RANGE	(1llu << 34)
#define DBG_NO_2D_TILING	(1llu << 35)
#define DBG_NO_TILING		(1llu << 36)
#define DBG_SWITCH_ON_EOP	(1llu << 37)
#define DBG_FORCE_DMA		(1llu << 38)
#define DBG_PRECOMPILE		(1llu << 39)
#define DBG_INFO		(1llu << 40)
#define DBG_NO_WC		(1llu << 41)
#define DBG_CHECK_VM		(1llu << 42)
#define DBG_NO_DCC		(1llu << 43)
#define DBG_NO_DCC_CLEAR	(1llu << 44)
#define DBG_NO_RB_PLUS		(1llu << 45)
#define DBG_SI_SCHED		(1llu << 46)
#define DBG_MONOLITHIC_SHADERS	(1llu << 47)
#define DBG_NO_CE		(1llu << 48)
#define DBG_UNSAFE_MATH		(1llu << 49)
#define DBG_NO_DCC_FB		(1llu << 50)

#define R600_MAP_BUFFER_ALIGNMENT 64
#define R600_MAX_VIEWPORTS        16

#define SI_MAX_VARIABLE_THREADS_PER_BLOCK 1024

enum r600_coherency {
	R600_COHERENCY_NONE, /* no cache flushes needed */
	R600_COHERENCY_SHADER,
	R600_COHERENCY_CB_META,
};

#ifdef PIPE_ARCH_BIG_ENDIAN
#define R600_BIG_ENDIAN 1
#else
#define R600_BIG_ENDIAN 0
#endif

struct r600_common_context;
struct r600_perfcounters;
struct tgsi_shader_info;
struct r600_qbo_state;

struct radeon_shader_reloc {
	char name[32];
	uint64_t offset;
};

struct radeon_shader_binary {
	/** Shader code */
	unsigned char *code;
	unsigned code_size;

	/** Config/Context register state that accompanies this shader.
	 * This is a stream of dword pairs.  First dword contains the
	 * register address, the second dword contains the value.*/
	unsigned char *config;
	unsigned config_size;

	/** The number of bytes of config information for each global symbol.
	 */
	unsigned config_size_per_symbol;

	/** Constant data accessed by the shader.  This will be uploaded
	 * into a constant buffer. */
	unsigned char *rodata;
	unsigned rodata_size;

	/** List of symbol offsets for the shader */
	uint64_t *global_symbol_offsets;
	unsigned global_symbol_count;

	struct radeon_shader_reloc *relocs;
	unsigned reloc_count;

	/** Disassembled shader in a string. */
	char *disasm_string;
	char *llvm_ir_string;
};

void radeon_shader_binary_init(struct radeon_shader_binary *b);
void radeon_shader_binary_clean(struct radeon_shader_binary *b);

/* Only 32-bit buffer allocations are supported, gallium doesn't support more
 * at the moment.
 */
struct r600_resource {
	struct u_resource		b;

	/* Winsys objects. */
	struct pb_buffer		*buf;
	uint64_t			gpu_address;
	/* Memory usage if the buffer placement is optimal. */
	uint64_t			vram_usage;
	uint64_t			gart_usage;

	/* Resource properties. */
	uint64_t			bo_size;
	unsigned			bo_alignment;
	enum radeon_bo_domain		domains;
	enum radeon_bo_flag		flags;
	unsigned			bind_history;

	/* The buffer range which is initialized (with a write transfer,
	 * streamout, DMA, or as a random access target). The rest of
	 * the buffer is considered invalid and can be mapped unsynchronized.
	 *
	 * This allows unsychronized mapping of a buffer range which hasn't
	 * been used yet. It's for applications which forget to use
	 * the unsynchronized map flag and expect the driver to figure it out.
         */
	struct util_range		valid_buffer_range;

	/* For buffers only. This indicates that a write operation has been
	 * performed by TC L2, but the cache hasn't been flushed.
	 * Any hw block which doesn't use or bypasses TC L2 should check this
	 * flag and flush the cache before using the buffer.
	 *
	 * For example, TC L2 must be flushed if a buffer which has been
	 * modified by a shader store instruction is about to be used as
	 * an index buffer. The reason is that VGT DMA index fetching doesn't
	 * use TC L2.
	 */
	bool				TC_L2_dirty;

	/* Whether the resource has been exported via resource_get_handle. */
	bool				is_shared;
	unsigned			external_usage; /* PIPE_HANDLE_USAGE_* */
};

struct r600_transfer {
	struct pipe_transfer		transfer;
	struct r600_resource		*staging;
	unsigned			offset;
};

struct r600_fmask_info {
	uint64_t offset;
	uint64_t size;
	unsigned alignment;
	unsigned pitch_in_pixels;
	unsigned bank_height;
	unsigned slice_tile_max;
	unsigned tile_mode_index;
};

struct r600_cmask_info {
	uint64_t offset;
	uint64_t size;
	unsigned alignment;
	unsigned slice_tile_max;
	unsigned base_address_reg;
};

struct r600_texture {
	struct r600_resource		resource;

	uint64_t			size;
	unsigned			num_level0_transfers;
	enum pipe_format		db_render_format;
	bool				is_depth;
	bool				db_compatible;
	bool				can_sample_z;
	bool				can_sample_s;
	unsigned			dirty_level_mask; /* each bit says if that mipmap is compressed */
	unsigned			stencil_dirty_level_mask; /* each bit says if that mipmap is compressed */
	struct r600_texture		*flushed_depth_texture;
	struct radeon_surf		surface;

	/* Colorbuffer compression and fast clear. */
	struct r600_fmask_info		fmask;
	struct r600_cmask_info		cmask;
	struct r600_resource		*cmask_buffer;
	uint64_t			dcc_offset; /* 0 = disabled */
	unsigned			cb_color_info; /* fast clear enable bit */
	unsigned			color_clear_value[2];
	unsigned			last_msaa_resolve_target_micro_mode;

	/* Depth buffer compression and fast clear. */
	struct r600_resource		*htile_buffer;
	bool				tc_compatible_htile;
	bool				depth_cleared; /* if it was cleared at least once */
	float				depth_clear_value;
	bool				stencil_cleared; /* if it was cleared at least once */
	uint8_t				stencil_clear_value;

	bool				non_disp_tiling; /* R600-Cayman only */

	/* Whether the texture is a displayable back buffer and needs DCC
	 * decompression, which is expensive. Therefore, it's enabled only
	 * if statistics suggest that it will pay off and it's allocated
	 * separately. It can't be bound as a sampler by apps. Limited to
	 * target == 2D and last_level == 0. If enabled, dcc_offset contains
	 * the absolute GPUVM address, not the relative one.
	 */
	struct r600_resource		*dcc_separate_buffer;
	/* When DCC is temporarily disabled, the separate buffer is here. */
	struct r600_resource		*last_dcc_separate_buffer;
	/* We need to track DCC dirtiness, because st/dri usually calls
	 * flush_resource twice per frame (not a bug) and we don't wanna
	 * decompress DCC twice. Also, the dirty tracking must be done even
	 * if DCC isn't used, because it's required by the DCC usage analysis
	 * for a possible future enablement.
	 */
	bool				separate_dcc_dirty;
	/* Statistics gathering for the DCC enablement heuristic. */
	bool				dcc_gather_statistics;
	/* Estimate of how much this color buffer is written to in units of
	 * full-screen draws: ps_invocations / (width * height)
	 * Shader kills, late Z, and blending with trivial discards make it
	 * inaccurate (we need to count CB updates, not PS invocations).
	 */
	unsigned			ps_draw_ratio;
	/* The number of clears since the last DCC usage analysis. */
	unsigned			num_slow_clears;

	/* Counter that should be non-zero if the texture is bound to a
	 * framebuffer. Implemented in radeonsi only.
	 */
	uint32_t			framebuffers_bound;
};

struct r600_surface {
	struct pipe_surface		base;

	bool color_initialized;
	bool depth_initialized;

	/* Misc. color flags. */
	bool alphatest_bypass;
	bool export_16bpc;
	bool color_is_int8;
	bool color_is_int10;

	/* Color registers. */
	unsigned cb_color_info;
	unsigned cb_color_base;
	unsigned cb_color_view;
	unsigned cb_color_size;		/* R600 only */
	unsigned cb_color_dim;		/* EG only */
	unsigned cb_color_pitch;	/* EG and later */
	unsigned cb_color_slice;	/* EG and later */
	unsigned cb_color_attrib;	/* EG and later */
	unsigned cb_dcc_control;	/* VI and later */
	unsigned cb_color_fmask;	/* CB_COLORn_FMASK (EG and later) or CB_COLORn_FRAG (r600) */
	unsigned cb_color_fmask_slice;	/* EG and later */
	unsigned cb_color_cmask;	/* CB_COLORn_TILE (r600 only) */
	unsigned cb_color_mask;		/* R600 only */
	unsigned spi_shader_col_format;		/* SI+, no blending, no alpha-to-coverage. */
	unsigned spi_shader_col_format_alpha;	/* SI+, alpha-to-coverage */
	unsigned spi_shader_col_format_blend;	/* SI+, blending without alpha. */
	unsigned spi_shader_col_format_blend_alpha; /* SI+, blending with alpha. */
	struct r600_resource *cb_buffer_fmask; /* Used for FMASK relocations. R600 only */
	struct r600_resource *cb_buffer_cmask; /* Used for CMASK relocations. R600 only */

	/* DB registers. */
	unsigned db_depth_info;		/* R600 only, then SI and later */
	unsigned db_z_info;		/* EG and later */
	unsigned db_depth_base;		/* DB_Z_READ/WRITE_BASE (EG and later) or DB_DEPTH_BASE (r600) */
	unsigned db_depth_view;
	unsigned db_depth_size;
	unsigned db_depth_slice;	/* EG and later */
	unsigned db_stencil_base;	/* EG and later */
	unsigned db_stencil_info;	/* EG and later */
	unsigned db_prefetch_limit;	/* R600 only */
	unsigned db_htile_surface;
	unsigned db_htile_data_base;
	unsigned db_preload_control;	/* EG and later */
};

union r600_grbm_counters {
	struct {
		unsigned spi_busy;
		unsigned spi_idle;
		unsigned gui_busy;
		unsigned gui_idle;
	} named;
	unsigned array[0];
};

struct r600_common_screen {
	struct pipe_screen		b;
	struct radeon_winsys		*ws;
	enum radeon_family		family;
	enum chip_class			chip_class;
	struct radeon_info		info;
	uint64_t			debug_flags;
	bool				has_cp_dma;
	bool				has_streamout;

	struct slab_parent_pool		pool_transfers;

	/* Texture filter settings. */
	int				force_aniso; /* -1 = disabled */

	/* Auxiliary context. Mainly used to initialize resources.
	 * It must be locked prior to using and flushed before unlocking. */
	struct pipe_context		*aux_context;
	pipe_mutex			aux_context_lock;

	/* This must be in the screen, because UE4 uses one context for
	 * compilation and another one for rendering.
	 */
	unsigned			num_compilations;
	/* Along with ST_DEBUG=precompile, this should show if applications
	 * are loading shaders on demand. This is a monotonic counter.
	 */
	unsigned			num_shaders_created;
	unsigned			num_shader_cache_hits;

	/* GPU load thread. */
	pipe_mutex			gpu_load_mutex;
	pipe_thread			gpu_load_thread;
	union r600_grbm_counters	grbm_counters;
	volatile unsigned		gpu_load_stop_thread; /* bool */

	char				renderer_string[100];

	/* Performance counters. */
	struct r600_perfcounters	*perfcounters;

	/* If pipe_screen wants to re-emit the framebuffer state of all
	 * contexts, it should atomically increment this. Each context will
	 * compare this with its own last known value of the counter before
	 * drawing and re-emit the framebuffer state accordingly.
	 */
	unsigned			dirty_fb_counter;

	/* Atomically increment this counter when an existing texture's
	 * metadata is enabled or disabled in a way that requires changing
	 * contexts' compressed texture binding masks.
	 */
	unsigned			compressed_colortex_counter;

	/* Atomically increment this counter when an existing texture's
	 * backing buffer or tile mode parameters have changed that requires
	 * recomputation of shader descriptors.
	 */
	unsigned			dirty_tex_descriptor_counter;

	struct {
		/* Context flags to set so that all writes from earlier jobs
		 * in the CP are seen by L2 clients.
		 */
		unsigned cp_to_L2;

		/* Context flags to set so that all writes from earlier
		 * compute jobs are seen by L2 clients.
		 */
		unsigned compute_to_L2;
	} barrier_flags;

	void (*query_opaque_metadata)(struct r600_common_screen *rscreen,
				      struct r600_texture *rtex,
				      struct radeon_bo_metadata *md);

	void (*apply_opaque_metadata)(struct r600_common_screen *rscreen,
				    struct r600_texture *rtex,
				    struct radeon_bo_metadata *md);
};

/* This encapsulates a state or an operation which can emitted into the GPU
 * command stream. */
struct r600_atom {
	void (*emit)(struct r600_common_context *ctx, struct r600_atom *state);
	unsigned		num_dw;
	unsigned short		id;
};

struct r600_so_target {
	struct pipe_stream_output_target b;

	/* The buffer where BUFFER_FILLED_SIZE is stored. */
	struct r600_resource	*buf_filled_size;
	unsigned		buf_filled_size_offset;
	bool			buf_filled_size_valid;

	unsigned		stride_in_dw;
};

struct r600_streamout {
	struct r600_atom		begin_atom;
	bool				begin_emitted;
	unsigned			num_dw_for_end;

	unsigned			enabled_mask;
	unsigned			num_targets;
	struct r600_so_target		*targets[PIPE_MAX_SO_BUFFERS];

	unsigned			append_bitmask;
	bool				suspended;

	/* External state which comes from the vertex shader,
	 * it must be set explicitly when binding a shader. */
	unsigned			*stride_in_dw;
	unsigned			enabled_stream_buffers_mask; /* stream0 buffers0-3 in 4 LSB */

	/* The state of VGT_STRMOUT_BUFFER_(CONFIG|EN). */
	unsigned			hw_enabled_mask;

	/* The state of VGT_STRMOUT_(CONFIG|EN). */
	struct r600_atom		enable_atom;
	bool				streamout_enabled;
	bool				prims_gen_query_enabled;
	int				num_prims_gen_queries;
};

struct r600_signed_scissor {
	int minx;
	int miny;
	int maxx;
	int maxy;
};

struct r600_scissors {
	struct r600_atom		atom;
	unsigned			dirty_mask;
	struct pipe_scissor_state	states[R600_MAX_VIEWPORTS];
};

struct r600_viewports {
	struct r600_atom		atom;
	unsigned			dirty_mask;
	unsigned			depth_range_dirty_mask;
	struct pipe_viewport_state	states[R600_MAX_VIEWPORTS];
	struct r600_signed_scissor	as_scissor[R600_MAX_VIEWPORTS];
};

struct r600_ring {
	struct radeon_winsys_cs		*cs;
	void (*flush)(void *ctx, unsigned flags,
		      struct pipe_fence_handle **fence);
};

/* Saved CS data for debugging features. */
struct radeon_saved_cs {
	uint32_t			*ib;
	unsigned			num_dw;

	struct radeon_bo_list_item	*bo_list;
	unsigned			bo_count;
};

struct r600_common_context {
	struct pipe_context b; /* base class */

	struct r600_common_screen	*screen;
	struct radeon_winsys		*ws;
	struct radeon_winsys_ctx	*ctx;
	enum radeon_family		family;
	enum chip_class			chip_class;
	struct r600_ring		gfx;
	struct r600_ring		dma;
	struct pipe_fence_handle	*last_gfx_fence;
	struct pipe_fence_handle	*last_sdma_fence;
	unsigned			num_gfx_cs_flushes;
	unsigned			initial_gfx_cs_size;
	unsigned			gpu_reset_counter;
	unsigned			last_dirty_fb_counter;
	unsigned			last_compressed_colortex_counter;
	unsigned			last_dirty_tex_descriptor_counter;

	struct u_upload_mgr		*uploader;
	struct u_suballocator		*allocator_zeroed_memory;
	struct slab_child_pool		pool_transfers;

	/* Current unaccounted memory usage. */
	uint64_t			vram;
	uint64_t			gtt;

	/* States. */
	struct r600_streamout		streamout;
	struct r600_scissors		scissors;
	struct r600_viewports		viewports;
	bool				scissor_enabled;
	bool				clip_halfz;
	bool				vs_writes_viewport_index;
	bool				vs_disables_clipping_viewport;

	/* Additional context states. */
	unsigned flags; /* flush flags */

	/* Queries. */
	/* Maintain the list of active queries for pausing between IBs. */
	int				num_occlusion_queries;
	int				num_perfect_occlusion_queries;
	struct list_head		active_queries;
	unsigned			num_cs_dw_queries_suspend;
	/* Additional hardware info. */
	unsigned			backend_mask;
	unsigned			max_db; /* for OQ */
	/* Misc stats. */
	unsigned			num_draw_calls;
	unsigned			num_spill_draw_calls;
	unsigned			num_compute_calls;
	unsigned			num_spill_compute_calls;
	unsigned			num_dma_calls;
	unsigned			num_cp_dma_calls;
	unsigned			num_vs_flushes;
	unsigned			num_ps_flushes;
	unsigned			num_cs_flushes;
	unsigned			num_fb_cache_flushes;
	unsigned			num_L2_invalidates;
	unsigned			num_L2_writebacks;
	uint64_t			num_alloc_tex_transfer_bytes;
	unsigned			last_tex_ps_draw_ratio; /* for query */

	/* Render condition. */
	struct r600_atom		render_cond_atom;
	struct pipe_query		*render_cond;
	unsigned			render_cond_mode;
	bool				render_cond_invert;
	bool				render_cond_force_off; /* for u_blitter */

	/* MSAA sample locations.
	 * The first index is the sample index.
	 * The second index is the coordinate: X, Y. */
	float				sample_locations_1x[1][2];
	float				sample_locations_2x[2][2];
	float				sample_locations_4x[4][2];
	float				sample_locations_8x[8][2];
	float				sample_locations_16x[16][2];

	/* Statistics gathering for the DCC enablement heuristic. It can't be
	 * in r600_texture because r600_texture can be shared by multiple
	 * contexts. This is for back buffers only. We shouldn't get too many
	 * of those.
	 *
	 * X11 DRI3 rotates among a finite set of back buffers. They should
	 * all fit in this array. If they don't, separate DCC might never be
	 * enabled by DCC stat gathering.
	 */
	struct {
		struct r600_texture		*tex;
		/* Query queue: 0 = usually active, 1 = waiting, 2 = readback. */
		struct pipe_query		*ps_stats[3];
		/* If all slots are used and another slot is needed,
		 * the least recently used slot is evicted based on this. */
		int64_t				last_use_timestamp;
		bool				query_active;
	} dcc_stats[5];

	struct pipe_debug_callback	debug;
	struct pipe_device_reset_callback device_reset_callback;

	void				*query_result_shader;

	/* Copy one resource to another using async DMA. */
	void (*dma_copy)(struct pipe_context *ctx,
			 struct pipe_resource *dst,
			 unsigned dst_level,
			 unsigned dst_x, unsigned dst_y, unsigned dst_z,
			 struct pipe_resource *src,
			 unsigned src_level,
			 const struct pipe_box *src_box);

	void (*dma_clear_buffer)(struct pipe_context *ctx, struct pipe_resource *dst,
				 uint64_t offset, uint64_t size, unsigned value);

	void (*clear_buffer)(struct pipe_context *ctx, struct pipe_resource *dst,
			     uint64_t offset, uint64_t size, unsigned value,
			     enum r600_coherency coher);

	void (*blit_decompress_depth)(struct pipe_context *ctx,
				      struct r600_texture *texture,
				      struct r600_texture *staging,
				      unsigned first_level, unsigned last_level,
				      unsigned first_layer, unsigned last_layer,
				      unsigned first_sample, unsigned last_sample);

	void (*decompress_dcc)(struct pipe_context *ctx,
			       struct r600_texture *rtex);

	/* Reallocate the buffer and update all resource bindings where
	 * the buffer is bound, including all resource descriptors. */
	void (*invalidate_buffer)(struct pipe_context *ctx, struct pipe_resource *buf);

	/* Enable or disable occlusion queries. */
	void (*set_occlusion_query_state)(struct pipe_context *ctx, bool enable);

	void (*save_qbo_state)(struct pipe_context *ctx, struct r600_qbo_state *st);

	/* This ensures there is enough space in the command stream. */
	void (*need_gfx_cs_space)(struct pipe_context *ctx, unsigned num_dw,
				  bool include_draw_vbo);

	void (*set_atom_dirty)(struct r600_common_context *ctx,
			       struct r600_atom *atom, bool dirty);

	void (*check_vm_faults)(struct r600_common_context *ctx,
				struct radeon_saved_cs *saved,
				enum ring_type ring);
};

/* r600_buffer.c */
bool r600_rings_is_buffer_referenced(struct r600_common_context *ctx,
				     struct pb_buffer *buf,
				     enum radeon_bo_usage usage);
void *r600_buffer_map_sync_with_rings(struct r600_common_context *ctx,
                                      struct r600_resource *resource,
                                      unsigned usage);
void r600_buffer_subdata(struct pipe_context *ctx,
			 struct pipe_resource *buffer,
			 unsigned usage, unsigned offset,
			 unsigned size, const void *data);
void r600_init_resource_fields(struct r600_common_screen *rscreen,
			       struct r600_resource *res,
			       uint64_t size, unsigned alignment);
bool r600_alloc_resource(struct r600_common_screen *rscreen,
			 struct r600_resource *res);
struct pipe_resource *r600_buffer_create(struct pipe_screen *screen,
					 const struct pipe_resource *templ,
					 unsigned alignment);
struct pipe_resource * r600_aligned_buffer_create(struct pipe_screen *screen,
						  unsigned bind,
						  unsigned usage,
						  unsigned size,
						  unsigned alignment);
struct pipe_resource *
r600_buffer_from_user_memory(struct pipe_screen *screen,
			     const struct pipe_resource *templ,
			     void *user_memory);
void
r600_invalidate_resource(struct pipe_context *ctx,
			 struct pipe_resource *resource);

/* r600_common_pipe.c */
void r600_gfx_write_event_eop(struct r600_common_context *ctx,
			      unsigned event, unsigned event_flags,
			      unsigned data_sel,
			      struct r600_resource *buf, uint64_t va,
			      uint32_t old_fence, uint32_t new_fence);
unsigned r600_gfx_write_fence_dwords(struct r600_common_screen *screen);
void r600_gfx_wait_fence(struct r600_common_context *ctx,
			 uint64_t va, uint32_t ref, uint32_t mask);
void r600_draw_rectangle(struct blitter_context *blitter,
			 int x1, int y1, int x2, int y2, float depth,
			 enum blitter_attrib_type type,
			 const union pipe_color_union *attrib);
bool r600_common_screen_init(struct r600_common_screen *rscreen,
			     struct radeon_winsys *ws);
void r600_destroy_common_screen(struct r600_common_screen *rscreen);
void r600_preflush_suspend_features(struct r600_common_context *ctx);
void r600_postflush_resume_features(struct r600_common_context *ctx);
bool r600_common_context_init(struct r600_common_context *rctx,
			      struct r600_common_screen *rscreen,
			      unsigned context_flags);
void r600_common_context_cleanup(struct r600_common_context *rctx);
bool r600_can_dump_shader(struct r600_common_screen *rscreen,
			  unsigned processor);
bool r600_extra_shader_checks(struct r600_common_screen *rscreen,
			      unsigned processor);
void r600_screen_clear_buffer(struct r600_common_screen *rscreen, struct pipe_resource *dst,
			      uint64_t offset, uint64_t size, unsigned value);
struct pipe_resource *r600_resource_create_common(struct pipe_screen *screen,
						  const struct pipe_resource *templ);
const char *r600_get_llvm_processor_name(enum radeon_family family);
void r600_need_dma_space(struct r600_common_context *ctx, unsigned num_dw,
			 struct r600_resource *dst, struct r600_resource *src);
void radeon_save_cs(struct radeon_winsys *ws, struct radeon_winsys_cs *cs,
		    struct radeon_saved_cs *saved);
void radeon_clear_saved_cs(struct radeon_saved_cs *saved);
bool r600_check_device_reset(struct r600_common_context *rctx);

/* r600_gpu_load.c */
void r600_gpu_load_kill_thread(struct r600_common_screen *rscreen);
uint64_t r600_begin_counter_spi(struct r600_common_screen *rscreen);
unsigned r600_end_counter_spi(struct r600_common_screen *rscreen, uint64_t begin);
uint64_t r600_begin_counter_gui(struct r600_common_screen *rscreen);
unsigned r600_end_counter_gui(struct r600_common_screen *rscreen, uint64_t begin);

/* r600_perfcounters.c */
void r600_perfcounters_destroy(struct r600_common_screen *rscreen);

/* r600_query.c */
void r600_init_screen_query_functions(struct r600_common_screen *rscreen);
void r600_query_init(struct r600_common_context *rctx);
void r600_suspend_queries(struct r600_common_context *ctx);
void r600_resume_queries(struct r600_common_context *ctx);
void r600_query_init_backend_mask(struct r600_common_context *ctx);

/* r600_streamout.c */
void r600_streamout_buffers_dirty(struct r600_common_context *rctx);
void r600_set_streamout_targets(struct pipe_context *ctx,
				unsigned num_targets,
				struct pipe_stream_output_target **targets,
				const unsigned *offset);
void r600_emit_streamout_end(struct r600_common_context *rctx);
void r600_update_prims_generated_query_state(struct r600_common_context *rctx,
					     unsigned type, int diff);
void r600_streamout_init(struct r600_common_context *rctx);

/* r600_test_dma.c */
void r600_test_dma(struct r600_common_screen *rscreen);

/* r600_texture.c */
bool r600_prepare_for_dma_blit(struct r600_common_context *rctx,
				struct r600_texture *rdst,
				unsigned dst_level, unsigned dstx,
				unsigned dsty, unsigned dstz,
				struct r600_texture *rsrc,
				unsigned src_level,
				const struct pipe_box *src_box);
void r600_texture_get_fmask_info(struct r600_common_screen *rscreen,
				 struct r600_texture *rtex,
				 unsigned nr_samples,
				 struct r600_fmask_info *out);
void r600_texture_get_cmask_info(struct r600_common_screen *rscreen,
				 struct r600_texture *rtex,
				 struct r600_cmask_info *out);
bool r600_init_flushed_depth_texture(struct pipe_context *ctx,
				     struct pipe_resource *texture,
				     struct r600_texture **staging);
void r600_print_texture_info(struct r600_texture *rtex, FILE *f);
struct pipe_resource *r600_texture_create(struct pipe_screen *screen,
					const struct pipe_resource *templ);
bool vi_dcc_formats_compatible(enum pipe_format format1,
			       enum pipe_format format2);
void vi_dcc_disable_if_incompatible_format(struct r600_common_context *rctx,
					   struct pipe_resource *tex,
					   unsigned level,
					   enum pipe_format view_format);
struct pipe_surface *r600_create_surface_custom(struct pipe_context *pipe,
						struct pipe_resource *texture,
						const struct pipe_surface *templ,
						unsigned width, unsigned height);
unsigned r600_translate_colorswap(enum pipe_format format, bool do_endian_swap);
void vi_separate_dcc_start_query(struct pipe_context *ctx,
				 struct r600_texture *tex);
void vi_separate_dcc_stop_query(struct pipe_context *ctx,
				struct r600_texture *tex);
void vi_separate_dcc_process_and_reset_stats(struct pipe_context *ctx,
					     struct r600_texture *tex);
void vi_dcc_clear_level(struct r600_common_context *rctx,
			struct r600_texture *rtex,
			unsigned level, unsigned clear_value);
void evergreen_do_fast_color_clear(struct r600_common_context *rctx,
				   struct pipe_framebuffer_state *fb,
				   struct r600_atom *fb_state,
				   unsigned *buffers, unsigned *dirty_cbufs,
				   const union pipe_color_union *color);
bool r600_texture_disable_dcc(struct r600_common_context *rctx,
			      struct r600_texture *rtex);
void r600_init_screen_texture_functions(struct r600_common_screen *rscreen);
void r600_init_context_texture_functions(struct r600_common_context *rctx);

/* r600_viewport.c */
void evergreen_apply_scissor_bug_workaround(struct r600_common_context *rctx,
					    struct pipe_scissor_state *scissor);
void r600_viewport_set_rast_deps(struct r600_common_context *rctx,
				 bool scissor_enable, bool clip_halfz);
void r600_update_vs_writes_viewport_index(struct r600_common_context *rctx,
					  struct tgsi_shader_info *info);
void r600_init_viewport_functions(struct r600_common_context *rctx);

/* cayman_msaa.c */
extern const uint32_t eg_sample_locs_2x[4];
extern const unsigned eg_max_dist_2x;
extern const uint32_t eg_sample_locs_4x[4];
extern const unsigned eg_max_dist_4x;
void cayman_get_sample_position(struct pipe_context *ctx, unsigned sample_count,
				unsigned sample_index, float *out_value);
void cayman_init_msaa(struct pipe_context *ctx);
void cayman_emit_msaa_sample_locs(struct radeon_winsys_cs *cs, int nr_samples);
void cayman_emit_msaa_config(struct radeon_winsys_cs *cs, int nr_samples,
			     int ps_iter_samples, int overrast_samples,
			     unsigned sc_mode_cntl_1);


/* Inline helpers. */

static inline struct r600_resource *r600_resource(struct pipe_resource *r)
{
	return (struct r600_resource*)r;
}

static inline void
r600_resource_reference(struct r600_resource **ptr, struct r600_resource *res)
{
	pipe_resource_reference((struct pipe_resource **)ptr,
				(struct pipe_resource *)res);
}

static inline void
r600_texture_reference(struct r600_texture **ptr, struct r600_texture *res)
{
	pipe_resource_reference((struct pipe_resource **)ptr, &res->resource.b.b);
}

static inline void
r600_context_add_resource_size(struct pipe_context *ctx, struct pipe_resource *r)
{
	struct r600_common_context *rctx = (struct r600_common_context *)ctx;
	struct r600_resource *res = (struct r600_resource *)r;

	if (res) {
		/* Add memory usage for need_gfx_cs_space */
		rctx->vram += res->vram_usage;
		rctx->gtt += res->gart_usage;
	}
}

static inline bool r600_get_strmout_en(struct r600_common_context *rctx)
{
	return rctx->streamout.streamout_enabled ||
	       rctx->streamout.prims_gen_query_enabled;
}

#define     SQ_TEX_XY_FILTER_POINT                         0x00
#define     SQ_TEX_XY_FILTER_BILINEAR                      0x01
#define     SQ_TEX_XY_FILTER_ANISO_POINT                   0x02
#define     SQ_TEX_XY_FILTER_ANISO_BILINEAR                0x03

static inline unsigned eg_tex_filter(unsigned filter, unsigned max_aniso)
{
	if (filter == PIPE_TEX_FILTER_LINEAR)
		return max_aniso > 1 ? SQ_TEX_XY_FILTER_ANISO_BILINEAR
				     : SQ_TEX_XY_FILTER_BILINEAR;
	else
		return max_aniso > 1 ? SQ_TEX_XY_FILTER_ANISO_POINT
				     : SQ_TEX_XY_FILTER_POINT;
}

static inline unsigned r600_tex_aniso_filter(unsigned filter)
{
	if (filter < 2)
		return 0;
	if (filter < 4)
		return 1;
	if (filter < 8)
		return 2;
	if (filter < 16)
		return 3;
	return 4;
}

static inline unsigned r600_wavefront_size(enum radeon_family family)
{
	switch (family) {
	case CHIP_RV610:
	case CHIP_RS780:
	case CHIP_RV620:
	case CHIP_RS880:
		return 16;
	case CHIP_RV630:
	case CHIP_RV635:
	case CHIP_RV730:
	case CHIP_RV710:
	case CHIP_PALM:
	case CHIP_CEDAR:
		return 32;
	default:
		return 64;
	}
}

static inline enum radeon_bo_priority
r600_get_sampler_view_priority(struct r600_resource *res)
{
	if (res->b.b.target == PIPE_BUFFER)
		return RADEON_PRIO_SAMPLER_BUFFER;

	if (res->b.b.nr_samples > 1)
		return RADEON_PRIO_SAMPLER_TEXTURE_MSAA;

	return RADEON_PRIO_SAMPLER_TEXTURE;
}

static inline bool
r600_can_sample_zs(struct r600_texture *tex, bool stencil_sampler)
{
	return (stencil_sampler && tex->can_sample_s) ||
	       (!stencil_sampler && tex->can_sample_z);
}

#define COMPUTE_DBG(rscreen, fmt, args...) \
	do { \
		if ((rscreen->b.debug_flags & DBG_COMPUTE)) fprintf(stderr, fmt, ##args); \
	} while (0);

#define R600_ERR(fmt, args...) \
	fprintf(stderr, "EE %s:%d %s - " fmt, __FILE__, __LINE__, __func__, ##args)

/* For MSAA sample positions. */
#define FILL_SREG(s0x, s0y, s1x, s1y, s2x, s2y, s3x, s3y)  \
	(((s0x) & 0xf) | (((unsigned)(s0y) & 0xf) << 4) |		   \
	(((unsigned)(s1x) & 0xf) << 8) | (((unsigned)(s1y) & 0xf) << 12) |	   \
	(((unsigned)(s2x) & 0xf) << 16) | (((unsigned)(s2y) & 0xf) << 20) |	   \
	 (((unsigned)(s3x) & 0xf) << 24) | (((unsigned)(s3y) & 0xf) << 28))

#endif
