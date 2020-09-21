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

#include "GraphDump.h"

#include "HalInterfaces.h"

#include <set>
#include <iostream>

namespace android {
namespace nn {

// Provide short name for OperandType value.
static std::string translate(OperandType type) {
    switch (type) {
        case OperandType::FLOAT32:             return "F32";
        case OperandType::INT32:               return "I32";
        case OperandType::UINT32:              return "U32";
        case OperandType::TENSOR_FLOAT32:      return "TF32";
        case OperandType::TENSOR_INT32:        return "TI32";
        case OperandType::TENSOR_QUANT8_ASYMM: return "TQ8A";
        case OperandType::OEM:                 return "OEM";
        case OperandType::TENSOR_OEM_BYTE:     return "TOEMB";
        default:                               return toString(type);
    }
}

void graphDump(const char* name, const Model& model, std::ostream& outStream) {
    // Operand nodes are named "d" (operanD) followed by operand index.
    // Operation nodes are named "n" (operatioN) followed by operation index.
    // (These names are not the names that are actually displayed -- those
    //  names are given by the "label" attribute.)

    outStream << "// " << name << std::endl;
    outStream << "digraph {" << std::endl;

    // model inputs and outputs
    std::set<uint32_t> modelIO;
    for (unsigned i = 0, e = model.inputIndexes.size(); i < e; i++) {
        modelIO.insert(model.inputIndexes[i]);
    }
    for (unsigned i = 0, e = model.outputIndexes.size(); i < e; i++) {
        modelIO.insert(model.outputIndexes[i]);
    }

    // model operands
    for (unsigned i = 0, e = model.operands.size(); i < e; i++) {
        outStream << "    d" << i << " [";
        if (modelIO.count(i)) {
            outStream << "style=filled fillcolor=black fontcolor=white ";
        }
        outStream << "label=\"" << i;
        const Operand& opnd = model.operands[i];
        const char* kind = nullptr;
        switch (opnd.lifetime) {
            case OperandLifeTime::CONSTANT_COPY:
                kind = "COPY";
                break;
            case OperandLifeTime::CONSTANT_REFERENCE:
                kind = "REF";
                break;
            case OperandLifeTime::NO_VALUE:
                kind = "NO";
                break;
            default:
                // nothing interesting
                break;
        }
        if (kind) {
            outStream << ": " << kind;
        }
        outStream << "\\n" << translate(opnd.type);
        if (opnd.dimensions.size()) {
            outStream << "(";
            for (unsigned i = 0, e = opnd.dimensions.size(); i < e; i++) {
                if (i > 0) {
                    outStream << "x";
                }
                outStream << opnd.dimensions[i];
            }
            outStream << ")";
        }
        outStream << "\"]" << std::endl;
    }

    // model operations
    for (unsigned i = 0, e = model.operations.size(); i < e; i++) {
        const Operation& operation = model.operations[i];
        outStream << "    n" << i << " [shape=box";
        const uint32_t maxArity = std::max(operation.inputs.size(), operation.outputs.size());
        if (maxArity > 1) {
            if (maxArity == operation.inputs.size()) {
                outStream << " ordering=in";
            } else {
                outStream << " ordering=out";
            }
        }
        outStream << " label=\"" << i << ": "
                  << toString(operation.type) << "\"]" << std::endl;
        {
            // operation inputs
            for (unsigned in = 0, inE = operation.inputs.size(); in < inE; in++) {
                outStream << "    d" << operation.inputs[in] << " -> n" << i;
                if (inE > 1) {
                    outStream << " [label=" << in << "]";
                }
                outStream << std::endl;
            }
        }

        {
            // operation outputs
            for (unsigned out = 0, outE = operation.outputs.size(); out < outE; out++) {
                outStream << "    n" << i << " -> d" << operation.outputs[out];
                if (outE > 1) {
                    outStream << " [label=" << out << "]";
                }
                outStream << std::endl;
            }
        }
    }
    outStream << "}" << std::endl;
}

}  // namespace nn
}  // namespace android
