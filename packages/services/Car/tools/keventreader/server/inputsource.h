/*
 * Copyright (C) 2018 The Android Open Source Project
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
#ifndef CAR_KEVENTREADER_INPUTSOURCE
#define CAR_KEVENTREADER_INPUTSOURCE

#include <linux/input.h>
#include <optional>
#include <string>
#include "event.h"

namespace com::android::car::keventreader {
    class InputSource {
    public:
        explicit InputSource(const char* file);

        explicit operator bool() const;

        int descriptor() const;

        std::optional<com::android::car::keventreader::KeypressEvent> read() const;

        virtual ~InputSource();
    private:
        struct kevent : public ::input_event {
            kevent();
            bool isKeypress() const;
            bool isKeydown() const;
            bool isKeyup() const;
        };
        static_assert(sizeof(kevent) == sizeof(::input_event), "do not add data to input_event");

        std::string mFilePath;
        int mDescriptor;
    };
}

#endif //CAR_KEVENTREADER_INPUTSOURCE
