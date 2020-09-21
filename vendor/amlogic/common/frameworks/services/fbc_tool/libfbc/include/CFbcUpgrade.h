/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef __TV_UPGRADE_FBC_H__
#define __TV_UPGRADE_FBC_H__

#include "CFbcCommand.h"
#include "CFbcProtocol.h"
#include <utils/Thread.h>
#include <list>
#include <string>

#define CC_UPGRADE_MAX_BLOCK_LEN                      (0x10000)
#define CC_UPGRADE_DATA_BUF_SIZE                      (CC_UPGRADE_MAX_BLOCK_LEN + 4)

using namespace android;
class CFbcUpgrade: public Thread {
public:
    CFbcUpgrade();
    ~CFbcUpgrade();

    int start();
    int stop();
    void SetUpgradeBaudRate(unsigned int baudRate);
    int SetUpgradeFileName(char *file_name);
    int SetUpgradeBlockSize(int block_size);
    int SetUpgradeMode(int mode);

    class IUpgradeFBCObserver {
    public:
        IUpgradeFBCObserver() {};
        virtual ~IUpgradeFBCObserver() {};
        virtual void onUpgradeStatus(int state, int param) = 0;
    };

    void setObserver(IUpgradeFBCObserver *pOb)
    {
        mpObserver = pOb;
    };


    enum
    {
        SECTION_0 = 0,
        SECTION_1,
        SECTION_NUM,
    };

    enum
    {
        PARTITION_FIRST_BOOT = 0,
        PARTITION_SECOND_BOOT,
        PARTITION_SUSPEND,
        PARTITION_UPDATE,
        PARTITION_MAIN,
        PARTITION_PQ,
        PARTITION_USER,
        //PARTITION_FACTORY,
        PARTITION_NUM,
    };

    struct partition_info_t
    {
    public:
        unsigned code_offset;
        unsigned code_size;
        unsigned data_offset;
        unsigned data_size;
        unsigned bss_offset;
        unsigned bss_size;
        unsigned readonly_offset;
        unsigned readonly_size;
        unsigned char signature[256];
        unsigned spi_code_offset;
        unsigned spi_code_size;
        unsigned audio_param_offset;
        unsigned audio_param_size;
        unsigned sys_param_offset;
        unsigned sys_param_size;
        unsigned crc;
        unsigned char sha[32];
    } ;

    class sectionInfo
    {
    public:
        int start;
        int length;
        std::string name;
        bool checkAble;

        sectionInfo(int start = 0, int length = 0, std::string name = "", bool checkAble = false)
        {
            this->start = start;
            this->length = length;
            this->name = name;
            this->checkAble = checkAble;
        }
    };

private:
    bool threadLoop();
    bool AdjustUpgradeSpeed();
    bool parseUpgradeInfo(const int section, const int partition, int &start, int &length);
    bool initBlocksInfo(std::list<sectionInfo> &mList);
    bool sendUpgradeCmd(int &ret_code);
    bool checkUpgradeConfigure(int &ret_code);
    bool loadUpgradeFile(int &ret_code, std::list<sectionInfo> &partitionList);
	bool check_partition(unsigned char *fileBuff, partition_info_t *info);
    int AddCRCToDataBuf(unsigned char data_buf[], int data_len);
    partition_info_t* getPartitionInfoUpgFile(int section, int partition);

    int mState;
    int mUpgradeMode;
    int mOPTotalSize;
    int mBinFileSize;
    int mUpgradeBlockSize;
    unsigned char *mBinFileBuf;
	unsigned int mNewBaudRate;
    char mFileName[256];
    unsigned char mDataBuf[CC_UPGRADE_DATA_BUF_SIZE];
    IUpgradeFBCObserver *mpObserver;
    CFbcProtocol *mCfbcIns;

    const int delayAfterRebootUs = 20000 * 1000;

    const int LAYOUT_VERSION_OFFSET = 0x40000;
    const int LAYOUT_VERSION_SIZE = 0x1000;
    const int KEY_OFFSET = 0x0;
    const int KEY_SIZE = 0x41000;
    const int PARTITION_INFO_SIZE = 0x200;
    const int FIRST_BOOT_INFO_OFFSET = 0x41000;
    const int FIRST_BOOT_INFO_SIZE = 0x1000;
    const int SECTION_INFO_SIZE = 0x1000;
    const int SECTION_0_INFO_OFFSET = 0x42000;
    const int SECTION_1_INFO_OFFSET = 0x43000;
    const int FIRST_BOOT_OFFSET = 0x44000;
    const int FIRST_BOOT_SIZE = 0x5000;
    const int SECTION_SIZE = 0xAD000;
    const int SECTION_0_OFFSET = 0x49000;
    const int SECTION_1_OFFSET = 0xF6000;

    enum UpgradeState {
        STATE_STOPED = 0,
        STATE_RUNNING,
        STATE_FINISHED,
        STATE_ABORT
    };

    enum FBCUpgradeErrorCode {
        ERR_SERIAL_CONNECT = -1,
        ERR_OPEN_BIN_FILE = -2,
        ERR_BIN_FILE_SIZE = -3,
        ERR_READ_BIN_FILE = -4,
        ERR_NOT_SUPPORT_UPGRADE_MDOE = -5,
        ERR_NOT_CORRECT_UPGRADE_BLKSIZE = -6,
        ERR_DATA_CRC_ERROR = -7,
    };
};

extern CFbcUpgrade *getFbcUpgradeInstance();
#endif  //__TV_UPGRADE_FBC_H__
