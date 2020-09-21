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

#ifndef ANDROID_HARDWARE_IBINDER_H
#define ANDROID_HARDWARE_IBINDER_H

#include <functional>

#include <utils/Errors.h>
#include <utils/RefBase.h>
#include <utils/String16.h>

// ---------------------------------------------------------------------------
namespace android {
namespace hardware {

class BHwBinder;
class BpHwBinder;
class IInterface;
class Parcel;

/**
 * Base class and low-level protocol for a remotable object.
 * You can derive from this class to create an object for which other
 * processes can hold references to it.  Communication between processes
 * (method calls, property get and set) is down through a low-level
 * protocol implemented on top of the transact() API.
 */
class IBinder : public virtual RefBase
{
public:
    using TransactCallback = std::function<void(Parcel&)>;

    enum {
        // Corresponds to TF_ONE_WAY -- an asynchronous call.
        FLAG_ONEWAY             = 0x00000001
    };

                          IBinder();

    virtual status_t        transact(   uint32_t code,
                                        const Parcel& data,
                                        Parcel* reply,
                                        uint32_t flags = 0,
                                        TransactCallback callback = nullptr) = 0;

    class DeathRecipient : public virtual RefBase
    {
    public:
        virtual void binderDied(const wp<IBinder>& who) = 0;
    };

    /**
     * Register the @a recipient for a notification if this binder
     * goes away.  If this binder object unexpectedly goes away
     * (typically because its hosting process has been killed),
     * then DeathRecipient::binderDied() will be called with a reference
     * to this.
     *
     * The @a cookie is optional -- if non-NULL, it should be a
     * memory address that you own (that is, you know it is unique).
     *
     * @note You will only receive death notifications for remote binders,
     * as local binders by definition can't die without you dying as well.
     * Trying to use this function on a local binder will result in an
     * INVALID_OPERATION code being returned and nothing happening.
     *
     * @note This link always holds a weak reference to its recipient.
     *
     * @note You will only receive a weak reference to the dead
     * binder.  You should not try to promote this to a strong reference.
     * (Nor should you need to, as there is nothing useful you can
     * directly do with it now that it has passed on.)
     */
    virtual status_t        linkToDeath(const sp<DeathRecipient>& recipient,
                                        void* cookie = NULL,
                                        uint32_t flags = 0) = 0;

    /**
     * Remove a previously registered death notification.
     * The @a recipient will no longer be called if this object
     * dies.  The @a cookie is optional.  If non-NULL, you can
     * supply a NULL @a recipient, and the recipient previously
     * added with that cookie will be unlinked.
     */
    virtual status_t        unlinkToDeath(  const wp<DeathRecipient>& recipient,
                                            void* cookie = NULL,
                                            uint32_t flags = 0,
                                            wp<DeathRecipient>* outRecipient = NULL) = 0;

    virtual bool            checkSubclass(const void* subclassID) const;

    typedef void (*object_cleanup_func)(const void* id, void* obj, void* cleanupCookie);

    virtual void            attachObject(   const void* objectID,
                                            void* object,
                                            void* cleanupCookie,
                                            object_cleanup_func func) = 0;
    virtual void*           findObject(const void* objectID) const = 0;
    virtual void            detachObject(const void* objectID) = 0;

    virtual BHwBinder*        localBinder();
    virtual BpHwBinder*       remoteBinder();

protected:
    virtual          ~IBinder();

private:
};

}; // namespace hardware
}; // namespace android

// ---------------------------------------------------------------------------

#endif // ANDROID_HARDWARE_IBINDER_H
