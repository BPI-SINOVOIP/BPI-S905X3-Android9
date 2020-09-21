#include "android/hardware/nfc/1.0/NfcClientCallback.vts.h"
#include "vts_measurement.h"
#include <android-base/logging.h>
#include <android/hidl/allocator/1.0/IAllocator.h>
#include <fmq/MessageQueue.h>
#include <sys/stat.h>
#include <unistd.h>


using namespace android::hardware::nfc::V1_0;
namespace android {
namespace vts {
bool FuzzerExtended_android_hardware_nfc_V1_0_INfcClientCallback::GetService(bool get_stub, const char* service_name) {
    static bool initialized = false;
    if (!initialized) {
        LOG(INFO) << "HIDL getService";
        if (service_name) {
          LOG(INFO) << "  - service name: " << service_name;
        }
        hw_binder_proxy_ = ::android::hardware::nfc::V1_0::INfcClientCallback::getService(service_name, get_stub);
        if (hw_binder_proxy_ == nullptr) {
            LOG(ERROR) << "getService() returned a null pointer.";
            return false;
        }
        LOG(DEBUG) << "hw_binder_proxy_ = " << hw_binder_proxy_.get();
        initialized = true;
    }
    return true;
}


::android::hardware::Return<void> Vts_android_hardware_nfc_V1_0_INfcClientCallback::sendEvent(
    ::android::hardware::nfc::V1_0::NfcEvent arg0 __attribute__((__unused__)),
    ::android::hardware::nfc::V1_0::NfcStatus arg1 __attribute__((__unused__))) {
    LOG(INFO) << "sendEvent called";
    AndroidSystemCallbackRequestMessage callback_message;
    callback_message.set_id(GetCallbackID("sendEvent"));
    callback_message.set_name("Vts_android_hardware_nfc_V1_0_INfcClientCallback::sendEvent");
    VariableSpecificationMessage* var_msg0 = callback_message.add_arg();
    var_msg0->set_type(TYPE_ENUM);
    SetResult__android__hardware__nfc__V1_0__NfcEvent(var_msg0, arg0);
    VariableSpecificationMessage* var_msg1 = callback_message.add_arg();
    var_msg1->set_type(TYPE_ENUM);
    SetResult__android__hardware__nfc__V1_0__NfcStatus(var_msg1, arg1);
    RpcCallToAgent(callback_message, callback_socket_name_);
    return ::android::hardware::Void();
}

::android::hardware::Return<void> Vts_android_hardware_nfc_V1_0_INfcClientCallback::sendData(
    const ::android::hardware::hidl_vec<uint8_t>& arg0 __attribute__((__unused__))) {
    LOG(INFO) << "sendData called";
    AndroidSystemCallbackRequestMessage callback_message;
    callback_message.set_id(GetCallbackID("sendData"));
    callback_message.set_name("Vts_android_hardware_nfc_V1_0_INfcClientCallback::sendData");
    VariableSpecificationMessage* var_msg0 = callback_message.add_arg();
    var_msg0->set_type(TYPE_VECTOR);
    var_msg0->set_vector_size(arg0.size());
    for (int i = 0; i < (int)arg0.size(); i++) {
        auto *var_msg0_vector_i = var_msg0->add_vector_value();
        var_msg0_vector_i->set_type(TYPE_SCALAR);
        var_msg0_vector_i->set_scalar_type("uint8_t");
        var_msg0_vector_i->mutable_scalar_value()->set_uint8_t(arg0[i]);
    }
    RpcCallToAgent(callback_message, callback_socket_name_);
    return ::android::hardware::Void();
}

sp<::android::hardware::nfc::V1_0::INfcClientCallback> VtsFuzzerCreateVts_android_hardware_nfc_V1_0_INfcClientCallback(const string& callback_socket_name) {
    static sp<::android::hardware::nfc::V1_0::INfcClientCallback> result;
    result = new Vts_android_hardware_nfc_V1_0_INfcClientCallback(callback_socket_name);
    return result;
}

bool FuzzerExtended_android_hardware_nfc_V1_0_INfcClientCallback::Fuzz(
    FunctionSpecificationMessage* /*func_msg*/,
    void** /*result*/, const string& /*callback_socket_name*/) {
    return true;
}
bool FuzzerExtended_android_hardware_nfc_V1_0_INfcClientCallback::GetAttribute(
    FunctionSpecificationMessage* /*func_msg*/,
    void** /*result*/) {
    LOG(ERROR) << "attribute not found.";
    return false;
}
bool FuzzerExtended_android_hardware_nfc_V1_0_INfcClientCallback::CallFunction(
    const FunctionSpecificationMessage& func_msg,
    const string& callback_socket_name __attribute__((__unused__)),
    FunctionSpecificationMessage* result_msg) {
    const char* func_name = func_msg.name().c_str();
    if (hw_binder_proxy_ == nullptr) {
        LOG(ERROR) << "hw_binder_proxy_ is null. ";
        return false;
    }
    if (!strcmp(func_name, "sendEvent")) {
        ::android::hardware::nfc::V1_0::NfcEvent arg0;
        arg0 = EnumValue__android__hardware__nfc__V1_0__NfcEvent(func_msg.arg(0).scalar_value());
        ::android::hardware::nfc::V1_0::NfcStatus arg1;
        arg1 = EnumValue__android__hardware__nfc__V1_0__NfcStatus(func_msg.arg(1).scalar_value());
        LOG(DEBUG) << "local_device = " << hw_binder_proxy_.get();
        hw_binder_proxy_->sendEvent(arg0, arg1);
        result_msg->set_name("sendEvent");
        return true;
    }
    if (!strcmp(func_name, "sendData")) {
        ::android::hardware::hidl_vec<uint8_t> arg0;
        arg0.resize(func_msg.arg(0).vector_value_size());
        for (int arg0_index = 0; arg0_index < func_msg.arg(0).vector_value_size(); arg0_index++) {
            arg0[arg0_index] = func_msg.arg(0).vector_value(arg0_index).scalar_value().uint8_t();
        }
        LOG(DEBUG) << "local_device = " << hw_binder_proxy_.get();
        hw_binder_proxy_->sendData(arg0);
        result_msg->set_name("sendData");
        return true;
    }
    if (!strcmp(func_name, "notifySyspropsChanged")) {
        LOG(INFO) << "Call notifySyspropsChanged";
        hw_binder_proxy_->notifySyspropsChanged();
        result_msg->set_name("notifySyspropsChanged");
        return true;
    }
    return false;
}

bool FuzzerExtended_android_hardware_nfc_V1_0_INfcClientCallback::VerifyResults(const FunctionSpecificationMessage& expected_result __attribute__((__unused__)),
    const FunctionSpecificationMessage& actual_result __attribute__((__unused__))) {
    if (!strcmp(actual_result.name().c_str(), "sendEvent")) {
        if (actual_result.return_type_hidl_size() != expected_result.return_type_hidl_size() ) { return false; }
        return true;
    }
    if (!strcmp(actual_result.name().c_str(), "sendData")) {
        if (actual_result.return_type_hidl_size() != expected_result.return_type_hidl_size() ) { return false; }
        return true;
    }
    return false;
}

extern "C" {
android::vts::DriverBase* vts_func_4_android_hardware_nfc_V1_0_INfcClientCallback_() {
    return (android::vts::DriverBase*) new android::vts::FuzzerExtended_android_hardware_nfc_V1_0_INfcClientCallback();
}

android::vts::DriverBase* vts_func_4_android_hardware_nfc_V1_0_INfcClientCallback_with_arg(uint64_t hw_binder_proxy) {
    ::android::hardware::nfc::V1_0::INfcClientCallback* arg = nullptr;
    if (hw_binder_proxy) {
        arg = reinterpret_cast<::android::hardware::nfc::V1_0::INfcClientCallback*>(hw_binder_proxy);
    } else {
        LOG(INFO) << " Creating DriverBase with null proxy.";
    }
    android::vts::DriverBase* result =
        new android::vts::FuzzerExtended_android_hardware_nfc_V1_0_INfcClientCallback(
            arg);
    if (arg != nullptr) {
        arg->decStrong(arg);
    }
    return result;
}

}
}  // namespace vts
}  // namespace android
