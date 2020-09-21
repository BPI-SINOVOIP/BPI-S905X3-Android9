/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#define LOG_TAG "FBC"
#define LOG_FBC_TAG "CFbcUpgrade"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <fcntl.h>
#include <errno.h>
#include <CFbcLog.h>

#include "CFbcUpgrade.h"

static CFbcUpgrade *mInstance;

CFbcUpgrade *getFbcUpgradeInstance()
{
    if (mInstance == NULL)
        mInstance = new CFbcUpgrade();

    return mInstance;
}

CFbcUpgrade::CFbcUpgrade()
{
    mUpgradeMode = CC_UPGRADE_MODE_MAIN;
    mFileName[0] = 0;

    mOPTotalSize = 0;
    mBinFileSize = 0;
    mBinFileBuf = NULL;
    mUpgradeBlockSize = 0x10000;
    //mNewBaudRate = 115200*2;//Default AdjustUpgradeSpeed baudRate
    mNewBaudRate = 0;

    mpObserver = NULL;
    mState = STATE_STOPED;
    mCfbcIns = GetFbcProtocolInstance();
    mCfbcIns->SetUpgradeFlag(0);
}

CFbcUpgrade::~CFbcUpgrade()
{
    if (mBinFileBuf != NULL) {
        delete mBinFileBuf;
        mBinFileBuf = NULL;
    }
}

int CFbcUpgrade::start()
{
    if (mState == STATE_STOPED || mState == STATE_ABORT
        || mState == STATE_FINISHED) {
        mCfbcIns->SetUpgradeFlag(1);
        this->run("CFbcUpgrade");
    } else {
        mCfbcIns->SetUpgradeFlag(0);
        LOGD("%s, Upgrade is running now...\n", __FUNCTION__);
    }

    return 0;
}

int CFbcUpgrade::stop()
{
    requestExit();
    mState = STATE_STOPED;

    return 0;
}

void CFbcUpgrade::SetUpgradeBaudRate(unsigned int baudRate)
{
    mNewBaudRate = baudRate;
}

int CFbcUpgrade::SetUpgradeFileName(char *file_name)
{
    if (NULL == file_name)
        return -1;

    strcpy(mFileName, file_name);

    return 0;
}

int CFbcUpgrade::SetUpgradeBlockSize(int block_size)
{
    mUpgradeBlockSize = block_size;

    return 0;
}

//TODO, from fbc3 mode is invalid, partition describe upgrade infomation
int CFbcUpgrade::SetUpgradeMode(int mode)
{
    int tmp_val = 0;

    tmp_val = mUpgradeMode;
    mUpgradeMode = mode;

    return tmp_val;
}

int CFbcUpgrade::AddCRCToDataBuf(unsigned char data_buf[], int data_len)
{
    unsigned int tmp_crc = 0;

    tmp_crc = mCfbcIns->Calcrc32(0, data_buf, data_len);

    data_buf[data_len + 0] = (tmp_crc >> 0) & 0xFF;
    data_buf[data_len + 1] = (tmp_crc >> 8) & 0xFF;
    data_buf[data_len + 2] = (tmp_crc >> 16) & 0xFF;
    data_buf[data_len + 3] = (tmp_crc >> 24) & 0xFF;

    return 0;
}

CFbcUpgrade::partition_info_t* CFbcUpgrade::getPartitionInfoUpgFile(int section,
                                                                    int partition)
{
    unsigned int offset = 0;
    partition_info_t* ret = NULL;

    if (PARTITION_FIRST_BOOT == partition) {
        offset = FIRST_BOOT_INFO_OFFSET;
    } else if ((section < SECTION_NUM) && (partition < PARTITION_NUM)) {
        offset = section ? SECTION_1_INFO_OFFSET : SECTION_0_INFO_OFFSET;
        offset += PARTITION_INFO_SIZE * (partition - 1);
    }

    ret = reinterpret_cast<partition_info_t*>(mBinFileBuf + offset);

    return ret;
}

