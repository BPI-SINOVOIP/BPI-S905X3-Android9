#include "VsiDriver.h"
#include "VsiPlatform.h"

namespace android {
namespace nn {
namespace vsi_driver {

    Return<void> VsiDriver::getCapabilities_1_2(V1_2::IDevice::getCapabilities_1_2_cb _hidl_cb) {
        static const PerformanceInfo kPerf = {.execTime = 0.9f, .powerUsage = 0.9f};
        static const PerformanceInfo kPerf_float = {.execTime = 1.9f, .powerUsage = 1.9f};
        V1_2::Capabilities capabilities;
        capabilities.relaxedFloat32toFloat16PerformanceScalar = kPerf_float;
        capabilities.relaxedFloat32toFloat16PerformanceTensor = kPerf_float;
        // Set the base value for all operand types
#if ANDROID_NN_API >= 30
        capabilities.operandPerformance = nonExtensionOperandPerformance<HalVersion::V1_2>({FLT_MAX, FLT_MAX});
#else
        capabilities.operandPerformance = nonExtensionOperandPerformance({FLT_MAX, FLT_MAX});
#endif

        // Load supported operand types
        update(&capabilities.operandPerformance, OperandType::TENSOR_QUANT8_ASYMM, kPerf);
        update(&capabilities.operandPerformance, OperandType::TENSOR_BOOL8, kPerf);
        update(&capabilities.operandPerformance, OperandType::TENSOR_INT32, kPerf);
        if (!disable_float_feature_) {
            update(&capabilities.operandPerformance, OperandType::TENSOR_FLOAT32, kPerf_float);
            update(&capabilities.operandPerformance, OperandType::TENSOR_FLOAT16, kPerf_float);
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
