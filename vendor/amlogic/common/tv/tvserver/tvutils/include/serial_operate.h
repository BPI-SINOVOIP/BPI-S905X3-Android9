/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef __SERIAL_OPERATE_H__
#define __SERIAL_OPERATE_H__
#include <utils/Thread.h>

using namespace android;
class CTv2d4GHeadSetDetect: public Thread {

public:
    CTv2d4GHeadSetDetect();
    ~CTv2d4GHeadSetDetect();

    int startDetect();

    class IHeadSetObserver {
    public:
        IHeadSetObserver() {};
        virtual ~IHeadSetObserver() {};
        virtual void onHeadSetDetect(int state __unused, int para __unused) {
        };
        virtual void onThermalDetect(int state __unused) {
        };
    };

    void setObserver ( IHeadSetObserver *pOb ) {
        mpObserver = pOb;
    };

private:
    bool threadLoop();
    IHeadSetObserver *mpObserver;

};

#endif//__SERIAL_OPERATE_H__
