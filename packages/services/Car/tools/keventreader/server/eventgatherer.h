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
#ifndef CAR_KEVENTREADER_EVENTGATHERER
#define CAR_KEVENTREADER_EVENTGATHERER

#include "inputsource.h"
#include "event.h"

#include <map>
#include <memory>
#include <poll.h>
#include <vector>

namespace com::android::car::keventreader {
    class EventGatherer {
    public:
        EventGatherer(int argc, const char** argv);

        size_t size() const;

        std::vector<com::android::car::keventreader::KeypressEvent> read();
    private:
        std::map<int, std::unique_ptr<InputSource>> mDevices;
        std::vector<pollfd> mFds;
    };
}

#endif //CAR_KEVENTREADER_EVENTGATHERER
