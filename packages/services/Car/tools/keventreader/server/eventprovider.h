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
#ifndef CAR_KEVENTREADER_EVENTPROVIDER
#define CAR_KEVENTREADER_EVENTPROVIDER

#include "com/android/car/keventreader/IEventCallback.h"
#include "com/android/car/keventreader/BnEventProvider.h"
#include <binder/Binder.h>
#include "eventgatherer.h"
#include <mutex>
#include <thread>
#include <vector>

using namespace android;
using namespace android::binder;

namespace com::android::car::keventreader {
    class EventProviderImpl : public BnEventProvider {
    public:
        EventProviderImpl(EventGatherer&&);
        std::thread startLoop();

        virtual Status registerCallback(const sp<IEventCallback>& callback) override;
        virtual Status unregisterCallback(const sp<IEventCallback>& callback) override;
    private:
        EventGatherer mGatherer;
        std::mutex mMutex;
        std::vector<sp<IEventCallback>> mCallbacks;
    };
}

#endif
