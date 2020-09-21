/*
 * Copyright (C) 2012 The Android Open Source Project
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

#define LOG_NDEBUG 0
#define LOG_TAG "HDCP"
#include "HDCP.h"
#include <dlfcn.h>

namespace vendor {
namespace amlogic {
namespace hardware {
namespace miracast_hdcp2 {
namespace V1_0 {
namespace implementation {

HDCP::HDCP(bool createEncryptionModule)
    : mIsEncryptionModule(createEncryptionModule),
      mLibHandle(NULL),
      mHDCPModule(NULL),
      mObserver(NULL) {
    mLibHandle = dlopen("libstagefright_hdcp.so", RTLD_NOW);
    if (mLibHandle == NULL) {
        ALOGE("Unable to locate libstagefright_hdcp.so");
        return;
    }

    typedef HDCPModule *(*CreateHDCPModuleFunc)(
            void *, HDCPModule::ObserverFunc);

    CreateHDCPModuleFunc createHDCPModule =
        mIsEncryptionModule
            ? (CreateHDCPModuleFunc)dlsym(mLibHandle, "createHDCPModule")
            : (CreateHDCPModuleFunc)dlsym(
                    mLibHandle, "createHDCPModuleForDecryption");

    if (createHDCPModule == NULL) {
        ALOGE("Unable to find symbol 'createHDCPModule'.");
    } else if ((mHDCPModule = createHDCPModule(
                    this, &HDCP::ObserveWrapper)) == NULL) {
        ALOGE("createHDCPModule failed.");
    }
}

HDCP::~HDCP() {
    Mutex::Autolock autoLock(mLock);

    if (mHDCPModule != NULL) {
        delete mHDCPModule;
        mHDCPModule = NULL;
    }

    if (mLibHandle != NULL) {
        dlclose(mLibHandle);
        mLibHandle = NULL;
    }
}

Return<Status> HDCP::setObserver(const sp<IHDCPObserver> &observer) {
    Mutex::Autolock autoLock(mLock);

    if (mHDCPModule == NULL) {
        return Status::NO_INIT;
    }

    if (mObserver != NULL) {
        mObserver->unlinkToDeath(this);
    }

    if (observer)
        observer->linkToDeath(this, OBSERVER_COOKIE);

    mObserver = observer;

    return Status::OK;
}

Return<Status> HDCP::initAsync(const hidl_string& host, unsigned port) {
    const char *hostname = NULL;

    Mutex::Autolock autoLock(mLock);

    if (mHDCPModule == NULL) {
        return Status::NO_INIT;
    }

    if (host.size())
        hostname = host.c_str();

    return (Status)mHDCPModule->initAsync(hostname, port);
}

Return<Status> HDCP::shutdownAsync() {
    Mutex::Autolock autoLock(mLock);

    if (mHDCPModule == NULL) {
        return Status::NO_INIT;
    }

    return (Status)mHDCPModule->shutdownAsync();
}

Return<uint32_t> HDCP::getCaps() {
    Mutex::Autolock autoLock(mLock);

    if (mHDCPModule == NULL) {
        return 0;
    }

    return mHDCPModule->getCaps();
}

Return<void> HDCP::encrypt(
        const hidl_vec<uint8_t>& inData, uint32_t streamCTR,
        encrypt_cb _hidl_cb) {
    Status s = Status::OK;
    uint64_t outInputCTR = 0;
    hidl_vec<uint8_t> outData;
    uint32_t size = inData.size();

    Mutex::Autolock autoLock(mLock);

    assert(mIsEncryptionModule);

    if (mHDCPModule == NULL) {
        outInputCTR = 0;
        s = Status::NO_INIT;
    } else {
        outData.resize(size);
        s = (Status)mHDCPModule->encrypt(inData.data(), size, streamCTR, &outInputCTR, outData.data());
    }
    _hidl_cb(s, outInputCTR, outData);

    return Void();
}

Return<void> HDCP::decrypt(
        const hidl_vec<uint8_t>& inData,
        uint32_t streamCTR, uint64_t outInputCTR, uint32_t outAddr, decrypt_cb _hidl_cb) {
    Status s = Status::OK;
    uint32_t size = inData.size();
    hidl_vec<uint8_t> outData;

    Mutex::Autolock autoLock(mLock);

    assert(!mIsEncryptionModule);

    if (mHDCPModule == NULL) {
        s = Status::NO_INIT;
    } else {
        if (outAddr == 0) {
            outData.resize(size);
            s = (Status)mHDCPModule->decrypt(inData.data(), size, streamCTR, outInputCTR, outData.data());
        } else {
            s = (Status)mHDCPModule->decrypt(inData.data(), size, streamCTR, outInputCTR, (void *)outAddr);
        }
    }
    _hidl_cb(s, outData);

    return Void();
}
// static
void HDCP::ObserveWrapper(void *me, int msg, int ext1, int ext2) {
    static_cast<HDCP *>(me)->observe(msg, ext1, ext2);
}

void HDCP::observe(int msg, int ext1, int ext2) {
    Mutex::Autolock autoLock(mLock);

    if (mObserver != NULL) {
        if (!mObserver->notify(msg, ext1, ext2).isOk())
            ALOGE("Failed to send data to remote observer\n");
    }
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace miracast_hdcp2
}  // namespace hardware
}  // namespace amlogic
}  // namespace vendor

