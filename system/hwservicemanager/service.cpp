#define LOG_TAG "hwservicemanager"

#include <utils/Log.h>

#include <inttypes.h>
#include <unistd.h>

#include <android/hidl/manager/1.1/BnHwServiceManager.h>
#include <android/hidl/token/1.0/ITokenManager.h>
#include <cutils/properties.h>
#include <hidl/Status.h>
#include <hwbinder/IPCThreadState.h>
#include <hwbinder/ProcessState.h>
#include <utils/Errors.h>
#include <utils/Looper.h>
#include <utils/StrongPointer.h>

#include "ServiceManager.h"
#include "TokenManager.h"

// libutils:
using android::sp;
using android::status_t;

// libhwbinder:
using android::hardware::IPCThreadState;
using android::hardware::ProcessState;

// libhidl
using android::hardware::configureRpcThreadpool;
using android::hardware::hidl_string;
using android::hardware::hidl_vec;
using android::hardware::joinRpcThreadpool;

// hidl types
using android::hidl::manager::V1_1::BnHwServiceManager;
using android::hidl::token::V1_0::ITokenManager;

// implementations
using android::hidl::manager::implementation::ServiceManager;
using android::hidl::token::V1_0::implementation::TokenManager;

static std::string serviceName = "default";

int main() {
    configureRpcThreadpool(1, true /* callerWillJoin */);

    ServiceManager *manager = new ServiceManager();

    if (!manager->add(serviceName, manager)) {
        ALOGE("Failed to register hwservicemanager with itself.");
    }

    TokenManager *tokenManager = new TokenManager();

    if (!manager->add(serviceName, tokenManager)) {
        ALOGE("Failed to register ITokenManager with hwservicemanager.");
    }

    // Tell IPCThreadState we're the service manager
    sp<BnHwServiceManager> service = new BnHwServiceManager(manager);
    IPCThreadState::self()->setTheContextObject(service);
    // Then tell the kernel
    ProcessState::self()->becomeContextManager(nullptr, nullptr);

    int rc = property_set("hwservicemanager.ready", "true");
    if (rc) {
        ALOGE("Failed to set \"hwservicemanager.ready\" (error %d). "\
              "HAL services will not start!\n", rc);
    }

    ALOGI("hwservicemanager is ready now.");
    joinRpcThreadpool();

    return 0;
}
