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
#ifndef __OVXLIB_COMPILATION_H__
#define __OVXLIB_COMPILATION_H__

#include <mutex>
#include "vsi_nn_pub.h"
#include "interpreter.hpp"
#include "prepared_model.hpp"
#include "shared_context.hpp"

namespace nnrt
{
class Model;

class Compilation
{
    public:
        Compilation(Model* model);

        virtual ~Compilation();

        virtual int run();

        virtual int finish() {return 0;};

        Model* getModel() {return model_;}

        void setInterpreter(Interpreter* interpreter) {
            if (nullptr != interpreter_)   delete interpreter_;
            interpreter_ = interpreter;
        }

        Interpreter* getInterpreter() { return interpreter_; };

        PreparedModelPtr prepareModel(int* err_ptr,
            std::vector<ExecutionIOPtr> &inputs);
    private:
    Compilation(const Compilation&) = default;
    Compilation(Compilation&& ) = default;
    Compilation& operator=(const Compilation&) = default;

    private:
        void cachePreparedModel(PreparedModelPtr& model);

        Model* model_;
        Interpreter* interpreter_;  //!< default interpreter_ set as NNapi interperter
        uint32_t prepared_model_cache_size_;
        std::map<std::string, PreparedModelPtr> prepared_models_;
        SharedContextPtr context_;
        std::mutex cache_mutex_;
};

using CompilerUniquePtr = std::unique_ptr<Compilation>;
}

#endif
