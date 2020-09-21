/*
 * Copyright (C) 2015 The Android Open Source Project
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

#ifndef SIMPLE_PERF_OFFLINE_UNWINDER_H_
#define SIMPLE_PERF_OFFLINE_UNWINDER_H_

#include <memory>
#include <vector>
#include <unordered_map>

#include "perf_regs.h"

#include <backtrace/BacktraceMap.h>

namespace simpleperf {
struct ThreadEntry;

struct UnwindingResult {
  // time used for unwinding, in ns.
  uint64_t used_time;
  enum {
    UNKNOWN_REASON,
    EXCEED_MAX_FRAMES_LIMIT,
    ACCESS_REG_FAILED,
    ACCESS_STACK_FAILED,
    ACCESS_MEM_FAILED,
    FIND_PROC_INFO_FAILED,
    EXECUTE_DWARF_INSTRUCTION_FAILED,
    DIFFERENT_ARCH,
    MAP_MISSING,
  } stop_reason;
  union {
    // for ACCESS_REG_FAILED
    uint64_t regno;
    // for ACCESS_MEM_FAILED and ACCESS_STACK_FAILED
    uint64_t addr;
  } stop_info;
  uint64_t stack_start;
  uint64_t stack_end;
};

class OfflineUnwinder {
 public:
  OfflineUnwinder(bool collect_stat);

  bool UnwindCallChain(const ThreadEntry& thread, const RegSet& regs, const char* stack,
                       size_t stack_size, std::vector<uint64_t>* ips, std::vector<uint64_t>* sps);

  bool HasStat() const {
    return collect_stat_;
  }

  const UnwindingResult& GetUnwindingResult() const {
    return unwinding_result_;
  }

 private:
  bool collect_stat_;
  UnwindingResult unwinding_result_;

  // Cache of the most recently used map.
  struct CachedMap {
    uint64_t version = 0u;
    std::unique_ptr<BacktraceMap> map;
  };
  // use unused attribute to pass mac build.
  std::unordered_map<pid_t, CachedMap> cached_maps_  __attribute__((unused));
};

} // namespace simpleperf

#endif  // SIMPLE_PERF_OFFLINE_UNWINDER_H_
