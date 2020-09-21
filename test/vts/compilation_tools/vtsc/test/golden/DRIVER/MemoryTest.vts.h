#ifndef __VTS_DRIVER__android_hardware_tests_memory_V1_0_IMemoryTest__
#define __VTS_DRIVER__android_hardware_tests_memory_V1_0_IMemoryTest__

#undef LOG_TAG
#define LOG_TAG "FuzzerExtended_android_hardware_tests_memory_V1_0_IMemoryTest"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <utils/Log.h>

#include <driver_base/DriverBase.h>
#include <driver_base/DriverCallbackBase.h>

#include <VtsDriverCommUtil.h>

#include <android/hardware/tests/memory/1.0/IMemoryTest.h>
#include <hidl/HidlSupport.h>
#include <android/hidl/base/1.0/types.h>
#include <android/hidl/memory/block/1.0/types.h>
#include <android/hidl/memory/block/1.0/types.vts.h>
#include <android/hidl/memory/token/1.0/IMemoryToken.h>
#include <android/hidl/memory/token/1.0/MemoryToken.vts.h>


using namespace android::hardware::tests::memory::V1_0;
namespace android {
namespace vts {

class Vts_android_hardware_tests_memory_V1_0_IMemoryTest : public ::android::hardware::tests::memory::V1_0::IMemoryTest, public DriverCallbackBase {
 public:
    Vts_android_hardware_tests_memory_V1_0_IMemoryTest(const string& callback_socket_name)
        : callback_socket_name_(callback_socket_name) {};

    virtual ~Vts_android_hardware_tests_memory_V1_0_IMemoryTest() = default;

    ::android::hardware::Return<void> haveSomeMemory(
        const ::android::hardware::hidl_memory& arg0, std::function<void(const ::android::hardware::hidl_memory& arg0)> cb) override;

    ::android::hardware::Return<void> fillMemory(
        const ::android::hardware::hidl_memory& arg0,
        uint8_t arg1) override;

    ::android::hardware::Return<void> haveSomeMemoryBlock(
        const ::android::hidl::memory::block::V1_0::MemoryBlock& arg0, std::function<void(const ::android::hidl::memory::block::V1_0::MemoryBlock& arg0)> cb) override;

    ::android::hardware::Return<void> set(
        const ::android::hardware::hidl_memory& arg0) override;

    ::android::hardware::Return<sp<::android::hidl::memory::token::V1_0::IMemoryToken>> get(
        ) override;


 private:
    string callback_socket_name_;
};

sp<::android::hardware::tests::memory::V1_0::IMemoryTest> VtsFuzzerCreateVts_android_hardware_tests_memory_V1_0_IMemoryTest(const string& callback_socket_name);

class FuzzerExtended_android_hardware_tests_memory_V1_0_IMemoryTest : public DriverBase {
 public:
    FuzzerExtended_android_hardware_tests_memory_V1_0_IMemoryTest() : DriverBase(HAL_HIDL), hw_binder_proxy_() {}

    explicit FuzzerExtended_android_hardware_tests_memory_V1_0_IMemoryTest(::android::hardware::tests::memory::V1_0::IMemoryTest* hw_binder_proxy) : DriverBase(HAL_HIDL), hw_binder_proxy_(hw_binder_proxy) {}
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
    sp<::android::hardware::tests::memory::V1_0::IMemoryTest> hw_binder_proxy_;
};


extern "C" {
extern android::vts::DriverBase* vts_func_4_android_hardware_tests_memory_V1_0_IMemoryTest_();
extern android::vts::DriverBase* vts_func_4_android_hardware_tests_memory_V1_0_IMemoryTest_with_arg(uint64_t hw_binder_proxy);
}
}  // namespace vts
}  // namespace android
#endif
