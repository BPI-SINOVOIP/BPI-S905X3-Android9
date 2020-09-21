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

#include "CompilationBuilder.h"
#include "ExecutionPlan.h"
#include "GraphDump.h"
#include "HalInterfaces.h"
#include "Manager.h"
#include "ModelBuilder.h"
#include "NeuralNetworks.h"
#include "NeuralNetworksOEM.h"
#include "NeuralNetworksWrapper.h"
#include "SampleDriver.h"
#include "Utils.h"
#include "ValidateHal.h"

#include <gtest/gtest.h>

#include <map>
#include <queue>

// Uncomment the following line to generate some debugging output that
// may be useful when analyzing failures:
//
// #define VERBOSE VERBOSE

// Uncomment the following line to generate DOT graphs.
//
// #define GRAPH GRAPH

// These tests do whitebox testing of the graph partitioning
// algorithm.  It is "whitebox" in the sense that we're not evaluating
// whether a particular partitioning is legal, or "good enough"
// according to some metric, but whether it exactly matches the
// expected behavior of the current partitioning algorithm.
//
// A key part of the current partitioning algorithm is to determine
// which device among the available devices should be the one to
// execute a particular operation from the graph.  This determination
// is made "locally" -- i.e., it does not depend on the graph
// topology, only on the properties of the operation in question.
// IDevice::getSupportedOperations() indicates which operations in a
// graph can be executed on a device, and IDevice::getCapabilities()
// indicates how "good" that device is for executing particular kinds
// of operations.  For each operation, the partitioning algorithm
// picks the "best" device that is capable of executing that
// operation; if no device can do so, then the algorithm picks the
// cpu.
//
// As part of this testing approach, we want to make it easy to
// specify which operations in a test graph can be executed on which
// devices.  We accomplish this with an abstraction: There are eight
// different kinds of operations (each of which has two inputs and one
// output), and when we instantiate a device for testing purposes, we
// specify what subset of those eight kinds of operations the device
// is able to execute.
//
// The eight kinds of operations are represented in the graph as ADD
// or MUL with a particular activation function -- two opcodes times
// four activation functions means eight available operation kinds.
// This is a low-level representation detail -- when we specify the
// behavior of the device or build a graph, we do so in terms of
// operation encodings 0..7.
//
// In order to determine whether or not a partitioning matches the
// expected partitioning, we check the number of partitions, check
// which device each partition targets, and compare each partition's
// subgraph, model inputs, model outputs, submodel inputs, and
// submodel outputs against what is expected.  In order to perform
// that comparison, we build a model to compare against a partition's
// submodel and run a graph comparison algorithm on it.  The graph
// comparison and the inputs and outputs comparisons are syntactic
// rather than semantic comparisons -- they don't allow for
// reorderings of inputs and outputs.  Because of this, we need to
// know exactly how the partitioning algorithm orders inputs and
// outputs in order to construct the models and operand lists to
// compare against.  Here are some relevant behaviors of the
// partitioning algorithm:
//
// - It builds a subgraph by walking operations in forward topological
//   order, and adding each operation's input operands and output
//   operands in index order (input followed by output) when that
//   operation is added.  (It does not add an input that has already
//   been added.)
// - It finds model inputs, model outputs, and submodel inputs in
//   the order the corresponding operands were added to the subgraph
//   (see ExecutionStep methods getModelInputs(), getModelOutputs(),
//   getTempsAsSubModelInputs(), getOutputsAsSubModelInputs()).
// - It finds temps as submodel outputs in numerical order of corresponding
//   operand number in the original model (see ExecutionStep method
//   getTempsAsSubModelOutputs()).
// - When it calls identifyInputsAndOutputs() on the submodel, it
//   passes inputs from getModelInputs() in order, followed by temps as
//   submodel inputs from getTempsAsSubModelInputs() in order,
//   followed by outputs as submodel inputs from
//   getOutputsAsSubModelInputs() in order; and it passes outputs from
//   getModelOutputs() in order followed by submodel outputs from
//   getTempsAsSubModelOutputs() in order.
//
// TODO: Maybe the logic for comparing a partition to an expected
//       model should be changed to tolerate reorderings of inputs and
//       outputs, so that when we build models and lists to compare
//       against, we don't need to worry about input and output
//       orderings.  But is there a way to do this that still lets us
//       verify that we have the correct relationships between
//       an (original) model's inputs and outputs and each submodel's
//       inputs and outputs, as well as the correct relationship
//       between submodel inputs and outputs across partitions?

namespace {

using CompilationBuilder = ::android::nn::CompilationBuilder;
using Device = ::android::nn::Device;
using DeviceManager = ::android::nn::DeviceManager;
using ExecutePreference = ::android::nn::wrapper::ExecutePreference;
using ExecutionPlan = ::android::nn::ExecutionPlan;
using ExecutionStep = ::android::nn::ExecutionStep;
using HidlModel = ::android::hardware::neuralnetworks::V1_1::Model;
using ModelBuilder = ::android::nn::ModelBuilder;
using Result = ::android::nn::wrapper::Result;
using SampleDriver = ::android::nn::sample_driver::SampleDriver;
using WrapperCompilation = ::android::nn::wrapper::Compilation;
using WrapperModel = ::android::nn::wrapper::Model;
using WrapperOperandType = ::android::nn::wrapper::OperandType;
using WrapperType = ::android::nn::wrapper::Type;

template <typename T> using sp = ::android::sp<T>;

// We employ an operation numbering scheme:
// - 0..FuseCode-1 = ADD with the appropriate activation function
// - FuseCode..2*FuseCode-1 = MUL with the appropriate activation function
const uint32_t kNumFuseCodes = 4;
const uint32_t kBadOperation = ~0;

// Look up the operation with the specified index in a graph, and
// return the operation encoding -- 0..7; or, if for some reason this
// is not one of the encoded operations, then return kBadOperation.
uint32_t lookupOperation(std::function<const Operation&(uint32_t)> getOperation,
                         std::function<const Operand&(uint32_t)> getOperand,
                         std::function<const uint8_t*(uint32_t)> getValue,
                         uint32_t operationIndex) {
    const Operation& operation = getOperation(operationIndex);
    switch (operation.type) {
        case OperationType::ADD:
        case OperationType::MUL: {
            // input2 is the fused activation function
            const Operand& input2 = getOperand(operation.inputs[2]);
            if ((input2.type == OperandType::INT32) &&
                (input2.lifetime == OperandLifeTime::CONSTANT_COPY)) {
                int32_t value;
                memcpy(&value,
                       getValue(input2.location.offset),
                       input2.location.length);
                if (operation.type == OperationType::MUL) {
                    value += kNumFuseCodes;
                }
                return value;
            }
            break;
        }
        default:
            break;
    }
    return kBadOperation;
}

uint32_t lookupOperation(const HidlModel& model, uint32_t operationIndex) {
    return lookupOperation(
        [&model](uint32_t index) -> const Operation& {
            return model.operations[index];
        },
        [&model](uint32_t index) -> const Operand& {
            return model.operands[index];
        },
        [&model](uint32_t offset) {return &model.operandValues[offset];},
        operationIndex);
}

#ifdef VERBOSE
// This is a debugging utility function
void dump(const char* name, const ModelBuilder* model) {
    HidlModel hidlModel;
    model->setHidlModel(&hidlModel);
    std::cout << name << ": " << toString(hidlModel) << std::endl;
    std::cout << "inputs: " << toString(hidlModel.inputIndexes) << std::endl;
    std::cout << "outputs: " << toString(hidlModel.outputIndexes) << std::endl;
    for (size_t i = 0, e = hidlModel.operations.size(); i < e; i++) {
        std::cout << "operation[" << i << "]: " << toString(hidlModel.operations[i]) << std::endl;
    }
}
#endif

#ifdef GRAPH
inline void hidlGraphDump(const char* name, const HidlModel& model) {
    ::android::nn::graphDump(name, model);
}
#endif

void graphDump([[maybe_unused]] const char* name, [[maybe_unused]] const WrapperModel& model) {
#ifdef GRAPH
    HidlModel hidlModel;
    reinterpret_cast<const ModelBuilder*>(model.getHandle())->setHidlModel(&hidlModel);
    hidlGraphDump(name, hidlModel);
#endif
}

// This is an IDevice for testing purposes.  It only has a few
// interesting properties, all of which are specified as constructor
// arguments: device capabilities; which subset of operation kinds
// (0..7) does the device support; does the device support the OEM
// operation.  The subset is represented with a bitmask, in which
// operation kind K corresponds to the bit (1 << K).
class PartitioningDriver : public SampleDriver {
private:
    // Dummy class -- a prepared model must not be nullptr.
    class PartitioningPreparedModel : public IPreparedModel {
    public:
        Return<ErrorStatus> execute(const Request&,
                                    const sp<IExecutionCallback>&) override {
            return ErrorStatus::DEVICE_UNAVAILABLE;
        }
    };
public:
    enum OEM { OEMNo, OEMYes };

