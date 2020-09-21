/* Copyright 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "dsp_util.h"

#ifndef max
#define max(a, b) ({ __typeof__(a) _a = (a);	\
			__typeof__(b) _b = (b);	\
			_a > _b ? _a : _b; })
#endif

#ifndef min
#define min(a, b) ({ __typeof__(a) _a = (a);	\
			__typeof__(b) _b = (b);	\
			_a < _b ? _a : _b; })
#endif

#undef deinterleave_stereo
#undef interleave_stereo

/* Converts shorts in range of -32768 to 32767 to floats in range of
 * -1.0f to 1.0f.
 * scvtf instruction accepts fixed point ints, so sxtl is used to lengthen
 * shorts to int with sign extension.
 */
#ifdef __aarch64__
static void deinterleave_stereo(int16_t *input, float *output1,
				float *output2, int frames)
{
	int chunk = frames >> 3;
	frames &= 7;
	/* Process 8 frames (16 samples) each loop. */
	/* L0 R0 L1 R1 L2 R2 L3 R3... -> L0 L1 L2 L3... R0 R1 R2 R3... */
	if (chunk) {
		__asm__ __volatile__ (
			"1:                                         \n"
			"ld2  {v2.8h, v3.8h}, [%[input]], #32       \n"
			"subs %w[chunk], %w[chunk], #1              \n"
			"sxtl   v0.4s, v2.4h                        \n"
			"sxtl2  v1.4s, v2.8h                        \n"
			"sxtl   v2.4s, v3.4h                        \n"
			"sxtl2  v3.4s, v3.8h                        \n"
			"scvtf  v0.4s, v0.4s, #15                   \n"
			"scvtf  v1.4s, v1.4s, #15                   \n"
			"scvtf  v2.4s, v2.4s, #15                   \n"
			"scvtf  v3.4s, v3.4s, #15                   \n"
			"st1    {v0.4s, v1.4s}, [%[output1]], #32   \n"
			"st1    {v2.4s, v3.4s}, [%[output2]], #32   \n"
			"b.ne   1b                                  \n"
			: /* output */
			  [chunk]"+r"(chunk),
			  [input]"+r"(input),
			  [output1]"+r"(output1),
			  [output2]"+r"(output2)
			: /* input */
			: /* clobber */
			  "v0", "v1", "v2", "v3", "memory", "cc"
			);
	}

	/* The remaining samples. */
	while (frames--) {
		*output1++ = *input++ / 32768.0f;
		*output2++ = *input++ / 32768.0f;
	}
}
#define deinterleave_stereo deinterleave_stereo

/* Converts floats in range of -1.0f to 1.0f to shorts in range of
 * -32768 to 32767 with rounding to nearest, with ties (0.5) rounding away
 * from zero.
 * Rounding is achieved by using fcvtas instruction. (a = away)
 * The float scaled to a range of -32768 to 32767 by adding 15 to the exponent.
 * Add to exponent is equivalent to multiply for exponent range of 0 to 239,
 * which is 2.59 * 10^33.  A signed saturating add (sqadd) limits exponents
 * from 240 to 255 to clamp to 255.
 * For very large values, beyond +/- 2 billion, fcvtas will clamp the result
 * to the min or max value that fits an int.
 * For other values, sqxtn clamps the output to -32768 to 32767 range.
 */
static void interleave_stereo(float *input1, float *input2,
			      int16_t *output, int frames)
{
	/* Process 4 frames (8 samples) each loop. */
	/* L0 L1 L2 L3, R0 R1 R2 R3 -> L0 R0 L1 R1, L2 R2 L3 R3 */
	int chunk = frames >> 2;
	frames &= 3;

	if (chunk) {
		__asm__ __volatile__ (
			"dup    v2.4s, %w[scale]                    \n"
			"1:                                         \n"
			"ld1    {v0.4s}, [%[input1]], #16           \n"
			"ld1    {v1.4s}, [%[input2]], #16           \n"
			"subs   %w[chunk], %w[chunk], #1            \n"
			"sqadd  v0.4s, v0.4s, v2.4s                 \n"
			"sqadd  v1.4s, v1.4s, v2.4s                 \n"
			"fcvtas v0.4s, v0.4s                        \n"
			"fcvtas v1.4s, v1.4s                        \n"
			"sqxtn  v0.4h, v0.4s                        \n"
			"sqxtn  v1.4h, v1.4s                        \n"
			"st2    {v0.4h, v1.4h}, [%[output]], #16    \n"
			"b.ne   1b                                  \n"
			: /* output */
			  [chunk]"+r"(chunk),
			  [input1]"+r"(input1),
			  [input2]"+r"(input2),
			  [output]"+r"(output)
			: /* input */
			  [scale]"r"(15 << 23)
			: /* clobber */
			  "v0", "v1", "v2", "memory", "cc"
			);
	}

	/* The remaining samples */
	while (frames--) {
		float f;
		f = *input1++ * 32768.0f;
		f += (f >= 0) ? 0.5f : -0.5f;
		*output++ = max(-32768, min(32767, (int)(f)));
		f = *input2++ * 32768.0f;
		f += (f >= 0) ? 0.5f : -0.5f;
		*output++ = max(-32768, min(32767, (int)(f)));
	}
}
#define interleave_stereo interleave_stereo
#endif

