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
#ifndef _NNA_ERROR_H__
#define _NNA_ERROR_H__

typedef enum {
    /**
     * Operation was succesful.
     */
    NNA_NO_ERROR = 0,

    /**
     * Failure caused by not enough available memory.
     */
    NNA_OUT_OF_MEMORY = 1,

    NNA_INCOMPLETE = 2,

    /**
     * Failure caused by unexpected null argument.
     */
    NNA_UNEXPECTED_NULL = 3,

    /**
     * Failure caused by invalid function arguments, invalid model definition,
     * invalid execution definition or invalid data at execution time.
     */
    NNA_BAD_DATA = 4,

    /**
     * Failure caused by failed model execution.
     */
    NNA_OP_FAILED = 5,

    /**
     * Failure caused by object being in the wrong state.
     */
    NNA_BAD_STATE = 6,

    /**
     * Failure caused by not being able to map a file into memory.
     * This may be caused by a file descriptor not being mappable.
     * Mitigate by reading its content into memory.
     */
    NNA_UNMAPPABLE = 7,

    /**
     * Failure caused by insufficient buffer size provided to a model output.
     */
    NNA_OUTPUT_INSUFFICIENT_SIZE = 8,
} NNAdapterResultCode;

#define NNA_ERROR_CODE(code)   (NNA_##code)

#define CHECK_NULL_PTR(ptr) {\
    if (!ptr) {\
        NNRT_LOGE_PRINT("Invalid pointer: %s", #ptr);\
        assert(0);\
        return;\
    }\
}

#endif
