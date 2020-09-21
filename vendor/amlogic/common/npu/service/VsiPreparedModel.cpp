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

#include "VsiPreparedModel.h"
#include "VsiDriver.h"

#include <sys/mman.h>
#include <sys/system_properties.h>

#include "OperationsUtils.h"
#if ANDROID_SDK_VERSION >= 28
#include "ValidateHal.h"
#endif
#include "nnrt/event.hpp"
#include "nnrt/error.hpp"


namespace android {
namespace nn {
namespace vsi_driver {

class VsiDriver;
static Return<ErrorStatus> convertResultCodeToErrorStatus(int resultCode) {
    switch (resultCode) {
        case NNA_NO_ERROR:
            return ErrorStatus::NONE;

        case NNA_BAD_DATA:
        case NNA_UNEXPECTED_NULL:
            LOG(ERROR)<<"INVALID_ARGUMENT";
            return ErrorStatus::INVALID_ARGUMENT;

        case NNA_OUTPUT_INSUFFICIENT_SIZE:
            LOG(ERROR)<<"output insufficient size";
            return ErrorStatus::OUTPUT_INSUFFICIENT_SIZE;

        default:
            LOG(ERROR) << "Unknown result code " << resultCode
                       << " mapped to ErrorStatus::GENERAL_FAILURE";
            return ErrorStatus::GENERAL_FAILURE;
        case NNA_BAD_STATE:
        case NNA_INCOMPLETE:
        case NNA_OP_FAILED:
        case NNA_OUT_OF_MEMORY:
        case NNA_UNMAPPABLE:
            LOG(ERROR)<<"GENERAL_FAILURE";
            return ErrorStatus::GENERAL_FAILURE;
    }
}

