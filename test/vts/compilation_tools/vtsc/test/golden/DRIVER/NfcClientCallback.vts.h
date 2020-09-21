#ifndef __VTS_DRIVER__android_hardware_nfc_V1_0_INfcClientCallback__
#define __VTS_DRIVER__android_hardware_nfc_V1_0_INfcClientCallback__

#undef LOG_TAG
#define LOG_TAG "FuzzerExtended_android_hardware_nfc_V1_0_INfcClientCallback"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <utils/Log.h>

#include <driver_base/DriverBase.h>
#include <driver_base/DriverCallbackBase.h>

#include <VtsDriverCommUtil.h>

#include <android/hardware/nfc/1.0/INfcClientCallback.h>
#include <hidl/HidlSupport.h>
#include <android/hardware/nfc/1.0/types.h>
#include <android/hardware/nfc/1.0/types.vts.h>
#include <android/hidl/base/1.0/types.h>


using namespace android::hardware::nfc::V1_0;
namespace android {
namespace vts {

class Vts_android_hardware_nfc_V1_0_INfcClientCallback : public ::android::hardware::nfc::V1_0::INfcClientCallback, public DriverCallbackBase {
 public:
    Vts_android_hardware_nfc_V1_0_INfcClientCallback(const string& callback_socket_name)
        : callback_socket_name_(callback_socket_name) {};

    virtual ~Vts_android_hardware_nfc_V1_0_INfcClientCallback() = default;

    ::android::hardware::Return<void> sendEvent(
        ::android::hardware::nfc::V1_0::NfcEvent arg0,
        ::android::hardware::nfc::V1_0::NfcStatus arg1) override;

    ::android::hardware::Return<void> sendData(
        const ::android::hardware::hidl_vec<uint8_t>& arg0) override;


 private:
    string callback_socket_name_;
};

sp<::android::hardware::nfc::V1_0::INfcClientCallback> VtsFuzzerCreateVts_android_hardware_nfc_V1_0_INfcClientCallback(const string& callback_socket_name);

class FuzzerExtended_android_hardware_nfc_V1_0_INfcClientCallback : public DriverBase {
 public:
    FuzzerExtended_android_hardware_nfc_V1_0_INfcClientCallback() : DriverBase(HAL_HIDL), hw_binder_proxy_() {}

    explicit FuzzerExtended_android_hardware_nfc_V1_0_INfcClientCallback(::android::hardware::nfc::V1_0::INfcClientCallback* hw_binder_proxy) : DriverBase(HAL_HIDL), hw_binder_proxy_(hw_binder_proxy) {}
    uint64_t GetHidlInterfaceProxy() const {
        return reinterpret_cast<uintptr_t>(hw_binder_proxy_.get());
    }
 protected:
    bool Fuzz(FunctionSpecificationMessage* func_msg, void** result, const string& callback_socket_name);
    bool CallFunction(const FunctionSpecificationMessage& func_msg, const string& callback_socket_name, FunctionSpecificationMessage* result_msg);
    bool VerifyResults(const FunctionSpecificationMessage& expected_result, const FunctionSpecificationMessage& actual_result);
    bool GetAttribute(FunctionSpecificationMessage* func_msg, void** result);
    bool GetService(bool get_stub, const char* service_name);

 private:
    sp<::android::hardware::nfc::V1_0::INfcClientCallback> hw_binder_proxy_;
};


extern "C" {
extern android::vts::DriverBase* vts_func_4_android_hardware_nfc_V1_0_INfcClientCallback_();
extern android::vts::DriverBase* vts_func_4_android_hardware_nfc_V1_0_INfcClientCallback_with_arg(uint64_t hw_binder_proxy);
}
}  // namespace vts
}  // namespace android
#endif
