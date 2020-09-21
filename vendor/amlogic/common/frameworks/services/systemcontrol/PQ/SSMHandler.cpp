/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#define LOG_TAG "SystemControl"
#define LOG_TV_TAG "SSMHandler"

#include "SSMHandler.h"

android::Mutex SSMHandler::sLock;
SSMHandler* SSMHandler::mSSMHandler = NULL;

SSMHandler* SSMHandler::GetSingletonInstance()
{

    android::Mutex::Autolock _l(sLock);

    if (!mSSMHandler) {
        mSSMHandler = new SSMHandler();

        if (mSSMHandler && !mSSMHandler->Construct()) {
            delete mSSMHandler;
            mSSMHandler = NULL;
        }
    }

    return mSSMHandler;
}

SSMHandler::SSMHandler()
{
    unsigned int sum = 0;

    memset (&mSSMHeader_section1, 0, sizeof (SSMHeader_section1_t));

    for (unsigned int i = 1; i < gSSMHeader_section1.count; i++) {
        sum += gSSMHeader_section2[i-1].size;

        gSSMHeader_section2[i].addr = sum;
    }
}

SSMHandler::~SSMHandler()
{
    if (mFd > 0) {
        close(mFd);
        mFd = -1;
    }
}

bool SSMHandler::Construct()
{
    bool ret = true;

    mFd = open(SSM_HANDLER_PATH, O_RDWR | O_SYNC | O_CREAT, S_IRUSR | S_IWUSR);

    if (-1 == mFd) {
        ret = false;
        SYS_LOGD ("%s, Open %s failure\n", __FUNCTION__, SSM_HANDLER_PATH);
    }

    return ret;
}

SSM_status_t SSMHandler::SSMSection1Verify()
{
    SSM_status_t ret = SSM_HEADER_VALID;

    lseek (mFd, 0, SEEK_SET);
    ssize_t ssize = read(mFd, &mSSMHeader_section1, sizeof (SSMHeader_section1_t));

    if (ssize != sizeof (SSMHeader_section1_t) ||
        mSSMHeader_section1.magic != gSSMHeader_section1.magic ||
        mSSMHeader_section1.count != gSSMHeader_section1.count) {
        ret = SSM_HEADER_INVALID;
    }

    if (ret != SSM_HEADER_INVALID &&
        mSSMHeader_section1.version != gSSMHeader_section1.version) {
        ret = SSM_HEADER_STRUCT_CHANGE;
    }

    return ret;
}

SSM_status_t SSMHandler::SSMSection2Verify(SSM_status_t SSM_status)
{
    return SSM_status;
}

int SSMHandler::SSMSaveCurrentHeader(current_ssmheader_section2_t *header_cur)
{
    std::vector<SSMHeader_section2_t> vsection2;
    unsigned int size = 0;

    SYS_LOGD ("%s --- line:%d\n", __FUNCTION__, __LINE__);

    lseek (mFd, sizeof (SSMHeader_section1_t), SEEK_SET);

    for (unsigned int i = 0; i < gSSMHeader_section1.count; i++) {
        SSMHeader_section2_t temp;

        read (mFd, &temp, sizeof (SSMHeader_section2_t));
        vsection2.push_back(temp);
        size += temp.size;
    }
    header_cur->size = size;
    header_cur->header_section2_data = &vsection2[0];

    //if (header_cur->size < 0) {
    //    SYS_LOGE ("SSMSaveCurrentHeader error!\n");
    //    return -1;
    //}

    return 0;
}

bool SSMHandler::SSMRecreateHeader()
{
    bool ret = true;

    SYS_LOGD ("%s ---   ", __FUNCTION__);

    ftruncate(mFd, 0);
    lseek (mFd, 0, SEEK_SET);

    //cal Addr and write
    write (mFd, &gSSMHeader_section1, sizeof (SSMHeader_section1_t));
    write (mFd, gSSMHeader_section2, gSSMHeader_section1.count * sizeof (SSMHeader_section2_t));

    return ret;
}

unsigned int SSMHandler::SSMGetActualAddr(int id)
{
    return gSSMHeader_section2[id].addr;
}

unsigned int SSMHandler::SSMGetActualSize(int id)
{
    return gSSMHeader_section2[id].size;
}

SSM_status_t SSMHandler::SSMVerify()
{
    return  SSMSection2Verify(SSMSection1Verify());
}

SSMHandler& SSMHandler::operator = (const SSMHandler& obj)
{
    return const_cast<SSMHandler&>(obj);
}
