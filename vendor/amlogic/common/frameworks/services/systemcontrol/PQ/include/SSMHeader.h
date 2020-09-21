/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef __SSM_DATA_STRUCT_H__
#define __SSM_DATA_STRUCT_H__

#include "PQSettingCfg.h"
#include "PQType.h"

struct SSMHeader_section1_t
{
	unsigned int magic;
	unsigned int version;//count by ID and its size
	unsigned int count;//count by column of excel
	unsigned int rsv[6];//reserve
};

struct SSMHeader_section2_t
{
    unsigned int addr;//Id's addr
	unsigned int size;//this item size
	unsigned int valid;
	unsigned char rsv[6];
};


typedef struct current_ssmheader_section2_s {
	unsigned int size;
	SSMHeader_section2_t *header_section2_data;
    void *reserved;
} current_ssmheader_section2_t;

extern struct SSMHeader_section1_t gSSMHeader_section1;
extern struct SSMHeader_section2_t gSSMHeader_section2[];

#endif
