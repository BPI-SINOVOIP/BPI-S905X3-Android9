@/******************************************************************************
@ *
@ * Copyright (C) 2015 The Android Open Source Project
@ *
@ * Licensed under the Apache License, Version 2.0 (the "License");
@ * you may not use this file except in compliance with the License.
@ * You may obtain a copy of the License at:
@ *
@ * http://www.apache.org/licenses/LICENSE-2.0
@ *
@ * Unless required by applicable law or agreed to in writing, software
@ * distributed under the License is distributed on an "AS IS" BASIS,
@ * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
@ * See the License for the specific language governing permissions and
@ * limitations under the License.
@ *
@ *****************************************************************************
@ * Originally developed and contributed by Ittiam Systems Pvt. Ltd, Bangalore
@*/
@**
@ *******************************************************************************
@ * @file
@ *  ih264_mem_fns_neon.s
@ *
@ * @brief
@ *  Contains function definitions for memory manipulation
@ *
@ * @author
@ *  Naveen SR
@ *
@ * @par List of Functions:
@ *  - ih264_memcpy_mul_8_a9q()
@ *  - ih264_memcpy_a9q()
@ *  - ih264_memset_mul_8_a9q()
@ *  - ih264_memset_a9q()
@ *  - ih264_memset_16bit_mul_8_a9q()
@ *  - ih264_memset_a9q()
@ *
@ * @remarks
@ *  None
@ *
@ *******************************************************************************
@*

@**
@*******************************************************************************
@*
@* @brief
@*   memcpy of a 1d array
@*
@* @par Description:
@*   Does memcpy of 8bit data from source to destination for 8,16 or 32 number of bytes
@*
@* @param[in] pu1_dst
@*  UWORD8 pointer to the destination
@*
@* @param[in] pu1_src
@*  UWORD8 pointer to the source
@*
@* @param[in] num_bytes
@*  number of bytes to copy
@* @returns
@*
@* @remarks
@*  None
@*
@*******************************************************************************
@*
@void ih264_memcpy_mul_8(UWORD8 *pu1_dst,
@                    UWORD8 *pu1_src,
@                   UWORD32 num_bytes)
@**************Variables Vs Registers*************************
@   r0 => *pu1_dst
@   r1 => *pu1_src
@   r2 => num_bytes

.text
.p2align 2


    .global ih264_memcpy_mul_8_a9q

ih264_memcpy_mul_8_a9q:

loop_neon_memcpy_mul_8:
    @ Memcpy 8 bytes
    vld1.8        d0, [r1]!
    vst1.8        d0, [r0]!

    subs          r2, r2, #8
    bne           loop_neon_memcpy_mul_8
    bx            lr



@*******************************************************************************
@*
@void ih264_memcpy(UWORD8 *pu1_dst,
@                  UWORD8 *pu1_src,
@                  UWORD32 num_bytes)
@**************Variables Vs Registers*************************
@   r0 => *pu1_dst
@   r1 => *pu1_src
@   r2 => num_bytes



    .global ih264_memcpy_a9q

ih264_memcpy_a9q:
    subs          r2, #8
    blt           memcpy
loop_neon_memcpy:
    @ Memcpy 8 bytes
    vld1.8        d0, [r1]!
    vst1.8        d0, [r0]!

    subs          r2, #8
    bge           loop_neon_memcpy
    cmp           r2, #-8
    bxeq          lr

memcpy:
    add           r2, #8

loop_memcpy:
    ldrb          r3, [r1], #1
    strb          r3, [r0], #1
    subs          r2, #1
    bne           loop_memcpy
    bx            lr




@void ih264_memset_mul_8(UWORD8 *pu1_dst,
@                       UWORD8 value,
@                       UWORD32 num_bytes)
@**************Variables Vs Registers*************************
@   r0 => *pu1_dst
@   r1 => value
@   r2 => num_bytes





    .global ih264_memset_mul_8_a9q

ih264_memset_mul_8_a9q:

@ Assumptions: numbytes is either 8, 16 or 32
    vdup.8        d0, r1
loop_memset_mul_8:
    @ Memset 8 bytes
    vst1.8        d0, [r0]!

    subs          r2, r2, #8
    bne           loop_memset_mul_8

    bx            lr




@void ih264_memset(UWORD8 *pu1_dst,
@                       UWORD8 value,
@                       UWORD8 num_bytes)
@**************Variables Vs Registers*************************
@   r0 => *pu1_dst
@   r1 => value
@   r2 => num_bytes



    .global ih264_memset_a9q

ih264_memset_a9q:
    subs          r2, #8
    blt           memset
    vdup.8        d0, r1
loop_neon_memset:
    @ Memcpy 8 bytes
    vst1.8        d0, [r0]!

    subs          r2, #8
    bge           loop_neon_memset
    cmp           r2, #-8
    bxeq          lr

memset:
    add           r2, #8

loop_memset:
    strb          r1, [r0], #1
    subs          r2, #1
    bne           loop_memset
    bx            lr




@void ih264_memset_16bit_mul_8(UWORD16 *pu2_dst,
@                                   UWORD16 value,
@                                   UWORD32 num_words)
@**************Variables Vs Registers*************************
@   r0 => *pu2_dst
@   r1 => value
@   r2 => num_words





    .global ih264_memset_16bit_mul_8_a9q

ih264_memset_16bit_mul_8_a9q:

@ Assumptions: num_words is either 8, 16 or 32

    @ Memset 8 words
    vdup.16       d0, r1
loop_memset_16bit_mul_8:
    vst1.16       d0, [r0]!
    vst1.16       d0, [r0]!

    subs          r2, r2, #8
    bne           loop_memset_16bit_mul_8

    bx            lr




@void ih264_memset_16bit(UWORD16 *pu2_dst,
@                       UWORD16 value,
@                       UWORD32 num_words)
@**************Variables Vs Registers*************************
@   r0 => *pu2_dst
@   r1 => value
@   r2 => num_words



    .global ih264_memset_16bit_a9q

ih264_memset_16bit_a9q:
    subs          r2, #8
    blt           memset_16bit
    vdup.16       d0, r1
loop_neon_memset_16bit:
    @ Memset 8 words
    vst1.16       d0, [r0]!
    vst1.16       d0, [r0]!

    subs          r2, #8
    bge           loop_neon_memset_16bit
    cmp           r2, #-8
    bxeq          lr

memset_16bit:
    add           r2, #8

loop_memset_16bit:
    strh          r1, [r0], #2
    subs          r2, #1
    bne           loop_memset_16bit
    bx            lr




