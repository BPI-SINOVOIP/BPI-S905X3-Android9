#define LOG_TAG "hwservicemanager"

#include <android-base/logging.h>
#include <hidl-util/FQName.h>
#include <log/log.h>

#include "AccessControl.h"

namespace android {

static const char *kPermissionAdd = "add";
static const char *kPermissionGet = "find";
static const char *kPermissionList = "list";

struct audit_data {
    const char* interfaceName;
    pid_t       pid;
};

using android::FQName;
using Context = AccessControl::Context;

AccessControl::AccessControl() {
    mSeHandle = selinux_android_hw_service_context_handle();
    LOG_ALWAYS_FATAL_IF(mSeHandle == NULL, "Failed to acquire SELinux handle.");

    if (getcon(&mSeContext) != 0) {
        LOG_ALWAYS_FATAL("Failed to acquire hwservicemanager context.");
    }

    selinux_status_open(true);

    mSeCallbacks.func_audit = AccessControl::auditCallback;
    selinux_set_callback(SELINUX_CB_AUDIT, mSeCallbacks);

    mSeCallbacks.func_log = selinux_log_callback; /* defined in libselinux */
    selinux_set_callback(SELINUX_CB_LOG, mSeCallbacks);
}

bool AccessControl::canAdd(const std::string& fqName, const Context &context, pid_t pid) {
    FQName fqIface(fqName);

    if (!fqIface.isValid()) {
        return false;
    }
    const std::string checkName = fqIface.package() + "::" + fqIface.name();

    return checkPermission(context, pid, kPermissionAdd, checkName.c_str());
}

bool AccessControl::canGet(const std::string& fqName, pid_t pid) {
    FQName fqIface(fqName);

    if (!fqIface.isValid()) {
        return false;
    }
    const std::string checkName = fqIface.package() + "::" + fqIface.name();

    return checkPermission(getContext(pid), pid, kPermissionGet, checkName.c_str());
}

bool AccessControl::canList(pid_t pid) {
    return checkPermission(getContext(pid), pid, mSeContext, kPermissionList, nullptr);
}

Context AccessControl::getContext(pid_t sourcePid) {
    char *sourceContext = NULL;

    if (getpidcon(sourcePid, &sourceContext) < 0) {
        ALOGE("SELinux: failed to retrieve process context for pid %d", sourcePid);
        return Context(nullptr, freecon);
    }

    return Context(sourceContext, freecon);
}

bool AccessControl::checkPermission(const Context &context, pid_t sourceAuditPid, const char *targetContext, const char *perm, const char *interface) {
    if (context == nullptr) {
        return false;
    }

    bool allowed = false;
    struct audit_data ad;

    ad.pid = sourceAuditPid;
    ad.interfaceName = interface;

    allowed = (selinux_check_access(context.get(), targetContext, "hwservice_manager",
                                    perm, (void *) &ad) == 0);

    return allowed;
}

bool AccessControl::checkPermission(const Context &context, pid_t sourceAuditPid, const char *perm, const char *interface) {
    char *targetContext = NULL;
    bool allowed = false;

    // Lookup service in hwservice_contexts
    if (selabel_lookup(mSeHandle, &targetContext, interface, 0) != 0) {
        ALOGE("No match for interface %s in hwservice_contexts", interface);
        return false;
    }

    allowed = checkPermission(context, sourceAuditPid, targetContext, perm, interface);

    freecon(targetContext);

    return allowed;
}

int AccessControl::auditCallback(void *data, security_class_t /*cls*/, char *buf, size_t len) {
    struct audit_data *ad = (struct audit_data *)data;

    if (!ad || !ad->interfaceName) {
        ALOGE("No valid hwservicemanager audit data");
        return 0;
    }

    snprintf(buf, len, "interface=%s pid=%d", ad->interfaceName, ad->pid);
    return 0;
}

} // namespace android
