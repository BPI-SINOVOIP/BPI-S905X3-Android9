/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */


/**
* @file codec_msg.h
* @brief  Function prototype of codec error
* 
* @version 1.0.0
* @date 2011-02-24
*/
/* Copyright (C) 2007-2011, Amlogic Inc.
* All right reserved
*
*/
#ifndef CODEC_MSG_H
#define CODEC_MSG_H

const char * codec_error_msg(int error);
int system_error_to_codec_error(int error);
void print_error_msg(int error, int syserr, const char *func, int line);

#endif
