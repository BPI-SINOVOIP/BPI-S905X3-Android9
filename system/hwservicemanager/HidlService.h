#ifndef ANDROID_HARDWARE_MANAGER_HIDLSERVICE_H
#define ANDROID_HARDWARE_MANAGER_HIDLSERVICE_H

#include <set>

#include <android/hidl/manager/1.1/IServiceManager.h>
#include <hidl/Status.h>
#include <hidl/MQDescriptor.h>

namespace android {
namespace hidl {
namespace manager {
namespace implementation {

using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hidl::base::V1_0::IBase;
using ::android::hidl::manager::V1_0::IServiceNotification;
using ::android::hidl::manager::V1_1::IServiceManager;
using ::android::sp;

struct HidlService {
    HidlService(const std::string &interfaceName,
                const std::string &instanceName,
                const sp<IBase> &service,
                const pid_t pid);
    HidlService(const std::string &interfaceName,
                const std::string &instanceName)
    : HidlService(
        interfaceName,
        instanceName,
        nullptr,
        static_cast<pid_t>(IServiceManager::PidConstant::NO_PID))
    {}

    /**
     * Note, getService() can be nullptr. This is because you can have a HidlService
     * with registered IServiceNotification objects but no service registered yet.
     */
    sp<IBase> getService() const;
    void setService(sp<IBase> service, pid_t pid);
    pid_t getPid() const;
    const std::string &getInterfaceName() const;
    const std::string &getInstanceName() const;

    void addListener(const sp<IServiceNotification> &listener);
    bool removeListener(const wp<IBase> &listener);
    void registerPassthroughClient(pid_t pid);

    std::string string() const; // e.x. "android.hidl.manager@1.0::IServiceManager/manager"
    const std::set<pid_t> &getPassthroughClients() const;

private:
    void sendRegistrationNotifications();

    const std::string                     mInterfaceName; // e.x. "android.hidl.manager@1.0::IServiceManager"
    const std::string                     mInstanceName;  // e.x. "manager"
    sp<IBase>                             mService;

    std::vector<sp<IServiceNotification>> mListeners{};
    std::set<pid_t>                       mPassthroughClients{};
    pid_t                                 mPid = static_cast<pid_t>(IServiceManager::PidConstant::NO_PID);
};

}  // namespace implementation
}  // namespace manager
}  // namespace hidl
}  // namespace android

#endif // ANDROID_HARDWARE_MANAGER_HIDLSERVICE_H
