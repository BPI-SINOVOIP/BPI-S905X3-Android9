#ifndef __VTS_DRIVER__android_hardware_tests_bar_V1_0_IBar__
#define __VTS_DRIVER__android_hardware_tests_bar_V1_0_IBar__

#undef LOG_TAG
#define LOG_TAG "FuzzerExtended_android_hardware_tests_bar_V1_0_IBar"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <utils/Log.h>

#include <driver_base/DriverBase.h>
#include <driver_base/DriverCallbackBase.h>

#include <VtsDriverCommUtil.h>

#include <android/hardware/tests/bar/1.0/IBar.h>
#include <hidl/HidlSupport.h>
#include <android/hardware/tests/foo/1.0/IFoo.h>
#include <android/hardware/tests/foo/1.0/Foo.vts.h>
#include <android/hardware/tests/foo/1.0/IFooCallback.h>
#include <android/hardware/tests/foo/1.0/FooCallback.vts.h>
#include <android/hardware/tests/foo/1.0/IMyTypes.h>
#include <android/hardware/tests/foo/1.0/MyTypes.vts.h>
#include <android/hardware/tests/foo/1.0/ISimple.h>
#include <android/hardware/tests/foo/1.0/Simple.vts.h>
#include <android/hardware/tests/foo/1.0/ITheirTypes.h>
#include <android/hardware/tests/foo/1.0/TheirTypes.vts.h>
#include <android/hardware/tests/foo/1.0/types.h>
#include <android/hardware/tests/foo/1.0/types.vts.h>
#include <android/hidl/base/1.0/types.h>


using namespace android::hardware::tests::bar::V1_0;
namespace android {
namespace vts {
void MessageTo__android__hardware__tests__bar__V1_0__IBar__SomethingRelated(const VariableSpecificationMessage& var_msg, ::android::hardware::tests::bar::V1_0::IBar::SomethingRelated* arg, const string& callback_socket_name);
bool Verify__android__hardware__tests__bar__V1_0__IBar__SomethingRelated(const VariableSpecificationMessage& expected_result, const VariableSpecificationMessage& actual_result);
void SetResult__android__hardware__tests__bar__V1_0__IBar__SomethingRelated(VariableSpecificationMessage* result_msg, ::android::hardware::tests::bar::V1_0::IBar::SomethingRelated result_value);

class Vts_android_hardware_tests_bar_V1_0_IBar : public ::android::hardware::tests::bar::V1_0::IBar, public DriverCallbackBase {
 public:
    Vts_android_hardware_tests_bar_V1_0_IBar(const string& callback_socket_name)
        : callback_socket_name_(callback_socket_name) {};

    virtual ~Vts_android_hardware_tests_bar_V1_0_IBar() = default;

    ::android::hardware::Return<void> convertToBoolIfSmall(
        ::android::hardware::tests::foo::V1_0::IFoo::Discriminator arg0,
        const ::android::hardware::hidl_vec<::android::hardware::tests::foo::V1_0::IFoo::Union>& arg1, std::function<void(const ::android::hardware::hidl_vec<::android::hardware::tests::foo::V1_0::IFoo::ContainsUnion>& arg0)> cb) override;

    ::android::hardware::Return<void> doThis(
        float arg0) override;

    ::android::hardware::Return<int32_t> doThatAndReturnSomething(
        int64_t arg0) override;

    ::android::hardware::Return<double> doQuiteABit(
        int32_t arg0,
        int64_t arg1,
        float arg2,
        double arg3) override;

    ::android::hardware::Return<void> doSomethingElse(
        const ::android::hardware::hidl_array<int32_t, 15>& arg0, std::function<void(const ::android::hardware::hidl_array<int32_t, 32>& arg0)> cb) override;

    ::android::hardware::Return<void> doStuffAndReturnAString(
        std::function<void(const ::android::hardware::hidl_string& arg0)> cb) override;

    ::android::hardware::Return<void> mapThisVector(
        const ::android::hardware::hidl_vec<int32_t>& arg0, std::function<void(const ::android::hardware::hidl_vec<int32_t>& arg0)> cb) override;

    ::android::hardware::Return<void> callMe(
        const sp<::android::hardware::tests::foo::V1_0::IFooCallback>& arg0) override;

    ::android::hardware::Return<::android::hardware::tests::foo::V1_0::IFoo::SomeEnum> useAnEnum(
        ::android::hardware::tests::foo::V1_0::IFoo::SomeEnum arg0) override;

    ::android::hardware::Return<void> haveAGooberVec(
        const ::android::hardware::hidl_vec<::android::hardware::tests::foo::V1_0::IFoo::Goober>& arg0) override;

    ::android::hardware::Return<void> haveAGoober(
        const ::android::hardware::tests::foo::V1_0::IFoo::Goober& arg0) override;

    ::android::hardware::Return<void> haveAGooberArray(
        const ::android::hardware::hidl_array<::android::hardware::tests::foo::V1_0::IFoo::Goober, 20>& arg0) override;

    ::android::hardware::Return<void> haveATypeFromAnotherFile(
        const ::android::hardware::tests::foo::V1_0::Abc& arg0) override;

    ::android::hardware::Return<void> haveSomeStrings(
        const ::android::hardware::hidl_array<::android::hardware::hidl_string, 3>& arg0, std::function<void(const ::android::hardware::hidl_array<::android::hardware::hidl_string, 2>& arg0)> cb) override;

