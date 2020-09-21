#include <string>

#include <selinux/android.h>
#include <selinux/avc.h>

namespace android {

class AccessControl {
public:
    AccessControl();

    using Context = std::unique_ptr<char, decltype(&freecon)>;
    Context getContext(pid_t sourcePid);

    bool canAdd(const std::string& fqName, const Context &context, pid_t pid);
    bool canGet(const std::string& fqName, pid_t pid);
    bool canList(pid_t pid);

private:

    bool checkPermission(const Context &context, pid_t sourceAuditPid, const char *targetContext, const char *perm, const char *interface);
    bool checkPermission(const Context &context, pid_t sourcePid, const char *perm, const char *interface);

    static int auditCallback(void *data, security_class_t cls, char *buf, size_t len);

    char*                  mSeContext;
    struct selabel_handle* mSeHandle;
    union selinux_callback mSeCallbacks;
};

} // namespace android