    PartitioningDriver(const char *name, Capabilities capabilities,
                       uint32_t operationMask, OEM oem = OEMNo) :
            SampleDriver(name), mCapabilities(capabilities),
            mOperationMask(operationMask), mOEM(oem) {}
    ~PartitioningDriver() override {}

    Return<ErrorStatus> prepareModel_1_1(const Model&, ExecutionPreference,
                                         const sp<IPreparedModelCallback>& cb) override {
        cb->notify(ErrorStatus::NONE, new PartitioningPreparedModel);
        return ErrorStatus::NONE;
    }

    Return<DeviceStatus> getStatus() override {
        return DeviceStatus::AVAILABLE;
    }

    Return<void> getCapabilities_1_1(getCapabilities_1_1_cb cb) override {
        cb(ErrorStatus::NONE, mCapabilities);
        return Void();
    }

    Return<void> getSupportedOperations_1_1(const Model& model,
                                            getSupportedOperations_cb cb) override {
        if (!android::nn::validateModel(model)) {
            cb(ErrorStatus::INVALID_ARGUMENT, std::vector<bool>());
            return Void();
        }

        const size_t count = model.operations.size();
        std::vector<bool> supported(count);
        for (size_t i = 0; i < count; i++) {
            if (model.operations[i].type == OperationType::OEM_OPERATION) {
                supported[i] = (mOEM == OEMYes);
                continue;
            }
            supported[i] = false;
            uint32_t operation = lookupOperation(model, i);
            if ((operation != kBadOperation) && (mOperationMask & (1 << operation))) {
                supported[i] = true;
            }
        }
        cb(ErrorStatus::NONE, supported);
        return Void();
    }

private:
    Capabilities mCapabilities;
    uint32_t mOperationMask;
    OEM mOEM;
};

// This class adds some simple abstractions and utilities on top of
// ::android::nn::wrapper::Model.  For example, it provides methods
// that work in terms of operation kind (0..7); and because we care
// about graph topology rather than details of operand types and
// values, it greatly simplifies the process of creating operands.
class PartitioningModel : public WrapperModel {
public:
    // Create a tensor operand of the specified type, and return the
    // corresponding operand index.
    uint32_t addFloatOperand() {
        static const WrapperOperandType type(WrapperType::TENSOR_FLOAT32, { 1 });
        return addOperand(&type);
    }
    uint32_t addQuantOperand() {
        static const WrapperOperandType type(WrapperType::TENSOR_QUANT8_ASYMM, { 1 });
        return addOperand(&type);
    }

    // Create an operation with two inputs and one output, specifying
    // the operation kind (0..7) and the input operand indexes.
    // Returns the output operand index.
    enum class Dimensioned { NO, YES };
    uint32_t addOperation2To1(uint32_t operation, const uint32_t input0, const uint32_t input1,
                              Dimensioned dimensionedOutput = Dimensioned::YES) {
        ANeuralNetworksOperationType type =
                (operation < kNumFuseCodes ? ANEURALNETWORKS_ADD : ANEURALNETWORKS_MUL);
        int32_t fuseCode = (operation < kNumFuseCodes ? operation : operation - kNumFuseCodes);
        uint32_t input2 = addIntOperand(fuseCode);
        uint32_t output = addOperandOfSameType(input0, dimensionedOutput);
        addOperation(type, { input0, input1, input2 }, { output });
        return output;
    }

    // Create an OEM operation with one input and one output,
    // specifying the input operand index.  Returns the output operand
    // index.
    uint32_t addOperationOEM1To1(const uint32_t input,
                                 Dimensioned dimensionedOutput = Dimensioned::YES) {
        uint32_t output = addOperandOfSameType(input, dimensionedOutput);
        addOperation(ANEURALNETWORKS_OEM_OPERATION, { input }, { output });
        return output;
    }

    // Run the partitioning algorithm to create an ExecutionPlan.
    int partitionTheWork(const std::vector<std::shared_ptr<Device>>& devices,
                         ExecutePreference preference, ExecutionPlan* plan) {
        return reinterpret_cast<ModelBuilder*>(getHandle())->partitionTheWork(
            devices, static_cast<uint32_t>(preference), plan);
    }

#ifdef VERBOSE
    // This is a debugging utility function.
    void dump(const char* name) const {
        const ModelBuilder* mb = reinterpret_cast<const ModelBuilder*>(getHandle());
        ::dump(name, mb);
    }
#endif

private:

