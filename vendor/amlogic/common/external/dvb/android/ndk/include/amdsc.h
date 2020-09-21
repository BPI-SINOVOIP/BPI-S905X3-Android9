/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package. *
* Description:
*/
#ifndef _AMDSC_H
#define _AMDSC_H

#include <linux/types.h>

enum am_dsc_key_type_t {
	AM_DSC_EVEN_KEY = 0,
	AM_DSC_ODD_KEY = 1,
	AM_DSC_EVEN_KEY_AES = 2,
	AM_DSC_ODD_KEY_AES = 3,
	AM_DSC_EVEN_KEY_AES_IV = 4,
	AM_DSC_ODD_KEY_AES_IV = 5,
	AM_DSC_FROM_KL_KEY = (1<<7)
};
struct am_dsc_key {
	enum am_dsc_key_type_t    type;
	__u8                 key[16];
};

#define AMDSC_IOC_MAGIC  'D'

#define AMDSC_IOC_SET_PID      _IO(AMDSC_IOC_MAGIC, 0x00)
#define AMDSC_IOC_SET_KEY      _IOW(AMDSC_IOC_MAGIC, 0x01, int)

#endif

