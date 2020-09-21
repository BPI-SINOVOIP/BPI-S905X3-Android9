/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */


#define LOG_TAG "media_ctl"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <cutils/log.h>
#include <amports/amstream.h>
#include "common_ctl.h"
#include <audio_ctl.h>

#ifdef  __cplusplus
extern "C" {
#endif
/*adec*/
int media_set_adec_format(int format)
{
	return media_set_ctl("media.adec.format",format);
}

int media_get_adec_format()
{
    return media_get_ctl("media.adec.format");
}

int media_set_adec_samplerate(int rate)
{
	return media_set_ctl("media.adec.samplerate",rate);
}

int media_get_adec_samplerate()
{
    return media_get_ctl("media.adec.samplerate");
}

int media_set_adec_channum(int num)
{
	return media_set_ctl("media.adec.channum",num);
}

int media_get_adec_channum()
{
    return media_get_ctl("media.adec.channum");
}

int media_set_adec_pts(int pts)
{
	return media_set_ctl("media.adec.pts",pts);
}

int media_get_adec_pts()
{
    return media_get_ctl("media.adec.pts");
}

int media_set_adec_datawidth(int width)
{
    return media_set_ctl("media.adec.datawidth",width);
}

int media_get_adec_datawidth()
{
    return media_get_ctl("media.adec.datawidth");
}

int media_get_audio_digital_output_mode()
{
	return media_get_ctl("media.audio.audiodsp.digital_raw");
}
#ifdef  __cplusplus
}
#endif