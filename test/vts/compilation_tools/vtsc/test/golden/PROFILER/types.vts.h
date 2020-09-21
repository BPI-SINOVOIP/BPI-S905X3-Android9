#ifndef __VTS_PROFILER_android_hardware_nfc_V1_0_types__
#define __VTS_PROFILER_android_hardware_nfc_V1_0_types__


#include <android-base/logging.h>
#include <hidl/HidlSupport.h>
#include <linux/limits.h>
#include <test/vts/proto/ComponentSpecificationMessage.pb.h>
#include "VtsProfilingInterface.h"

#include <android/hardware/nfc/1.0/types.h>


using namespace android::hardware::nfc::V1_0;
using namespace android::hardware;

namespace android {
namespace vts {
void profile____android__hardware__nfc__V1_0__NfcEvent(VariableSpecificationMessage* arg_name,
::android::hardware::nfc::V1_0::NfcEvent arg_val_name);
void profile____android__hardware__nfc__V1_0__NfcStatus(VariableSpecificationMessage* arg_name,
::android::hardware::nfc::V1_0::NfcStatus arg_val_name);
}  // namespace vts
}  // namespace android
#endif