#ifdef __ARM_NEON__
#include <arm_neon.h>

static void deinterleave_stereo(int16_t *input, float *output1,
				float *output2, int frames)
{
	/* Process 8 frames (16 samples) each loop. */
	/* L0 R0 L1 R1 L2 R2 L3 R3... -> L0 L1 L2 L3... R0 R1 R2 R3... */
	int chunk = frames >> 3;
	frames &= 7;
	if (chunk) {
		__asm__ __volatile__ (
			"1:					    \n"
			"vld2.16 {d0-d3}, [%[input]]!		    \n"
			"subs %[chunk], #1			    \n"
			"vmovl.s16 q3, d3			    \n"
			"vmovl.s16 q2, d2			    \n"
			"vmovl.s16 q1, d1			    \n"
			"vmovl.s16 q0, d0			    \n"
			"vcvt.f32.s32 q3, q3, #15		    \n"
			"vcvt.f32.s32 q2, q2, #15		    \n"
			"vcvt.f32.s32 q1, q1, #15		    \n"
			"vcvt.f32.s32 q0, q0, #15		    \n"
			"vst1.32 {d4-d7}, [%[output2]]!		    \n"
			"vst1.32 {d0-d3}, [%[output1]]!		    \n"
			"bne 1b					    \n"
			: /* output */
			  [chunk]"+r"(chunk),
			  [input]"+r"(input),
			  [output1]"+r"(output1),
			  [output2]"+r"(output2)
			: /* input */
			: /* clobber */
			  "q0", "q1", "q2", "q3", "memory", "cc"
			);
	}

	/* The remaining samples. */
	while (frames--) {
		*output1++ = *input++ / 32768.0f;
		*output2++ = *input++ / 32768.0f;
	}
}
#define deinterleave_stereo deinterleave_stereo

/* Converts floats in range of -1.0f to 1.0f to shorts in range of
 * -32768 to 32767 with rounding to nearest, with ties (0.5) rounding away
 * from zero.
 * Rounding is achieved by adding 0.5 or -0.5 adjusted for fixed point
 * precision, and then converting float to fixed point using vcvt instruction
 * which truncated toward zero.
 * For very large values, beyond +/- 2 billion, vcvt will clamp the result
 * to the min or max value that fits an int.
 * For other values, vqmovn clamps the output to -32768 to 32767 range.
 */
static void interleave_stereo(float *input1, float *input2,
			      int16_t *output, int frames)
{
	/* Process 4 frames (8 samples) each loop. */
	/* L0 L1 L2 L3, R0 R1 R2 R3 -> L0 R0 L1 R1, L2 R2 L3 R3 */
	float32x4_t pos = vdupq_n_f32(0.5f / 32768.0f);
	float32x4_t neg = vdupq_n_f32(-0.5f / 32768.0f);
	int chunk = frames >> 2;
	frames &= 3;

	if (chunk) {
		__asm__ __volatile__ (
			"veor q0, q0, q0			    \n"
			"1:					    \n"
			"vld1.32 {d2-d3}, [%[input1]]!		    \n"
			"vld1.32 {d4-d5}, [%[input2]]!		    \n"
			"subs %[chunk], #1			    \n"
			/* We try to round to the nearest number by adding 0.5
			 * to positive input, and adding -0.5 to the negative
			 * input, then truncate.
			 */
			"vcgt.f32 q3, q1, q0			    \n"
			"vcgt.f32 q4, q2, q0			    \n"
			"vbsl q3, %q[pos], %q[neg]		    \n"
			"vbsl q4, %q[pos], %q[neg]		    \n"
			"vadd.f32 q1, q1, q3			    \n"
			"vadd.f32 q2, q2, q4			    \n"
			"vcvt.s32.f32 q1, q1, #15		    \n"
			"vcvt.s32.f32 q2, q2, #15		    \n"
			"vqmovn.s32 d2, q1			    \n"
			"vqmovn.s32 d3, q2			    \n"
			"vst2.16 {d2-d3}, [%[output]]!		    \n"
			"bne 1b					    \n"
			: /* output */
			  [chunk]"+r"(chunk),
			  [input1]"+r"(input1),
			  [input2]"+r"(input2),
			  [output]"+r"(output)
			: /* input */
			  [pos]"w"(pos),
			  [neg]"w"(neg)
			: /* clobber */
			  "q0", "q1", "q2", "q3", "q4", "memory", "cc"
			);
	}

	/* The remaining samples */
	while (frames--) {
		float f;
		f = *input1++ * 32768.0f;
		f += (f >= 0) ? 0.5f : -0.5f;
		*output++ = max(-32768, min(32767, (int)(f)));
		f = *input2++ * 32768.0f;
		f += (f >= 0) ? 0.5f : -0.5f;
		*output++ = max(-32768, min(32767, (int)(f)));
	}
}
#define interleave_stereo interleave_stereo
#endif

