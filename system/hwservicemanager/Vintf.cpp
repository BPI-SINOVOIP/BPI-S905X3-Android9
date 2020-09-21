#define LOG_TAG "hwservicemanager"
//#define LOG_NDEBUG 0

#include "Vintf.h"

#include <android-base/logging.h>
#include <hidl-util/FQName.h>
#include <vintf/parse_string.h>
#include <vintf/VintfObject.h>

namespace android {
namespace hardware {

vintf::Transport getTransportFromManifest(
        const FQName &fqName, const std::string &instanceName,
        const std::shared_ptr<const vintf::HalManifest>& vm) {
    if (vm == nullptr) {
        return vintf::Transport::EMPTY;
    }
    return vm->getTransport(fqName.package(),
            vintf::Version{fqName.getPackageMajorVersion(), fqName.getPackageMinorVersion()},
            fqName.name(), instanceName);
}

vintf::Transport getTransport(const std::string &interfaceName, const std::string &instanceName) {
    FQName fqName(interfaceName);
    if (!fqName.isValid()) {
        LOG(ERROR) << __FUNCTION__ << ": " << interfaceName
                   << " is not a valid fully-qualified name ";
        return vintf::Transport::EMPTY;
    }
    if (!fqName.hasVersion()) {
        LOG(ERROR) << __FUNCTION__ << ": " << fqName.string()
                   << " does not specify a version.";
        return vintf::Transport::EMPTY;
    }
    if (fqName.name().empty()) {
        LOG(ERROR) << __FUNCTION__ << ": " << fqName.string()
                   << " does not specify an interface name.";
        return vintf::Transport::EMPTY;
    }

    vintf::Transport tr = getTransportFromManifest(fqName, instanceName,
            vintf::VintfObject::GetFrameworkHalManifest());
    if (tr != vintf::Transport::EMPTY) {
        return tr;
    }
    tr = getTransportFromManifest(fqName, instanceName,
            vintf::VintfObject::GetDeviceHalManifest());
    if (tr != vintf::Transport::EMPTY) {
        return tr;
    }

    LOG(WARNING) << __FUNCTION__ << ": Cannot find entry "
                 << fqName.string() << "/" << instanceName
                 << " in either framework or device manifest.";
    return vintf::Transport::EMPTY;
}

}  // hardware
}  // android
