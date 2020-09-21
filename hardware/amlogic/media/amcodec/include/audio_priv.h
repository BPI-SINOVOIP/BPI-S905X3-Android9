/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */


/**
* @file audio_priv.h
* @brief  Funtion prototypes of audio lib
* 
* @version 1.0.0
* @date 2011-02-24
*/
/* Copyright (C) 2007-2011, Amlogic Inc.
* All right reserved
*
*/
#ifndef CODEC_PRIV_H_
#define CODEC_PRIV_H_
void audio_start(void **priv, arm_audio_info *a_ainfo);
void audio_stop(void **priv);
void audio_pause(void *priv);
void audio_resume(void *priv);
#endif
