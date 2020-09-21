#include "android/hardware/nfc/1.0/types.vts.h"
#include "vts_measurement.h"
#include <android-base/logging.h>
#include <android/hidl/allocator/1.0/IAllocator.h>
#include <fmq/MessageQueue.h>
#include <sys/stat.h>
#include <unistd.h>


using namespace android::hardware::nfc::V1_0;
namespace android {
namespace vts {
::android::hardware::nfc::V1_0::NfcEvent EnumValue__android__hardware__nfc__V1_0__NfcEvent(const ScalarDataValueMessage& arg) {
    return (::android::hardware::nfc::V1_0::NfcEvent) arg.uint32_t();
}
uint32_t Random__android__hardware__nfc__V1_0__NfcEvent() {
    uint32_t choice = (uint32_t) rand() / 7;
    if (choice == (uint32_t) 0UL) return static_cast<uint32_t>(::android::hardware::nfc::V1_0::NfcEvent::OPEN_CPLT);
    if (choice == (uint32_t) 1UL) return static_cast<uint32_t>(::android::hardware::nfc::V1_0::NfcEvent::CLOSE_CPLT);
    if (choice == (uint32_t) 2UL) return static_cast<uint32_t>(::android::hardware::nfc::V1_0::NfcEvent::POST_INIT_CPLT);
    if (choice == (uint32_t) 3UL) return static_cast<uint32_t>(::android::hardware::nfc::V1_0::NfcEvent::PRE_DISCOVER_CPLT);
    if (choice == (uint32_t) 4UL) return static_cast<uint32_t>(::android::hardware::nfc::V1_0::NfcEvent::REQUEST_CONTROL);
    if (choice == (uint32_t) 5UL) return static_cast<uint32_t>(::android::hardware::nfc::V1_0::NfcEvent::RELEASE_CONTROL);
    if (choice == (uint32_t) 6UL) return static_cast<uint32_t>(::android::hardware::nfc::V1_0::NfcEvent::ERROR);
    return static_cast<uint32_t>(::android::hardware::nfc::V1_0::NfcEvent::OPEN_CPLT);
}
bool Verify__android__hardware__nfc__V1_0__NfcEvent(const VariableSpecificationMessage& expected_result __attribute__((__unused__)), const VariableSpecificationMessage& actual_result __attribute__((__unused__))){
    if (actual_result.scalar_value().uint32_t() != expected_result.scalar_value().uint32_t()) { return false; }
    return true;
}

void SetResult__android__hardware__nfc__V1_0__NfcEvent(VariableSpecificationMessage* result_msg, ::android::hardware::nfc::V1_0::NfcEvent result_value __attribute__((__unused__))){
    result_msg->set_type(TYPE_ENUM);
    result_msg->set_scalar_type("uint32_t");
    result_msg->mutable_scalar_value()->set_uint32_t(static_cast<uint32_t>(result_value));
}

::android::hardware::nfc::V1_0::NfcStatus EnumValue__android__hardware__nfc__V1_0__NfcStatus(const ScalarDataValueMessage& arg) {
    return (::android::hardware::nfc::V1_0::NfcStatus) arg.uint32_t();
}
uint32_t Random__android__hardware__nfc__V1_0__NfcStatus() {
    uint32_t choice = (uint32_t) rand() / 5;
    if (choice == (uint32_t) 0UL) return static_cast<uint32_t>(::android::hardware::nfc::V1_0::NfcStatus::OK);
    if (choice == (uint32_t) 1UL) return static_cast<uint32_t>(::android::hardware::nfc::V1_0::NfcStatus::FAILED);
    if (choice == (uint32_t) 2UL) return static_cast<uint32_t>(::android::hardware::nfc::V1_0::NfcStatus::ERR_TRANSPORT);
    if (choice == (uint32_t) 3UL) return static_cast<uint32_t>(::android::hardware::nfc::V1_0::NfcStatus::ERR_CMD_TIMEOUT);
    if (choice == (uint32_t) 4UL) return static_cast<uint32_t>(::android::hardware::nfc::V1_0::NfcStatus::REFUSED);
    return static_cast<uint32_t>(::android::hardware::nfc::V1_0::NfcStatus::OK);
}
bool Verify__android__hardware__nfc__V1_0__NfcStatus(const VariableSpecificationMessage& expected_result __attribute__((__unused__)), const VariableSpecificationMessage& actual_result __attribute__((__unused__))){
    if (actual_result.scalar_value().uint32_t() != expected_result.scalar_value().uint32_t()) { return false; }
    return true;
}

void SetResult__android__hardware__nfc__V1_0__NfcStatus(VariableSpecificationMessage* result_msg, ::android::hardware::nfc::V1_0::NfcStatus result_value __attribute__((__unused__))){
    result_msg->set_type(TYPE_ENUM);
    result_msg->set_scalar_type("uint32_t");
    result_msg->mutable_scalar_value()->set_uint32_t(static_cast<uint32_t>(result_value));
}

}  // namespace vts
}  // namespace android
