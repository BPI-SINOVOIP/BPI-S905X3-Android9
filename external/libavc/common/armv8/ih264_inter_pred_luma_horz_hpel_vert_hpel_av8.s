//******************************************************************************
//*
//* Copyright (C) 2015 The Android Open Source Project
//*
//* Licensed under the Apache License, Version 2.0 (the "License");
//* you may not use this file except in compliance with the License.
//* You may obtain a copy of the License at:
//*
//* http://www.apache.org/licenses/LICENSE-2.0
//*
//* Unless required by applicable law or agreed to in writing, software
//* distributed under the License is distributed on an "AS IS" BASIS,
//* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//* See the License for the specific language governing permissions and
//* limitations under the License.
//*
//*****************************************************************************
//* Originally developed and contributed by Ittiam Systems Pvt. Ltd, Bangalore
//*/
///**
//******************************************************************************
//* @file
//*  ih264_inter_pred_luma_horz_hpel_vert_hpel_av8.s
//*
//* @brief
//*  Contains function definitions for inter prediction  interpolation.
//*
//* @author
//*  Mohit
//*
//* @par List of Functions:
//*
//*  - ih264_inter_pred_luma_horz_hpel_vert_hpel_av8()
//*
//* @remarks
//*  None
//*
//*******************************************************************************
//*/



//void ih264_inter_pred_luma_horz_hpel_vert_hpel(UWORD8 *pu1_src,
//                                UWORD8 *pu1_dst,
//                                WORD32 src_strd,,
//                                WORD32 dst_strd,
//                                WORD32 ht,
//                                WORD32 wd,
//                                    UWORD8* pu1_tmp,
//                                  UWORD32 dydx)

//**************Variables Vs Registers*****************************************
//    x0 => *pu1_src
//    x1 => *pu1_dst
//    w2 =>  src_strd
//    w3 =>  dst_strd
//    w4 =>  ht
//    w5 =>  wd


.text
.p2align 2
.include "ih264_neon_macros.s"



    .global ih264_inter_pred_luma_horz_hpel_vert_hpel_av8

