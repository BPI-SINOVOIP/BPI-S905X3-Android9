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

#ifndef ANDROID_VINTF_VINTF_OBJECT_H_
#define ANDROID_VINTF_VINTF_OBJECT_H_

#include <memory>

#include "CompatibilityMatrix.h"
#include "DisabledChecks.h"
#include "HalManifest.h"
#include "Named.h"
#include "RuntimeInfo.h"

namespace android {
namespace vintf {
/*
 * The top level class for libvintf.
 * An overall diagram of the public API:
 * VintfObject
 *   + GetDeviceHalManfiest
 *   |   + getTransport
 *   |   + checkCompatibility
 *   + GetFrameworkHalManifest
 *   |   + getTransport
 *   |   + checkCompatibility
 *   + GetRuntimeInfo
 *       + checkCompatibility
 *
 * Each of the function gathers all information and encapsulate it into the object.
 * If no error, it return the same singleton object in the future, and the HAL manifest
 * file won't be touched again.
 * If any error, nullptr is returned, and Get will try to parse the HAL manifest
 * again when it is called again.
 * All these operations are thread-safe.
 * If skipCache, always skip the cache in memory and read the files / get runtime information
 * again from the device.
 */
class VintfObject {
public:
    /*
     * Return the API that access the device-side HAL manifest stored
     * in /vendor/manifest.xml.
     */
    static std::shared_ptr<const HalManifest> GetDeviceHalManifest(bool skipCache = false);

    /*
     * Return the API that access the framework-side HAL manifest stored
     * in /system/manfiest.xml.
     */
    static std::shared_ptr<const HalManifest> GetFrameworkHalManifest(bool skipCache = false);

    /*
     * Return the API that access the device-side compatibility matrix stored
     * in /vendor/compatibility_matrix.xml.
     */
    static std::shared_ptr<const CompatibilityMatrix> GetDeviceCompatibilityMatrix(
        bool skipCache = false);

    /*
     * Return the API that access the framework-side compatibility matrix stored
     * in /system/compatibility_matrix.xml.
     */
    static std::shared_ptr<const CompatibilityMatrix> GetFrameworkCompatibilityMatrix(
        bool skipCache = false);

    /*
     * Return the API that access device runtime info.
     *
     * {skipCache == true, flags == ALL}: re-fetch everything
     * {skipCache == false, flags == ALL}: fetch everything if not previously fetched
     * {skipCache == true, flags == selected info}: re-fetch selected information
     *                                if not previously fetched.
     * {skipCache == false, flags == selected info}: fetch selected information
     *                                if not previously fetched.
     *
     * @param skipCache do not fetch if previously fetched
     * @param flags bitwise-or of RuntimeInfo::FetchFlag
     */
    static std::shared_ptr<const RuntimeInfo> GetRuntimeInfo(
        bool skipCache = false, RuntimeInfo::FetchFlags flags = RuntimeInfo::FetchFlag::ALL);

    /**
     * Check compatibility, given a set of manifests / matrices in packageInfo.
     * They will be checked against the manifests / matrices on the device.
     *
     * @param packageInfo a list of XMLs of HalManifest /
     * CompatibilityMatrix objects.
     * @param error error message
     * @param disabledChecks flags to disable certain checks. See DisabledChecks.
     *
     * @return = 0 if success (compatible)
     *         > 0 if incompatible
     *         < 0 if any error (mount partition fails, illformed XML, etc.)
     */
    static int32_t CheckCompatibility(const std::vector<std::string>& packageInfo,
                                      std::string* error = nullptr,
                                      DisabledChecks disabledChecks = ENABLE_ALL_CHECKS);

