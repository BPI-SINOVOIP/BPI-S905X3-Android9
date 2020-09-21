/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */


#ifndef VFM_CTL_H
#define VFM_CTL_H
#ifdef  __cplusplus
extern "C" {
#endif
/*val --"rm default" or "add default decoder ...." */
int media_set_vfm_map(const char* val);
int media_get_vfm_map(char* val,int size);
/* name --"default"; val -- "default decoder ..."*/
int media_rm_vfm_map(const char* name,const char* val);
int media_add_vfm_map(const char* name,const char* val);

#ifdef  __cplusplus
}
#endif
#endif