    static nnrt::OperationType op_code_mapping(HalPlatform::OperationType op) { // Android O 8.1 API LEVEL 27
        switch (op)
        {
#define MAP_OP(op)  \
        case HalPlatform::OperationType::op:{ \
        LOG(INFO)<<"add operation: "<<#op; \
            return nnrt::OperationType::op;     \
        }
            MAP_OP(ADD);
            MAP_OP(CONV_2D);
            MAP_OP(DEPTHWISE_CONV_2D);
            MAP_OP(RELU);
            MAP_OP(RESHAPE);
            MAP_OP(FULLY_CONNECTED);
            MAP_OP(SOFTMAX);
            MAP_OP(CONCATENATION);
            MAP_OP(AVERAGE_POOL_2D);
            MAP_OP(MAX_POOL_2D);
            MAP_OP(MUL);
            MAP_OP(RELU1);
            MAP_OP(RELU6);
            MAP_OP(TANH);
            MAP_OP(LOGISTIC);
            MAP_OP(FLOOR);
            MAP_OP(DEQUANTIZE);
            MAP_OP(SPACE_TO_DEPTH);
            MAP_OP(DEPTH_TO_SPACE);
            MAP_OP(L2_NORMALIZATION);
            MAP_OP(RESIZE_BILINEAR);
            MAP_OP(LOCAL_RESPONSE_NORMALIZATION);
            MAP_OP(EMBEDDING_LOOKUP);
            MAP_OP(RNN);
            MAP_OP(HASHTABLE_LOOKUP);
            MAP_OP(LSTM);
            MAP_OP(SVDF);
            MAP_OP(LSH_PROJECTION);
            MAP_OP(L2_POOL_2D);
#if ANDROID_SDK_VERSION >= 28
            MAP_OP(BATCH_TO_SPACE_ND);
            MAP_OP(DIV);
            MAP_OP(MEAN);
            MAP_OP(PAD);
            MAP_OP(SPACE_TO_BATCH_ND);
            MAP_OP(SQUEEZE);
            MAP_OP(STRIDED_SLICE);
            MAP_OP(SUB);
            MAP_OP(TRANSPOSE);
#endif
#if ANDROID_SDK_VERSION >= 29
            MAP_OP(ABS);
            MAP_OP(RESIZE_NEAREST_NEIGHBOR);
            MAP_OP(ARGMAX);
            MAP_OP(ARGMIN);
            MAP_OP(MAXIMUM);
            MAP_OP(MINIMUM);
            MAP_OP(SQRT);
            MAP_OP(RSQRT);
            MAP_OP(LOG);
            MAP_OP(EXP);
            MAP_OP(SIN);
            MAP_OP(REDUCE_MAX);
            MAP_OP(REDUCE_MIN);
            MAP_OP(REDUCE_PROD);
            MAP_OP(REDUCE_SUM);
            MAP_OP(REDUCE_ALL);
            MAP_OP(REDUCE_ANY);
            MAP_OP(NEG);
            MAP_OP(PRELU);
            MAP_OP(UNIDIRECTIONAL_SEQUENCE_RNN);
            MAP_OP(BIDIRECTIONAL_SEQUENCE_RNN);
            MAP_OP(UNIDIRECTIONAL_SEQUENCE_LSTM);
            MAP_OP(BIDIRECTIONAL_SEQUENCE_LSTM);
            MAP_OP(GENERATE_PROPOSALS);
            MAP_OP(AXIS_ALIGNED_BBOX_TRANSFORM);
            MAP_OP(DETECTION_POSTPROCESSING);
            MAP_OP(LESS);
            MAP_OP(LESS_EQUAL);
            MAP_OP(EQUAL);
            MAP_OP(GREATER);
            MAP_OP(GREATER_EQUAL);
            MAP_OP(NOT_EQUAL);
            MAP_OP(LOGICAL_AND);
            MAP_OP(LOGICAL_OR);
            MAP_OP(LOGICAL_NOT);
            MAP_OP(EXPAND_DIMS);
            MAP_OP(POW);
            MAP_OP(INSTANCE_NORMALIZATION);
            MAP_OP(SPLIT);
            MAP_OP(LOG_SOFTMAX);
            MAP_OP(GATHER);
#endif
#undef MAP_OP

            default:
            LOG(ERROR)<<"Unknown operation code:"<< static_cast<int32_t>(op);
                break;
        }

        return nnrt::OperationType::NONE;
    };

    static nnrt::OperandType operand_mapping(HalPlatform::OperandType code)
    {
#define MAP_OPERAND(code) \
        case HalPlatform::OperandType::code:{\
            LOG(INFO)<<"add operand: "<<#code; \
            return nnrt::OperandType::code;\
            }

        switch (code) {
#if ANDROID_SDK_VERSION >= 29
            MAP_OPERAND(BOOL);
            MAP_OPERAND(FLOAT16);
            MAP_OPERAND(TENSOR_BOOL8);
            MAP_OPERAND(TENSOR_FLOAT16);
            MAP_OPERAND(TENSOR_QUANT8_SYMM);
            MAP_OPERAND(TENSOR_QUANT16_ASYMM);
            MAP_OPERAND(TENSOR_QUANT16_SYMM);
            MAP_OPERAND(TENSOR_QUANT8_SYMM_PER_CHANNEL);
#endif
            MAP_OPERAND(FLOAT32);
            MAP_OPERAND(INT32);
            MAP_OPERAND(UINT32);
            MAP_OPERAND(TENSOR_FLOAT32);
            MAP_OPERAND(TENSOR_INT32);
            MAP_OPERAND(TENSOR_QUANT8_ASYMM);
            default:
                break;
        }

#undef MAP_OPERAND

        return nnrt::OperandType::NONE;
    }

