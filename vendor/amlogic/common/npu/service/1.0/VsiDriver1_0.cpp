#include "VsiDriver.h"
#include "VsiPlatform.h"

namespace android {
namespace nn {
namespace vsi_driver {
    Return<void> VsiDriver::getCapabilities(V1_0::IDevice::getCapabilities_cb cb) {
        V1_0::Capabilities capabilities;
        if (disable_float_feature_) {
            capabilities.float32Performance = {.execTime = 1.9f, .powerUsage = 1.9f};
            capabilities.quantized8Performance = {.execTime = 0.9f, .powerUsage = 0.9f};
        } else {
            capabilities.float32Performance = {.execTime = 0.9f, .powerUsage = 0.9f};
            capabilities.quantized8Performance = {.execTime = 0.9f, .powerUsage = 0.9f};
        }
        cb(ErrorStatus::NONE, capabilities);
        return Void();
    }

    Return<void> VsiDriver::getSupportedOperations(const V1_0::Model& model,
                                                   V1_0::IDevice::getSupportedOperations_cb cb) {
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
