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

#include <err.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/user.h>
#include <unistd.h>

#include <gtest/gtest.h>

#include <chrono>
#include <thread>
#include <vector>

#include <android-base/macros.h>

#include "utils.h"

using namespace std::chrono_literals;

static void WaitUntilAllExited(pid_t* pids, size_t pid_count) {
  // Wait until all children have exited.
  bool alive = true;
  while (alive) {
    alive = false;
    for (size_t i = 0; i < pid_count; ++i) {
      if (pids[i] != 0) {
        if (kill(pids[i], 0) == 0) {
          alive = true;
        } else {
          EXPECT_EQ(errno, ESRCH);
          pids[i] = 0;  // Skip in next loop.
        }
      }
    }
  }
}

class LeakChecker {
 public:
  LeakChecker() {
    // Avoid resizing and using memory later.
    // 64Ki is the default limit on VMAs per process.
    maps_.reserve(64*1024);
    Reset();
  }

  ~LeakChecker() {
    Check();
  }

  void Reset() {
    previous_size_ = GetMappingSize();
  }

  void DumpTo(std::ostream& os) const {
    os << previous_size_;
  }

 private:
  size_t previous_size_;
  std::vector<map_record> maps_;

  void Check() {
    auto current_size = GetMappingSize();
    if (current_size > previous_size_) {
      FAIL() << "increase in process map size: " << previous_size_ << " -> " << current_size;
    }
  }

  size_t GetMappingSize() {
    if (!Maps::parse_maps(&maps_)) {
      err(1, "failed to parse maps");
    }

    size_t result = 0;
    for (const map_record& map : maps_) {
      result += map.addr_end - map.addr_start;
    }

    return result;
  }
};

std::ostream& operator<<(std::ostream& os, const LeakChecker& lc) {
  lc.DumpTo(os);
  return os;
}

// http://b/36045112
TEST(pthread_leak, join) {
  LeakChecker lc;

  for (size_t pass = 0; pass < 2; ++pass) {
    for (int i = 0; i < 100; ++i) {
      pthread_t thread;
      ASSERT_EQ(0, pthread_create(&thread, nullptr, [](void*) -> void* { return nullptr; }, nullptr));
      ASSERT_EQ(0, pthread_join(thread, nullptr));
    }

    // A native bridge implementation might need a warm up pass to reach a steady state.
    // http://b/37920774.
    if (pass == 0) lc.Reset();
  }
}

// http://b/36045112
TEST(pthread_leak, detach) {
  LeakChecker lc;

  for (size_t pass = 0; pass < 2; ++pass) {
    constexpr int kThreadCount = 100;
    struct thread_data { pthread_barrier_t* barrier; pid_t* tid; } threads[kThreadCount] = {};

    pthread_barrier_t barrier;
    ASSERT_EQ(pthread_barrier_init(&barrier, nullptr, kThreadCount + 1), 0);

    // Start child threads.
    pid_t tids[kThreadCount];
    for (int i = 0; i < kThreadCount; ++i) {
      threads[i] = {&barrier, &tids[i]};
      const auto thread_function = +[](void* ptr) -> void* {
        thread_data* data = static_cast<thread_data*>(ptr);
        *data->tid = gettid();
        pthread_barrier_wait(data->barrier);
        return nullptr;
      };
      pthread_t thread;
      ASSERT_EQ(0, pthread_create(&thread, nullptr, thread_function, &threads[i]));
      ASSERT_EQ(0, pthread_detach(thread));
    }

    pthread_barrier_wait(&barrier);
    ASSERT_EQ(pthread_barrier_destroy(&barrier), 0);

    WaitUntilAllExited(tids, arraysize(tids));

    // A native bridge implementation might need a warm up pass to reach a steady state.
    // http://b/37920774.
    if (pass == 0) lc.Reset();
  }
}
