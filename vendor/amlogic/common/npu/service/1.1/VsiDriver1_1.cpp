#include "VsiDriver.h"
#include "VsiPlatform.h"

namespace android {
namespace nn {
namespace vsi_driver {

    Return<void> VsiDriver::getCapabilities_1_1(V1_1::IDevice::getCapabilities_1_1_cb cb) {
        V1_1::Capabilities capabilities;
        if (disable_float_feature_) {
            capabilities.float32Performance = {.execTime = 1.9f, .powerUsage = 1.9f};
            capabilities.quantized8Performance = {.execTime = 0.9f, .powerUsage = 0.9f};
            capabilities.relaxedFloat32toFloat16Performance = {.execTime = 1.5f, .powerUsage = 1.5f};
        } else {
            capabilities.float32Performance = {.execTime = 0.9f, .powerUsage = 0.9f};
            capabilities.quantized8Performance = {.execTime = 0.9f, .powerUsage = 0.9f};
            capabilities.relaxedFloat32toFloat16Performance = {.execTime = 0.5f, .powerUsage = 0.5f};
        }
        cb(ErrorStatus::NONE, capabilities);
        return Void();
    }

    Return<void> VsiDriver::getSupportedOperations_1_1(
        const V1_1::Model& model, V1_1::IDevice::getSupportedOperations_1_1_cb cb) {
        if (!validateModel(model)) {
            LOG(ERROR) << __FUNCTION__;
            std::vector<bool> supported;
            cb(ErrorStatus::INVALID_ARGUMENT, supported);
            return Void();
        }
        return getSupportedOperationsBase(HalPlatform::convertVersion(model), cb);
    }
}
}
}