ih264_inter_pred_luma_horz_hpel_vert_hpel_av8:

    //store register values to stack
    push_v_regs
    stp       x19, x20, [sp, #-16]!
    sxtw      x2, w2
    sxtw      x3, w3
    sxtw      x4, w4
    sxtw      x5, w5

    sub       x0, x0, x2, lsl #1        //pu1_src-2*src_strd
    sub       x0, x0, #2                //pu1_src-2

    movi      v26.8h, #0x14             // Filter coeff 20 into Q13
    movi      v24.8h, #0x5              // Filter coeff 5  into Q12
    movi      v27.8h, #0x14             // Filter coeff 20 into Q13
    movi      v25.8h, #0x5              // Filter coeff 5  into Q12
    mov       x7, #0x20
    mov       x8, #0x30
    subs      x12, x5, #4               //if wd=4 branch to loop_4
    beq       loop_4_start

    subs      x12, x5, #8               //if wd=8 branch to loop_8
    beq       loop_8_start

    //when  wd=16
    movi      v28.8h, #0x14             // Filter coeff 20 into Q13
    movi      v30.8h, #0x5              // Filter coeff 5  into Q12
    sub       x2, x2, #16
    ld1       {v0.2s, v1.2s}, [x0], #16 // Vector load from src[0_0]
    ld1       {v12.2s}, [x0], x2        // Vector load from src[0_0]
    ld1       {v2.2s, v3.2s}, [x0], #16 // Vector load from src[1_0]
    ld1       {v13.2s}, [x0], x2        // Vector load from src[1_0]
    ld1       {v4.2s, v5.2s}, [x0], #16 // Vector load from src[2_0]
    ld1       {v14.2s}, [x0], x2        // Vector load from src[2_0]
    ld1       {v6.2s, v7.2s}, [x0], #16 // Vector load from src[3_0]
    ld1       {v15.2s}, [x0], x2        // Vector load from src[3_0]
    ld1       {v8.2s, v9.2s}, [x0], #16 // Vector load from src[4_0]
    ld1       {v16.2s}, [x0], x2        // Vector load from src[4_0]
loop_16:

    ld1       {v10.2s, v11.2s}, [x0], #16 // Vector load from src[5_0]
    ld1       {v17.2s}, [x0], x2        // Vector load from src[5_0]


    uaddl     v20.8h, v4.8b, v6.8b
    uaddl     v18.8h, v0.8b, v10.8b
    uaddl     v22.8h, v2.8b, v8.8b
    mla       v18.8h, v20.8h , v28.8h
    uaddl     v24.8h, v5.8b, v7.8b
    uaddl     v20.8h, v1.8b, v11.8b
    uaddl     v26.8h, v3.8b, v9.8b
    mla       v20.8h, v24.8h , v28.8h
    uaddl     v24.8h, v14.8b, v15.8b
    mls       v18.8h, v22.8h , v30.8h
    uaddl     v22.8h, v12.8b, v17.8b
    mls       v20.8h, v26.8h , v30.8h
    uaddl     v26.8h, v13.8b, v16.8b
    mla       v22.8h, v24.8h , v28.8h
    mls       v22.8h, v26.8h , v30.8h

    ext       v24.16b, v18.16b , v20.16b , #4
    ext       v26.16b, v18.16b , v20.16b , #6

    ext       v23.16b, v18.16b , v20.16b , #10
    add       v0.8h, v24.8h , v26.8h
    ext       v24.16b, v18.16b , v20.16b , #2
    ext       v26.16b, v18.16b , v20.16b , #8
    add       v24.8h, v24.8h , v26.8h

    saddl     v26.4s, v18.4h, v23.4h
    smlal     v26.4s, v0.4h, v28.4h
    smlsl     v26.4s, v24.4h, v30.4h

    saddl2    v23.4s, v18.8h, v23.8h
    smlal2    v23.4s, v0.8h, v28.8h
    smlsl2    v23.4s, v24.8h, v30.8h

    sqrshrun  v18.4h, v26.4s, #10
    sqrshrun  v19.4h, v23.4s, #10


    uqxtn     v18.8b, v18.8h
    uqxtn     v19.8b, v19.8h
    mov       v18.s[1], v19.s[0]

    ext       v24.16b, v20.16b , v22.16b , #4
    ext       v26.16b, v20.16b , v22.16b , #6
    ext       v0.16b, v20.16b , v22.16b , #10

    add       v25.8h, v24.8h , v26.8h
    ext       v24.16b, v20.16b , v22.16b , #2
    ext       v26.16b, v20.16b , v22.16b , #8
    add       v24.8h, v24.8h , v26.8h

    saddl     v26.4s, v0.4h, v20.4h
    smlal     v26.4s, v25.4h, v28.4h
    smlsl     v26.4s, v24.4h, v30.4h

    saddl2    v22.4s, v0.8h, v20.8h
    smlal2    v22.4s, v25.8h, v28.8h
    smlsl2    v22.4s, v24.8h, v30.8h

    sqrshrun  v19.4h, v26.4s, #10
    sqrshrun  v25.4h, v22.4s, #10

    uaddl     v24.8h, v7.8b, v9.8b



    uqxtn     v19.8b, v19.8h
    uqxtn     v25.8b, v25.8h
    mov       v19.s[1], v25.s[0]

    uaddl     v22.8h, v4.8b, v10.8b
    ld1       {v0.2s, v1.2s}, [x0], #16 // Vector load from src[6_0]


    ld1       {v12.2s}, [x0], x2        // Vector load from src[6_0]
    uaddl     v20.8h, v6.8b, v8.8b
    uaddl     v26.8h, v5.8b, v11.8b
    st1       {v18.2s, v19.2s}, [x1], x3 // store row 0


//ROW_2


    uaddl     v18.8h, v2.8b, v0.8b

    mla       v18.8h, v20.8h , v28.8h

    uaddl     v20.8h, v3.8b, v1.8b

    mla       v20.8h, v24.8h , v28.8h
    uaddl     v24.8h, v15.8b, v16.8b
    mls       v18.8h, v22.8h , v30.8h
    uaddl     v22.8h, v13.8b, v12.8b
    mls       v20.8h, v26.8h , v30.8h
    uaddl     v26.8h, v14.8b, v17.8b
    mla       v22.8h, v24.8h , v28.8h
    mls       v22.8h, v26.8h , v30.8h

    ext       v24.16b, v18.16b , v20.16b , #4
    ext       v26.16b, v18.16b , v20.16b , #6

    ext       v23.16b, v18.16b , v20.16b , #10
    add       v2.8h, v24.8h , v26.8h
    ext       v24.16b, v18.16b , v20.16b , #2
    ext       v26.16b, v18.16b , v20.16b , #8
    add       v24.8h, v24.8h , v26.8h

    saddl     v26.4s, v18.4h, v23.4h
    smlal     v26.4s, v2.4h, v28.4h
    smlsl     v26.4s, v24.4h, v30.4h

    saddl2    v23.4s, v18.8h, v23.8h
    smlal2    v23.4s, v2.8h, v28.8h
    smlsl2    v23.4s, v24.8h, v30.8h

    sqrshrun  v18.4h, v26.4s, #10
    sqrshrun  v19.4h, v23.4s, #10



    uqxtn     v18.8b, v18.8h
    uqxtn     v19.8b, v19.8h
    mov       v18.s[1], v19.s[0]

    ext       v24.16b, v20.16b , v22.16b , #4
    ext       v26.16b, v20.16b , v22.16b , #6
    ext       v2.16b, v20.16b , v22.16b , #10

    add       v25.8h, v24.8h , v26.8h
    ext       v24.16b, v20.16b , v22.16b , #2
    ext       v26.16b, v20.16b , v22.16b , #8
    add       v24.8h, v24.8h , v26.8h

    saddl     v26.4s, v2.4h, v20.4h
    smlal     v26.4s, v25.4h, v28.4h
    smlsl     v26.4s, v24.4h, v30.4h

    saddl2    v22.4s, v2.8h, v20.8h
    smlal2    v22.4s, v25.8h, v28.8h
    smlsl2    v22.4s, v24.8h, v30.8h

    sqrshrun  v19.4h, v26.4s, #10
    sqrshrun  v25.4h, v22.4s, #10
    uaddl     v24.8h, v9.8b, v11.8b

    uqxtn     v19.8b, v19.8h
    uqxtn     v25.8b, v25.8h
    mov       v19.s[1], v25.s[0]


    uaddl     v22.8h, v6.8b, v0.8b
    ld1       {v2.2s, v3.2s}, [x0], #16 // Vector load from src[7_0]


    ld1       {v13.2s}, [x0], x2        // Vector load from src[7_0]
    uaddl     v20.8h, v8.8b, v10.8b
    uaddl     v26.8h, v7.8b, v1.8b
    st1       {v18.2s, v19.2s}, [x1], x3 // store row 1

//ROW_3


    uaddl     v18.8h, v4.8b, v2.8b

    mla       v18.8h, v20.8h , v28.8h

    uaddl     v20.8h, v5.8b, v3.8b

    mla       v20.8h, v24.8h , v28.8h
    uaddl     v24.8h, v16.8b, v17.8b
    mls       v18.8h, v22.8h , v30.8h
    uaddl     v22.8h, v14.8b, v13.8b
    mls       v20.8h, v26.8h , v30.8h
    uaddl     v26.8h, v15.8b, v12.8b
    mla       v22.8h, v24.8h , v28.8h
    mls       v22.8h, v26.8h , v30.8h

    ext       v24.16b, v18.16b , v20.16b , #4
    ext       v26.16b, v18.16b , v20.16b , #6

    ext       v23.16b, v18.16b , v20.16b , #10
    add       v4.8h, v24.8h , v26.8h
    ext       v24.16b, v18.16b , v20.16b , #2
    ext       v26.16b, v18.16b , v20.16b , #8
    add       v24.8h, v24.8h , v26.8h

    saddl     v26.4s, v18.4h, v23.4h
    smlal     v26.4s, v4.4h, v28.4h
    smlsl     v26.4s, v24.4h, v30.4h

    saddl2    v23.4s, v18.8h, v23.8h
    smlal2    v23.4s, v4.8h, v28.8h
    smlsl2    v23.4s, v24.8h, v30.8h

    sqrshrun  v18.4h, v26.4s, #10
    sqrshrun  v19.4h, v23.4s, #10


    uqxtn     v18.8b, v18.8h
    uqxtn     v19.8b, v19.8h
    mov       v18.s[1], v19.s[0]


    ext       v24.16b, v20.16b , v22.16b , #4
    ext       v26.16b, v20.16b , v22.16b , #6
    ext       v4.16b, v20.16b , v22.16b , #10

    add       v25.8h, v24.8h , v26.8h
    ext       v24.16b, v20.16b , v22.16b , #2
    ext       v26.16b, v20.16b , v22.16b , #8
    add       v24.8h, v24.8h , v26.8h

    saddl     v26.4s, v4.4h, v20.4h
    smlal     v26.4s, v25.4h, v28.4h
    smlsl     v26.4s, v24.4h, v30.4h

    saddl2    v22.4s, v4.8h, v20.8h
    smlal2    v22.4s, v25.8h, v28.8h
    smlsl2    v22.4s, v24.8h, v30.8h

    sqrshrun  v19.4h, v26.4s, #10
    sqrshrun  v25.4h, v22.4s, #10

    uaddl     v24.8h, v11.8b, v1.8b


    uqxtn     v19.8b, v19.8h
    uqxtn     v25.8b, v25.8h
    mov       v19.s[1], v25.s[0]



    uaddl     v22.8h, v8.8b, v2.8b
    ld1       {v4.2s, v5.2s}, [x0], #16 // Vector load from src[8_0]


    ld1       {v14.2s}, [x0], x2        // Vector load from src[8_0]
    uaddl     v20.8h, v10.8b, v0.8b
    uaddl     v26.8h, v9.8b, v3.8b
    st1       {v18.2s, v19.2s}, [x1], x3 // store row 2


//ROW_4

    uaddl     v18.8h, v6.8b, v4.8b

    mla       v18.8h, v20.8h , v28.8h

    uaddl     v20.8h, v7.8b, v5.8b

    mla       v20.8h, v24.8h , v28.8h
    uaddl     v24.8h, v17.8b, v12.8b
    mls       v18.8h, v22.8h , v30.8h
    uaddl     v22.8h, v15.8b, v14.8b
    mls       v20.8h, v26.8h , v30.8h
    uaddl     v26.8h, v16.8b, v13.8b
    mla       v22.8h, v24.8h , v28.8h
    mls       v22.8h, v26.8h , v30.8h

    ext       v24.16b, v18.16b , v20.16b , #4
    ext       v26.16b, v18.16b , v20.16b , #6

    ext       v23.16b, v18.16b , v20.16b , #10
    add       v6.8h, v24.8h , v26.8h
    ext       v24.16b, v18.16b , v20.16b , #2
    ext       v26.16b, v18.16b , v20.16b , #8
    add       v24.8h, v24.8h , v26.8h

    saddl     v26.4s, v18.4h, v23.4h
    smlal     v26.4s, v6.4h, v28.4h
    smlsl     v26.4s, v24.4h, v30.4h

    saddl2    v23.4s, v18.8h, v23.8h
    smlal2    v23.4s, v6.8h, v28.8h
    smlsl2    v23.4s, v24.8h, v30.8h

    sqrshrun  v18.4h, v26.4s, #10
    sqrshrun  v19.4h, v23.4s, #10

    uqxtn     v18.8b, v18.8h
    uqxtn     v19.8b, v19.8h
    mov       v18.s[1], v19.s[0]


    ext       v24.16b, v20.16b , v22.16b , #4
    ext       v26.16b, v20.16b , v22.16b , #6
    ext       v6.16b, v20.16b , v22.16b , #10

    add       v25.8h, v24.8h , v26.8h
    ext       v24.16b, v20.16b , v22.16b , #2
    ext       v26.16b, v20.16b , v22.16b , #8
    add       v24.8h, v24.8h , v26.8h

    saddl     v26.4s, v6.4h, v20.4h
    smlal     v26.4s, v25.4h, v28.4h
    smlsl     v26.4s, v24.4h, v30.4h

    saddl2    v22.4s, v6.8h, v20.8h
    smlal2    v22.4s, v25.8h, v28.8h
    smlsl2    v22.4s, v24.8h, v30.8h

    mov       v6.16b, v2.16b
    mov       v7.16b, v3.16b

    mov       v2.16b, v10.16b
    mov       v3.16b, v11.16b

    subs      x4, x4, #4
    sqrshrun  v19.4h, v26.4s, #10
    sqrshrun  v25.4h, v22.4s, #10
    mov       v10.16b, v0.16b
    mov       v11.16b, v1.16b

    mov       v24.8b, v14.8b

    mov       v14.16b, v12.16b
    mov       v15.16b, v13.16b


    uqxtn     v19.8b, v19.8h
    uqxtn     v25.8b, v25.8h
    mov       v19.s[1], v25.s[0]



    mov       v0.16b, v8.16b
    mov       v1.16b, v9.16b

    mov       v8.16b, v4.16b
    mov       v9.16b, v5.16b

    mov       v12.16b, v16.16b
    mov       v13.16b, v17.16b

    mov       v4.16b, v10.16b
    mov       v5.16b, v11.16b

    mov       v16.8b, v24.8b
    st1       {v18.2s, v19.2s}, [x1], x3 // store row 3

    bgt       loop_16                   // looping if height =16
    b         end_func

loop_8_start:
    ld1       {v0.2s, v1.2s}, [x0], x2  // Vector load from src[0_0]
    ld1       {v2.2s, v3.2s}, [x0], x2  // Vector load from src[1_0]
    ld1       {v4.2s, v5.2s}, [x0], x2  // Vector load from src[2_0]
    ld1       {v6.2s, v7.2s}, [x0], x2  // Vector load from src[3_0]
    ld1       {v8.2s, v9.2s}, [x0], x2  // Vector load from src[4_0]

loop_8:

    ld1       {v10.2s, v11.2s}, [x0], x2 // Vector load from src[5_0]
    uaddl     v14.8h, v4.8b, v6.8b
    uaddl     v12.8h, v0.8b, v10.8b
    uaddl     v16.8h, v2.8b, v8.8b
    mla       v12.8h, v14.8h , v26.8h
    uaddl     v18.8h, v5.8b, v7.8b
    uaddl     v14.8h, v1.8b, v11.8b
    uaddl     v22.8h, v3.8b, v9.8b
    mla       v14.8h, v18.8h , v26.8h
    mls       v12.8h, v16.8h , v24.8h
    ld1       {v0.2s, v1.2s}, [x0], x2  // Vector load from src[6_0]
    uaddl     v16.8h, v6.8b, v8.8b
    mls       v14.8h, v22.8h , v24.8h
    uaddl     v28.8h, v2.8b, v0.8b

    ext       v22.16b, v12.16b , v14.16b , #10
    uaddl     v18.8h, v4.8b, v10.8b
    mla       v28.8h, v16.8h , v26.8h
    saddl     v30.4s, v12.4h, v22.4h

    saddl2    v22.4s, v12.8h, v22.8h
    ext       v16.16b, v12.16b , v14.16b , #4
    mls       v28.8h, v18.8h , v24.8h
    ext       v18.16b, v12.16b , v14.16b , #6
    ext       v20.16b, v12.16b , v14.16b , #8
    ext       v14.16b, v12.16b , v14.16b , #2
    add       v16.8h, v16.8h , v18.8h
    add       v18.8h, v14.8h , v20.8h
    uaddl     v20.8h, v7.8b, v9.8b
    smlal     v30.4s, v16.4h, v26.4h
    smlsl     v30.4s, v18.4h, v24.4h
    smlal2    v22.4s, v16.8h, v26.8h
    smlsl2    v22.4s, v18.8h, v24.8h
    uaddl     v14.8h, v3.8b, v1.8b

    mla       v14.8h, v20.8h , v26.8h
    sqrshrun  v12.4h, v30.4s, #10
    uaddl     v16.8h, v5.8b, v11.8b
    sqrshrun  v13.4h, v22.4s, #10
    mls       v14.8h, v16.8h , v24.8h
    ld1       {v2.2s, v3.2s}, [x0], x2  // Vector load from src[7_0]
    uqxtn     v25.8b, v12.8h
    uqxtn     v13.8b, v13.8h
    mov       v25.s[1], v13.s[0]
    uaddl     v16.8h, v8.8b, v10.8b


    ext       v22.16b, v28.16b , v14.16b , #10
    uaddl     v20.8h, v4.8b, v2.8b
    saddl     v30.4s, v28.4h, v22.4h
    mla       v20.8h, v16.8h , v26.8h

    saddl2    v22.4s, v28.8h, v22.8h
    ext       v16.16b, v28.16b , v14.16b , #4
    ext       v18.16b, v28.16b , v14.16b , #6
    ext       v12.16b, v28.16b , v14.16b , #8
    ext       v14.16b, v28.16b , v14.16b , #2
    add       v16.8h, v16.8h , v18.8h
    add       v18.8h, v12.8h , v14.8h

    smlal     v30.4s, v16.4h, v26.4h
    smlsl     v30.4s, v18.4h, v24.4h
    smlal2    v22.4s, v16.8h, v26.8h
    smlsl2    v22.4s, v18.8h, v24.8h


    uaddl     v18.8h, v6.8b, v0.8b
    sqrshrun  v16.4h, v30.4s, #10

    sqrshrun  v17.4h, v22.4s, #10

    mov       v12.8b, v25.8b
    mov       v25.8b, v24.8b

    uaddl     v28.8h, v9.8b, v11.8b
    uqxtn     v13.8b, v16.8h
    uqxtn     v17.8b, v17.8h
    mov       v13.s[1], v17.s[0]


    uaddl     v14.8h, v5.8b, v3.8b
    uaddl     v22.8h, v7.8b, v1.8b
    mls       v20.8h, v18.8h , v24.8h
    st1       {v12.2s}, [x1], x3        // store row 0
    mla       v14.8h, v28.8h , v26.8h
    ld1       {v4.2s, v5.2s}, [x0], x2  // Vector load from src[8_0]
    uaddl     v30.8h, v10.8b, v0.8b
    uaddl     v28.8h, v6.8b, v4.8b
    mls       v14.8h, v22.8h , v24.8h
    st1       {v13.2s}, [x1], x3        // store row 1
    mla       v28.8h, v30.8h , v26.8h

    ext       v22.16b, v20.16b , v14.16b , #10
    saddl     v30.4s, v20.4h, v22.4h

    saddl2    v22.4s, v20.8h, v22.8h
    ext       v16.16b, v20.16b , v14.16b , #4
    ext       v18.16b, v20.16b , v14.16b , #6
    ext       v12.16b, v20.16b , v14.16b , #8
    ext       v14.16b, v20.16b , v14.16b , #2
    add       v16.8h, v16.8h , v18.8h
    add       v18.8h, v14.8h , v12.8h
    uaddl     v20.8h, v8.8b, v2.8b
    smlal     v30.4s, v16.4h, v26.4h
    smlsl     v30.4s, v18.4h, v24.4h
    smlal2    v22.4s, v16.8h, v26.8h
    smlsl2    v22.4s, v18.8h, v24.8h
    uaddl     v18.8h, v11.8b, v1.8b
    uaddl     v16.8h, v7.8b, v5.8b
    sqrshrun  v12.4h, v30.4s, #10
    uaddl     v30.8h, v9.8b, v3.8b
    mla       v16.8h, v18.8h , v26.8h
    sqrshrun  v13.4h, v22.4s, #10
    mls       v28.8h, v20.8h , v24.8h

    mls       v16.8h, v30.8h , v24.8h
    uqxtn     v27.8b, v12.8h
    uqxtn     v13.8b, v13.8h
    mov       v27.s[1], v13.s[0]


    ext       v22.16b, v28.16b , v16.16b , #10

    saddl     v30.4s, v28.4h, v22.4h

    saddl2    v22.4s, v28.8h, v22.8h
    ext       v12.16b, v28.16b , v16.16b , #4
    ext       v18.16b, v28.16b , v16.16b , #6
    ext       v20.16b, v28.16b , v16.16b , #8
    ext       v28.16b, v28.16b , v16.16b , #2
    add       v12.8h, v12.8h , v18.8h
    add       v18.8h, v28.8h , v20.8h

    smlal     v30.4s, v12.4h, v26.4h
    smlsl     v30.4s, v18.4h, v24.4h
    smlal2    v22.4s, v12.8h, v26.8h
    smlsl2    v22.4s, v18.8h, v24.8h


    mov       v12.8b, v27.8b
    mov       v27.8b, v26.8b

    sqrshrun  v16.4h, v30.4s, #10

    mov       v6.16b, v2.16b
    mov       v7.16b, v3.16b

    sqrshrun  v17.4h, v22.4s, #10

    mov       v2.16b, v10.16b
    mov       v3.16b, v11.16b

    mov       v10.16b, v0.16b
    mov       v11.16b, v1.16b

    subs      x4, x4, #4
    uqxtn     v13.8b, v16.8h
    uqxtn     v17.8b, v17.8h
    mov       v13.s[1], v17.s[0]


    mov       v0.16b, v8.16b
    mov       v1.16b, v9.16b

    mov       v8.16b, v4.16b
    mov       v9.16b, v5.16b

    mov       v4.16b, v10.16b
    mov       v5.16b, v11.16b

    st1       {v12.2s}, [x1], x3        // store row 2
    st1       {v13.2s}, [x1], x3        // store row 3

    bgt       loop_8                    //if height =8  loop
    b         end_func

loop_4_start:
    ld1       {v0.2s, v1.2s}, [x0], x2  // Vector load from src[0_0]
    ld1       {v2.2s, v3.2s}, [x0], x2  // Vector load from src[1_0]
    ld1       {v4.2s, v5.2s}, [x0], x2  // Vector load from src[2_0]
    ld1       {v6.2s, v7.2s}, [x0], x2  // Vector load from src[3_0]
    ld1       {v8.2s, v9.2s}, [x0], x2  // Vector load from src[4_0]

loop_4:
    ld1       {v10.2s, v11.2s}, [x0], x2 // Vector load from src[5_0]
    uaddl     v14.8h, v4.8b, v6.8b      // temp1 = src[2_0] + src[3_0]
    uaddl     v12.8h, v0.8b, v10.8b     // temp = src[0_0] + src[5_0]
    uaddl     v16.8h, v2.8b, v8.8b      // temp2 = src[1_0] + src[4_0]
    mla       v12.8h, v14.8h , v26.8h   // temp += temp1 * 20
    uaddl     v18.8h, v5.8b, v7.8b      // temp1 = src[2_0] + src[3_0]
    uaddl     v14.8h, v1.8b, v11.8b     // temp = src[0_0] + src[5_0]
    uaddl     v22.8h, v3.8b, v9.8b      // temp2 = src[1_0] + src[4_0]
    mla       v14.8h, v18.8h , v26.8h   // temp += temp1 * 20
    mls       v12.8h, v16.8h , v24.8h   // temp -= temp2 * 5
    ld1       {v0.2s, v1.2s}, [x0], x2  // Vector load from src[6_0]
    uaddl     v16.8h, v6.8b, v8.8b
    mls       v14.8h, v22.8h , v24.8h   // temp -= temp2 * 5
    //Q6 and Q7 have filtered values
    uaddl     v28.8h, v2.8b, v0.8b

    ext       v22.16b, v12.16b , v14.16b , #10
    uaddl     v18.8h, v4.8b, v10.8b
    mla       v28.8h, v16.8h , v26.8h
    saddl     v30.4s, v12.4h, v22.4h

    saddl     v22.4s, v13.4h, v23.4h
    ext       v16.16b, v12.16b , v14.16b , #4
    mls       v28.8h, v18.8h , v24.8h
    ext       v18.16b, v12.16b , v14.16b , #6
    ext       v20.16b, v12.16b , v14.16b , #8
    ext       v14.16b, v12.16b , v14.16b , #2
    add       v16.8h, v16.8h , v18.8h
    add       v18.8h, v14.8h , v20.8h
    uaddl     v20.8h, v7.8b, v9.8b
    smlal     v30.4s, v16.4h, v26.4h
    smlsl     v30.4s, v18.4h, v24.4h
    smlal     v22.4s, v17.4h, v26.4h
    smlsl     v22.4s, v19.4h, v24.4h
    uaddl     v14.8h, v3.8b, v1.8b

    mla       v14.8h, v20.8h , v26.8h
    sqrshrun  v12.4h, v30.4s, #10
    uaddl     v16.8h, v5.8b, v11.8b
    sqrshrun  v13.4h, v22.4s, #10
    mls       v14.8h, v16.8h , v24.8h
    ld1       {v2.2s, v3.2s}, [x0], x2  // Vector load from src[7_0]
    uqxtn     v25.8b, v12.8h
    uaddl     v16.8h, v8.8b, v10.8b

    ext       v22.16b, v28.16b , v14.16b , #10
    uaddl     v20.8h, v4.8b, v2.8b
    saddl     v30.4s, v28.4h, v22.4h
    mla       v20.8h, v16.8h , v26.8h

    saddl     v22.4s, v29.4h, v23.4h
    ext       v16.16b, v28.16b , v14.16b , #4
    ext       v18.16b, v28.16b , v14.16b , #6
    ext       v12.16b, v28.16b , v14.16b , #8
    ext       v14.16b, v28.16b , v14.16b , #2
    add       v16.8h, v16.8h , v18.8h
    add       v18.8h, v12.8h , v14.8h

    smlal     v30.4s, v16.4h, v26.4h
    smlsl     v30.4s, v18.4h, v24.4h
    smlal     v22.4s, v17.4h, v26.4h
    smlsl     v22.4s, v19.4h, v24.4h


    uaddl     v18.8h, v6.8b, v0.8b
    sqrshrun  v16.4h, v30.4s, #10

    sqrshrun  v17.4h, v22.4s, #10

    mov       v12.8b, v25.8b
    mov       v25.8b, v24.8b

    uaddl     v28.8h, v9.8b, v11.8b
    uqxtn     v13.8b, v16.8h



    uaddl     v14.8h, v5.8b, v3.8b
    uaddl     v22.8h, v7.8b, v1.8b
    mls       v20.8h, v18.8h , v24.8h
    st1       {v12.s}[0], [x1], x3      // store row 0
    mla       v14.8h, v28.8h , v26.8h
    ld1       {v4.2s, v5.2s}, [x0], x2  // Vector load from src[8_0]
    uaddl     v30.8h, v10.8b, v0.8b
    uaddl     v28.8h, v6.8b, v4.8b
    mls       v14.8h, v22.8h , v24.8h
    st1       {v13.s}[0], [x1], x3      //store row 1
    mla       v28.8h, v30.8h , v26.8h

    ext       v22.16b, v20.16b , v14.16b , #10
    saddl     v30.4s, v20.4h, v22.4h

    saddl     v22.4s, v21.4h, v23.4h
    ext       v16.16b, v20.16b , v14.16b , #4
    ext       v18.16b, v20.16b , v14.16b , #6
    ext       v12.16b, v20.16b , v14.16b , #8
    ext       v14.16b, v20.16b , v14.16b , #2
    add       v16.8h, v16.8h , v18.8h
    add       v18.8h, v14.8h , v12.8h
    uaddl     v20.8h, v8.8b, v2.8b
    smlal     v30.4s, v16.4h, v26.4h
    smlsl     v30.4s, v18.4h, v24.4h
    smlal     v22.4s, v17.4h, v26.4h
    smlsl     v22.4s, v19.4h, v24.4h
    uaddl     v18.8h, v11.8b, v1.8b
    uaddl     v16.8h, v7.8b, v5.8b
    sqrshrun  v12.4h, v30.4s, #10
    uaddl     v30.8h, v9.8b, v3.8b
    mla       v16.8h, v18.8h , v26.8h
    sqrshrun  v13.4h, v22.4s, #10
    mls       v28.8h, v20.8h , v24.8h

    mls       v16.8h, v30.8h , v24.8h
    uqxtn     v27.8b, v12.8h

    ext       v22.16b, v28.16b , v16.16b , #10

    saddl     v30.4s, v28.4h, v22.4h

    saddl     v22.4s, v29.4h, v23.4h
    ext       v12.16b, v28.16b , v16.16b , #4
    ext       v18.16b, v28.16b , v16.16b , #6
    ext       v20.16b, v28.16b , v16.16b , #8
    ext       v28.16b, v28.16b , v16.16b , #2
    add       v12.8h, v12.8h , v18.8h
    add       v18.8h, v28.8h , v20.8h

    smlal     v30.4s, v12.4h, v26.4h
    smlsl     v30.4s, v18.4h, v24.4h
    smlal     v22.4s, v13.4h, v26.4h
    smlsl     v22.4s, v19.4h, v24.4h


    mov       v12.8b, v27.8b
    mov       v27.8b, v26.8b

    sqrshrun  v16.4h, v30.4s, #10

    mov       v6.16b, v2.16b
    mov       v7.16b, v3.16b

    sqrshrun  v17.4h, v22.4s, #10

    mov       v2.16b, v10.16b
    mov       v3.16b, v11.16b

    mov       v10.16b, v0.16b
    mov       v11.16b, v1.16b

    subs      x4, x4, #4
    uqxtn     v13.8b, v16.8h

    mov       v0.16b, v8.16b
    mov       v1.16b, v9.16b

    mov       v8.16b, v4.16b
    mov       v9.16b, v5.16b


    mov       v4.16b, v10.16b
    mov       v5.16b, v11.16b


    st1       {v12.s}[0], [x1], x3      // store row 2
    st1       {v13.s}[0], [x1], x3      // store row 3

    bgt       loop_4

end_func:
    //Restoring registers from stack
    ldp       x19, x20, [sp], #16
    pop_v_regs
    ret



