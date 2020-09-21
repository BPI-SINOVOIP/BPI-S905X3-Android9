#ifndef __VTS_PROFILER_android_hardware_nfc_V1_0_INfcClientCallback__
#define __VTS_PROFILER_android_hardware_nfc_V1_0_INfcClientCallback__


#include <android-base/logging.h>
#include <hidl/HidlSupport.h>
#include <linux/limits.h>
#include <test/vts/proto/ComponentSpecificationMessage.pb.h>
#include "VtsProfilingInterface.h"

#include <android/hardware/nfc/1.0/INfcClientCallback.h>
#include <android/hardware/nfc/1.0/types.h>
#include <android/hardware/nfc/1.0/types.vts.h>
#include <android/hidl/base/1.0/types.h>


using namespace android::hardware::nfc::V1_0;
using namespace android::hardware;

namespace android {
namespace vts {
extern "C" {

    void HIDL_INSTRUMENTATION_FUNCTION_android_hardware_nfc_V1_0_INfcClientCallback(
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
