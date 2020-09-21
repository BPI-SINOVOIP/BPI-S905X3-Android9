/*
 * Copyright (c) 2015 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 */
/*
 * \file        res_dll_if.h
 * \brief       This interfaces is included by user who use this dll
 *
 * \version     1.0.0
 * \date        29/10/2013   14:19
 * \author      Sam.Wu <yihui.wu@amlgic.com>
 *
 * Copyright (c) 2013 Amlogic Inc.. All Rights Reserved.
 *
 */
#ifndef __RES_DLL_IF_H__
#define __RES_DLL_IF_H__


#ifdef BUILD_DLL

/* DLL export */
#define DLL_API __declspec(dllexport)
#else

/* DLL import */
#define DLL_API __declspec(dllimport)
#endif

DLL_API int res_img_pack(const char* const szDir, const char* const outResImg);

DLL_API int res_img_unpack(const char* const path_src, const char* const unPackDirPath, int needCheckCrc);

#endif // __RES_DLL_IF$_H__
