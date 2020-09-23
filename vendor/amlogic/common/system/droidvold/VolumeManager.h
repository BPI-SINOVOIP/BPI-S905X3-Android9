/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANDROID_DROIDVOLD_VOLUME_MANAGER_H
#define ANDROID_DROIDVOLD_VOLUME_MANAGER_H

#include <pthread.h>
#include <fnmatch.h>
#include <stdlib.h>

#include <list>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <cutils/multiuser.h>
#include <utils/List.h>
#include <utils/Timers.h>
#include <sysutils/NetlinkEvent.h>

#include "Disk.h"
#include "VolumeBase.h"
#include "DroidVold.h"

using namespace android;
using ::vendor::amlogic::hardware::droidvold::V1_0::implementation::DroidVold;

class VolumeManager {
public:
#ifdef HAS_VIRTUAL_CDROM
    char *mLoopPath;
#endif

private:
    static VolumeManager *sInstance;

    DroidVold        *mBroadcaster;

    bool                   mDebug;

public:
    virtual ~VolumeManager();
    // TODO: pipe all requests through VM to avoid exposing this lock
    std::mutex& getLock() { return mLock; }

    int start();
    int stop();

    void handleBlockEvent(NetlinkEvent *evt);

    class DiskSource {
    public:
        DiskSource(const std::string& sysPattern, const std::string& nickname, int flags) :
                mSysPattern(sysPattern), mNickname(nickname), mFlags(flags) {
        }

        bool matches(const std::string& sysPath) {
            return !fnmatch(mSysPattern.c_str(), sysPath.c_str(), 0);
        }

        const std::string& getNickname() { return mNickname; }
        int getFlags() { return mFlags; }

    private:
        std::string mSysPattern;
        std::string mNickname;
        int mFlags;
    };

    void addDiskSource(const std::shared_ptr<DiskSource>& diskSource);
    std::shared_ptr<android::droidvold::VolumeBase> findVolume(const std::string& id);
    void listVolumes(android::droidvold::VolumeBase::Type type, std::list<std::string>& list);

    /* Reset all internal state, typically during framework boot */
    int reset();
    /* Prepare for device shutdown, safely unmounting all devices */
    int shutdown();

    /* Unmount all volumes, usually for encryption */
    int unmountAll();

    /* for iso file mount and umount */
#ifdef HAS_VIRTUAL_CDROM
    int loopsetfd(const char * path);
    int loopclrfd();
    int mountloop(const char * path);
    int unmountloop(bool force);
    void unmountLoopIfNeed(const char *label);
#endif

    int setDebug(bool enable);
    bool getDebug() { return mDebug; }

    void setBroadcaster(DroidVold *sl) { mBroadcaster = sl; }
    DroidVold *getBroadcaster() { return mBroadcaster; }

    static VolumeManager *Instance();
    bool isMountpointMounted(const char *mp);
    int mkdirs(char* path);
    void coldboot(const char *path);
    int getDiskFlag(const std::string &path);

private:
    VolumeManager();

    std::mutex mLock;
    std::list<std::shared_ptr<DiskSource>> mDiskSources;
    std::list<std::shared_ptr<android::droidvold::Disk>> mDisks;
};

#endif
