#include <android-base/logging.h>
#include <android/security/IKeystoreService.h>
#include <binder/IServiceManager.h>
#include <private/android_filesystem_config.h>

#include <vector>
#include "include/wifikeystorehal/keystore.h"

namespace android {
namespace system {
namespace wifi {
namespace keystore {
namespace V1_0 {
namespace implementation {

using security::IKeystoreService;
// Methods from ::android::hardware::wifi::keystore::V1_0::IKeystore follow.
Return<void> Keystore::getBlob(const hidl_string& key, getBlob_cb _hidl_cb) {
    sp<IKeystoreService> service = interface_cast<IKeystoreService>(
        defaultServiceManager()->getService(String16("android.security.keystore")));
    if (service == nullptr) {
        _hidl_cb(KeystoreStatusCode::ERROR_UNKNOWN, {});
        return Void();
    }
    ::std::vector<uint8_t> value;
    // Retrieve the blob as wifi user.
    auto ret = service->get(String16(key.c_str()), AID_WIFI, &value);
    if (!ret.isOk()) {
        _hidl_cb(KeystoreStatusCode::ERROR_UNKNOWN, {});
        return Void();
    }
    _hidl_cb(KeystoreStatusCode::SUCCESS, (hidl_vec<uint8_t>)value);
    return Void();
}

Return<void> Keystore::getPublicKey(const hidl_string& keyId, getPublicKey_cb _hidl_cb) {
    sp<IKeystoreService> service = interface_cast<IKeystoreService>(
        defaultServiceManager()->getService(String16("android.security.keystore")));
    if (service == nullptr) {
        _hidl_cb(KeystoreStatusCode::ERROR_UNKNOWN, {});
        return Void();
    }
    ::std::vector<uint8_t> pubkey;
    auto ret = service->get_pubkey(String16(keyId.c_str()), &pubkey);
    if (!ret.isOk()) {
        _hidl_cb(KeystoreStatusCode::ERROR_UNKNOWN, {});
        return Void();
    }
    _hidl_cb(KeystoreStatusCode::SUCCESS, (hidl_vec<uint8_t>)pubkey);
    return Void();
}

Return<void> Keystore::sign(const hidl_string& keyId, const hidl_vec<uint8_t>& dataToSign,
                            sign_cb _hidl_cb) {
    sp<IKeystoreService> service = interface_cast<IKeystoreService>(
        defaultServiceManager()->getService(String16("android.security.keystore")));
    if (service == nullptr) {
        _hidl_cb(KeystoreStatusCode::ERROR_UNKNOWN, {});
        return Void();
    }
    ::std::vector<uint8_t> signedData;

    auto ret = service->sign(String16(keyId.c_str()), dataToSign, &signedData);
    if (!ret.isOk()) {
        _hidl_cb(KeystoreStatusCode::ERROR_UNKNOWN, {});
        return Void();
    }
    _hidl_cb(KeystoreStatusCode::SUCCESS, (hidl_vec<uint8_t>)signedData);
    return Void();
}

IKeystore* HIDL_FETCH_IKeystore(const char* /* name */) {
    return new Keystore();
}
}  // namespace implementation
}  // namespace V1_0
}  // namespace keystore
}  // namespace wifi
}  // namespace system
}  // namespace android