bool CFbcUpgrade::parseUpgradeInfo(const int section, const int partition,
                                   int &start, int &length)
{
    bool ret = true;
    partition_info_t *info = NULL;

    if (SECTION_0 == section) {
        info = getPartitionInfoUpgFile(section, partition);

        switch (partition) {
            case PARTITION_FIRST_BOOT:
            case PARTITION_SECOND_BOOT:
            case PARTITION_SUSPEND:
            case PARTITION_UPDATE:
                start = info->code_offset;
                length = info->code_size + info->data_size;
                break;
            case PARTITION_MAIN:
                start = info->code_offset;
                length = info->code_size + info->data_size + info->spi_code_size
                        + info->readonly_size + info->audio_param_size
                        + info->sys_param_size;
                break;
            case PARTITION_PQ:
            case PARTITION_USER:
            //case PARTITION_FACTORY:
                start = info->data_offset;
                length = info->data_size;
                break;
            default:
                ret = false;
                break;
        }
    } else {
        ret = false;
    }

    //LOGD("%s, start:%08x, length:%08x\n", __FUNCTION__, start, length);

    return ret;
}

bool CFbcUpgrade::initBlocksInfo(std::list<sectionInfo> &mList)
{
    int currentPartition = 0;
    int start = 0, length = 0;
    bool ret = true;

    mList.clear();

    //TODO - Only PcTool factory mode support upgrade KEY, FIRST_BOOT, FIRST_BOOT_INFO
    /*
         mList.push_back(sectionInfo(KEY_OFFSET, KEY_SIZE, "key"));
         mOPTotalSize += KEY_SIZE;
         mList.push_back(sectionInfo(FIRST_BOOT_INFO_OFFSET, FIRST_BOOT_INFO_SIZE, "first boot info"));
         mOPTotalSize += FIRST_BOOT_INFO_SIZE;
         */
    mList.push_back(sectionInfo(SECTION_0_INFO_OFFSET, SECTION_INFO_SIZE));
    mOPTotalSize += SECTION_INFO_SIZE;

    for (currentPartition = PARTITION_SECOND_BOOT;
         currentPartition < PARTITION_NUM; currentPartition++) {
        if (!parseUpgradeInfo(SECTION_0, currentPartition, start, length)) {
            ret = false;
            mList.clear();
            break;
        }

        mList.push_back(sectionInfo(start, length));
        mOPTotalSize += length;
    }

    return ret;
}

bool CFbcUpgrade::sendUpgradeCmd(int &ret_code)
{
    int cmdLen;
    bool ret = true;
    unsigned int cmd;

    if (mNewBaudRate != 0) {
        cmd = FBC_REBOOT_UPGRADE_AUTO_SPEED;
        cmdLen = 14;
    }
    else {
        cmd = FBC_REBOOT_UPGRADE;
        cmdLen = 10;
    }

    mDataBuf[0] = 0x5A;
    mDataBuf[1] = 0x5A;
    mDataBuf[2] = cmdLen + 4;
    mDataBuf[3] = 0x00;
    mDataBuf[4] = 0x00;
    mDataBuf[5] = cmd;
    mDataBuf[6] = 0x88;
    mDataBuf[7] = 0x88;
    mDataBuf[8] = 0x88;
    mDataBuf[9] = 0x88;

    if (mNewBaudRate != 0)
        memcpy (mDataBuf + 10, &mNewBaudRate, 4);

    AddCRCToDataBuf(mDataBuf, cmdLen);

    if (mCfbcIns->sendDataOneway(COMM_DEV_SERIAL, mDataBuf, cmdLen + 4, 0)
        <= 0) {
        LOGD ("sendUpgradeCmd failure!!!\n");
        ret_code = ERR_SERIAL_CONNECT;
        ret = false;
    }

    return ret;
}

bool CFbcUpgrade::checkUpgradeConfigure(int &ret_code)
{
    bool ret = true;

    if (mUpgradeBlockSize != 0x10000) {
        ret_code = ERR_NOT_CORRECT_UPGRADE_BLKSIZE;
        ret = false;
        LOGD("%s() - blockSize:%d not support\n", __func__, mUpgradeBlockSize);
    }

    if (mUpgradeMode < 0) {
        ret_code = ERR_NOT_SUPPORT_UPGRADE_MDOE;
        ret = false;
        LOGD("%s() - upgradeMode:%d not support\n", __func__, mUpgradeMode);
    }

    return ret;
}

bool CFbcUpgrade::check_partition(unsigned char *fileBuff, partition_info_t *info)
{
    unsigned int crc = 0;
    int crc_flag = 0;
    bool ret = true;

    if (info) {
        if (info->code_offset && info->code_size) {
            crc = mCfbcIns->Calcrc32(crc, fileBuff + info->code_offset, info->code_size);
            crc_flag = 1;
        }

        if (info->data_offset && info->data_size) {
            crc = mCfbcIns->Calcrc32(crc, fileBuff + info->data_offset, info->data_size);
            crc_flag = 1;
        }

        if (0 == crc_flag || crc != info->crc)
            ret = false;
    }

    return ret;
}


