#include "android_common.h"

// For 'system', 'vendor' (mandatory) and/or 'odm' (optional).
#define MAX_FILE_CONTEXT_SIZE 3

#ifdef __ANDROID_VNDK__
#ifndef LOG_EVENT_STRING
#define LOG_EVENT_STRING(...)
#endif  // LOG_EVENT_STRING
#endif  // __ANDROID_VNDK__

static const struct selinux_opt seopts_service_plat[] = {
    { SELABEL_OPT_PATH, "/system/etc/selinux/plat_service_contexts" },
    { SELABEL_OPT_PATH, "/plat_service_contexts" }
};

static const struct selinux_opt seopts_service_vendor[] = {
    { SELABEL_OPT_PATH, "/vendor/etc/selinux/vendor_service_contexts" },
    { SELABEL_OPT_PATH, "/vendor_service_contexts" },
    // TODO: remove nonplat* when no need to retain backward compatibility.
    { SELABEL_OPT_PATH, "/vendor/etc/selinux/nonplat_service_contexts" },
    { SELABEL_OPT_PATH, "/nonplat_service_contexts" }
};

static const struct selinux_opt seopts_hwservice_plat[] = {
    { SELABEL_OPT_PATH, "/system/etc/selinux/plat_hwservice_contexts" },
    { SELABEL_OPT_PATH, "/plat_hwservice_contexts" }
};
static const struct selinux_opt seopts_hwservice_vendor[] = {
    { SELABEL_OPT_PATH, "/vendor/etc/selinux/vendor_hwservice_contexts" },
    { SELABEL_OPT_PATH, "/vendor_hwservice_contexts" },
    // TODO: remove nonplat* when no need to retain backward compatibility.
    { SELABEL_OPT_PATH, "/vendor/etc/selinux/nonplat_hwservice_contexts" },
    { SELABEL_OPT_PATH, "/nonplat_hwservice_contexts" }
};
static const struct selinux_opt seopts_hwservice_odm[] = {
    { SELABEL_OPT_PATH, "/odm/etc/selinux/odm_hwservice_contexts" },
    { SELABEL_OPT_PATH, "/odm_hwservice_contexts" }
};

static const struct selinux_opt seopts_vndservice =
    { SELABEL_OPT_PATH, "/vendor/etc/selinux/vndservice_contexts" };

static const struct selinux_opt seopts_vndservice_rootfs =
    { SELABEL_OPT_PATH, "/vndservice_contexts" };

struct selabel_handle* selinux_android_service_open_context_handle(const struct selinux_opt* seopts_service,
                                                                   unsigned nopts)
{
    struct selabel_handle* sehandle;

    sehandle = selabel_open(SELABEL_CTX_ANDROID_SERVICE,
            seopts_service, nopts);

    if (!sehandle) {
        selinux_log(SELINUX_ERROR, "%s: Error getting service context handle (%s)\n",
                __FUNCTION__, strerror(errno));
        return NULL;
    }
    selinux_log(SELINUX_INFO, "SELinux: Loaded service_contexts from:\n");
    for (unsigned i = 0; i < nopts; i++) {
        selinux_log(SELINUX_INFO, "    %s\n", seopts_service[i].value);
    }
    return sehandle;
}

struct selabel_handle* selinux_android_service_context_handle(void)
{
    struct selinux_opt seopts_service[MAX_FILE_CONTEXT_SIZE];
    int size = 0;
    unsigned int i;
    for (i = 0; i < ARRAY_SIZE(seopts_service_plat); i++) {
        if (access(seopts_service_plat[i].value, R_OK) != -1) {
            seopts_service[size++] = seopts_service_plat[i];
            break;
        }
    }
    for (i = 0; i < ARRAY_SIZE(seopts_service_vendor); i++) {
        if (access(seopts_service_vendor[i].value, R_OK) != -1) {
            seopts_service[size++] = seopts_service_vendor[i];
            break;
        }
    }

    return selinux_android_service_open_context_handle(seopts_service, size);
}

struct selabel_handle* selinux_android_hw_service_context_handle(void)
{
    struct selinux_opt seopts_service[MAX_FILE_CONTEXT_SIZE];
    int size = 0;
    unsigned int i;
    for (i = 0; i < ARRAY_SIZE(seopts_hwservice_plat); i++) {
        if (access(seopts_hwservice_plat[i].value, R_OK) != -1) {
            seopts_service[size++] = seopts_hwservice_plat[i];
            break;
        }
    }
    for (i = 0; i < ARRAY_SIZE(seopts_hwservice_vendor); i++) {
        if (access(seopts_hwservice_vendor[i].value, R_OK) != -1) {
            seopts_service[size++] = seopts_hwservice_vendor[i];
            break;
        }
    }
    for (i = 0; i < ARRAY_SIZE(seopts_hwservice_odm); i++) {
        if (access(seopts_hwservice_odm[i].value, R_OK) != -1) {
            seopts_service[size++] = seopts_hwservice_odm[i];
            break;
        }
    }
    return selinux_android_service_open_context_handle(seopts_service, size);
}

struct selabel_handle* selinux_android_vendor_service_context_handle(void)
{
    const struct selinux_opt* seopts_service;
    if (access(seopts_vndservice.value, R_OK) != -1) {
        seopts_service = &seopts_vndservice;
    } else {
        seopts_service = &seopts_vndservice_rootfs;
    }

    return selinux_android_service_open_context_handle(seopts_service, 1);
}

int selinux_log_callback(int type, const char *fmt, ...)
{
    va_list ap;
    int priority;
    char *strp;

    switch(type) {
    case SELINUX_WARNING:
        priority = ANDROID_LOG_WARN;
        break;
    case SELINUX_INFO:
        priority = ANDROID_LOG_INFO;
        break;
    default:
        priority = ANDROID_LOG_ERROR;
        break;
    }

    va_start(ap, fmt);
    if (vasprintf(&strp, fmt, ap) != -1) {
        LOG_PRI(priority, "SELinux", "%s", strp);
        LOG_EVENT_STRING(AUDITD_LOG_TAG, strp);
        free(strp);
    }
    va_end(ap);
    return 0;
}
