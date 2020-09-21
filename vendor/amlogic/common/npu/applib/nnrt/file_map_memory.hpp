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
#ifndef __OVXLIB_MEMORY_H__
#define __OVXLIB_MEMORY_H__

#include <cstdint>
#ifdef __linux__
#include <unistd.h>
#include <sys/mman.h>
#endif

namespace nnrt
{
class Memory
{
    public:
        Memory(){};
        virtual ~Memory() {
#ifdef __linux__
            if (nullptr != data_ptr_)
            {
                munmap(data_ptr_, data_length_);
            }
#endif
        }

        virtual int readFromFd(size_t size,
                int protect, int fd, size_t offset);

        virtual void* data(size_t offset) const {
            if (nullptr == data_ptr_ || offset >= data_length_)
            {
                return nullptr;
            }
            uint8_t* ptr = static_cast<uint8_t*>(data_ptr_);
            return static_cast<void*>(&ptr[offset]);
        }

        virtual size_t length() const {return data_length_;}

    private:
        void * data_ptr_{nullptr};
        size_t data_length_{0};
};
}

#endif
