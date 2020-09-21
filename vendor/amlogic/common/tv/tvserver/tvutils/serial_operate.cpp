/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#define LOG_TAG "tvserver"
#define LOG_TV_TAG "serial_operate"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <termios.h>
#include <errno.h>
#include <linux/hidraw.h>

#include "include/serial_base.h"
#include "include/serial_operate.h"
#include "include/CTvLog.h"

//******************************************************
#ifndef HIDIOCSFEATURE
#define HIDIOCSFEATURE(len)    _IOC(_IOC_WRITE|_IOC_READ, 'H', 0x06, len)
#define HIDIOCGFEATURE(len)    _IOC(_IOC_WRITE|_IOC_READ, 'H', 0x07, len)
#endif
#define product_nid  0x00e0
//#define product_nid  0x00e0

#define vendor_id    0x1A1D
#define ENDPOINT     3

#define EVENT_2_4G_HEADSET_ON     0
#define EVENT_2_4G_HEADSET_OFF    1

CTv2d4GHeadSetDetect::CTv2d4GHeadSetDetect()
{
}

CTv2d4GHeadSetDetect::~CTv2d4GHeadSetDetect()
{
}

int CTv2d4GHeadSetDetect::startDetect()
{
    this->run("CTv2d4GHeadSetDetect");

    return 0;
}

bool  CTv2d4GHeadSetDetect::threadLoop()
{
    int i = 0, rd_len = 0;
    int thread_cmd_dly_tm = 1000 * 1000;
    int tvThermal_cnt = 0, fd = 0;
    char data[10] = "0";

    LOGD("%s, entering...\n", __FUNCTION__);

    //SetSerialBThreadExecFlag(1);

    //*********************************
    int hidraw_fd;
    unsigned char buf[32];
    char phybuf[256];
    struct hidraw_devinfo info;
    int read_size = 0;
    bool  debug = true;
    char device[68];
    int HeadsetConnectState = false;

    int curdeviceID = -1;
    for (int deviceID = 0; deviceID < 5; deviceID++) {
        sprintf(device, "/dev/hidraw%d", deviceID);
        LOGD(" thread  device =%s ", device  );
        if ((hidraw_fd = open(device, O_RDWR)) < 0  ) {
            LOGD("cann't open path:%s!!!\n", device);
            continue;
        }
        memset(phybuf, 0x0, 256);
        LOGD("AAAAAAAAAAAAAA:%s!!!\n", device);
        if (ioctl(hidraw_fd, HIDIOCGRAWINFO, &info) >= 0  &&
                ioctl(hidraw_fd, HIDIOCGRAWPHYS(256), phybuf) >= 0) {
            LOGD("\t %d, product id = 0x%04x \n", __LINE__, info.product);
            LOGD("\t %d, vendor  id = 0x%04x \n", __LINE__, info.vendor);
            int len = strlen(phybuf);
            if (phybuf[len - 1] - '0' == ENDPOINT) {
                if (info.vendor == vendor_id) {
                    curdeviceID = deviceID;
                    LOGD("\t product id = 0x%04x \n", info.product);
                    LOGD("\t vendor  id = 0x%04x\n", info.vendor);
                    break;
                }
            }
        }
        close(hidraw_fd);
    }
    if (curdeviceID ==  -1)
        return 0;

    sprintf(device, "/dev/hidraw%d", curdeviceID);
    LOGD(" thread  device =%s ", device  );
    if ( (hidraw_fd = open(device,  O_RDWR | O_NONBLOCK) ) < 0 ) {
        printf("cann't open path:%s!!!\n", device);
        return 0;
    }
    int checkvalue[300] ;
    int countcheck = 0;
    int count = 0;
    int ritemcounts = 15;
    //****************************************

    while ( !exitPending() ) { //requietexit() or requietexitWait() not call
        //loop codes
        //LOGD("while 2.4G %s ", __FUNCTION__);

        memset(buf, 0x0, 32);
        for (int ritem = 0; ritem < ritemcounts ; ritem++ ) {
            read_size = read(hidraw_fd, buf, 32);
            //for (int i = 0; i < 32; i++)
            //ALOGD("read_size %d ", read_size);
            if (debug) {
                count ++;
                if (count == 3000) {
                    LOGD("%02x %02x %02x %02x %02x %02x ", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
                    count = 0;
                }
            }
            if (read_size < 0 ) {

            }

            checkvalue[countcheck] = buf[4] & 0x1f;
            if (countcheck == 299) {
                int checkcountvalue = 0;
                for (int icheck = 0; icheck <  countcheck ; icheck++ )
                    checkcountvalue  += checkvalue[icheck];
                // LOGD("checkcountvalue = %d",checkcountvalue);
                if (checkcountvalue <=   5 * 4) {
                    if (HeadsetConnectState == true) {
                        if (debug) {
                            LOGD("headset connect   false");
                            LOGD("headset connect   false");
                        }

                        mpObserver->onHeadSetDetect(0, 0);
                        //usleep(1000 * 200);
                    }
                    HeadsetConnectState = false;
                } else if (checkcountvalue >=    200 * 4) {
                    if (HeadsetConnectState == false) {
                        if (debug) {
                            LOGD("headset connect	 true");
                            LOGD("headset connect	 true");
                        }
                        mpObserver->onHeadSetDetect(1, 0);
                        //usleep(1000 * 200);
                    }
                    HeadsetConnectState = true;
                }
                countcheck = 0;
            }
            countcheck ++;

            // bit 0: headset mic in/off; bit 1:headset on/off; bit 2: headphone on/off; bit 3: soundbar on/off ;bit 4: subwoofer on/off
            /*  else if (buf[4] & 0x1f)
            {
            if (HeadsetConnectState == false)
            {
                  if (debug)
                  {
                     ALOGD("headset connect   true");
                     ALOGD("headset connect   true");
                  }
                  android::TvService::getIntance()->SendDtvStats(1,0,0,0,0,0);
                  //usleep(1000 * 200);
            }
            HeadsetConnectState = true;
            }
            else
            {
            if (HeadsetConnectState == true)
            {
                  if (debug)
                  {
                       ALOGD("headset connect   false");
                       ALOGD("headset connect   false");
                  }
                  android::TvService::getIntance()->SendDtvStats(2,0,0,0,0,0);
                  //usleep(1000 * 200);
            }
             HeadsetConnectState = false;
            }*/
        }
        {
            //added for fbc thermal setting
            tvThermal_cnt++;
            if (tvThermal_cnt == 300) {  //60 sec
                tvThermal_cnt = 0;
                fd = open("/sys/class/thermal/thermal_zone0/temp", O_RDONLY);
                if (fd < 0) {
                    LOGE("ERROR: failed to open file error: %d\n", errno);
                } else {
                    read(fd, data, sizeof(data));
                    close(fd);
                    LOGD("thermal temp data = %s ~~~~~~\n", data);
                    int x = 0;
                    x = atoi(data);
                    mpObserver->onThermalDetect(x);
                    LOGD("int data :%d\n", x);
                }
            }
        }
        usleep(1000 * 200);
    }
    //exit
    //return true, run again, return false,not run.
    return false;
}


