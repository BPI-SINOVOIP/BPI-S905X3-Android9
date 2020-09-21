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

#include "OfflineUnwinder.h"

#include <android-base/logging.h>
#include <backtrace/Backtrace.h>
#include <backtrace/BacktraceMap.h>
#include <unwindstack/MachineArm.h>
#include <unwindstack/MachineArm64.h>
#include <unwindstack/MachineX86.h>
#include <unwindstack/MachineX86_64.h>
#include <unwindstack/Regs.h>
#include <unwindstack/RegsArm.h>
#include <unwindstack/RegsArm64.h>
#include <unwindstack/RegsX86.h>
#include <unwindstack/RegsX86_64.h>
#include <unwindstack/UserArm.h>
#include <unwindstack/UserArm64.h>
#include <unwindstack/UserX86.h>
#include <unwindstack/UserX86_64.h>

#include "environment.h"
#include "perf_regs.h"
#include "read_apk.h"
#include "thread_tree.h"

namespace simpleperf {

static unwindstack::Regs* GetBacktraceRegs(const RegSet& regs) {
  switch (regs.arch) {
    case ARCH_ARM: {
      unwindstack::arm_user_regs arm_user_regs;
      memset(&arm_user_regs, 0, sizeof(arm_user_regs));
      static_assert(
          static_cast<int>(unwindstack::ARM_REG_R0) == static_cast<int>(PERF_REG_ARM_R0), "");
      static_assert(
          static_cast<int>(unwindstack::ARM_REG_LAST) == static_cast<int>(PERF_REG_ARM_MAX), "");
      for (size_t i = unwindstack::ARM_REG_R0; i < unwindstack::ARM_REG_LAST; ++i) {
        arm_user_regs.regs[i] = static_cast<uint32_t>(regs.data[i]);
      }
      return unwindstack::RegsArm::Read(&arm_user_regs);
    }
    case ARCH_ARM64: {
      unwindstack::arm64_user_regs arm64_user_regs;
      memset(&arm64_user_regs, 0, sizeof(arm64_user_regs));
      static_assert(
          static_cast<int>(unwindstack::ARM64_REG_R0) == static_cast<int>(PERF_REG_ARM64_X0), "");
      static_assert(
          static_cast<int>(unwindstack::ARM64_REG_R30) == static_cast<int>(PERF_REG_ARM64_LR), "");
      memcpy(&arm64_user_regs.regs[unwindstack::ARM64_REG_R0], &regs.data[PERF_REG_ARM64_X0],
             sizeof(uint64_t) * (PERF_REG_ARM64_LR - PERF_REG_ARM64_X0 + 1));
      arm64_user_regs.sp = regs.data[PERF_REG_ARM64_SP];
      arm64_user_regs.pc = regs.data[PERF_REG_ARM64_PC];
      return unwindstack::RegsArm64::Read(&arm64_user_regs);
    }
    case ARCH_X86_32: {
      unwindstack::x86_user_regs x86_user_regs;
      memset(&x86_user_regs, 0, sizeof(x86_user_regs));
      x86_user_regs.eax = static_cast<uint32_t>(regs.data[PERF_REG_X86_AX]);
      x86_user_regs.ebx = static_cast<uint32_t>(regs.data[PERF_REG_X86_BX]);
      x86_user_regs.ecx = static_cast<uint32_t>(regs.data[PERF_REG_X86_CX]);
      x86_user_regs.edx = static_cast<uint32_t>(regs.data[PERF_REG_X86_DX]);
      x86_user_regs.ebp = static_cast<uint32_t>(regs.data[PERF_REG_X86_BP]);
      x86_user_regs.edi = static_cast<uint32_t>(regs.data[PERF_REG_X86_DI]);
      x86_user_regs.esi = static_cast<uint32_t>(regs.data[PERF_REG_X86_SI]);
      x86_user_regs.esp = static_cast<uint32_t>(regs.data[PERF_REG_X86_SP]);
      x86_user_regs.eip = static_cast<uint32_t>(regs.data[PERF_REG_X86_IP]);
      return unwindstack::RegsX86::Read(&x86_user_regs);
    }
    case ARCH_X86_64: {
      unwindstack::x86_64_user_regs x86_64_user_regs;
      memset(&x86_64_user_regs, 0, sizeof(x86_64_user_regs));
      x86_64_user_regs.rax = regs.data[PERF_REG_X86_AX];
      x86_64_user_regs.rbx = regs.data[PERF_REG_X86_BX];
      x86_64_user_regs.rcx = regs.data[PERF_REG_X86_CX];
      x86_64_user_regs.rdx = regs.data[PERF_REG_X86_DX];
      x86_64_user_regs.r8 = regs.data[PERF_REG_X86_R8];
      x86_64_user_regs.r9 = regs.data[PERF_REG_X86_R9];
      x86_64_user_regs.r10 = regs.data[PERF_REG_X86_R10];
      x86_64_user_regs.r11 = regs.data[PERF_REG_X86_R11];
      x86_64_user_regs.r12 = regs.data[PERF_REG_X86_R12];
      x86_64_user_regs.r13 = regs.data[PERF_REG_X86_R13];
      x86_64_user_regs.r14 = regs.data[PERF_REG_X86_R14];
      x86_64_user_regs.r15 = regs.data[PERF_REG_X86_R15];
      x86_64_user_regs.rdi = regs.data[PERF_REG_X86_DI];
      x86_64_user_regs.rsi = regs.data[PERF_REG_X86_SI];
      x86_64_user_regs.rbp = regs.data[PERF_REG_X86_BP];
      x86_64_user_regs.rsp = regs.data[PERF_REG_X86_SP];
      x86_64_user_regs.rip = regs.data[PERF_REG_X86_IP];
      return unwindstack::RegsX86_64::Read(&x86_64_user_regs);
    }
    default:
      return nullptr;
  }
}

OfflineUnwinder::OfflineUnwinder(bool collect_stat) : collect_stat_(collect_stat) {
  Backtrace::SetGlobalElfCache(true);
}

bool OfflineUnwinder::UnwindCallChain(const ThreadEntry& thread, const RegSet& regs,
                                      const char* stack, size_t stack_size,
                                      std::vector<uint64_t>* ips, std::vector<uint64_t>* sps) {
  uint64_t start_time;
  if (collect_stat_) {
    start_time = GetSystemClock();
  }
  std::vector<uint64_t> result;
  uint64_t sp_reg_value;
  if (!regs.GetSpRegValue(&sp_reg_value)) {
    LOG(ERROR) << "can't get sp reg value";
    return false;
  }
  uint64_t stack_addr = sp_reg_value;

  // Create map only if the maps have changed since the last unwind.
  auto map_it = cached_maps_.find(thread.pid);
  CachedMap& cached_map = (map_it == cached_maps_.end() ? cached_maps_[thread.pid]
                                                        : map_it->second);
  if (cached_map.version < thread.maps->version) {
    std::vector<backtrace_map_t> bt_maps(thread.maps->maps.size());
    size_t map_index = 0;
    for (auto& map : thread.maps->maps) {
      backtrace_map_t& bt_map = bt_maps[map_index++];
      bt_map.start = map->start_addr;
      bt_map.end = map->start_addr + map->len;
      bt_map.offset = map->pgoff;
      bt_map.name = map->dso->GetDebugFilePath();
      if (bt_map.offset == 0) {
        size_t apk_pos = bt_map.name.find_last_of('!');
        if (apk_pos != std::string::npos) {
          // The unwinder does not understand the ! format, so change back to
          // the previous format (apk, offset).
          std::string shared_lib(bt_map.name.substr(apk_pos + 2));
          bt_map.name = bt_map.name.substr(0, apk_pos);
          uint64_t offset;
          uint32_t length;
          if (ApkInspector::FindOffsetInApkByName(bt_map.name, shared_lib, &offset, &length)) {
            bt_map.offset = offset;
          }
        }
      }
      bt_map.flags = PROT_READ | PROT_EXEC;
    }
    cached_map.map.reset(BacktraceMap::CreateOffline(thread.pid, bt_maps));
    if (!cached_map.map) {
      return false;
    }
    // Disable the resolving of names, this data is not used.
    cached_map.map->SetResolveNames(false);
    cached_map.version = thread.maps->version;
  }

  backtrace_stackinfo_t stack_info;
  stack_info.start = stack_addr;
  stack_info.end = stack_addr + stack_size;
  stack_info.data = reinterpret_cast<const uint8_t*>(stack);

  std::unique_ptr<unwindstack::Regs> unwind_regs(GetBacktraceRegs(regs));
  if (!unwind_regs) {
    return false;
  }
  std::vector<backtrace_frame_data_t> frames;
  BacktraceUnwindError error;
  if (Backtrace::UnwindOffline(unwind_regs.get(), cached_map.map.get(), stack_info, &frames, &error)) {
    for (auto& frame : frames) {
      // Unwinding in arm architecture can return 0 pc address.
      if (frame.pc == 0) {
        break;
      }
      ips->push_back(frame.pc);
      sps->push_back(frame.sp);
    }
  }

  uint64_t ip_reg_value;
  if (!regs.GetIpRegValue(&ip_reg_value)) {
    LOG(ERROR) << "can't get ip reg value";
    return false;
  }
  if (ips->empty()) {
    ips->push_back(ip_reg_value);
    sps->push_back(sp_reg_value);
  } else {
    // Check if the unwinder returns ip reg value as the first ip address in callstack.
    CHECK_EQ((*ips)[0], ip_reg_value);
  }
  if (collect_stat_) {
    unwinding_result_.used_time = GetSystemClock() - start_time;
    switch (error.error_code) {
      case BACKTRACE_UNWIND_ERROR_EXCEED_MAX_FRAMES_LIMIT:
        unwinding_result_.stop_reason = UnwindingResult::EXCEED_MAX_FRAMES_LIMIT;
        break;
      case BACKTRACE_UNWIND_ERROR_ACCESS_REG_FAILED:
        unwinding_result_.stop_reason = UnwindingResult::ACCESS_REG_FAILED;
        unwinding_result_.stop_info.regno = error.error_info.regno;
        break;
      case BACKTRACE_UNWIND_ERROR_ACCESS_MEM_FAILED:
        // Because we don't have precise stack range here, just guess an addr is in stack
        // if sp - 128K <= addr <= sp.
        if (error.error_info.addr <= stack_addr &&
            error.error_info.addr >= stack_addr - 128 * 1024) {
          unwinding_result_.stop_reason = UnwindingResult::ACCESS_STACK_FAILED;
        } else {
          unwinding_result_.stop_reason = UnwindingResult::ACCESS_MEM_FAILED;
        }
        unwinding_result_.stop_info.addr = error.error_info.addr;
        break;
      case BACKTRACE_UNWIND_ERROR_FIND_PROC_INFO_FAILED:
        unwinding_result_.stop_reason = UnwindingResult::FIND_PROC_INFO_FAILED;
        break;
      case BACKTRACE_UNWIND_ERROR_EXECUTE_DWARF_INSTRUCTION_FAILED:
        unwinding_result_.stop_reason = UnwindingResult::EXECUTE_DWARF_INSTRUCTION_FAILED;
        break;
      case BACKTRACE_UNWIND_ERROR_MAP_MISSING:
        unwinding_result_.stop_reason = UnwindingResult::MAP_MISSING;
        break;
      default:
        unwinding_result_.stop_reason = UnwindingResult::UNKNOWN_REASON;
        break;
    }
    unwinding_result_.stack_start = stack_info.start;
    unwinding_result_.stack_end = stack_info.end;
  }
  return true;
}

}  // namespace simpleperf