    // Create a scalar integer operand of the specified value, and
    // return the corresponding operand index.
    uint32_t addIntOperand(int32_t value) {
        static const WrapperOperandType type(WrapperType::INT32, { });
        uint32_t operand = addOperand(&type);
        setOperandValue(operand, &value, sizeof(value));
        return operand;
    }

    // Create an operand of the same type as the specified operand,
    // and return the operand index of the new operand.
    uint32_t addOperandOfSameType(uint32_t operand, Dimensioned dimensioned = Dimensioned::YES) {
        const Operand& operandStruct =
                reinterpret_cast<const ModelBuilder*>(getHandle())->getOperand(operand);
        WrapperOperandType type(static_cast<WrapperType>(operandStruct.type), { 1 });
        if (dimensioned == Dimensioned::NO) {
            for (auto& dimension : type.dimensions) {
                dimension = 0;
            }
        }
        return addOperand(&type);
    }
};

// This class adds some utilities on top of ::android::nn::wrapper::Compilation.
class PartitioningCompilation : public WrapperCompilation {
public:
    PartitioningCompilation(const WrapperModel* model) : WrapperCompilation(model) { }

    Result setPartitioning(uint32_t partitioning) {
        return static_cast<Result>(builder()->setPartitioning(partitioning));
    }

    using WrapperCompilation::finish;
    Result finish(const std::vector<std::shared_ptr<Device>>& devices) {
        return static_cast<Result>(builder()->finish(devices));
    }

    const ExecutionPlan& getExecutionPlan() const {
        return builder()->forTest_getExecutionPlan();
    }

private:
    CompilationBuilder* builder() {
        return reinterpret_cast<CompilationBuilder*>(getHandle());
    }

    const CompilationBuilder* builder() const {
        return reinterpret_cast<const CompilationBuilder*>(getHandle());
    }
};

#ifdef VERBOSE
#define RETURN_TRUE()                                                          \
    {                                                                          \
        std::cerr << "returning true from " << __LINE__ << std::endl;          \
        return true;                                                           \
    }
#else
#define RETURN_TRUE()                                                          \
    {                                                                          \
        return true;                                                           \
    }
#endif
#ifdef VERBOSE
#define RETURN_FALSE(MESSAGE)                                                  \
    {                                                                          \
        std::cerr << "returning false from " << __LINE__ MESSAGE << std::endl; \
        return false;                                                          \
    }
#else
#define RETURN_FALSE(MESSAGE)                                                  \
    {                                                                          \
        return false;                                                          \
    }
#endif

class PartitioningTest : public ::testing::Test {
protected:
    using RemapVectorType = ExecutionStep::RemapVectorType;
    using SubModelOutputSetType = ExecutionStep::SubModelOutputSetType;

    virtual void SetUp() {
    }

    // From a vector of DeviceSpecification, create a vector of
    // Devices.
    struct DeviceSpecification {
        DeviceSpecification(const std::string &name, Capabilities capabilities,
                            uint32_t operationMask,
                            PartitioningDriver::OEM oem = PartitioningDriver::OEMNo) :
                mName(name), mCapabilities(capabilities),
                mOperationMask(operationMask), mOEM(oem) { }
        std::string mName;
        Capabilities mCapabilities;
        uint32_t mOperationMask;
        PartitioningDriver::OEM mOEM;
    };
    static std::vector<std::shared_ptr<Device>>
    makeDevices(std::vector<DeviceSpecification> specifications) {
        std::vector<std::shared_ptr<Device>> devices;
        for (const auto& specification : specifications) {
            devices.push_back(std::make_shared<Device>(
                specification.mName,
                new PartitioningDriver(specification.mName.c_str(),
                                       specification.mCapabilities,
                                       specification.mOperationMask,
                                       specification.mOEM)));
            if (!devices.back()->initialize()) {
                EXPECT_NE("failed to initialize device", nullptr);
                return {};
            }
        }
        return devices;
    }

    /*-- Graph comparision ----------------------------------------------------------------*/

    // An operand with certain values for its lifetime does not have a
    // defining operation in the graph.  For the purposes of the graph
    // comparison algorithm, we encode the "defining operation" index of
    // such an operand as follows:
    // - NO_VALUE       kPseudoDefiningOperationNoValue
    // - MODEL_INPUT    kPseudoDefiningOperationModelInput0 + (position in list of inputs)
    // - CONSTANT_COPY  kPseudoDefiningOperationConstantCopy0 + (constant value)
    //                    Note: For the graphs we build in this test, we
    //                          only expect to see 4-byte constants within
    //                          a very restricted range, so we only make
    //                          room for such constants in our encoding
    //                          space.
    // We do not expect to see CONSTANT_REFERENCE, and so we do not handle
    // it.
    //
    // The encoding is intended to be relatively human readable; it is not
    // designed to represent some optimal balance of ranges for the items
    // within its scope (actual operations, inputs, constants).

    enum PseudoDefiningOperationEncodings : uint32_t {
        kPseudoDefiningOperationModelInput0   = 0x80000000U,
        kPseudoDefiningOperationConstantCopy0 = 0x90000000U,
        kPseudoDefiningOperationNoValue       = 0xeeeeeeeeU,

        // lowest value for special encoding
        kPseudoDefiningOperationBase          = 0x80000000U,

        // range of encoded input or constant
        kPseudoDefiningOperationRange         = 0x10000000U,
    };

    // Build a map from operand to defining operation.
    // TODO: Replace map with vector?
    void buildDefinitionMap(const ModelBuilder* model,
                            std::map<uint32_t, uint32_t>* defMap) {
        // actual definitions
        ASSERT_LT(model->operationCount(), kPseudoDefiningOperationBase);
        for (uint32_t i = 0, e = model->operationCount(); i < e; i++) {
            const Operation& operation = model->getOperation(i);
            for (uint32_t output : operation.outputs) {
                (*defMap)[output] = i;
            }
        }
        // inputs
        ASSERT_LT(model->inputCount(), kPseudoDefiningOperationRange);
        for (uint32_t i = 0, e = model->inputCount(); i < e; i++) {
            (*defMap)[model->getInputOperandIndex(i)] = kPseudoDefiningOperationModelInput0 + i;
        }
        // look for NO_VALUE and CONSTANT_COPY
        for (uint32_t i = 0, e = model->operandCount(); i < e; i++) {
            const Operand& operand = model->getOperand(i);
            switch (operand.lifetime) {
                case OperandLifeTime::NO_VALUE:
                    (*defMap)[i] = kPseudoDefiningOperationNoValue;
                    break;
                case OperandLifeTime::CONSTANT_COPY: {
                    ASSERT_EQ(operand.location.length, sizeof(uint32_t));
                    uint32_t value;
                    memcpy(&value, model->getPointerToOperandValue(operand.location.offset), sizeof(uint32_t));
                    ASSERT_LT(value, kPseudoDefiningOperationNoValue);
                    (*defMap)[i] = kPseudoDefiningOperationConstantCopy0 + value;
                    break;
                }
                case OperandLifeTime::TEMPORARY_VARIABLE:
                case OperandLifeTime::MODEL_INPUT:
                case OperandLifeTime::MODEL_OUTPUT:
                    // already handled
                    break;
                default:
                    FAIL();
                    break;
            }
        }
        // sanity check
        ASSERT_EQ(model->operandCount(), defMap->size());
    }

#ifdef VERBOSE
    void dump(const char* name, const std::map<uint32_t, uint32_t>* aMap) {
        auto writeNum = [](uint32_t num) {
            if (num >= kPseudoDefiningOperationBase) {
                std::cout << "0x" << std::hex << num << std::dec;
            } else {
                std::cout << num;
            }
        };

        std::cout << name << ": { ";
        bool gotOne = false;
        for (const auto& entry : *aMap) {
            if (gotOne) {
                std::cout << ", ";
            } else {
                gotOne = true;
            }
            std::cout << "(";
            writeNum(entry.first);
            std::cout << ", ";
            writeNum(entry.second);
            std::cout << ")";
        }
        std::cout << " }" << std::endl;
    }
#endif

