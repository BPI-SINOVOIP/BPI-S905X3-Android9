/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: C++ file
 */

#define LOG_TAG "ITvClient"
#include <utils/Log.h>
#include <stdint.h>
#include <sys/types.h>
#include "include/ITvClient.h"
#include "include/tvcmd.h"
enum {
    NOTIFY_CALLBACK = IBinder::FIRST_CALL_TRANSACTION,
};

class BpTvClient: public BpInterface<ITvClient> {
public:
    BpTvClient(const sp<IBinder> &impl) :
        BpInterface<ITvClient> (impl)
    {
    }

    // generic callback from tv service to app
    void notifyCallback(int32_t msgType, const Parcel &p)
    {
        ALOGV("BpTvClient notifyCallback datasize = %d pos = %d", p.dataSize(), p.dataPosition());
        Parcel data, reply;
        data.writeInterfaceToken(ITvClient::getInterfaceDescriptor());
        data.writeInt32(msgType);
        data.write(p.data(), p.dataSize());
        remote()->transact(NOTIFY_CALLBACK, data, &reply, IBinder::FLAG_ONEWAY);
    }
};

IMPLEMENT_META_INTERFACE(TvClient, "android.amlogic.ITvClient");

// ----------------------------------------------------------------------
status_t BnTvClient::onTransact(uint32_t code, const Parcel &data, Parcel *reply, uint32_t flags)
{
    int i = 0, loop_count = 0;

    switch (code) {
    case NOTIFY_CALLBACK: {
        CHECK_INTERFACE(ITvClient, data, reply);
        Parcel ext;
        int32_t msgType = data.readInt32();
        ALOGV("BnTvClient::onTransact NOTIFY_CALLBACK msg type :%d", msgType);

        ext.appendFrom(const_cast<Parcel *>(&data), data.dataPosition(), data.dataAvail());
        notifyCallback(msgType, ext);
        return NO_ERROR;
    }
    break;
    default:
        return BBinder::onTransact(code, data, reply, flags);
    }
}
