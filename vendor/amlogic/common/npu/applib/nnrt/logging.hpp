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
#ifndef _NNRT_LOGGING_H_
#define _NNRT_LOGGING_H_

#include <sstream>
#include <string>
#include <stdarg.h>
#include <stdio.h>
#include <cstdlib>

#include "utils.hpp"
namespace nnrt {
namespace logging {

enum class Level {
    None = -1,
    Error = 0,
    Warn = 1,
    Info = 2,
    Debug = 3,
};

using endl_type = std::ostream&(std::ostream&);
struct Logger
{
    Logger(const Level level) : Logger(level, "NNRT") {}
    Logger(const Level level, const std::string& tag) : Logger(level, tag, "", "", -1) {}
    Logger(const Level level, const std::string& tag, const std::string& file,
            const std::string& function, const int number)
        : level_(level), tag_(tag), new_msg_(true) {}

    inline bool enabled() const {
        static Level slogLevel = Level::None;

        if (Level::None == slogLevel) {
            int env_lvl = -1;
            if (::nnrt::OS::getEnv("NNRT_LOG_LEVEL", env_lvl)) {
                slogLevel = static_cast<Level>(env_lvl);
            }
        }

        return (slogLevel >= level_);
    }

    inline std::string getLabel() {
        std::string label;
        switch (level_) {
            case Level::Debug:
                label = "D";
                break;
            case Level::Info:
                label = "I";
                break;
            case Level::Warn:
                label = "W";
                break;
            case Level::Error:
                label = "E";
                break;
            default:
                break;
        }
        return label;
    }

    inline std::string getHeader() {
        std::string header = getLabel() + " ";
        if (tag_.length() > 0) {
            header += "(" + tag_ + ")" + " ";
        }
        return header;
    }

    void print(const char* fmt, ...);

    Logger& operator<<(endl_type endl) {
        ostream_ << endl;
        new_msg_ = true;
        this->print(ostream_.str().c_str());
        return *this;
    }

    template<typename T>
    Logger& operator<<(const T& msg) {
        if (enabled()) {
            if (new_msg_) {
                new_msg_ = false;
                ostream_ << getHeader();
            }
            ostream_ << msg;
        }
        return *this;
    }
private:
    Level level_;
    std::string tag_;
    bool new_msg_;
    std::ostringstream ostream_;
};

}

#define NNRT_LOGI_PRINT(fmt, ...)  \
    nnrt::logging::Logger(nnrt::logging::Level::Info).print("[%s:%d]" fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define NNRT_LOGD_PRINT(fmt, ...)  \
    nnrt::logging::Logger(nnrt::logging::Level::Debug).print("[%s:%d]" fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define NNRT_LOGW_PRINT(fmt, ...)  \
    nnrt::logging::Logger(nnrt::logging::Level::Warn).print("[%s:%d]" fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define NNRT_LOGE_PRINT(fmt, ...)  \
    nnrt::logging::Logger(nnrt::logging::Level::Error).print("[%s:%d]" fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#define NNRT_LOGI(TAG) nnrt::logging::Logger(nnrt::logging::Level::Info, TAG, __FILE__, __FUNCTION__, __LINE__)
#define NNRT_LOGD(TAG) nnrt::logging::Logger(nnrt::logging::Level::Debug, TAG, __FILE__, __FUNCTION__, __LINE__)
#define NNRT_LOGW(TAG) nnrt::logging::Logger(nnrt::logging::Level::Warn, TAG, __FILE__, __FUNCTION__, __LINE__)
#define NNRT_LOGE(TAG) nnrt::logging::Logger(nnrt::logging::Level::Error, TAG, __FILE__, __FUNCTION__, __LINE__)

}

#endif
