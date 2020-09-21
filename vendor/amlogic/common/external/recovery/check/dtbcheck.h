/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
* *
This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
* *
Description:
*/

#ifndef _SECURITY_H_
#define _SECURITY_H_

#define DTB_IMG "dt.img"

extern unsigned int recovery_size1;

#define DTB_ERROR                  (-1)
#define DTB_ALLOW                  (0)
#define DTB_TWO_STEP            (3)

typedef struct Dtb_Partition_s
{
    char partition_name[16];
    unsigned int  partition_size;
}Dtb_Partition_S;


#endif