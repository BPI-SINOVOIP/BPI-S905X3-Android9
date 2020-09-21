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
#ifndef __OVXLIB_EVENT_H__
#define __OVXLIB_EVENT_H__

#include <mutex>
#include <condition_variable>

namespace nnrt
{
class Event
{
    public:
        enum
        {
            IDLE,
            IN_PROCESS,
            ERROR_OCCURED,
            CANCELED,
            COMPLETED,
        };

        Event(int state = IDLE);
        ~Event();

        int wait();

        void notify(int state);

        bool is_canceled() { return (state_ == CANCELED); }

        int state() { return state_; }

        void lock() { mutex_.lock(); };

        void unlock() { mutex_.unlock(); };

    private:
        int state_;
        std::mutex mutex_;
        std::condition_variable condition_;
};

using EventPtr = std::shared_ptr<Event>;
}
#endif
