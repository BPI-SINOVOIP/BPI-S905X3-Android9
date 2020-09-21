/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description: h file
*/
#ifndef VOB_SUB_H
#define VOB_SUB_H

#include "sub_subtitle.h"

int get_vob_spu(char *input_buf, int *buf_size, unsigned length,
                AML_SPUVAR *spu);
void dvdsub_init_decoder(void);
unsigned short doDCSQC(unsigned char *pdata, unsigned char *pend);

typedef enum
{
    FSTA_DSP = 0,
    STA_DSP = 1,
    STP_DSP = 2,
    SET_COLOR = 3,
    SET_CONTR = 4,
    SET_DAREA = 5,
    SET_DSPXA = 6,
    CHG_COLCON = 7,
    CMD_END = 0xFF,
} CommandID;

#endif
