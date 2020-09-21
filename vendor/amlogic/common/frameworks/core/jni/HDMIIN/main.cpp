/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC test code for hdmi audio pass through using our audiodsp
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <errno.h>
#include "audio_global_cfg.h"
#include "audiodsp_control.h"
#include "audio_utils_ctl.h"
#include "mAlsa.h"
#define LOG  printf

int main(int argc,char **argv)
{
	LOG("audio passthrough test code start\n");
	AudioUtilsInit();
	LOG("init audioutil finished\n");
	mAlsaInit(0, CC_FLAG_CREATE_TRACK | CC_FLAG_START_TRACK|CC_FLAG_CREATE_RECORD|CC_FLAG_START_RECORD|CC_FLAG_SOP_RECORD);
	LOG("start audio track finished\n");
	audiodsp_start();
	LOG("audiodsp start finished\n");
	AudioUtilsStartLineIn();
	LOG("start line in finished\n");
	while(1){
		usleep(500);
	}
exit:
	AudioUtilsStopLineIn();
	LOG("stop line in finished\n");
	AudioUtilsUninit();
	LOG("uninit audioutil finished\n");
	mAlsaUninit(0);
	LOG("stop audio track finished\n");
	audiodsp_stop();
	LOG("audiodsp stop finished\n");
}