    void VsiPreparedModel::release_rtinfo(std::vector<VsiRTInfo>& rtInfos){
        while(!rtInfos.empty()){
            rtInfos.pop_back();
        }
    }
     Return<ErrorStatus> VsiPreparedModel::map_rtinfo_from_hidl_memory(const hidl_vec<hidl_memory>& pools,
                std::vector<VsiRTInfo>& rtInfos){
            rtInfos.clear();
            rtInfos.resize(pools.size());

        for(size_t i = 0; i < pools.size(); i++){
            if(!mapHidlMem(pools[i], rtInfos[i])){
                return ErrorStatus::INVALID_ARGUMENT;
            }
        }
        return ErrorStatus::NONE;
    }

    void VsiPreparedModel::fill_operand_value( nnrt::op::OperandPtr ovx_operand,
                                              const HalPlatform::Operand& hal_operand) {
        switch (hal_operand.lifetime) {
            case OperandLifeTime::MODEL_INPUT:
            case OperandLifeTime::MODEL_OUTPUT:
            case OperandLifeTime::TEMPORARY_VARIABLE:
                // Skip lifetime is TEMPORARY_VARIABLE, MODEL_INPUT, MODEL_OUTPUT, or NO_VALUE
                break;
            case OperandLifeTime::NO_VALUE: {
                native_model_->setOperandValue(ovx_operand, nullptr, 0);
            } break;
            case OperandLifeTime::CONSTANT_COPY: {
                const auto& location = hal_operand.location;
                native_model_->setOperandValue(ovx_operand,
                                model_.operandValues.data() + location.offset,
                                location.length);
            } break;
            case OperandLifeTime::CONSTANT_REFERENCE: {
                const auto& location = hal_operand.location;
                auto &rt_info = const_buffer_[location.poolIndex];

                if ("ashmem" == rt_info.mem_type ||
                    "hardware_buffer_blob" == rt_info.mem_type) {
                    const uint8_t* buffer = rt_info.ptr;
                    native_model_->setOperandValue(ovx_operand, buffer + location.offset, location.length);
                } else if ("mmap_fd" == rt_info.mem_type) {
                    native_model_->setOperandValueFromMemory(ovx_operand, rt_info.vsi_mem.get(),
                                                            location.offset, location.length);
                }
            } break;
        }
    }

    Return<ErrorStatus> VsiPreparedModel::construct_ovx_operand( nnrt::op::OperandPtr ovx_operand,
                                                                 const HalPlatform::Operand& hal_operand) {
        auto ovx_nnrt_type = operand_mapping(hal_operand.type);
        if(nnrt::OperandType::NONE == ovx_nnrt_type ){
            LOG(ERROR)<<" do not support operand type: "<<static_cast<int>(hal_operand.type);
            return ErrorStatus::INVALID_ARGUMENT;
        }
        ovx_operand->type = ovx_nnrt_type;
        ovx_operand->quant.scalar.scale = hal_operand.scale;
        ovx_operand->quant.scalar.zeroPoint = hal_operand.zeroPoint;
        ovx_operand->dimensions = hal_operand.dimensions;

        // TODO: add check error
        switch (ovx_operand->type) {
            case nnrt::OperandType::FLOAT32:
            case nnrt::OperandType::INT32:
            case nnrt::OperandType::UINT32:
            break;
            case nnrt::OperandType::TENSOR_FLOAT32:
            case nnrt::OperandType::TENSOR_INT32:
            case nnrt::OperandType::TENSOR_QUANT8_ASYMM: {
                break;
            }
            default:
                break;
        }
        return ErrorStatus::NONE;
    }

