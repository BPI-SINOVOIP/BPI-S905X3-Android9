/*
 * Copyright (C) 2016 The Android Open Source Project
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

#ifndef NETD_INCLUDE_STOPWATCH_H
#define NETD_INCLUDE_STOPWATCH_H

#include <chrono>

class Stopwatch {
public:
    Stopwatch() : mStart(clock::now()) {}

    virtual ~Stopwatch() {};

    float timeTaken() const {
        return getElapsed(clock::now());
    }

    float getTimeAndReset() {
        const auto& now = clock::now();
        float elapsed = getElapsed(now);
        mStart = now;
        return elapsed;
    }

private:
    typedef std::chrono::steady_clock clock;
    typedef std::chrono::time_point<clock> time_point;
    time_point mStart;

    float getElapsed(const time_point& now) const {
        using ms = std::chrono::duration<float, std::ratio<1, 1000>>;
        return (std::chrono::duration_cast<ms>(now - mStart)).count();
    }
};

#endif  // NETD_INCLUDE_STOPWATCH_H
