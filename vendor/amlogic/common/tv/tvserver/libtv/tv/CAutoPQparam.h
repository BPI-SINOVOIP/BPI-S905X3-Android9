/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#if !defined(_CAUTOPQPARAM_H)
#define _CAUTOPQPARAM_H
#include "CAv.h"
#include <utils/Thread.h>
#include "../vpp/CVpp.h"
#include <tvconfig.h>

class CAutoPQparam: public Thread {
private:
    tv_source_input_t mAutoPQSource;
    bool mAutoPQ_OnOff_Flag;
    int preFmtType, curFmtType, autofreq_checkcount, autofreq_checkflag;
    int adjustPQparameters();
    bool  threadLoop();

public:

    CAutoPQparam();
    ~CAutoPQparam();
    void startAutoPQ( tv_source_input_t tv_source_input );
    void stopAutoPQ();
    bool isAutoPQing();
};
#endif
