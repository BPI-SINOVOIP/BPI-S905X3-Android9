/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: C++ file
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "ITv"
#include <utils/Log.h>
#include <stdint.h>
#include <sys/types.h>
#include <binder/Parcel.h>
#include "include/ITv.h"

enum {
    DISCONNECT = IBinder::FIRST_CALL_TRANSACTION,
    START_PREVIEW,
    SEND_COMMAND,
    CONNECT,
    LOCK,
    UNLOCK,
    TV_CMD,
    TV_CREATE_SUBTITLE,
    TV_CREATE_VIDEO_FRAME,
};

class BpTv: public BpInterface<ITv> {
public:
    BpTv(const sp<IBinder> &impl)
        : BpInterface<ITv>(impl)
    {
    }

    // disconnect from tv service
    void disconnect()
    {
        Parcel data, reply;
        data.writeInterfaceToken(ITv::getInterfaceDescriptor());
        if (remote()->transact(DISCONNECT, data, &reply) != NO_ERROR) {
            ALOGV("%s failed!\n", __FUNCTION__);
        }
    }

    status_t processCmd(const Parcel &p, Parcel *r)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ITv::getInterfaceDescriptor());
        data.write(p.data(), p.dataSize());
        if (remote()->transact(TV_CMD, data, &reply) == NO_ERROR) {
            r->write(reply.data(), reply.dataSize());
            r->setDataPosition(0);
        } else {
            ALOGV("%s failed!\n", __FUNCTION__);
        }
        return 0;
    }

    virtual status_t createVideoFrame(const sp<IMemory> &share_mem, int iSourceMode, int iCapVideoLayerOnly)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ITv::getInterfaceDescriptor());
        data.writeStrongBinder(IInterface::asBinder(share_mem));
        data.writeInt32(iSourceMode);
        data.writeInt32(iCapVideoLayerOnly);
        if (remote()->transact(TV_CREATE_VIDEO_FRAME, data, &reply) == NO_ERROR) {
            return reply.readInt32();
        } else {
            ALOGV("%s failed!\n", __FUNCTION__);
            return 0;
        }
    }

    virtual status_t createSubtitle(const sp<IMemory> &share_mem)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ITv::getInterfaceDescriptor());
        data.writeStrongBinder(IInterface::asBinder(share_mem));
        if (remote()->transact(TV_CREATE_SUBTITLE, data, &reply) == NO_ERROR) {
            return reply.readInt32();
        } else {
            ALOGV("%s failed!\n", __FUNCTION__);
            return 0;
        }
    }
    virtual status_t connect(const sp<ITvClient> &tvClient)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ITv::getInterfaceDescriptor());
        data.writeStrongBinder(IInterface::asBinder(tvClient));
        if (remote()->transact(CONNECT, data, &reply) == NO_ERROR) {
            return reply.readInt32();
        } else {
            ALOGV("%s failed!\n", __FUNCTION__);
            return 0;
        }
    }
    virtual status_t lock()
    {
        Parcel data, reply;
        data.writeInterfaceToken(ITv::getInterfaceDescriptor());
        remote()->transact(LOCK, data, &reply);
        return reply.readInt32();
    }
    virtual status_t unlock()
    {
        Parcel data, reply;
        data.writeInterfaceToken(ITv::getInterfaceDescriptor());
        remote()->transact(UNLOCK, data, &reply);
        return reply.readInt32();
    }
};

IMPLEMENT_META_INTERFACE(Tv, "android.amlogic.ITv");

status_t BnTv::onTransact(
    uint32_t code, const Parcel &data, Parcel *reply, uint32_t flags)
{
    switch (code) {
    case DISCONNECT: {
        CHECK_INTERFACE(ITv, data, reply);
        disconnect();
        return NO_ERROR;
    }
    break;
    case CONNECT: {
        CHECK_INTERFACE(ITv, data, reply);
        sp<ITvClient> tvClient = interface_cast<ITvClient>(data.readStrongBinder());
        reply->writeInt32(connect(tvClient));
        return NO_ERROR;
    }
    break;
    case LOCK: {
        CHECK_INTERFACE(ITv, data, reply);
        reply->writeInt32(lock());
        return NO_ERROR;
    }
    break;
    case UNLOCK: {
        CHECK_INTERFACE(ITv, data, reply);
        reply->writeInt32(unlock());
        return NO_ERROR;
    }
    break;
    case TV_CMD: {
        CHECK_INTERFACE(ITv, data, reply);
        processCmd(data, reply);
        //reply->write(tmp.data(), tmp.dataSize());
        return NO_ERROR;
    }
    case TV_CREATE_SUBTITLE: {
        CHECK_INTERFACE(ITv, data, reply);
        sp<IMemory> buffer = interface_cast<IMemory>(data.readStrongBinder());
        createSubtitle(buffer);
        //reply->write(tmp.data(), tmp.dataSize());
        return NO_ERROR;
    }
    case TV_CREATE_VIDEO_FRAME: {
        CHECK_INTERFACE(ITv, data, reply);
        sp<IMemory> buffer = interface_cast<IMemory>(data.readStrongBinder());
        int srcMode = data.readInt32();
        int capVideoLayerOnly = data.readInt32();
        createVideoFrame(buffer, srcMode, capVideoLayerOnly);
        //reply->write(tmp.data(), tmp.dataSize());
        return NO_ERROR;
    }
    break;
    default:
        return BBinder::onTransact(code, data, reply, flags);
    }
}

