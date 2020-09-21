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

#include "CompatibilityMatrix.h"

#include <iostream>
#include <utility>

#include "parse_string.h"
#include "parse_xml.h"
#include "utils.h"

namespace android {
namespace vintf {

bool CompatibilityMatrix::add(MatrixHal &&hal) {
    return HalGroup<MatrixHal>::add(std::move(hal));
}

bool CompatibilityMatrix::add(MatrixKernel &&kernel) {
    if (mType != SchemaType::FRAMEWORK) {
        return false;
    }
    framework.mKernels.push_back(std::move(kernel));
    return true;
}

SchemaType CompatibilityMatrix::type() const {
    return mType;
}

Level CompatibilityMatrix::level() const {
    return mLevel;
}

Version CompatibilityMatrix::getMinimumMetaVersion() const {
    // TODO(b/62801658): this needs to depend on whether there are 1.1 requirements
    // (e.g. required <xmlfile> entry)
    return {1, 0};
}

status_t CompatibilityMatrix::fetchAllInformation(const std::string& path, std::string* error) {
    return details::fetchAllInformation(path, gCompatibilityMatrixConverter, this, error);
}

std::string CompatibilityMatrix::getXmlSchemaPath(const std::string& xmlFileName,
                                                  const Version& version) const {
    using std::literals::string_literals::operator""s;
    auto range = getXmlFiles(xmlFileName);
    for (auto it = range.first; it != range.second; ++it) {
        const MatrixXmlFile& matrixXmlFile = it->second;
        if (matrixXmlFile.versionRange().contains(version)) {
            if (!matrixXmlFile.overriddenPath().empty()) {
                return matrixXmlFile.overriddenPath();
            }
            return "/"s + (type() == SchemaType::DEVICE ? "vendor" : "system") + "/etc/" +
                   xmlFileName + "_V" + std::to_string(matrixXmlFile.versionRange().majorVer) +
                   "_" + std::to_string(matrixXmlFile.versionRange().maxMinor) + "." +
                   to_string(matrixXmlFile.format());
        }
    }
    return "";
}

// Split existingHal into a HAL that contains only interface/instance and a HAL
// that does not contain it. Return the HAL that contains only interface/instance.
// - Return nullptr if existingHal does not contain interface/instance
// - Return existingHal if existingHal contains only interface/instance
// - Remove interface/instance from existingHal, and return a new MatrixHal (that is added
//   to "this") that contains only interface/instance.
MatrixHal* CompatibilityMatrix::splitInstance(MatrixHal* existingHal, const std::string& interface,
                                              const std::string& instanceOrPattern, bool isRegex) {
    bool found = false;
    bool foundOthers = false;
    existingHal->forEachInstance([&](const auto& matrixInstance) {
        bool interfaceMatch = matrixInstance.interface() == interface;
        bool instanceMatch = false;
        if (matrixInstance.isRegex() && isRegex) {
            instanceMatch = (matrixInstance.regexPattern() == instanceOrPattern);
        } else if (!matrixInstance.isRegex() && !isRegex) {
            instanceMatch = (matrixInstance.exactInstance() == instanceOrPattern);
        }

        bool match = interfaceMatch && instanceMatch;

        found |= match;
        foundOthers |= (!match);

        return !found || !foundOthers;
    });

    if (!found) {
        return nullptr;
    }

    if (!foundOthers) {
        return existingHal;
    }

    existingHal->removeInstance(interface, instanceOrPattern, isRegex);
    MatrixHal copy = *existingHal;
    copy.clearInstances();
    copy.insertInstance(interface, instanceOrPattern, isRegex);

    return addInternal(std::move(copy));
}

// Add all package@other_version::interface/instance as an optional instance.
// If package@this_version::interface/instance is in this (that is, some instance
// with the same package and interface and instance exists), then other_version is
// considered a possible replacement to this_version.
// See LibVintfTest.AddOptionalHal* tests for details.
bool CompatibilityMatrix::addAllHalsAsOptional(CompatibilityMatrix* other, std::string* error) {
    if (other == nullptr || other->level() <= level()) {
        return true;
    }

    for (auto& pair : other->mHals) {
        const std::string& name = pair.first;
        MatrixHal& halToAdd = pair.second;

        std::set<std::pair<std::string, std::string>> insertedInstances;
        std::set<std::pair<std::string, std::string>> insertedRegex;
        auto existingHals = getHals(name);

        halToAdd.forEachInstance([&](const std::vector<VersionRange>& versionRanges,
                                     const std::string& interface,
                                     const std::string& instanceOrPattern, bool isRegex) {
            for (auto* existingHal : existingHals) {
                MatrixHal* splitInstance =
                    this->splitInstance(existingHal, interface, instanceOrPattern, isRegex);
                if (splitInstance != nullptr) {
                    splitInstance->insertVersionRanges(versionRanges);
                    if (isRegex) {
                        insertedRegex.insert(std::make_pair(interface, instanceOrPattern));
                    } else {
                        insertedInstances.insert(std::make_pair(interface, instanceOrPattern));
                    }
                }
            }
            return true;
        });

        // Add the remaining instances.
        for (const auto& pair : insertedInstances) {
            halToAdd.removeInstance(pair.first, pair.second, false /* isRegex */);
        }
        for (const auto& pair : insertedRegex) {
            halToAdd.removeInstance(pair.first, pair.second, true /* isRegex */);
        }

        if (halToAdd.instancesCount() > 0) {
            halToAdd.setOptional(true);
            if (!add(std::move(halToAdd))) {
                if (error) {
                    *error = "Cannot add HAL " + name + " for unknown reason.";
                }
                return false;
            }
        }
    }
    return true;
}

bool CompatibilityMatrix::addAllXmlFilesAsOptional(CompatibilityMatrix* other, std::string* error) {
    if (other == nullptr || other->level() <= level()) {
        return true;
    }
    for (auto& pair : other->mXmlFiles) {
        const std::string& name = pair.first;
        MatrixXmlFile& xmlFileToAdd = pair.second;

        xmlFileToAdd.mOptional = true;
        if (!addXmlFile(std::move(xmlFileToAdd))) {
            if (error) {
                *error = "Cannot add XML File " + name + " for unknown reason.";
            }
            return false;
        }
    }
    return true;
}

bool CompatibilityMatrix::addAllKernelsAsOptional(CompatibilityMatrix* other, std::string* error) {
    if (other == nullptr || other->level() <= level()) {
        return true;
    }

    for (MatrixKernel& kernelToAdd : other->framework.mKernels) {
        bool exists =
            std::any_of(this->framework.mKernels.begin(), this->framework.mKernels.end(),
                        [&kernelToAdd](const MatrixKernel& existing) {
                            return kernelToAdd.minLts().version == existing.minLts().version &&
                                   kernelToAdd.minLts().majorRev == existing.minLts().majorRev;
                        });

        if (exists) {
            // Shouldn't retroactively add requirements to minLts(), so ignore this.
            // This happens even when kernelToAdd.conditions() != existing.conditions().
            continue;
        }

        KernelVersion minLts = kernelToAdd.minLts();
        if (!add(std::move(kernelToAdd))) {
            if (error) {
                *error = "Cannot add " + to_string(minLts) + " for unknown reason.";
            }
            return false;
        }
    }
    return true;
}

bool operator==(const CompatibilityMatrix &lft, const CompatibilityMatrix &rgt) {
    return lft.mType == rgt.mType && lft.mLevel == rgt.mLevel && lft.mHals == rgt.mHals &&
           lft.mXmlFiles == rgt.mXmlFiles &&
           (lft.mType != SchemaType::DEVICE ||
            (
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
                lft.device.mVndk == rgt.device.mVndk &&
#pragma clang diagnostic pop
                lft.device.mVendorNdk == rgt.device.mVendorNdk &&
                lft.device.mSystemSdk == rgt.device.mSystemSdk)) &&
           (lft.mType != SchemaType::FRAMEWORK ||
            (lft.framework.mKernels == rgt.framework.mKernels &&
             lft.framework.mSepolicy == rgt.framework.mSepolicy &&
             lft.framework.mAvbMetaVersion == rgt.framework.mAvbMetaVersion));
}

// Find compatibility_matrix.empty.xml (which has unspecified level) and use it
// as a base matrix.
CompatibilityMatrix* CompatibilityMatrix::findOrInsertBaseMatrix(
    std::vector<Named<CompatibilityMatrix>>* matrices, std::string* error) {
    std::vector<CompatibilityMatrix*> matricesUnspecified;
    std::vector<CompatibilityMatrix*> matricesEmpty;
    for (auto& e : *matrices) {
        if (e.object.level() == Level::UNSPECIFIED) {
            matricesUnspecified.push_back(&e.object);

            if (!e.object.mHals.empty()) {
                continue;
            }

            if (!e.object.mXmlFiles.empty()) {
                continue;
            }

            matricesEmpty.push_back(&e.object);
        }
    }

    if (matricesEmpty.size() > 1) {
        if (error) {
            *error =
                "Error: multiple framework compatibility matrix files have "
                "unspecified level; there should only be one such file.\n";
            for (auto& e : *matrices) {
                if (e.object.level() == Level::UNSPECIFIED) {
                    *error += "    " + e.name + "\n";
                }
            }
        }
        return nullptr;
    }
    if (matricesEmpty.size() == 1) {
        return matricesEmpty.front();
    }
    if (!matricesUnspecified.empty()) {
        return matricesUnspecified.front();
    }
    auto matrix = &matrices->emplace(matrices->end())->object;
    matrix->mType = SchemaType::FRAMEWORK;
    matrix->mLevel = Level::UNSPECIFIED;
    return matrix;
}

// Check if there are two files declaring level="1", for example, because
// combine() use this assumption to simplify a lot of logic.
static bool checkDuplicateLevels(const std::vector<Named<CompatibilityMatrix>>& matrices,
                                 std::string* error) {
    std::map<Level, const std::string*> existingLevels;
    for (const auto& e : matrices) {
        if (e.object.level() == Level::UNSPECIFIED &&
            existingLevels.find(e.object.level()) != existingLevels.end()) {
            if (error) {
                *error = "Conflict of levels: file \"" +
                         *existingLevels.find(e.object.level())->second + "\" and \"" + e.name +
                         " both have level " + to_string(e.object.level());
            }
            return false;
        }
        existingLevels.emplace(e.object.level(), &e.name);
    }
    return true;
}

CompatibilityMatrix* CompatibilityMatrix::combine(Level deviceLevel,
                                                  std::vector<Named<CompatibilityMatrix>>* matrices,
                                                  std::string* error) {
    if (!checkDuplicateLevels(*matrices, error)) {
        return nullptr;
    }

    CompatibilityMatrix* matrix = findOrInsertBaseMatrix(matrices, error);
    if (matrix == nullptr) {
        return nullptr;
    }

    matrix->mLevel = deviceLevel;

    for (auto& e : *matrices) {
        if (&e.object != matrix &&
            (e.object.level() == deviceLevel || e.object.level() == Level::UNSPECIFIED)) {
            if (!matrix->addAllHals(&e.object, error)) {
                if (error) {
                    *error = "File \"" + e.name + "\" cannot be added: HAL " + *error +
                             " has a conflict.";
                }
                return nullptr;
            }

            if (!matrix->addAllXmlFiles(&e.object, error)) {
                if (error) {
                    *error = "File \"" + e.name + "\" cannot be added: XML File entry " + *error +
                             " has a conflict.";
                }
                return nullptr;
            }
        }
    }

    for (auto& e : *matrices) {
        if (&e.object != matrix && e.object.level() != Level::UNSPECIFIED &&
            e.object.level() > deviceLevel) {
            if (!matrix->addAllHalsAsOptional(&e.object, error)) {
                if (error) {
                    *error = "File \"" + e.name + "\" cannot be added: " + *error +
                             ". See <hal> with the same name " +
                             "in previously parsed files or previously declared in this file.";
                }
                return nullptr;
            }

            if (!matrix->addAllXmlFilesAsOptional(&e.object, error)) {
                if (error) {
                    *error = "File \"" + e.name + "\" cannot be added: XML File entry " + *error +
                             " has a conflict.";
                }
                return nullptr;
            }
        }
    }

    // Add <kernel> from exact "level", then optionally add <kernel> from high levels to low levels.
    // For example, (each letter is a kernel version x.y.z)
    // 1.xml: A1, B1
    // 2.xml: B2, C2, D2
    // 3.xml: D3, E3
    // Then the combined 1.xml should have
    // A1, B1 (from 1.xml, required), C2, D2, E3 (optional, use earliest possible).
    for (auto& e : *matrices) {
        if (&e.object != matrix && e.object.level() == deviceLevel &&
            e.object.type() == SchemaType::FRAMEWORK) {
            for (MatrixKernel& kernel : e.object.framework.mKernels) {
                KernelVersion ver = kernel.minLts();
                if (!matrix->add(std::move(kernel))) {
                    if (error) {
                        *error = "Cannot add kernel version " + to_string(ver) +
                                 " from FCM version " + to_string(deviceLevel);
                    }
                    return nullptr;
                }
            }
        }
    }

    // There is only one file per level, hence a map is used instead of a multimap. Also, there is
    // no good ordering (i.e. priority) for multiple files with the same level.
    std::map<Level, Named<CompatibilityMatrix>*> matricesMap;
    for (auto& e : *matrices) {
        if (&e.object != matrix && e.object.level() != Level::UNSPECIFIED &&
            e.object.level() > deviceLevel && e.object.type() == SchemaType::FRAMEWORK) {
            matricesMap.emplace(e.object.level(), &e);
        }
    }

    for (auto&& pair : matricesMap) {
        if (!matrix->addAllKernelsAsOptional(&pair.second->object, error)) {
            if (error) {
                *error = "Cannot add new kernel versions from FCM version " +
                         to_string(pair.first) + " (" + pair.second->name + ")" +
                         " to FCM version " + to_string(deviceLevel) + ": " + *error;
            }
            return nullptr;
        }
    }

    return matrix;
}

bool CompatibilityMatrix::forEachInstanceOfVersion(
    const std::string& package, const Version& expectVersion,
    const std::function<bool(const MatrixInstance&)>& func) const {
    for (const MatrixHal* hal : getHals(package)) {
        bool cont = hal->forEachInstance([&](const MatrixInstance& matrixInstance) {
            if (matrixInstance.versionRange().contains(expectVersion)) {
                return func(matrixInstance);
            }
            return true;
        });
        if (!cont) return false;
    }
    return true;
}

bool CompatibilityMatrix::matchInstance(const std::string& halName, const Version& version,
                                        const std::string& interfaceName,
                                        const std::string& instance) const {
    bool found = false;
    (void)forEachInstanceOfInterface(halName, version, interfaceName,
                                     [&found, &instance](const auto& e) {
                                         found |= (e.matchInstance(instance));
                                         return !found;  // if not found, continue
                                     });
    return found;
}

} // namespace vintf
} // namespace android
