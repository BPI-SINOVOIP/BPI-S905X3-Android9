#include "android/hardware/tests/memory/1.0/MemoryTest.vts.h"
#include <cutils/ashmem.h>
#include <fcntl.h>
#include <fmq/MessageQueue.h>
#include <sys/stat.h>

using namespace android::hardware::tests::memory::V1_0;
using namespace android::hardware;

#define TRACEFILEPREFIX "/data/local/tmp/"

namespace android {
namespace vts {

void HIDL_INSTRUMENTATION_FUNCTION_android_hardware_tests_memory_V1_0_IMemoryTest(
        details::HidlInstrumentor::InstrumentationEvent event __attribute__((__unused__)),
        const char* package,
        const char* version,
        const char* interface,
        const char* method __attribute__((__unused__)),
        std::vector<void *> *args __attribute__((__unused__))) {
    if (strcmp(package, "android.hardware.tests.memory") != 0) {
        LOG(WARNING) << "incorrect package. Expect: android.hardware.tests.memory actual: " << package;
        return;
    }
    std::string version_str = std::string(version);
    int major_version = stoi(version_str.substr(0, version_str.find('.')));
    int minor_version = stoi(version_str.substr(version_str.find('.') + 1));
    if (major_version != 1 || minor_version > 0) {
        LOG(WARNING) << "incorrect version. Expect: 1.0 or lower (if version != x.0), actual: " << version;
        return;
    }
    if (strcmp(interface, "IMemoryTest") != 0) {
        LOG(WARNING) << "incorrect interface. Expect: IMemoryTest actual: " << interface;
        return;
    }

    VtsProfilingInterface& profiler = VtsProfilingInterface::getInstance(TRACEFILEPREFIX);

    if (strcmp(method, "haveSomeMemory") == 0) {
        FunctionSpecificationMessage msg;
        msg.set_name("haveSomeMemory");
        if (!args) {
            LOG(WARNING) << "no argument passed";
        } else {
            switch (event) {
                case details::HidlInstrumentor::CLIENT_API_ENTRY:
                case details::HidlInstrumentor::SERVER_API_ENTRY:
                case details::HidlInstrumentor::PASSTHROUGH_ENTRY:
                {
                    if ((*args).size() != 1) {
                        LOG(ERROR) << "Number of arguments does not match. expect: 1, actual: " << (*args).size() << ", method name: haveSomeMemory, event type: " << event;
                        break;
                    }
                    auto *arg_0 __attribute__((__unused__)) = msg.add_arg();
                    ::android::hardware::hidl_memory *arg_val_0 __attribute__((__unused__)) = reinterpret_cast<::android::hardware::hidl_memory*> ((*args)[0]);
                    if (arg_val_0 != nullptr) {
                        arg_0->set_type(TYPE_HIDL_MEMORY);
                        arg_0->mutable_hidl_memory_value()->set_size((*arg_val_0).size());
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
                        LOG(ERROR) << "Number of return values does not match. expect: 1, actual: " << (*args).size() << ", method name: haveSomeMemory, event type: " << event;
                        break;
                    }
                    auto *result_0 __attribute__((__unused__)) = msg.add_return_type_hidl();
                    ::android::hardware::hidl_memory *result_val_0 __attribute__((__unused__)) = reinterpret_cast<::android::hardware::hidl_memory*> ((*args)[0]);
                    if (result_val_0 != nullptr) {
                        result_0->set_type(TYPE_HIDL_MEMORY);
                        result_0->mutable_hidl_memory_value()->set_size((*result_val_0).size());
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
    if (strcmp(method, "fillMemory") == 0) {
        FunctionSpecificationMessage msg;
        msg.set_name("fillMemory");
        if (!args) {
            LOG(WARNING) << "no argument passed";
        } else {
            switch (event) {
                case details::HidlInstrumentor::CLIENT_API_ENTRY:
                case details::HidlInstrumentor::SERVER_API_ENTRY:
                case details::HidlInstrumentor::PASSTHROUGH_ENTRY:
                {
                    if ((*args).size() != 2) {
                        LOG(ERROR) << "Number of arguments does not match. expect: 2, actual: " << (*args).size() << ", method name: fillMemory, event type: " << event;
                        break;
                    }
                    auto *arg_0 __attribute__((__unused__)) = msg.add_arg();
                    ::android::hardware::hidl_memory *arg_val_0 __attribute__((__unused__)) = reinterpret_cast<::android::hardware::hidl_memory*> ((*args)[0]);
                    if (arg_val_0 != nullptr) {
                        arg_0->set_type(TYPE_HIDL_MEMORY);
                        arg_0->mutable_hidl_memory_value()->set_size((*arg_val_0).size());
                    } else {
                        LOG(WARNING) << "argument 0 is null.";
                    }
                    auto *arg_1 __attribute__((__unused__)) = msg.add_arg();
                    uint8_t *arg_val_1 __attribute__((__unused__)) = reinterpret_cast<uint8_t*> ((*args)[1]);
                    if (arg_val_1 != nullptr) {
                        arg_1->set_type(TYPE_SCALAR);
                        arg_1->mutable_scalar_value()->set_uint8_t((*arg_val_1));
                    } else {
                        LOG(WARNING) << "argument 1 is null.";
                    }
                    break;
                }
                case details::HidlInstrumentor::CLIENT_API_EXIT:
                case details::HidlInstrumentor::SERVER_API_EXIT:
                case details::HidlInstrumentor::PASSTHROUGH_EXIT:
                {
                    if ((*args).size() != 0) {
                        LOG(ERROR) << "Number of return values does not match. expect: 0, actual: " << (*args).size() << ", method name: fillMemory, event type: " << event;
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
    if (strcmp(method, "haveSomeMemoryBlock") == 0) {
        FunctionSpecificationMessage msg;
        msg.set_name("haveSomeMemoryBlock");
        if (!args) {
            LOG(WARNING) << "no argument passed";
        } else {
            switch (event) {
                case details::HidlInstrumentor::CLIENT_API_ENTRY:
                case details::HidlInstrumentor::SERVER_API_ENTRY:
                case details::HidlInstrumentor::PASSTHROUGH_ENTRY:
                {
                    if ((*args).size() != 1) {
                        LOG(ERROR) << "Number of arguments does not match. expect: 1, actual: " << (*args).size() << ", method name: haveSomeMemoryBlock, event type: " << event;
                        break;
                    }
                    auto *arg_0 __attribute__((__unused__)) = msg.add_arg();
                    ::android::hidl::memory::block::V1_0::MemoryBlock *arg_val_0 __attribute__((__unused__)) = reinterpret_cast<::android::hidl::memory::block::V1_0::MemoryBlock*> ((*args)[0]);
                    if (arg_val_0 != nullptr) {
                        arg_0->set_type(TYPE_STRUCT);
                        profile____android__hidl__memory__block__V1_0__MemoryBlock(arg_0, (*arg_val_0));
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
                        LOG(ERROR) << "Number of return values does not match. expect: 1, actual: " << (*args).size() << ", method name: haveSomeMemoryBlock, event type: " << event;
                        break;
                    }
                    auto *result_0 __attribute__((__unused__)) = msg.add_return_type_hidl();
                    ::android::hidl::memory::block::V1_0::MemoryBlock *result_val_0 __attribute__((__unused__)) = reinterpret_cast<::android::hidl::memory::block::V1_0::MemoryBlock*> ((*args)[0]);
                    if (result_val_0 != nullptr) {
                        result_0->set_type(TYPE_STRUCT);
                        profile____android__hidl__memory__block__V1_0__MemoryBlock(result_0, (*result_val_0));
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
    if (strcmp(method, "set") == 0) {
        FunctionSpecificationMessage msg;
        msg.set_name("set");
        if (!args) {
            LOG(WARNING) << "no argument passed";
        } else {
            switch (event) {
                case details::HidlInstrumentor::CLIENT_API_ENTRY:
                case details::HidlInstrumentor::SERVER_API_ENTRY:
                case details::HidlInstrumentor::PASSTHROUGH_ENTRY:
                {
                    if ((*args).size() != 1) {
                        LOG(ERROR) << "Number of arguments does not match. expect: 1, actual: " << (*args).size() << ", method name: set, event type: " << event;
                        break;
                    }
                    auto *arg_0 __attribute__((__unused__)) = msg.add_arg();
                    ::android::hardware::hidl_memory *arg_val_0 __attribute__((__unused__)) = reinterpret_cast<::android::hardware::hidl_memory*> ((*args)[0]);
                    if (arg_val_0 != nullptr) {
                        arg_0->set_type(TYPE_HIDL_MEMORY);
                        arg_0->mutable_hidl_memory_value()->set_size((*arg_val_0).size());
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
                        LOG(ERROR) << "Number of return values does not match. expect: 0, actual: " << (*args).size() << ", method name: set, event type: " << event;
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
    if (strcmp(method, "get") == 0) {
        FunctionSpecificationMessage msg;
        msg.set_name("get");
        if (!args) {
            LOG(WARNING) << "no argument passed";
        } else {
            switch (event) {
                case details::HidlInstrumentor::CLIENT_API_ENTRY:
                case details::HidlInstrumentor::SERVER_API_ENTRY:
                case details::HidlInstrumentor::PASSTHROUGH_ENTRY:
                {
                    if ((*args).size() != 0) {
                        LOG(ERROR) << "Number of arguments does not match. expect: 0, actual: " << (*args).size() << ", method name: get, event type: " << event;
                        break;
                    }
                    break;
                }
                case details::HidlInstrumentor::CLIENT_API_EXIT:
                case details::HidlInstrumentor::SERVER_API_EXIT:
                case details::HidlInstrumentor::PASSTHROUGH_EXIT:
                {
                    if ((*args).size() != 1) {
                        LOG(ERROR) << "Number of return values does not match. expect: 1, actual: " << (*args).size() << ", method name: get, event type: " << event;
                        break;
                    }
                    auto *result_0 __attribute__((__unused__)) = msg.add_return_type_hidl();
                    sp<::android::hidl::memory::token::V1_0::IMemoryToken> *result_val_0 __attribute__((__unused__)) = reinterpret_cast<sp<::android::hidl::memory::token::V1_0::IMemoryToken>*> ((*args)[0]);
                    if (result_val_0 != nullptr) {
                        result_0->set_type(TYPE_HIDL_INTERFACE);
                        result_0->set_predefined_type("::android::hidl::memory::token::V1_0::IMemoryToken");
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