    bool compare(const Operand& operandA, const Operand& operandB) {
        if (operandA.type != operandB.type ||
            operandA.dimensions != operandB.dimensions ||
            operandA.numberOfConsumers != operandB.numberOfConsumers ||
            operandA.scale != operandB.scale ||
            operandA.zeroPoint != operandB.zeroPoint) {
            return false;
        }
        return true;
    }

    // Compare two graphs.  We ignore operand and operation indexes (i.e.,
    // two nodes can be the same even if they are numbered differently)
    // but we also ignore semantics (e.g., even if an operation kind is
    // such that the operand is commutative, we still pay attention to the
    // order of its input operands).
    //
    // The comparison algorithm works by walking modelA from outputs
    // towards inputs, along the edge from each operand to its
    // defining operation, and then along the edges to the operation's
    // input operands.  At each step along the way, we try to match up
    // operands and operations from modelA with equivalent operands
    // and operations from modelB.
    //
    // We start by assuming that modelA's outputs and modelB's outputs
    // match positionally (e.g., modelA's first output operand is
    // equivalent to modelB's first output operand).  Once we've
    // discovered two equivalent operands (such as those outputs), we
    // place them in a work queue.  We repeatedly pull operands off
    // the queue and compare their defining operations and those
    // operations' input operands, to discover more pairs of
    // equivalent operands.  If we ever find operations that do not
    // match (e.g., because operation kind differs), or operands that
    // do not match (e.g., because operand type differs); or if we
    // ever find a conflict (we've already decided that operand A's
    // equivalent operand is B0, but it looks like we need its
    // equivalent operand to be B1); then the graphs compare unequal.
    // Otherwise, we'll eventually exhaust the work queue, and
    // conclude that the graphs compare equal.
    bool compare(const ModelBuilder* modelA, const ModelBuilder* modelB) {
#ifdef VERBOSE
        ::dump("compare(A)", modelA);
        ::dump("compare(B)", modelB);
#endif

        if (modelA->operandCount()   != modelB->operandCount()   ||
            modelA->operationCount() != modelB->operationCount() ||
            modelA->inputCount()     != modelB->inputCount()     ||
            modelA->outputCount()    != modelB->outputCount()) {
            RETURN_FALSE();
        }

        // Maps from operand index to index of defining operation.
        std::map<uint32_t, uint32_t> defsA, defsB;
        buildDefinitionMap(modelA, &defsA);
        buildDefinitionMap(modelB, &defsB);
        if (HasFatalFailure()) return false;

        // Maps from operand index in modelA to equivalent operand index
        // in modelB; and from operation index in modelA to equivalent
        // operation index in modelB.
        std::map<uint32_t, uint32_t> equivalentOperandsAToB;
        std::map<uint32_t, uint32_t> equivalentOperationsAToB;

        // Queue of operand indexes from modelA, each of whose defining
        // operations are to be checked for equivalence with modelB.
        std::queue<uint32_t> workQueueOperandsA;

        // Seed operand equivalence map and work queue from model outputs.
        for (uint32_t i = 0, e = modelA->outputCount(); i < e; i++) {
            uint32_t outputA = modelA->getOutputOperandIndex(i);
            uint32_t outputB = modelB->getOutputOperandIndex(i);
            if (!compare(modelA->getOperand(outputA), modelB->getOperand(outputB))) {
                RETURN_FALSE();
            }
            equivalentOperandsAToB[outputA] = outputB;
            workQueueOperandsA.push(outputA);
        }

#ifdef VERBOSE
        dump("defsA", &defsA);
        dump("defsB", &defsB);
#endif

        // Process the queue.
        uint32_t pseudoDefinitionCount = 0;
        while (!workQueueOperandsA.empty()) {
#ifdef VERBOSE
            dump("equivalentOperandsAToB", &equivalentOperandsAToB);
            dump("equivalentOperationsAToB", &equivalentOperationsAToB);
#endif
            uint32_t operandIndexA = workQueueOperandsA.front();
#ifdef VERBOSE
            std::cout << "operandIndexA: " << operandIndexA << std::endl;
#endif
            workQueueOperandsA.pop();
            uint32_t operandIndexB = equivalentOperandsAToB.at(operandIndexA);

            uint32_t operationIndexA = defsA.at(operandIndexA);
            uint32_t operationIndexB = defsB.at(operandIndexB);
            auto it = equivalentOperationsAToB.find(operationIndexA);
            if (it != equivalentOperationsAToB.end()) {
                if (it->second != operationIndexB) {
                    RETURN_FALSE();
                }
                continue;
            }

            // We haven't identified an equivalent operation for
            // operationIndexA.

            if ((operationIndexA >= kPseudoDefiningOperationBase) !=
                (operationIndexB >= kPseudoDefiningOperationBase)) {
                RETURN_FALSE();
            }
            // Either both operands have pseudo-definitions, or neither
            // does.
            if (operationIndexA >= kPseudoDefiningOperationBase) {
                // Both operands have pseudo-definitions.
                if (operationIndexA != operationIndexB) {
                    RETURN_FALSE();
                }
                equivalentOperationsAToB[operationIndexA] = operationIndexB;
                ++pseudoDefinitionCount;
                continue;
            }

            // If we get here, neither operation A nor operation B is a
            // pseudo-definition.

            const Operation& operationA = modelA->getOperation(operationIndexA);
            const Operation& operationB = modelB->getOperation(operationIndexB);
            if (operationA.type != operationB.type ||
                operationA.inputs.size() != operationB.inputs.size() ||
                operationA.outputs.size() != operationB.outputs.size()) {
                RETURN_FALSE();
            }
            equivalentOperationsAToB[operationIndexA] = operationIndexB;
            for (uint32_t i = 0, e = operationA.inputs.size(); i < e; i++) {
                uint32_t inputA = operationA.inputs[i];
                uint32_t inputB = operationB.inputs[i];
                auto it = equivalentOperandsAToB.find(inputA);
                if (it != equivalentOperandsAToB.end()) {
                    if (it->second != inputB) {
                        RETURN_FALSE();
                    }
                    continue;
                }
                // We haven't identified an equivalent operand for inputA.
                if (!compare(modelA->getOperand(inputA), modelB->getOperand(inputB))) {
                    RETURN_FALSE();
                }
                equivalentOperandsAToB[inputA] = inputB;
                workQueueOperandsA.push(inputA);
            }
        }

        // Sanity check
        if (modelA->operandCount() != defsA.size() ||
            modelA->operandCount() != defsB.size() ||
            modelA->operandCount() != equivalentOperandsAToB.size() ||
            modelA->operationCount() + pseudoDefinitionCount != equivalentOperationsAToB.size()) {
            RETURN_FALSE();
        }

        RETURN_TRUE();
    }

