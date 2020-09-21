#ifndef __VTS_PROFILER_android_hardware_tests_memory_V1_0_IMemoryTest__
#define __VTS_PROFILER_android_hardware_tests_memory_V1_0_IMemoryTest__


#include <android-base/logging.h>
#include <hidl/HidlSupport.h>
#include <linux/limits.h>
#include <test/vts/proto/ComponentSpecificationMessage.pb.h>
#include "VtsProfilingInterface.h"

#include <android/hardware/tests/memory/1.0/IMemoryTest.h>
#include <android/hidl/base/1.0/types.h>
#include <android/hidl/memory/block/1.0/types.h>
#include <android/hidl/memory/block/1.0/types.vts.h>
#include <android/hidl/memory/token/1.0/IMemoryToken.h>
#include <android/hidl/memory/token/1.0/MemoryToken.vts.h>


using namespace android::hardware::tests::memory::V1_0;
using namespace android::hardware;

namespace android {
namespace vts {
extern "C" {

    void HIDL_INSTRUMENTATION_FUNCTION_android_hardware_tests_memory_V1_0_IMemoryTest(
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
