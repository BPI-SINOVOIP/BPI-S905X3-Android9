/*
 * Copyright (C) 2016 The Android Open Source Project
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

#define LOG_TAG "HidlInternal"

#include <hidl/HidlInternal.h>

#include <android-base/logging.h>
#include <android-base/properties.h>
#include <android-base/stringprintf.h>
#include <cutils/properties.h>

#ifdef LIBHIDL_TARGET_DEBUGGABLE
#include <dirent.h>
#include <dlfcn.h>
#include <link.h>
#include <utils/misc.h>
#include <regex>

extern "C" __attribute__((weak)) void __sanitizer_cov_dump();
const char* kGcovPrefixEnvVar = "GCOV_PREFIX";
const char* kGcovPrefixOverrideEnvVar = "GCOV_PREFIX_OVERRIDE";
const char* kGcovPrefixPath = "/data/misc/trace/";
const char* kSysPropHalCoverage = "hal.coverage.enable";
#if defined(__LP64__)
const char* kSysPropInstrumentationPath = "hal.instrumentation.lib.path.64";
#else
const char* kSysPropInstrumentationPath = "hal.instrumentation.lib.path.32";
#endif
#endif

namespace android {
namespace hardware {
namespace details {

void logAlwaysFatal(const char* message) {
    LOG(FATAL) << message;
}

std::string getVndkVersionStr() {
    static std::string vndkVersion("0");
    // "0" means the vndkVersion must be initialized with the property value.
    // Otherwise, return the value.
    if (vndkVersion == "0") {
        vndkVersion = android::base::GetProperty("ro.vndk.version", "");
        if (vndkVersion != "" && vndkVersion != "current") {
            vndkVersion = "-" + vndkVersion;
        } else {
            vndkVersion = "";
        }
    }
    return vndkVersion;
}

// ----------------------------------------------------------------------
// HidlInstrumentor implementation.
HidlInstrumentor::HidlInstrumentor(const std::string& package, const std::string& interface)
    : mEnableInstrumentation(false),
      mInstrumentationLibPackage(package),
      mInterfaceName(interface) {
#ifdef LIBHIDL_TARGET_DEBUGGABLE
    configureInstrumentation(false);
    if (__sanitizer_cov_dump != nullptr) {
        ::android::add_sysprop_change_callback(
            []() {
                bool enableCoverage = property_get_bool(kSysPropHalCoverage, false);
                if (enableCoverage) {
                    __sanitizer_cov_dump();
                }
            },
            0);
    }
    if (property_get_bool("ro.vts.coverage", false)) {
        const char* prefixOverride = getenv(kGcovPrefixOverrideEnvVar);
        if (prefixOverride == nullptr || strcmp(prefixOverride, "true") != 0) {
            const std::string gcovPath = kGcovPrefixPath + std::to_string(getpid());
            setenv(kGcovPrefixEnvVar, gcovPath.c_str(), true /* overwrite */);
        }
        ::android::add_sysprop_change_callback(
            []() {
                const bool enableCoverage = property_get_bool(kSysPropHalCoverage, false);
                if (enableCoverage) {
                    dl_iterate_phdr(
                        [](struct dl_phdr_info* info, size_t /* size */, void* /* data */) {
                            if (strlen(info->dlpi_name) == 0) return 0;

                            void* handle = dlopen(info->dlpi_name, RTLD_LAZY);
                            if (handle == nullptr) {
                                LOG(INFO) << "coverage dlopen failed: " << dlerror();
                                return 0;
                            }
                            void (*flush)() = (void (*)())dlsym(handle, "__gcov_flush");
                            if (flush == nullptr) {
                                return 0;
                            }
                            flush();
                            return 0;
                        },
                        nullptr /* data */);
                }
            },
            0 /* priority */);
    }
#endif
}

HidlInstrumentor::~HidlInstrumentor() {}

