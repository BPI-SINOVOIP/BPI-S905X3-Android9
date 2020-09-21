/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#if !defined(_CTVMANAGER_H_)
#define _CTVMANAGER_H_

#include <string.h>
#include <utils/Mutex.h>
#include <utils/Singleton.h>

#include "CTvPlayer.h"
#include "CTvRecord.h"


using namespace android;

template<typename T, int MAX>
class DevManager {
    private:
        T *mDevs[MAX];
        mutable Mutex mLock;

        static DevManager *mInstance;

    public:
        DevManager(){
            memset(mDevs, 0, MAX * sizeof(T*));
        }
        ~DevManager(){
            for (int i = 0; i < MAX; i++) {
                if (!mDevs[i]) {
                    mDevs[i]->stop(NULL);
                    delete mDevs[i];
                }
            }
        }
        T *getDev(const char *id) {
            AutoMutex _l( mLock );
            T *dev = NULL;
            for (int i = 0; i < MAX; i++)
                if (mDevs[i]
                    && mDevs[i]->getId()
                    && (strcmp(mDevs[i]->getId(), id) == 0))
                    return mDevs[i];
            return dev;
        }

        int addDev(T &dev) {
            AutoMutex _l( mLock );
            for (int i = 0; i < MAX; i++) {
                if (mDevs[i] && mDevs[i]->equals(dev))
                    return 1;
            }
            for (int i = 0; i < MAX; i++) {
                if (!mDevs[i]) {
                    mDevs[i] = &dev;
                    return 0;
                }
            }
            return -1;
        }

        void removeDev(const char *id) {
            AutoMutex _l( mLock );
            for (int i = 0; i < MAX; i++) {
                if (mDevs[i]
                    && mDevs[i]->getId()
                    && (strcmp(mDevs[i]->getId(), id) == 0)) {
                    delete mDevs[i];
                    mDevs[i] = NULL;
                }
            }
        }

        void releaseAll() {
            AutoMutex _l( mLock );
            for (int i = 0; i < MAX; i++) {
                if (mDevs[i]) {
                    mDevs[i]->stop(NULL);
                    delete mDevs[i];
                    mDevs[i] = NULL;
                }
            }
        }

        T *getFirstDevWithIdContains(const char *pattern) {
            AutoMutex _l( mLock );
            T *dev = NULL;
            for (int i = 0; i < MAX; i++)
                if (mDevs[i]
                    && mDevs[i]->getId()
                    && (strstr(mDevs[i]->getId(), pattern) != 0))
                    return mDevs[i];
            return dev;
        }

};

class CTvRecord;

class RecorderManager : public DevManager<CTvRecord, 2>, public Singleton<RecorderManager> {

    friend class Singleton<RecorderManager>;

    public:
        RecorderManager(){}
    ~RecorderManager(){}
};

class CTvPlayer;

class PlayerManager : public DevManager<CTvPlayer, 1>, public Singleton<PlayerManager> {

    friend class Singleton<PlayerManager>;

    public:
        PlayerManager(){}
    ~PlayerManager(){}

    static const bool singleMode = true;
};

#endif  //_CTVMANAGER_H_

