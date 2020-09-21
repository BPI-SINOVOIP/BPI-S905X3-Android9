/****************************************************************************
*
*    Copyright (c) 2020 Vivante Corporation
*
*    Permission is hereby granted, free of charge, to any person obtaining a
*    copy of this software and associated documentation files (the "Software"),
*    to deal in the Software without restriction, including without limitation
*    the rights to use, copy, modify, merge, publish, distribute, sublicense,
*    and/or sell copies of the Software, and to permit persons to whom the
*    Software is furnished to do so, subject to the following conditions:
*
*    The above copyright notice and this permission notice shall be included in
*    all copies or substantial portions of the Software.
*
*    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
*    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
*    DEALINGS IN THE SOFTWARE.
*
*****************************************************************************/

#ifndef ANDROID_ML_NN_VSI_DEVICE_H
#define ANDROID_ML_NN_VSI_DEVICE_H

#include <thread>
#include <pthread.h>
#include <string>
#include <hidl/LegacySupport.h>
#include <sys/system_properties.h>

#include "HalInterfaces.h"
#include "nnrt/model.hpp"
#include "Utils.h"

#include "VsiPreparedModel.h"
using android::sp;

namespace android {
namespace nn {
namespace vsi_driver {

class VsiDevice : public HalPlatform::Device
{
   public:
    VsiDevice(const char* name) : name_(name) {}
    ~VsiDevice() override {}

    Return<ErrorStatus> prepareModel(
        const V1_0::Model& model,
        const sp<V1_0::IPreparedModelCallback>& callback) override;

#if ANDROID_SDK_VERSION >= 28
    Return<ErrorStatus> prepareModel_1_1(
        const V1_1::Model& model,
        ExecutionPreference preference,
        const sp<V1_0::IPreparedModelCallback>& callback) override;
#endif

#if ANDROID_SDK_VERSION >= 29
    Return<ErrorStatus> prepareModel_1_2(const V1_2::Model& model, ExecutionPreference preference,
        const hidl_vec<hidl_handle>& modelCache,
        const hidl_vec<hidl_handle>& dataCache,
        const HidlToken& token,
        const sp<V1_2::IPreparedModelCallback>& callback) override;

    Return<ErrorStatus> prepareModelFromCache(
        const hidl_vec<hidl_handle>& modelCache, const hidl_vec<hidl_handle>& dataCache,
        const HidlToken& token, const sp<V1_2::IPreparedModelCallback>& callback) override;

    Return<void>
        getVersionString(V1_2::IDevice::getVersionString_cb _hidl_cb) override{
        _hidl_cb(ErrorStatus::NONE, "android hal vsi npu 1.2 alpha");
        return Void();
    };

    Return<void>
        getType(V1_2::IDevice::getType_cb _hidl_cb) override{
        _hidl_cb(ErrorStatus::NONE, V1_2::DeviceType::ACCELERATOR);
        return Void();
    };

    Return<void>
        getSupportedExtensions(V1_2::IDevice::getSupportedExtensions_cb _hidl_cb) override{
        _hidl_cb(ErrorStatus::NONE, {/* No extensions. */});
        return Void();
    };

    Return<void>
        getNumberOfCacheFilesNeeded(V1_2::IDevice::getNumberOfCacheFilesNeeded_cb _hidl_cb)override {
        // Set both numbers to be 0 for cache not supported.
        _hidl_cb(ErrorStatus::NONE, /*numModelCache=*/0, /*numDataCache=*/0);
        return Void();
    };
#endif

    Return<DeviceStatus> getStatus() override {
        VLOG(DRIVER) << "getStatus()";
        return DeviceStatus::AVAILABLE;
    }

    // Device driver entry_point
    virtual int run(){
        // TODO: Increase ThreadPool to 4 ?
        android::hardware::configureRpcThreadpool(1, true);
        if (registerAsService(name_) != android::OK) {
            LOG(ERROR) << "Could not register service";
            return 1;
        }
        android::hardware::joinRpcThreadpool();

        return 1;
    }

   protected:
    std::string name_;
   private:
        static void notify(const sp<V1_0::IPreparedModelCallback>& callback, const ErrorStatus& status,
                           const sp<VsiPreparedModel>& preparedModel) {
            callback->notify(status, preparedModel);
        }

#if ANDROID_SDK_VERSION >= 29
        static void notify(const sp<V1_2::IPreparedModelCallback>& callback, const ErrorStatus& status,
                           const sp<VsiPreparedModel>& preparedModel) {
            callback->notify_1_2(status, preparedModel);
        }
#endif

        template <typename T_IPreparedModelCallback>
        static void asyncPrepareModel(sp<VsiPreparedModel> vsiPreapareModel, const T_IPreparedModelCallback &callback){
            auto status = vsiPreapareModel->initialize();
            if( ErrorStatus::NONE != status){
                notify(callback, status, nullptr);
                return;
            }
            notify(callback, status, vsiPreapareModel);
        }

        template <typename T_Model, typename T_IPreparedModelCallback>
        Return<ErrorStatus> prepareModelBase(const T_Model& model,
                                         ExecutionPreference preference,
                                         const sp<T_IPreparedModelCallback>& callback){
            if (VLOG_IS_ON(DRIVER)) {
                VLOG(DRIVER) << "prepareModel";
                logModelToInfo(model);
            }
            if (callback.get() == nullptr) {
                LOG(ERROR) << "invalid callback passed to prepareModel";
                return ErrorStatus::INVALID_ARGUMENT;
            }
            if (!validateModel(model)) {
                LOG(ERROR) << "invalid hal model";
                notify(callback, ErrorStatus::INVALID_ARGUMENT, nullptr);
                return ErrorStatus::INVALID_ARGUMENT;
            }
            if( !validateExecutionPreference(preference)){
                LOG(ERROR) << "invalid preference" << static_cast<int32_t>(preference);
                notify(callback, ErrorStatus::INVALID_ARGUMENT, nullptr);
                return ErrorStatus::INVALID_ARGUMENT;
            }

            sp<VsiPreparedModel> preparedModel = new VsiPreparedModel( HalPlatform::convertVersion(model), preference);
            std::thread(asyncPrepareModel<sp<T_IPreparedModelCallback>>, preparedModel, callback).detach();

            return ErrorStatus::NONE;
        }
};
}
}
}
#endif
