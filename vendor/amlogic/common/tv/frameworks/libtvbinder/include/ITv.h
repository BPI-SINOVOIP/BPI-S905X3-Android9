/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: Header file
 */

#ifndef ANDROID_AMLOGIC_ITV_H
#define ANDROID_AMLOGIC_ITV_H

#include <utils/RefBase.h>
#include <binder/IInterface.h>
#include <binder/Parcel.h>
#include <binder/IMemory.h>
#include <utils/String8.h>
#include "TvClient.h"

using namespace android;

class ITvClient;

class ITv: public IInterface {
public:
    DECLARE_META_INTERFACE(Tv);

    virtual void            disconnect() = 0;

    // connect new client with existing tv remote
    virtual status_t        connect(const sp<ITvClient> &client) = 0;

    // prevent other processes from using this ITv interface
    virtual status_t        lock() = 0;

    // allow other processes to use this ITv interface
    virtual status_t        unlock() = 0;

    virtual status_t        processCmd(const Parcel &p, Parcel *r) = 0;

    //share mem for subtitle bmp
    virtual status_t        createSubtitle(const sp<IMemory> &share_mem) = 0;
    //share mem for video/hdmi bmp
    virtual status_t        createVideoFrame(const sp<IMemory> &share_mem, int iSourceMode, int iCapVideoLayerOnly) = 0;

};

// ----------------------------------------------------------------------------

class BnTv: public BnInterface<ITv> {
public:
    virtual status_t    onTransact( uint32_t code,
                                    const Parcel &data,
                                    Parcel *reply,
                                    uint32_t flags = 0);
};

#endif
