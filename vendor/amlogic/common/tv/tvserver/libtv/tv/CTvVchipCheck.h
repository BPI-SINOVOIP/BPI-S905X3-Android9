/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#if !defined(_CTVVCHIPCHECK_H)
#define _CTVVCHIPCHECK_H
#include <utils/Vector.h>
#include "CTvDatabase.h"
#include <utils/String8.h>
#include <stdlib.h>
#include "CTvDimension.h"
#include "CTvProgram.h"
#include "CTvTime.h"
#include "CTvEvent.h"
#include "CTvLog.h"
#include <utils/Thread.h>
// TV ATSC rating dimension
class CTvVchipCheck: public Thread {
public:
    CTvVchipCheck();
    ~CTvVchipCheck();
    bool CheckProgramBlock(int id);
    static void *VchipCheckingThread ( void *arg );
    int startVChipCheck();
    int stopVChipCheck();
    int pauseVChipCheck();
    int resumeVChipCheck();
    int requestAndWaitPauseVChipCheck();
private:
    bool  threadLoop();
    mutable Mutex mLock;
    Condition mDetectPauseCondition;
    Condition mRequestPauseCondition;
    volatile bool m_request_pause_detect;
    enum DetectState {
        STATE_STOPED = 0,
        STATE_RUNNING,
        STATE_PAUSE
    };
    int mDetectState;
};
#endif  //_CTVDIMENSION_H
