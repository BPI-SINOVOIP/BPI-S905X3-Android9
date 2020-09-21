/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: Header file
 */

#ifndef ANDROID_AMLOGIC_ITV_CLIENT_H
#define ANDROID_AMLOGIC_ITV_CLIENT_H

#include <utils/RefBase.h>
#include <binder/IInterface.h>
#include <binder/Parcel.h>
#include <binder/IMemory.h>
#include <utils/Timers.h>

using namespace android;

class ITvClient: public IInterface {
public:
    DECLARE_META_INTERFACE(TvClient);

    virtual void notifyCallback(int32_t msgType, const Parcel &p) = 0;
};


class BnTvClient: public BnInterface<ITvClient> {
public:
    virtual status_t onTransact(uint32_t code,
                                const Parcel &data,
                                Parcel *reply,
                                uint32_t flags = 0);
};

#endif