void HidlInstrumentor::configureInstrumentation(bool log) {
    mEnableInstrumentation = property_get_bool("hal.instrumentation.enable", false);
    if (mEnableInstrumentation) {
        if (log) {
            LOG(INFO) << "Enable instrumentation.";
        }
        mInstrumentationCallbacks.clear();
        registerInstrumentationCallbacks(&mInstrumentationCallbacks);
    } else {
        if (log) {
            LOG(INFO) << "Disable instrumentation.";
        }
        mInstrumentationCallbacks.clear();
    }
}

void HidlInstrumentor::registerInstrumentationCallbacks(
        std::vector<InstrumentationCallback> *instrumentationCallbacks) {
#ifdef LIBHIDL_TARGET_DEBUGGABLE
    std::vector<std::string> instrumentationLibPaths;
    char instrumentationLibPath[PROPERTY_VALUE_MAX];
    if (property_get(kSysPropInstrumentationPath, instrumentationLibPath, "") > 0) {
        instrumentationLibPaths.push_back(instrumentationLibPath);
    } else {
        static std::string halLibPathVndkSp = android::base::StringPrintf(
            HAL_LIBRARY_PATH_VNDK_SP_FOR_VERSION, getVndkVersionStr().c_str());
        instrumentationLibPaths.push_back(HAL_LIBRARY_PATH_SYSTEM);
        instrumentationLibPaths.push_back(halLibPathVndkSp);
        instrumentationLibPaths.push_back(HAL_LIBRARY_PATH_VENDOR);
        instrumentationLibPaths.push_back(HAL_LIBRARY_PATH_ODM);
    }

    for (const auto& path : instrumentationLibPaths) {
        DIR *dir = opendir(path.c_str());
        if (dir == 0) {
            LOG(WARNING) << path << " does not exist. ";
            return;
        }

        struct dirent *file;
        while ((file = readdir(dir)) != nullptr) {
            if (!isInstrumentationLib(file))
                continue;

            void *handle = dlopen((path + file->d_name).c_str(), RTLD_NOW);
            char *error;
            if (handle == nullptr) {
                LOG(WARNING) << "couldn't load file: " << file->d_name
                    << " error: " << dlerror();
                continue;
            }

            dlerror(); /* Clear any existing error */

            using cbFun = void (*)(
                    const InstrumentationEvent,
                    const char *,
                    const char *,
                    const char *,
                    const char *,
                    std::vector<void *> *);
            std::string package = mInstrumentationLibPackage;
            for (size_t i = 0; i < package.size(); i++) {
                if (package[i] == '.') {
                    package[i] = '_';
                    continue;
                }

                if (package[i] == '@') {
                    package[i] = '_';
                    package.insert(i + 1, "V");
                    continue;
                }
            }
            auto cb = (cbFun)dlsym(handle, ("HIDL_INSTRUMENTATION_FUNCTION_"
                        + package + "_" + mInterfaceName).c_str());
            if ((error = dlerror()) != nullptr) {
                LOG(WARNING)
                    << "couldn't find symbol: HIDL_INSTRUMENTATION_FUNCTION_"
                    << package << "_" << mInterfaceName << ", error: " << error;
                continue;
            }
            instrumentationCallbacks->push_back(cb);
            LOG(INFO) << "Register instrumentation callback from "
                << file->d_name;
        }
        closedir(dir);
    }
#else
    // No-op for user builds.
    (void) instrumentationCallbacks;
    return;
#endif
}

bool HidlInstrumentor::isInstrumentationLib(const dirent *file) {
#ifdef LIBHIDL_TARGET_DEBUGGABLE
    if (file->d_type != DT_REG) return false;
    std::cmatch cm;
    std::regex e("^" + mInstrumentationLibPackage + "(.*).profiler.so$");
    if (std::regex_match(file->d_name, cm, e)) return true;
#else
    (void) file;
#endif
    return false;
}

}  // namespace details
}  // namespace hardware
}  // namespace android
