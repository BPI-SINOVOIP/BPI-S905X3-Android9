#include "android/hardware/tests/bar/1.0/Bar.vts.h"
#include "vts_measurement.h"
#include <android-base/logging.h>
#include <android/hidl/allocator/1.0/IAllocator.h>
#include <fmq/MessageQueue.h>
#include <sys/stat.h>
#include <unistd.h>


using namespace android::hardware::tests::bar::V1_0;
namespace android {
namespace vts {
void MessageTo__android__hardware__tests__bar__V1_0__IBar__SomethingRelated(const VariableSpecificationMessage& var_msg __attribute__((__unused__)), ::android::hardware::tests::bar::V1_0::IBar::SomethingRelated* arg __attribute__((__unused__)), const string& callback_socket_name __attribute__((__unused__))) {
    MessageTo__android__hardware__tests__foo__V1_0__Unrelated(var_msg.struct_value(0), &(arg->myRelated), callback_socket_name);
}
bool Verify__android__hardware__tests__bar__V1_0__IBar__SomethingRelated(const VariableSpecificationMessage& expected_result __attribute__((__unused__)), const VariableSpecificationMessage& actual_result __attribute__((__unused__))){
    if (!Verify__android__hardware__tests__foo__V1_0__Unrelated(expected_result.struct_value(0), actual_result.struct_value(0))) { return false; }
    return true;
}

void SetResult__android__hardware__tests__bar__V1_0__IBar__SomethingRelated(VariableSpecificationMessage* result_msg, ::android::hardware::tests::bar::V1_0::IBar::SomethingRelated result_value __attribute__((__unused__))){
    result_msg->set_type(TYPE_STRUCT);
    auto *result_msg_myRelated = result_msg->add_struct_value();
    result_msg_myRelated->set_type(TYPE_STRUCT);
    SetResult__android__hardware__tests__foo__V1_0__Unrelated(result_msg_myRelated, result_value.myRelated);
    result_msg_myRelated->set_name("myRelated");
}

bool FuzzerExtended_android_hardware_tests_bar_V1_0_IBar::GetService(bool get_stub, const char* service_name) {
    static bool initialized = false;
    if (!initialized) {
        LOG(INFO) << "HIDL getService";
        if (service_name) {
          LOG(INFO) << "  - service name: " << service_name;
        }
        hw_binder_proxy_ = ::android::hardware::tests::bar::V1_0::IBar::getService(service_name, get_stub);
        if (hw_binder_proxy_ == nullptr) {
            LOG(ERROR) << "getService() returned a null pointer.";
            return false;
        }
        LOG(DEBUG) << "hw_binder_proxy_ = " << hw_binder_proxy_.get();
        initialized = true;
    }
    return true;
}


::android::hardware::Return<void> Vts_android_hardware_tests_bar_V1_0_IBar::convertToBoolIfSmall(
    ::android::hardware::tests::foo::V1_0::IFoo::Discriminator arg0 __attribute__((__unused__)),
    const ::android::hardware::hidl_vec<::android::hardware::tests::foo::V1_0::IFoo::Union>& arg1 __attribute__((__unused__)), std::function<void(const ::android::hardware::hidl_vec<::android::hardware::tests::foo::V1_0::IFoo::ContainsUnion>& arg0)> cb) {
    LOG(INFO) << "convertToBoolIfSmall called";
    AndroidSystemCallbackRequestMessage callback_message;
    callback_message.set_id(GetCallbackID("convertToBoolIfSmall"));
    callback_message.set_name("Vts_android_hardware_tests_bar_V1_0_IBar::convertToBoolIfSmall");
    VariableSpecificationMessage* var_msg0 = callback_message.add_arg();
    var_msg0->set_type(TYPE_ENUM);
    SetResult__android__hardware__tests__foo__V1_0__IFoo__Discriminator(var_msg0, arg0);
    VariableSpecificationMessage* var_msg1 = callback_message.add_arg();
    var_msg1->set_type(TYPE_VECTOR);
    var_msg1->set_vector_size(arg1.size());
    for (int i = 0; i < (int)arg1.size(); i++) {
        auto *var_msg1_vector_i = var_msg1->add_vector_value();
        var_msg1_vector_i->set_type(TYPE_UNION);
        SetResult__android__hardware__tests__foo__V1_0__IFoo__Union(var_msg1_vector_i, arg1[i]);
    }
    RpcCallToAgent(callback_message, callback_socket_name_);
    cb(::android::hardware::hidl_vec<::android::hardware::tests::foo::V1_0::IFoo::ContainsUnion>());
    return ::android::hardware::Void();
}

::android::hardware::Return<void> Vts_android_hardware_tests_bar_V1_0_IBar::doThis(
    float arg0 __attribute__((__unused__))) {
    LOG(INFO) << "doThis called";
    AndroidSystemCallbackRequestMessage callback_message;
    callback_message.set_id(GetCallbackID("doThis"));
    callback_message.set_name("Vts_android_hardware_tests_bar_V1_0_IBar::doThis");
    VariableSpecificationMessage* var_msg0 = callback_message.add_arg();
    var_msg0->set_type(TYPE_SCALAR);
    var_msg0->set_scalar_type("float_t");
    var_msg0->mutable_scalar_value()->set_float_t(arg0);
    RpcCallToAgent(callback_message, callback_socket_name_);
    return ::android::hardware::Void();
}

::android::hardware::Return<int32_t> Vts_android_hardware_tests_bar_V1_0_IBar::doThatAndReturnSomething(
    int64_t arg0 __attribute__((__unused__))) {
    LOG(INFO) << "doThatAndReturnSomething called";
    AndroidSystemCallbackRequestMessage callback_message;
    callback_message.set_id(GetCallbackID("doThatAndReturnSomething"));
    callback_message.set_name("Vts_android_hardware_tests_bar_V1_0_IBar::doThatAndReturnSomething");
    VariableSpecificationMessage* var_msg0 = callback_message.add_arg();
    var_msg0->set_type(TYPE_SCALAR);
    var_msg0->set_scalar_type("int64_t");
    var_msg0->mutable_scalar_value()->set_int64_t(arg0);
    RpcCallToAgent(callback_message, callback_socket_name_);
    return static_cast<int32_t>(0);
}

::android::hardware::Return<double> Vts_android_hardware_tests_bar_V1_0_IBar::doQuiteABit(
    int32_t arg0 __attribute__((__unused__)),
    int64_t arg1 __attribute__((__unused__)),
    float arg2 __attribute__((__unused__)),
    double arg3 __attribute__((__unused__))) {
    LOG(INFO) << "doQuiteABit called";
    AndroidSystemCallbackRequestMessage callback_message;
    callback_message.set_id(GetCallbackID("doQuiteABit"));
    callback_message.set_name("Vts_android_hardware_tests_bar_V1_0_IBar::doQuiteABit");
    VariableSpecificationMessage* var_msg0 = callback_message.add_arg();
    var_msg0->set_type(TYPE_SCALAR);
    var_msg0->set_scalar_type("int32_t");
    var_msg0->mutable_scalar_value()->set_int32_t(arg0);
    VariableSpecificationMessage* var_msg1 = callback_message.add_arg();
    var_msg1->set_type(TYPE_SCALAR);
    var_msg1->set_scalar_type("int64_t");
    var_msg1->mutable_scalar_value()->set_int64_t(arg1);
    VariableSpecificationMessage* var_msg2 = callback_message.add_arg();
    var_msg2->set_type(TYPE_SCALAR);
    var_msg2->set_scalar_type("float_t");
    var_msg2->mutable_scalar_value()->set_float_t(arg2);
    VariableSpecificationMessage* var_msg3 = callback_message.add_arg();
    var_msg3->set_type(TYPE_SCALAR);
    var_msg3->set_scalar_type("double_t");
    var_msg3->mutable_scalar_value()->set_double_t(arg3);
    RpcCallToAgent(callback_message, callback_socket_name_);
    return static_cast<double>(0);
}

::android::hardware::Return<void> Vts_android_hardware_tests_bar_V1_0_IBar::doSomethingElse(
    const ::android::hardware::hidl_array<int32_t, 15>& arg0 __attribute__((__unused__)), std::function<void(const ::android::hardware::hidl_array<int32_t, 32>& arg0)> cb) {
    LOG(INFO) << "doSomethingElse called";
    AndroidSystemCallbackRequestMessage callback_message;
    callback_message.set_id(GetCallbackID("doSomethingElse"));
    callback_message.set_name("Vts_android_hardware_tests_bar_V1_0_IBar::doSomethingElse");
    VariableSpecificationMessage* var_msg0 = callback_message.add_arg();
    var_msg0->set_type(TYPE_ARRAY);
    var_msg0->set_vector_size(1);
    for (int i = 0; i < 1; i++) {
        auto *var_msg0_array_i = var_msg0->add_vector_value();
        var_msg0_array_i->set_type(TYPE_SCALAR);
        var_msg0_array_i->set_scalar_type("int32_t");
        var_msg0_array_i->mutable_scalar_value()->set_int32_t(arg0[i]);
    }
    RpcCallToAgent(callback_message, callback_socket_name_);
    cb(::android::hardware::hidl_array<int32_t, 32>());
    return ::android::hardware::Void();
}

::android::hardware::Return<void> Vts_android_hardware_tests_bar_V1_0_IBar::doStuffAndReturnAString(
    std::function<void(const ::android::hardware::hidl_string& arg0)> cb) {
    LOG(INFO) << "doStuffAndReturnAString called";
    AndroidSystemCallbackRequestMessage callback_message;
    callback_message.set_id(GetCallbackID("doStuffAndReturnAString"));
    callback_message.set_name("Vts_android_hardware_tests_bar_V1_0_IBar::doStuffAndReturnAString");
    RpcCallToAgent(callback_message, callback_socket_name_);
    cb(::android::hardware::hidl_string());
    return ::android::hardware::Void();
}

::android::hardware::Return<void> Vts_android_hardware_tests_bar_V1_0_IBar::mapThisVector(
    const ::android::hardware::hidl_vec<int32_t>& arg0 __attribute__((__unused__)), std::function<void(const ::android::hardware::hidl_vec<int32_t>& arg0)> cb) {
    LOG(INFO) << "mapThisVector called";
    AndroidSystemCallbackRequestMessage callback_message;
    callback_message.set_id(GetCallbackID("mapThisVector"));
    callback_message.set_name("Vts_android_hardware_tests_bar_V1_0_IBar::mapThisVector");
    VariableSpecificationMessage* var_msg0 = callback_message.add_arg();
    var_msg0->set_type(TYPE_VECTOR);
    var_msg0->set_vector_size(arg0.size());
    for (int i = 0; i < (int)arg0.size(); i++) {
        auto *var_msg0_vector_i = var_msg0->add_vector_value();
        var_msg0_vector_i->set_type(TYPE_SCALAR);
        var_msg0_vector_i->set_scalar_type("int32_t");
        var_msg0_vector_i->mutable_scalar_value()->set_int32_t(arg0[i]);
    }
    RpcCallToAgent(callback_message, callback_socket_name_);
    cb(::android::hardware::hidl_vec<int32_t>());
    return ::android::hardware::Void();
}

::android::hardware::Return<void> Vts_android_hardware_tests_bar_V1_0_IBar::callMe(
    const sp<::android::hardware::tests::foo::V1_0::IFooCallback>& arg0 __attribute__((__unused__))) {
    LOG(INFO) << "callMe called";
    AndroidSystemCallbackRequestMessage callback_message;
    callback_message.set_id(GetCallbackID("callMe"));
    callback_message.set_name("Vts_android_hardware_tests_bar_V1_0_IBar::callMe");
    VariableSpecificationMessage* var_msg0 = callback_message.add_arg();
    var_msg0->set_type(TYPE_HIDL_CALLBACK);
    /* ERROR: TYPE_HIDL_CALLBACK is not supported yet. */
    RpcCallToAgent(callback_message, callback_socket_name_);
    return ::android::hardware::Void();
}

::android::hardware::Return<::android::hardware::tests::foo::V1_0::IFoo::SomeEnum> Vts_android_hardware_tests_bar_V1_0_IBar::useAnEnum(
    ::android::hardware::tests::foo::V1_0::IFoo::SomeEnum arg0 __attribute__((__unused__))) {
    LOG(INFO) << "useAnEnum called";
    AndroidSystemCallbackRequestMessage callback_message;
    callback_message.set_id(GetCallbackID("useAnEnum"));
    callback_message.set_name("Vts_android_hardware_tests_bar_V1_0_IBar::useAnEnum");
    VariableSpecificationMessage* var_msg0 = callback_message.add_arg();
    var_msg0->set_type(TYPE_ENUM);
    SetResult__android__hardware__tests__foo__V1_0__IFoo__SomeEnum(var_msg0, arg0);
    RpcCallToAgent(callback_message, callback_socket_name_);
    return ::android::hardware::tests::foo::V1_0::IFoo::SomeEnum();
}

::android::hardware::Return<void> Vts_android_hardware_tests_bar_V1_0_IBar::haveAGooberVec(
    const ::android::hardware::hidl_vec<::android::hardware::tests::foo::V1_0::IFoo::Goober>& arg0 __attribute__((__unused__))) {
    LOG(INFO) << "haveAGooberVec called";
    AndroidSystemCallbackRequestMessage callback_message;
    callback_message.set_id(GetCallbackID("haveAGooberVec"));
    callback_message.set_name("Vts_android_hardware_tests_bar_V1_0_IBar::haveAGooberVec");
    VariableSpecificationMessage* var_msg0 = callback_message.add_arg();
    var_msg0->set_type(TYPE_VECTOR);
    var_msg0->set_vector_size(arg0.size());
    for (int i = 0; i < (int)arg0.size(); i++) {
        auto *var_msg0_vector_i = var_msg0->add_vector_value();
        var_msg0_vector_i->set_type(TYPE_STRUCT);
        SetResult__android__hardware__tests__foo__V1_0__IFoo__Goober(var_msg0_vector_i, arg0[i]);
    }
    RpcCallToAgent(callback_message, callback_socket_name_);
    return ::android::hardware::Void();
}

::android::hardware::Return<void> Vts_android_hardware_tests_bar_V1_0_IBar::haveAGoober(
    const ::android::hardware::tests::foo::V1_0::IFoo::Goober& arg0 __attribute__((__unused__))) {
    LOG(INFO) << "haveAGoober called";
    AndroidSystemCallbackRequestMessage callback_message;
    callback_message.set_id(GetCallbackID("haveAGoober"));
    callback_message.set_name("Vts_android_hardware_tests_bar_V1_0_IBar::haveAGoober");
    VariableSpecificationMessage* var_msg0 = callback_message.add_arg();
    var_msg0->set_type(TYPE_STRUCT);
    SetResult__android__hardware__tests__foo__V1_0__IFoo__Goober(var_msg0, arg0);
    RpcCallToAgent(callback_message, callback_socket_name_);
    return ::android::hardware::Void();
}

::android::hardware::Return<void> Vts_android_hardware_tests_bar_V1_0_IBar::haveAGooberArray(
    const ::android::hardware::hidl_array<::android::hardware::tests::foo::V1_0::IFoo::Goober, 20>& arg0 __attribute__((__unused__))) {
    LOG(INFO) << "haveAGooberArray called";
    AndroidSystemCallbackRequestMessage callback_message;
    callback_message.set_id(GetCallbackID("haveAGooberArray"));
    callback_message.set_name("Vts_android_hardware_tests_bar_V1_0_IBar::haveAGooberArray");
    VariableSpecificationMessage* var_msg0 = callback_message.add_arg();
    var_msg0->set_type(TYPE_ARRAY);
    var_msg0->set_vector_size(1);
    for (int i = 0; i < 1; i++) {
        auto *var_msg0_array_i = var_msg0->add_vector_value();
        var_msg0_array_i->set_type(TYPE_STRUCT);
        auto *var_msg0_array_i_q = var_msg0_array_i->add_struct_value();
        var_msg0_array_i_q->set_type(TYPE_SCALAR);
        var_msg0_array_i_q->set_scalar_type("int32_t");
        var_msg0_array_i_q->mutable_scalar_value()->set_int32_t(arg0[i].q);
        var_msg0_array_i_q->set_name("q");
        auto *var_msg0_array_i_name = var_msg0_array_i->add_struct_value();
        var_msg0_array_i_name->set_type(TYPE_STRING);
        var_msg0_array_i_name->mutable_string_value()->set_message(arg0[i].name.c_str());
        var_msg0_array_i_name->mutable_string_value()->set_length(arg0[i].name.size());
        var_msg0_array_i_name->set_name("name");
        auto *var_msg0_array_i_address = var_msg0_array_i->add_struct_value();
        var_msg0_array_i_address->set_type(TYPE_STRING);
        var_msg0_array_i_address->mutable_string_value()->set_message(arg0[i].address.c_str());
        var_msg0_array_i_address->mutable_string_value()->set_length(arg0[i].address.size());
        var_msg0_array_i_address->set_name("address");
        auto *var_msg0_array_i_numbers = var_msg0_array_i->add_struct_value();
        var_msg0_array_i_numbers->set_type(TYPE_ARRAY);
        var_msg0_array_i_numbers->set_vector_size(1);
        for (int i = 0; i < 1; i++) {
            auto *var_msg0_array_i_numbers_array_i = var_msg0_array_i_numbers->add_vector_value();
            var_msg0_array_i_numbers_array_i->set_type(TYPE_SCALAR);
            var_msg0_array_i_numbers_array_i->set_scalar_type("double_t");
            var_msg0_array_i_numbers_array_i->mutable_scalar_value()->set_double_t(arg0[i].numbers[i]);
        }
        var_msg0_array_i_numbers->set_name("numbers");
        auto *var_msg0_array_i_fumble = var_msg0_array_i->add_struct_value();
        var_msg0_array_i_fumble->set_type(TYPE_STRUCT);
        SetResult__android__hardware__tests__foo__V1_0__IFoo__Fumble(var_msg0_array_i_fumble, arg0[i].fumble);
        var_msg0_array_i_fumble->set_name("fumble");
        auto *var_msg0_array_i_gumble = var_msg0_array_i->add_struct_value();
        var_msg0_array_i_gumble->set_type(TYPE_STRUCT);
        SetResult__android__hardware__tests__foo__V1_0__IFoo__Fumble(var_msg0_array_i_gumble, arg0[i].gumble);
        var_msg0_array_i_gumble->set_name("gumble");
    }
    RpcCallToAgent(callback_message, callback_socket_name_);
    return ::android::hardware::Void();
}

::android::hardware::Return<void> Vts_android_hardware_tests_bar_V1_0_IBar::haveATypeFromAnotherFile(
    const ::android::hardware::tests::foo::V1_0::Abc& arg0 __attribute__((__unused__))) {
    LOG(INFO) << "haveATypeFromAnotherFile called";
    AndroidSystemCallbackRequestMessage callback_message;
    callback_message.set_id(GetCallbackID("haveATypeFromAnotherFile"));
    callback_message.set_name("Vts_android_hardware_tests_bar_V1_0_IBar::haveATypeFromAnotherFile");
    VariableSpecificationMessage* var_msg0 = callback_message.add_arg();
    var_msg0->set_type(TYPE_STRUCT);
    SetResult__android__hardware__tests__foo__V1_0__Abc(var_msg0, arg0);
    RpcCallToAgent(callback_message, callback_socket_name_);
    return ::android::hardware::Void();
}

::android::hardware::Return<void> Vts_android_hardware_tests_bar_V1_0_IBar::haveSomeStrings(
    const ::android::hardware::hidl_array<::android::hardware::hidl_string, 3>& arg0 __attribute__((__unused__)), std::function<void(const ::android::hardware::hidl_array<::android::hardware::hidl_string, 2>& arg0)> cb) {
    LOG(INFO) << "haveSomeStrings called";
    AndroidSystemCallbackRequestMessage callback_message;
    callback_message.set_id(GetCallbackID("haveSomeStrings"));
    callback_message.set_name("Vts_android_hardware_tests_bar_V1_0_IBar::haveSomeStrings");
    VariableSpecificationMessage* var_msg0 = callback_message.add_arg();
    var_msg0->set_type(TYPE_ARRAY);
    var_msg0->set_vector_size(1);
    for (int i = 0; i < 1; i++) {
        auto *var_msg0_array_i = var_msg0->add_vector_value();
        var_msg0_array_i->set_type(TYPE_STRING);
        var_msg0_array_i->mutable_string_value()->set_message(arg0[i].c_str());
        var_msg0_array_i->mutable_string_value()->set_length(arg0[i].size());
    }
    RpcCallToAgent(callback_message, callback_socket_name_);
    cb(::android::hardware::hidl_array<::android::hardware::hidl_string, 2>());
    return ::android::hardware::Void();
}

::android::hardware::Return<void> Vts_android_hardware_tests_bar_V1_0_IBar::haveAStringVec(
    const ::android::hardware::hidl_vec<::android::hardware::hidl_string>& arg0 __attribute__((__unused__)), std::function<void(const ::android::hardware::hidl_vec<::android::hardware::hidl_string>& arg0)> cb) {
    LOG(INFO) << "haveAStringVec called";
    AndroidSystemCallbackRequestMessage callback_message;
    callback_message.set_id(GetCallbackID("haveAStringVec"));
    callback_message.set_name("Vts_android_hardware_tests_bar_V1_0_IBar::haveAStringVec");
    VariableSpecificationMessage* var_msg0 = callback_message.add_arg();
    var_msg0->set_type(TYPE_VECTOR);
    var_msg0->set_vector_size(arg0.size());
    for (int i = 0; i < (int)arg0.size(); i++) {
        auto *var_msg0_vector_i = var_msg0->add_vector_value();
        var_msg0_vector_i->set_type(TYPE_STRING);
        var_msg0_vector_i->mutable_string_value()->set_message(arg0[i].c_str());
        var_msg0_vector_i->mutable_string_value()->set_length(arg0[i].size());
    }
    RpcCallToAgent(callback_message, callback_socket_name_);
    cb(::android::hardware::hidl_vec<::android::hardware::hidl_string>());
    return ::android::hardware::Void();
}

::android::hardware::Return<void> Vts_android_hardware_tests_bar_V1_0_IBar::transposeMe(
    const ::android::hardware::hidl_array<float, 3, 5>& arg0 __attribute__((__unused__)), std::function<void(const ::android::hardware::hidl_array<float, 5, 3>& arg0)> cb) {
    LOG(INFO) << "transposeMe called";
    AndroidSystemCallbackRequestMessage callback_message;
    callback_message.set_id(GetCallbackID("transposeMe"));
    callback_message.set_name("Vts_android_hardware_tests_bar_V1_0_IBar::transposeMe");
    VariableSpecificationMessage* var_msg0 = callback_message.add_arg();
    var_msg0->set_type(TYPE_ARRAY);
    var_msg0->set_vector_size(1);
    for (int i = 0; i < 1; i++) {
        auto *var_msg0_array_i = var_msg0->add_vector_value();
        var_msg0_array_i->set_type(TYPE_ARRAY);
        var_msg0_array_i->set_vector_size(1);
        for (int i = 0; i < 1; i++) {
            auto *var_msg0_array_i_array_i = var_msg0_array_i->add_vector_value();
            var_msg0_array_i_array_i->set_type(TYPE_SCALAR);
            var_msg0_array_i_array_i->set_scalar_type("float_t");
            var_msg0_array_i_array_i->mutable_scalar_value()->set_float_t(arg0[i][i]);
        }
    }
    RpcCallToAgent(callback_message, callback_socket_name_);
    cb(::android::hardware::hidl_array<float, 5, 3>());
    return ::android::hardware::Void();
}

::android::hardware::Return<void> Vts_android_hardware_tests_bar_V1_0_IBar::callingDrWho(
    const ::android::hardware::tests::foo::V1_0::IFoo::MultiDimensional& arg0 __attribute__((__unused__)), std::function<void(const ::android::hardware::tests::foo::V1_0::IFoo::MultiDimensional& arg0)> cb) {
    LOG(INFO) << "callingDrWho called";
    AndroidSystemCallbackRequestMessage callback_message;
    callback_message.set_id(GetCallbackID("callingDrWho"));
    callback_message.set_name("Vts_android_hardware_tests_bar_V1_0_IBar::callingDrWho");
    VariableSpecificationMessage* var_msg0 = callback_message.add_arg();
    var_msg0->set_type(TYPE_STRUCT);
    SetResult__android__hardware__tests__foo__V1_0__IFoo__MultiDimensional(var_msg0, arg0);
    RpcCallToAgent(callback_message, callback_socket_name_);
    cb(::android::hardware::tests::foo::V1_0::IFoo::MultiDimensional());
    return ::android::hardware::Void();
}

::android::hardware::Return<void> Vts_android_hardware_tests_bar_V1_0_IBar::transpose(
    const ::android::hardware::tests::foo::V1_0::IFoo::StringMatrix5x3& arg0 __attribute__((__unused__)), std::function<void(const ::android::hardware::tests::foo::V1_0::IFoo::StringMatrix3x5& arg0)> cb) {
    LOG(INFO) << "transpose called";
    AndroidSystemCallbackRequestMessage callback_message;
    callback_message.set_id(GetCallbackID("transpose"));
    callback_message.set_name("Vts_android_hardware_tests_bar_V1_0_IBar::transpose");
    VariableSpecificationMessage* var_msg0 = callback_message.add_arg();
    var_msg0->set_type(TYPE_STRUCT);
    SetResult__android__hardware__tests__foo__V1_0__IFoo__StringMatrix5x3(var_msg0, arg0);
    RpcCallToAgent(callback_message, callback_socket_name_);
    cb(::android::hardware::tests::foo::V1_0::IFoo::StringMatrix3x5());
    return ::android::hardware::Void();
}

::android::hardware::Return<void> Vts_android_hardware_tests_bar_V1_0_IBar::transpose2(
    const ::android::hardware::hidl_array<::android::hardware::hidl_string, 5, 3>& arg0 __attribute__((__unused__)), std::function<void(const ::android::hardware::hidl_array<::android::hardware::hidl_string, 3, 5>& arg0)> cb) {
    LOG(INFO) << "transpose2 called";
    AndroidSystemCallbackRequestMessage callback_message;
    callback_message.set_id(GetCallbackID("transpose2"));
    callback_message.set_name("Vts_android_hardware_tests_bar_V1_0_IBar::transpose2");
    VariableSpecificationMessage* var_msg0 = callback_message.add_arg();
    var_msg0->set_type(TYPE_ARRAY);
    var_msg0->set_vector_size(1);
    for (int i = 0; i < 1; i++) {
        auto *var_msg0_array_i = var_msg0->add_vector_value();
        var_msg0_array_i->set_type(TYPE_ARRAY);
        var_msg0_array_i->set_vector_size(1);
        for (int i = 0; i < 1; i++) {
            auto *var_msg0_array_i_array_i = var_msg0_array_i->add_vector_value();
            var_msg0_array_i_array_i->set_type(TYPE_STRING);
            var_msg0_array_i_array_i->mutable_string_value()->set_message(arg0[i][i].c_str());
            var_msg0_array_i_array_i->mutable_string_value()->set_length(arg0[i][i].size());
        }
    }
    RpcCallToAgent(callback_message, callback_socket_name_);
    cb(::android::hardware::hidl_array<::android::hardware::hidl_string, 3, 5>());
    return ::android::hardware::Void();
}

::android::hardware::Return<void> Vts_android_hardware_tests_bar_V1_0_IBar::sendVec(
    const ::android::hardware::hidl_vec<uint8_t>& arg0 __attribute__((__unused__)), std::function<void(const ::android::hardware::hidl_vec<uint8_t>& arg0)> cb) {
    LOG(INFO) << "sendVec called";
    AndroidSystemCallbackRequestMessage callback_message;
    callback_message.set_id(GetCallbackID("sendVec"));
    callback_message.set_name("Vts_android_hardware_tests_bar_V1_0_IBar::sendVec");
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
    cb(::android::hardware::hidl_vec<uint8_t>());
    return ::android::hardware::Void();
}

::android::hardware::Return<void> Vts_android_hardware_tests_bar_V1_0_IBar::sendVecVec(
    std::function<void(const ::android::hardware::hidl_vec<::android::hardware::hidl_vec<uint8_t>>& arg0)> cb) {
    LOG(INFO) << "sendVecVec called";
    AndroidSystemCallbackRequestMessage callback_message;
    callback_message.set_id(GetCallbackID("sendVecVec"));
    callback_message.set_name("Vts_android_hardware_tests_bar_V1_0_IBar::sendVecVec");
    RpcCallToAgent(callback_message, callback_socket_name_);
    cb(::android::hardware::hidl_vec<::android::hardware::hidl_vec<uint8_t>>());
    return ::android::hardware::Void();
}

::android::hardware::Return<void> Vts_android_hardware_tests_bar_V1_0_IBar::haveAVectorOfInterfaces(
    const ::android::hardware::hidl_vec<sp<::android::hardware::tests::foo::V1_0::ISimple>>& arg0 __attribute__((__unused__)), std::function<void(const ::android::hardware::hidl_vec<sp<::android::hardware::tests::foo::V1_0::ISimple>>& arg0)> cb) {
    LOG(INFO) << "haveAVectorOfInterfaces called";
    AndroidSystemCallbackRequestMessage callback_message;
    callback_message.set_id(GetCallbackID("haveAVectorOfInterfaces"));
    callback_message.set_name("Vts_android_hardware_tests_bar_V1_0_IBar::haveAVectorOfInterfaces");
    VariableSpecificationMessage* var_msg0 = callback_message.add_arg();
    var_msg0->set_type(TYPE_VECTOR);
    var_msg0->set_vector_size(arg0.size());
    for (int i = 0; i < (int)arg0.size(); i++) {
        auto *var_msg0_vector_i = var_msg0->add_vector_value();
        var_msg0_vector_i->set_type(TYPE_HIDL_INTERFACE);
        var_msg0_vector_i->set_predefined_type("::android::hardware::tests::foo::V1_0::ISimple");
        if (arg0[i] != nullptr) {
            arg0[i]->incStrong(arg0[i].get());
            var_msg0_vector_i->set_hidl_interface_pointer(reinterpret_cast<uintptr_t>(arg0[i].get()));
        } else {
            var_msg0_vector_i->set_hidl_interface_pointer(0);
        }
    }
    RpcCallToAgent(callback_message, callback_socket_name_);
    cb(::android::hardware::hidl_vec<sp<::android::hardware::tests::foo::V1_0::ISimple>>());
    return ::android::hardware::Void();
}

::android::hardware::Return<void> Vts_android_hardware_tests_bar_V1_0_IBar::haveAVectorOfGenericInterfaces(
    const ::android::hardware::hidl_vec<sp<::android::hidl::base::V1_0::IBase>>& arg0 __attribute__((__unused__)), std::function<void(const ::android::hardware::hidl_vec<sp<::android::hidl::base::V1_0::IBase>>& arg0)> cb) {
    LOG(INFO) << "haveAVectorOfGenericInterfaces called";
    AndroidSystemCallbackRequestMessage callback_message;
    callback_message.set_id(GetCallbackID("haveAVectorOfGenericInterfaces"));
    callback_message.set_name("Vts_android_hardware_tests_bar_V1_0_IBar::haveAVectorOfGenericInterfaces");
    VariableSpecificationMessage* var_msg0 = callback_message.add_arg();
    var_msg0->set_type(TYPE_VECTOR);
    var_msg0->set_vector_size(arg0.size());
    for (int i = 0; i < (int)arg0.size(); i++) {
        auto *var_msg0_vector_i = var_msg0->add_vector_value();
        var_msg0_vector_i->set_type(TYPE_HIDL_INTERFACE);
        var_msg0_vector_i->set_predefined_type("::android::hidl::base::V1_0::IBase");
        if (arg0[i] != nullptr) {
            arg0[i]->incStrong(arg0[i].get());
            var_msg0_vector_i->set_hidl_interface_pointer(reinterpret_cast<uintptr_t>(arg0[i].get()));
        } else {
            var_msg0_vector_i->set_hidl_interface_pointer(0);
        }
    }
    RpcCallToAgent(callback_message, callback_socket_name_);
    cb(::android::hardware::hidl_vec<sp<::android::hidl::base::V1_0::IBase>>());
    return ::android::hardware::Void();
}

::android::hardware::Return<void> Vts_android_hardware_tests_bar_V1_0_IBar::echoNullInterface(
    const sp<::android::hardware::tests::foo::V1_0::IFooCallback>& arg0 __attribute__((__unused__)), std::function<void(bool arg0,const sp<::android::hardware::tests::foo::V1_0::IFooCallback>& arg1)> cb) {
    LOG(INFO) << "echoNullInterface called";
    AndroidSystemCallbackRequestMessage callback_message;
    callback_message.set_id(GetCallbackID("echoNullInterface"));
    callback_message.set_name("Vts_android_hardware_tests_bar_V1_0_IBar::echoNullInterface");
    VariableSpecificationMessage* var_msg0 = callback_message.add_arg();
    var_msg0->set_type(TYPE_HIDL_CALLBACK);
    /* ERROR: TYPE_HIDL_CALLBACK is not supported yet. */
    RpcCallToAgent(callback_message, callback_socket_name_);
    cb(static_cast<bool>(0), nullptr);
    return ::android::hardware::Void();
}

::android::hardware::Return<void> Vts_android_hardware_tests_bar_V1_0_IBar::createMyHandle(
    std::function<void(const ::android::hardware::tests::foo::V1_0::IFoo::MyHandle& arg0)> cb) {
    LOG(INFO) << "createMyHandle called";
    AndroidSystemCallbackRequestMessage callback_message;
    callback_message.set_id(GetCallbackID("createMyHandle"));
    callback_message.set_name("Vts_android_hardware_tests_bar_V1_0_IBar::createMyHandle");
    RpcCallToAgent(callback_message, callback_socket_name_);
    cb(::android::hardware::tests::foo::V1_0::IFoo::MyHandle());
    return ::android::hardware::Void();
}

::android::hardware::Return<void> Vts_android_hardware_tests_bar_V1_0_IBar::createHandles(
    uint32_t arg0 __attribute__((__unused__)), std::function<void(const ::android::hardware::hidl_vec<::android::hardware::hidl_handle>& arg0)> cb) {
    LOG(INFO) << "createHandles called";
    AndroidSystemCallbackRequestMessage callback_message;
    callback_message.set_id(GetCallbackID("createHandles"));
    callback_message.set_name("Vts_android_hardware_tests_bar_V1_0_IBar::createHandles");
    VariableSpecificationMessage* var_msg0 = callback_message.add_arg();
    var_msg0->set_type(TYPE_SCALAR);
    var_msg0->set_scalar_type("uint32_t");
    var_msg0->mutable_scalar_value()->set_uint32_t(arg0);
    RpcCallToAgent(callback_message, callback_socket_name_);
    cb(::android::hardware::hidl_vec<::android::hardware::hidl_handle>());
    return ::android::hardware::Void();
}

::android::hardware::Return<void> Vts_android_hardware_tests_bar_V1_0_IBar::closeHandles(
    ) {
    LOG(INFO) << "closeHandles called";
    AndroidSystemCallbackRequestMessage callback_message;
    callback_message.set_id(GetCallbackID("closeHandles"));
    callback_message.set_name("Vts_android_hardware_tests_bar_V1_0_IBar::closeHandles");
    RpcCallToAgent(callback_message, callback_socket_name_);
    return ::android::hardware::Void();
}

::android::hardware::Return<void> Vts_android_hardware_tests_bar_V1_0_IBar::thisIsNew(
    ) {
    LOG(INFO) << "thisIsNew called";
    AndroidSystemCallbackRequestMessage callback_message;
    callback_message.set_id(GetCallbackID("thisIsNew"));
    callback_message.set_name("Vts_android_hardware_tests_bar_V1_0_IBar::thisIsNew");
    RpcCallToAgent(callback_message, callback_socket_name_);
    return ::android::hardware::Void();
}

::android::hardware::Return<void> Vts_android_hardware_tests_bar_V1_0_IBar::expectNullHandle(
    const ::android::hardware::hidl_handle& arg0 __attribute__((__unused__)),
    const ::android::hardware::tests::foo::V1_0::Abc& arg1 __attribute__((__unused__)), std::function<void(bool arg0,bool arg1)> cb) {
    LOG(INFO) << "expectNullHandle called";
    AndroidSystemCallbackRequestMessage callback_message;
    callback_message.set_id(GetCallbackID("expectNullHandle"));
    callback_message.set_name("Vts_android_hardware_tests_bar_V1_0_IBar::expectNullHandle");
    VariableSpecificationMessage* var_msg0 = callback_message.add_arg();
    var_msg0->set_type(TYPE_HANDLE);
    /* ERROR: TYPE_HANDLE is not supported yet. */
    VariableSpecificationMessage* var_msg1 = callback_message.add_arg();
    var_msg1->set_type(TYPE_STRUCT);
    SetResult__android__hardware__tests__foo__V1_0__Abc(var_msg1, arg1);
    RpcCallToAgent(callback_message, callback_socket_name_);
    cb(static_cast<bool>(0), static_cast<bool>(0));
    return ::android::hardware::Void();
}

::android::hardware::Return<void> Vts_android_hardware_tests_bar_V1_0_IBar::takeAMask(
    ::android::hardware::tests::foo::V1_0::IFoo::BitField arg0 __attribute__((__unused__)),
    uint8_t arg1 __attribute__((__unused__)),
    const ::android::hardware::tests::foo::V1_0::IFoo::MyMask& arg2 __attribute__((__unused__)),
    uint8_t arg3 __attribute__((__unused__)), std::function<void(::android::hardware::tests::foo::V1_0::IFoo::BitField arg0,uint8_t arg1,uint8_t arg2,uint8_t arg3)> cb) {
    LOG(INFO) << "takeAMask called";
    AndroidSystemCallbackRequestMessage callback_message;
    callback_message.set_id(GetCallbackID("takeAMask"));
    callback_message.set_name("Vts_android_hardware_tests_bar_V1_0_IBar::takeAMask");
    VariableSpecificationMessage* var_msg0 = callback_message.add_arg();
    var_msg0->set_type(TYPE_ENUM);
    SetResult__android__hardware__tests__foo__V1_0__IFoo__BitField(var_msg0, arg0);
    VariableSpecificationMessage* var_msg1 = callback_message.add_arg();
    var_msg1->set_type(TYPE_MASK);
    var_msg1->set_scalar_type("uint8_t");
    var_msg1->mutable_scalar_value()->set_uint8_t(arg1);
    VariableSpecificationMessage* var_msg2 = callback_message.add_arg();
    var_msg2->set_type(TYPE_STRUCT);
    SetResult__android__hardware__tests__foo__V1_0__IFoo__MyMask(var_msg2, arg2);
    VariableSpecificationMessage* var_msg3 = callback_message.add_arg();
    var_msg3->set_type(TYPE_MASK);
    var_msg3->set_scalar_type("uint8_t");
    var_msg3->mutable_scalar_value()->set_uint8_t(arg3);
    RpcCallToAgent(callback_message, callback_socket_name_);
    cb(::android::hardware::tests::foo::V1_0::IFoo::BitField(), static_cast<uint8_t>(0), static_cast<uint8_t>(0), static_cast<uint8_t>(0));
    return ::android::hardware::Void();
}

::android::hardware::Return<sp<::android::hardware::tests::foo::V1_0::ISimple>> Vts_android_hardware_tests_bar_V1_0_IBar::haveAInterface(
    const sp<::android::hardware::tests::foo::V1_0::ISimple>& arg0 __attribute__((__unused__))) {
    LOG(INFO) << "haveAInterface called";
    AndroidSystemCallbackRequestMessage callback_message;
    callback_message.set_id(GetCallbackID("haveAInterface"));
    callback_message.set_name("Vts_android_hardware_tests_bar_V1_0_IBar::haveAInterface");
    VariableSpecificationMessage* var_msg0 = callback_message.add_arg();
    var_msg0->set_type(TYPE_HIDL_INTERFACE);
    var_msg0->set_predefined_type("::android::hardware::tests::foo::V1_0::ISimple");
    if (arg0 != nullptr) {
        arg0->incStrong(arg0.get());
        var_msg0->set_hidl_interface_pointer(reinterpret_cast<uintptr_t>(arg0.get()));
    } else {
        var_msg0->set_hidl_interface_pointer(0);
    }
    RpcCallToAgent(callback_message, callback_socket_name_);
    return nullptr;
}

sp<::android::hardware::tests::bar::V1_0::IBar> VtsFuzzerCreateVts_android_hardware_tests_bar_V1_0_IBar(const string& callback_socket_name) {
    static sp<::android::hardware::tests::bar::V1_0::IBar> result;
    result = new Vts_android_hardware_tests_bar_V1_0_IBar(callback_socket_name);
    return result;
}

bool FuzzerExtended_android_hardware_tests_bar_V1_0_IBar::Fuzz(
    FunctionSpecificationMessage* /*func_msg*/,
    void** /*result*/, const string& /*callback_socket_name*/) {
    return true;
}
bool FuzzerExtended_android_hardware_tests_bar_V1_0_IBar::GetAttribute(
    FunctionSpecificationMessage* /*func_msg*/,
    void** /*result*/) {
    LOG(ERROR) << "attribute not found.";
    return false;
}
bool FuzzerExtended_android_hardware_tests_bar_V1_0_IBar::CallFunction(
    const FunctionSpecificationMessage& func_msg,
    const string& callback_socket_name __attribute__((__unused__)),
    FunctionSpecificationMessage* result_msg) {
    const char* func_name = func_msg.name().c_str();
    if (hw_binder_proxy_ == nullptr) {
        LOG(ERROR) << "hw_binder_proxy_ is null. ";
        return false;
    }
    if (!strcmp(func_name, "convertToBoolIfSmall")) {
        ::android::hardware::tests::foo::V1_0::IFoo::Discriminator arg0;
        arg0 = EnumValue__android__hardware__tests__foo__V1_0__IFoo__Discriminator(func_msg.arg(0).scalar_value());
        ::android::hardware::hidl_vec<::android::hardware::tests::foo::V1_0::IFoo::Union> arg1;
        arg1.resize(func_msg.arg(1).vector_value_size());
        for (int arg1_index = 0; arg1_index < func_msg.arg(1).vector_value_size(); arg1_index++) {
            MessageTo__android__hardware__tests__foo__V1_0__IFoo__Union(func_msg.arg(1).vector_value(arg1_index), &(arg1[arg1_index]), callback_socket_name);
        }
        LOG(DEBUG) << "local_device = " << hw_binder_proxy_.get();
        ::android::hardware::hidl_vec<::android::hardware::tests::foo::V1_0::IFoo::ContainsUnion> result0;
        hw_binder_proxy_->convertToBoolIfSmall(arg0, arg1, [&](const ::android::hardware::hidl_vec<::android::hardware::tests::foo::V1_0::IFoo::ContainsUnion>& arg0){
            LOG(INFO) << "callback convertToBoolIfSmall called";
            result0 = arg0;
        });
        result_msg->set_name("convertToBoolIfSmall");
        VariableSpecificationMessage* result_val_0 = result_msg->add_return_type_hidl();
        result_val_0->set_type(TYPE_VECTOR);
        result_val_0->set_vector_size(result0.size());
        for (int i = 0; i < (int)result0.size(); i++) {
            auto *result_val_0_vector_i = result_val_0->add_vector_value();
            result_val_0_vector_i->set_type(TYPE_STRUCT);
            SetResult__android__hardware__tests__foo__V1_0__IFoo__ContainsUnion(result_val_0_vector_i, result0[i]);
        }
        return true;
    }
    if (!strcmp(func_name, "doThis")) {
        float arg0 = 0;
        arg0 = func_msg.arg(0).scalar_value().float_t();
        LOG(DEBUG) << "local_device = " << hw_binder_proxy_.get();
        hw_binder_proxy_->doThis(arg0);
        result_msg->set_name("doThis");
        return true;
    }
    if (!strcmp(func_name, "doThatAndReturnSomething")) {
        int64_t arg0 = 0;
        arg0 = func_msg.arg(0).scalar_value().int64_t();
        LOG(DEBUG) << "local_device = " << hw_binder_proxy_.get();
        int32_t result0;
        result0 = hw_binder_proxy_->doThatAndReturnSomething(arg0);
        result_msg->set_name("doThatAndReturnSomething");
        VariableSpecificationMessage* result_val_0 = result_msg->add_return_type_hidl();
        result_val_0->set_type(TYPE_SCALAR);
        result_val_0->set_scalar_type("int32_t");
        result_val_0->mutable_scalar_value()->set_int32_t(result0);
        return true;
    }
    if (!strcmp(func_name, "doQuiteABit")) {
        int32_t arg0 = 0;
        arg0 = func_msg.arg(0).scalar_value().int32_t();
        int64_t arg1 = 0;
        arg1 = func_msg.arg(1).scalar_value().int64_t();
        float arg2 = 0;
        arg2 = func_msg.arg(2).scalar_value().float_t();
        double arg3 = 0;
        arg3 = func_msg.arg(3).scalar_value().double_t();
        LOG(DEBUG) << "local_device = " << hw_binder_proxy_.get();
        double result0;
        result0 = hw_binder_proxy_->doQuiteABit(arg0, arg1, arg2, arg3);
        result_msg->set_name("doQuiteABit");
        VariableSpecificationMessage* result_val_0 = result_msg->add_return_type_hidl();
        result_val_0->set_type(TYPE_SCALAR);
        result_val_0->set_scalar_type("double_t");
        result_val_0->mutable_scalar_value()->set_double_t(result0);
        return true;
    }
    if (!strcmp(func_name, "doSomethingElse")) {
        ::android::hardware::hidl_array<int32_t, 15> arg0;
        for (int arg0_index = 0; arg0_index < func_msg.arg(0).vector_value_size(); arg0_index++) {
            arg0[arg0_index] = func_msg.arg(0).vector_value(arg0_index).scalar_value().int32_t();
        }
        LOG(DEBUG) << "local_device = " << hw_binder_proxy_.get();
        ::android::hardware::hidl_array<int32_t, 32> result0;
        hw_binder_proxy_->doSomethingElse(arg0, [&](const ::android::hardware::hidl_array<int32_t, 32>& arg0){
            LOG(INFO) << "callback doSomethingElse called";
            result0 = arg0;
        });
        result_msg->set_name("doSomethingElse");
        VariableSpecificationMessage* result_val_0 = result_msg->add_return_type_hidl();
        result_val_0->set_type(TYPE_ARRAY);
        result_val_0->set_vector_size(1);
        for (int i = 0; i < 1; i++) {
            auto *result_val_0_array_i = result_val_0->add_vector_value();
            result_val_0_array_i->set_type(TYPE_SCALAR);
            result_val_0_array_i->set_scalar_type("int32_t");
            result_val_0_array_i->mutable_scalar_value()->set_int32_t(result0[i]);
        }
        return true;
    }
    if (!strcmp(func_name, "doStuffAndReturnAString")) {
        LOG(DEBUG) << "local_device = " << hw_binder_proxy_.get();
        ::android::hardware::hidl_string result0;
        hw_binder_proxy_->doStuffAndReturnAString([&](const ::android::hardware::hidl_string& arg0){
            LOG(INFO) << "callback doStuffAndReturnAString called";
            result0 = arg0;
        });
        result_msg->set_name("doStuffAndReturnAString");
        VariableSpecificationMessage* result_val_0 = result_msg->add_return_type_hidl();
        result_val_0->set_type(TYPE_STRING);
        result_val_0->mutable_string_value()->set_message(result0.c_str());
        result_val_0->mutable_string_value()->set_length(result0.size());
        return true;
    }
    if (!strcmp(func_name, "mapThisVector")) {
        ::android::hardware::hidl_vec<int32_t> arg0;
        arg0.resize(func_msg.arg(0).vector_value_size());
        for (int arg0_index = 0; arg0_index < func_msg.arg(0).vector_value_size(); arg0_index++) {
            arg0[arg0_index] = func_msg.arg(0).vector_value(arg0_index).scalar_value().int32_t();
        }
        LOG(DEBUG) << "local_device = " << hw_binder_proxy_.get();
        ::android::hardware::hidl_vec<int32_t> result0;
        hw_binder_proxy_->mapThisVector(arg0, [&](const ::android::hardware::hidl_vec<int32_t>& arg0){
            LOG(INFO) << "callback mapThisVector called";
            result0 = arg0;
        });
        result_msg->set_name("mapThisVector");
        VariableSpecificationMessage* result_val_0 = result_msg->add_return_type_hidl();
        result_val_0->set_type(TYPE_VECTOR);
        result_val_0->set_vector_size(result0.size());
        for (int i = 0; i < (int)result0.size(); i++) {
            auto *result_val_0_vector_i = result_val_0->add_vector_value();
            result_val_0_vector_i->set_type(TYPE_SCALAR);
            result_val_0_vector_i->set_scalar_type("int32_t");
            result_val_0_vector_i->mutable_scalar_value()->set_int32_t(result0[i]);
        }
        return true;
    }
    if (!strcmp(func_name, "callMe")) {
        sp<::android::hardware::tests::foo::V1_0::IFooCallback> arg0;
        arg0 = VtsFuzzerCreateVts_android_hardware_tests_foo_V1_0_IFooCallback(callback_socket_name);
        static_cast<Vts_android_hardware_tests_foo_V1_0_IFooCallback*>(arg0.get())->Register(func_msg.arg(0));
        LOG(DEBUG) << "local_device = " << hw_binder_proxy_.get();
        hw_binder_proxy_->callMe(arg0);
        result_msg->set_name("callMe");
        return true;
    }
    if (!strcmp(func_name, "useAnEnum")) {
        ::android::hardware::tests::foo::V1_0::IFoo::SomeEnum arg0;
        arg0 = EnumValue__android__hardware__tests__foo__V1_0__IFoo__SomeEnum(func_msg.arg(0).scalar_value());
        LOG(DEBUG) << "local_device = " << hw_binder_proxy_.get();
        ::android::hardware::tests::foo::V1_0::IFoo::SomeEnum result0;
        result0 = hw_binder_proxy_->useAnEnum(arg0);
        result_msg->set_name("useAnEnum");
        VariableSpecificationMessage* result_val_0 = result_msg->add_return_type_hidl();
        result_val_0->set_type(TYPE_ENUM);
        SetResult__android__hardware__tests__foo__V1_0__IFoo__SomeEnum(result_val_0, result0);
        return true;
    }
    if (!strcmp(func_name, "haveAGooberVec")) {
        ::android::hardware::hidl_vec<::android::hardware::tests::foo::V1_0::IFoo::Goober> arg0;
        arg0.resize(func_msg.arg(0).vector_value_size());
        for (int arg0_index = 0; arg0_index < func_msg.arg(0).vector_value_size(); arg0_index++) {
            MessageTo__android__hardware__tests__foo__V1_0__IFoo__Goober(func_msg.arg(0).vector_value(arg0_index), &(arg0[arg0_index]), callback_socket_name);
        }
        LOG(DEBUG) << "local_device = " << hw_binder_proxy_.get();
        hw_binder_proxy_->haveAGooberVec(arg0);
        result_msg->set_name("haveAGooberVec");
        return true;
    }
    if (!strcmp(func_name, "haveAGoober")) {
        ::android::hardware::tests::foo::V1_0::IFoo::Goober arg0;
        MessageTo__android__hardware__tests__foo__V1_0__IFoo__Goober(func_msg.arg(0), &(arg0), callback_socket_name);
        LOG(DEBUG) << "local_device = " << hw_binder_proxy_.get();
        hw_binder_proxy_->haveAGoober(arg0);
        result_msg->set_name("haveAGoober");
        return true;
    }
    if (!strcmp(func_name, "haveAGooberArray")) {
        ::android::hardware::hidl_array<::android::hardware::tests::foo::V1_0::IFoo::Goober, 20> arg0;
        for (int arg0_index = 0; arg0_index < func_msg.arg(0).vector_value_size(); arg0_index++) {
            arg0[arg0_index].q = func_msg.arg(0).vector_value(arg0_index).struct_value(0).scalar_value().int32_t();
            arg0[arg0_index].name = ::android::hardware::hidl_string(func_msg.arg(0).vector_value(arg0_index).struct_value(1).string_value().message());
            arg0[arg0_index].address = ::android::hardware::hidl_string(func_msg.arg(0).vector_value(arg0_index).struct_value(2).string_value().message());
            for (int arg0_arg0_index__numbers_index = 0; arg0_arg0_index__numbers_index < func_msg.arg(0).vector_value(arg0_index).struct_value(3).vector_value_size(); arg0_arg0_index__numbers_index++) {
                arg0[arg0_index].numbers[arg0_arg0_index__numbers_index] = func_msg.arg(0).vector_value(arg0_index).struct_value(3).vector_value(arg0_arg0_index__numbers_index).scalar_value().double_t();
            }
            MessageTo__android__hardware__tests__foo__V1_0__IFoo__Fumble(func_msg.arg(0).vector_value(arg0_index).struct_value(4), &(arg0[arg0_index].fumble), callback_socket_name);
            MessageTo__android__hardware__tests__foo__V1_0__IFoo__Fumble(func_msg.arg(0).vector_value(arg0_index).struct_value(5), &(arg0[arg0_index].gumble), callback_socket_name);
        }
        LOG(DEBUG) << "local_device = " << hw_binder_proxy_.get();
        hw_binder_proxy_->haveAGooberArray(arg0);
        result_msg->set_name("haveAGooberArray");
        return true;
    }
    if (!strcmp(func_name, "haveATypeFromAnotherFile")) {
        ::android::hardware::tests::foo::V1_0::Abc arg0;
        MessageTo__android__hardware__tests__foo__V1_0__Abc(func_msg.arg(0), &(arg0), callback_socket_name);
        LOG(DEBUG) << "local_device = " << hw_binder_proxy_.get();
        hw_binder_proxy_->haveATypeFromAnotherFile(arg0);
        result_msg->set_name("haveATypeFromAnotherFile");
        return true;
    }
    if (!strcmp(func_name, "haveSomeStrings")) {
        ::android::hardware::hidl_array<::android::hardware::hidl_string, 3> arg0;
        for (int arg0_index = 0; arg0_index < func_msg.arg(0).vector_value_size(); arg0_index++) {
            arg0[arg0_index] = ::android::hardware::hidl_string(func_msg.arg(0).vector_value(arg0_index).string_value().message());
        }
        LOG(DEBUG) << "local_device = " << hw_binder_proxy_.get();
        ::android::hardware::hidl_array<::android::hardware::hidl_string, 2> result0;
        hw_binder_proxy_->haveSomeStrings(arg0, [&](const ::android::hardware::hidl_array<::android::hardware::hidl_string, 2>& arg0){
            LOG(INFO) << "callback haveSomeStrings called";
            result0 = arg0;
        });
        result_msg->set_name("haveSomeStrings");
        VariableSpecificationMessage* result_val_0 = result_msg->add_return_type_hidl();
        result_val_0->set_type(TYPE_ARRAY);
        result_val_0->set_vector_size(1);
        for (int i = 0; i < 1; i++) {
            auto *result_val_0_array_i = result_val_0->add_vector_value();
            result_val_0_array_i->set_type(TYPE_STRING);
            result_val_0_array_i->mutable_string_value()->set_message(result0[i].c_str());
            result_val_0_array_i->mutable_string_value()->set_length(result0[i].size());
        }
        return true;
    }
    if (!strcmp(func_name, "haveAStringVec")) {
        ::android::hardware::hidl_vec<::android::hardware::hidl_string> arg0;
        arg0.resize(func_msg.arg(0).vector_value_size());
        for (int arg0_index = 0; arg0_index < func_msg.arg(0).vector_value_size(); arg0_index++) {
            arg0[arg0_index] = ::android::hardware::hidl_string(func_msg.arg(0).vector_value(arg0_index).string_value().message());
        }
        LOG(DEBUG) << "local_device = " << hw_binder_proxy_.get();
        ::android::hardware::hidl_vec<::android::hardware::hidl_string> result0;
        hw_binder_proxy_->haveAStringVec(arg0, [&](const ::android::hardware::hidl_vec<::android::hardware::hidl_string>& arg0){
            LOG(INFO) << "callback haveAStringVec called";
            result0 = arg0;
        });
        result_msg->set_name("haveAStringVec");
        VariableSpecificationMessage* result_val_0 = result_msg->add_return_type_hidl();
        result_val_0->set_type(TYPE_VECTOR);
        result_val_0->set_vector_size(result0.size());
        for (int i = 0; i < (int)result0.size(); i++) {
            auto *result_val_0_vector_i = result_val_0->add_vector_value();
            result_val_0_vector_i->set_type(TYPE_STRING);
            result_val_0_vector_i->mutable_string_value()->set_message(result0[i].c_str());
            result_val_0_vector_i->mutable_string_value()->set_length(result0[i].size());
        }
        return true;
    }
    if (!strcmp(func_name, "transposeMe")) {
        ::android::hardware::hidl_array<float, 3, 5> arg0;
        for (int arg0_index = 0; arg0_index < func_msg.arg(0).vector_value_size(); arg0_index++) {
            for (int arg0_arg0_index__index = 0; arg0_arg0_index__index < func_msg.arg(0).vector_value(arg0_index).vector_value_size(); arg0_arg0_index__index++) {
                arg0[arg0_index][arg0_arg0_index__index] = func_msg.arg(0).vector_value(arg0_index).vector_value(arg0_arg0_index__index).scalar_value().float_t();
            }
        }
        LOG(DEBUG) << "local_device = " << hw_binder_proxy_.get();
        ::android::hardware::hidl_array<float, 5, 3> result0;
        hw_binder_proxy_->transposeMe(arg0, [&](const ::android::hardware::hidl_array<float, 5, 3>& arg0){
            LOG(INFO) << "callback transposeMe called";
            result0 = arg0;
        });
        result_msg->set_name("transposeMe");
        VariableSpecificationMessage* result_val_0 = result_msg->add_return_type_hidl();
        result_val_0->set_type(TYPE_ARRAY);
        result_val_0->set_vector_size(1);
        for (int i = 0; i < 1; i++) {
            auto *result_val_0_array_i = result_val_0->add_vector_value();
            result_val_0_array_i->set_type(TYPE_ARRAY);
            result_val_0_array_i->set_vector_size(1);
            for (int i = 0; i < 1; i++) {
                auto *result_val_0_array_i_array_i = result_val_0_array_i->add_vector_value();
                result_val_0_array_i_array_i->set_type(TYPE_SCALAR);
                result_val_0_array_i_array_i->set_scalar_type("float_t");
                result_val_0_array_i_array_i->mutable_scalar_value()->set_float_t(result0[i][i]);
            }
        }
        return true;
    }
    if (!strcmp(func_name, "callingDrWho")) {
        ::android::hardware::tests::foo::V1_0::IFoo::MultiDimensional arg0;
        MessageTo__android__hardware__tests__foo__V1_0__IFoo__MultiDimensional(func_msg.arg(0), &(arg0), callback_socket_name);
        LOG(DEBUG) << "local_device = " << hw_binder_proxy_.get();
        ::android::hardware::tests::foo::V1_0::IFoo::MultiDimensional result0;
        hw_binder_proxy_->callingDrWho(arg0, [&](const ::android::hardware::tests::foo::V1_0::IFoo::MultiDimensional& arg0){
            LOG(INFO) << "callback callingDrWho called";
            result0 = arg0;
        });
        result_msg->set_name("callingDrWho");
        VariableSpecificationMessage* result_val_0 = result_msg->add_return_type_hidl();
        result_val_0->set_type(TYPE_STRUCT);
        SetResult__android__hardware__tests__foo__V1_0__IFoo__MultiDimensional(result_val_0, result0);
        return true;
    }
    if (!strcmp(func_name, "transpose")) {
        ::android::hardware::tests::foo::V1_0::IFoo::StringMatrix5x3 arg0;
        MessageTo__android__hardware__tests__foo__V1_0__IFoo__StringMatrix5x3(func_msg.arg(0), &(arg0), callback_socket_name);
        LOG(DEBUG) << "local_device = " << hw_binder_proxy_.get();
        ::android::hardware::tests::foo::V1_0::IFoo::StringMatrix3x5 result0;
        hw_binder_proxy_->transpose(arg0, [&](const ::android::hardware::tests::foo::V1_0::IFoo::StringMatrix3x5& arg0){
            LOG(INFO) << "callback transpose called";
            result0 = arg0;
        });
        result_msg->set_name("transpose");
        VariableSpecificationMessage* result_val_0 = result_msg->add_return_type_hidl();
        result_val_0->set_type(TYPE_STRUCT);
        SetResult__android__hardware__tests__foo__V1_0__IFoo__StringMatrix3x5(result_val_0, result0);
        return true;
    }
    if (!strcmp(func_name, "transpose2")) {
        ::android::hardware::hidl_array<::android::hardware::hidl_string, 5, 3> arg0;
        for (int arg0_index = 0; arg0_index < func_msg.arg(0).vector_value_size(); arg0_index++) {
            for (int arg0_arg0_index__index = 0; arg0_arg0_index__index < func_msg.arg(0).vector_value(arg0_index).vector_value_size(); arg0_arg0_index__index++) {
                arg0[arg0_index][arg0_arg0_index__index] = ::android::hardware::hidl_string(func_msg.arg(0).vector_value(arg0_index).vector_value(arg0_arg0_index__index).string_value().message());
            }
        }
        LOG(DEBUG) << "local_device = " << hw_binder_proxy_.get();
        ::android::hardware::hidl_array<::android::hardware::hidl_string, 3, 5> result0;
        hw_binder_proxy_->transpose2(arg0, [&](const ::android::hardware::hidl_array<::android::hardware::hidl_string, 3, 5>& arg0){
            LOG(INFO) << "callback transpose2 called";
            result0 = arg0;
        });
        result_msg->set_name("transpose2");
        VariableSpecificationMessage* result_val_0 = result_msg->add_return_type_hidl();
        result_val_0->set_type(TYPE_ARRAY);
        result_val_0->set_vector_size(1);
        for (int i = 0; i < 1; i++) {
            auto *result_val_0_array_i = result_val_0->add_vector_value();
            result_val_0_array_i->set_type(TYPE_ARRAY);
            result_val_0_array_i->set_vector_size(1);
            for (int i = 0; i < 1; i++) {
                auto *result_val_0_array_i_array_i = result_val_0_array_i->add_vector_value();
                result_val_0_array_i_array_i->set_type(TYPE_STRING);
                result_val_0_array_i_array_i->mutable_string_value()->set_message(result0[i][i].c_str());
                result_val_0_array_i_array_i->mutable_string_value()->set_length(result0[i][i].size());
            }
        }
        return true;
    }
    if (!strcmp(func_name, "sendVec")) {
        ::android::hardware::hidl_vec<uint8_t> arg0;
        arg0.resize(func_msg.arg(0).vector_value_size());
        for (int arg0_index = 0; arg0_index < func_msg.arg(0).vector_value_size(); arg0_index++) {
            arg0[arg0_index] = func_msg.arg(0).vector_value(arg0_index).scalar_value().uint8_t();
        }
        LOG(DEBUG) << "local_device = " << hw_binder_proxy_.get();
        ::android::hardware::hidl_vec<uint8_t> result0;
        hw_binder_proxy_->sendVec(arg0, [&](const ::android::hardware::hidl_vec<uint8_t>& arg0){
            LOG(INFO) << "callback sendVec called";
            result0 = arg0;
        });
        result_msg->set_name("sendVec");
        VariableSpecificationMessage* result_val_0 = result_msg->add_return_type_hidl();
        result_val_0->set_type(TYPE_VECTOR);
        result_val_0->set_vector_size(result0.size());
        for (int i = 0; i < (int)result0.size(); i++) {
            auto *result_val_0_vector_i = result_val_0->add_vector_value();
            result_val_0_vector_i->set_type(TYPE_SCALAR);
            result_val_0_vector_i->set_scalar_type("uint8_t");
            result_val_0_vector_i->mutable_scalar_value()->set_uint8_t(result0[i]);
        }
        return true;
    }
    if (!strcmp(func_name, "sendVecVec")) {
        LOG(DEBUG) << "local_device = " << hw_binder_proxy_.get();
        ::android::hardware::hidl_vec<::android::hardware::hidl_vec<uint8_t>> result0;
        hw_binder_proxy_->sendVecVec([&](const ::android::hardware::hidl_vec<::android::hardware::hidl_vec<uint8_t>>& arg0){
            LOG(INFO) << "callback sendVecVec called";
            result0 = arg0;
        });
        result_msg->set_name("sendVecVec");
        VariableSpecificationMessage* result_val_0 = result_msg->add_return_type_hidl();
        result_val_0->set_type(TYPE_VECTOR);
        result_val_0->set_vector_size(result0.size());
        for (int i = 0; i < (int)result0.size(); i++) {
            auto *result_val_0_vector_i = result_val_0->add_vector_value();
            result_val_0_vector_i->set_type(TYPE_VECTOR);
            result_val_0_vector_i->set_vector_size(result0[i].size());
            for (int i = 0; i < (int)result0[i].size(); i++) {
                auto *result_val_0_vector_i_vector_i = result_val_0_vector_i->add_vector_value();
                result_val_0_vector_i_vector_i->set_type(TYPE_SCALAR);
                result_val_0_vector_i_vector_i->set_scalar_type("uint8_t");
                result_val_0_vector_i_vector_i->mutable_scalar_value()->set_uint8_t(result0[i][i]);
            }
        }
        return true;
    }
    if (!strcmp(func_name, "haveAVectorOfInterfaces")) {
        ::android::hardware::hidl_vec<sp<::android::hardware::tests::foo::V1_0::ISimple>> arg0;
        arg0.resize(func_msg.arg(0).vector_value_size());
        for (int arg0_index = 0; arg0_index < func_msg.arg(0).vector_value_size(); arg0_index++) {
            if (func_msg.arg(0).vector_value(arg0_index).has_hidl_interface_pointer()) {
                arg0[arg0_index] = reinterpret_cast<::android::hardware::tests::foo::V1_0::ISimple*>(func_msg.arg(0).vector_value(arg0_index).hidl_interface_pointer());
            } else {
                arg0[arg0_index] = VtsFuzzerCreateVts_android_hardware_tests_foo_V1_0_ISimple(callback_socket_name);
            }
        }
        LOG(DEBUG) << "local_device = " << hw_binder_proxy_.get();
        ::android::hardware::hidl_vec<sp<::android::hardware::tests::foo::V1_0::ISimple>> result0;
        hw_binder_proxy_->haveAVectorOfInterfaces(arg0, [&](const ::android::hardware::hidl_vec<sp<::android::hardware::tests::foo::V1_0::ISimple>>& arg0){
            LOG(INFO) << "callback haveAVectorOfInterfaces called";
            result0 = arg0;
        });
        result_msg->set_name("haveAVectorOfInterfaces");
        VariableSpecificationMessage* result_val_0 = result_msg->add_return_type_hidl();
        result_val_0->set_type(TYPE_VECTOR);
        result_val_0->set_vector_size(result0.size());
        for (int i = 0; i < (int)result0.size(); i++) {
            auto *result_val_0_vector_i = result_val_0->add_vector_value();
            result_val_0_vector_i->set_type(TYPE_HIDL_INTERFACE);
            result_val_0_vector_i->set_predefined_type("::android::hardware::tests::foo::V1_0::ISimple");
            if (result0[i] != nullptr) {
                result0[i]->incStrong(result0[i].get());
                result_val_0_vector_i->set_hidl_interface_pointer(reinterpret_cast<uintptr_t>(result0[i].get()));
            } else {
                result_val_0_vector_i->set_hidl_interface_pointer(0);
            }
        }
        return true;
    }
    if (!strcmp(func_name, "haveAVectorOfGenericInterfaces")) {
        ::android::hardware::hidl_vec<sp<::android::hidl::base::V1_0::IBase>> arg0;
        arg0.resize(func_msg.arg(0).vector_value_size());
        for (int arg0_index = 0; arg0_index < func_msg.arg(0).vector_value_size(); arg0_index++) {
            if (func_msg.arg(0).vector_value(arg0_index).has_hidl_interface_pointer()) {
                arg0[arg0_index] = reinterpret_cast<::android::hidl::base::V1_0::IBase*>(func_msg.arg(0).vector_value(arg0_index).hidl_interface_pointer());
            } else {
                /* ERROR: general interface is not supported yet. */
            }
        }
        LOG(DEBUG) << "local_device = " << hw_binder_proxy_.get();
        ::android::hardware::hidl_vec<sp<::android::hidl::base::V1_0::IBase>> result0;
        hw_binder_proxy_->haveAVectorOfGenericInterfaces(arg0, [&](const ::android::hardware::hidl_vec<sp<::android::hidl::base::V1_0::IBase>>& arg0){
            LOG(INFO) << "callback haveAVectorOfGenericInterfaces called";
            result0 = arg0;
        });
        result_msg->set_name("haveAVectorOfGenericInterfaces");
        VariableSpecificationMessage* result_val_0 = result_msg->add_return_type_hidl();
        result_val_0->set_type(TYPE_VECTOR);
        result_val_0->set_vector_size(result0.size());
        for (int i = 0; i < (int)result0.size(); i++) {
            auto *result_val_0_vector_i = result_val_0->add_vector_value();
            result_val_0_vector_i->set_type(TYPE_HIDL_INTERFACE);
            result_val_0_vector_i->set_predefined_type("::android::hidl::base::V1_0::IBase");
            if (result0[i] != nullptr) {
                result0[i]->incStrong(result0[i].get());
                result_val_0_vector_i->set_hidl_interface_pointer(reinterpret_cast<uintptr_t>(result0[i].get()));
            } else {
                result_val_0_vector_i->set_hidl_interface_pointer(0);
            }
        }
        return true;
    }
    if (!strcmp(func_name, "echoNullInterface")) {
        sp<::android::hardware::tests::foo::V1_0::IFooCallback> arg0;
        arg0 = VtsFuzzerCreateVts_android_hardware_tests_foo_V1_0_IFooCallback(callback_socket_name);
        static_cast<Vts_android_hardware_tests_foo_V1_0_IFooCallback*>(arg0.get())->Register(func_msg.arg(0));
        LOG(DEBUG) << "local_device = " << hw_binder_proxy_.get();
        bool result0;
        sp<::android::hardware::tests::foo::V1_0::IFooCallback> result1;
        hw_binder_proxy_->echoNullInterface(arg0, [&](bool arg0,const sp<::android::hardware::tests::foo::V1_0::IFooCallback>& arg1){
            LOG(INFO) << "callback echoNullInterface called";
            result0 = arg0;
            result1 = arg1;
        });
        result_msg->set_name("echoNullInterface");
        VariableSpecificationMessage* result_val_0 = result_msg->add_return_type_hidl();
        result_val_0->set_type(TYPE_SCALAR);
        result_val_0->set_scalar_type("bool_t");
        result_val_0->mutable_scalar_value()->set_bool_t(result0);
        VariableSpecificationMessage* result_val_1 = result_msg->add_return_type_hidl();
        result_val_1->set_type(TYPE_HIDL_CALLBACK);
        /* ERROR: TYPE_HIDL_CALLBACK is not supported yet. */
        return true;
    }
    if (!strcmp(func_name, "createMyHandle")) {
        LOG(DEBUG) << "local_device = " << hw_binder_proxy_.get();
        ::android::hardware::tests::foo::V1_0::IFoo::MyHandle result0;
        hw_binder_proxy_->createMyHandle([&](const ::android::hardware::tests::foo::V1_0::IFoo::MyHandle& arg0){
            LOG(INFO) << "callback createMyHandle called";
            result0 = arg0;
        });
        result_msg->set_name("createMyHandle");
        VariableSpecificationMessage* result_val_0 = result_msg->add_return_type_hidl();
        result_val_0->set_type(TYPE_STRUCT);
        SetResult__android__hardware__tests__foo__V1_0__IFoo__MyHandle(result_val_0, result0);
        return true;
    }
    if (!strcmp(func_name, "createHandles")) {
        uint32_t arg0 = 0;
        arg0 = func_msg.arg(0).scalar_value().uint32_t();
        LOG(DEBUG) << "local_device = " << hw_binder_proxy_.get();
        ::android::hardware::hidl_vec<::android::hardware::hidl_handle> result0;
        hw_binder_proxy_->createHandles(arg0, [&](const ::android::hardware::hidl_vec<::android::hardware::hidl_handle>& arg0){
            LOG(INFO) << "callback createHandles called";
            result0 = arg0;
        });
        result_msg->set_name("createHandles");
        VariableSpecificationMessage* result_val_0 = result_msg->add_return_type_hidl();
        result_val_0->set_type(TYPE_VECTOR);
        result_val_0->set_vector_size(result0.size());
        for (int i = 0; i < (int)result0.size(); i++) {
            auto *result_val_0_vector_i = result_val_0->add_vector_value();
            result_val_0_vector_i->set_type(TYPE_HANDLE);
            /* ERROR: TYPE_HANDLE is not supported yet. */
        }
        return true;
    }
    if (!strcmp(func_name, "closeHandles")) {
        LOG(DEBUG) << "local_device = " << hw_binder_proxy_.get();
        hw_binder_proxy_->closeHandles();
        result_msg->set_name("closeHandles");
        return true;
    }
    if (!strcmp(func_name, "thisIsNew")) {
        LOG(DEBUG) << "local_device = " << hw_binder_proxy_.get();
        hw_binder_proxy_->thisIsNew();
        result_msg->set_name("thisIsNew");
        return true;
    }
    if (!strcmp(func_name, "expectNullHandle")) {
        ::android::hardware::hidl_handle arg0;
        if (func_msg.arg(0).has_handle_value()) {
            native_handle_t* handle = native_handle_create(func_msg.arg(0).handle_value().num_fds(), func_msg.arg(0).handle_value().num_ints());
            if (!handle) {
                LOG(ERROR) << "Failed to create handle. ";
                exit(-1);
            }
            for (int fd_index = 0; fd_index < func_msg.arg(0).handle_value().num_fds() + func_msg.arg(0).handle_value().num_ints(); fd_index++) {
                if (fd_index < func_msg.arg(0).handle_value().num_fds()) {
                    FdMessage fd_val = func_msg.arg(0).handle_value().fd_val(fd_index);
                    string file_name = fd_val.file_name();
                    switch (fd_val.type()) {
                        case FdType::FILE_TYPE:
                        {
                            size_t pre = 0; size_t pos = 0;
                            string dir;
                            struct stat st;
                            while((pos=file_name.find_first_of('/', pre)) != string::npos){
                                dir = file_name.substr(0, pos++);
                                pre = pos;
                                if(dir.size() == 0) continue; // ignore leading /
                                if (stat(dir.c_str(), &st) == -1) {
                                LOG(INFO) << " Creating dir: " << dir;
                                    mkdir(dir.c_str(), 0700);
                                }
                            }
                            int fd = open(file_name.c_str(), fd_val.flags() | O_CREAT, fd_val.mode());
                            if (fd == -1) {
                                LOG(ERROR) << "Failed to open file: " << file_name << " error: " << errno;
                                exit (-1);
                            }
                            handle->data[fd_index] = fd;
                            break;
                        }
                        case FdType::DIR_TYPE:
                        {
                            struct stat st;
                            if (!stat(file_name.c_str(), &st)) {
                                mkdir(file_name.c_str(), fd_val.mode());
                            }
                            handle->data[fd_index] = open(file_name.c_str(), O_DIRECTORY, fd_val.mode());
                            break;
                        }
                        case FdType::DEV_TYPE:
                        {
                            if(file_name == "/dev/ashmem") {
                                handle->data[fd_index] = ashmem_create_region("SharedMemory", fd_val.memory().size());
                            }
                            break;
                        }
                        case FdType::PIPE_TYPE:
                        case FdType::SOCKET_TYPE:
                        case FdType::LINK_TYPE:
                        {
                            LOG(ERROR) << "Not supported yet. ";
                            break;
                        }
                    }
                } else {
                    handle->data[fd_index] = func_msg.arg(0).handle_value().int_val(fd_index -func_msg.arg(0).handle_value().num_fds());
                }
            }
            arg0 = handle;
        } else {
            arg0 = nullptr;
        }
        ::android::hardware::tests::foo::V1_0::Abc arg1;
        MessageTo__android__hardware__tests__foo__V1_0__Abc(func_msg.arg(1), &(arg1), callback_socket_name);
        LOG(DEBUG) << "local_device = " << hw_binder_proxy_.get();
        bool result0;
        bool result1;
        hw_binder_proxy_->expectNullHandle(arg0, arg1, [&](bool arg0,bool arg1){
            LOG(INFO) << "callback expectNullHandle called";
            result0 = arg0;
            result1 = arg1;
        });
        result_msg->set_name("expectNullHandle");
        VariableSpecificationMessage* result_val_0 = result_msg->add_return_type_hidl();
        result_val_0->set_type(TYPE_SCALAR);
        result_val_0->set_scalar_type("bool_t");
        result_val_0->mutable_scalar_value()->set_bool_t(result0);
        VariableSpecificationMessage* result_val_1 = result_msg->add_return_type_hidl();
        result_val_1->set_type(TYPE_SCALAR);
        result_val_1->set_scalar_type("bool_t");
        result_val_1->mutable_scalar_value()->set_bool_t(result1);
        return true;
    }
    if (!strcmp(func_name, "takeAMask")) {
        ::android::hardware::tests::foo::V1_0::IFoo::BitField arg0;
        arg0 = EnumValue__android__hardware__tests__foo__V1_0__IFoo__BitField(func_msg.arg(0).scalar_value());
        uint8_t arg1;
        arg1 = func_msg.arg(1).scalar_value().uint8_t();
        ::android::hardware::tests::foo::V1_0::IFoo::MyMask arg2;
        MessageTo__android__hardware__tests__foo__V1_0__IFoo__MyMask(func_msg.arg(2), &(arg2), callback_socket_name);
        uint8_t arg3;
        arg3 = func_msg.arg(3).scalar_value().uint8_t();
        LOG(DEBUG) << "local_device = " << hw_binder_proxy_.get();
        ::android::hardware::tests::foo::V1_0::IFoo::BitField result0;
        uint8_t result1;
        uint8_t result2;
        uint8_t result3;
        hw_binder_proxy_->takeAMask(arg0, arg1, arg2, arg3, [&](::android::hardware::tests::foo::V1_0::IFoo::BitField arg0,uint8_t arg1,uint8_t arg2,uint8_t arg3){
            LOG(INFO) << "callback takeAMask called";
            result0 = arg0;
            result1 = arg1;
            result2 = arg2;
            result3 = arg3;
        });
        result_msg->set_name("takeAMask");
        VariableSpecificationMessage* result_val_0 = result_msg->add_return_type_hidl();
        result_val_0->set_type(TYPE_ENUM);
        SetResult__android__hardware__tests__foo__V1_0__IFoo__BitField(result_val_0, result0);
        VariableSpecificationMessage* result_val_1 = result_msg->add_return_type_hidl();
        result_val_1->set_type(TYPE_SCALAR);
        result_val_1->set_scalar_type("uint8_t");
        result_val_1->mutable_scalar_value()->set_uint8_t(result1);
        VariableSpecificationMessage* result_val_2 = result_msg->add_return_type_hidl();
        result_val_2->set_type(TYPE_SCALAR);
        result_val_2->set_scalar_type("uint8_t");
        result_val_2->mutable_scalar_value()->set_uint8_t(result2);
        VariableSpecificationMessage* result_val_3 = result_msg->add_return_type_hidl();
        result_val_3->set_type(TYPE_SCALAR);
        result_val_3->set_scalar_type("uint8_t");
        result_val_3->mutable_scalar_value()->set_uint8_t(result3);
        return true;
    }
    if (!strcmp(func_name, "haveAInterface")) {
        sp<::android::hardware::tests::foo::V1_0::ISimple> arg0;
        if (func_msg.arg(0).has_hidl_interface_pointer()) {
            arg0 = reinterpret_cast<::android::hardware::tests::foo::V1_0::ISimple*>(func_msg.arg(0).hidl_interface_pointer());
        } else {
            arg0 = VtsFuzzerCreateVts_android_hardware_tests_foo_V1_0_ISimple(callback_socket_name);
        }
        LOG(DEBUG) << "local_device = " << hw_binder_proxy_.get();
        sp<::android::hardware::tests::foo::V1_0::ISimple> result0;
        result0 = hw_binder_proxy_->haveAInterface(arg0);
        result_msg->set_name("haveAInterface");
        VariableSpecificationMessage* result_val_0 = result_msg->add_return_type_hidl();
        result_val_0->set_type(TYPE_HIDL_INTERFACE);
        result_val_0->set_predefined_type("::android::hardware::tests::foo::V1_0::ISimple");
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

bool FuzzerExtended_android_hardware_tests_bar_V1_0_IBar::VerifyResults(const FunctionSpecificationMessage& expected_result __attribute__((__unused__)),
    const FunctionSpecificationMessage& actual_result __attribute__((__unused__))) {
    if (!strcmp(actual_result.name().c_str(), "convertToBoolIfSmall")) {
        if (actual_result.return_type_hidl_size() != expected_result.return_type_hidl_size() ) { return false; }
        if (actual_result.return_type_hidl(0).vector_value_size() != expected_result.return_type_hidl(0).vector_value_size()) {
            LOG(ERROR) << "Verification failed for vector size. expected: " << expected_result.return_type_hidl(0).vector_value_size() << " actual: " << actual_result.return_type_hidl(0).vector_value_size();
            return false;
        }
        for (int i = 0; i <expected_result.return_type_hidl(0).vector_value_size(); i++) {
            if (!Verify__android__hardware__tests__foo__V1_0__IFoo__ContainsUnion(expected_result.return_type_hidl(0).vector_value(i), actual_result.return_type_hidl(0).vector_value(i))) { return false; }
        }
        return true;
    }
    if (!strcmp(actual_result.name().c_str(), "doThis")) {
        if (actual_result.return_type_hidl_size() != expected_result.return_type_hidl_size() ) { return false; }
        return true;
    }
    if (!strcmp(actual_result.name().c_str(), "doThatAndReturnSomething")) {
        if (actual_result.return_type_hidl_size() != expected_result.return_type_hidl_size() ) { return false; }
        if (actual_result.return_type_hidl(0).scalar_value().int32_t() != expected_result.return_type_hidl(0).scalar_value().int32_t()) { return false; }
        return true;
    }
    if (!strcmp(actual_result.name().c_str(), "doQuiteABit")) {
        if (actual_result.return_type_hidl_size() != expected_result.return_type_hidl_size() ) { return false; }
        if (actual_result.return_type_hidl(0).scalar_value().double_t() != expected_result.return_type_hidl(0).scalar_value().double_t()) { return false; }
        return true;
    }
    if (!strcmp(actual_result.name().c_str(), "doSomethingElse")) {
        if (actual_result.return_type_hidl_size() != expected_result.return_type_hidl_size() ) { return false; }
        if (actual_result.return_type_hidl(0).vector_value_size() != expected_result.return_type_hidl(0).vector_value_size()) {
            LOG(ERROR) << "Verification failed for vector size. expected: " << expected_result.return_type_hidl(0).vector_value_size() << " actual: " << actual_result.return_type_hidl(0).vector_value_size();
            return false;
        }
        for (int i = 0; i < expected_result.return_type_hidl(0).vector_value_size(); i++) {
            if (actual_result.return_type_hidl(0).vector_value(i).scalar_value().int32_t() != expected_result.return_type_hidl(0).vector_value(i).scalar_value().int32_t()) { return false; }
        }
        return true;
    }
    if (!strcmp(actual_result.name().c_str(), "doStuffAndReturnAString")) {
        if (actual_result.return_type_hidl_size() != expected_result.return_type_hidl_size() ) { return false; }
        if (strcmp(actual_result.return_type_hidl(0).string_value().message().c_str(), expected_result.return_type_hidl(0).string_value().message().c_str())!= 0){ return false; }
        return true;
    }
    if (!strcmp(actual_result.name().c_str(), "mapThisVector")) {
        if (actual_result.return_type_hidl_size() != expected_result.return_type_hidl_size() ) { return false; }
        if (actual_result.return_type_hidl(0).vector_value_size() != expected_result.return_type_hidl(0).vector_value_size()) {
            LOG(ERROR) << "Verification failed for vector size. expected: " << expected_result.return_type_hidl(0).vector_value_size() << " actual: " << actual_result.return_type_hidl(0).vector_value_size();
            return false;
        }
        for (int i = 0; i <expected_result.return_type_hidl(0).vector_value_size(); i++) {
            if (actual_result.return_type_hidl(0).vector_value(i).scalar_value().int32_t() != expected_result.return_type_hidl(0).vector_value(i).scalar_value().int32_t()) { return false; }
        }
        return true;
    }
    if (!strcmp(actual_result.name().c_str(), "callMe")) {
        if (actual_result.return_type_hidl_size() != expected_result.return_type_hidl_size() ) { return false; }
        return true;
    }
    if (!strcmp(actual_result.name().c_str(), "useAnEnum")) {
        if (actual_result.return_type_hidl_size() != expected_result.return_type_hidl_size() ) { return false; }
        if(!Verify__android__hardware__tests__foo__V1_0__IFoo__SomeEnum(expected_result.return_type_hidl(0), actual_result.return_type_hidl(0))) { return false; }
        return true;
    }
    if (!strcmp(actual_result.name().c_str(), "haveAGooberVec")) {
        if (actual_result.return_type_hidl_size() != expected_result.return_type_hidl_size() ) { return false; }
        return true;
    }
    if (!strcmp(actual_result.name().c_str(), "haveAGoober")) {
        if (actual_result.return_type_hidl_size() != expected_result.return_type_hidl_size() ) { return false; }
        return true;
    }
    if (!strcmp(actual_result.name().c_str(), "haveAGooberArray")) {
        if (actual_result.return_type_hidl_size() != expected_result.return_type_hidl_size() ) { return false; }
        return true;
    }
    if (!strcmp(actual_result.name().c_str(), "haveATypeFromAnotherFile")) {
        if (actual_result.return_type_hidl_size() != expected_result.return_type_hidl_size() ) { return false; }
        return true;
    }
    if (!strcmp(actual_result.name().c_str(), "haveSomeStrings")) {
        if (actual_result.return_type_hidl_size() != expected_result.return_type_hidl_size() ) { return false; }
        if (actual_result.return_type_hidl(0).vector_value_size() != expected_result.return_type_hidl(0).vector_value_size()) {
            LOG(ERROR) << "Verification failed for vector size. expected: " << expected_result.return_type_hidl(0).vector_value_size() << " actual: " << actual_result.return_type_hidl(0).vector_value_size();
            return false;
        }
        for (int i = 0; i < expected_result.return_type_hidl(0).vector_value_size(); i++) {
            if (strcmp(actual_result.return_type_hidl(0).vector_value(i).string_value().message().c_str(), expected_result.return_type_hidl(0).vector_value(i).string_value().message().c_str())!= 0){ return false; }
        }
        return true;
    }
    if (!strcmp(actual_result.name().c_str(), "haveAStringVec")) {
        if (actual_result.return_type_hidl_size() != expected_result.return_type_hidl_size() ) { return false; }
        if (actual_result.return_type_hidl(0).vector_value_size() != expected_result.return_type_hidl(0).vector_value_size()) {
            LOG(ERROR) << "Verification failed for vector size. expected: " << expected_result.return_type_hidl(0).vector_value_size() << " actual: " << actual_result.return_type_hidl(0).vector_value_size();
            return false;
        }
        for (int i = 0; i <expected_result.return_type_hidl(0).vector_value_size(); i++) {
            if (strcmp(actual_result.return_type_hidl(0).vector_value(i).string_value().message().c_str(), expected_result.return_type_hidl(0).vector_value(i).string_value().message().c_str())!= 0){ return false; }
        }
        return true;
    }
    if (!strcmp(actual_result.name().c_str(), "transposeMe")) {
        if (actual_result.return_type_hidl_size() != expected_result.return_type_hidl_size() ) { return false; }
        if (actual_result.return_type_hidl(0).vector_value_size() != expected_result.return_type_hidl(0).vector_value_size()) {
            LOG(ERROR) << "Verification failed for vector size. expected: " << expected_result.return_type_hidl(0).vector_value_size() << " actual: " << actual_result.return_type_hidl(0).vector_value_size();
            return false;
        }
        for (int i = 0; i < expected_result.return_type_hidl(0).vector_value_size(); i++) {
            if (actual_result.return_type_hidl(0).vector_value(i).vector_value_size() != expected_result.return_type_hidl(0).vector_value(i).vector_value_size()) {
                LOG(ERROR) << "Verification failed for vector size. expected: " << expected_result.return_type_hidl(0).vector_value(i).vector_value_size() << " actual: " << actual_result.return_type_hidl(0).vector_value(i).vector_value_size();
                return false;
            }
            for (int i = 0; i < expected_result.return_type_hidl(0).vector_value(i).vector_value_size(); i++) {
                if (actual_result.return_type_hidl(0).vector_value(i).vector_value(i).scalar_value().float_t() != expected_result.return_type_hidl(0).vector_value(i).vector_value(i).scalar_value().float_t()) { return false; }
            }
        }
        return true;
    }
    if (!strcmp(actual_result.name().c_str(), "callingDrWho")) {
        if (actual_result.return_type_hidl_size() != expected_result.return_type_hidl_size() ) { return false; }
        if (!Verify__android__hardware__tests__foo__V1_0__IFoo__MultiDimensional(expected_result.return_type_hidl(0), actual_result.return_type_hidl(0))) { return false; }
        return true;
    }
    if (!strcmp(actual_result.name().c_str(), "transpose")) {
        if (actual_result.return_type_hidl_size() != expected_result.return_type_hidl_size() ) { return false; }
        if (!Verify__android__hardware__tests__foo__V1_0__IFoo__StringMatrix3x5(expected_result.return_type_hidl(0), actual_result.return_type_hidl(0))) { return false; }
        return true;
    }
    if (!strcmp(actual_result.name().c_str(), "transpose2")) {
        if (actual_result.return_type_hidl_size() != expected_result.return_type_hidl_size() ) { return false; }
        if (actual_result.return_type_hidl(0).vector_value_size() != expected_result.return_type_hidl(0).vector_value_size()) {
            LOG(ERROR) << "Verification failed for vector size. expected: " << expected_result.return_type_hidl(0).vector_value_size() << " actual: " << actual_result.return_type_hidl(0).vector_value_size();
            return false;
        }
        for (int i = 0; i < expected_result.return_type_hidl(0).vector_value_size(); i++) {
            if (actual_result.return_type_hidl(0).vector_value(i).vector_value_size() != expected_result.return_type_hidl(0).vector_value(i).vector_value_size()) {
                LOG(ERROR) << "Verification failed for vector size. expected: " << expected_result.return_type_hidl(0).vector_value(i).vector_value_size() << " actual: " << actual_result.return_type_hidl(0).vector_value(i).vector_value_size();
                return false;
            }
            for (int i = 0; i < expected_result.return_type_hidl(0).vector_value(i).vector_value_size(); i++) {
                if (strcmp(actual_result.return_type_hidl(0).vector_value(i).vector_value(i).string_value().message().c_str(), expected_result.return_type_hidl(0).vector_value(i).vector_value(i).string_value().message().c_str())!= 0){ return false; }
            }
        }
        return true;
    }
    if (!strcmp(actual_result.name().c_str(), "sendVec")) {
        if (actual_result.return_type_hidl_size() != expected_result.return_type_hidl_size() ) { return false; }
        if (actual_result.return_type_hidl(0).vector_value_size() != expected_result.return_type_hidl(0).vector_value_size()) {
            LOG(ERROR) << "Verification failed for vector size. expected: " << expected_result.return_type_hidl(0).vector_value_size() << " actual: " << actual_result.return_type_hidl(0).vector_value_size();
            return false;
        }
        for (int i = 0; i <expected_result.return_type_hidl(0).vector_value_size(); i++) {
            if (actual_result.return_type_hidl(0).vector_value(i).scalar_value().uint8_t() != expected_result.return_type_hidl(0).vector_value(i).scalar_value().uint8_t()) { return false; }
        }
        return true;
    }
    if (!strcmp(actual_result.name().c_str(), "sendVecVec")) {
        if (actual_result.return_type_hidl_size() != expected_result.return_type_hidl_size() ) { return false; }
        if (actual_result.return_type_hidl(0).vector_value_size() != expected_result.return_type_hidl(0).vector_value_size()) {
            LOG(ERROR) << "Verification failed for vector size. expected: " << expected_result.return_type_hidl(0).vector_value_size() << " actual: " << actual_result.return_type_hidl(0).vector_value_size();
            return false;
        }
        for (int i = 0; i <expected_result.return_type_hidl(0).vector_value_size(); i++) {
            if (actual_result.return_type_hidl(0).vector_value(i).vector_value_size() != expected_result.return_type_hidl(0).vector_value(i).vector_value_size()) {
                LOG(ERROR) << "Verification failed for vector size. expected: " << expected_result.return_type_hidl(0).vector_value(i).vector_value_size() << " actual: " << actual_result.return_type_hidl(0).vector_value(i).vector_value_size();
                return false;
            }
            for (int i = 0; i <expected_result.return_type_hidl(0).vector_value(i).vector_value_size(); i++) {
                if (actual_result.return_type_hidl(0).vector_value(i).vector_value(i).scalar_value().uint8_t() != expected_result.return_type_hidl(0).vector_value(i).vector_value(i).scalar_value().uint8_t()) { return false; }
            }
        }
        return true;
    }
    if (!strcmp(actual_result.name().c_str(), "haveAVectorOfInterfaces")) {
        if (actual_result.return_type_hidl_size() != expected_result.return_type_hidl_size() ) { return false; }
        if (actual_result.return_type_hidl(0).vector_value_size() != expected_result.return_type_hidl(0).vector_value_size()) {
            LOG(ERROR) << "Verification failed for vector size. expected: " << expected_result.return_type_hidl(0).vector_value_size() << " actual: " << actual_result.return_type_hidl(0).vector_value_size();
            return false;
        }
        for (int i = 0; i <expected_result.return_type_hidl(0).vector_value_size(); i++) {
            /* ERROR: TYPE_HIDL_INTERFACE is not supported yet. */
        }
        return true;
    }
    if (!strcmp(actual_result.name().c_str(), "haveAVectorOfGenericInterfaces")) {
        if (actual_result.return_type_hidl_size() != expected_result.return_type_hidl_size() ) { return false; }
        if (actual_result.return_type_hidl(0).vector_value_size() != expected_result.return_type_hidl(0).vector_value_size()) {
            LOG(ERROR) << "Verification failed for vector size. expected: " << expected_result.return_type_hidl(0).vector_value_size() << " actual: " << actual_result.return_type_hidl(0).vector_value_size();
            return false;
        }
        for (int i = 0; i <expected_result.return_type_hidl(0).vector_value_size(); i++) {
            /* ERROR: TYPE_HIDL_INTERFACE is not supported yet. */
        }
        return true;
    }
    if (!strcmp(actual_result.name().c_str(), "echoNullInterface")) {
        if (actual_result.return_type_hidl_size() != expected_result.return_type_hidl_size() ) { return false; }
        if (actual_result.return_type_hidl(0).scalar_value().bool_t() != expected_result.return_type_hidl(0).scalar_value().bool_t()) { return false; }
        /* ERROR: TYPE_HIDL_CALLBACK is not supported yet. */
        return true;
    }
    if (!strcmp(actual_result.name().c_str(), "createMyHandle")) {
        if (actual_result.return_type_hidl_size() != expected_result.return_type_hidl_size() ) { return false; }
        if (!Verify__android__hardware__tests__foo__V1_0__IFoo__MyHandle(expected_result.return_type_hidl(0), actual_result.return_type_hidl(0))) { return false; }
        return true;
    }
    if (!strcmp(actual_result.name().c_str(), "createHandles")) {
        if (actual_result.return_type_hidl_size() != expected_result.return_type_hidl_size() ) { return false; }
        if (actual_result.return_type_hidl(0).vector_value_size() != expected_result.return_type_hidl(0).vector_value_size()) {
            LOG(ERROR) << "Verification failed for vector size. expected: " << expected_result.return_type_hidl(0).vector_value_size() << " actual: " << actual_result.return_type_hidl(0).vector_value_size();
            return false;
        }
        for (int i = 0; i <expected_result.return_type_hidl(0).vector_value_size(); i++) {
            /* ERROR: TYPE_HANDLE is not supported yet. */
        }
        return true;
    }
    if (!strcmp(actual_result.name().c_str(), "closeHandles")) {
        if (actual_result.return_type_hidl_size() != expected_result.return_type_hidl_size() ) { return false; }
        return true;
    }
    if (!strcmp(actual_result.name().c_str(), "thisIsNew")) {
        if (actual_result.return_type_hidl_size() != expected_result.return_type_hidl_size() ) { return false; }
        return true;
    }
    if (!strcmp(actual_result.name().c_str(), "expectNullHandle")) {
        if (actual_result.return_type_hidl_size() != expected_result.return_type_hidl_size() ) { return false; }
        if (actual_result.return_type_hidl(0).scalar_value().bool_t() != expected_result.return_type_hidl(0).scalar_value().bool_t()) { return false; }
        if (actual_result.return_type_hidl(1).scalar_value().bool_t() != expected_result.return_type_hidl(1).scalar_value().bool_t()) { return false; }
        return true;
    }
    if (!strcmp(actual_result.name().c_str(), "takeAMask")) {
        if (actual_result.return_type_hidl_size() != expected_result.return_type_hidl_size() ) { return false; }
        if(!Verify__android__hardware__tests__foo__V1_0__IFoo__BitField(expected_result.return_type_hidl(0), actual_result.return_type_hidl(0))) { return false; }
        if (actual_result.return_type_hidl(1).scalar_value().uint8_t() != expected_result.return_type_hidl(1).scalar_value().uint8_t()) { return false; }
        if (actual_result.return_type_hidl(2).scalar_value().uint8_t() != expected_result.return_type_hidl(2).scalar_value().uint8_t()) { return false; }
        if (actual_result.return_type_hidl(3).scalar_value().uint8_t() != expected_result.return_type_hidl(3).scalar_value().uint8_t()) { return false; }
        return true;
    }
    if (!strcmp(actual_result.name().c_str(), "haveAInterface")) {
        if (actual_result.return_type_hidl_size() != expected_result.return_type_hidl_size() ) { return false; }
        /* ERROR: TYPE_HIDL_INTERFACE is not supported yet. */
        return true;
    }
    return false;
}

extern "C" {
android::vts::DriverBase* vts_func_4_android_hardware_tests_bar_V1_0_IBar_() {
    return (android::vts::DriverBase*) new android::vts::FuzzerExtended_android_hardware_tests_bar_V1_0_IBar();
}

android::vts::DriverBase* vts_func_4_android_hardware_tests_bar_V1_0_IBar_with_arg(uint64_t hw_binder_proxy) {
    ::android::hardware::tests::bar::V1_0::IBar* arg = nullptr;
    if (hw_binder_proxy) {
        arg = reinterpret_cast<::android::hardware::tests::bar::V1_0::IBar*>(hw_binder_proxy);
    } else {
        LOG(INFO) << " Creating DriverBase with null proxy.";
    }
    android::vts::DriverBase* result =
        new android::vts::FuzzerExtended_android_hardware_tests_bar_V1_0_IBar(
            arg);
    if (arg != nullptr) {
        arg->decStrong(arg);
    }
    return result;
}

}
}  // namespace vts
}  // namespace android
