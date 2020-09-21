#ifndef __VTS_PROFILER_android_hardware_tests_msgq_V1_0_ITestMsgQ__
#define __VTS_PROFILER_android_hardware_tests_msgq_V1_0_ITestMsgQ__


#include <android-base/logging.h>
#include <hidl/HidlSupport.h>
#include <linux/limits.h>
#include <test/vts/proto/ComponentSpecificationMessage.pb.h>
#include "VtsProfilingInterface.h"

#include <android/hardware/tests/msgq/1.0/ITestMsgQ.h>
#include <android/hidl/base/1.0/types.h>


using namespace android::hardware::tests::msgq::V1_0;
using namespace android::hardware;

namespace android {
namespace vts {
void profile____android__hardware__tests__msgq__V1_0__ITestMsgQ__EventFlagBits(VariableSpecificationMessage* arg_name,
::android::hardware::tests::msgq::V1_0::ITestMsgQ::EventFlagBits arg_val_name);
extern "C" {

    void HIDL_INSTRUMENTATION_FUNCTION_android_hardware_tests_msgq_V1_0_ITestMsgQ(
            details::HidlInstrumentor::InstrumentationEvent event,
            const char* package,
            const char* version,
            const char* interface,
            const char* method,
            std::vector<void *> *args);
}

}  // namespace vts
}  // namespace android
#endif
