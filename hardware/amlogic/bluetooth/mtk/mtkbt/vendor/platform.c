/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 *
 * MediaTek Inc. (C) 2014. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

#define LOG_TAG "bt-platform"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <cutils/properties.h>
#include <errno.h>
#include <asm/ioctl.h>
//#include <cutils/properties.h>
#include <cutils/log.h>
#include <string.h>
#include <unistd.h>

#define SDIO_POWER_UP    _IO('m',3)
#define SDIO_POWER_DOWN  _IO('m',4)

#define BT_POWER_OFF 0
#define BT_POWER_ON  1

#define RFKILL_DEVICE 1
#define DEV_NODE_DEVICE 2

#ifndef PROPERTY_VALUE_MAX
#define PROPERTY_VALUE_MAX 92
#endif

#define CLEAR_ERROR_INFO (errno = 0)

static const char DRIVER_PROP_NAME[] = "vendor.sys.mtkbtdriver";


const char *rfkill_board_list[] = {
    "24",
    NULL
};
const char *devNode_board_List[] = {
    "19",
    NULL
};

int getBoardCapabilities(){
	
    int index_ptr = 0;
    char prpt[PROPERTY_VALUE_MAX];
    memset(prpt, '\0', PROPERTY_VALUE_MAX);
    property_get("ro.build.version.sdk", prpt, NULL);

    do{
        if(0 == strcmp(devNode_board_List[index_ptr], prpt))
        {
            ALOGD("Matched a dev_node device: %s", prpt);
            return DEV_NODE_DEVICE;
        }
        index_ptr ++;
    }while(devNode_board_List[index_ptr] != NULL);


    ALOGD("We're goting to access rfkill to power on this device(sdk: %s) by default...", prpt);
    return RFKILL_DEVICE;
}

static int is_rfkill_disabled(void)
{
    char value[PROPERTY_VALUE_MAX];

    property_get("ro.rfkilldisabled", value, "0");

    if (strcmp(value, "1") == 0) {
        ALOGE("is_rfkill_disabled ? [%s]", value);
        return -1;
    }
    return 0;
}

static int init_rfkill(char **rfkill_state_path)
{
    char path[64];
    char buf[16];
    int fd, sz, id;

    if (is_rfkill_disabled())
    {
        ALOGE("The rfkill module has been disabled!");
        return -1;
    }

    for (id = 0; ; id++)
    {
        snprintf(path, sizeof(path), "/sys/class/rfkill/rfkill%d/type", id);
        fd = open(path, O_RDONLY);
        if (fd < 0)
        {
            ALOGE("init_rfkill : open(%s) failed: %s (%d)\n", \
                 path, strerror(errno), errno);
            errno = 0;
            return -1;
        }

        sz = read(fd, &buf, sizeof(buf));
        close(fd);

        if (sz >= 9 && memcmp(buf, "bluetooth", 9) == 0)
            break;
    }

    asprintf(rfkill_state_path, "/sys/class/rfkill/rfkill%d/state", id);
    ALOGD("rfkill state path init successed: %s", *rfkill_state_path);
    return 0;
}

int rfkill_operations(int on)
{
    int fd = -1, ret = -1;
    char buffer = '0';
    static char *rfkill_state_path = NULL;
    CLEAR_ERROR_INFO;
    if(rfkill_state_path)
        ALOGW("rfkill_state_path already init: %s", rfkill_state_path);
    else
    {
        if (init_rfkill(&rfkill_state_path))
        {
            ALOGE("####INIT rfkill fail###");
            return ret;
        }
    }
    
    fd = open(rfkill_state_path, O_WRONLY);
    ALOGD("open %s :%d(%s)", rfkill_state_path, fd, strerror(errno));
    if(fd > 0)
    {
        switch(on)
        {
            case BT_POWER_OFF:
                buffer = '0';
                break;
    
            case BT_POWER_ON:
                buffer = '1';
                break;
            default:
                ALOGE("Unsupported ops(%d)!", on);
                close(fd);
                return ret;
        }
    
        //write(fd, "0", 1);//pull down first.
        //usleep(10000);//10ms
        
        if(write(fd, &buffer, 1) > 0)
        {
        //  fsync(fd);
            usleep(10000);//10ms
            ret = 0;
        }
        close(fd);
        if(buffer == '0')
        {
            free(rfkill_state_path);
            rfkill_state_path = NULL;
        }
    }
    ALOGD("%s: power %s %s(%s)!\n", __FUNCTION__, BT_POWER_ON == on? "up":"down", ret == 0? "done":"failed", strerror(errno));
    CLEAR_ERROR_INFO;
    return ret;
}

int dev_node_operations(int on)
{
    int fd = -1, ret = -1,ops;

    CLEAR_ERROR_INFO;
    
    fd = open("/dev/wifi_power", O_RDWR);
    ALOGD("open: %d", fd);
    if(fd > 0)
    {
        switch(on)
        {
            case BT_POWER_OFF:
                ops = SDIO_POWER_DOWN;
                break;

            case BT_POWER_ON:
                ops = SDIO_POWER_UP;
                break;
            default:
                ALOGE("Unsupported ops(%d)!", on);
                close(fd);
                return ret;
        }
        if(ioctl(fd, ops) >= 0)
        {
            usleep(10000);//10ms
            ret = 0;
        }
        close(fd);
    }
    ALOGD("%s: power %s %s(%s)!\n", __FUNCTION__, BT_POWER_ON == on? "up":"down", ret == 0? "done":"failed", strerror(errno));
    CLEAR_ERROR_INFO;
    return ret;

}

int set_sdio_power(int on)
{
    CLEAR_ERROR_INFO;

    switch(getBoardCapabilities())
    {
        case RFKILL_DEVICE:
            return rfkill_operations(on);
        case DEV_NODE_DEVICE:
            return dev_node_operations(on);
		default:
			return -1; //never get to this line.
    }
}

int ismod_bt_driver()
{
    char driver_status[PROPERTY_VALUE_MAX];
    CLEAR_ERROR_INFO;
    
    property_get(DRIVER_PROP_NAME, driver_status, "mtkdrunkown");
    ALOGD("%s: driver_status = %s ", __FUNCTION__, driver_status);
    if(strcmp("true", driver_status) == 0)
    {
        ALOGW("%s: btmtksdio.ko is already insmod!", __FUNCTION__);
        return 0;
    }
    ALOGE("%s: set vendor.sys.mtkbtdriver true\n", __FUNCTION__);
    property_set(DRIVER_PROP_NAME,"true");
    sleep(2);
    return 0;
}

int load_mtkbt()
{
    if(set_sdio_power(1))
    {
        ALOGE("%s: failed to power up, so return directly!\n", __FUNCTION__);
        return -1;
    }

    if(ismod_bt_driver())
    {
        ALOGE("%s: failed to insmod bt driver, so return directly!\n", __FUNCTION__);
        return -1;
    }
    return 0;
}

void mtkbt_unload()
{
return;  //for android p
    property_set(DRIVER_PROP_NAME,"false");
    sleep(2);
    set_sdio_power(0);
}
