
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
#ifndef __EXECUTION_IO_HPP__
#define __EXECUTION_IO_HPP__

namespace nnrt
{
    struct ExecutionIO
    {
        enum {
            UNSPECIFIED,
            BUFFER,
            POINTER,
            HAS_NO_VALUE,   //!< this is for optional inputs
        } state = UNSPECIFIED;

        ExecutionIO(const op::OperandPtr& operand = nullptr, uint8_t *alignedBuf = nullptr) {
            if (!operand) {
                assert(false);
                return;
            }
            tensorHandle = alignedBuf;
            // API not allow to use the operand value to set an execution IO value.
            // mem_ref = operand->weak_mem_ref.lock();
            dimensions = operand->dimensions;
        }

        void setNoValue() {
            state = HAS_NO_VALUE;
        }

        mem_pool::weak_ref weak_mem_ref;

        // the handle is for driver's handle tensor.
        uint8_t *tensorHandle;

        std::vector<uint32_t> dimensions;

        ~ExecutionIO() {
            if (tensorHandle)
                vsi_nn_FreeAlignedBuffer(tensorHandle);
        }
    };

    using ExecutionIOPtr = std::shared_ptr<ExecutionIO>;

}


#endif
