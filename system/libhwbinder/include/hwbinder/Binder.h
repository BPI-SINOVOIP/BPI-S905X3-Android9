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

#ifndef ANDROID_HARDWARE_BINDER_H
#define ANDROID_HARDWARE_BINDER_H

#include <atomic>
#include <stdint.h>
#include <hwbinder/IBinder.h>

// ---------------------------------------------------------------------------
namespace android {
namespace hardware {

class BHwBinder : public IBinder
{
public:
                        BHwBinder();

    virtual status_t    transact(   uint32_t code,
                                    const Parcel& data,
                                    Parcel* reply,
                                    uint32_t flags = 0,
                                    TransactCallback callback = nullptr);

    virtual status_t    linkToDeath(const sp<DeathRecipient>& recipient,
                                    void* cookie = NULL,
                                    uint32_t flags = 0);

    virtual status_t    unlinkToDeath(  const wp<DeathRecipient>& recipient,
                                        void* cookie = NULL,
                                        uint32_t flags = 0,
                                        wp<DeathRecipient>* outRecipient = NULL);

    virtual void        attachObject(   const void* objectID,
                                        void* object,
                                        void* cleanupCookie,
                                        object_cleanup_func func);
    virtual void*       findObject(const void* objectID) const;
    virtual void        detachObject(const void* objectID);

    virtual BHwBinder*    localBinder();

    int                 getMinSchedulingPolicy();
    int                 getMinSchedulingPriority();
protected:
    virtual             ~BHwBinder();

    virtual status_t    onTransact( uint32_t code,
                                    const Parcel& data,
                                    Parcel* reply,
                                    uint32_t flags = 0,
                                    TransactCallback callback = nullptr);
    int                 mSchedPolicy; // policy to run transaction from this node at
    // priority [-20..19] for SCHED_NORMAL, [1..99] for SCHED_FIFO/RT
    int                 mSchedPriority;
private:
                        BHwBinder(const BHwBinder& o);
            BHwBinder&    operator=(const BHwBinder& o);

    class Extras;

    std::atomic<Extras*> mExtras;
            void*       mReserved0;
};

// ---------------------------------------------------------------------------

class BpHwRefBase : public virtual RefBase
{
protected:
                            BpHwRefBase(const sp<IBinder>& o);
    virtual                 ~BpHwRefBase();
    virtual void            onFirstRef();
    virtual void            onLastStrongRef(const void* id);
    virtual bool            onIncStrongAttempted(uint32_t flags, const void* id);

    inline  IBinder*        remote() const          { return mRemote; }

private:
                            BpHwRefBase(const BpHwRefBase& o);
    BpHwRefBase&              operator=(const BpHwRefBase& o);

    IBinder* const          mRemote;
    RefBase::weakref_type*  mRefs;
    std::atomic<int32_t>    mState;
};

}; // namespace hardware
}; // namespace android

// ---------------------------------------------------------------------------

#endif // ANDROID_HARDWARE_BINDER_H