#ifdef __SSE3__
#include <emmintrin.h>

/* Converts shorts in range of -32768 to 32767 to floats in range of
 * -1.0f to 1.0f.
 * pslld and psrad shifts are used to isolate the low and high word, but
 * each in a different range:
 * The low word is shifted to the high bits in range 0x80000000 .. 0x7fff0000.
 * The high word is shifted to the low bits in range 0x00008000 .. 0x00007fff.
 * cvtdq2ps converts ints to floats as is.
 * mulps is used to normalize the range of the low and high words, adjusting
 * for high and low words being in different range.
 */
static void deinterleave_stereo(int16_t *input, float *output1,
				float *output2, int frames)
{
	/* Process 8 frames (16 samples) each loop. */
	/* L0 R0 L1 R1 L2 R2 L3 R3... -> L0 L1 L2 L3... R0 R1 R2 R3... */
	int chunk = frames >> 3;
	frames &= 7;
	if (chunk) {
		__asm__ __volatile__ (
			"1:                                         \n"
			"lddqu (%[input]), %%xmm0                   \n"
			"lddqu 16(%[input]), %%xmm1                 \n"
			"add $32, %[input]                          \n"
			"movdqa %%xmm0, %%xmm2                      \n"
			"movdqa %%xmm1, %%xmm3                      \n"
			"pslld $16, %%xmm0                          \n"
			"pslld $16, %%xmm1                          \n"
			"psrad $16, %%xmm2                          \n"
			"psrad $16, %%xmm3                          \n"
			"cvtdq2ps %%xmm0, %%xmm0                    \n"
			"cvtdq2ps %%xmm1, %%xmm1                    \n"
			"cvtdq2ps %%xmm2, %%xmm2                    \n"
			"cvtdq2ps %%xmm3, %%xmm3                    \n"
			"mulps %[scale_2_n31], %%xmm0               \n"
			"mulps %[scale_2_n31], %%xmm1               \n"
			"mulps %[scale_2_n15], %%xmm2               \n"
			"mulps %[scale_2_n15], %%xmm3               \n"
			"movdqu %%xmm0, (%[output1])                \n"
			"movdqu %%xmm1, 16(%[output1])              \n"
			"movdqu %%xmm2, (%[output2])                \n"
			"movdqu %%xmm3, 16(%[output2])              \n"
			"add $32, %[output1]                        \n"
			"add $32, %[output2]                        \n"
			"sub $1, %[chunk]                           \n"
			"jnz 1b                                     \n"
			: /* output */
			  [chunk]"+r"(chunk),
			  [input]"+r"(input),
			  [output1]"+r"(output1),
			  [output2]"+r"(output2)
			: /* input */
			  [scale_2_n31]"x"(_mm_set1_ps(1.0f/(1<<15)/(1<<16))),
			  [scale_2_n15]"x"(_mm_set1_ps(1.0f/(1<<15)))
			: /* clobber */
			  "xmm0", "xmm1", "xmm2", "xmm3", "memory", "cc"
			);
	}

	/* The remaining samples. */
	while (frames--) {
		*output1++ = *input++ / 32768.0f;
		*output2++ = *input++ / 32768.0f;
	}
}
#define deinterleave_stereo deinterleave_stereo

/* Converts floats in range of -1.0f to 1.0f to shorts in range of
 * -32768 to 32767 with rounding to nearest, with ties (0.5) rounding to
 * even.
 * For very large values, beyond +/- 2 billion, cvtps2dq will produce
 * 0x80000000 and packssdw will clamp -32768.
 */
