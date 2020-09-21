/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef __SSM_HANDLER_H__
#define __SSM_HANDLER_H__

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string>
#include <utils/threads.h>
#include "SSMHeader.h"
#include <vector>
#include <memory>
#include "CPQLog.h"

#define SSM_HANDLER_PATH             "/mnt/vendor/param/pq/SSMHandler"

class SSMHandler
{
public:
    static SSMHandler* GetSingletonInstance();
    virtual ~SSMHandler();
    SSM_status_t SSMVerify(void);
    bool SSMRecreateHeader(void);
    unsigned int SSMGetActualAddr(int id);
    unsigned int SSMGetActualSize(int id);
	int SSMSaveCurrentHeader(current_ssmheader_section2_t *header_cur);
private:
    int mFd;
    static SSMHandler *mSSMHandler;
    SSMHeader_section1_t mSSMHeader_section1;
    static android::Mutex sLock;

    bool Construct();
    explicit SSMHandler();
    SSM_status_t SSMSection1Verify();
    SSM_status_t  SSMSection2Verify(SSM_status_t);
    SSMHandler& operator = (const SSMHandler&);
};


#endif