    ::android::hardware::Return<void> haveAStringVec(
        const ::android::hardware::hidl_vec<::android::hardware::hidl_string>& arg0, std::function<void(const ::android::hardware::hidl_vec<::android::hardware::hidl_string>& arg0)> cb) override;

    ::android::hardware::Return<void> transposeMe(
        const ::android::hardware::hidl_array<float, 3, 5>& arg0, std::function<void(const ::android::hardware::hidl_array<float, 5, 3>& arg0)> cb) override;

    ::android::hardware::Return<void> callingDrWho(
        const ::android::hardware::tests::foo::V1_0::IFoo::MultiDimensional& arg0, std::function<void(const ::android::hardware::tests::foo::V1_0::IFoo::MultiDimensional& arg0)> cb) override;

    ::android::hardware::Return<void> transpose(
        const ::android::hardware::tests::foo::V1_0::IFoo::StringMatrix5x3& arg0, std::function<void(const ::android::hardware::tests::foo::V1_0::IFoo::StringMatrix3x5& arg0)> cb) override;

    ::android::hardware::Return<void> transpose2(
        const ::android::hardware::hidl_array<::android::hardware::hidl_string, 5, 3>& arg0, std::function<void(const ::android::hardware::hidl_array<::android::hardware::hidl_string, 3, 5>& arg0)> cb) override;

    ::android::hardware::Return<void> sendVec(
        const ::android::hardware::hidl_vec<uint8_t>& arg0, std::function<void(const ::android::hardware::hidl_vec<uint8_t>& arg0)> cb) override;

    ::android::hardware::Return<void> sendVecVec(
        std::function<void(const ::android::hardware::hidl_vec<::android::hardware::hidl_vec<uint8_t>>& arg0)> cb) override;

    ::android::hardware::Return<void> haveAVectorOfInterfaces(
        const ::android::hardware::hidl_vec<sp<::android::hardware::tests::foo::V1_0::ISimple>>& arg0, std::function<void(const ::android::hardware::hidl_vec<sp<::android::hardware::tests::foo::V1_0::ISimple>>& arg0)> cb) override;

    ::android::hardware::Return<void> haveAVectorOfGenericInterfaces(
        const ::android::hardware::hidl_vec<sp<::android::hidl::base::V1_0::IBase>>& arg0, std::function<void(const ::android::hardware::hidl_vec<sp<::android::hidl::base::V1_0::IBase>>& arg0)> cb) override;

    ::android::hardware::Return<void> echoNullInterface(
        const sp<::android::hardware::tests::foo::V1_0::IFooCallback>& arg0, std::function<void(bool arg0,const sp<::android::hardware::tests::foo::V1_0::IFooCallback>& arg1)> cb) override;

    ::android::hardware::Return<void> createMyHandle(
        std::function<void(const ::android::hardware::tests::foo::V1_0::IFoo::MyHandle& arg0)> cb) override;

    ::android::hardware::Return<void> createHandles(
        uint32_t arg0, std::function<void(const ::android::hardware::hidl_vec<::android::hardware::hidl_handle>& arg0)> cb) override;

    ::android::hardware::Return<void> closeHandles(
        ) override;

    ::android::hardware::Return<void> thisIsNew(
        ) override;

    ::android::hardware::Return<void> expectNullHandle(
        const ::android::hardware::hidl_handle& arg0,
        const ::android::hardware::tests::foo::V1_0::Abc& arg1, std::function<void(bool arg0,bool arg1)> cb) override;

    ::android::hardware::Return<void> takeAMask(
        ::android::hardware::tests::foo::V1_0::IFoo::BitField arg0,
        uint8_t arg1,
        const ::android::hardware::tests::foo::V1_0::IFoo::MyMask& arg2,
        uint8_t arg3, std::function<void(::android::hardware::tests::foo::V1_0::IFoo::BitField arg0,uint8_t arg1,uint8_t arg2,uint8_t arg3)> cb) override;

    ::android::hardware::Return<sp<::android::hardware::tests::foo::V1_0::ISimple>> haveAInterface(
        const sp<::android::hardware::tests::foo::V1_0::ISimple>& arg0) override;


 private:
    string callback_socket_name_;
};

sp<::android::hardware::tests::bar::V1_0::IBar> VtsFuzzerCreateVts_android_hardware_tests_bar_V1_0_IBar(const string& callback_socket_name);

class FuzzerExtended_android_hardware_tests_bar_V1_0_IBar : public DriverBase {
 public:
    FuzzerExtended_android_hardware_tests_bar_V1_0_IBar() : DriverBase(HAL_HIDL), hw_binder_proxy_() {}

    explicit FuzzerExtended_android_hardware_tests_bar_V1_0_IBar(::android::hardware::tests::bar::V1_0::IBar* hw_binder_proxy) : DriverBase(HAL_HIDL), hw_binder_proxy_(hw_binder_proxy) {}
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
    sp<::android::hardware::tests::bar::V1_0::IBar> hw_binder_proxy_;
};


extern "C" {
extern android::vts::DriverBase* vts_func_4_android_hardware_tests_bar_V1_0_IBar_();
extern android::vts::DriverBase* vts_func_4_android_hardware_tests_bar_V1_0_IBar_with_arg(uint64_t hw_binder_proxy);
}
}  // namespace vts
}  // namespace android
#endif