    Return<ErrorStatus> VsiPreparedModel::initialize(){
        // [0] validate HAL::Model, return ErrorCode if validate failed
        // For scalar operand, dimension must be 0
        // [1] create async procedure to prepare model
        // [1.0] convert HAL model to nnrt::Model
        LOG(INFO) << __FUNCTION__;

        auto status = map_rtinfo_from_hidl_memory(model_.pools, const_buffer_);
        if(ErrorStatus::NONE != status)
            return status;

        // add operand and set its value
        for (const auto& hal_operand : model_.operands) {
            uint32_t registered_idx = 0;
            auto ovx_operand = native_model_->addOperand(nullptr, &registered_idx);
            auto status = construct_ovx_operand(ovx_operand, hal_operand);
            if (ErrorStatus::NONE != status){
                LOG(INFO)<<"Deivce do not support the operand type"<< static_cast<int32_t>(hal_operand.type);
                return status;
            }
            fill_operand_value(ovx_operand, hal_operand);
        }

        for (const auto& hal_op : model_.operations) {
            if(!VsiDriver::isSupportedOperation(hal_op, model_)){
                LOG(ERROR)<<"Device do not support operation type: "<<static_cast<int>(hal_op.type);
                return ErrorStatus::INVALID_ARGUMENT;
                }

            auto ovx_op_type = op_code_mapping(hal_op.type);
            if( nnrt::OperationType::NONE == ovx_op_type){
                LOG(ERROR)<<"Device do not support operation type: "<<static_cast<int>(hal_op.type);
                return ErrorStatus::INVALID_ARGUMENT;
            }

             auto ovx_op =
                native_model_->addOperation(ovx_op_type/* Operation Type*/,
                                           &hal_op.inputs[0],    /*inputs */
                                           hal_op.inputs.size(), /*num of inputs */
                                           &hal_op.outputs[0],   /*outputs */
                                           hal_op.outputs.size() /*num of outputs */
                                           );

            ovx_op->setDataLayout(nnrt::DataLayout::NHWC);
        }

        if(model_.relaxComputationFloat32toFloat16)
            native_model_->relax(true); // convert fp32 data to fp16 in nnrt.

        native_model_->finish();
        std::vector<uint32_t> inputs = model_.inputIndexes;
        std::vector<uint32_t> outputs = model_.outputIndexes;
        native_model_->identifyInputsAndOutputs(inputs.data(), inputs.size(), outputs.data(), outputs.size());
        return ErrorStatus::NONE;
    }

    void VsiPreparedModel::update_operand_from_request( const std::vector<uint32_t>& indexes,
                                                                        const hidl_vec<RequestArgument>& arguments){
        nnAssert(indexes.size() == arguments.size());
        auto ovx_operands = native_model_->getOperands(indexes);
        for (size_t i = 0; i < indexes.size(); i++) {
            const RequestArgument& request_arg = arguments[i];
            auto& ovx_operand = ovx_operands[i];
            if (request_arg.dimensions.size() > 0) {
                // It's the responsibility of the caller to validate that
                // arg.dimensions only modifies the dimensions that were
                // unspecified in the model.  That's the case in SampleDriver.cpp
                // with the call to validateRequest().
                // TODO: after revaluing the dimension, model should be re-compiled
                ovx_operand->dimensions = request_arg.dimensions;
            }
        }
    }

