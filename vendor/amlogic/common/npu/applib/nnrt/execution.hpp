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
#ifndef __OVXLIB_EXECUTION_H__
#define __OVXLIB_EXECUTION_H__

#include <mutex>
#include <vector>
#include <memory>
#include "model.hpp"
#include "compilation.hpp"
#include "prepared_model.hpp"
#include "event.hpp"

#include "op/public.hpp"

namespace nnrt
{
class Execution
{
    public:
        Execution(Compilation* compilation);
        virtual ~Execution();

        /**
         * Start compute in async mode.
         * Create a new task and send it to TaskQueue.
         * @note This API is thread safe.
         */
        virtual int startCompute(EventPtr event);

        /**
         * Start compute in sync mode.
         * Create a new task and execute it host thread.
         * @note This API is thread safe.
         */
        virtual int compute();

        /**
         * Set input buffer
         * If current execution is running, it will return OP_FAILD.
         * @note This API is thread safe.
         */
        int setInput(uint32_t index, const op::OperandPtr operand_type,
                const void* buffer, size_t length);

        /**
         * Set input buffer
         * If current execution is running, it will return OP_FAILD.
         * @note This API is thread safe.
         */
        int setInputFromMemory(uint32_t index, const op::OperandPtr operand_type,
                const Memory* memory, size_t offset, size_t length);
        /**
         * Set output buffer
         * If current execution is running, it will return OP_FAILD.
         * @note This API is thread safe.
         */
        int setOutput(uint32_t index, const op::OperandPtr operand_type,
                void* buffer, size_t length);

        /**
         * Set output buffer
         * If current execution is running, it will return OP_FAILD.
         * @note This API is thread safe.
         */
        int setOutputFromMemory(uint32_t index, const op::OperandPtr operand_type,
                const Memory* memory, size_t offset, size_t length);


        int getOutputOperandRank(uint32_t index, uint32_t* rank);

        int getOutputOperandDimensions(uint32_t index, uint32_t* dimensions);

        const std::vector<ExecutionIOPtr> &inputs() const { return inputs_; }
    private:
        /**
         * Private interface to check if current is running.
         * @note This API is NOT thread safe.
         */
        inline bool isRunning() {
            return (event_ && event_->state() == Event::IN_PROCESS);
        }

        std::vector<ExecutionIOPtr> inputs_;
        std::vector<ExecutionIOPtr> outputs_;
        Compilation* compilation_;
        std::mutex mutex_;
        EventPtr event_;
};

using ExecUniquePtr = std::unique_ptr<Execution>;

}
#endif