bool CFbcUpgrade::loadUpgradeFile(int &ret_code,
                                  std::list<sectionInfo> &partitionList)
{
    struct stat tmp_st;
    bool ret = true;

    int file_handle = open(mFileName, O_RDONLY);

    if (file_handle < 0) {
        LOGE("%s, Can't Open file %s\n", __FUNCTION__, mFileName);
        ret_code = ERR_OPEN_BIN_FILE;
        ret = false;
    }

    if (ret && (-1 == stat(mFileName, &tmp_st))) {
        ret_code = ERR_OPEN_BIN_FILE;
        ret = false;
    }

    if (ret && tmp_st.st_size != 0x200000 && tmp_st.st_size != 0x400000) {
        LOGE("%s, don't support %d size upgrade binary!\n", __FUNCTION__,
             tmp_st.st_size);
        ret_code = ERR_BIN_FILE_SIZE;
        ret = false;
    }

    if (ret) {
        mBinFileSize = tmp_st.st_size;
        lseek(file_handle, 0, SEEK_SET);

        mBinFileBuf = new unsigned char[mBinFileSize];

        if (mBinFileBuf) {
            memset(mBinFileBuf, 0, mBinFileSize);
            int rw_size = read(file_handle, mBinFileBuf, mBinFileSize);

            if (rw_size != mBinFileSize || rw_size <= 0) {
                LOGE("%s, read file %s error(%d, %d)\n", __FUNCTION__,
                     mFileName, mBinFileSize, rw_size);
                ret = false;
                ret_code = ERR_READ_BIN_FILE;
            }
        } else {
            LOGE("%s, Malloc mBinFileBuf failure\n", __FUNCTION__);
            ret = false;
            ret_code = ERR_OPEN_BIN_FILE;
        }

        close(file_handle);
    }

    if (ret && !initBlocksInfo(partitionList)) {
        ret_code = ERR_OPEN_BIN_FILE;
        ret = false;
    }

    if (ret)  {
        partition_info_t *info = getPartitionInfoUpgFile(SECTION_0, PARTITION_FIRST_BOOT);

        if (!check_partition(mBinFileBuf, info)) {
            ret_code = ERR_DATA_CRC_ERROR;
            ret = false;
        }
    }

    return ret;
}

bool CFbcUpgrade::AdjustUpgradeSpeed()
{
    bool ret = true;

    LOGD ("%s:%d Use baudRate:%d\n", __func__, __LINE__, mNewBaudRate);

    if (mNewBaudRate) {
        mCfbcIns = GetFbcProtocolInstance();

        if (mCfbcIns != NULL) {
            ResetFbcProtocolInstance();

            mCfbcIns = GetFbcProtocolInstance(mNewBaudRate);

            if (mCfbcIns != NULL)
                mCfbcIns->SetUpgradeFlag(1);
            else
                ret = false;
        }
        else {
            ret = false;
        }
    }

    return ret;
}

