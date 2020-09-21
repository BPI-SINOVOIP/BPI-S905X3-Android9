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

#include "ManifestHal.h"
#include <unordered_set>

#include "MapValueIterator.h"
#include "parse_string.h"

namespace android {
namespace vintf {

bool ManifestHal::isValid() const {
    std::unordered_set<size_t> existing;
    for (const auto &v : versions) {
        if (existing.find(v.majorVer) != existing.end()) {
            return false;
        }
        existing.insert(v.majorVer);
    }
    return transportArch.isValid();
}

bool ManifestHal::operator==(const ManifestHal &other) const {
    if (format != other.format)
        return false;
    if (name != other.name)
        return false;
    if (versions != other.versions)
        return false;
    if (!(transportArch == other.transportArch)) return false;
    if (interfaces != other.interfaces) return false;
    if (isOverride() != other.isOverride()) return false;
    if (mAdditionalInstances != other.mAdditionalInstances) return false;
    return true;
}

bool ManifestHal::forEachInstance(const std::function<bool(const ManifestInstance&)>& func) const {
    for (const auto& v : versions) {
        for (const auto& intf : iterateValues(interfaces)) {
            bool cont = intf.forEachInstance([&](const auto& interface, const auto& instance,
                                                 bool /* isRegex */) {
                // TODO(b/73556059): Store ManifestInstance as well to avoid creating temps
                FqInstance fqInstance;
                if (fqInstance.setTo(getName(), v.majorVer, v.minorVer, interface, instance)) {
                    if (!func(ManifestInstance(std::move(fqInstance), TransportArch{transportArch},
                                               format))) {
                        return false;
                    }
                }
                return true;
            });
            if (!cont) {
                return false;
            }
        }
    }

    for (const auto& manifestInstance : mAdditionalInstances) {
        if (!func(manifestInstance)) {
            return false;
        }
    }

    return true;
}

bool ManifestHal::isDisabledHal() const {
    if (!isOverride()) return false;
    bool hasInstance = false;
    forEachInstance([&hasInstance](const auto&) {
        hasInstance = true;
        return false;  // has at least one instance, stop here.
    });
    return !hasInstance;
}

void ManifestHal::appendAllVersions(std::set<Version>* ret) const {
    ret->insert(versions.begin(), versions.end());
    forEachInstance([&](const auto& e) {
        ret->insert(e.version());
        return true;
    });
}

static bool verifyInstances(const std::set<FqInstance>& fqInstances, std::string* error) {
    for (const FqInstance& fqInstance : fqInstances) {
        if (fqInstance.hasPackage()) {
            if (error) *error = "Should not specify package: \"" + fqInstance.string() + "\"";
            return false;
        }
        if (!fqInstance.hasVersion()) {
            if (error) *error = "Should specify version: \"" + fqInstance.string() + "\"";
            return false;
        }
        if (!fqInstance.hasInterface()) {
            if (error) *error = "Should specify interface: \"" + fqInstance.string() + "\"";
            return false;
        }
        if (!fqInstance.hasInstance()) {
            if (error) *error = "Should specify instance: \"" + fqInstance.string() + "\"";
            return false;
        }
    }
    return true;
}

bool ManifestHal::insertInstances(const std::set<FqInstance>& fqInstances, std::string* error) {
    if (!verifyInstances(fqInstances, error)) {
        return false;
    }

    for (const FqInstance& e : fqInstances) {
        FqInstance withPackage;
        if (!withPackage.setTo(this->getName(), e.getMajorVersion(), e.getMinorVersion(),
                               e.getInterface(), e.getInstance())) {
            if (error) {
                *error = "Cannot create FqInstance with package='" + this->getName() +
                         "', version='" + to_string(Version(e.getVersion())) + "', interface='" +
                         e.getInterface() + "', instance='" + e.getInstance() + "'";
            }
            return false;
        }
        mAdditionalInstances.emplace(std::move(withPackage), this->transportArch, this->format);
    }

    return true;
}

void ManifestHal::insertLegacyInstance(const std::string& interface, const std::string& instance) {
    auto it = interfaces.find(interface);
    if (it == interfaces.end())
        it = interfaces.emplace(interface, HalInterface{interface, {}}).first;
    it->second.insertInstance(instance, false /* isRegex */);
}

} // namespace vintf
} // namespace android
