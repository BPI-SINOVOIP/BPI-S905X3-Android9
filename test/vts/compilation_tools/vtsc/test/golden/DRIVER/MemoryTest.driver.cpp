#include "android/hardware/tests/memory/1.0/MemoryTest.vts.h"
#include "vts_measurement.h"
#include <android-base/logging.h>
#include <android/hidl/allocator/1.0/IAllocator.h>
#include <fmq/MessageQueue.h>
#include <sys/stat.h>
#include <unistd.h>


using namespace android::hardware::tests::memory::V1_0;
namespace android {
namespace vts {
bool FuzzerExtended_android_hardware_tests_memory_V1_0_IMemoryTest::GetService(bool get_stub, const char* service_name) {
    static bool initialized = false;
    if (!initialized) {
        LOG(INFO) << "HIDL getService";
        if (service_name) {
          LOG(INFO) << "  - service name: " << service_name;
        }
        hw_binder_proxy_ = ::android::hardware::tests::memory::V1_0::IMemoryTest::getService(service_name, get_stub);
        if (hw_binder_proxy_ == nullptr) {
            LOG(ERROR) << "getService() returned a null pointer.";
            return false;
        }
        LOG(DEBUG) << "hw_binder_proxy_ = " << hw_binder_proxy_.get();
        initialized = true;
    }
    return true;
}


::android::hardware::Return<void> Vts_android_hardware_tests_memory_V1_0_IMemoryTest::haveSomeMemory(
    const ::android::hardware::hidl_memory& arg0 __attribute__((__unused__)), std::function<void(const ::android::hardware::hidl_memory& arg0)> cb) {
    LOG(INFO) << "haveSomeMemory called";
    AndroidSystemCallbackRequestMessage callback_message;
    callback_message.set_id(GetCallbackID("haveSomeMemory"));
    callback_message.set_name("Vts_android_hardware_tests_memory_V1_0_IMemoryTest::haveSomeMemory");
    VariableSpecificationMessage* var_msg0 = callback_message.add_arg();
    var_msg0->set_type(TYPE_HIDL_MEMORY);
    /* ERROR: TYPE_HIDL_MEMORY is not supported yet. */
    RpcCallToAgent(callback_message, callback_socket_name_);
    cb(::android::hardware::hidl_memory());
    return ::android::hardware::Void();
}

::android::hardware::Return<void> Vts_android_hardware_tests_memory_V1_0_IMemoryTest::fillMemory(
    const ::android::hardware::hidl_memory& arg0 __attribute__((__unused__)),
    uint8_t arg1 __attribute__((__unused__))) {
    LOG(INFO) << "fillMemory called";
    AndroidSystemCallbackRequestMessage callback_message;
    callback_message.set_id(GetCallbackID("fillMemory"));
    callback_message.set_name("Vts_android_hardware_tests_memory_V1_0_IMemoryTest::fillMemory");
    VariableSpecificationMessage* var_msg0 = callback_message.add_arg();
    var_msg0->set_type(TYPE_HIDL_MEMORY);
    /* ERROR: TYPE_HIDL_MEMORY is not supported yet. */
    VariableSpecificationMessage* var_msg1 = callback_message.add_arg();
    var_msg1->set_type(TYPE_SCALAR);
    var_msg1->set_scalar_type("uint8_t");
    var_msg1->mutable_scalar_value()->set_uint8_t(arg1);
    RpcCallToAgent(callback_message, callback_socket_name_);
    return ::android::hardware::Void();
}

::android::hardware::Return<void> Vts_android_hardware_tests_memory_V1_0_IMemoryTest::haveSomeMemoryBlock(
    const ::android::hidl::memory::block::V1_0::MemoryBlock& arg0 __attribute__((__unused__)), std::function<void(const ::android::hidl::memory::block::V1_0::MemoryBlock& arg0)> cb) {
    LOG(INFO) << "haveSomeMemoryBlock called";
    AndroidSystemCallbackRequestMessage callback_message;
    callback_message.set_id(GetCallbackID("haveSomeMemoryBlock"));
    callback_message.set_name("Vts_android_hardware_tests_memory_V1_0_IMemoryTest::haveSomeMemoryBlock");
    VariableSpecificationMessage* var_msg0 = callback_message.add_arg();
    var_msg0->set_type(TYPE_STRUCT);
    SetResult__android__hidl__memory__block__V1_0__MemoryBlock(var_msg0, arg0);
    RpcCallToAgent(callback_message, callback_socket_name_);
    cb(::android::hidl::memory::block::V1_0::MemoryBlock());
    return ::android::hardware::Void();
}

::android::hardware::Return<void> Vts_android_hardware_tests_memory_V1_0_IMemoryTest::set(
    const ::android::hardware::hidl_memory& arg0 __attribute__((__unused__))) {
    LOG(INFO) << "set called";
    AndroidSystemCallbackRequestMessage callback_message;
    callback_message.set_id(GetCallbackID("set"));
    callback_message.set_name("Vts_android_hardware_tests_memory_V1_0_IMemoryTest::set");
    VariableSpecificationMessage* var_msg0 = callback_message.add_arg();
    var_msg0->set_type(TYPE_HIDL_MEMORY);
    /* ERROR: TYPE_HIDL_MEMORY is not supported yet. */
    RpcCallToAgent(callback_message, callback_socket_name_);
    return ::android::hardware::Void();
}

::android::hardware::Return<sp<::android::hidl::memory::token::V1_0::IMemoryToken>> Vts_android_hardware_tests_memory_V1_0_IMemoryTest::get(
    ) {
    LOG(INFO) << "get called";
    AndroidSystemCallbackRequestMessage callback_message;
    callback_message.set_id(GetCallbackID("get"));
    callback_message.set_name("Vts_android_hardware_tests_memory_V1_0_IMemoryTest::get");
    RpcCallToAgent(callback_message, callback_socket_name_);
    return nullptr;
}

sp<::android::hardware::tests::memory::V1_0::IMemoryTest> VtsFuzzerCreateVts_android_hardware_tests_memory_V1_0_IMemoryTest(const string& callback_socket_name) {
    static sp<::android::hardware::tests::memory::V1_0::IMemoryTest> result;
    result = new Vts_android_hardware_tests_memory_V1_0_IMemoryTest(callback_socket_name);
    return result;
}

bool FuzzerExtended_android_hardware_tests_memory_V1_0_IMemoryTest::Fuzz(
    FunctionSpecificationMessage* /*func_msg*/,
    void** /*result*/, const string& /*callback_socket_name*/) {
    return true;
}
bool FuzzerExtended_android_hardware_tests_memory_V1_0_IMemoryTest::GetAttribute(
    FunctionSpecificationMessage* /*func_msg*/,
    void** /*result*/) {
    LOG(ERROR) << "attribute not found.";
    return false;
}
bool FuzzerExtended_android_hardware_tests_memory_V1_0_IMemoryTest::CallFunction(
    const FunctionSpecificationMessage& func_msg,
    const string& callback_socket_name __attribute__((__unused__)),
    FunctionSpecificationMessage* result_msg) {
    const char* func_name = func_msg.name().c_str();
    if (hw_binder_proxy_ == nullptr) {
        LOG(ERROR) << "hw_binder_proxy_ is null. ";
        return false;
    }
    if (!strcmp(func_name, "haveSomeMemory")) {
        ::android::hardware::hidl_memory arg0;
        sp<::android::hidl::allocator::V1_0::IAllocator> ashmemAllocator = ::android::hidl::allocator::V1_0::IAllocator::getService("ashmem");
        if (ashmemAllocator == nullptr) {
            LOG(ERROR) << "Failed to get ashmemAllocator! ";
            exit(-1);
        }
        auto res = ashmemAllocator->allocate(func_msg.arg(0).hidl_memory_value().size(), [&](bool success, const hardware::hidl_memory& memory) {
            if (!success) {
                LOG(ERROR) << "Failed to allocate memory! ";
                arg0 = ::android::hardware::hidl_memory();
                return;
            }
            arg0 = memory;
        });
        LOG(DEBUG) << "local_device = " << hw_binder_proxy_.get();
        ::android::hardware::hidl_memory result0;
        hw_binder_proxy_->haveSomeMemory(arg0, [&](const ::android::hardware::hidl_memory& arg0){
            LOG(INFO) << "callback haveSomeMemory called";
            result0 = arg0;
        });
        result_msg->set_name("haveSomeMemory");
        VariableSpecificationMessage* result_val_0 = result_msg->add_return_type_hidl();
        result_val_0->set_type(TYPE_HIDL_MEMORY);
        /* ERROR: TYPE_HIDL_MEMORY is not supported yet. */
        return true;
    }
    if (!strcmp(func_name, "fillMemory")) {
        ::android::hardware::hidl_memory arg0;
        sp<::android::hidl::allocator::V1_0::IAllocator> ashmemAllocator = ::android::hidl::allocator::V1_0::IAllocator::getService("ashmem");
        if (ashmemAllocator == nullptr) {
            LOG(ERROR) << "Failed to get ashmemAllocator! ";
            exit(-1);
        }
        auto res = ashmemAllocator->allocate(func_msg.arg(0).hidl_memory_value().size(), [&](bool success, const hardware::hidl_memory& memory) {
            if (!success) {
                LOG(ERROR) << "Failed to allocate memory! ";
                arg0 = ::android::hardware::hidl_memory();
                return;
            }
            arg0 = memory;
        });
        uint8_t arg1 = 0;
        arg1 = func_msg.arg(1).scalar_value().uint8_t();
        LOG(DEBUG) << "local_device = " << hw_binder_proxy_.get();
        hw_binder_proxy_->fillMemory(arg0, arg1);
        result_msg->set_name("fillMemory");
        return true;
    }
    if (!strcmp(func_name, "haveSomeMemoryBlock")) {
        ::android::hidl::memory::block::V1_0::MemoryBlock arg0;
        MessageTo__android__hidl__memory__block__V1_0__MemoryBlock(func_msg.arg(0), &(arg0), callback_socket_name);
        LOG(DEBUG) << "local_device = " << hw_binder_proxy_.get();
        ::android::hidl::memory::block::V1_0::MemoryBlock result0;
        hw_binder_proxy_->haveSomeMemoryBlock(arg0, [&](const ::android::hidl::memory::block::V1_0::MemoryBlock& arg0){
            LOG(INFO) << "callback haveSomeMemoryBlock called";
            result0 = arg0;
        });
        result_msg->set_name("haveSomeMemoryBlock");
        VariableSpecificationMessage* result_val_0 = result_msg->add_return_type_hidl();
        result_val_0->set_type(TYPE_STRUCT);
        SetResult__android__hidl__memory__block__V1_0__MemoryBlock(result_val_0, result0);
        return true;
    }
    if (!strcmp(func_name, "set")) {
        ::android::hardware::hidl_memory arg0;
        sp<::android::hidl::allocator::V1_0::IAllocator> ashmemAllocator = ::android::hidl::allocator::V1_0::IAllocator::getService("ashmem");
        if (ashmemAllocator == nullptr) {
            LOG(ERROR) << "Failed to get ashmemAllocator! ";
            exit(-1);
        }
        auto res = ashmemAllocator->allocate(func_msg.arg(0).hidl_memory_value().size(), [&](bool success, const hardware::hidl_memory& memory) {
            if (!success) {
                LOG(ERROR) << "Failed to allocate memory! ";
                arg0 = ::android::hardware::hidl_memory();
                return;
            }
            arg0 = memory;
        });
        LOG(DEBUG) << "local_device = " << hw_binder_proxy_.get();
        hw_binder_proxy_->set(arg0);
        result_msg->set_name("set");
        return true;
    }
    if (!strcmp(func_name, "get")) {
        LOG(DEBUG) << "local_device = " << hw_binder_proxy_.get();
        sp<::android::hidl::memory::token::V1_0::IMemoryToken> result0;
        result0 = hw_binder_proxy_->get();
        result_msg->set_name("get");
        VariableSpecificationMessage* result_val_0 = result_msg->add_return_type_hidl();
        result_val_0->set_type(TYPE_HIDL_INTERFACE);
        result_val_0->set_predefined_type("::android::hidl::memory::token::V1_0::IMemoryToken");
        if (result0 != nullptr) {
            result0->incStrong(result0.get());
            result_val_0->set_hidl_interface_pointer(reinterpret_cast<uintptr_t>(result0.get()));
        } else {
            result_val_0->set_hidl_interface_pointer(0);
        }
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

bool FuzzerExtended_android_hardware_tests_memory_V1_0_IMemoryTest::VerifyResults(const FunctionSpecificationMessage& expected_result __attribute__((__unused__)),
    const FunctionSpecificationMessage& actual_result __attribute__((__unused__))) {
    if (!strcmp(actual_result.name().c_str(), "haveSomeMemory")) {
        if (actual_result.return_type_hidl_size() != expected_result.return_type_hidl_size() ) { return false; }
        /* ERROR: TYPE_HIDL_MEMORY is not supported yet. */
        return true;
    }
    if (!strcmp(actual_result.name().c_str(), "fillMemory")) {
        if (actual_result.return_type_hidl_size() != expected_result.return_type_hidl_size() ) { return false; }
        return true;
    }
    if (!strcmp(actual_result.name().c_str(), "haveSomeMemoryBlock")) {
        if (actual_result.return_type_hidl_size() != expected_result.return_type_hidl_size() ) { return false; }
        if (!Verify__android__hidl__memory__block__V1_0__MemoryBlock(expected_result.return_type_hidl(0), actual_result.return_type_hidl(0))) { return false; }
        return true;
    }
    if (!strcmp(actual_result.name().c_str(), "set")) {
        if (actual_result.return_type_hidl_size() != expected_result.return_type_hidl_size() ) { return false; }
        return true;
    }
    if (!strcmp(actual_result.name().c_str(), "get")) {
        if (actual_result.return_type_hidl_size() != expected_result.return_type_hidl_size() ) { return false; }
        /* ERROR: TYPE_HIDL_INTERFACE is not supported yet. */
        return true;
    }
    return false;
}

extern "C" {
android::vts::DriverBase* vts_func_4_android_hardware_tests_memory_V1_0_IMemoryTest_() {
    return (android::vts::DriverBase*) new android::vts::FuzzerExtended_android_hardware_tests_memory_V1_0_IMemoryTest();
}

android::vts::DriverBase* vts_func_4_android_hardware_tests_memory_V1_0_IMemoryTest_with_arg(uint64_t hw_binder_proxy) {
    ::android::hardware::tests::memory::V1_0::IMemoryTest* arg = nullptr;
    if (hw_binder_proxy) {
        arg = reinterpret_cast<::android::hardware::tests::memory::V1_0::IMemoryTest*>(hw_binder_proxy);
    } else {
        LOG(INFO) << " Creating DriverBase with null proxy.";
    }
    android::vts::DriverBase* result =
        new android::vts::FuzzerExtended_android_hardware_tests_memory_V1_0_IMemoryTest(
            arg);
    if (arg != nullptr) {
        arg->decStrong(arg);
    }
    return result;
}

}
}  // namespace vts
}  // namespace android