    /**
     * A std::function that abstracts a list of "provided" instance names. Given package, version
     * and interface, the function returns a list of instance names that matches.
     * This function can represent a manifest, an IServiceManager, etc.
     * If the source is passthrough service manager, a list of instance names cannot be provided.
     * Instead, the function should call getService on each of the "hintInstances", and
     * return those instances for which getService does not return a nullptr. This means that for
     * passthrough HALs, the deprecation on <regex-instance>s cannot be enforced; only <instance>s
     * can be enforced.
     */
    using ListInstances = std::function<std::vector<std::pair<std::string, Version>>(
        const std::string& package, Version version, const std::string& interface,
        const std::vector<std::string>& hintInstances)>;
    /**
     * Check deprecation on framework matrices with a provided predicate.
     *
     * @param listInstances predicate that takes parameter in this format:
     *        android.hardware.foo@1.0::IFoo
     *        and returns {{"default", version}...} if HAL is in use, where version =
     *        first version in interfaceChain where package + major version matches.
     *
     * @return = 0 if success (no deprecated HALs)
     *         > 0 if there is at least one deprecated HAL
     *         < 0 if any error (mount partition fails, illformed XML, etc.)
     */
    static int32_t CheckDeprecation(const ListInstances& listInstances,
                                    std::string* error = nullptr);

    /**
     * Check deprecation on existing VINTF metadata. Use Device Manifest as the
     * predicate to check if a HAL is in use.
     *
     * @return = 0 if success (no deprecated HALs)
     *         > 0 if there is at least one deprecated HAL
     *         < 0 if any error (mount partition fails, illformed XML, etc.)
     */
    static int32_t CheckDeprecation(std::string* error = nullptr);

   private:
    static status_t GetCombinedFrameworkMatrix(
        const std::shared_ptr<const HalManifest>& deviceManifest, CompatibilityMatrix* out,
        std::string* error = nullptr);
    static std::vector<Named<CompatibilityMatrix>> GetAllFrameworkMatrixLevels(
        std::string* error = nullptr);
    static status_t FetchDeviceHalManifest(HalManifest* out, std::string* error = nullptr);
    static status_t FetchDeviceMatrix(CompatibilityMatrix* out, std::string* error = nullptr);
    static status_t FetchOdmHalManifest(HalManifest* out, std::string* error = nullptr);
    static status_t FetchOneHalManifest(const std::string& path, HalManifest* out,
                                        std::string* error = nullptr);
    static status_t FetchFrameworkHalManifest(HalManifest* out, std::string* error = nullptr);

    static bool isHalDeprecated(const MatrixHal& oldMatrixHal,
                                const CompatibilityMatrix& targetMatrix,
                                const ListInstances& listInstances, std::string* error);
    static bool isInstanceDeprecated(const MatrixInstance& oldMatrixInstance,
                                     const CompatibilityMatrix& targetMatrix,
                                     const ListInstances& listInstances, std::string* error);
};

enum : int32_t {
    COMPATIBLE = 0,
    INCOMPATIBLE = 1,

    NO_DEPRECATED_HALS = 0,
    DEPRECATED = 1,
};

// exposed for testing and VintfObjectRecovery.
namespace details {
class PartitionMounter;
int32_t checkCompatibility(const std::vector<std::string>& xmls, bool mount,
                           const PartitionMounter& partitionMounter, std::string* error,
                           DisabledChecks disabledChecks = ENABLE_ALL_CHECKS);

extern const std::string kSystemVintfDir;
extern const std::string kVendorVintfDir;
extern const std::string kOdmVintfDir;
extern const std::string kOdmLegacyVintfDir;
extern const std::string kOdmLegacyManifest;
extern const std::string kVendorManifest;
extern const std::string kSystemManifest;
extern const std::string kVendorMatrix;
extern const std::string kOdmManifest;
extern const std::string kVendorLegacyManifest;
extern const std::string kVendorLegacyMatrix;
extern const std::string kSystemLegacyManifest;
extern const std::string kSystemLegacyMatrix;

// Convenience function to dump all files and directories that could be read
// by calling Get(Framework|Device)(HalManifest|CompatibilityMatrix). The list
// include files that may not actually be read when the four functions are called
// because some files have a higher priority than others. The list does NOT
// include "files" (including kernel interfaces) that are read when GetRuntimeInfo
// is called.
std::vector<std::string> dumpFileList();

} // namespace details

} // namespace vintf
} // namespace android

#endif // ANDROID_VINTF_VINTF_OBJECT_H_
