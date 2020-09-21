#include "VsiDriver.h"
#include "VsiPlatform.h"

namespace android {
namespace nn {
namespace vsi_driver {

    Return<void> VsiDriver::getCapabilities_1_2(V1_2::IDevice::getCapabilities_1_2_cb _hidl_cb) {
        static const PerformanceInfo kPerf = {.execTime = 0.9f, .powerUsage = 0.9f};
        V1_2::Capabilities capabilities;
        capabilities.relaxedFloat32toFloat16PerformanceScalar = kPerf;
        capabilities.relaxedFloat32toFloat16PerformanceTensor = kPerf;
        // Set the base value for all operand types
        capabilities.operandPerformance = nonExtensionOperandPerformance({FLT_MAX, FLT_MAX});

        // Load supported operand types
        update(&capabilities.operandPerformance, OperandType::TENSOR_QUANT8_ASYMM, kPerf);
        if (!disable_float_feature_) {
            update(&capabilities.operandPerformance, OperandType::TENSOR_FLOAT32, kPerf);
            update(&capabilities.operandPerformance, OperandType::TENSOR_FLOAT16, kPerf);
        }
        _hidl_cb(ErrorStatus::NONE, capabilities);
        return Void();
    }

    Return<void> VsiDriver::getSupportedOperations_1_2(
        const V1_2::Model& model, V1_2::IDevice::getSupportedOperations_1_2_cb _hidl_cb) {
        if (!validateModel(model)) {
            LOG(ERROR) << __FUNCTION__;
            std::vector<bool> supported;
            _hidl_cb(ErrorStatus::INVALID_ARGUMENT, supported);
            return Void();
        }
        return getSupportedOperationsBase(model, _hidl_cb);
    }
}
}
}
