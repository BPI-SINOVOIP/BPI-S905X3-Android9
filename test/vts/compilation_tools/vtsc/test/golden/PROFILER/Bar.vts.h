#ifndef __VTS_PROFILER_android_hardware_tests_bar_V1_0_IBar__
#define __VTS_PROFILER_android_hardware_tests_bar_V1_0_IBar__


#include <android-base/logging.h>
#include <hidl/HidlSupport.h>
#include <linux/limits.h>
#include <test/vts/proto/ComponentSpecificationMessage.pb.h>
#include "VtsProfilingInterface.h"

#include <android/hardware/tests/bar/1.0/IBar.h>
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
using namespace android::hardware;

namespace android {
namespace vts {
void profile____android__hardware__tests__bar__V1_0__IBar__SomethingRelated(VariableSpecificationMessage* arg_name,
::android::hardware::tests::bar::V1_0::IBar::SomethingRelated arg_val_name);
extern "C" {

    void HIDL_INSTRUMENTATION_FUNCTION_android_hardware_tests_bar_V1_0_IBar(
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
