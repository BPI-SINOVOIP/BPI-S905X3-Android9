/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC AUDIO_UTILS_CTL
 */

#ifndef __AUDIO_UTILS_CTL_H__
#define __AUDIO_UTILS_CTL_H__

#define AMAUDIO_IOC_MAGIC                   'A'
#define AMAUDIO_IOC_START_LINE_IN           _IOW(AMAUDIO_IOC_MAGIC, 0x15, int)
#define AMAUDIO_IOC_START_HDMI_IN           _IOW(AMAUDIO_IOC_MAGIC, 0x16, int)
#define AMAUDIO_IOC_STOP_LINE_IN            _IOW(AMAUDIO_IOC_MAGIC, 0x17, int)
#define AMAUDIO_IOC_STOP_HDMI_IN            _IOW(AMAUDIO_IOC_MAGIC, 0x18, int)

#ifdef __cplusplus
extern "C" {
#endif

int AudioUtilsInit(void);
int AudioUtilsUninit(void);
int AudioUtilsStartLineIn(void);
int AudioUtilsStartHDMIIn(void);
int AudioUtilsStopLineIn(void);
int AudioUtilsStopHDMIIn(void);

#ifdef __cplusplus
}
#endif

#endif
