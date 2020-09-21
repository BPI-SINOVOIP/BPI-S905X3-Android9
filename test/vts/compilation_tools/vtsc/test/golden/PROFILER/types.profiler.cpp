#include "android/hardware/nfc/1.0/types.vts.h"
#include <cutils/ashmem.h>
#include <fcntl.h>
#include <fmq/MessageQueue.h>
#include <sys/stat.h>

using namespace android::hardware::nfc::V1_0;
using namespace android::hardware;

#define TRACEFILEPREFIX "/data/local/tmp/"

namespace android {
namespace vts {
void profile____android__hardware__nfc__V1_0__NfcEvent(VariableSpecificationMessage* arg_name,
::android::hardware::nfc::V1_0::NfcEvent arg_val_name __attribute__((__unused__))) {
    arg_name->set_type(TYPE_ENUM);
    arg_name->mutable_scalar_value()->set_uint32_t(static_cast<uint32_t>(arg_val_name));
    arg_name->set_scalar_type("uint32_t");
}

void profile____android__hardware__nfc__V1_0__NfcStatus(VariableSpecificationMessage* arg_name,
::android::hardware::nfc::V1_0::NfcStatus arg_val_name __attribute__((__unused__))) {
    arg_name->set_type(TYPE_ENUM);
    arg_name->mutable_scalar_value()->set_uint32_t(static_cast<uint32_t>(arg_val_name));
    arg_name->set_scalar_type("uint32_t");
}

}  // namespace vts
}  // namespace android
