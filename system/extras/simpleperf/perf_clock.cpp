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

#include "perf_clock.h"

#include <sys/mman.h>
#include <sys/syscall.h>

#include <atomic>
#include <chrono>
#include <thread>

#include <android-base/logging.h>

#include "environment.h"
#include "event_attr.h"
#include "event_fd.h"
#include "event_type.h"
#include "record.h"

static bool perf_clock_initialized = false;
static int64_t perf_clock_and_system_clock_diff_in_ns = 0;

struct ThreadArg {
  std::atomic<pid_t> thread_a_tid;
  std::atomic<bool> start_mmap;
  std::atomic<uint64_t> mmap_start_addr;
  uint64_t system_time_in_ns;
  std::atomic<bool> has_error;
};

static void ThreadA(ThreadArg* thread_arg) {
  thread_arg->thread_a_tid = syscall(SYS_gettid);
  while (!thread_arg->start_mmap) {
    usleep(1000);
  }

  size_t TRY_MMAP_COUNT = 10;

  struct TryMmap {
    void* mmap_start_addr;
    uint64_t start_system_time_in_ns;
    uint64_t end_system_time_in_ns;
  };
  TryMmap array[TRY_MMAP_COUNT];

  // In case current thread is preempted by other threads, we run mmap()
  // multiple times and use the one with the smallest time interval.
  for (size_t i = 0; i < TRY_MMAP_COUNT; ++i) {
    array[i].start_system_time_in_ns = GetSystemClock();
    array[i].mmap_start_addr =
        mmap(NULL, 4096, PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (array[i].mmap_start_addr == MAP_FAILED) {
      PLOG(ERROR) << "mmap() failed";
      thread_arg->has_error = true;
      return;
    }

    array[i].end_system_time_in_ns = GetSystemClock();
  }
  size_t best_index = 0;
  uint64_t min_duration_in_ns = UINT64_MAX;
  for (size_t i = 0; i < TRY_MMAP_COUNT; ++i) {
    uint64_t d =
        array[i].end_system_time_in_ns - array[i].start_system_time_in_ns;
    if (min_duration_in_ns > d) {
      min_duration_in_ns = d;
      best_index = i;
    }
    munmap(array[i].mmap_start_addr, 4096);
  }
  thread_arg->mmap_start_addr =
      reinterpret_cast<uint64_t>(array[best_index].mmap_start_addr);
  // Perf time is generated at the end of mmap() syscall, which is close to
  // the end time instead of the start time.
  thread_arg->system_time_in_ns = array[best_index].end_system_time_in_ns;
}

static bool GetClockDiff(int64_t* clock_diff_in_ns) {
  ThreadArg thread_arg;
  thread_arg.thread_a_tid = 0;
  thread_arg.start_mmap = false;
  thread_arg.has_error = false;
  std::thread thread_a(ThreadA, &thread_arg);
  while (thread_arg.thread_a_tid == 0) {
    usleep(1000);
  }
  std::unique_ptr<EventTypeAndModifier> event_type =
      ParseEventType("cpu-clock");
  if (event_type == nullptr) {
    return false;
  }
  perf_event_attr attr = CreateDefaultPerfEventAttr(event_type->event_type);
  attr.comm = 0;
  attr.mmap_data = 1;
  attr.mmap = 0;
  attr.inherit = 0;
  attr.sample_id_all = 1;
  attr.freq = 0;
  attr.sample_period = 1ULL << 62;  // Sample records are not needed.
  std::unique_ptr<EventFd> event_fd =
      EventFd::OpenEventFile(attr, thread_arg.thread_a_tid, -1, nullptr);
  if (event_fd == nullptr) {
    return false;
  }
  if (!event_fd->CreateMappedBuffer(4, true)) {
    return false;
  }

  thread_arg.start_mmap = true;
  thread_a.join();

  if (thread_arg.has_error) {
    return false;
  }

  std::vector<char> buffer;
  size_t buffer_pos = 0;
  size_t size = event_fd->GetAvailableMmapData(buffer, buffer_pos);
  std::vector<std::unique_ptr<Record>> records =
      ReadRecordsFromBuffer(attr, buffer.data(), size);
  uint64_t perf_time_in_ns = 0;
  for (auto& r : records) {
    if (r->type() == PERF_RECORD_MMAP) {
      auto& record = *static_cast<MmapRecord*>(r.get());
      if (record.data->addr == thread_arg.mmap_start_addr) {
        perf_time_in_ns = record.Timestamp();
      }
    }
  }
  if (perf_time_in_ns == 0) {
    LOG(ERROR) << "GetPerfClockAndSystemClockDiff: can't get perf time.";
    return false;
  }

  *clock_diff_in_ns = perf_time_in_ns - thread_arg.system_time_in_ns;
  LOG(VERBOSE) << "perf_time is " << perf_time_in_ns << " ns, system_time is "
               << thread_arg.system_time_in_ns << " ns , clock_diff is "
               << *clock_diff_in_ns << " ns.";
  return true;
}

bool InitPerfClock() {
  if (!perf_clock_initialized) {
    if (!GetClockDiff(&perf_clock_and_system_clock_diff_in_ns)) {
      return false;
    }
    perf_clock_initialized = true;
  }
  return true;
}

uint64_t GetPerfClock() {
  CHECK(perf_clock_initialized);
  return GetSystemClock() + perf_clock_and_system_clock_diff_in_ns;
}
