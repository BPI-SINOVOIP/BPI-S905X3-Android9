#include "android/hardware/tests/bar/1.0/Bar.vts.h"
#include <cutils/ashmem.h>
#include <fcntl.h>
#include <fmq/MessageQueue.h>
#include <sys/stat.h>

using namespace android::hardware::tests::bar::V1_0;
using namespace android::hardware;

#define TRACEFILEPREFIX "/data/local/tmp/"

namespace android {
namespace vts {
void profile____android__hardware__tests__bar__V1_0__IBar__SomethingRelated(VariableSpecificationMessage* arg_name,
::android::hardware::tests::bar::V1_0::IBar::SomethingRelated arg_val_name __attribute__((__unused__))) {
    arg_name->set_type(TYPE_STRUCT);
    auto *arg_name_myRelated __attribute__((__unused__)) = arg_name->add_struct_value();
    arg_name_myRelated->set_type(TYPE_STRUCT);
    profile____android__hardware__tests__foo__V1_0__Unrelated(arg_name_myRelated, arg_val_name.myRelated);
}


void HIDL_INSTRUMENTATION_FUNCTION_android_hardware_tests_bar_V1_0_IBar(
        details::HidlInstrumentor::InstrumentationEvent event __attribute__((__unused__)),
        const char* package,
        const char* version,
        const char* interface,
        const char* method __attribute__((__unused__)),
        std::vector<void *> *args __attribute__((__unused__))) {
    if (strcmp(package, "android.hardware.tests.bar") != 0) {
        LOG(WARNING) << "incorrect package. Expect: android.hardware.tests.bar actual: " << package;
        return;
    }
    std::string version_str = std::string(version);
    int major_version = stoi(version_str.substr(0, version_str.find('.')));
    int minor_version = stoi(version_str.substr(version_str.find('.') + 1));
    if (major_version != 1 || minor_version > 0) {
        LOG(WARNING) << "incorrect version. Expect: 1.0 or lower (if version != x.0), actual: " << version;
        return;
    }
    if (strcmp(interface, "IBar") != 0) {
        LOG(WARNING) << "incorrect interface. Expect: IBar actual: " << interface;
        return;
    }

    VtsProfilingInterface& profiler = VtsProfilingInterface::getInstance(TRACEFILEPREFIX);

    if (strcmp(method, "convertToBoolIfSmall") == 0) {
        FunctionSpecificationMessage msg;
        msg.set_name("convertToBoolIfSmall");
        if (!args) {
            LOG(WARNING) << "no argument passed";
        } else {
            switch (event) {
                case details::HidlInstrumentor::CLIENT_API_ENTRY:
                case details::HidlInstrumentor::SERVER_API_ENTRY:
                case details::HidlInstrumentor::PASSTHROUGH_ENTRY:
                {
                    if ((*args).size() != 2) {
                        LOG(ERROR) << "Number of arguments does not match. expect: 2, actual: " << (*args).size() << ", method name: convertToBoolIfSmall, event type: " << event;
                        break;
                    }
                    auto *arg_0 __attribute__((__unused__)) = msg.add_arg();
                    ::android::hardware::tests::foo::V1_0::IFoo::Discriminator *arg_val_0 __attribute__((__unused__)) = reinterpret_cast<::android::hardware::tests::foo::V1_0::IFoo::Discriminator*> ((*args)[0]);
                    if (arg_val_0 != nullptr) {
                        arg_0->set_type(TYPE_ENUM);
                        profile____android__hardware__tests__foo__V1_0__IFoo__Discriminator(arg_0, (*arg_val_0));
                    } else {
                        LOG(WARNING) << "argument 0 is null.";
                    }
                    auto *arg_1 __attribute__((__unused__)) = msg.add_arg();
                    ::android::hardware::hidl_vec<::android::hardware::tests::foo::V1_0::IFoo::Union> *arg_val_1 __attribute__((__unused__)) = reinterpret_cast<::android::hardware::hidl_vec<::android::hardware::tests::foo::V1_0::IFoo::Union>*> ((*args)[1]);
                    if (arg_val_1 != nullptr) {
                        arg_1->set_type(TYPE_VECTOR);
                        arg_1->set_vector_size((*arg_val_1).size());
                        for (int arg_1_index = 0; arg_1_index < (int)(*arg_val_1).size(); arg_1_index++) {
                            auto *arg_1_vector_arg_1_index __attribute__((__unused__)) = arg_1->add_vector_value();
                            arg_1_vector_arg_1_index->set_type(TYPE_UNION);
                            profile____android__hardware__tests__foo__V1_0__IFoo__Union(arg_1_vector_arg_1_index, (*arg_val_1)[arg_1_index]);
                        }
                    } else {
                        LOG(WARNING) << "argument 1 is null.";
                    }
                    break;
                }
                case details::HidlInstrumentor::CLIENT_API_EXIT:
                case details::HidlInstrumentor::SERVER_API_EXIT:
                case details::HidlInstrumentor::PASSTHROUGH_EXIT:
                {
                    if ((*args).size() != 1) {
                        LOG(ERROR) << "Number of return values does not match. expect: 1, actual: " << (*args).size() << ", method name: convertToBoolIfSmall, event type: " << event;
                        break;
                    }
                    auto *result_0 __attribute__((__unused__)) = msg.add_return_type_hidl();
                    ::android::hardware::hidl_vec<::android::hardware::tests::foo::V1_0::IFoo::ContainsUnion> *result_val_0 __attribute__((__unused__)) = reinterpret_cast<::android::hardware::hidl_vec<::android::hardware::tests::foo::V1_0::IFoo::ContainsUnion>*> ((*args)[0]);
                    if (result_val_0 != nullptr) {
                        result_0->set_type(TYPE_VECTOR);
                        result_0->set_vector_size((*result_val_0).size());
                        for (int result_0_index = 0; result_0_index < (int)(*result_val_0).size(); result_0_index++) {
                            auto *result_0_vector_result_0_index __attribute__((__unused__)) = result_0->add_vector_value();
                            result_0_vector_result_0_index->set_type(TYPE_STRUCT);
                            profile____android__hardware__tests__foo__V1_0__IFoo__ContainsUnion(result_0_vector_result_0_index, (*result_val_0)[result_0_index]);
                        }
                    } else {
                        LOG(WARNING) << "return value 0 is null.";
                    }
                    break;
                }
                default:
                {
                    LOG(WARNING) << "not supported. ";
                    break;
                }
            }
        }
        profiler.AddTraceEvent(event, package, version, interface, msg);
    }
    if (strcmp(method, "doThis") == 0) {
        FunctionSpecificationMessage msg;
        msg.set_name("doThis");
        if (!args) {
            LOG(WARNING) << "no argument passed";
        } else {
            switch (event) {
                case details::HidlInstrumentor::CLIENT_API_ENTRY:
                case details::HidlInstrumentor::SERVER_API_ENTRY:
                case details::HidlInstrumentor::PASSTHROUGH_ENTRY:
                {
                    if ((*args).size() != 1) {
                        LOG(ERROR) << "Number of arguments does not match. expect: 1, actual: " << (*args).size() << ", method name: doThis, event type: " << event;
                        break;
                    }
                    auto *arg_0 __attribute__((__unused__)) = msg.add_arg();
                    float *arg_val_0 __attribute__((__unused__)) = reinterpret_cast<float*> ((*args)[0]);
                    if (arg_val_0 != nullptr) {
                        arg_0->set_type(TYPE_SCALAR);
                        arg_0->mutable_scalar_value()->set_float_t((*arg_val_0));
                    } else {
                        LOG(WARNING) << "argument 0 is null.";
                    }
                    break;
                }
                case details::HidlInstrumentor::CLIENT_API_EXIT:
                case details::HidlInstrumentor::SERVER_API_EXIT:
                case details::HidlInstrumentor::PASSTHROUGH_EXIT:
                {
                    if ((*args).size() != 0) {
                        LOG(ERROR) << "Number of return values does not match. expect: 0, actual: " << (*args).size() << ", method name: doThis, event type: " << event;
                        break;
                    }
                    break;
                }
                default:
                {
                    LOG(WARNING) << "not supported. ";
                    break;
                }
            }
        }
        profiler.AddTraceEvent(event, package, version, interface, msg);
    }
    if (strcmp(method, "doThatAndReturnSomething") == 0) {
        FunctionSpecificationMessage msg;
        msg.set_name("doThatAndReturnSomething");
        if (!args) {
            LOG(WARNING) << "no argument passed";
        } else {
            switch (event) {
                case details::HidlInstrumentor::CLIENT_API_ENTRY:
                case details::HidlInstrumentor::SERVER_API_ENTRY:
                case details::HidlInstrumentor::PASSTHROUGH_ENTRY:
                {
                    if ((*args).size() != 1) {
                        LOG(ERROR) << "Number of arguments does not match. expect: 1, actual: " << (*args).size() << ", method name: doThatAndReturnSomething, event type: " << event;
                        break;
                    }
                    auto *arg_0 __attribute__((__unused__)) = msg.add_arg();
                    int64_t *arg_val_0 __attribute__((__unused__)) = reinterpret_cast<int64_t*> ((*args)[0]);
                    if (arg_val_0 != nullptr) {
                        arg_0->set_type(TYPE_SCALAR);
                        arg_0->mutable_scalar_value()->set_int64_t((*arg_val_0));
                    } else {
                        LOG(WARNING) << "argument 0 is null.";
                    }
                    break;
                }
                case details::HidlInstrumentor::CLIENT_API_EXIT:
                case details::HidlInstrumentor::SERVER_API_EXIT:
                case details::HidlInstrumentor::PASSTHROUGH_EXIT:
                {
                    if ((*args).size() != 1) {
                        LOG(ERROR) << "Number of return values does not match. expect: 1, actual: " << (*args).size() << ", method name: doThatAndReturnSomething, event type: " << event;
                        break;
                    }
                    auto *result_0 __attribute__((__unused__)) = msg.add_return_type_hidl();
                    int32_t *result_val_0 __attribute__((__unused__)) = reinterpret_cast<int32_t*> ((*args)[0]);
                    if (result_val_0 != nullptr) {
                        result_0->set_type(TYPE_SCALAR);
                        result_0->mutable_scalar_value()->set_int32_t((*result_val_0));
                    } else {
                        LOG(WARNING) << "return value 0 is null.";
                    }
                    break;
                }
                default:
                {
                    LOG(WARNING) << "not supported. ";
                    break;
                }
            }
        }
        profiler.AddTraceEvent(event, package, version, interface, msg);
    }
    if (strcmp(method, "doQuiteABit") == 0) {
        FunctionSpecificationMessage msg;
        msg.set_name("doQuiteABit");
        if (!args) {
            LOG(WARNING) << "no argument passed";
        } else {
            switch (event) {
                case details::HidlInstrumentor::CLIENT_API_ENTRY:
                case details::HidlInstrumentor::SERVER_API_ENTRY:
                case details::HidlInstrumentor::PASSTHROUGH_ENTRY:
                {
                    if ((*args).size() != 4) {
                        LOG(ERROR) << "Number of arguments does not match. expect: 4, actual: " << (*args).size() << ", method name: doQuiteABit, event type: " << event;
                        break;
                    }
                    auto *arg_0 __attribute__((__unused__)) = msg.add_arg();
                    int32_t *arg_val_0 __attribute__((__unused__)) = reinterpret_cast<int32_t*> ((*args)[0]);
                    if (arg_val_0 != nullptr) {
                        arg_0->set_type(TYPE_SCALAR);
                        arg_0->mutable_scalar_value()->set_int32_t((*arg_val_0));
                    } else {
                        LOG(WARNING) << "argument 0 is null.";
                    }
                    auto *arg_1 __attribute__((__unused__)) = msg.add_arg();
                    int64_t *arg_val_1 __attribute__((__unused__)) = reinterpret_cast<int64_t*> ((*args)[1]);
                    if (arg_val_1 != nullptr) {
                        arg_1->set_type(TYPE_SCALAR);
                        arg_1->mutable_scalar_value()->set_int64_t((*arg_val_1));
                    } else {
                        LOG(WARNING) << "argument 1 is null.";
                    }
                    auto *arg_2 __attribute__((__unused__)) = msg.add_arg();
                    float *arg_val_2 __attribute__((__unused__)) = reinterpret_cast<float*> ((*args)[2]);
                    if (arg_val_2 != nullptr) {
                        arg_2->set_type(TYPE_SCALAR);
                        arg_2->mutable_scalar_value()->set_float_t((*arg_val_2));
                    } else {
                        LOG(WARNING) << "argument 2 is null.";
                    }
                    auto *arg_3 __attribute__((__unused__)) = msg.add_arg();
                    double *arg_val_3 __attribute__((__unused__)) = reinterpret_cast<double*> ((*args)[3]);
                    if (arg_val_3 != nullptr) {
                        arg_3->set_type(TYPE_SCALAR);
                        arg_3->mutable_scalar_value()->set_double_t((*arg_val_3));
                    } else {
                        LOG(WARNING) << "argument 3 is null.";
                    }
                    break;
                }
                case details::HidlInstrumentor::CLIENT_API_EXIT:
                case details::HidlInstrumentor::SERVER_API_EXIT:
                case details::HidlInstrumentor::PASSTHROUGH_EXIT:
                {
                    if ((*args).size() != 1) {
                        LOG(ERROR) << "Number of return values does not match. expect: 1, actual: " << (*args).size() << ", method name: doQuiteABit, event type: " << event;
                        break;
                    }
                    auto *result_0 __attribute__((__unused__)) = msg.add_return_type_hidl();
                    double *result_val_0 __attribute__((__unused__)) = reinterpret_cast<double*> ((*args)[0]);
                    if (result_val_0 != nullptr) {
                        result_0->set_type(TYPE_SCALAR);
                        result_0->mutable_scalar_value()->set_double_t((*result_val_0));
                    } else {
                        LOG(WARNING) << "return value 0 is null.";
                    }
                    break;
                }
                default:
                {
                    LOG(WARNING) << "not supported. ";
                    break;
                }
            }
        }
        profiler.AddTraceEvent(event, package, version, interface, msg);
    }
    if (strcmp(method, "doSomethingElse") == 0) {
        FunctionSpecificationMessage msg;
        msg.set_name("doSomethingElse");
        if (!args) {
            LOG(WARNING) << "no argument passed";
        } else {
            switch (event) {
                case details::HidlInstrumentor::CLIENT_API_ENTRY:
                case details::HidlInstrumentor::SERVER_API_ENTRY:
                case details::HidlInstrumentor::PASSTHROUGH_ENTRY:
                {
                    if ((*args).size() != 1) {
                        LOG(ERROR) << "Number of arguments does not match. expect: 1, actual: " << (*args).size() << ", method name: doSomethingElse, event type: " << event;
                        break;
                    }
                    auto *arg_0 __attribute__((__unused__)) = msg.add_arg();
                    ::android::hardware::hidl_array<int32_t, 15> *arg_val_0 __attribute__((__unused__)) = reinterpret_cast<::android::hardware::hidl_array<int32_t, 15>*> ((*args)[0]);
                    if (arg_val_0 != nullptr) {
                        arg_0->set_type(TYPE_ARRAY);
                        arg_0->set_vector_size(15);
                        for (int arg_0_index = 0; arg_0_index < 15; arg_0_index++) {
                            auto *arg_0_array_arg_0_index __attribute__((__unused__)) = arg_0->add_vector_value();
                            arg_0_array_arg_0_index->set_type(TYPE_SCALAR);
                            arg_0_array_arg_0_index->mutable_scalar_value()->set_int32_t((*arg_val_0)[arg_0_index]);
                        }
                    } else {
                        LOG(WARNING) << "argument 0 is null.";
                    }
                    break;
                }
                case details::HidlInstrumentor::CLIENT_API_EXIT:
                case details::HidlInstrumentor::SERVER_API_EXIT:
                case details::HidlInstrumentor::PASSTHROUGH_EXIT:
                {
                    if ((*args).size() != 1) {
                        LOG(ERROR) << "Number of return values does not match. expect: 1, actual: " << (*args).size() << ", method name: doSomethingElse, event type: " << event;
                        break;
                    }
                    auto *result_0 __attribute__((__unused__)) = msg.add_return_type_hidl();
                    ::android::hardware::hidl_array<int32_t, 32> *result_val_0 __attribute__((__unused__)) = reinterpret_cast<::android::hardware::hidl_array<int32_t, 32>*> ((*args)[0]);
                    if (result_val_0 != nullptr) {
                        result_0->set_type(TYPE_ARRAY);
                        result_0->set_vector_size(32);
                        for (int result_0_index = 0; result_0_index < 32; result_0_index++) {
                            auto *result_0_array_result_0_index __attribute__((__unused__)) = result_0->add_vector_value();
                            result_0_array_result_0_index->set_type(TYPE_SCALAR);
                            result_0_array_result_0_index->mutable_scalar_value()->set_int32_t((*result_val_0)[result_0_index]);
                        }
                    } else {
                        LOG(WARNING) << "return value 0 is null.";
                    }
                    break;
                }
                default:
                {
                    LOG(WARNING) << "not supported. ";
                    break;
                }
            }
        }
        profiler.AddTraceEvent(event, package, version, interface, msg);
    }
    if (strcmp(method, "doStuffAndReturnAString") == 0) {
        FunctionSpecificationMessage msg;
        msg.set_name("doStuffAndReturnAString");
        if (!args) {
            LOG(WARNING) << "no argument passed";
        } else {
            switch (event) {
                case details::HidlInstrumentor::CLIENT_API_ENTRY:
                case details::HidlInstrumentor::SERVER_API_ENTRY:
                case details::HidlInstrumentor::PASSTHROUGH_ENTRY:
                {
                    if ((*args).size() != 0) {
                        LOG(ERROR) << "Number of arguments does not match. expect: 0, actual: " << (*args).size() << ", method name: doStuffAndReturnAString, event type: " << event;
                        break;
                    }
                    break;
                }
                case details::HidlInstrumentor::CLIENT_API_EXIT:
                case details::HidlInstrumentor::SERVER_API_EXIT:
                case details::HidlInstrumentor::PASSTHROUGH_EXIT:
                {
                    if ((*args).size() != 1) {
                        LOG(ERROR) << "Number of return values does not match. expect: 1, actual: " << (*args).size() << ", method name: doStuffAndReturnAString, event type: " << event;
                        break;
                    }
                    auto *result_0 __attribute__((__unused__)) = msg.add_return_type_hidl();
                    ::android::hardware::hidl_string *result_val_0 __attribute__((__unused__)) = reinterpret_cast<::android::hardware::hidl_string*> ((*args)[0]);
                    if (result_val_0 != nullptr) {
                        result_0->set_type(TYPE_STRING);
                        result_0->mutable_string_value()->set_message((*result_val_0).c_str());
                        result_0->mutable_string_value()->set_length((*result_val_0).size());
                    } else {
                        LOG(WARNING) << "return value 0 is null.";
                    }
                    break;
                }
                default:
                {
                    LOG(WARNING) << "not supported. ";
                    break;
                }
            }
        }
        profiler.AddTraceEvent(event, package, version, interface, msg);
    }
    if (strcmp(method, "mapThisVector") == 0) {
        FunctionSpecificationMessage msg;
        msg.set_name("mapThisVector");
        if (!args) {
            LOG(WARNING) << "no argument passed";
        } else {
            switch (event) {
                case details::HidlInstrumentor::CLIENT_API_ENTRY:
                case details::HidlInstrumentor::SERVER_API_ENTRY:
                case details::HidlInstrumentor::PASSTHROUGH_ENTRY:
                {
                    if ((*args).size() != 1) {
                        LOG(ERROR) << "Number of arguments does not match. expect: 1, actual: " << (*args).size() << ", method name: mapThisVector, event type: " << event;
                        break;
                    }
                    auto *arg_0 __attribute__((__unused__)) = msg.add_arg();
                    ::android::hardware::hidl_vec<int32_t> *arg_val_0 __attribute__((__unused__)) = reinterpret_cast<::android::hardware::hidl_vec<int32_t>*> ((*args)[0]);
                    if (arg_val_0 != nullptr) {
                        arg_0->set_type(TYPE_VECTOR);
                        arg_0->set_vector_size((*arg_val_0).size());
                        for (int arg_0_index = 0; arg_0_index < (int)(*arg_val_0).size(); arg_0_index++) {
                            auto *arg_0_vector_arg_0_index __attribute__((__unused__)) = arg_0->add_vector_value();
                            arg_0_vector_arg_0_index->set_type(TYPE_SCALAR);
                            arg_0_vector_arg_0_index->mutable_scalar_value()->set_int32_t((*arg_val_0)[arg_0_index]);
                        }
                    } else {
                        LOG(WARNING) << "argument 0 is null.";
                    }
                    break;
                }
                case details::HidlInstrumentor::CLIENT_API_EXIT:
                case details::HidlInstrumentor::SERVER_API_EXIT:
                case details::HidlInstrumentor::PASSTHROUGH_EXIT:
                {
                    if ((*args).size() != 1) {
                        LOG(ERROR) << "Number of return values does not match. expect: 1, actual: " << (*args).size() << ", method name: mapThisVector, event type: " << event;
                        break;
                    }
                    auto *result_0 __attribute__((__unused__)) = msg.add_return_type_hidl();
                    ::android::hardware::hidl_vec<int32_t> *result_val_0 __attribute__((__unused__)) = reinterpret_cast<::android::hardware::hidl_vec<int32_t>*> ((*args)[0]);
                    if (result_val_0 != nullptr) {
                        result_0->set_type(TYPE_VECTOR);
                        result_0->set_vector_size((*result_val_0).size());
                        for (int result_0_index = 0; result_0_index < (int)(*result_val_0).size(); result_0_index++) {
                            auto *result_0_vector_result_0_index __attribute__((__unused__)) = result_0->add_vector_value();
                            result_0_vector_result_0_index->set_type(TYPE_SCALAR);
                            result_0_vector_result_0_index->mutable_scalar_value()->set_int32_t((*result_val_0)[result_0_index]);
                        }
                    } else {
                        LOG(WARNING) << "return value 0 is null.";
                    }
                    break;
                }
                default:
                {
                    LOG(WARNING) << "not supported. ";
                    break;
                }
            }
        }
        profiler.AddTraceEvent(event, package, version, interface, msg);
    }
    if (strcmp(method, "callMe") == 0) {
        FunctionSpecificationMessage msg;
        msg.set_name("callMe");
        if (!args) {
            LOG(WARNING) << "no argument passed";
        } else {
            switch (event) {
                case details::HidlInstrumentor::CLIENT_API_ENTRY:
                case details::HidlInstrumentor::SERVER_API_ENTRY:
                case details::HidlInstrumentor::PASSTHROUGH_ENTRY:
                {
                    if ((*args).size() != 1) {
                        LOG(ERROR) << "Number of arguments does not match. expect: 1, actual: " << (*args).size() << ", method name: callMe, event type: " << event;
                        break;
                    }
                    auto *arg_0 __attribute__((__unused__)) = msg.add_arg();
                    sp<::android::hardware::tests::foo::V1_0::IFooCallback> *arg_val_0 __attribute__((__unused__)) = reinterpret_cast<sp<::android::hardware::tests::foo::V1_0::IFooCallback>*> ((*args)[0]);
                    if (arg_val_0 != nullptr) {
                        arg_0->set_type(TYPE_HIDL_CALLBACK);
                        arg_0->set_predefined_type("::android::hardware::tests::foo::V1_0::IFooCallback");
                    } else {
                        LOG(WARNING) << "argument 0 is null.";
                    }
                    break;
                }
                case details::HidlInstrumentor::CLIENT_API_EXIT:
                case details::HidlInstrumentor::SERVER_API_EXIT:
                case details::HidlInstrumentor::PASSTHROUGH_EXIT:
                {
                    if ((*args).size() != 0) {
                        LOG(ERROR) << "Number of return values does not match. expect: 0, actual: " << (*args).size() << ", method name: callMe, event type: " << event;
                        break;
                    }
                    break;
                }
                default:
                {
                    LOG(WARNING) << "not supported. ";
                    break;
                }
            }
        }
        profiler.AddTraceEvent(event, package, version, interface, msg);
    }
    if (strcmp(method, "useAnEnum") == 0) {
        FunctionSpecificationMessage msg;
        msg.set_name("useAnEnum");
        if (!args) {
            LOG(WARNING) << "no argument passed";
        } else {
            switch (event) {
                case details::HidlInstrumentor::CLIENT_API_ENTRY:
                case details::HidlInstrumentor::SERVER_API_ENTRY:
                case details::HidlInstrumentor::PASSTHROUGH_ENTRY:
                {
                    if ((*args).size() != 1) {
                        LOG(ERROR) << "Number of arguments does not match. expect: 1, actual: " << (*args).size() << ", method name: useAnEnum, event type: " << event;
                        break;
                    }
                    auto *arg_0 __attribute__((__unused__)) = msg.add_arg();
                    ::android::hardware::tests::foo::V1_0::IFoo::SomeEnum *arg_val_0 __attribute__((__unused__)) = reinterpret_cast<::android::hardware::tests::foo::V1_0::IFoo::SomeEnum*> ((*args)[0]);
                    if (arg_val_0 != nullptr) {
                        arg_0->set_type(TYPE_ENUM);
                        profile____android__hardware__tests__foo__V1_0__IFoo__SomeEnum(arg_0, (*arg_val_0));
                    } else {
                        LOG(WARNING) << "argument 0 is null.";
                    }
                    break;
                }
                case details::HidlInstrumentor::CLIENT_API_EXIT:
                case details::HidlInstrumentor::SERVER_API_EXIT:
                case details::HidlInstrumentor::PASSTHROUGH_EXIT:
                {
                    if ((*args).size() != 1) {
                        LOG(ERROR) << "Number of return values does not match. expect: 1, actual: " << (*args).size() << ", method name: useAnEnum, event type: " << event;
                        break;
                    }
                    auto *result_0 __attribute__((__unused__)) = msg.add_return_type_hidl();
                    ::android::hardware::tests::foo::V1_0::IFoo::SomeEnum *result_val_0 __attribute__((__unused__)) = reinterpret_cast<::android::hardware::tests::foo::V1_0::IFoo::SomeEnum*> ((*args)[0]);
                    if (result_val_0 != nullptr) {
                        result_0->set_type(TYPE_ENUM);
                        profile____android__hardware__tests__foo__V1_0__IFoo__SomeEnum(result_0, (*result_val_0));
                    } else {
                        LOG(WARNING) << "return value 0 is null.";
                    }
                    break;
                }
                default:
                {
                    LOG(WARNING) << "not supported. ";
                    break;
                }
            }
        }
        profiler.AddTraceEvent(event, package, version, interface, msg);
    }
    if (strcmp(method, "haveAGooberVec") == 0) {
        FunctionSpecificationMessage msg;
        msg.set_name("haveAGooberVec");
        if (!args) {
            LOG(WARNING) << "no argument passed";
        } else {
            switch (event) {
                case details::HidlInstrumentor::CLIENT_API_ENTRY:
                case details::HidlInstrumentor::SERVER_API_ENTRY:
                case details::HidlInstrumentor::PASSTHROUGH_ENTRY:
                {
                    if ((*args).size() != 1) {
                        LOG(ERROR) << "Number of arguments does not match. expect: 1, actual: " << (*args).size() << ", method name: haveAGooberVec, event type: " << event;
                        break;
                    }
                    auto *arg_0 __attribute__((__unused__)) = msg.add_arg();
                    ::android::hardware::hidl_vec<::android::hardware::tests::foo::V1_0::IFoo::Goober> *arg_val_0 __attribute__((__unused__)) = reinterpret_cast<::android::hardware::hidl_vec<::android::hardware::tests::foo::V1_0::IFoo::Goober>*> ((*args)[0]);
                    if (arg_val_0 != nullptr) {
                        arg_0->set_type(TYPE_VECTOR);
                        arg_0->set_vector_size((*arg_val_0).size());
                        for (int arg_0_index = 0; arg_0_index < (int)(*arg_val_0).size(); arg_0_index++) {
                            auto *arg_0_vector_arg_0_index __attribute__((__unused__)) = arg_0->add_vector_value();
                            arg_0_vector_arg_0_index->set_type(TYPE_STRUCT);
                            profile____android__hardware__tests__foo__V1_0__IFoo__Goober(arg_0_vector_arg_0_index, (*arg_val_0)[arg_0_index]);
                        }
                    } else {
                        LOG(WARNING) << "argument 0 is null.";
                    }
                    break;
                }
                case details::HidlInstrumentor::CLIENT_API_EXIT:
                case details::HidlInstrumentor::SERVER_API_EXIT:
                case details::HidlInstrumentor::PASSTHROUGH_EXIT:
                {
                    if ((*args).size() != 0) {
                        LOG(ERROR) << "Number of return values does not match. expect: 0, actual: " << (*args).size() << ", method name: haveAGooberVec, event type: " << event;
                        break;
                    }
                    break;
                }
                default:
                {
                    LOG(WARNING) << "not supported. ";
                    break;
                }
            }
        }
        profiler.AddTraceEvent(event, package, version, interface, msg);
    }
    if (strcmp(method, "haveAGoober") == 0) {
        FunctionSpecificationMessage msg;
        msg.set_name("haveAGoober");
        if (!args) {
            LOG(WARNING) << "no argument passed";
        } else {
            switch (event) {
                case details::HidlInstrumentor::CLIENT_API_ENTRY:
                case details::HidlInstrumentor::SERVER_API_ENTRY:
                case details::HidlInstrumentor::PASSTHROUGH_ENTRY:
                {
                    if ((*args).size() != 1) {
                        LOG(ERROR) << "Number of arguments does not match. expect: 1, actual: " << (*args).size() << ", method name: haveAGoober, event type: " << event;
                        break;
                    }
                    auto *arg_0 __attribute__((__unused__)) = msg.add_arg();
                    ::android::hardware::tests::foo::V1_0::IFoo::Goober *arg_val_0 __attribute__((__unused__)) = reinterpret_cast<::android::hardware::tests::foo::V1_0::IFoo::Goober*> ((*args)[0]);
                    if (arg_val_0 != nullptr) {
                        arg_0->set_type(TYPE_STRUCT);
                        profile____android__hardware__tests__foo__V1_0__IFoo__Goober(arg_0, (*arg_val_0));
                    } else {
                        LOG(WARNING) << "argument 0 is null.";
                    }
                    break;
                }
                case details::HidlInstrumentor::CLIENT_API_EXIT:
                case details::HidlInstrumentor::SERVER_API_EXIT:
                case details::HidlInstrumentor::PASSTHROUGH_EXIT:
                {
                    if ((*args).size() != 0) {
                        LOG(ERROR) << "Number of return values does not match. expect: 0, actual: " << (*args).size() << ", method name: haveAGoober, event type: " << event;
                        break;
                    }
                    break;
                }
                default:
                {
                    LOG(WARNING) << "not supported. ";
                    break;
                }
            }
        }
        profiler.AddTraceEvent(event, package, version, interface, msg);
    }
    if (strcmp(method, "haveAGooberArray") == 0) {
        FunctionSpecificationMessage msg;
        msg.set_name("haveAGooberArray");
        if (!args) {
            LOG(WARNING) << "no argument passed";
        } else {
            switch (event) {
                case details::HidlInstrumentor::CLIENT_API_ENTRY:
                case details::HidlInstrumentor::SERVER_API_ENTRY:
                case details::HidlInstrumentor::PASSTHROUGH_ENTRY:
                {
                    if ((*args).size() != 1) {
                        LOG(ERROR) << "Number of arguments does not match. expect: 1, actual: " << (*args).size() << ", method name: haveAGooberArray, event type: " << event;
                        break;
                    }
                    auto *arg_0 __attribute__((__unused__)) = msg.add_arg();
                    ::android::hardware::hidl_array<::android::hardware::tests::foo::V1_0::IFoo::Goober, 20> *arg_val_0 __attribute__((__unused__)) = reinterpret_cast<::android::hardware::hidl_array<::android::hardware::tests::foo::V1_0::IFoo::Goober, 20>*> ((*args)[0]);
                    if (arg_val_0 != nullptr) {
                        arg_0->set_type(TYPE_ARRAY);
                        arg_0->set_vector_size(20);
                        for (int arg_0_index = 0; arg_0_index < 20; arg_0_index++) {
                            auto *arg_0_array_arg_0_index __attribute__((__unused__)) = arg_0->add_vector_value();
                            arg_0_array_arg_0_index->set_type(TYPE_STRUCT);
                            auto *arg_0_array_arg_0_index_q __attribute__((__unused__)) = arg_0_array_arg_0_index->add_struct_value();
                            arg_0_array_arg_0_index_q->set_type(TYPE_SCALAR);
                            arg_0_array_arg_0_index_q->mutable_scalar_value()->set_int32_t((*arg_val_0)[arg_0_index].q);
                            auto *arg_0_array_arg_0_index_name __attribute__((__unused__)) = arg_0_array_arg_0_index->add_struct_value();
                            arg_0_array_arg_0_index_name->set_type(TYPE_STRING);
                            arg_0_array_arg_0_index_name->mutable_string_value()->set_message((*arg_val_0)[arg_0_index].name.c_str());
                            arg_0_array_arg_0_index_name->mutable_string_value()->set_length((*arg_val_0)[arg_0_index].name.size());
                            auto *arg_0_array_arg_0_index_address __attribute__((__unused__)) = arg_0_array_arg_0_index->add_struct_value();
                            arg_0_array_arg_0_index_address->set_type(TYPE_STRING);
                            arg_0_array_arg_0_index_address->mutable_string_value()->set_message((*arg_val_0)[arg_0_index].address.c_str());
                            arg_0_array_arg_0_index_address->mutable_string_value()->set_length((*arg_val_0)[arg_0_index].address.size());
                            auto *arg_0_array_arg_0_index_numbers __attribute__((__unused__)) = arg_0_array_arg_0_index->add_struct_value();
                            arg_0_array_arg_0_index_numbers->set_type(TYPE_ARRAY);
                            arg_0_array_arg_0_index_numbers->set_vector_size(10);
                            for (int arg_0_array_arg_0_index_numbers_index = 0; arg_0_array_arg_0_index_numbers_index < 10; arg_0_array_arg_0_index_numbers_index++) {
                                auto *arg_0_array_arg_0_index_numbers_array_arg_0_array_arg_0_index_numbers_index __attribute__((__unused__)) = arg_0_array_arg_0_index_numbers->add_vector_value();
                                arg_0_array_arg_0_index_numbers_array_arg_0_array_arg_0_index_numbers_index->set_type(TYPE_SCALAR);
                                arg_0_array_arg_0_index_numbers_array_arg_0_array_arg_0_index_numbers_index->mutable_scalar_value()->set_double_t((*arg_val_0)[arg_0_index].numbers[arg_0_array_arg_0_index_numbers_index]);
                            }
                            auto *arg_0_array_arg_0_index_fumble __attribute__((__unused__)) = arg_0_array_arg_0_index->add_struct_value();
                            arg_0_array_arg_0_index_fumble->set_type(TYPE_STRUCT);
                            profile____android__hardware__tests__foo__V1_0__IFoo__Fumble(arg_0_array_arg_0_index_fumble, (*arg_val_0)[arg_0_index].fumble);
                            auto *arg_0_array_arg_0_index_gumble __attribute__((__unused__)) = arg_0_array_arg_0_index->add_struct_value();
                            arg_0_array_arg_0_index_gumble->set_type(TYPE_STRUCT);
                            profile____android__hardware__tests__foo__V1_0__IFoo__Fumble(arg_0_array_arg_0_index_gumble, (*arg_val_0)[arg_0_index].gumble);
                        }
                    } else {
                        LOG(WARNING) << "argument 0 is null.";
                    }
                    break;
                }
                case details::HidlInstrumentor::CLIENT_API_EXIT:
                case details::HidlInstrumentor::SERVER_API_EXIT:
                case details::HidlInstrumentor::PASSTHROUGH_EXIT:
                {
                    if ((*args).size() != 0) {
                        LOG(ERROR) << "Number of return values does not match. expect: 0, actual: " << (*args).size() << ", method name: haveAGooberArray, event type: " << event;
                        break;
                    }
                    break;
                }
                default:
                {
                    LOG(WARNING) << "not supported. ";
                    break;
                }
            }
        }
        profiler.AddTraceEvent(event, package, version, interface, msg);
    }
    if (strcmp(method, "haveATypeFromAnotherFile") == 0) {
        FunctionSpecificationMessage msg;
        msg.set_name("haveATypeFromAnotherFile");
        if (!args) {
            LOG(WARNING) << "no argument passed";
        } else {
            switch (event) {
                case details::HidlInstrumentor::CLIENT_API_ENTRY:
                case details::HidlInstrumentor::SERVER_API_ENTRY:
                case details::HidlInstrumentor::PASSTHROUGH_ENTRY:
                {
                    if ((*args).size() != 1) {
                        LOG(ERROR) << "Number of arguments does not match. expect: 1, actual: " << (*args).size() << ", method name: haveATypeFromAnotherFile, event type: " << event;
                        break;
                    }
                    auto *arg_0 __attribute__((__unused__)) = msg.add_arg();
                    ::android::hardware::tests::foo::V1_0::Abc *arg_val_0 __attribute__((__unused__)) = reinterpret_cast<::android::hardware::tests::foo::V1_0::Abc*> ((*args)[0]);
                    if (arg_val_0 != nullptr) {
                        arg_0->set_type(TYPE_STRUCT);
                        profile____android__hardware__tests__foo__V1_0__Abc(arg_0, (*arg_val_0));
                    } else {
                        LOG(WARNING) << "argument 0 is null.";
                    }
                    break;
                }
                case details::HidlInstrumentor::CLIENT_API_EXIT:
                case details::HidlInstrumentor::SERVER_API_EXIT:
                case details::HidlInstrumentor::PASSTHROUGH_EXIT:
                {
                    if ((*args).size() != 0) {
                        LOG(ERROR) << "Number of return values does not match. expect: 0, actual: " << (*args).size() << ", method name: haveATypeFromAnotherFile, event type: " << event;
                        break;
                    }
                    break;
                }
                default:
                {
                    LOG(WARNING) << "not supported. ";
                    break;
                }
            }
        }
        profiler.AddTraceEvent(event, package, version, interface, msg);
    }
    if (strcmp(method, "haveSomeStrings") == 0) {
        FunctionSpecificationMessage msg;
        msg.set_name("haveSomeStrings");
        if (!args) {
            LOG(WARNING) << "no argument passed";
        } else {
            switch (event) {
                case details::HidlInstrumentor::CLIENT_API_ENTRY:
                case details::HidlInstrumentor::SERVER_API_ENTRY:
                case details::HidlInstrumentor::PASSTHROUGH_ENTRY:
                {
                    if ((*args).size() != 1) {
                        LOG(ERROR) << "Number of arguments does not match. expect: 1, actual: " << (*args).size() << ", method name: haveSomeStrings, event type: " << event;
                        break;
                    }
                    auto *arg_0 __attribute__((__unused__)) = msg.add_arg();
                    ::android::hardware::hidl_array<::android::hardware::hidl_string, 3> *arg_val_0 __attribute__((__unused__)) = reinterpret_cast<::android::hardware::hidl_array<::android::hardware::hidl_string, 3>*> ((*args)[0]);
                    if (arg_val_0 != nullptr) {
                        arg_0->set_type(TYPE_ARRAY);
                        arg_0->set_vector_size(3);
                        for (int arg_0_index = 0; arg_0_index < 3; arg_0_index++) {
                            auto *arg_0_array_arg_0_index __attribute__((__unused__)) = arg_0->add_vector_value();
                            arg_0_array_arg_0_index->set_type(TYPE_STRING);
                            arg_0_array_arg_0_index->mutable_string_value()->set_message((*arg_val_0)[arg_0_index].c_str());
                            arg_0_array_arg_0_index->mutable_string_value()->set_length((*arg_val_0)[arg_0_index].size());
                        }
                    } else {
                        LOG(WARNING) << "argument 0 is null.";
                    }
                    break;
                }
                case details::HidlInstrumentor::CLIENT_API_EXIT:
                case details::HidlInstrumentor::SERVER_API_EXIT:
                case details::HidlInstrumentor::PASSTHROUGH_EXIT:
                {
                    if ((*args).size() != 1) {
                        LOG(ERROR) << "Number of return values does not match. expect: 1, actual: " << (*args).size() << ", method name: haveSomeStrings, event type: " << event;
                        break;
                    }
                    auto *result_0 __attribute__((__unused__)) = msg.add_return_type_hidl();
                    ::android::hardware::hidl_array<::android::hardware::hidl_string, 2> *result_val_0 __attribute__((__unused__)) = reinterpret_cast<::android::hardware::hidl_array<::android::hardware::hidl_string, 2>*> ((*args)[0]);
                    if (result_val_0 != nullptr) {
                        result_0->set_type(TYPE_ARRAY);
                        result_0->set_vector_size(2);
                        for (int result_0_index = 0; result_0_index < 2; result_0_index++) {
                            auto *result_0_array_result_0_index __attribute__((__unused__)) = result_0->add_vector_value();
                            result_0_array_result_0_index->set_type(TYPE_STRING);
                            result_0_array_result_0_index->mutable_string_value()->set_message((*result_val_0)[result_0_index].c_str());
                            result_0_array_result_0_index->mutable_string_value()->set_length((*result_val_0)[result_0_index].size());
                        }
                    } else {
                        LOG(WARNING) << "return value 0 is null.";
                    }
                    break;
                }
                default:
                {
                    LOG(WARNING) << "not supported. ";
                    break;
                }
            }
        }
        profiler.AddTraceEvent(event, package, version, interface, msg);
    }
    if (strcmp(method, "haveAStringVec") == 0) {
        FunctionSpecificationMessage msg;
        msg.set_name("haveAStringVec");
        if (!args) {
            LOG(WARNING) << "no argument passed";
        } else {
            switch (event) {
                case details::HidlInstrumentor::CLIENT_API_ENTRY:
                case details::HidlInstrumentor::SERVER_API_ENTRY:
                case details::HidlInstrumentor::PASSTHROUGH_ENTRY:
                {
                    if ((*args).size() != 1) {
                        LOG(ERROR) << "Number of arguments does not match. expect: 1, actual: " << (*args).size() << ", method name: haveAStringVec, event type: " << event;
                        break;
                    }
                    auto *arg_0 __attribute__((__unused__)) = msg.add_arg();
                    ::android::hardware::hidl_vec<::android::hardware::hidl_string> *arg_val_0 __attribute__((__unused__)) = reinterpret_cast<::android::hardware::hidl_vec<::android::hardware::hidl_string>*> ((*args)[0]);
                    if (arg_val_0 != nullptr) {
                        arg_0->set_type(TYPE_VECTOR);
                        arg_0->set_vector_size((*arg_val_0).size());
                        for (int arg_0_index = 0; arg_0_index < (int)(*arg_val_0).size(); arg_0_index++) {
                            auto *arg_0_vector_arg_0_index __attribute__((__unused__)) = arg_0->add_vector_value();
                            arg_0_vector_arg_0_index->set_type(TYPE_STRING);
                            arg_0_vector_arg_0_index->mutable_string_value()->set_message((*arg_val_0)[arg_0_index].c_str());
                            arg_0_vector_arg_0_index->mutable_string_value()->set_length((*arg_val_0)[arg_0_index].size());
                        }
                    } else {
                        LOG(WARNING) << "argument 0 is null.";
                    }
                    break;
                }
                case details::HidlInstrumentor::CLIENT_API_EXIT:
                case details::HidlInstrumentor::SERVER_API_EXIT:
                case details::HidlInstrumentor::PASSTHROUGH_EXIT:
                {
                    if ((*args).size() != 1) {
                        LOG(ERROR) << "Number of return values does not match. expect: 1, actual: " << (*args).size() << ", method name: haveAStringVec, event type: " << event;
                        break;
                    }
                    auto *result_0 __attribute__((__unused__)) = msg.add_return_type_hidl();
                    ::android::hardware::hidl_vec<::android::hardware::hidl_string> *result_val_0 __attribute__((__unused__)) = reinterpret_cast<::android::hardware::hidl_vec<::android::hardware::hidl_string>*> ((*args)[0]);
                    if (result_val_0 != nullptr) {
                        result_0->set_type(TYPE_VECTOR);
                        result_0->set_vector_size((*result_val_0).size());
                        for (int result_0_index = 0; result_0_index < (int)(*result_val_0).size(); result_0_index++) {
                            auto *result_0_vector_result_0_index __attribute__((__unused__)) = result_0->add_vector_value();
                            result_0_vector_result_0_index->set_type(TYPE_STRING);
                            result_0_vector_result_0_index->mutable_string_value()->set_message((*result_val_0)[result_0_index].c_str());
                            result_0_vector_result_0_index->mutable_string_value()->set_length((*result_val_0)[result_0_index].size());
                        }
                    } else {
                        LOG(WARNING) << "return value 0 is null.";
                    }
                    break;
                }
                default:
                {
                    LOG(WARNING) << "not supported. ";
                    break;
                }
            }
        }
        profiler.AddTraceEvent(event, package, version, interface, msg);
    }
    if (strcmp(method, "transposeMe") == 0) {
        FunctionSpecificationMessage msg;
        msg.set_name("transposeMe");
        if (!args) {
            LOG(WARNING) << "no argument passed";
        } else {
            switch (event) {
                case details::HidlInstrumentor::CLIENT_API_ENTRY:
                case details::HidlInstrumentor::SERVER_API_ENTRY:
                case details::HidlInstrumentor::PASSTHROUGH_ENTRY:
                {
                    if ((*args).size() != 1) {
                        LOG(ERROR) << "Number of arguments does not match. expect: 1, actual: " << (*args).size() << ", method name: transposeMe, event type: " << event;
                        break;
                    }
                    auto *arg_0 __attribute__((__unused__)) = msg.add_arg();
                    ::android::hardware::hidl_array<float, 3, 5> *arg_val_0 __attribute__((__unused__)) = reinterpret_cast<::android::hardware::hidl_array<float, 3, 5>*> ((*args)[0]);
                    if (arg_val_0 != nullptr) {
                        arg_0->set_type(TYPE_ARRAY);
                        arg_0->set_vector_size(3);
                        for (int arg_0_index = 0; arg_0_index < 3; arg_0_index++) {
                            auto *arg_0_array_arg_0_index __attribute__((__unused__)) = arg_0->add_vector_value();
                            arg_0_array_arg_0_index->set_type(TYPE_ARRAY);
                            arg_0_array_arg_0_index->set_vector_size(5);
                            for (int arg_0_array_arg_0_index_index = 0; arg_0_array_arg_0_index_index < 5; arg_0_array_arg_0_index_index++) {
                                auto *arg_0_array_arg_0_index_array_arg_0_array_arg_0_index_index __attribute__((__unused__)) = arg_0_array_arg_0_index->add_vector_value();
                                arg_0_array_arg_0_index_array_arg_0_array_arg_0_index_index->set_type(TYPE_SCALAR);
                                arg_0_array_arg_0_index_array_arg_0_array_arg_0_index_index->mutable_scalar_value()->set_float_t((*arg_val_0)[arg_0_index][arg_0_array_arg_0_index_index]);
                            }
                        }
                    } else {
                        LOG(WARNING) << "argument 0 is null.";
                    }
                    break;
                }
                case details::HidlInstrumentor::CLIENT_API_EXIT:
                case details::HidlInstrumentor::SERVER_API_EXIT:
                case details::HidlInstrumentor::PASSTHROUGH_EXIT:
                {
                    if ((*args).size() != 1) {
                        LOG(ERROR) << "Number of return values does not match. expect: 1, actual: " << (*args).size() << ", method name: transposeMe, event type: " << event;
                        break;
                    }
                    auto *result_0 __attribute__((__unused__)) = msg.add_return_type_hidl();
                    ::android::hardware::hidl_array<float, 5, 3> *result_val_0 __attribute__((__unused__)) = reinterpret_cast<::android::hardware::hidl_array<float, 5, 3>*> ((*args)[0]);
                    if (result_val_0 != nullptr) {
                        result_0->set_type(TYPE_ARRAY);
                        result_0->set_vector_size(5);
                        for (int result_0_index = 0; result_0_index < 5; result_0_index++) {
                            auto *result_0_array_result_0_index __attribute__((__unused__)) = result_0->add_vector_value();
                            result_0_array_result_0_index->set_type(TYPE_ARRAY);
                            result_0_array_result_0_index->set_vector_size(3);
                            for (int result_0_array_result_0_index_index = 0; result_0_array_result_0_index_index < 3; result_0_array_result_0_index_index++) {
                                auto *result_0_array_result_0_index_array_result_0_array_result_0_index_index __attribute__((__unused__)) = result_0_array_result_0_index->add_vector_value();
                                result_0_array_result_0_index_array_result_0_array_result_0_index_index->set_type(TYPE_SCALAR);
                                result_0_array_result_0_index_array_result_0_array_result_0_index_index->mutable_scalar_value()->set_float_t((*result_val_0)[result_0_index][result_0_array_result_0_index_index]);
                            }
                        }
                    } else {
                        LOG(WARNING) << "return value 0 is null.";
                    }
                    break;
                }
                default:
                {
                    LOG(WARNING) << "not supported. ";
                    break;
                }
            }
        }
        profiler.AddTraceEvent(event, package, version, interface, msg);
    }
    if (strcmp(method, "callingDrWho") == 0) {
        FunctionSpecificationMessage msg;
        msg.set_name("callingDrWho");
        if (!args) {
            LOG(WARNING) << "no argument passed";
        } else {
            switch (event) {
                case details::HidlInstrumentor::CLIENT_API_ENTRY:
                case details::HidlInstrumentor::SERVER_API_ENTRY:
                case details::HidlInstrumentor::PASSTHROUGH_ENTRY:
                {
                    if ((*args).size() != 1) {
                        LOG(ERROR) << "Number of arguments does not match. expect: 1, actual: " << (*args).size() << ", method name: callingDrWho, event type: " << event;
                        break;
                    }
                    auto *arg_0 __attribute__((__unused__)) = msg.add_arg();
                    ::android::hardware::tests::foo::V1_0::IFoo::MultiDimensional *arg_val_0 __attribute__((__unused__)) = reinterpret_cast<::android::hardware::tests::foo::V1_0::IFoo::MultiDimensional*> ((*args)[0]);
                    if (arg_val_0 != nullptr) {
                        arg_0->set_type(TYPE_STRUCT);
                        profile____android__hardware__tests__foo__V1_0__IFoo__MultiDimensional(arg_0, (*arg_val_0));
                    } else {
                        LOG(WARNING) << "argument 0 is null.";
                    }
                    break;
                }
                case details::HidlInstrumentor::CLIENT_API_EXIT:
                case details::HidlInstrumentor::SERVER_API_EXIT:
                case details::HidlInstrumentor::PASSTHROUGH_EXIT:
                {
                    if ((*args).size() != 1) {
                        LOG(ERROR) << "Number of return values does not match. expect: 1, actual: " << (*args).size() << ", method name: callingDrWho, event type: " << event;
                        break;
                    }
                    auto *result_0 __attribute__((__unused__)) = msg.add_return_type_hidl();
                    ::android::hardware::tests::foo::V1_0::IFoo::MultiDimensional *result_val_0 __attribute__((__unused__)) = reinterpret_cast<::android::hardware::tests::foo::V1_0::IFoo::MultiDimensional*> ((*args)[0]);
                    if (result_val_0 != nullptr) {
                        result_0->set_type(TYPE_STRUCT);
                        profile____android__hardware__tests__foo__V1_0__IFoo__MultiDimensional(result_0, (*result_val_0));
                    } else {
                        LOG(WARNING) << "return value 0 is null.";
                    }
                    break;
                }
                default:
                {
                    LOG(WARNING) << "not supported. ";
                    break;
                }
            }
        }
        profiler.AddTraceEvent(event, package, version, interface, msg);
    }
    if (strcmp(method, "transpose") == 0) {
        FunctionSpecificationMessage msg;
        msg.set_name("transpose");
        if (!args) {
            LOG(WARNING) << "no argument passed";
        } else {
            switch (event) {
                case details::HidlInstrumentor::CLIENT_API_ENTRY:
                case details::HidlInstrumentor::SERVER_API_ENTRY:
                case details::HidlInstrumentor::PASSTHROUGH_ENTRY:
                {
                    if ((*args).size() != 1) {
                        LOG(ERROR) << "Number of arguments does not match. expect: 1, actual: " << (*args).size() << ", method name: transpose, event type: " << event;
                        break;
                    }
                    auto *arg_0 __attribute__((__unused__)) = msg.add_arg();
                    ::android::hardware::tests::foo::V1_0::IFoo::StringMatrix5x3 *arg_val_0 __attribute__((__unused__)) = reinterpret_cast<::android::hardware::tests::foo::V1_0::IFoo::StringMatrix5x3*> ((*args)[0]);
                    if (arg_val_0 != nullptr) {
                        arg_0->set_type(TYPE_STRUCT);
                        profile____android__hardware__tests__foo__V1_0__IFoo__StringMatrix5x3(arg_0, (*arg_val_0));
                    } else {
                        LOG(WARNING) << "argument 0 is null.";
                    }
                    break;
                }
                case details::HidlInstrumentor::CLIENT_API_EXIT:
                case details::HidlInstrumentor::SERVER_API_EXIT:
                case details::HidlInstrumentor::PASSTHROUGH_EXIT:
                {
                    if ((*args).size() != 1) {
                        LOG(ERROR) << "Number of return values does not match. expect: 1, actual: " << (*args).size() << ", method name: transpose, event type: " << event;
                        break;
                    }
                    auto *result_0 __attribute__((__unused__)) = msg.add_return_type_hidl();
                    ::android::hardware::tests::foo::V1_0::IFoo::StringMatrix3x5 *result_val_0 __attribute__((__unused__)) = reinterpret_cast<::android::hardware::tests::foo::V1_0::IFoo::StringMatrix3x5*> ((*args)[0]);
                    if (result_val_0 != nullptr) {
                        result_0->set_type(TYPE_STRUCT);
                        profile____android__hardware__tests__foo__V1_0__IFoo__StringMatrix3x5(result_0, (*result_val_0));
                    } else {
                        LOG(WARNING) << "return value 0 is null.";
                    }
                    break;
                }
                default:
                {
                    LOG(WARNING) << "not supported. ";
                    break;
                }
            }
        }
        profiler.AddTraceEvent(event, package, version, interface, msg);
    }
    if (strcmp(method, "transpose2") == 0) {
        FunctionSpecificationMessage msg;
        msg.set_name("transpose2");
        if (!args) {
            LOG(WARNING) << "no argument passed";
        } else {
            switch (event) {
                case details::HidlInstrumentor::CLIENT_API_ENTRY:
                case details::HidlInstrumentor::SERVER_API_ENTRY:
                case details::HidlInstrumentor::PASSTHROUGH_ENTRY:
                {
                    if ((*args).size() != 1) {
                        LOG(ERROR) << "Number of arguments does not match. expect: 1, actual: " << (*args).size() << ", method name: transpose2, event type: " << event;
                        break;
                    }
                    auto *arg_0 __attribute__((__unused__)) = msg.add_arg();
                    ::android::hardware::hidl_array<::android::hardware::hidl_string, 5, 3> *arg_val_0 __attribute__((__unused__)) = reinterpret_cast<::android::hardware::hidl_array<::android::hardware::hidl_string, 5, 3>*> ((*args)[0]);
                    if (arg_val_0 != nullptr) {
                        arg_0->set_type(TYPE_ARRAY);
                        arg_0->set_vector_size(5);
                        for (int arg_0_index = 0; arg_0_index < 5; arg_0_index++) {
                            auto *arg_0_array_arg_0_index __attribute__((__unused__)) = arg_0->add_vector_value();
                            arg_0_array_arg_0_index->set_type(TYPE_ARRAY);
                            arg_0_array_arg_0_index->set_vector_size(3);
                            for (int arg_0_array_arg_0_index_index = 0; arg_0_array_arg_0_index_index < 3; arg_0_array_arg_0_index_index++) {
                                auto *arg_0_array_arg_0_index_array_arg_0_array_arg_0_index_index __attribute__((__unused__)) = arg_0_array_arg_0_index->add_vector_value();
                                arg_0_array_arg_0_index_array_arg_0_array_arg_0_index_index->set_type(TYPE_STRING);
                                arg_0_array_arg_0_index_array_arg_0_array_arg_0_index_index->mutable_string_value()->set_message((*arg_val_0)[arg_0_index][arg_0_array_arg_0_index_index].c_str());
                                arg_0_array_arg_0_index_array_arg_0_array_arg_0_index_index->mutable_string_value()->set_length((*arg_val_0)[arg_0_index][arg_0_array_arg_0_index_index].size());
                            }
                        }
                    } else {
                        LOG(WARNING) << "argument 0 is null.";
                    }
                    break;
                }
                case details::HidlInstrumentor::CLIENT_API_EXIT:
                case details::HidlInstrumentor::SERVER_API_EXIT:
                case details::HidlInstrumentor::PASSTHROUGH_EXIT:
                {
                    if ((*args).size() != 1) {
                        LOG(ERROR) << "Number of return values does not match. expect: 1, actual: " << (*args).size() << ", method name: transpose2, event type: " << event;
                        break;
                    }
                    auto *result_0 __attribute__((__unused__)) = msg.add_return_type_hidl();
                    ::android::hardware::hidl_array<::android::hardware::hidl_string, 3, 5> *result_val_0 __attribute__((__unused__)) = reinterpret_cast<::android::hardware::hidl_array<::android::hardware::hidl_string, 3, 5>*> ((*args)[0]);
                    if (result_val_0 != nullptr) {
                        result_0->set_type(TYPE_ARRAY);
                        result_0->set_vector_size(3);
                        for (int result_0_index = 0; result_0_index < 3; result_0_index++) {
                            auto *result_0_array_result_0_index __attribute__((__unused__)) = result_0->add_vector_value();
                            result_0_array_result_0_index->set_type(TYPE_ARRAY);
                            result_0_array_result_0_index->set_vector_size(5);
                            for (int result_0_array_result_0_index_index = 0; result_0_array_result_0_index_index < 5; result_0_array_result_0_index_index++) {
                                auto *result_0_array_result_0_index_array_result_0_array_result_0_index_index __attribute__((__unused__)) = result_0_array_result_0_index->add_vector_value();
                                result_0_array_result_0_index_array_result_0_array_result_0_index_index->set_type(TYPE_STRING);
                                result_0_array_result_0_index_array_result_0_array_result_0_index_index->mutable_string_value()->set_message((*result_val_0)[result_0_index][result_0_array_result_0_index_index].c_str());
                                result_0_array_result_0_index_array_result_0_array_result_0_index_index->mutable_string_value()->set_length((*result_val_0)[result_0_index][result_0_array_result_0_index_index].size());
                            }
                        }
                    } else {
                        LOG(WARNING) << "return value 0 is null.";
                    }
                    break;
                }
                default:
                {
                    LOG(WARNING) << "not supported. ";
                    break;
                }
            }
        }
        profiler.AddTraceEvent(event, package, version, interface, msg);
    }
    if (strcmp(method, "sendVec") == 0) {
        FunctionSpecificationMessage msg;
        msg.set_name("sendVec");
        if (!args) {
            LOG(WARNING) << "no argument passed";
        } else {
            switch (event) {
                case details::HidlInstrumentor::CLIENT_API_ENTRY:
                case details::HidlInstrumentor::SERVER_API_ENTRY:
                case details::HidlInstrumentor::PASSTHROUGH_ENTRY:
                {
                    if ((*args).size() != 1) {
                        LOG(ERROR) << "Number of arguments does not match. expect: 1, actual: " << (*args).size() << ", method name: sendVec, event type: " << event;
                        break;
                    }
                    auto *arg_0 __attribute__((__unused__)) = msg.add_arg();
                    ::android::hardware::hidl_vec<uint8_t> *arg_val_0 __attribute__((__unused__)) = reinterpret_cast<::android::hardware::hidl_vec<uint8_t>*> ((*args)[0]);
                    if (arg_val_0 != nullptr) {
                        arg_0->set_type(TYPE_VECTOR);
                        arg_0->set_vector_size((*arg_val_0).size());
                        for (int arg_0_index = 0; arg_0_index < (int)(*arg_val_0).size(); arg_0_index++) {
                            auto *arg_0_vector_arg_0_index __attribute__((__unused__)) = arg_0->add_vector_value();
                            arg_0_vector_arg_0_index->set_type(TYPE_SCALAR);
                            arg_0_vector_arg_0_index->mutable_scalar_value()->set_uint8_t((*arg_val_0)[arg_0_index]);
                        }
                    } else {
                        LOG(WARNING) << "argument 0 is null.";
                    }
                    break;
                }
                case details::HidlInstrumentor::CLIENT_API_EXIT:
                case details::HidlInstrumentor::SERVER_API_EXIT:
                case details::HidlInstrumentor::PASSTHROUGH_EXIT:
                {
                    if ((*args).size() != 1) {
                        LOG(ERROR) << "Number of return values does not match. expect: 1, actual: " << (*args).size() << ", method name: sendVec, event type: " << event;
                        break;
                    }
                    auto *result_0 __attribute__((__unused__)) = msg.add_return_type_hidl();
                    ::android::hardware::hidl_vec<uint8_t> *result_val_0 __attribute__((__unused__)) = reinterpret_cast<::android::hardware::hidl_vec<uint8_t>*> ((*args)[0]);
                    if (result_val_0 != nullptr) {
                        result_0->set_type(TYPE_VECTOR);
                        result_0->set_vector_size((*result_val_0).size());
                        for (int result_0_index = 0; result_0_index < (int)(*result_val_0).size(); result_0_index++) {
                            auto *result_0_vector_result_0_index __attribute__((__unused__)) = result_0->add_vector_value();
                            result_0_vector_result_0_index->set_type(TYPE_SCALAR);
                            result_0_vector_result_0_index->mutable_scalar_value()->set_uint8_t((*result_val_0)[result_0_index]);
                        }
                    } else {
                        LOG(WARNING) << "return value 0 is null.";
                    }
                    break;
                }
                default:
                {
                    LOG(WARNING) << "not supported. ";
                    break;
                }
            }
        }
        profiler.AddTraceEvent(event, package, version, interface, msg);
    }
    if (strcmp(method, "sendVecVec") == 0) {
        FunctionSpecificationMessage msg;
        msg.set_name("sendVecVec");
        if (!args) {
            LOG(WARNING) << "no argument passed";
        } else {
            switch (event) {
                case details::HidlInstrumentor::CLIENT_API_ENTRY:
                case details::HidlInstrumentor::SERVER_API_ENTRY:
                case details::HidlInstrumentor::PASSTHROUGH_ENTRY:
                {
                    if ((*args).size() != 0) {
                        LOG(ERROR) << "Number of arguments does not match. expect: 0, actual: " << (*args).size() << ", method name: sendVecVec, event type: " << event;
                        break;
                    }
                    break;
                }
                case details::HidlInstrumentor::CLIENT_API_EXIT:
                case details::HidlInstrumentor::SERVER_API_EXIT:
                case details::HidlInstrumentor::PASSTHROUGH_EXIT:
                {
                    if ((*args).size() != 1) {
                        LOG(ERROR) << "Number of return values does not match. expect: 1, actual: " << (*args).size() << ", method name: sendVecVec, event type: " << event;
                        break;
                    }
                    auto *result_0 __attribute__((__unused__)) = msg.add_return_type_hidl();
                    ::android::hardware::hidl_vec<::android::hardware::hidl_vec<uint8_t>> *result_val_0 __attribute__((__unused__)) = reinterpret_cast<::android::hardware::hidl_vec<::android::hardware::hidl_vec<uint8_t>>*> ((*args)[0]);
                    if (result_val_0 != nullptr) {
                        result_0->set_type(TYPE_VECTOR);
                        result_0->set_vector_size((*result_val_0).size());
                        for (int result_0_index = 0; result_0_index < (int)(*result_val_0).size(); result_0_index++) {
                            auto *result_0_vector_result_0_index __attribute__((__unused__)) = result_0->add_vector_value();
                            result_0_vector_result_0_index->set_type(TYPE_VECTOR);
                            result_0_vector_result_0_index->set_vector_size((*result_val_0)[result_0_index].size());
                            for (int result_0_vector_result_0_index_index = 0; result_0_vector_result_0_index_index < (int)(*result_val_0)[result_0_index].size(); result_0_vector_result_0_index_index++) {
                                auto *result_0_vector_result_0_index_vector_result_0_vector_result_0_index_index __attribute__((__unused__)) = result_0_vector_result_0_index->add_vector_value();
                                result_0_vector_result_0_index_vector_result_0_vector_result_0_index_index->set_type(TYPE_SCALAR);
                                result_0_vector_result_0_index_vector_result_0_vector_result_0_index_index->mutable_scalar_value()->set_uint8_t((*result_val_0)[result_0_index][result_0_vector_result_0_index_index]);
                            }
                        }
                    } else {
                        LOG(WARNING) << "return value 0 is null.";
                    }
                    break;
                }
                default:
                {
                    LOG(WARNING) << "not supported. ";
                    break;
                }
            }
        }
        profiler.AddTraceEvent(event, package, version, interface, msg);
    }
    if (strcmp(method, "haveAVectorOfInterfaces") == 0) {
        FunctionSpecificationMessage msg;
        msg.set_name("haveAVectorOfInterfaces");
        if (!args) {
            LOG(WARNING) << "no argument passed";
        } else {
            switch (event) {
                case details::HidlInstrumentor::CLIENT_API_ENTRY:
                case details::HidlInstrumentor::SERVER_API_ENTRY:
                case details::HidlInstrumentor::PASSTHROUGH_ENTRY:
                {
                    if ((*args).size() != 1) {
                        LOG(ERROR) << "Number of arguments does not match. expect: 1, actual: " << (*args).size() << ", method name: haveAVectorOfInterfaces, event type: " << event;
                        break;
                    }
                    auto *arg_0 __attribute__((__unused__)) = msg.add_arg();
                    ::android::hardware::hidl_vec<sp<::android::hardware::tests::foo::V1_0::ISimple>> *arg_val_0 __attribute__((__unused__)) = reinterpret_cast<::android::hardware::hidl_vec<sp<::android::hardware::tests::foo::V1_0::ISimple>>*> ((*args)[0]);
                    if (arg_val_0 != nullptr) {
                        arg_0->set_type(TYPE_VECTOR);
                        arg_0->set_vector_size((*arg_val_0).size());
                        for (int arg_0_index = 0; arg_0_index < (int)(*arg_val_0).size(); arg_0_index++) {
                            auto *arg_0_vector_arg_0_index __attribute__((__unused__)) = arg_0->add_vector_value();
                            arg_0_vector_arg_0_index->set_type(TYPE_HIDL_INTERFACE);
                            arg_0_vector_arg_0_index->set_predefined_type("::android::hardware::tests::foo::V1_0::ISimple");
                        }
                    } else {
                        LOG(WARNING) << "argument 0 is null.";
                    }
                    break;
                }
                case details::HidlInstrumentor::CLIENT_API_EXIT:
                case details::HidlInstrumentor::SERVER_API_EXIT:
                case details::HidlInstrumentor::PASSTHROUGH_EXIT:
                {
                    if ((*args).size() != 1) {
                        LOG(ERROR) << "Number of return values does not match. expect: 1, actual: " << (*args).size() << ", method name: haveAVectorOfInterfaces, event type: " << event;
                        break;
                    }
                    auto *result_0 __attribute__((__unused__)) = msg.add_return_type_hidl();
                    ::android::hardware::hidl_vec<sp<::android::hardware::tests::foo::V1_0::ISimple>> *result_val_0 __attribute__((__unused__)) = reinterpret_cast<::android::hardware::hidl_vec<sp<::android::hardware::tests::foo::V1_0::ISimple>>*> ((*args)[0]);
                    if (result_val_0 != nullptr) {
                        result_0->set_type(TYPE_VECTOR);
                        result_0->set_vector_size((*result_val_0).size());
                        for (int result_0_index = 0; result_0_index < (int)(*result_val_0).size(); result_0_index++) {
                            auto *result_0_vector_result_0_index __attribute__((__unused__)) = result_0->add_vector_value();
                            result_0_vector_result_0_index->set_type(TYPE_HIDL_INTERFACE);
                            result_0_vector_result_0_index->set_predefined_type("::android::hardware::tests::foo::V1_0::ISimple");
                        }
                    } else {
                        LOG(WARNING) << "return value 0 is null.";
                    }
                    break;
                }
                default:
                {
                    LOG(WARNING) << "not supported. ";
                    break;
                }
            }
        }
        profiler.AddTraceEvent(event, package, version, interface, msg);
    }
    if (strcmp(method, "haveAVectorOfGenericInterfaces") == 0) {
        FunctionSpecificationMessage msg;
        msg.set_name("haveAVectorOfGenericInterfaces");
        if (!args) {
            LOG(WARNING) << "no argument passed";
        } else {
            switch (event) {
                case details::HidlInstrumentor::CLIENT_API_ENTRY:
                case details::HidlInstrumentor::SERVER_API_ENTRY:
                case details::HidlInstrumentor::PASSTHROUGH_ENTRY:
                {
                    if ((*args).size() != 1) {
                        LOG(ERROR) << "Number of arguments does not match. expect: 1, actual: " << (*args).size() << ", method name: haveAVectorOfGenericInterfaces, event type: " << event;
                        break;
                    }
                    auto *arg_0 __attribute__((__unused__)) = msg.add_arg();
                    ::android::hardware::hidl_vec<sp<::android::hidl::base::V1_0::IBase>> *arg_val_0 __attribute__((__unused__)) = reinterpret_cast<::android::hardware::hidl_vec<sp<::android::hidl::base::V1_0::IBase>>*> ((*args)[0]);
                    if (arg_val_0 != nullptr) {
                        arg_0->set_type(TYPE_VECTOR);
                        arg_0->set_vector_size((*arg_val_0).size());
                        for (int arg_0_index = 0; arg_0_index < (int)(*arg_val_0).size(); arg_0_index++) {
                            auto *arg_0_vector_arg_0_index __attribute__((__unused__)) = arg_0->add_vector_value();
                            arg_0_vector_arg_0_index->set_type(TYPE_HIDL_INTERFACE);
                            arg_0_vector_arg_0_index->set_predefined_type("::android::hidl::base::V1_0::IBase");
                        }
                    } else {
                        LOG(WARNING) << "argument 0 is null.";
                    }
                    break;
                }
                case details::HidlInstrumentor::CLIENT_API_EXIT:
                case details::HidlInstrumentor::SERVER_API_EXIT:
                case details::HidlInstrumentor::PASSTHROUGH_EXIT:
                {
                    if ((*args).size() != 1) {
                        LOG(ERROR) << "Number of return values does not match. expect: 1, actual: " << (*args).size() << ", method name: haveAVectorOfGenericInterfaces, event type: " << event;
                        break;
                    }
                    auto *result_0 __attribute__((__unused__)) = msg.add_return_type_hidl();
                    ::android::hardware::hidl_vec<sp<::android::hidl::base::V1_0::IBase>> *result_val_0 __attribute__((__unused__)) = reinterpret_cast<::android::hardware::hidl_vec<sp<::android::hidl::base::V1_0::IBase>>*> ((*args)[0]);
                    if (result_val_0 != nullptr) {
                        result_0->set_type(TYPE_VECTOR);
                        result_0->set_vector_size((*result_val_0).size());
                        for (int result_0_index = 0; result_0_index < (int)(*result_val_0).size(); result_0_index++) {
                            auto *result_0_vector_result_0_index __attribute__((__unused__)) = result_0->add_vector_value();
                            result_0_vector_result_0_index->set_type(TYPE_HIDL_INTERFACE);
                            result_0_vector_result_0_index->set_predefined_type("::android::hidl::base::V1_0::IBase");
                        }
                    } else {
                        LOG(WARNING) << "return value 0 is null.";
                    }
                    break;
                }
                default:
                {
                    LOG(WARNING) << "not supported. ";
                    break;
                }
            }
        }
        profiler.AddTraceEvent(event, package, version, interface, msg);
    }
    if (strcmp(method, "echoNullInterface") == 0) {
        FunctionSpecificationMessage msg;
        msg.set_name("echoNullInterface");
        if (!args) {
            LOG(WARNING) << "no argument passed";
        } else {
            switch (event) {
                case details::HidlInstrumentor::CLIENT_API_ENTRY:
                case details::HidlInstrumentor::SERVER_API_ENTRY:
                case details::HidlInstrumentor::PASSTHROUGH_ENTRY:
                {
                    if ((*args).size() != 1) {
                        LOG(ERROR) << "Number of arguments does not match. expect: 1, actual: " << (*args).size() << ", method name: echoNullInterface, event type: " << event;
                        break;
                    }
                    auto *arg_0 __attribute__((__unused__)) = msg.add_arg();
                    sp<::android::hardware::tests::foo::V1_0::IFooCallback> *arg_val_0 __attribute__((__unused__)) = reinterpret_cast<sp<::android::hardware::tests::foo::V1_0::IFooCallback>*> ((*args)[0]);
                    if (arg_val_0 != nullptr) {
                        arg_0->set_type(TYPE_HIDL_CALLBACK);
                        arg_0->set_predefined_type("::android::hardware::tests::foo::V1_0::IFooCallback");
                    } else {
                        LOG(WARNING) << "argument 0 is null.";
                    }
                    break;
                }
                case details::HidlInstrumentor::CLIENT_API_EXIT:
                case details::HidlInstrumentor::SERVER_API_EXIT:
                case details::HidlInstrumentor::PASSTHROUGH_EXIT:
                {
                    if ((*args).size() != 2) {
                        LOG(ERROR) << "Number of return values does not match. expect: 2, actual: " << (*args).size() << ", method name: echoNullInterface, event type: " << event;
                        break;
                    }
                    auto *result_0 __attribute__((__unused__)) = msg.add_return_type_hidl();
                    bool *result_val_0 __attribute__((__unused__)) = reinterpret_cast<bool*> ((*args)[0]);
                    if (result_val_0 != nullptr) {
                        result_0->set_type(TYPE_SCALAR);
                        result_0->mutable_scalar_value()->set_bool_t((*result_val_0));
                    } else {
                        LOG(WARNING) << "return value 0 is null.";
                    }
                    auto *result_1 __attribute__((__unused__)) = msg.add_return_type_hidl();
                    sp<::android::hardware::tests::foo::V1_0::IFooCallback> *result_val_1 __attribute__((__unused__)) = reinterpret_cast<sp<::android::hardware::tests::foo::V1_0::IFooCallback>*> ((*args)[1]);
                    if (result_val_1 != nullptr) {
                        result_1->set_type(TYPE_HIDL_CALLBACK);
                        result_1->set_predefined_type("::android::hardware::tests::foo::V1_0::IFooCallback");
                    } else {
                        LOG(WARNING) << "return value 1 is null.";
                    }
                    break;
                }
                default:
                {
                    LOG(WARNING) << "not supported. ";
                    break;
                }
            }
        }
        profiler.AddTraceEvent(event, package, version, interface, msg);
    }
    if (strcmp(method, "createMyHandle") == 0) {
        FunctionSpecificationMessage msg;
        msg.set_name("createMyHandle");
        if (!args) {
            LOG(WARNING) << "no argument passed";
        } else {
            switch (event) {
                case details::HidlInstrumentor::CLIENT_API_ENTRY:
                case details::HidlInstrumentor::SERVER_API_ENTRY:
                case details::HidlInstrumentor::PASSTHROUGH_ENTRY:
                {
                    if ((*args).size() != 0) {
                        LOG(ERROR) << "Number of arguments does not match. expect: 0, actual: " << (*args).size() << ", method name: createMyHandle, event type: " << event;
                        break;
                    }
                    break;
                }
                case details::HidlInstrumentor::CLIENT_API_EXIT:
                case details::HidlInstrumentor::SERVER_API_EXIT:
                case details::HidlInstrumentor::PASSTHROUGH_EXIT:
                {
                    if ((*args).size() != 1) {
                        LOG(ERROR) << "Number of return values does not match. expect: 1, actual: " << (*args).size() << ", method name: createMyHandle, event type: " << event;
                        break;
                    }
                    auto *result_0 __attribute__((__unused__)) = msg.add_return_type_hidl();
                    ::android::hardware::tests::foo::V1_0::IFoo::MyHandle *result_val_0 __attribute__((__unused__)) = reinterpret_cast<::android::hardware::tests::foo::V1_0::IFoo::MyHandle*> ((*args)[0]);
                    if (result_val_0 != nullptr) {
                        result_0->set_type(TYPE_STRUCT);
                        profile____android__hardware__tests__foo__V1_0__IFoo__MyHandle(result_0, (*result_val_0));
                    } else {
                        LOG(WARNING) << "return value 0 is null.";
                    }
                    break;
                }
                default:
                {
                    LOG(WARNING) << "not supported. ";
                    break;
                }
            }
        }
        profiler.AddTraceEvent(event, package, version, interface, msg);
    }
    if (strcmp(method, "createHandles") == 0) {
        FunctionSpecificationMessage msg;
        msg.set_name("createHandles");
        if (!args) {
            LOG(WARNING) << "no argument passed";
        } else {
            switch (event) {
                case details::HidlInstrumentor::CLIENT_API_ENTRY:
                case details::HidlInstrumentor::SERVER_API_ENTRY:
                case details::HidlInstrumentor::PASSTHROUGH_ENTRY:
                {
                    if ((*args).size() != 1) {
                        LOG(ERROR) << "Number of arguments does not match. expect: 1, actual: " << (*args).size() << ", method name: createHandles, event type: " << event;
                        break;
                    }
                    auto *arg_0 __attribute__((__unused__)) = msg.add_arg();
                    uint32_t *arg_val_0 __attribute__((__unused__)) = reinterpret_cast<uint32_t*> ((*args)[0]);
                    if (arg_val_0 != nullptr) {
                        arg_0->set_type(TYPE_SCALAR);
                        arg_0->mutable_scalar_value()->set_uint32_t((*arg_val_0));
                    } else {
                        LOG(WARNING) << "argument 0 is null.";
                    }
                    break;
                }
                case details::HidlInstrumentor::CLIENT_API_EXIT:
                case details::HidlInstrumentor::SERVER_API_EXIT:
                case details::HidlInstrumentor::PASSTHROUGH_EXIT:
                {
                    if ((*args).size() != 1) {
                        LOG(ERROR) << "Number of return values does not match. expect: 1, actual: " << (*args).size() << ", method name: createHandles, event type: " << event;
                        break;
                    }
                    auto *result_0 __attribute__((__unused__)) = msg.add_return_type_hidl();
                    ::android::hardware::hidl_vec<::android::hardware::hidl_handle> *result_val_0 __attribute__((__unused__)) = reinterpret_cast<::android::hardware::hidl_vec<::android::hardware::hidl_handle>*> ((*args)[0]);
                    if (result_val_0 != nullptr) {
                        result_0->set_type(TYPE_VECTOR);
                        result_0->set_vector_size((*result_val_0).size());
                        for (int result_0_index = 0; result_0_index < (int)(*result_val_0).size(); result_0_index++) {
                            auto *result_0_vector_result_0_index __attribute__((__unused__)) = result_0->add_vector_value();
                            result_0_vector_result_0_index->set_type(TYPE_HANDLE);
                            auto result_0_vector_result_0_index_h = (*result_val_0)[result_0_index].getNativeHandle();
                            if (!result_0_vector_result_0_index_h) {
                                LOG(WARNING) << "null handle";
                                return;
                            }
                            result_0_vector_result_0_index->mutable_handle_value()->set_version(result_0_vector_result_0_index_h->version);
                            result_0_vector_result_0_index->mutable_handle_value()->set_num_ints(result_0_vector_result_0_index_h->numInts);
                            result_0_vector_result_0_index->mutable_handle_value()->set_num_fds(result_0_vector_result_0_index_h->numFds);
                            for (int i = 0; i < result_0_vector_result_0_index_h->numInts + result_0_vector_result_0_index_h->numFds; i++) {
                                if(i < result_0_vector_result_0_index_h->numFds) {
                                    auto* fd_val_i = result_0_vector_result_0_index->mutable_handle_value()->add_fd_val();
                                    char filePath[PATH_MAX];
                                    string procPath = "/proc/self/fd/" + to_string(result_0_vector_result_0_index_h->data[i]);
                                    ssize_t r = readlink(procPath.c_str(), filePath, sizeof(filePath));
                                    if (r == -1) {
                                        LOG(ERROR) << "Unable to get file path";
                                        continue;
                                    }
                                    filePath[r] = '\0';
                                    fd_val_i->set_file_name(filePath);
                                    struct stat statbuf;
                                    fstat(result_0_vector_result_0_index_h->data[i], &statbuf);
                                    fd_val_i->set_mode(statbuf.st_mode);
                                    if (S_ISREG(statbuf.st_mode) || S_ISDIR(statbuf.st_mode)) {
                                        fd_val_i->set_type(S_ISREG(statbuf.st_mode)? FILE_TYPE: DIR_TYPE);
                                        int flags = fcntl(result_0_vector_result_0_index_h->data[i], F_GETFL);
                                        fd_val_i->set_flags(flags);
                                    }
                                    else if (S_ISCHR(statbuf.st_mode) || S_ISBLK(statbuf.st_mode)) {
                                        fd_val_i->set_type(DEV_TYPE);
                                        if (strcmp(filePath, "/dev/ashmem") == 0) {
                                            int size = ashmem_get_size_region(result_0_vector_result_0_index_h->data[i]);
                                            fd_val_i->mutable_memory()->set_size(size);
                                        }
                                    }
                                    else if (S_ISFIFO(statbuf.st_mode)){
                                        fd_val_i->set_type(PIPE_TYPE);
                                    }
                                    else if (S_ISSOCK(statbuf.st_mode)) {
                                        fd_val_i->set_type(SOCKET_TYPE);
                                    }
                                    else {
                                        fd_val_i->set_type(LINK_TYPE);
                                    }
                                } else {
                                    result_0_vector_result_0_index->mutable_handle_value()->add_int_val(result_0_vector_result_0_index_h->data[i]);
                                }
                            }
                        }
                    } else {
                        LOG(WARNING) << "return value 0 is null.";
                    }
                    break;
                }
                default:
                {
                    LOG(WARNING) << "not supported. ";
                    break;
                }
            }
        }
        profiler.AddTraceEvent(event, package, version, interface, msg);
    }
    if (strcmp(method, "closeHandles") == 0) {
        FunctionSpecificationMessage msg;
        msg.set_name("closeHandles");
        if (!args) {
            LOG(WARNING) << "no argument passed";
        } else {
            switch (event) {
                case details::HidlInstrumentor::CLIENT_API_ENTRY:
                case details::HidlInstrumentor::SERVER_API_ENTRY:
                case details::HidlInstrumentor::PASSTHROUGH_ENTRY:
                {
                    if ((*args).size() != 0) {
                        LOG(ERROR) << "Number of arguments does not match. expect: 0, actual: " << (*args).size() << ", method name: closeHandles, event type: " << event;
                        break;
                    }
                    break;
                }
                case details::HidlInstrumentor::CLIENT_API_EXIT:
                case details::HidlInstrumentor::SERVER_API_EXIT:
                case details::HidlInstrumentor::PASSTHROUGH_EXIT:
                {
                    if ((*args).size() != 0) {
                        LOG(ERROR) << "Number of return values does not match. expect: 0, actual: " << (*args).size() << ", method name: closeHandles, event type: " << event;
                        break;
                    }
                    break;
                }
                default:
                {
                    LOG(WARNING) << "not supported. ";
                    break;
                }
            }
        }
        profiler.AddTraceEvent(event, package, version, interface, msg);
    }
    if (strcmp(method, "thisIsNew") == 0) {
        FunctionSpecificationMessage msg;
        msg.set_name("thisIsNew");
        if (!args) {
            LOG(WARNING) << "no argument passed";
        } else {
            switch (event) {
                case details::HidlInstrumentor::CLIENT_API_ENTRY:
                case details::HidlInstrumentor::SERVER_API_ENTRY:
                case details::HidlInstrumentor::PASSTHROUGH_ENTRY:
                {
                    if ((*args).size() != 0) {
                        LOG(ERROR) << "Number of arguments does not match. expect: 0, actual: " << (*args).size() << ", method name: thisIsNew, event type: " << event;
                        break;
                    }
                    break;
                }
                case details::HidlInstrumentor::CLIENT_API_EXIT:
                case details::HidlInstrumentor::SERVER_API_EXIT:
                case details::HidlInstrumentor::PASSTHROUGH_EXIT:
                {
                    if ((*args).size() != 0) {
                        LOG(ERROR) << "Number of return values does not match. expect: 0, actual: " << (*args).size() << ", method name: thisIsNew, event type: " << event;
                        break;
                    }
                    break;
                }
                default:
                {
                    LOG(WARNING) << "not supported. ";
                    break;
                }
            }
        }
        profiler.AddTraceEvent(event, package, version, interface, msg);
    }
    if (strcmp(method, "expectNullHandle") == 0) {
        FunctionSpecificationMessage msg;
        msg.set_name("expectNullHandle");
        if (!args) {
            LOG(WARNING) << "no argument passed";
        } else {
            switch (event) {
                case details::HidlInstrumentor::CLIENT_API_ENTRY:
                case details::HidlInstrumentor::SERVER_API_ENTRY:
                case details::HidlInstrumentor::PASSTHROUGH_ENTRY:
                {
                    if ((*args).size() != 2) {
                        LOG(ERROR) << "Number of arguments does not match. expect: 2, actual: " << (*args).size() << ", method name: expectNullHandle, event type: " << event;
                        break;
                    }
                    auto *arg_0 __attribute__((__unused__)) = msg.add_arg();
                    ::android::hardware::hidl_handle *arg_val_0 __attribute__((__unused__)) = reinterpret_cast<::android::hardware::hidl_handle*> ((*args)[0]);
                    if (arg_val_0 != nullptr) {
                        arg_0->set_type(TYPE_HANDLE);
                        auto arg_0_h = (*arg_val_0).getNativeHandle();
                        if (!arg_0_h) {
                            LOG(WARNING) << "null handle";
                            return;
                        }
                        arg_0->mutable_handle_value()->set_version(arg_0_h->version);
                        arg_0->mutable_handle_value()->set_num_ints(arg_0_h->numInts);
                        arg_0->mutable_handle_value()->set_num_fds(arg_0_h->numFds);
                        for (int i = 0; i < arg_0_h->numInts + arg_0_h->numFds; i++) {
                            if(i < arg_0_h->numFds) {
                                auto* fd_val_i = arg_0->mutable_handle_value()->add_fd_val();
                                char filePath[PATH_MAX];
                                string procPath = "/proc/self/fd/" + to_string(arg_0_h->data[i]);
                                ssize_t r = readlink(procPath.c_str(), filePath, sizeof(filePath));
                                if (r == -1) {
                                    LOG(ERROR) << "Unable to get file path";
                                    continue;
                                }
                                filePath[r] = '\0';
                                fd_val_i->set_file_name(filePath);
                                struct stat statbuf;
                                fstat(arg_0_h->data[i], &statbuf);
                                fd_val_i->set_mode(statbuf.st_mode);
                                if (S_ISREG(statbuf.st_mode) || S_ISDIR(statbuf.st_mode)) {
                                    fd_val_i->set_type(S_ISREG(statbuf.st_mode)? FILE_TYPE: DIR_TYPE);
                                    int flags = fcntl(arg_0_h->data[i], F_GETFL);
                                    fd_val_i->set_flags(flags);
                                }
                                else if (S_ISCHR(statbuf.st_mode) || S_ISBLK(statbuf.st_mode)) {
                                    fd_val_i->set_type(DEV_TYPE);
                                    if (strcmp(filePath, "/dev/ashmem") == 0) {
                                        int size = ashmem_get_size_region(arg_0_h->data[i]);
                                        fd_val_i->mutable_memory()->set_size(size);
                                    }
                                }
                                else if (S_ISFIFO(statbuf.st_mode)){
                                    fd_val_i->set_type(PIPE_TYPE);
                                }
                                else if (S_ISSOCK(statbuf.st_mode)) {
                                    fd_val_i->set_type(SOCKET_TYPE);
                                }
                                else {
                                    fd_val_i->set_type(LINK_TYPE);
                                }
                            } else {
                                arg_0->mutable_handle_value()->add_int_val(arg_0_h->data[i]);
                            }
                        }
                    } else {
                        LOG(WARNING) << "argument 0 is null.";
                    }
                    auto *arg_1 __attribute__((__unused__)) = msg.add_arg();
                    ::android::hardware::tests::foo::V1_0::Abc *arg_val_1 __attribute__((__unused__)) = reinterpret_cast<::android::hardware::tests::foo::V1_0::Abc*> ((*args)[1]);
                    if (arg_val_1 != nullptr) {
                        arg_1->set_type(TYPE_STRUCT);
                        profile____android__hardware__tests__foo__V1_0__Abc(arg_1, (*arg_val_1));
                    } else {
                        LOG(WARNING) << "argument 1 is null.";
                    }
                    break;
                }
                case details::HidlInstrumentor::CLIENT_API_EXIT:
                case details::HidlInstrumentor::SERVER_API_EXIT:
                case details::HidlInstrumentor::PASSTHROUGH_EXIT:
                {
                    if ((*args).size() != 2) {
                        LOG(ERROR) << "Number of return values does not match. expect: 2, actual: " << (*args).size() << ", method name: expectNullHandle, event type: " << event;
                        break;
                    }
                    auto *result_0 __attribute__((__unused__)) = msg.add_return_type_hidl();
                    bool *result_val_0 __attribute__((__unused__)) = reinterpret_cast<bool*> ((*args)[0]);
                    if (result_val_0 != nullptr) {
                        result_0->set_type(TYPE_SCALAR);
                        result_0->mutable_scalar_value()->set_bool_t((*result_val_0));
                    } else {
                        LOG(WARNING) << "return value 0 is null.";
                    }
                    auto *result_1 __attribute__((__unused__)) = msg.add_return_type_hidl();
                    bool *result_val_1 __attribute__((__unused__)) = reinterpret_cast<bool*> ((*args)[1]);
                    if (result_val_1 != nullptr) {
                        result_1->set_type(TYPE_SCALAR);
                        result_1->mutable_scalar_value()->set_bool_t((*result_val_1));
                    } else {
                        LOG(WARNING) << "return value 1 is null.";
                    }
                    break;
                }
                default:
                {
                    LOG(WARNING) << "not supported. ";
                    break;
                }
            }
        }
        profiler.AddTraceEvent(event, package, version, interface, msg);
    }
    if (strcmp(method, "takeAMask") == 0) {
        FunctionSpecificationMessage msg;
        msg.set_name("takeAMask");
        if (!args) {
            LOG(WARNING) << "no argument passed";
        } else {
            switch (event) {
                case details::HidlInstrumentor::CLIENT_API_ENTRY:
                case details::HidlInstrumentor::SERVER_API_ENTRY:
                case details::HidlInstrumentor::PASSTHROUGH_ENTRY:
                {
                    if ((*args).size() != 4) {
                        LOG(ERROR) << "Number of arguments does not match. expect: 4, actual: " << (*args).size() << ", method name: takeAMask, event type: " << event;
                        break;
                    }
                    auto *arg_0 __attribute__((__unused__)) = msg.add_arg();
                    ::android::hardware::tests::foo::V1_0::IFoo::BitField *arg_val_0 __attribute__((__unused__)) = reinterpret_cast<::android::hardware::tests::foo::V1_0::IFoo::BitField*> ((*args)[0]);
                    if (arg_val_0 != nullptr) {
                        arg_0->set_type(TYPE_ENUM);
                        profile____android__hardware__tests__foo__V1_0__IFoo__BitField(arg_0, (*arg_val_0));
                    } else {
                        LOG(WARNING) << "argument 0 is null.";
                    }
                    auto *arg_1 __attribute__((__unused__)) = msg.add_arg();
                    uint8_t *arg_val_1 __attribute__((__unused__)) = reinterpret_cast<uint8_t*> ((*args)[1]);
                    if (arg_val_1 != nullptr) {
                        arg_1->set_type(TYPE_MASK);
                        arg_1->set_scalar_type("uint8_t");
                        arg_1->mutable_scalar_value()->set_uint8_t((*arg_val_1));
                    } else {
                        LOG(WARNING) << "argument 1 is null.";
                    }
                    auto *arg_2 __attribute__((__unused__)) = msg.add_arg();
                    ::android::hardware::tests::foo::V1_0::IFoo::MyMask *arg_val_2 __attribute__((__unused__)) = reinterpret_cast<::android::hardware::tests::foo::V1_0::IFoo::MyMask*> ((*args)[2]);
                    if (arg_val_2 != nullptr) {
                        arg_2->set_type(TYPE_STRUCT);
                        profile____android__hardware__tests__foo__V1_0__IFoo__MyMask(arg_2, (*arg_val_2));
                    } else {
                        LOG(WARNING) << "argument 2 is null.";
                    }
                    auto *arg_3 __attribute__((__unused__)) = msg.add_arg();
                    uint8_t *arg_val_3 __attribute__((__unused__)) = reinterpret_cast<uint8_t*> ((*args)[3]);
                    if (arg_val_3 != nullptr) {
                        arg_3->set_type(TYPE_MASK);
                        arg_3->set_scalar_type("uint8_t");
                        arg_3->mutable_scalar_value()->set_uint8_t((*arg_val_3));
                    } else {
                        LOG(WARNING) << "argument 3 is null.";
                    }
                    break;
                }
                case details::HidlInstrumentor::CLIENT_API_EXIT:
                case details::HidlInstrumentor::SERVER_API_EXIT:
                case details::HidlInstrumentor::PASSTHROUGH_EXIT:
                {
                    if ((*args).size() != 4) {
                        LOG(ERROR) << "Number of return values does not match. expect: 4, actual: " << (*args).size() << ", method name: takeAMask, event type: " << event;
                        break;
                    }
                    auto *result_0 __attribute__((__unused__)) = msg.add_return_type_hidl();
                    ::android::hardware::tests::foo::V1_0::IFoo::BitField *result_val_0 __attribute__((__unused__)) = reinterpret_cast<::android::hardware::tests::foo::V1_0::IFoo::BitField*> ((*args)[0]);
                    if (result_val_0 != nullptr) {
                        result_0->set_type(TYPE_ENUM);
                        profile____android__hardware__tests__foo__V1_0__IFoo__BitField(result_0, (*result_val_0));
                    } else {
                        LOG(WARNING) << "return value 0 is null.";
                    }
                    auto *result_1 __attribute__((__unused__)) = msg.add_return_type_hidl();
                    uint8_t *result_val_1 __attribute__((__unused__)) = reinterpret_cast<uint8_t*> ((*args)[1]);
                    if (result_val_1 != nullptr) {
                        result_1->set_type(TYPE_SCALAR);
                        result_1->mutable_scalar_value()->set_uint8_t((*result_val_1));
                    } else {
                        LOG(WARNING) << "return value 1 is null.";
                    }
                    auto *result_2 __attribute__((__unused__)) = msg.add_return_type_hidl();
                    uint8_t *result_val_2 __attribute__((__unused__)) = reinterpret_cast<uint8_t*> ((*args)[2]);
                    if (result_val_2 != nullptr) {
                        result_2->set_type(TYPE_SCALAR);
                        result_2->mutable_scalar_value()->set_uint8_t((*result_val_2));
                    } else {
                        LOG(WARNING) << "return value 2 is null.";
                    }
                    auto *result_3 __attribute__((__unused__)) = msg.add_return_type_hidl();
                    uint8_t *result_val_3 __attribute__((__unused__)) = reinterpret_cast<uint8_t*> ((*args)[3]);
                    if (result_val_3 != nullptr) {
                        result_3->set_type(TYPE_SCALAR);
                        result_3->mutable_scalar_value()->set_uint8_t((*result_val_3));
                    } else {
                        LOG(WARNING) << "return value 3 is null.";
                    }
                    break;
                }
                default:
                {
                    LOG(WARNING) << "not supported. ";
                    break;
                }
            }
        }
        profiler.AddTraceEvent(event, package, version, interface, msg);
    }
    if (strcmp(method, "haveAInterface") == 0) {
        FunctionSpecificationMessage msg;
        msg.set_name("haveAInterface");
        if (!args) {
            LOG(WARNING) << "no argument passed";
        } else {
            switch (event) {
                case details::HidlInstrumentor::CLIENT_API_ENTRY:
                case details::HidlInstrumentor::SERVER_API_ENTRY:
                case details::HidlInstrumentor::PASSTHROUGH_ENTRY:
                {
                    if ((*args).size() != 1) {
                        LOG(ERROR) << "Number of arguments does not match. expect: 1, actual: " << (*args).size() << ", method name: haveAInterface, event type: " << event;
                        break;
                    }
                    auto *arg_0 __attribute__((__unused__)) = msg.add_arg();
                    sp<::android::hardware::tests::foo::V1_0::ISimple> *arg_val_0 __attribute__((__unused__)) = reinterpret_cast<sp<::android::hardware::tests::foo::V1_0::ISimple>*> ((*args)[0]);
                    if (arg_val_0 != nullptr) {
                        arg_0->set_type(TYPE_HIDL_INTERFACE);
                        arg_0->set_predefined_type("::android::hardware::tests::foo::V1_0::ISimple");
                    } else {
                        LOG(WARNING) << "argument 0 is null.";
                    }
                    break;
                }
                case details::HidlInstrumentor::CLIENT_API_EXIT:
                case details::HidlInstrumentor::SERVER_API_EXIT:
                case details::HidlInstrumentor::PASSTHROUGH_EXIT:
                {
                    if ((*args).size() != 1) {
                        LOG(ERROR) << "Number of return values does not match. expect: 1, actual: " << (*args).size() << ", method name: haveAInterface, event type: " << event;
                        break;
                    }
                    auto *result_0 __attribute__((__unused__)) = msg.add_return_type_hidl();
                    sp<::android::hardware::tests::foo::V1_0::ISimple> *result_val_0 __attribute__((__unused__)) = reinterpret_cast<sp<::android::hardware::tests::foo::V1_0::ISimple>*> ((*args)[0]);
                    if (result_val_0 != nullptr) {
                        result_0->set_type(TYPE_HIDL_INTERFACE);
                        result_0->set_predefined_type("::android::hardware::tests::foo::V1_0::ISimple");
                    } else {
                        LOG(WARNING) << "return value 0 is null.";
                    }
                    break;
                }
                default:
                {
                    LOG(WARNING) << "not supported. ";
                    break;
                }
            }
        }
        profiler.AddTraceEvent(event, package, version, interface, msg);
    }
}

}  // namespace vts
}  // namespace android
