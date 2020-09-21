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

#ifndef CPP_ESE_H_
#define CPP_ESE_H_

#include <vector>

#include <ese/ese.h>

namespace android {

/**
 * Mockable wrapper for libese's C API.
 *
 * `init` and `open` must be implemented to select the appropriate HW implementation.
 */
class EseInterface {
public:
    EseInterface() : mEse(nullptr) {}
    virtual ~EseInterface() = default;

    virtual void init() = 0;
    virtual int open() = 0;
    virtual void close() = 0;

    virtual const char* name() { return ese_name(mEse); }

    /**
     * Doesn't match the libese interface perfectly as this uses vectors instead
     * of raw C-style arrays but it makes it much easier to mock/test.
     */
    virtual int transceive(const std::vector<uint8_t>& tx, std::vector<uint8_t>& rx) {
        return ese_transceive(mEse,
                              tx.data(), static_cast<uint32_t>(tx.size()),
                              rx.data(), static_cast<uint32_t>(rx.size()));
    }

    // TODO: overload transceive for ese_transceive_sg

    virtual bool error() { return ese_error(mEse); }
    virtual const char* error_message() { return ese_error_message(mEse); }
    virtual int error_code() { return ese_error_code(mEse); }

    /**
     * This is needed in order to call the applet libraries written in C. We
     * should work out a better way to do this.
     */
    ::EseInterface* ese_interface() { return mEse; }

protected:
    ::EseInterface* mEse;
};

} // namespace android

#endif // CPP_ESE_H_
