/***************************************************************************
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 ***************************************************************************/

#ifndef _AM_CRYPT_H
#define _AM_CRYPT_H

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

/****************************************************************************
 * Error code definitions
 ****************************************************************************/

/****************************************************************************
 * Type definitions
 ***************************************************************************/

/**\brief */
typedef struct
{
	void *(*open)();
	int   (*close)(void *cryptor);
	void  (*crypt)(void *cryptor,
		uint8_t *dst, uint8_t *src, int len,
		int decrypt);
} AM_Crypt_Ops_t;

#define CRYPT_SET(_ops_) ((_ops_) && (_ops_)->crypt)

/****************************************************************************
 * Function prototypes
 ***************************************************************************/

void *AM_CRYPT_des_open(const uint8_t *key, int key_bits);

int AM_CRYPT_des_close(void *cryptor);

void AM_CRYPT_des_crypt(void* cryptor, uint8_t* dst, const uint8_t *src, int len, uint8_t *iv, int decrypt);

#ifdef __cplusplus
}
#endif

#endif

