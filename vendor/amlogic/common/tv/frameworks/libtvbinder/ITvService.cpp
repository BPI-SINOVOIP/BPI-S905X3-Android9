/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: C++ file
 */

#include <stdint.h>
#include <sys/types.h>
#include <utils/Log.h>
#include <binder/Parcel.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>

#include "include/ITvService.h"

class BpTvService: public BpInterface<ITvService> {
public:
    BpTvService(const sp<IBinder> &impl)
        : BpInterface<ITvService>(impl)
    {
    }

    // connect to tv service
    virtual sp<ITv> connect(const sp<ITvClient> &tvClient)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ITvService::getInterfaceDescriptor());
        data.writeStrongBinder(IInterface::asBinder(tvClient));
        remote()->transact(BnTvService::CONNECT, data, &reply);
        return interface_cast<ITv>(reply.readStrongBinder());
    }
};

IMPLEMENT_META_INTERFACE(TvService, "android.amlogic.ITvService");

// ----------------------------------------------------------------------

status_t BnTvService::onTransact(
    uint32_t code, const Parcel &data, Parcel *reply, uint32_t flags)
{
    switch (code) {
    case CONNECT: {
        CHECK_INTERFACE(ITvService, data, reply);
        sp<ITvClient> tvClient = interface_cast<ITvClient>(data.readStrongBinder());
        sp<ITv> tv = connect(tvClient);
        reply->writeStrongBinder(IInterface::asBinder(tv));
        return NO_ERROR;
    }
    break;
    default:
        return BBinder::onTransact(code, data, reply, flags);
    }
}