    Return<ErrorStatus> VsiPreparedModel::update_pool_info_from_request(const Request& request,
                                                                        const std::vector<uint32_t>& indexes,
                                                                        const hidl_vec<RequestArgument>& arguments,
                                                                        IO flag,
                                                                        std::vector<OutputShape> &outputShapes){
        nnAssert(indexes.size() == arguments.size());
        auto ovx_operands = native_model_->getOperands(indexes);
        for (size_t i = 0; i < indexes.size(); i++) {
            const RequestArgument& request_arg = arguments[i];
            auto ovx_operand = ovx_operands[i];
            if (request_arg.hasNoValue) {
                if(flag == IO::INPUT)
                    native_exec_->setInput(i, nullptr /*operand */, nullptr, 0);
                else
                    native_exec_->setOutput(i, nullptr, nullptr, 0);
            }
            else {
                auto location    = request_arg.location;
                auto poolIndex = location.poolIndex;
                nnAssert(poolIndex < request.pools.size());

                if (flag == IO::OUTPUT){
                    outputShapes[i].dimensions = ovx_operand->dimensions;
                    outputShapes[i].isSufficient =
                                        io_buffer_[poolIndex].buffer_size >= location.length + location.offset;
                    if (io_buffer_[poolIndex].buffer_size < location.length + location.offset)
                    {
                        LOG(ERROR)<<"No Sufficient output, Execute failed";
                        return ErrorStatus::OUTPUT_INSUFFICIENT_SIZE;
                    }
                }

                auto &rt_info = io_buffer_[poolIndex];
                int nn_return = NNA_NO_ERROR;
                if("ashmem" == rt_info.mem_type){
                    uint8_t* buffer = rt_info.ptr;
                    if (flag == IO::INPUT)
                        nn_return = native_exec_->setInput(i, ovx_operand, buffer + location.offset, location.length);
                    else{
                        nn_return = native_exec_->setOutput(i, ovx_operand, buffer + location.offset, location.length);
                    }
                }else if ("mmap_fd" == rt_info.mem_type) {
                    auto &vsi_mem = rt_info.vsi_mem;
                    if (flag == IO::INPUT)
                        nn_return = native_exec_->setInputFromMemory(
                            i, ovx_operand, vsi_mem.get(), location.offset, location.length);
                    else
                        nn_return = native_exec_->setOutputFromMemory(
                            i, ovx_operand, vsi_mem.get(), location.offset, location.length);
                }

                auto status = convertResultCodeToErrorStatus(nn_return);
                if(status != ErrorStatus::NONE){
                    LOG(ERROR)<<"fail to set IO, return error code :";
                    return status;
                }
             }
        }

        return ErrorStatus::NONE;
    }

    template <typename T_IExecutionCallback>
    Return<ErrorStatus> VsiPreparedModel::executeBase(const Request& request,
                                              const T_IExecutionCallback& callback){
        LOG(INFO) << __FUNCTION__;

        if(!validateRequest(request, model_)){
            LOG(ERROR)<<"invalid request";
            callback->notify(ErrorStatus::INVALID_ARGUMENT);
            return ErrorStatus::INVALID_ARGUMENT;
        }
        if(nullptr == native_compile_)
            native_compile_ = std::make_shared<nnrt::Compilation>(native_model_.get());
        map_rtinfo_from_hidl_memory(request.pools, io_buffer_);
        if(!native_exec_)
            native_exec_ = std::make_shared<nnrt::Execution>(native_compile_.get());

        std::vector<hidl_memory> io_pools = request.pools;
        std::vector<RequestArgument> input_args = request.inputs;
        std::vector<RequestArgument> output_args = request.outputs;
        std::vector<OutputShape> outputShapes(request.outputs.size());

        update_operand_from_request(model_.inputIndexes, input_args);
        update_operand_from_request(model_.outputIndexes, output_args);
        if (!native_model_->isCompiled()) {
            native_compile_->run();
        }

        auto status = update_pool_info_from_request(request, model_.inputIndexes, input_args, IO::INPUT, outputShapes);
        if( ErrorStatus::NONE != status){
            callback->notify(status);
            return status;
        }

        status =  update_pool_info_from_request(request, model_.outputIndexes, output_args, IO::OUTPUT, outputShapes);
        if( ErrorStatus::NONE != status){
            callback->notify(status);
            return status;
        }

        int error = native_exec_->compute();
        status = convertResultCodeToErrorStatus(error);
        native_exec_.reset();
        release_rtinfo(io_buffer_);

        if(status != ErrorStatus::NONE){
            callback->notify(ErrorStatus::INVALID_ARGUMENT);
            return status;
        }

        callback->notify(ErrorStatus::NONE);
        return ErrorStatus::NONE;
    }
    Return<ErrorStatus> VsiPreparedModel::execute(const Request& request,
                                const sp<V1_0::IExecutionCallback>& callback) {
        return executeBase(request, callback);
    };
}
}
}
