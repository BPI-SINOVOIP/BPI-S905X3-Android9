#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif
/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-08-13: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include <am_misc.h>
#include <am_mem.h>
#include "../am_vout_internal.h"
#include <string.h>

/****************************************************************************
 * Macro definitions
 ***************************************************************************/

#define DISP_MODE_FILE    "/sys/class/display/mode"
#define DISP_ENABLE_FILE  "/sys/class/display/enable"

/****************************************************************************
 * Static data
 ***************************************************************************/

static AM_ErrorCode_t aml_open(AM_VOUT_Device_t *dev, const AM_VOUT_OpenPara_t *para);
static AM_ErrorCode_t aml_set_format(AM_VOUT_Device_t *dev, AM_VOUT_Format_t fmt);
static AM_ErrorCode_t aml_enable(AM_VOUT_Device_t *dev, AM_Bool_t enable);
static AM_ErrorCode_t aml_close(AM_VOUT_Device_t *dev);

const AM_VOUT_Driver_t aml_vout_drv =
{
.open       = aml_open,
.set_format = aml_set_format,
.enable     = aml_enable,
.close      = aml_close
};

/****************************************************************************
 * API functions
 ***************************************************************************/

static AM_ErrorCode_t aml_open(AM_VOUT_Device_t *dev, const AM_VOUT_OpenPara_t *para)
{
	char buf[32];

	UNUSED(para);
	
	if(AM_FileRead(DISP_MODE_FILE, buf, sizeof(buf))==AM_SUCCESS)
	{
		if(!strncmp(buf, "576cvbs", 7))
			dev->format = AM_VOUT_FORMAT_576CVBS;
		else if(!strncmp(buf, "480cvbs", 7))
			dev->format = AM_VOUT_FORMAT_480CVBS;
		else if(!strncmp(buf, "576i", 4))
			dev->format = AM_VOUT_FORMAT_576I;
		else if(!strncmp(buf, "576p", 4))
			dev->format = AM_VOUT_FORMAT_576P;
		else if(!strncmp(buf, "480i", 4))
			dev->format = AM_VOUT_FORMAT_480I;
		else if(!strncmp(buf, "480p", 4))
			dev->format = AM_VOUT_FORMAT_480P;
		else if(!strncmp(buf, "720p", 4))
			dev->format = AM_VOUT_FORMAT_720P;
		else if(!strncmp(buf, "1080i", 5))
			dev->format = AM_VOUT_FORMAT_1080I;
		else if(!strncmp(buf, "1080p", 5))
			dev->format = AM_VOUT_FORMAT_1080P;
	}
	
	if(AM_FileRead(DISP_ENABLE_FILE, buf, sizeof(buf))==AM_SUCCESS)
	{
		int v;
		
		if(sscanf(buf, "%d", &v)==1)
		{
			dev->enable = v?AM_TRUE:AM_FALSE;
		}
	}
	
	return AM_SUCCESS;
}

static AM_ErrorCode_t aml_set_format(AM_VOUT_Device_t *dev, AM_VOUT_Format_t fmt)
{
	const char *cmd;
	
	UNUSED(dev);
	
	switch(fmt)
	{
		case AM_VOUT_FORMAT_576CVBS:
			cmd = "576cvbs";
		break;
		case AM_VOUT_FORMAT_480CVBS:
			cmd = "480cvbs";
		break;
		case AM_VOUT_FORMAT_576I:
			cmd = "576i";
		break;
		case AM_VOUT_FORMAT_576P:
			cmd = "576p";
		break;
		case AM_VOUT_FORMAT_480I:
			cmd = "480i";
		break;
		case AM_VOUT_FORMAT_480P:
			cmd = "480p";
		break;
		case AM_VOUT_FORMAT_720P:
			cmd = "720p";
		break;
		case AM_VOUT_FORMAT_1080I:
			cmd = "1080i";
		break;
		case AM_VOUT_FORMAT_1080P:
			cmd = "1080p";
		break;
		default:
			AM_DEBUG(1, "unknown video format %d", fmt);
			return AM_VOUT_ERR_INVAL_ARG;
		break;
	}
	
	return AM_FileEcho(DISP_MODE_FILE, cmd);
}

static AM_ErrorCode_t aml_enable(AM_VOUT_Device_t *dev, AM_Bool_t enable)
{
	char *cmd = enable?"1":"0";

	UNUSED(dev);
	
	return AM_FileEcho(DISP_ENABLE_FILE, cmd);
}

static AM_ErrorCode_t aml_close(AM_VOUT_Device_t *dev)
{
	UNUSED(dev);

	return AM_SUCCESS;
}

