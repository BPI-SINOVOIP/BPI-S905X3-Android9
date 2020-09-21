/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package. *
* Description:
*/
#ifndef _HUFFMAN_DECODE_PRIVATE_H
#define _HUFFMAN_DECODE_PRIVATE_H

#include "am_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

int psi_atsc_huffman_to_string(unsigned char *out_str, const unsigned char *compressed,
							unsigned int size, unsigned int table_index);

#ifdef __cplusplus
}
#endif

#endif