static void interleave_stereo(float *input1, float *input2,
			      int16_t *output, int frames)
{
	/* Process 4 frames (8 samples) each loop. */
	/* L0 L1 L2 L3, R0 R1 R2 R3 -> L0 R0 L1 R1, L2 R2 L3 R3 */
	int chunk = frames >> 2;
	frames &= 3;

	if (chunk) {
		__asm__ __volatile__ (
			"1:                                         \n"
			"lddqu (%[input1]), %%xmm0                  \n"
			"lddqu (%[input2]), %%xmm2                  \n"
			"add $16, %[input1]                         \n"
			"add $16, %[input2]                         \n"
			"movaps %%xmm0, %%xmm1                      \n"
			"unpcklps %%xmm2, %%xmm0                    \n"
			"unpckhps %%xmm2, %%xmm1                    \n"
			"paddsw %[scale_2_15], %%xmm0               \n"
			"paddsw %[scale_2_15], %%xmm1               \n"
			"cvtps2dq %%xmm0, %%xmm0                    \n"
			"cvtps2dq %%xmm1, %%xmm1                    \n"
			"packssdw %%xmm1, %%xmm0                    \n"
			"movdqu %%xmm0, (%[output])                 \n"
			"add $16, %[output]                         \n"
			"sub $1, %[chunk]                           \n"
			"jnz 1b                                     \n"
			: /* output */
			  [chunk]"+r"(chunk),
			  [input1]"+r"(input1),
			  [input2]"+r"(input2),
			  [output]"+r"(output)
			: /* input */
			  [scale_2_15]"x"(_mm_set1_epi32(15 << 23)),
			  [clamp_large]"x"(_mm_set1_ps(32767.0f))
			: /* clobber */
			  "xmm0", "xmm1", "xmm2", "memory", "cc"
			);
	}

	/* The remaining samples */
	while (frames--) {
		float f;
		f = *input1++ * 32768.0f;
		f += (f >= 0) ? 0.5f : -0.5f;
		*output++ = max(-32768, min(32767, (int)(f)));
		f = *input2++ * 32768.0f;
		f += (f >= 0) ? 0.5f : -0.5f;
		*output++ = max(-32768, min(32767, (int)(f)));
	}
}
#define interleave_stereo interleave_stereo
#endif

void dsp_util_deinterleave(int16_t *input, float *const *output, int channels,
			   int frames)
{
	float *output_ptr[channels];
	int i, j;

#ifdef deinterleave_stereo
	if (channels == 2) {
		deinterleave_stereo(input, output[0], output[1], frames);
		return;
	}
#endif

	for (i = 0; i < channels; i++)
		output_ptr[i] = output[i];

	for (i = 0; i < frames; i++)
		for (j = 0; j < channels; j++)
			*(output_ptr[j]++) = *input++ / 32768.0f;
}

void dsp_util_interleave(float *const *input, int16_t *output, int channels,
			 int frames)
{
	float *input_ptr[channels];
	int i, j;

#ifdef interleave_stereo
	if (channels == 2) {
		interleave_stereo(input[0], input[1], output, frames);
		return;
	}
#endif

	for (i = 0; i < channels; i++)
		input_ptr[i] = input[i];

	for (i = 0; i < frames; i++)
		for (j = 0; j < channels; j++) {
			float f = *(input_ptr[j]++) * 32768.0f;
			f += (f >= 0) ? 0.5f : -0.5f;
			*output++ = max(-32768, min(32767, (int)(f)));
		}
}

void dsp_enable_flush_denormal_to_zero()
{
#if defined(__i386__) || defined(__x86_64__)
	unsigned int mxcsr;
	mxcsr = __builtin_ia32_stmxcsr();
	__builtin_ia32_ldmxcsr(mxcsr | 0x8040);
#elif defined(__aarch64__)
	uint64_t cw;
	__asm__ __volatile__ (
		"mrs    %0, fpcr			    \n"
		"orr    %0, %0, #0x1000000		    \n"
		"msr    fpcr, %0			    \n"
		"isb					    \n"
		: "=r"(cw) :: "memory");
#elif defined(__arm__)
	uint32_t cw;
	__asm__ __volatile__ (
		"vmrs   %0, fpscr			    \n"
		"orr    %0, %0, #0x1000000		    \n"
		"vmsr   fpscr, %0			    \n"
		: "=r"(cw) :: "memory");
#else
#warning "Don't know how to disable denorms. Performace may suffer."
#endif
}
