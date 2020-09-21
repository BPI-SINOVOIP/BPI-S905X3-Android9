/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: C++ file
 */

#define LOG_TAG "TvClient"
#include <utils/Log.h>
#include <utils/threads.h>
#include <signal.h>
#include <binder/IServiceManager.h>
#include <binder/IMemory.h>

#include "include/TvClient.h"
#include "include/ITvService.h"

// client singleton for tv service binder interface
Mutex TvClient::mLock;
sp<ITvService> TvClient::mTvService;
sp<TvClient::DeathNotifier> TvClient::mDeathNotifier;

// establish binder interface to tv service
const sp<ITvService> &TvClient::getTvService()
{
    Mutex::Autolock _l(mLock);
    if (mTvService.get() == 0) {
        sp<IServiceManager> sm = defaultServiceManager();
        sp<IBinder> binder;
        do {
            binder = sm->getService(String16("tvservice"));
            if (binder != 0)
                break;
            ALOGW("TvService not published, waiting...");
            usleep(500000); // 0.5 s
        } while (true);
        if (mDeathNotifier == NULL) {
            mDeathNotifier = new DeathNotifier();
        }
        binder->linkToDeath(mDeathNotifier);
        mTvService = interface_cast<ITvService>(binder);
    }
    ALOGE_IF(mTvService == 0, "no TvService!?");
    return mTvService;
}

TvClient::TvClient()
{
    init();
}

// construct a tv client from an existing tv remote
sp<TvClient> TvClient::create(const sp<ITv> &tv)
{
    ALOGD("create");
    if (tv == 0) {
        ALOGE("tv remote is a NULL pointer");
        return 0;
    }

    sp<TvClient> c = new TvClient();
    if (tv->connect(c) == NO_ERROR) {
        c->mStatus = NO_ERROR;
        c->mTv = tv;
        IInterface::asBinder(tv)->linkToDeath(c);
    }
    return c;
}

void TvClient::init()
{
    mStatus = UNKNOWN_ERROR;
}

TvClient::~TvClient()
{
    disconnect();
}

sp<TvClient> TvClient::connect()
{
    ALOGD("Tv::connect------------------------------------------");
    sp<TvClient> c = new TvClient();
    const sp<ITvService> &cs = getTvService();
    if (cs != 0) {
        c->mTv = cs->connect(c);
    }
    if (c->mTv != 0) {
        IInterface::asBinder(c->mTv)->linkToDeath(c);
        c->mStatus = NO_ERROR;
    } else {
        c.clear();
    }
    return c;
}

void TvClient::disconnect()
{
    ALOGD("disconnect");
    if (mTv != 0) {
        mTv->disconnect();
        IInterface::asBinder(mTv)->unlinkToDeath(this);
        mTv = 0;
    }
}

status_t TvClient::reconnect()
{
    ALOGD("reconnect");
    sp <ITv> c = mTv;
    if (c == 0) return NO_INIT;
    return c->connect(this);
}

sp<ITv> TvClient::remote()
{
    return mTv;
}

status_t TvClient::lock()
{
    sp <ITv> c = mTv;
    if (c == 0) return NO_INIT;
    return c->lock();
}

status_t TvClient::unlock()
{
    sp <ITv> c = mTv;
    if (c == 0) return NO_INIT;
    return c->unlock();
}

status_t TvClient::processCmd(const Parcel &p, Parcel *r)
{
    sp <ITv> c = mTv;
    if (c == 0) return NO_INIT;
    return c->processCmd(p, r);
}

status_t TvClient::createSubtitle(const sp<IMemory> &share_mem)
{
    sp <ITv> c = mTv;
    if (c == 0) return NO_INIT;
    return c->createSubtitle(share_mem);
}

status_t TvClient::createVideoFrame(const sp<IMemory> &share_mem, int iSourceMode, int iCapVideoLayerOnly)
{
    sp <ITv> c = mTv;
    if (c == 0) return NO_INIT;
    return c->createVideoFrame(share_mem, iSourceMode, iCapVideoLayerOnly);
}

void TvClient::setListener(const sp<TvListener> &listener)
{
    ALOGD("tv------------Tv::setListener");
    Mutex::Autolock _l(mLock);
    mListener = listener;
}

// callback from tv service
void TvClient::notifyCallback(int32_t msgType, const Parcel &p)
{
    int size = p.dataSize();
    int pos = p.dataPosition();
    p.setDataPosition(0);
    sp<TvListener> listener;
    {
        Mutex::Autolock _l(mLock);
        listener = mListener;
    }
    if (listener != NULL) {
        listener->notify(msgType, p);
    }
}

void TvClient::binderDied(const wp<IBinder> &who __unused)
{
    ALOGW("ITv died");
    //notifyCallback(1, 2, 0);
}

void TvClient::DeathNotifier::binderDied(const wp<IBinder> &who __unused)
{
    ALOGW("-----------------binderDied");
    Mutex::Autolock _l(TvClient::mLock);
    TvClient::mTvService.clear();
    ALOGW("kill myself");
    kill(getpid(), SIGKILL);
    ALOGW("----------------Tv server died!");
}

