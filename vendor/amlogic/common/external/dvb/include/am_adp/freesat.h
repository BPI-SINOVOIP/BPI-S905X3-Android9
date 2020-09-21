/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package. *
* Description:
*/
#ifndef FREESAT_H
#define FREESAT_H

#include <sys/types.h>

char *freesat_huffman_decode(const unsigned char *compressed, size_t size);

#endif