bool CFbcUpgrade::threadLoop()
{
    unsigned char tmp_buf[128] = { 0 };
    std::list<sectionInfo> partitionList;
    int start = 0, length = 0, rate = 0;
    int currentWriteLength = 0, allPartitionWriteLength = 0;
    int count = 0, reSendTimes = 0, ret_count = 0;
    int ret_code = 0, cmd_len = 0;
    bool ret = true, prepare_success = false;

    LOGD("%s, entering...\n", "TV");
    prctl(PR_SET_NAME, (unsigned long) "CFbcUpgrade thread loop");

    if (NULL == mpObserver)
        ret = false;

    if (ret && !checkUpgradeConfigure(ret_code))
        ret = false;

    if (ret && !loadUpgradeFile(ret_code, partitionList))
        ret = false;

    if (ret && !sendUpgradeCmd(ret_code))
        ret = false;

    LOGD("%s(), line:%d, upgradeFileName:%s, blkSize:%d\n", __func__, __LINE__,
         mFileName, mUpgradeBlockSize);

    if (mNewBaudRate != 0) {
        if (ret && !AdjustUpgradeSpeed()) {
            LOGD ("AdjustUpgradeSpeed failure\n");
            ret_code = ERR_SERIAL_CONNECT;
            ret = false;
        }
    }

    if (ret) {
        prepare_success = true;
        usleep(delayAfterRebootUs);

        for (std::list<sectionInfo>::iterator itor = partitionList.begin();
             !exitPending() && itor != partitionList.end(); *itor++) {

            mState = STATE_RUNNING;
            currentWriteLength = 0;

            start = static_cast<sectionInfo>(*itor).start;
            length = static_cast<sectionInfo>(*itor).length;

            //TODO - Avoid too old fbc fireware to upgrade successful.
            if (0x49030 == start) {
                start = 0x49000;
                length += 0x30;
            }

            LOGD("start:%08x, length:%08x\n", start, length);

            while (!exitPending() && (currentWriteLength < length)) {

                count = (mUpgradeBlockSize <= (length - currentWriteLength) ?
                             mUpgradeBlockSize : length - currentWriteLength);

                memcpy(mDataBuf, mBinFileBuf + start, count);
                sprintf((char *) tmp_buf, "upgrade 0x%x 0x%x\n", start, count);

                cmd_len = strlen((char *) tmp_buf);
                LOGD("%s, line:%d, cmd:%s. cmd_len:%d\n", __func__, __LINE__, (char*)tmp_buf, cmd_len);
                if (ret
                    && (mCfbcIns->sendDataOneway(COMM_DEV_SERIAL, tmp_buf, cmd_len, 0) <= 0)) {
                    ret = false;
                    LOGD("Send cmd data error\n");
                }
                else {
                    usleep(200 * 1000);
                }

                if (ret
                    && (mCfbcIns->sendDataOneway(COMM_DEV_SERIAL, mDataBuf, count, 0) <= 0)) {
                    ret = false;
                    LOGD("Send %x upgrade data %02x,%02x,%02x ... error\n", count, mDataBuf[0], mDataBuf[1], mDataBuf[2]);
                } else {
                    AddCRCToDataBuf(mDataBuf, count);
                    usleep(500 * 1000);
                }

                if (ret
                    && (mCfbcIns->sendDataOneway(COMM_DEV_SERIAL, mDataBuf + count, 4, 0) <= 0)) {
                    ret = false;
                    LOGD("Send crc data error\n");
                }

                if (ret) {
                    int tmp_flag = 0;
                    memset(mDataBuf, 0, CC_UPGRADE_DATA_BUF_SIZE);

                    ret_count = mCfbcIns->uartReadStream(mDataBuf,
                                                         CC_UPGRADE_DATA_BUF_SIZE, 2000);

                    for (int i = 0; i < ret_count - 3; i++) {
                        if ((0x5A == mDataBuf[i]) && (0x5A == mDataBuf[i + 1])
                            && (0x5A == mDataBuf[i + 2])) {
                            LOGD("%s, fbc write data at 0x%x ok!\n",
                                 __FUNCTION__, start);
                            tmp_flag = 1;
                            break;
                        }
                    }

                    if (0 == tmp_flag) {
                        reSendTimes++;

                        mpObserver->onUpgradeStatus(mState, ERR_SERIAL_CONNECT);
                        LOGE("%s, fbc write data at 0x%x error! rewrite!\n",
                             __FUNCTION__, start);

                        if (reSendTimes > 3) {
                            mState = STATE_ABORT;
                            ret_code = ERR_SERIAL_CONNECT;
                            requestExit();
                            LOGE("%s, we have rewrite more than %d times, abort.\n",
                                 __FUNCTION__, 3);
                            break;
                        }
                    } else {
                        reSendTimes = 0;
                        currentWriteLength += count;
                        allPartitionWriteLength += count;
                        start += count;
                        rate = (++allPartitionWriteLength) * 100 / mOPTotalSize;
                    }

                    usleep(100 * 1000);
                } else {
                    mState = STATE_ABORT;
                    ret_code = ERR_SERIAL_CONNECT;
                    requestExit();
                }
            }
        }
    }

    LOGD("ret_code:%d\n", ret_code);

    if (mBinFileBuf != NULL) {
        delete mBinFileBuf;
        mBinFileBuf = NULL;
    }

    LOGD("%s, exiting...\n", "TV");

    //Avoid upgrade not start, but get this instance to upgrade again.
    mState = STATE_STOPED;
    mCfbcIns->SetUpgradeFlag(0);
    mpObserver->onUpgradeStatus(mState, ret_code);

    if (prepare_success) {
        sprintf((char *) tmp_buf, "reboot\n");
        cmd_len = strlen((char *) tmp_buf);

		if (ret_code != ERR_SERIAL_CONNECT)
            mCfbcIns->sendDataOneway(COMM_DEV_SERIAL, tmp_buf, cmd_len, 0);

        usleep(500 * 1000);
        mState = STATE_FINISHED;

        system("reboot");
        LOGD ("Reboot system...\n");
    }

    return false;
}
