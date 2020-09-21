#include "android/hardware/nfc/1.0/Nfc.vts.h"
#include <cutils/ashmem.h>
#include <fcntl.h>
#include <fmq/MessageQueue.h>
#include <sys/stat.h>

using namespace android::hardware::nfc::V1_0;
using namespace android::hardware;

#define TRACEFILEPREFIX "/data/local/tmp/"

namespace android {
namespace vts {

void HIDL_INSTRUMENTATION_FUNCTION_android_hardware_nfc_V1_0_INfc(
        details::HidlInstrumentor::InstrumentationEvent event __attribute__((__unused__)),
        const char* package,
        const char* version,
        const char* interface,
        const char* method __attribute__((__unused__)),
        std::vector<void *> *args __attribute__((__unused__))) {
    if (strcmp(package, "android.hardware.nfc") != 0) {
        LOG(WARNING) << "incorrect package. Expect: android.hardware.nfc actual: " << package;
        return;
    }
    std::string version_str = std::string(version);
    int major_version = stoi(version_str.substr(0, version_str.find('.')));
    int minor_version = stoi(version_str.substr(version_str.find('.') + 1));
    if (major_version != 1 || minor_version > 0) {
        LOG(WARNING) << "incorrect version. Expect: 1.0 or lower (if version != x.0), actual: " << version;
        return;
    }
    if (strcmp(interface, "INfc") != 0) {
        LOG(WARNING) << "incorrect interface. Expect: INfc actual: " << interface;
        return;
    }

    VtsProfilingInterface& profiler = VtsProfilingInterface::getInstance(TRACEFILEPREFIX);

    if (strcmp(method, "open") == 0) {
        FunctionSpecificationMessage msg;
        msg.set_name("open");
        if (!args) {
            LOG(WARNING) << "no argument passed";
        } else {
            switch (event) {
                case details::HidlInstrumentor::CLIENT_API_ENTRY:
                case details::HidlInstrumentor::SERVER_API_ENTRY:
                case details::HidlInstrumentor::PASSTHROUGH_ENTRY:
                {
                    if ((*args).size() != 1) {
                        LOG(ERROR) << "Number of arguments does not match. expect: 1, actual: " << (*args).size() << ", method name: open, event type: " << event;
                        break;
                    }
                    auto *arg_0 __attribute__((__unused__)) = msg.add_arg();
                    sp<::android::hardware::nfc::V1_0::INfcClientCallback> *arg_val_0 __attribute__((__unused__)) = reinterpret_cast<sp<::android::hardware::nfc::V1_0::INfcClientCallback>*> ((*args)[0]);
                    if (arg_val_0 != nullptr) {
                        arg_0->set_type(TYPE_HIDL_CALLBACK);
                        arg_0->set_predefined_type("::android::hardware::nfc::V1_0::INfcClientCallback");
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
                        LOG(ERROR) << "Number of return values does not match. expect: 1, actual: " << (*args).size() << ", method name: open, event type: " << event;
                        break;
                    }
                    auto *result_0 __attribute__((__unused__)) = msg.add_return_type_hidl();
                    ::android::hardware::nfc::V1_0::NfcStatus *result_val_0 __attribute__((__unused__)) = reinterpret_cast<::android::hardware::nfc::V1_0::NfcStatus*> ((*args)[0]);
                    if (result_val_0 != nullptr) {
                        result_0->set_type(TYPE_ENUM);
                        profile____android__hardware__nfc__V1_0__NfcStatus(result_0, (*result_val_0));
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
    if (strcmp(method, "write") == 0) {
        FunctionSpecificationMessage msg;
        msg.set_name("write");
        if (!args) {
            LOG(WARNING) << "no argument passed";
        } else {
            switch (event) {
                case details::HidlInstrumentor::CLIENT_API_ENTRY:
                case details::HidlInstrumentor::SERVER_API_ENTRY:
                case details::HidlInstrumentor::PASSTHROUGH_ENTRY:
                {
                    if ((*args).size() != 1) {
                        LOG(ERROR) << "Number of arguments does not match. expect: 1, actual: " << (*args).size() << ", method name: write, event type: " << event;
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
                        LOG(ERROR) << "Number of return values does not match. expect: 1, actual: " << (*args).size() << ", method name: write, event type: " << event;
                        break;
                    }
                    auto *result_0 __attribute__((__unused__)) = msg.add_return_type_hidl();
                    uint32_t *result_val_0 __attribute__((__unused__)) = reinterpret_cast<uint32_t*> ((*args)[0]);
                    if (result_val_0 != nullptr) {
                        result_0->set_type(TYPE_SCALAR);
                        result_0->mutable_scalar_value()->set_uint32_t((*result_val_0));
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
    if (strcmp(method, "coreInitialized") == 0) {
        FunctionSpecificationMessage msg;
        msg.set_name("coreInitialized");
        if (!args) {
            LOG(WARNING) << "no argument passed";
        } else {
            switch (event) {
                case details::HidlInstrumentor::CLIENT_API_ENTRY:
                case details::HidlInstrumentor::SERVER_API_ENTRY:
                case details::HidlInstrumentor::PASSTHROUGH_ENTRY:
                {
                    if ((*args).size() != 1) {
                        LOG(ERROR) << "Number of arguments does not match. expect: 1, actual: " << (*args).size() << ", method name: coreInitialized, event type: " << event;
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
                        LOG(ERROR) << "Number of return values does not match. expect: 1, actual: " << (*args).size() << ", method name: coreInitialized, event type: " << event;
                        break;
                    }
                    auto *result_0 __attribute__((__unused__)) = msg.add_return_type_hidl();
                    ::android::hardware::nfc::V1_0::NfcStatus *result_val_0 __attribute__((__unused__)) = reinterpret_cast<::android::hardware::nfc::V1_0::NfcStatus*> ((*args)[0]);
                    if (result_val_0 != nullptr) {
                        result_0->set_type(TYPE_ENUM);
                        profile____android__hardware__nfc__V1_0__NfcStatus(result_0, (*result_val_0));
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
    if (strcmp(method, "prediscover") == 0) {
        FunctionSpecificationMessage msg;
        msg.set_name("prediscover");
        if (!args) {
            LOG(WARNING) << "no argument passed";
        } else {
            switch (event) {
                case details::HidlInstrumentor::CLIENT_API_ENTRY:
                case details::HidlInstrumentor::SERVER_API_ENTRY:
                case details::HidlInstrumentor::PASSTHROUGH_ENTRY:
                {
                    if ((*args).size() != 0) {
                        LOG(ERROR) << "Number of arguments does not match. expect: 0, actual: " << (*args).size() << ", method name: prediscover, event type: " << event;
                        break;
                    }
                    break;
                }
                case details::HidlInstrumentor::CLIENT_API_EXIT:
                case details::HidlInstrumentor::SERVER_API_EXIT:
                case details::HidlInstrumentor::PASSTHROUGH_EXIT:
                {
                    if ((*args).size() != 1) {
                        LOG(ERROR) << "Number of return values does not match. expect: 1, actual: " << (*args).size() << ", method name: prediscover, event type: " << event;
                        break;
                    }
                    auto *result_0 __attribute__((__unused__)) = msg.add_return_type_hidl();
                    ::android::hardware::nfc::V1_0::NfcStatus *result_val_0 __attribute__((__unused__)) = reinterpret_cast<::android::hardware::nfc::V1_0::NfcStatus*> ((*args)[0]);
                    if (result_val_0 != nullptr) {
                        result_0->set_type(TYPE_ENUM);
                        profile____android__hardware__nfc__V1_0__NfcStatus(result_0, (*result_val_0));
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
    if (strcmp(method, "close") == 0) {
        FunctionSpecificationMessage msg;
        msg.set_name("close");
        if (!args) {
            LOG(WARNING) << "no argument passed";
        } else {
            switch (event) {
                case details::HidlInstrumentor::CLIENT_API_ENTRY:
                case details::HidlInstrumentor::SERVER_API_ENTRY:
                case details::HidlInstrumentor::PASSTHROUGH_ENTRY:
                {
                    if ((*args).size() != 0) {
                        LOG(ERROR) << "Number of arguments does not match. expect: 0, actual: " << (*args).size() << ", method name: close, event type: " << event;
                        break;
                    }
                    break;
                }
                case details::HidlInstrumentor::CLIENT_API_EXIT:
                case details::HidlInstrumentor::SERVER_API_EXIT:
                case details::HidlInstrumentor::PASSTHROUGH_EXIT:
                {
                    if ((*args).size() != 1) {
                        LOG(ERROR) << "Number of return values does not match. expect: 1, actual: " << (*args).size() << ", method name: close, event type: " << event;
                        break;
                    }
                    auto *result_0 __attribute__((__unused__)) = msg.add_return_type_hidl();
                    ::android::hardware::nfc::V1_0::NfcStatus *result_val_0 __attribute__((__unused__)) = reinterpret_cast<::android::hardware::nfc::V1_0::NfcStatus*> ((*args)[0]);
                    if (result_val_0 != nullptr) {
                        result_0->set_type(TYPE_ENUM);
                        profile____android__hardware__nfc__V1_0__NfcStatus(result_0, (*result_val_0));
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
    if (strcmp(method, "controlGranted") == 0) {
        FunctionSpecificationMessage msg;
        msg.set_name("controlGranted");
        if (!args) {
            LOG(WARNING) << "no argument passed";
        } else {
            switch (event) {
                case details::HidlInstrumentor::CLIENT_API_ENTRY:
                case details::HidlInstrumentor::SERVER_API_ENTRY:
                case details::HidlInstrumentor::PASSTHROUGH_ENTRY:
                {
                    if ((*args).size() != 0) {
                        LOG(ERROR) << "Number of arguments does not match. expect: 0, actual: " << (*args).size() << ", method name: controlGranted, event type: " << event;
                        break;
                    }
                    break;
                }
                case details::HidlInstrumentor::CLIENT_API_EXIT:
                case details::HidlInstrumentor::SERVER_API_EXIT:
                case details::HidlInstrumentor::PASSTHROUGH_EXIT:
                {
                    if ((*args).size() != 1) {
                        LOG(ERROR) << "Number of return values does not match. expect: 1, actual: " << (*args).size() << ", method name: controlGranted, event type: " << event;
                        break;
                    }
                    auto *result_0 __attribute__((__unused__)) = msg.add_return_type_hidl();
                    ::android::hardware::nfc::V1_0::NfcStatus *result_val_0 __attribute__((__unused__)) = reinterpret_cast<::android::hardware::nfc::V1_0::NfcStatus*> ((*args)[0]);
                    if (result_val_0 != nullptr) {
                        result_0->set_type(TYPE_ENUM);
                        profile____android__hardware__nfc__V1_0__NfcStatus(result_0, (*result_val_0));
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
    if (strcmp(method, "powerCycle") == 0) {
        FunctionSpecificationMessage msg;
        msg.set_name("powerCycle");
        if (!args) {
            LOG(WARNING) << "no argument passed";
        } else {
            switch (event) {
                case details::HidlInstrumentor::CLIENT_API_ENTRY:
                case details::HidlInstrumentor::SERVER_API_ENTRY:
                case details::HidlInstrumentor::PASSTHROUGH_ENTRY:
                {
                    if ((*args).size() != 0) {
                        LOG(ERROR) << "Number of arguments does not match. expect: 0, actual: " << (*args).size() << ", method name: powerCycle, event type: " << event;
                        break;
                    }
                    break;
                }
                case details::HidlInstrumentor::CLIENT_API_EXIT:
                case details::HidlInstrumentor::SERVER_API_EXIT:
                case details::HidlInstrumentor::PASSTHROUGH_EXIT:
                {
                    if ((*args).size() != 1) {
                        LOG(ERROR) << "Number of return values does not match. expect: 1, actual: " << (*args).size() << ", method name: powerCycle, event type: " << event;
                        break;
                    }
                    auto *result_0 __attribute__((__unused__)) = msg.add_return_type_hidl();
                    ::android::hardware::nfc::V1_0::NfcStatus *result_val_0 __attribute__((__unused__)) = reinterpret_cast<::android::hardware::nfc::V1_0::NfcStatus*> ((*args)[0]);
                    if (result_val_0 != nullptr) {
                        result_0->set_type(TYPE_ENUM);
                        profile____android__hardware__nfc__V1_0__NfcStatus(result_0, (*result_val_0));
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
