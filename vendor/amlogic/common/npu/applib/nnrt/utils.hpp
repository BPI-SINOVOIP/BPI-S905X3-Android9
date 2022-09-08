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
#ifndef __OVXLIB_UTIL_H__
#define __OVXLIB_UTIL_H__

#include <vector>
#include <memory>
#include "types.hpp"

namespace nnrt
{
namespace op {
        class Operand;
        class Operation;
        using OperandPtr = std::shared_ptr<Operand>;
        using OperationPtr = std::shared_ptr<Operation>;
}

class Model;

namespace operand_utils
{

int GetTypeBytes(OperandType type);

int Transpose(Model* model, op::Operand* src, op::Operand* dst,
        std::vector<uint32_t>& perm, DataLayout layout);

int Reshape(op::Operand* src, op::Operand* dst, std::vector<int>& shape);

uint16_t Fp32toFp16(float in);

float Fp16toFp32(uint16_t);

bool IsDynamicShape(nnrt::op::OperandPtr operand);

bool InsertFp16ToFp32LayerBeforeOperand(Model* model,
                                        op::OperationPtr operation,
                                        op::OperandPtr operand);

bool InsertPermuteBeforeOperand(Model* model,
                                op::OperationPtr operation,
                                uint32_t operandId,
                                const std::vector<uint32_t>& permVal);
}

namespace OS {

/*
fetch env{@name} and put it into @result,
return:
0, fail to get this env.
*/
int getEnv(std::string name, int& result);
}  // namespace OS
}

#endif
