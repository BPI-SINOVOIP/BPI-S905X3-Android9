/*
 * Copyright 2014 The Android Open Source Project
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

#include <binder/Parcel.h>

#include <gui/IProducerListener.h>

namespace android {

enum {
    ON_BUFFER_RELEASED = IBinder::FIRST_CALL_TRANSACTION,
    NEEDS_RELEASE_NOTIFY,
};

class BpProducerListener : public BpInterface<IProducerListener>
{
public:
    explicit BpProducerListener(const sp<IBinder>& impl)
        : BpInterface<IProducerListener>(impl) {}

    virtual ~BpProducerListener();

    virtual void onBufferReleased() {
        Parcel data, reply;
        data.writeInterfaceToken(IProducerListener::getInterfaceDescriptor());
        remote()->transact(ON_BUFFER_RELEASED, data, &reply, IBinder::FLAG_ONEWAY);
    }

    virtual bool needsReleaseNotify() {
        bool result;
        Parcel data, reply;
        data.writeInterfaceToken(IProducerListener::getInterfaceDescriptor());
        status_t err = remote()->transact(NEEDS_RELEASE_NOTIFY, data, &reply);
        if (err != NO_ERROR) {
            ALOGE("IProducerListener: binder call \'needsReleaseNotify\' failed");
            return true;
        }
        err = reply.readBool(&result);
        if (err != NO_ERROR) {
            ALOGE("IProducerListener: malformed binder reply");
            return true;
        }
        return result;
    }
};

// Out-of-line virtual method definition to trigger vtable emission in this
// translation unit (see clang warning -Wweak-vtables)
BpProducerListener::~BpProducerListener() {}

IMPLEMENT_META_INTERFACE(ProducerListener, "android.gui.IProducerListener")

status_t BnProducerListener::onTransact(uint32_t code, const Parcel& data,
        Parcel* reply, uint32_t flags) {
    switch (code) {
        case ON_BUFFER_RELEASED:
            CHECK_INTERFACE(IProducerListener, data, reply);
            onBufferReleased();
            return NO_ERROR;
        case NEEDS_RELEASE_NOTIFY:
            CHECK_INTERFACE(IProducerListener, data, reply);
            reply->writeBool(needsReleaseNotify());
            return NO_ERROR;
    }
    return BBinder::onTransact(code, data, reply, flags);
}

ProducerListener::~ProducerListener() = default;

DummyProducerListener::~DummyProducerListener() = default;

bool BnProducerListener::needsReleaseNotify() {
    return true;
}

} // namespace android
