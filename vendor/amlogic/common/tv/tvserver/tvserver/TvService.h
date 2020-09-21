/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef ANDROID_SERVERS_TV_TVSERVICE_H
#define ANDROID_SERVERS_TV_TVSERVICE_H

#include <ITvService.h>
//#include <include/Tv.h>
#include <utils/threads.h>
#include <utils/Vector.h>
#include <stdint.h>
#include <tv/CTv.h>
#include "tv/CTvScreenCapture.h"

using namespace android;

class TvService: public BnTvService , public CTv::TvIObserver, public CTvScreenCapture::TvIObserver {
public:
    class Client: public BnTv {
    public:
        Client(const sp<TvService> &tvService, const sp<ITvClient> &tvClient, pid_t clientPid, CTv *pTv);
        virtual ~Client();
        virtual void disconnect();
        virtual status_t connect(const sp<ITvClient> &client);
        virtual status_t lock();
        virtual status_t unlock();
        virtual status_t processCmd(const Parcel &p, Parcel *r);
        virtual status_t createSubtitle(const sp<IMemory> &share_mem);
        virtual status_t createVideoFrame(const sp<IMemory> &share_mem, int iSourceMode, int iCapVideoLayerOnly);

        // our client...
        const sp<ITvClient> &getTvClient() const
        {
            return mTvClient;
        }

        int notifyCallback(const int &msgtype, const Parcel &p);
        int getPid() {
            return mClientPid;
        }

    private:
        //friend class CTv;
        //friend class TvService;
        status_t checkPid();

        mutable Mutex mLock;
        sp<TvService> mTvService;
        sp<ITvClient> mTvClient;
        pid_t mClientPid;
        CTv *mpTv;
        bool mIsStartTv;
    };//end client

    static void instantiate();

    virtual sp<ITv> connect(const sp<ITvClient> &tvClient);
    virtual void onTvEvent(const CTvEv &ev);
    void removeClient(const sp<ITvClient> &tvClient);

    virtual status_t dump(int fd, const Vector<String16>& args);

    wp<Client> mpScannerClient;
    wp<Client> mpSubClient;
    Vector< wp<Client> > mClients;

private:
    TvService();
    virtual ~TvService();

    virtual void incUsers();
    virtual void decUsers();

    volatile int32_t mUsers;
    CTv *mpTv;
    CTvScreenCapture mCapVidFrame;
    mutable Mutex mServiceLock;
};

#endif
