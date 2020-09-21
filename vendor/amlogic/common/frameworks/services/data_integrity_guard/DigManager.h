/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * frank.chen@amlogic.com
 */

#ifndef _DIGMANAGER_H
#define _DIGMANAGER_H

#include <pthread.h>
#include <sysutils/SocketListener.h>

#ifndef MD5_DIGEST_LENGTH
#define MD5_DIGEST_LENGTH 16
#endif

//data only check time in bootup:
#define INTERVAL_IN_BOOT 1

//data only check time in normal:
#define INTERVAL_AFTER_BOOT 60

//system partition check time: CHECK_SYSTEM_COUNT * "data only check time"
#define CHECK_SYSTEM_COUNT 5

//sleep between system file check, unit is us
#define CHECK_SYSTEM_FILE_SLEEP_TIME 5000

#define CHECKSUM_LIST_PATH "/system/chksum_list"

#define SYSTEM_BAK_NODE "/dev/block/backup"

#define DIG_DATA_RO_COUNT_FILE "/cache/dig_data_ro_count"

//could overwrite the g_data_ro_count_max value via DATA_RO_COUNT_MAX_PROP
#define DATA_RO_COUNT_MAX_PROP "ro.dig.dataro_count_max"
#define DATA_RO_COUNT_MAX_DEFAULT "5"

// data remount as read only by kernel
#define DIG_REPORT_DATA_READ_ONLY 680
// data can' t mount
#define DIG_REPORT_DATA_CRASH 681
// system file change
#define DIG_REPORT_SYSTEM_CHANGED 682

class DigManager {
private:
    static DigManager *sInstance;

private:
    SocketListener       *mBroadcaster;
    struct selabel_handle *mSehandle;
    pthread_t mTread;
    bool mBootCompleted;
    bool mSupportSysBak;
    int mDataRoCountMax;
    int mCheckInterval;
    int mCheckSystemCount;
    bool mConnected;

public:
    virtual ~DigManager();

    int start();
    int stop();

    void setBroadcaster(SocketListener *sl) { mBroadcaster = sl; }
    SocketListener *getBroadcaster() { return mBroadcaster; }

    static DigManager *Instance();
    static void StartDig();
    void setConnect(bool status) { mConnected = status; }
    bool isConnected() { return mConnected; }

private:
    DigManager();
    void* workThread(void);
    static void* _workThread(void *cookie);
    void startListener();
    bool waitForConnected();
    void sendBroadcast(int code, const char *msg, bool addErrno);

    int isFileExist(const char* path);
    int isSupportSystemBak();
    int isInitMountDataFail();
    int isABupdate();
    void doRestoreSystem();
    void HanldeSysChksumError(char* error_file_path);
    void doReboot();
    void doRebootRecoveryTipDataro();
    int getDataRoCount();
    void setDataRoCount(int count);
    void handleDataRo();
    void handleCacheRo();
    void handleCacheNull();
    void doRemount( char* dev, char* target, char* system, int readonly );
    int uRead(int  fd, void*  buff, int  len);
    int fRead(const char*  filename, char* buff, size_t  buffsize);
    int isVolumeRo(char *device);
    int isBootCompleted();
    void hextoa(char *szBuf, unsigned char nData[], int len);
    int getMd5(const char *path, unsigned char* md5);
    int checkSystemPartition(char* error_file_path);
    void handleInitMountDataFail();
    int getDataRoCountMax();
    bool isRebooting();
    int doMount(char *name, char *device);
};
#endif
