/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdint.h>

#include "cras_system_state.h"
#include "cras_mix.h"
#include "cras_mix_ops.h"

static const struct cras_mix_ops *ops = &mixer_ops;

static const struct cras_mix_ops* get_mixer_ops(unsigned int cpu_flags)
{
#if defined HAVE_FMA
	if (cpu_flags & CPU_X86_FMA)
		return &mixer_ops_fma;
#endif
#if defined HAVE_AVX2
	if (cpu_flags & CPU_X86_AVX2)
		return &mixer_ops_avx2;
#endif
#if defined HAVE_AVX
	if (cpu_flags & CPU_X86_AVX)
		return &mixer_ops_avx;
#endif
#if defined HAVE_SSE42
	if (cpu_flags & CPU_X86_SSE4_2)
		return &mixer_ops_sse42;
#endif

	/* default C implementation */
	return &mixer_ops;
}

void cras_mix_init(unsigned int flags)
{
	ops = get_mixer_ops(flags);
}

/*
 * Exported Interface
 */

void cras_scale_buffer_increment(snd_pcm_format_t fmt, uint8_t *buff,
				 unsigned int frame, float scaler,
				 float increment, int channel)
{
	ops->scale_buffer_increment(fmt, buff, frame * channel, scaler,
				    increment, channel);
}

void cras_scale_buffer(snd_pcm_format_t fmt, uint8_t *buff, unsigned int count,
		       float scaler)
{
	ops->scale_buffer(fmt, buff, count, scaler);
}

void cras_mix_add(snd_pcm_format_t fmt, uint8_t *dst, uint8_t *src,
		  unsigned int count, unsigned int index,
		  int mute, float mix_vol)
{
	ops->add(fmt, dst, src, count, index, mute, mix_vol);
}

void cras_mix_add_scale_stride(snd_pcm_format_t fmt, uint8_t *dst, uint8_t *src,
			 unsigned int count, unsigned int dst_stride,
			 unsigned int src_stride, float scaler)
{
	ops->add_scale_stride(fmt, dst, src, count, dst_stride, src_stride,
			      scaler);
}

size_t cras_mix_mute_buffer(uint8_t *dst,
			    size_t frame_bytes,
			    size_t count)
{
	return ops->mute_buffer(dst, frame_bytes, count);
}