    /*-------------------------------------------------------------------------------------*/

    bool compare(std::shared_ptr<const ExecutionStep> step,
                 const WrapperModel* model, std::shared_ptr<Device> device) {
        return (step->getDevice() == device) &&
                compare(step->getSubModel(),
                        reinterpret_cast<const ModelBuilder*>(model->getHandle()));
    }
};

TEST_F(PartitioningTest, SimpleModel) {
    PartitioningModel model;
    uint32_t opnd0 = model.addFloatOperand();
    uint32_t opnd1 = model.addFloatOperand();
    uint32_t opnd2 = model.addOperation2To1(0, opnd0, opnd1);
    uint32_t opnd3 = model.addFloatOperand();
    uint32_t opnd4 = model.addOperation2To1(1, opnd2, opnd3);
    model.identifyInputsAndOutputs({ opnd0, opnd1, opnd3 }, { opnd4 });
    model.finish();
    ASSERT_TRUE(model.isValid());
    graphDump("SimpleModel", model);

    // Simple partition (two devices are each capable of everything, one is the best).
    const auto devicesA = makeDevices(
        {
            {"bad", { .float32Performance = { .execTime = 0.9, .powerUsage = 0.9 },
                            .quantized8Performance = { .execTime = 0.9, .powerUsage = 0.9 } }, ~0U},
            {"good", { .float32Performance = { .execTime = 0.5, .powerUsage = 0.5 },
                            .quantized8Performance = { .execTime = 0.5, .powerUsage = 0.5 } }, ~0U}
        });
    ExecutionPlan planA;
    ASSERT_EQ(model.partitionTheWork(devicesA, ExecutePreference::PREFER_LOW_POWER, &planA),
              ANEURALNETWORKS_NO_ERROR);
    ASSERT_EQ(planA.forTest_getKind(), ExecutionPlan::Kind::SIMPLE);
    ASSERT_NE(planA.forTest_simpleGetDevice().get(), nullptr);
    ASSERT_EQ(planA.forTest_simpleGetDevice()->getName(), "good");

    // Simple partition (two devices are each capable of everything, none better than CPU).
    const auto devicesC = makeDevices(
        {
            {"bad", { .float32Performance = { .execTime = 1.1, .powerUsage = 1.1 },
                            .quantized8Performance = { .execTime = 1.1, .powerUsage = 1.1 } }, ~0U},
            {"bad2", { .float32Performance = { .execTime = 1.0, .powerUsage = 1.0 },
                            .quantized8Performance = { .execTime = 1.0, .powerUsage = 1.0 } }, ~0U}
        });
    ExecutionPlan planC;
    ASSERT_EQ(model.partitionTheWork(devicesC, ExecutePreference::PREFER_LOW_POWER, &planC),
              ANEURALNETWORKS_NO_ERROR);
    ASSERT_EQ(planC.forTest_getKind(), ExecutionPlan::Kind::SIMPLE);
    ASSERT_EQ(planC.forTest_simpleGetDevice(), nullptr);

    // Compound partition (two devices, each is capable of one of the
    // two operations).  We could do more extensive checking here --
    // for example, verify that each step within the plan has the
    // correct (model and submodel)x(inputs and outputs).
    const auto devicesB = makeDevices(
        {
            {"0", { .float32Performance = { .execTime = 0.9, .powerUsage = 0.9 },
                            .quantized8Performance = { .execTime = 0.9, .powerUsage = 0.9 } }, 1<<0},
            {"1", { .float32Performance = { .execTime = 0.5, .powerUsage = 0.5 },
                            .quantized8Performance = { .execTime = 0.5, .powerUsage = 0.5 } }, 1<<1}
        });
    ExecutionPlan planB;
    ASSERT_EQ(model.partitionTheWork(devicesB, ExecutePreference::PREFER_LOW_POWER, &planB),
              ANEURALNETWORKS_NO_ERROR);
    ASSERT_EQ(planB.forTest_getKind(), ExecutionPlan::Kind::COMPOUND);
    const auto& stepsB = planB.forTest_compoundGetSteps();
    ASSERT_EQ(stepsB.size(), size_t(2));
    {
        // Build a model to compare against the submodel from stepsB[0].
        PartitioningModel modelB0;
        uint32_t b0Opnd0 = modelB0.addFloatOperand();
        uint32_t b0Opnd1 = modelB0.addFloatOperand();
        uint32_t b0Opnd2 = modelB0.addOperation2To1(0, b0Opnd0, b0Opnd1);
        modelB0.identifyInputsAndOutputs({ b0Opnd0, b0Opnd1 }, { b0Opnd2 });
        modelB0.finish();
        ASSERT_TRUE(modelB0.isValid());
        ASSERT_NO_FATAL_FAILURE(ASSERT_TRUE(compare(stepsB[0], &modelB0, devicesB[0])));
        ASSERT_EQ(stepsB[0]->getModelInputs(),
                  (RemapVectorType{ { opnd0, b0Opnd0 }, { opnd1, b0Opnd1 } }));
        ASSERT_EQ(stepsB[0]->getModelOutputs(),
                  (RemapVectorType{}));
        ASSERT_EQ(stepsB[0]->getTempsAsSubModelInputs(),
                  (RemapVectorType{}));
        ASSERT_EQ(stepsB[0]->getTempsAsSubModelOutputs(),
                  (SubModelOutputSetType{ { opnd2, b0Opnd2 } }));
        ASSERT_EQ(stepsB[0]->getOutputsAsSubModelInputs(),
                  (RemapVectorType{}));
    }
    {
        // Build a model to compare against the submodel from stepsB[1].
        PartitioningModel modelB1;
        uint32_t b1Opnd2 = modelB1.addFloatOperand();
        uint32_t b1Opnd3 = modelB1.addFloatOperand();
        uint32_t b1Opnd4 = modelB1.addOperation2To1(1, b1Opnd2, b1Opnd3);
        // Note: In the partitioning algorithm, submodel inputs follow
        // model inputs.  In the original model "model", opnd2 is not
        // an input; so in the submodel "modelB1", the corresponding
        // input b1Opnd2 is a submodel input, and must follow the
        // model input b1Opnd3.
        modelB1.identifyInputsAndOutputs({ b1Opnd3, b1Opnd2 }, { b1Opnd4 });
        modelB1.finish();
        ASSERT_TRUE(modelB1.isValid());
        ASSERT_NO_FATAL_FAILURE(ASSERT_TRUE(compare(stepsB[1], &modelB1, devicesB[1])));
        ASSERT_EQ(stepsB[1]->getModelInputs(),
                  (RemapVectorType{ { opnd3, b1Opnd3 } }));
        ASSERT_EQ(stepsB[1]->getModelOutputs(),
                  (RemapVectorType{ { opnd4, b1Opnd4 } }));
        ASSERT_EQ(stepsB[1]->getTempsAsSubModelInputs(),
                  (RemapVectorType{ { opnd2, b1Opnd2 } }));
        ASSERT_EQ(stepsB[1]->getTempsAsSubModelOutputs(),
                  (SubModelOutputSetType{}));
        ASSERT_EQ(stepsB[1]->getOutputsAsSubModelInputs(),
                  (RemapVectorType{}));
    }
}

TEST_F(PartitioningTest, Cpu) {
    // Here's a model where some operations execute only on the Cpu.
    // To make things interesting, we produce three partitions --
    // device, cpu, same-device.

    static const uint32_t kCpuOp = 1;
    static const uint32_t kDevOp = 2;

    const auto devices = makeDevices(
        {
            {"1", { .float32Performance = { .execTime = 0.5, .powerUsage = 0.5 },
                    .quantized8Performance = { .execTime = 0.5, .powerUsage = 0.5 } }, 1<<kDevOp}
        });

    PartitioningModel model;

    uint32_t opnd0 = model.addFloatOperand();
    uint32_t opnd1 = model.addFloatOperand();

    uint32_t opnd2 = model.addOperation2To1(kDevOp, opnd0, opnd1);
    uint32_t opnd3 = model.addOperation2To1(kDevOp, opnd0, opnd2);

    uint32_t opnd4 = model.addOperation2To1(kCpuOp, opnd0, opnd3);
    uint32_t opnd5 = model.addOperation2To1(kCpuOp, opnd2, opnd4);

    uint32_t opnd6 = model.addFloatOperand();

    uint32_t opnd7 = model.addOperation2To1(kDevOp, opnd3, opnd5);
    uint32_t opnd8 = model.addOperation2To1(kDevOp, opnd6, opnd7);

    model.identifyInputsAndOutputs({ opnd0, opnd1, opnd6 }, { opnd4, opnd8 });
    model.finish();
    ASSERT_TRUE(model.isValid());

    ExecutionPlan plan;
    ASSERT_EQ(model.partitionTheWork(devices, ExecutePreference::PREFER_LOW_POWER, &plan),
              ANEURALNETWORKS_NO_ERROR);
    ASSERT_EQ(plan.forTest_getKind(), ExecutionPlan::Kind::COMPOUND);
    const auto& steps = plan.forTest_compoundGetSteps();
    ASSERT_EQ(steps.size(), size_t(3));
    {
        const auto& step0 = steps[0];

        // Build a model to compare against the submodel from steps[0].
        PartitioningModel model0;
        uint32_t m0Opnd0 = model0.addFloatOperand();
        uint32_t m0Opnd1 = model0.addFloatOperand();
        uint32_t m0Opnd2 = model0.addOperation2To1(kDevOp, m0Opnd0, m0Opnd1);
        uint32_t m0Opnd3 = model0.addOperation2To1(kDevOp, m0Opnd0, m0Opnd2);
        model0.identifyInputsAndOutputs({ m0Opnd0, m0Opnd1 }, { m0Opnd2, m0Opnd3 });
        model0.finish();
        ASSERT_TRUE(model0.isValid());
        ASSERT_NO_FATAL_FAILURE(ASSERT_TRUE(compare(step0, &model0, devices[0])));
        ASSERT_EQ(step0->getModelInputs(),
                  (RemapVectorType{ { opnd0, m0Opnd0 }, { opnd1, m0Opnd1 } }));
        ASSERT_EQ(step0->getModelOutputs(),
                  (RemapVectorType{}));
        ASSERT_EQ(step0->getTempsAsSubModelInputs(),
                  (RemapVectorType{}));
        ASSERT_EQ(step0->getTempsAsSubModelOutputs(),
                  (SubModelOutputSetType{ { opnd2, m0Opnd2 }, { opnd3, m0Opnd3 } }));
        ASSERT_EQ(step0->getOutputsAsSubModelInputs(),
                  (RemapVectorType{}));
    }
    {
        const auto& step1 = steps[1];

        // Build a model to compare against the submodel from steps[1].
        PartitioningModel model1;
        uint32_t m1Opnd0 = model1.addFloatOperand();
        uint32_t m1Opnd3 = model1.addFloatOperand();
        uint32_t m1Opnd4 = model1.addOperation2To1(kCpuOp, m1Opnd0, m1Opnd3);
        uint32_t m1Opnd2 = model1.addFloatOperand();
        uint32_t m1Opnd5 = model1.addOperation2To1(kCpuOp, m1Opnd2, m1Opnd4);
        model1.identifyInputsAndOutputs({ m1Opnd0, m1Opnd3, m1Opnd2 }, { m1Opnd4, m1Opnd5 });
        model1.finish();
        ASSERT_TRUE(model1.isValid());
        ASSERT_NO_FATAL_FAILURE(ASSERT_TRUE(compare(step1, &model1, nullptr)));
        ASSERT_EQ(step1->getModelInputs(),
                  (RemapVectorType{ { opnd0, m1Opnd0 } }));
        ASSERT_EQ(step1->getModelOutputs(),
                  (RemapVectorType{ { opnd4, m1Opnd4 } }));
        ASSERT_EQ(step1->getTempsAsSubModelInputs(),
                  (RemapVectorType{ { opnd3, m1Opnd3 }, { opnd2, m1Opnd2 } }));
        ASSERT_EQ(step1->getTempsAsSubModelOutputs(),
                  (SubModelOutputSetType{ { opnd5, m1Opnd5 } }));
        ASSERT_EQ(step1->getOutputsAsSubModelInputs(),
                  (RemapVectorType{}));
    }
    {
        const auto& step2 = steps[2];

        // Build a model to compare against the submodel from steps[2].
        PartitioningModel model2;
        uint32_t m2Opnd3 = model2.addFloatOperand();
        uint32_t m2Opnd5 = model2.addFloatOperand();
        uint32_t m2Opnd7 = model2.addOperation2To1(kDevOp, m2Opnd3, m2Opnd5);
        uint32_t m2Opnd6 = model2.addFloatOperand();
        uint32_t m2Opnd8 = model2.addOperation2To1(kDevOp, m2Opnd6, m2Opnd7);
        model2.identifyInputsAndOutputs({ m2Opnd6, m2Opnd3, m2Opnd5 }, { m2Opnd8 });
        model2.finish();
        ASSERT_TRUE(model2.isValid());
        ASSERT_NO_FATAL_FAILURE(ASSERT_TRUE(compare(step2, &model2, devices[0])));
        ASSERT_EQ(step2->getModelInputs(),
                  (RemapVectorType{ { opnd6, m2Opnd6 } }));
        ASSERT_EQ(step2->getModelOutputs(),
                  (RemapVectorType{ { opnd8, m2Opnd8 } }));
        ASSERT_EQ(step2->getTempsAsSubModelInputs(),
                  (RemapVectorType{ { opnd3, m2Opnd3 }, { opnd5, m2Opnd5 } }));
        ASSERT_EQ(step2->getTempsAsSubModelOutputs(),
                  (SubModelOutputSetType{}));
        ASSERT_EQ(step2->getOutputsAsSubModelInputs(),
                  (RemapVectorType{}));
    }
}

TEST_F(PartitioningTest, SetPartitioning) {
    PartitioningModel model;
    uint32_t opnd0 = model.addFloatOperand();
    uint32_t opnd1 = model.addFloatOperand();
    uint32_t opnd2 = model.addOperation2To1(0, opnd0, opnd1, PartitioningModel::Dimensioned::NO);
    uint32_t opnd3 = model.addFloatOperand();
    uint32_t opnd4 = model.addOperation2To1(1, opnd2, opnd3);
    model.identifyInputsAndOutputs({ opnd0, opnd1, opnd3 }, { opnd4 });
    model.finish();
    ASSERT_TRUE(model.isValid());

    // We expect that we cannot successfully partition, because we
    // have an intermediate operand (opnd2) without dimensions, and
    // this is not currently handled.

    // One device that can and should execute operation 0.
    const auto devices = makeDevices({
            {"hw", { .float32Performance = { .execTime = 0.5, .powerUsage = 0.5 },
                            .quantized8Performance = { .execTime = 0.5, .powerUsage = 0.5 } }, (1<<0)},
        });

    // Test kPartitioningNo.  We should not even attempt partitioning,
    // so there should be no execution plan.
    PartitioningCompilation cPNo(&model);
    ASSERT_EQ(cPNo.setPartitioning(DeviceManager::kPartitioningNo), Result::NO_ERROR);
    ASSERT_EQ(cPNo.finish(devices), Result::NO_ERROR);
    ASSERT_EQ(cPNo.getExecutionPlan().forTest_getKind(), ExecutionPlan::Kind::EMPTY);

    // Test kPartitioningWithFallback.  We should attempt
    // partitioning, reach the end of the partitioning process (so we
    // have an execution plan), discover the dimensionless
    // intermediate operand, and still return success (because of
    // fallback).
    PartitioningCompilation cPWithFallback(&model);
    ASSERT_EQ(cPWithFallback.setPartitioning(DeviceManager::kPartitioningWithFallback), Result::NO_ERROR);
    ASSERT_EQ(cPWithFallback.finish(devices), Result::NO_ERROR);
    ASSERT_EQ(cPWithFallback.getExecutionPlan().forTest_getKind(), ExecutionPlan::Kind::ERROR);

    // Test kPartitioningWithoutFallback.  We should attempt
    // partitioning, and fail.
    PartitioningCompilation cPWithoutFallback(&model);
    ASSERT_EQ(cPWithoutFallback.setPartitioning(DeviceManager::kPartitioningWithoutFallback), Result::NO_ERROR);
    ASSERT_EQ(cPWithoutFallback.finish(devices), Result::OP_FAILED);
    ASSERT_TRUE(cPWithoutFallback.getExecutionPlan().forTest_hasSubModelOutputsOfUnknownSize());
    ASSERT_EQ(cPWithoutFallback.getExecutionPlan().forTest_getKind(), ExecutionPlan::Kind::ERROR);
}

// Regression test for http://b/69166603:
//     "partitioned compilation and execution yields wrong results when model output is submodel input"
TEST_F(PartitioningTest, ModelOutputAsSubmodelInput) {
    PartitioningModel model;
    uint32_t opnd0 = model.addFloatOperand();
    uint32_t opnd1 = model.addFloatOperand();
    uint32_t opnd2 = model.addOperation2To1(0, opnd0, opnd1);
    uint32_t opnd3 = model.addOperation2To1(1, opnd2, opnd2);
    model.identifyInputsAndOutputs({ opnd0, opnd1 }, { opnd2, opnd3 });
    model.finish();
    ASSERT_TRUE(model.isValid());

    // Compound partition (two devices, each is capable of one of the
    // two operations).  We could do more extensive checking here --
    // for example, verify that each step within the plan has the
    // correct (model and submodel)x(inputs and outputs).
    const auto devices = makeDevices(
        {
            {"0", { .float32Performance = { .execTime = 0.5, .powerUsage = 0.5 },
                            .quantized8Performance = { .execTime = 0.5, .powerUsage = 0.5 } }, 1<<0},
            {"1", { .float32Performance = { .execTime = 0.5, .powerUsage = 0.5 },
                            .quantized8Performance = { .execTime = 0.5, .powerUsage = 0.5 } }, 1<<1}
        });
    ExecutionPlan plan;
    ASSERT_EQ(model.partitionTheWork(devices, ExecutePreference::PREFER_LOW_POWER, &plan),
              ANEURALNETWORKS_NO_ERROR);
    ASSERT_EQ(plan.forTest_getKind(), ExecutionPlan::Kind::COMPOUND);
    const auto& steps = plan.forTest_compoundGetSteps();
    ASSERT_EQ(steps.size(), size_t(2));
    {
        // Build a model to compare against the submodel from steps[0].
        PartitioningModel model0;
        uint32_t m0Opnd0 = model0.addFloatOperand();
        uint32_t m0Opnd1 = model0.addFloatOperand();
        uint32_t m0Opnd2 = model0.addOperation2To1(0, m0Opnd0, m0Opnd1);
        model0.identifyInputsAndOutputs({ m0Opnd0, m0Opnd1 }, { m0Opnd2 });
        model0.finish();
        ASSERT_TRUE(model0.isValid());
        ASSERT_NO_FATAL_FAILURE(ASSERT_TRUE(compare(steps[0], &model0, devices[0])));
        ASSERT_EQ(steps[0]->getModelInputs(),
                  (RemapVectorType{ { opnd0, m0Opnd0 }, { opnd1, m0Opnd1 } }));
        ASSERT_EQ(steps[0]->getModelOutputs(),
                  (RemapVectorType{ { opnd2, m0Opnd2 } }));
        ASSERT_EQ(steps[0]->getTempsAsSubModelInputs(),
                  (RemapVectorType{}));
        ASSERT_EQ(steps[0]->getTempsAsSubModelOutputs(),
                  (SubModelOutputSetType{}));
        ASSERT_EQ(steps[0]->getOutputsAsSubModelInputs(),
                  (RemapVectorType{}));
    }
    {
        // Build a model to compare against the submodel from steps[1].
        PartitioningModel model1;
        uint32_t m1Opnd2 = model1.addFloatOperand();
        uint32_t m1Opnd3 = model1.addOperation2To1(1, m1Opnd2, m1Opnd2);
        model1.identifyInputsAndOutputs({ m1Opnd2 }, { m1Opnd3 });
        model1.finish();
        ASSERT_TRUE(model1.isValid());
        ASSERT_NO_FATAL_FAILURE(ASSERT_TRUE(compare(steps[1], &model1, devices[1])));
        ASSERT_EQ(steps[1]->getModelInputs(),
                  (RemapVectorType{}));
        ASSERT_EQ(steps[1]->getModelOutputs(),
                  (RemapVectorType{ { opnd3, m1Opnd3 } }));
        ASSERT_EQ(steps[1]->getTempsAsSubModelInputs(),
                  (RemapVectorType{}));
        ASSERT_EQ(steps[1]->getTempsAsSubModelOutputs(),
                  (SubModelOutputSetType{}));
        ASSERT_EQ(steps[1]->getOutputsAsSubModelInputs(),
                  (RemapVectorType{ { opnd2, m1Opnd2 } }));
    }
}

TEST_F(PartitioningTest, OemOperations) {
    // Trivial model consisting solely of OEM operation.
    PartitioningModel model;
    uint32_t opndIn = model.addFloatOperand();
    uint32_t opndOut = model.addOperationOEM1To1(opndIn);
    model.identifyInputsAndOutputs({ opndIn }, { opndOut });
    model.finish();
    ASSERT_TRUE(model.isValid());

    // Verify that the best driver than can run an OEM operation is
    // used, even if it is not better than the CPU.
    const auto devicesBestOEM = makeDevices(
        {
            {"badOEM", { .float32Performance = { .execTime = 1.5, .powerUsage = 1.5 },
                         .quantized8Performance = { .execTime = 1.5, .powerUsage = 1.5 } },
                        ~0U, PartitioningDriver::OEMYes},
            {"noOEM", { .float32Performance = { .execTime = 0.5, .powerUsage = 0.5 },
                        .quantized8Performance = { .execTime = 0.5, .powerUsage = 0.5 } },
                        ~0U, PartitioningDriver::OEMNo},
            {"goodOEM", { .float32Performance = { .execTime = 1.2, .powerUsage = 1.2 },
                          .quantized8Performance = { .execTime = 1.2, .powerUsage = 1.2 } },
                        ~0U, PartitioningDriver::OEMYes}
        });
    PartitioningCompilation compilationBestOEM(&model);
    ASSERT_EQ(compilationBestOEM.finish(devicesBestOEM), Result::NO_ERROR);
    const auto& planBestOEM = compilationBestOEM.getExecutionPlan();
    ASSERT_EQ(planBestOEM.forTest_getKind(), ExecutionPlan::Kind::SIMPLE);
    ASSERT_NE(planBestOEM.forTest_simpleGetDevice().get(), nullptr);
    ASSERT_EQ(planBestOEM.forTest_simpleGetDevice()->getName(), "goodOEM");

    // Verify that we get an error if no driver can run an OEM operation.
    const auto devicesNoOEM = makeDevices(
        {
            {"noOEM", { .float32Performance = { .execTime = 0.5, .powerUsage = 0.5 },
                        .quantized8Performance = { .execTime = 0.5, .powerUsage = 0.5 } },
                        ~0U, PartitioningDriver::OEMNo}
        });
    PartitioningCompilation compilationNoOEM(&model);
    ASSERT_EQ(compilationNoOEM.finish(devicesNoOEM), Result::BAD_DATA);

    // Verify that we get an error if there are no drivers (only CPU fallback).
    PartitioningCompilation compilationNoDrivers(&model);
    ASSERT_EQ(compilationNoDrivers.finish({} /* no drivers */), Result::BAD_DATA);
}

TEST_F(PartitioningTest, RelaxedFP) {
    const auto devices = makeDevices(
        {
            // Best choice for non-relaxed model.
            {"f32", { .float32Performance = { .execTime = 0.8, .powerUsage = 0.8 },
                      .relaxedFloat32toFloat16Performance = { .execTime = 0.9, .powerUsage = 0.9 }},
                    ~0U},
            // Best choice for relaxed model.
            {"f16", { .float32Performance = { .execTime = 0.9, .powerUsage = 0.9 },
                      .relaxedFloat32toFloat16Performance = { .execTime = 0.8, .powerUsage = 0.8 }},
                    ~0U}
        });

    auto TrivialTest = [&devices](bool doRelax, const char* expectDevice) {
        // Trivial model consisting solely of one operation.
        SCOPED_TRACE(expectDevice);
        PartitioningModel model;
        uint32_t opnd0 = model.addFloatOperand();
        uint32_t opnd1 = model.addFloatOperand();
        uint32_t opnd2 = model.addOperation2To1(0, opnd0, opnd1);
        model.identifyInputsAndOutputs({ opnd0, opnd1 }, { opnd2 });
        model.relaxComputationFloat32toFloat16(doRelax);
        model.finish();
        ASSERT_TRUE(model.isValid());
        // Verify that the model will be executed on the appropriate device.
        ExecutionPlan plan;
        ASSERT_EQ(model.partitionTheWork(devices, ExecutePreference::PREFER_LOW_POWER, &plan),
                  ANEURALNETWORKS_NO_ERROR);
        ASSERT_EQ(plan.forTest_getKind(), ExecutionPlan::Kind::SIMPLE);
        ASSERT_EQ(plan.forTest_simpleGetDevice()->getName(), expectDevice);
    };

    ASSERT_NO_FATAL_FAILURE(TrivialTest(false, "f32"));
    ASSERT_NO_FATAL_FAILURE(TrivialTest(true, "f16"));
}

}  // namespace
