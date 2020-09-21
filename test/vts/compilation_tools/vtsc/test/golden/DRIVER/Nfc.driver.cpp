#include "android/hardware/nfc/1.0/Nfc.vts.h"
#include "vts_measurement.h"
#include <android-base/logging.h>
#include <android/hidl/allocator/1.0/IAllocator.h>
#include <fmq/MessageQueue.h>
#include <sys/stat.h>
#include <unistd.h>


using namespace android::hardware::nfc::V1_0;
namespace android {
namespace vts {
bool FuzzerExtended_android_hardware_nfc_V1_0_INfc::GetService(bool get_stub, const char* service_name) {
    static bool initialized = false;
    if (!initialized) {
        LOG(INFO) << "HIDL getService";
        if (service_name) {
          LOG(INFO) << "  - service name: " << service_name;
        }
        hw_binder_proxy_ = ::android::hardware::nfc::V1_0::INfc::getService(service_name, get_stub);
        if (hw_binder_proxy_ == nullptr) {
            LOG(ERROR) << "getService() returned a null pointer.";
            return false;
        }
        LOG(DEBUG) << "hw_binder_proxy_ = " << hw_binder_proxy_.get();
        initialized = true;
    }
    return true;
}


::android::hardware::Return<::android::hardware::nfc::V1_0::NfcStatus> Vts_android_hardware_nfc_V1_0_INfc::open(
    const sp<::android::hardware::nfc::V1_0::INfcClientCallback>& arg0 __attribute__((__unused__))) {
    LOG(INFO) << "open called";
    AndroidSystemCallbackRequestMessage callback_message;
    callback_message.set_id(GetCallbackID("open"));
    callback_message.set_name("Vts_android_hardware_nfc_V1_0_INfc::open");
    VariableSpecificationMessage* var_msg0 = callback_message.add_arg();
    var_msg0->set_type(TYPE_HIDL_CALLBACK);
    /* ERROR: TYPE_HIDL_CALLBACK is not supported yet. */
    RpcCallToAgent(callback_message, callback_socket_name_);
    return ::android::hardware::nfc::V1_0::NfcStatus();
}

::android::hardware::Return<uint32_t> Vts_android_hardware_nfc_V1_0_INfc::write(
    const ::android::hardware::hidl_vec<uint8_t>& arg0 __attribute__((__unused__))) {
    LOG(INFO) << "write called";
    AndroidSystemCallbackRequestMessage callback_message;
    callback_message.set_id(GetCallbackID("write"));
    callback_message.set_name("Vts_android_hardware_nfc_V1_0_INfc::write");
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
    return static_cast<uint32_t>(0);
}

::android::hardware::Return<::android::hardware::nfc::V1_0::NfcStatus> Vts_android_hardware_nfc_V1_0_INfc::coreInitialized(
    const ::android::hardware::hidl_vec<uint8_t>& arg0 __attribute__((__unused__))) {
    LOG(INFO) << "coreInitialized called";
    AndroidSystemCallbackRequestMessage callback_message;
    callback_message.set_id(GetCallbackID("coreInitialized"));
    callback_message.set_name("Vts_android_hardware_nfc_V1_0_INfc::coreInitialized");
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
    return ::android::hardware::nfc::V1_0::NfcStatus();
}

::android::hardware::Return<::android::hardware::nfc::V1_0::NfcStatus> Vts_android_hardware_nfc_V1_0_INfc::prediscover(
    ) {
    LOG(INFO) << "prediscover called";
    AndroidSystemCallbackRequestMessage callback_message;
    callback_message.set_id(GetCallbackID("prediscover"));
    callback_message.set_name("Vts_android_hardware_nfc_V1_0_INfc::prediscover");
    RpcCallToAgent(callback_message, callback_socket_name_);
    return ::android::hardware::nfc::V1_0::NfcStatus();
}

::android::hardware::Return<::android::hardware::nfc::V1_0::NfcStatus> Vts_android_hardware_nfc_V1_0_INfc::close(
    ) {
    LOG(INFO) << "close called";
    AndroidSystemCallbackRequestMessage callback_message;
    callback_message.set_id(GetCallbackID("close"));
    callback_message.set_name("Vts_android_hardware_nfc_V1_0_INfc::close");
    RpcCallToAgent(callback_message, callback_socket_name_);
    return ::android::hardware::nfc::V1_0::NfcStatus();
}

::android::hardware::Return<::android::hardware::nfc::V1_0::NfcStatus> Vts_android_hardware_nfc_V1_0_INfc::controlGranted(
    ) {
    LOG(INFO) << "controlGranted called";
    AndroidSystemCallbackRequestMessage callback_message;
    callback_message.set_id(GetCallbackID("controlGranted"));
    callback_message.set_name("Vts_android_hardware_nfc_V1_0_INfc::controlGranted");
    RpcCallToAgent(callback_message, callback_socket_name_);
    return ::android::hardware::nfc::V1_0::NfcStatus();
}

::android::hardware::Return<::android::hardware::nfc::V1_0::NfcStatus> Vts_android_hardware_nfc_V1_0_INfc::powerCycle(
    ) {
    LOG(INFO) << "powerCycle called";
    AndroidSystemCallbackRequestMessage callback_message;
    callback_message.set_id(GetCallbackID("powerCycle"));
    callback_message.set_name("Vts_android_hardware_nfc_V1_0_INfc::powerCycle");
    RpcCallToAgent(callback_message, callback_socket_name_);
    return ::android::hardware::nfc::V1_0::NfcStatus();
}

sp<::android::hardware::nfc::V1_0::INfc> VtsFuzzerCreateVts_android_hardware_nfc_V1_0_INfc(const string& callback_socket_name) {
    static sp<::android::hardware::nfc::V1_0::INfc> result;
    result = new Vts_android_hardware_nfc_V1_0_INfc(callback_socket_name);
    return result;
}

bool FuzzerExtended_android_hardware_nfc_V1_0_INfc::Fuzz(
    FunctionSpecificationMessage* /*func_msg*/,
    void** /*result*/, const string& /*callback_socket_name*/) {
    return true;
}
bool FuzzerExtended_android_hardware_nfc_V1_0_INfc::GetAttribute(
    FunctionSpecificationMessage* /*func_msg*/,
    void** /*result*/) {
    LOG(ERROR) << "attribute not found.";
    return false;
}
bool FuzzerExtended_android_hardware_nfc_V1_0_INfc::CallFunction(
    const FunctionSpecificationMessage& func_msg,
    const string& callback_socket_name __attribute__((__unused__)),
    FunctionSpecificationMessage* result_msg) {
    const char* func_name = func_msg.name().c_str();
    if (hw_binder_proxy_ == nullptr) {
        LOG(ERROR) << "hw_binder_proxy_ is null. ";
        return false;
    }
    if (!strcmp(func_name, "open")) {
        sp<::android::hardware::nfc::V1_0::INfcClientCallback> arg0;
        arg0 = VtsFuzzerCreateVts_android_hardware_nfc_V1_0_INfcClientCallback(callback_socket_name);
        static_cast<Vts_android_hardware_nfc_V1_0_INfcClientCallback*>(arg0.get())->Register(func_msg.arg(0));
        LOG(DEBUG) << "local_device = " << hw_binder_proxy_.get();
        ::android::hardware::nfc::V1_0::NfcStatus result0;
        result0 = hw_binder_proxy_->open(arg0);
        result_msg->set_name("open");
        VariableSpecificationMessage* result_val_0 = result_msg->add_return_type_hidl();
        result_val_0->set_type(TYPE_ENUM);
        SetResult__android__hardware__nfc__V1_0__NfcStatus(result_val_0, result0);
        return true;
    }
    if (!strcmp(func_name, "write")) {
        ::android::hardware::hidl_vec<uint8_t> arg0;
        arg0.resize(func_msg.arg(0).vector_value_size());
        for (int arg0_index = 0; arg0_index < func_msg.arg(0).vector_value_size(); arg0_index++) {
            arg0[arg0_index] = func_msg.arg(0).vector_value(arg0_index).scalar_value().uint8_t();
        }
        LOG(DEBUG) << "local_device = " << hw_binder_proxy_.get();
        uint32_t result0;
        result0 = hw_binder_proxy_->write(arg0);
        result_msg->set_name("write");
        VariableSpecificationMessage* result_val_0 = result_msg->add_return_type_hidl();
        result_val_0->set_type(TYPE_SCALAR);
        result_val_0->set_scalar_type("uint32_t");
        result_val_0->mutable_scalar_value()->set_uint32_t(result0);
        return true;
    }
    if (!strcmp(func_name, "coreInitialized")) {
        ::android::hardware::hidl_vec<uint8_t> arg0;
        arg0.resize(func_msg.arg(0).vector_value_size());
        for (int arg0_index = 0; arg0_index < func_msg.arg(0).vector_value_size(); arg0_index++) {
            arg0[arg0_index] = func_msg.arg(0).vector_value(arg0_index).scalar_value().uint8_t();
        }
        LOG(DEBUG) << "local_device = " << hw_binder_proxy_.get();
        ::android::hardware::nfc::V1_0::NfcStatus result0;
        result0 = hw_binder_proxy_->coreInitialized(arg0);
        result_msg->set_name("coreInitialized");
        VariableSpecificationMessage* result_val_0 = result_msg->add_return_type_hidl();
        result_val_0->set_type(TYPE_ENUM);
        SetResult__android__hardware__nfc__V1_0__NfcStatus(result_val_0, result0);
        return true;
    }
    if (!strcmp(func_name, "prediscover")) {
        LOG(DEBUG) << "local_device = " << hw_binder_proxy_.get();
        ::android::hardware::nfc::V1_0::NfcStatus result0;
        result0 = hw_binder_proxy_->prediscover();
        result_msg->set_name("prediscover");
        VariableSpecificationMessage* result_val_0 = result_msg->add_return_type_hidl();
        result_val_0->set_type(TYPE_ENUM);
        SetResult__android__hardware__nfc__V1_0__NfcStatus(result_val_0, result0);
        return true;
    }
    if (!strcmp(func_name, "close")) {
        LOG(DEBUG) << "local_device = " << hw_binder_proxy_.get();
        ::android::hardware::nfc::V1_0::NfcStatus result0;
        result0 = hw_binder_proxy_->close();
        result_msg->set_name("close");
        VariableSpecificationMessage* result_val_0 = result_msg->add_return_type_hidl();
        result_val_0->set_type(TYPE_ENUM);
        SetResult__android__hardware__nfc__V1_0__NfcStatus(result_val_0, result0);
        return true;
    }
    if (!strcmp(func_name, "controlGranted")) {
        LOG(DEBUG) << "local_device = " << hw_binder_proxy_.get();
        ::android::hardware::nfc::V1_0::NfcStatus result0;
        result0 = hw_binder_proxy_->controlGranted();
        result_msg->set_name("controlGranted");
        VariableSpecificationMessage* result_val_0 = result_msg->add_return_type_hidl();
        result_val_0->set_type(TYPE_ENUM);
        SetResult__android__hardware__nfc__V1_0__NfcStatus(result_val_0, result0);
        return true;
    }
    if (!strcmp(func_name, "powerCycle")) {
        LOG(DEBUG) << "local_device = " << hw_binder_proxy_.get();
        ::android::hardware::nfc::V1_0::NfcStatus result0;
        result0 = hw_binder_proxy_->powerCycle();
        result_msg->set_name("powerCycle");
        VariableSpecificationMessage* result_val_0 = result_msg->add_return_type_hidl();
        result_val_0->set_type(TYPE_ENUM);
        SetResult__android__hardware__nfc__V1_0__NfcStatus(result_val_0, result0);
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

bool FuzzerExtended_android_hardware_nfc_V1_0_INfc::VerifyResults(const FunctionSpecificationMessage& expected_result __attribute__((__unused__)),
    const FunctionSpecificationMessage& actual_result __attribute__((__unused__))) {
    if (!strcmp(actual_result.name().c_str(), "open")) {
        if (actual_result.return_type_hidl_size() != expected_result.return_type_hidl_size() ) { return false; }
        if(!Verify__android__hardware__nfc__V1_0__NfcStatus(expected_result.return_type_hidl(0), actual_result.return_type_hidl(0))) { return false; }
        return true;
    }
    if (!strcmp(actual_result.name().c_str(), "write")) {
        if (actual_result.return_type_hidl_size() != expected_result.return_type_hidl_size() ) { return false; }
        if (actual_result.return_type_hidl(0).scalar_value().uint32_t() != expected_result.return_type_hidl(0).scalar_value().uint32_t()) { return false; }
        return true;
    }
    if (!strcmp(actual_result.name().c_str(), "coreInitialized")) {
        if (actual_result.return_type_hidl_size() != expected_result.return_type_hidl_size() ) { return false; }
        if(!Verify__android__hardware__nfc__V1_0__NfcStatus(expected_result.return_type_hidl(0), actual_result.return_type_hidl(0))) { return false; }
        return true;
    }
    if (!strcmp(actual_result.name().c_str(), "prediscover")) {
        if (actual_result.return_type_hidl_size() != expected_result.return_type_hidl_size() ) { return false; }
        if(!Verify__android__hardware__nfc__V1_0__NfcStatus(expected_result.return_type_hidl(0), actual_result.return_type_hidl(0))) { return false; }
        return true;
    }
    if (!strcmp(actual_result.name().c_str(), "close")) {
        if (actual_result.return_type_hidl_size() != expected_result.return_type_hidl_size() ) { return false; }
        if(!Verify__android__hardware__nfc__V1_0__NfcStatus(expected_result.return_type_hidl(0), actual_result.return_type_hidl(0))) { return false; }
        return true;
    }
    if (!strcmp(actual_result.name().c_str(), "controlGranted")) {
        if (actual_result.return_type_hidl_size() != expected_result.return_type_hidl_size() ) { return false; }
        if(!Verify__android__hardware__nfc__V1_0__NfcStatus(expected_result.return_type_hidl(0), actual_result.return_type_hidl(0))) { return false; }
        return true;
    }
    if (!strcmp(actual_result.name().c_str(), "powerCycle")) {
        if (actual_result.return_type_hidl_size() != expected_result.return_type_hidl_size() ) { return false; }
        if(!Verify__android__hardware__nfc__V1_0__NfcStatus(expected_result.return_type_hidl(0), actual_result.return_type_hidl(0))) { return false; }
        return true;
    }
    return false;
}

extern "C" {
android::vts::DriverBase* vts_func_4_android_hardware_nfc_V1_0_INfc_() {
    return (android::vts::DriverBase*) new android::vts::FuzzerExtended_android_hardware_nfc_V1_0_INfc();
}

android::vts::DriverBase* vts_func_4_android_hardware_nfc_V1_0_INfc_with_arg(uint64_t hw_binder_proxy) {
    ::android::hardware::nfc::V1_0::INfc* arg = nullptr;
    if (hw_binder_proxy) {
        arg = reinterpret_cast<::android::hardware::nfc::V1_0::INfc*>(hw_binder_proxy);
    } else {
        LOG(INFO) << " Creating DriverBase with null proxy.";
    }
    android::vts::DriverBase* result =
        new android::vts::FuzzerExtended_android_hardware_nfc_V1_0_INfc(
            arg);
    if (arg != nullptr) {
        arg->decStrong(arg);
    }
    return result;
}

}
}  // namespace vts
}  // namespace android
