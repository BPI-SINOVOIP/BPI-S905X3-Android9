/******************************************************************************
 *                                                                            *
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *****************************************************************************
 * Originally developed and contributed by Ittiam Systems Pvt. Ltd, Bangalore
*/
#ifndef IXHEAACD_HYBRID_H
#define IXHEAACD_HYBRID_H

#define HYBRID_FILTER_LENGTH 13
#define HYBRID_FILTER_DELAY 6
#define NO_QMF_CHANNELS_IN_HYBRID 3
#define NO_HYBRID_CHANNELS_LOW 2
#define NO_HYBRID_CHANNELS_HIGH 8

typedef struct {
  const WORD16 *ptr_resol;
  WORD8 ptr_qmf_buf;
  WORD32 *ptr_work_re;
  WORD32 *ptr_work_im;
  WORD32 *ptr_qmf_buf_re[NO_QMF_CHANNELS_IN_HYBRID];
  WORD32 *ptr_qmf_buf_im[NO_QMF_CHANNELS_IN_HYBRID];
  WORD32 *ptr_temp_re;
  WORD32 *ptr_temp_im;
} ia_hybrid_struct;

VOID ixheaacd_hybrid_analysis(const WORD32 *ptr_qmf_real, WORD32 *ptr_hyb_real,
                              WORD32 *ptr_hyb_imag,
                              ia_hybrid_struct *ptr_hybrid, WORD16 scale,
                              ia_sbr_tables_struct *sbr_tables_ptr);

#endif /* IXHEAACD_HYBRID_H */
