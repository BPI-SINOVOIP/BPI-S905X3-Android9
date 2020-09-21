/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: Header file
 */

#ifndef _ANDROID_TV_CLIENT_H_
#define _ANDROID_TV_CLIENT_H_

#include <utils/Timers.h>
#include "ITvClient.h"
#include <binder/MemoryHeapBase.h>
#include <binder/MemoryBase.h>
#include <utils/threads.h>

using namespace android;

class ITvService;
class ITv;

// ref-counted object for callbacks
class TvListener: virtual public RefBase {
public:
    virtual void notify(int32_t msgType, const Parcel &ext) = 0;
};

class TvClient : public BnTvClient, public IBinder::DeathRecipient {
public:
    // construct a tv client from an existing remote
    static sp<TvClient> create(const sp<ITv> &tv);
    static sp<TvClient> connect();
    ~TvClient();
    void        init();
    status_t    reconnect();
    void        disconnect();
    status_t    lock();
    status_t    unlock();

    status_t    getStatus()
    {
        return mStatus;
    }
    status_t    processCmd(const Parcel &p, Parcel *r);
    status_t    createSubtitle(const sp<IMemory> &share_mem);
    status_t    createVideoFrame(const sp<IMemory> &share_mem, int iSourceMode, int iCapVideoLayerOnly);
    void        setListener(const sp<TvListener> &listener);

    // ITvClient interface
    virtual void notifyCallback(int32_t msgType, const Parcel &p);

    sp<ITv> remote();

private:
    TvClient();
    TvClient(const TvClient &);
    TvClient &operator = (const TvClient);
    virtual void binderDied(const wp<IBinder> &who);

    class DeathNotifier: public IBinder::DeathRecipient {
    public:
        DeathNotifier() {}
        virtual void binderDied(const wp<IBinder> &who);
    };

    static sp<DeathNotifier> mDeathNotifier;

    // helper function to obtain tv service handle
    static const sp<ITvService> &getTvService();

    sp<ITv>         mTv;
    status_t        mStatus;

    sp<TvListener>  mListener;

    friend class DeathNotifier;

    static  Mutex           mLock;
    static  sp<ITvService>  mTvService;

    sp<MemoryHeapBase>         mBmpMemHeap;
    sp<MemoryBase>             mBmpMemBase;
};
#endif/*_ANDROID_TV_CLIENT_H_*/

