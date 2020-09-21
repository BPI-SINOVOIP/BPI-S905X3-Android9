/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: Header file
 */

#ifndef ANDROID_AMLOGIC_ITVSERVICE_H
#define ANDROID_AMLOGIC_ITVSERVICE_H

#include <utils/RefBase.h>
#include <binder/IInterface.h>
#include <binder/Parcel.h>
#include "ITvClient.h"
#include "ITv.h"

using namespace android;


class ITvService : public IInterface {
public:
    enum {
        CONNECT = IBinder::FIRST_CALL_TRANSACTION,
    };

public:
    DECLARE_META_INTERFACE(TvService);

    virtual sp<ITv> connect(const sp<ITvClient> &tvClient) = 0;
};

class BnTvService: public BnInterface<ITvService> {
public:
    virtual status_t onTransact(uint32_t code,
                                const Parcel &data,
                                Parcel *reply,
                                uint32_t flags = 0);
};

#endif
