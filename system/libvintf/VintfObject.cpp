/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "VintfObject.h"

#include "CompatibilityMatrix.h"
#include "parse_string.h"
#include "parse_xml.h"
#include "utils.h"

#include <dirent.h>

#include <functional>
#include <memory>
#include <mutex>

#include <android-base/logging.h>

using std::placeholders::_1;
using std::placeholders::_2;

namespace android {
namespace vintf {

using namespace details;

template <typename T>
struct LockedSharedPtr {
    std::shared_ptr<T> object;
    std::mutex mutex;
    bool fetchedOnce = false;
};

struct LockedRuntimeInfoCache {
    std::shared_ptr<RuntimeInfo> object;
    std::mutex mutex;
    RuntimeInfo::FetchFlags fetchedFlags = RuntimeInfo::FetchFlag::NONE;
};

template <typename T, typename F>
static std::shared_ptr<const T> Get(
        LockedSharedPtr<T> *ptr,
        bool skipCache,
        const F &fetchAllInformation) {
    std::unique_lock<std::mutex> _lock(ptr->mutex);
    if (skipCache || !ptr->fetchedOnce) {
        ptr->object = std::make_unique<T>();
        std::string error;
        if (fetchAllInformation(ptr->object.get(), &error) != OK) {
            LOG(WARNING) << error;
            ptr->object = nullptr; // frees the old object
        }
        ptr->fetchedOnce = true;
    }
    return ptr->object;
}

// static
std::shared_ptr<const HalManifest> VintfObject::GetDeviceHalManifest(bool skipCache) {
    static LockedSharedPtr<HalManifest> gVendorManifest;
    return Get(&gVendorManifest, skipCache, &VintfObject::FetchDeviceHalManifest);
}

// static
std::shared_ptr<const HalManifest> VintfObject::GetFrameworkHalManifest(bool skipCache) {
    static LockedSharedPtr<HalManifest> gFrameworkManifest;
    return Get(&gFrameworkManifest, skipCache, &VintfObject::FetchFrameworkHalManifest);
}


// static
std::shared_ptr<const CompatibilityMatrix> VintfObject::GetDeviceCompatibilityMatrix(bool skipCache) {
    static LockedSharedPtr<CompatibilityMatrix> gDeviceMatrix;
    return Get(&gDeviceMatrix, skipCache, &VintfObject::FetchDeviceMatrix);
}

// static
std::shared_ptr<const CompatibilityMatrix> VintfObject::GetFrameworkCompatibilityMatrix(bool skipCache) {
    static LockedSharedPtr<CompatibilityMatrix> gFrameworkMatrix;
    static LockedSharedPtr<CompatibilityMatrix> gCombinedFrameworkMatrix;
    static std::mutex gFrameworkCompatibilityMatrixMutex;

    // To avoid deadlock, get device manifest before any locks.
    auto deviceManifest = GetDeviceHalManifest();

    std::unique_lock<std::mutex> _lock(gFrameworkCompatibilityMatrixMutex);

    auto combined =
        Get(&gCombinedFrameworkMatrix, skipCache,
            std::bind(&VintfObject::GetCombinedFrameworkMatrix, deviceManifest, _1, _2));
    if (combined != nullptr) {
        return combined;
    }

    return Get(&gFrameworkMatrix, skipCache,
               std::bind(&CompatibilityMatrix::fetchAllInformation, _1, kSystemLegacyMatrix, _2));
}

status_t VintfObject::GetCombinedFrameworkMatrix(
    const std::shared_ptr<const HalManifest>& deviceManifest, CompatibilityMatrix* out,
    std::string* error) {
    auto matrixFragments = GetAllFrameworkMatrixLevels(error);
    if (matrixFragments.empty()) {
        return NAME_NOT_FOUND;
    }

    Level deviceLevel = Level::UNSPECIFIED;

    if (deviceManifest != nullptr) {
        deviceLevel = deviceManifest->level();
    }

    // TODO(b/70628538): Do not infer from Shipping API level.
    if (deviceLevel == Level::UNSPECIFIED) {
        auto shippingApi = getPropertyFetcher().getUintProperty("ro.product.first_api_level", 0u);
        if (shippingApi != 0u) {
            deviceLevel = details::convertFromApiLevel(shippingApi);
        }
    }

    if (deviceLevel == Level::UNSPECIFIED) {
        // Cannot infer FCM version. Combine all matrices by assuming
        // Shipping FCM Version == min(all supported FCM Versions in the framework)
        for (auto&& pair : matrixFragments) {
            Level fragmentLevel = pair.object.level();
            if (fragmentLevel != Level::UNSPECIFIED && deviceLevel > fragmentLevel) {
                deviceLevel = fragmentLevel;
            }
        }
    }

    if (deviceLevel == Level::UNSPECIFIED) {
        // None of the fragments specify any FCM version. Should never happen except
        // for inconsistent builds.
        if (error) {
            *error = "No framework compatibility matrix files under " + kSystemVintfDir +
                     " declare FCM version.";
        }
        return NAME_NOT_FOUND;
    }

    CompatibilityMatrix* combined =
        CompatibilityMatrix::combine(deviceLevel, &matrixFragments, error);
    if (combined == nullptr) {
        return BAD_VALUE;
    }
    *out = std::move(*combined);
    return OK;
}

// Priority for loading vendor manifest:
// 1. /vendor/etc/vintf/manifest.xml + ODM manifest
// 2. /vendor/etc/vintf/manifest.xml
// 3. ODM manifest
// 4. /vendor/manifest.xml
// where:
// A + B means adding <hal> tags from B to A (so that <hal>s from B can override A)
status_t VintfObject::FetchDeviceHalManifest(HalManifest* out, std::string* error) {
    status_t vendorStatus = FetchOneHalManifest(kVendorManifest, out, error);
    if (vendorStatus != OK && vendorStatus != NAME_NOT_FOUND) {
        return vendorStatus;
    }

    HalManifest odmManifest;
    status_t odmStatus = FetchOdmHalManifest(&odmManifest, error);
    if (odmStatus != OK && odmStatus != NAME_NOT_FOUND) {
        return odmStatus;
    }

    if (vendorStatus == OK) {
        if (odmStatus == OK) {
            out->addAllHals(&odmManifest);
        }
        return OK;
    }

    // vendorStatus != OK, "out" is not changed.
    if (odmStatus == OK) {
        *out = std::move(odmManifest);
        return OK;
    }

    // Use legacy /vendor/manifest.xml
    return out->fetchAllInformation(kVendorLegacyManifest, error);
}

// "out" is written to iff return status is OK.
// Priority:
// 1. if {sku} is defined, /odm/etc/vintf/manifest_{sku}.xml
// 2. /odm/etc/vintf/manifest.xml
// 3. if {sku} is defined, /odm/etc/manifest_{sku}.xml
// 4. /odm/etc/manifest.xml
// where:
// {sku} is the value of ro.boot.product.hardware.sku
status_t VintfObject::FetchOdmHalManifest(HalManifest* out, std::string* error) {
    status_t status;

    std::string productModel;
    productModel = getPropertyFetcher().getProperty("ro.boot.product.hardware.sku", "");

    if (!productModel.empty()) {
        status =
            FetchOneHalManifest(kOdmVintfDir + "manifest_" + productModel + ".xml", out, error);
        if (status == OK || status != NAME_NOT_FOUND) {
            return status;
        }
    }

    status = FetchOneHalManifest(kOdmManifest, out, error);
    if (status == OK || status != NAME_NOT_FOUND) {
        return status;
    }

    if (!productModel.empty()) {
        status = FetchOneHalManifest(kOdmLegacyVintfDir + "manifest_" + productModel + ".xml", out,
                                     error);
        if (status == OK || status != NAME_NOT_FOUND) {
            return status;
        }
    }

    status = FetchOneHalManifest(kOdmLegacyManifest, out, error);
    if (status == OK || status != NAME_NOT_FOUND) {
        return status;
    }

    return NAME_NOT_FOUND;
}

// Fetch one manifest.xml file. "out" is written to iff return status is OK.
// Returns NAME_NOT_FOUND if file is missing.
status_t VintfObject::FetchOneHalManifest(const std::string& path, HalManifest* out,
                                          std::string* error) {
    HalManifest ret;
    status_t status = ret.fetchAllInformation(path, error);
    if (status == OK) {
        *out = std::move(ret);
    }
    return status;
}

status_t VintfObject::FetchDeviceMatrix(CompatibilityMatrix* out, std::string* error) {
    CompatibilityMatrix etcMatrix;
    if (etcMatrix.fetchAllInformation(kVendorMatrix, error) == OK) {
        *out = std::move(etcMatrix);
        return OK;
    }
    return out->fetchAllInformation(kVendorLegacyMatrix, error);
}

status_t VintfObject::FetchFrameworkHalManifest(HalManifest* out, std::string* error) {
    HalManifest etcManifest;
    if (etcManifest.fetchAllInformation(kSystemManifest, error) == OK) {
        *out = std::move(etcManifest);
        return OK;
    }
    return out->fetchAllInformation(kSystemLegacyManifest, error);
}

std::vector<Named<CompatibilityMatrix>> VintfObject::GetAllFrameworkMatrixLevels(
    std::string* error) {
    std::vector<std::string> fileNames;
    std::vector<Named<CompatibilityMatrix>> results;

    if (details::gFetcher->listFiles(kSystemVintfDir, &fileNames, error) != OK) {
        return {};
    }
    for (const std::string& fileName : fileNames) {
        std::string path = kSystemVintfDir + fileName;

        std::string content;
        std::string fetchError;
        status_t status = details::gFetcher->fetch(path, content, &fetchError);
        if (status != OK) {
            if (error) {
                *error += "Framework Matrix: Ignore file " + path + ": " + fetchError + "\n";
            }
            continue;
        }

        auto it = results.emplace(results.end());
        if (!gCompatibilityMatrixConverter(&it->object, content, error)) {
            if (error) {
                *error += "Framework Matrix: Ignore file " + path + ": " + *error + "\n";
            }
            results.erase(it);
            continue;
        }
    }

    if (results.empty()) {
        if (error) {
            *error = "No framework matrices under " + kSystemVintfDir +
                     " can be fetched or parsed.\n" + *error;
        }
    } else {
        if (error && !error->empty()) {
            LOG(WARNING) << *error;
            *error = "";
        }
    }

    return results;
}

// static
std::shared_ptr<const RuntimeInfo> VintfObject::GetRuntimeInfo(bool skipCache,
                                                               RuntimeInfo::FetchFlags flags) {
    static LockedRuntimeInfoCache gDeviceRuntimeInfo;
    std::unique_lock<std::mutex> _lock(gDeviceRuntimeInfo.mutex);

    if (!skipCache) {
        flags &= (~gDeviceRuntimeInfo.fetchedFlags);
    }

    if (gDeviceRuntimeInfo.object == nullptr) {
        gDeviceRuntimeInfo.object = details::gRuntimeInfoFactory->make_shared();
    }

    status_t status = gDeviceRuntimeInfo.object->fetchAllInformation(flags);
    if (status != OK) {
        gDeviceRuntimeInfo.fetchedFlags &= (~flags);  // mark the fields as "not fetched"
        return nullptr;
    }

    gDeviceRuntimeInfo.fetchedFlags |= flags;
    return gDeviceRuntimeInfo.object;
}

namespace details {

enum class ParseStatus {
    OK,
    PARSE_ERROR,
    DUPLICATED_FWK_ENTRY,
    DUPLICATED_DEV_ENTRY,
};

static std::string toString(ParseStatus status) {
    switch(status) {
        case ParseStatus::OK:                   return "OK";
        case ParseStatus::PARSE_ERROR:          return "parse error";
        case ParseStatus::DUPLICATED_FWK_ENTRY: return "duplicated framework";
        case ParseStatus::DUPLICATED_DEV_ENTRY: return "duplicated device";
    }
    return "";
}

template<typename T>
static ParseStatus tryParse(const std::string &xml, const XmlConverter<T> &parse,
        std::shared_ptr<T> *fwk, std::shared_ptr<T> *dev) {
    std::shared_ptr<T> ret = std::make_shared<T>();
    if (!parse(ret.get(), xml, nullptr /* error */)) {
        return ParseStatus::PARSE_ERROR;
    }
    if (ret->type() == SchemaType::FRAMEWORK) {
        if (fwk->get() != nullptr) {
            return ParseStatus::DUPLICATED_FWK_ENTRY;
        }
        *fwk = std::move(ret);
    } else if (ret->type() == SchemaType::DEVICE) {
        if (dev->get() != nullptr) {
            return ParseStatus::DUPLICATED_DEV_ENTRY;
        }
        *dev = std::move(ret);
    }
    return ParseStatus::OK;
}

template<typename T, typename GetFunction>
static status_t getMissing(const std::shared_ptr<T>& pkg, bool mount,
        std::function<status_t(void)> mountFunction,
        std::shared_ptr<const T>* updated,
        GetFunction getFunction) {
    if (pkg != nullptr) {
        *updated = pkg;
    } else {
        if (mount) {
            (void)mountFunction(); // ignore mount errors
        }
        *updated = getFunction();
    }
    return OK;
}

#define ADD_MESSAGE(__error__)  \
    if (error != nullptr) {     \
        *error += (__error__);  \
    }                           \

struct PackageInfo {
    struct Pair {
        std::shared_ptr<HalManifest>         manifest;
        std::shared_ptr<CompatibilityMatrix> matrix;
    };
    Pair dev;
    Pair fwk;
};

struct UpdatedInfo {
    struct Pair {
        std::shared_ptr<const HalManifest>         manifest;
        std::shared_ptr<const CompatibilityMatrix> matrix;
    };
    Pair dev;
    Pair fwk;
    std::shared_ptr<const RuntimeInfo> runtimeInfo;
};

// Checks given compatibility info against info on the device. If no
// compatability info is given then the device info will be checked against
// itself.
int32_t checkCompatibility(const std::vector<std::string>& xmls, bool mount,
                           const PartitionMounter& mounter, std::string* error,
                           DisabledChecks disabledChecks) {
    status_t status;
    ParseStatus parseStatus;
    PackageInfo pkg; // All information from package.
    UpdatedInfo updated; // All files and runtime info after the update.

    // parse all information from package
    for (const auto &xml : xmls) {
        parseStatus = tryParse(xml, gHalManifestConverter, &pkg.fwk.manifest, &pkg.dev.manifest);
        if (parseStatus == ParseStatus::OK) {
            continue; // work on next one
        }
        if (parseStatus != ParseStatus::PARSE_ERROR) {
            ADD_MESSAGE(toString(parseStatus) + " manifest");
            return ALREADY_EXISTS;
        }
        parseStatus = tryParse(xml, gCompatibilityMatrixConverter, &pkg.fwk.matrix, &pkg.dev.matrix);
        if (parseStatus == ParseStatus::OK) {
            continue; // work on next one
        }
        if (parseStatus != ParseStatus::PARSE_ERROR) {
            ADD_MESSAGE(toString(parseStatus) + " matrix");
            return ALREADY_EXISTS;
        }
        ADD_MESSAGE(toString(parseStatus)); // parse error
        return BAD_VALUE;
    }

    // get missing info from device
    // use functions instead of std::bind because std::bind doesn't work well with mock objects
    auto mountSystem = [&mounter] { return mounter.mountSystem(); };
    auto mountVendor = [&mounter] { return mounter.mountVendor(); };
    if ((status = getMissing(
             pkg.fwk.manifest, mount, mountSystem, &updated.fwk.manifest,
             std::bind(VintfObject::GetFrameworkHalManifest, true /* skipCache */))) != OK) {
        return status;
    }
    if ((status = getMissing(
             pkg.dev.manifest, mount, mountVendor, &updated.dev.manifest,
             std::bind(VintfObject::GetDeviceHalManifest, true /* skipCache */))) != OK) {
        return status;
    }
    if ((status = getMissing(
             pkg.fwk.matrix, mount, mountSystem, &updated.fwk.matrix,
             std::bind(VintfObject::GetFrameworkCompatibilityMatrix, true /* skipCache */))) !=
        OK) {
        return status;
    }
    if ((status = getMissing(
             pkg.dev.matrix, mount, mountVendor, &updated.dev.matrix,
             std::bind(VintfObject::GetDeviceCompatibilityMatrix, true /* skipCache */))) != OK) {
        return status;
    }

    if (mount) {
        (void)mounter.umountSystem(); // ignore errors
        (void)mounter.umountVendor(); // ignore errors
    }

    if ((disabledChecks & DISABLE_RUNTIME_INFO) == 0) {
        updated.runtimeInfo = VintfObject::GetRuntimeInfo(true /* skipCache */);
    }

    // null checks for files and runtime info after the update
    if (updated.fwk.manifest == nullptr) {
        ADD_MESSAGE("No framework manifest file from device or from update package");
        return NO_INIT;
    }
    if (updated.dev.manifest == nullptr) {
        ADD_MESSAGE("No device manifest file from device or from update package");
        return NO_INIT;
    }
    if (updated.fwk.matrix == nullptr) {
        ADD_MESSAGE("No framework matrix file from device or from update package");
        return NO_INIT;
    }
    if (updated.dev.matrix == nullptr) {
        ADD_MESSAGE("No device matrix file from device or from update package");
        return NO_INIT;
    }

    if ((disabledChecks & DISABLE_RUNTIME_INFO) == 0) {
        if (updated.runtimeInfo == nullptr) {
            ADD_MESSAGE("No runtime info from device");
            return NO_INIT;
        }
    }

    // compatiblity check.
    if (!updated.dev.manifest->checkCompatibility(*updated.fwk.matrix, error)) {
        if (error) {
            error->insert(0,
                          "Device manifest and framework compatibility matrix are incompatible: ");
        }
        return INCOMPATIBLE;
    }
    if (!updated.fwk.manifest->checkCompatibility(*updated.dev.matrix, error)) {
        if (error) {
            error->insert(0,
                          "Framework manifest and device compatibility matrix are incompatible: ");
        }
        return INCOMPATIBLE;
    }

    if ((disabledChecks & DISABLE_RUNTIME_INFO) == 0) {
        if (!updated.runtimeInfo->checkCompatibility(*updated.fwk.matrix, error, disabledChecks)) {
            if (error) {
                error->insert(0,
                              "Runtime info and framework compatibility matrix are incompatible: ");
            }
            return INCOMPATIBLE;
        }
    }

    return COMPATIBLE;
}

const std::string kSystemVintfDir = "/system/etc/vintf/";
const std::string kVendorVintfDir = "/vendor/etc/vintf/";
const std::string kOdmVintfDir = "/odm/etc/vintf/";

const std::string kVendorManifest = kVendorVintfDir + "manifest.xml";
const std::string kSystemManifest = kSystemVintfDir + "manifest.xml";
const std::string kVendorMatrix = kVendorVintfDir + "compatibility_matrix.xml";
const std::string kOdmManifest = kOdmVintfDir + "manifest.xml";

const std::string kVendorLegacyManifest = "/vendor/manifest.xml";
const std::string kVendorLegacyMatrix = "/vendor/compatibility_matrix.xml";
const std::string kSystemLegacyManifest = "/system/manifest.xml";
const std::string kSystemLegacyMatrix = "/system/compatibility_matrix.xml";
const std::string kOdmLegacyVintfDir = "/odm/etc/";
const std::string kOdmLegacyManifest = kOdmLegacyVintfDir + "manifest.xml";

std::vector<std::string> dumpFileList() {
    return {
        kSystemVintfDir,       kVendorVintfDir,     kOdmVintfDir,          kOdmLegacyVintfDir,

        kVendorLegacyManifest, kVendorLegacyMatrix, kSystemLegacyManifest, kSystemLegacyMatrix,
    };
}

} // namespace details

// static
int32_t VintfObject::CheckCompatibility(const std::vector<std::string>& xmls, std::string* error,
                                        DisabledChecks disabledChecks) {
    return details::checkCompatibility(xmls, false /* mount */, *details::gPartitionMounter, error,
                                       disabledChecks);
}

bool VintfObject::isHalDeprecated(const MatrixHal& oldMatrixHal,
                                  const CompatibilityMatrix& targetMatrix,
                                  const ListInstances& listInstances, std::string* error) {
    bool isDeprecated = false;
    oldMatrixHal.forEachInstance([&](const MatrixInstance& oldMatrixInstance) {
        if (isInstanceDeprecated(oldMatrixInstance, targetMatrix, listInstances, error)) {
            isDeprecated = true;
        }
        return !isDeprecated;  // continue if no deprecated instance is found.
    });
    return isDeprecated;
}

// Let oldMatrixInstance = package@x.y-w::interface with instancePattern.
// If any "servedInstance" in listInstances(package@x.y::interface) matches instancePattern, return
// true iff:
// 1. package@x.?::interface/servedInstance is not in targetMatrix; OR
// 2. package@x.z::interface/servedInstance is in targetMatrix but
//    servedInstance is not in listInstances(package@x.z::interface)
bool VintfObject::isInstanceDeprecated(const MatrixInstance& oldMatrixInstance,
                                       const CompatibilityMatrix& targetMatrix,
                                       const ListInstances& listInstances, std::string* error) {
    const std::string& package = oldMatrixInstance.package();
    const Version& version = oldMatrixInstance.versionRange().minVer();
    const std::string& interface = oldMatrixInstance.interface();

    std::vector<std::string> instanceHint;
    if (!oldMatrixInstance.isRegex()) {
        instanceHint.push_back(oldMatrixInstance.exactInstance());
    }

    auto list = listInstances(package, version, interface, instanceHint);
    for (const auto& pair : list) {
        const std::string& servedInstance = pair.first;
        Version servedVersion = pair.second;
        if (!oldMatrixInstance.matchInstance(servedInstance)) {
            continue;
        }

        // Find any package@x.? in target matrix, and check if instance is in target matrix.
        bool foundInstance = false;
        Version targetMatrixMinVer;
        targetMatrix.forEachInstanceOfPackage(package, [&](const auto& targetMatrixInstance) {
            if (targetMatrixInstance.versionRange().majorVer == version.majorVer &&
                targetMatrixInstance.interface() == interface &&
                targetMatrixInstance.matchInstance(servedInstance)) {
                targetMatrixMinVer = targetMatrixInstance.versionRange().minVer();
                foundInstance = true;
            }
            return !foundInstance;  // continue if not found
        });
        if (!foundInstance) {
            if (error) {
                *error = toFQNameString(package, servedVersion, interface, servedInstance) +
                         " is deprecated in compatibility matrix at FCM Version " +
                         to_string(targetMatrix.level()) + "; it should not be served.";
            }
            return true;
        }

        // Assuming that targetMatrix requires @x.u-v, require that at least @x.u is served.
        bool targetVersionServed = false;
        for (const auto& newPair :
             listInstances(package, targetMatrixMinVer, interface, instanceHint)) {
            if (newPair.first == servedInstance) {
                targetVersionServed = true;
                break;
            }
        }

        if (!targetVersionServed) {
            if (error) {
                *error += toFQNameString(package, servedVersion, interface, servedInstance) +
                          " is deprecated; requires at least " + to_string(targetMatrixMinVer) +
                          "\n";
            }
            return true;
        }
    }

    return false;
}

int32_t VintfObject::CheckDeprecation(const ListInstances& listInstances, std::string* error) {
    auto matrixFragments = GetAllFrameworkMatrixLevels(error);
    if (matrixFragments.empty()) {
        if (error && error->empty())
            *error = "Cannot get framework matrix for each FCM version for unknown error.";
        return NAME_NOT_FOUND;
    }
    auto deviceManifest = GetDeviceHalManifest();
    if (deviceManifest == nullptr) {
        if (error) *error = "No device manifest.";
        return NAME_NOT_FOUND;
    }
    Level deviceLevel = deviceManifest->level();
    if (deviceLevel == Level::UNSPECIFIED) {
        if (error) *error = "Device manifest does not specify Shipping FCM Version.";
        return BAD_VALUE;
    }

    const CompatibilityMatrix* targetMatrix = nullptr;
    for (const auto& namedMatrix : matrixFragments) {
        if (namedMatrix.object.level() == deviceLevel) {
            targetMatrix = &namedMatrix.object;
        }
    }
    if (targetMatrix == nullptr) {
        if (error)
            *error = "Cannot find framework matrix at FCM version " + to_string(deviceLevel) + ".";
        return NAME_NOT_FOUND;
    }

    bool hasDeprecatedHals = false;
    for (const auto& namedMatrix : matrixFragments) {
        if (namedMatrix.object.level() == Level::UNSPECIFIED) continue;
        if (namedMatrix.object.level() >= deviceLevel) continue;

        const auto& oldMatrix = namedMatrix.object;
        for (const MatrixHal& hal : oldMatrix.getHals()) {
            hasDeprecatedHals |= isHalDeprecated(hal, *targetMatrix, listInstances, error);
        }
    }

    return hasDeprecatedHals ? DEPRECATED : NO_DEPRECATED_HALS;
}

int32_t VintfObject::CheckDeprecation(std::string* error) {
    using namespace std::placeholders;
    auto deviceManifest = GetDeviceHalManifest();
    ListInstances inManifest =
        [&deviceManifest](const std::string& package, Version version, const std::string& interface,
                          const std::vector<std::string>& /* hintInstances */) {
            std::vector<std::pair<std::string, Version>> ret;
            deviceManifest->forEachInstanceOfInterface(
                package, version, interface, [&ret](const ManifestInstance& manifestInstance) {
                    ret.push_back(
                        std::make_pair(manifestInstance.instance(), manifestInstance.version()));
                    return true;
                });
            return ret;
        };
    return CheckDeprecation(inManifest, error);
}

} // namespace vintf
} // namespace android
