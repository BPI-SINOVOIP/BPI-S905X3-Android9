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

#include "thread_tree.h"

#include <inttypes.h>

#include <limits>

#include <android-base/logging.h>
#include <android-base/stringprintf.h>

#include "perf_event.h"
#include "record.h"

namespace simpleperf {

bool MapComparator::operator()(const MapEntry* map1,
                               const MapEntry* map2) const {
  if (map1->start_addr != map2->start_addr) {
    return map1->start_addr < map2->start_addr;
  }
  // Compare map->len instead of map->get_end_addr() here. Because we set map's
  // len to std::numeric_limits<uint64_t>::max() in FindMapByAddr(), which makes
  // map->get_end_addr() overflow.
  if (map1->len != map2->len) {
    return map1->len < map2->len;
  }
  if (map1->time != map2->time) {
    return map1->time < map2->time;
  }
  return false;
}

void ThreadTree::SetThreadName(int pid, int tid, const std::string& comm) {
  ThreadEntry* thread = FindThreadOrNew(pid, tid);
  if (comm != thread->comm) {
    thread_comm_storage_.push_back(
        std::unique_ptr<std::string>(new std::string(comm)));
    thread->comm = thread_comm_storage_.back()->c_str();
  }
}

void ThreadTree::ForkThread(int pid, int tid, int ppid, int ptid) {
  ThreadEntry* parent = FindThreadOrNew(ppid, ptid);
  ThreadEntry* child = FindThreadOrNew(pid, tid);
  child->comm = parent->comm;
  if (pid != ppid) {
    // Copy maps from parent process.
    *child->maps = *parent->maps;
  }
}

ThreadEntry* ThreadTree::FindThreadOrNew(int pid, int tid) {
  auto it = thread_tree_.find(tid);
  if (it == thread_tree_.end()) {
    return CreateThread(pid, tid);
  } else {
    if (pid != it->second.get()->pid) {
      // TODO: b/22185053.
      LOG(DEBUG) << "unexpected (pid, tid) pair: expected ("
                 << it->second.get()->pid << ", " << tid << "), actual (" << pid
                 << ", " << tid << ")";
    }
  }
  return it->second.get();
}

ThreadEntry* ThreadTree::CreateThread(int pid, int tid) {
  MapSet* maps = nullptr;
  if (pid == tid) {
    maps = new MapSet;
    map_set_storage_.push_back(std::unique_ptr<MapSet>(maps));
  } else {
    // Share maps among threads in the same thread group.
    ThreadEntry* process = FindThreadOrNew(pid, pid);
    maps = process->maps;
  }
  ThreadEntry* thread = new ThreadEntry{
    pid, tid,
    "unknown",
    maps,
  };
  auto pair = thread_tree_.insert(std::make_pair(tid, std::unique_ptr<ThreadEntry>(thread)));
  CHECK(pair.second);
  return thread;
}

void ThreadTree::AddKernelMap(uint64_t start_addr, uint64_t len, uint64_t pgoff,
                              uint64_t time, const std::string& filename) {
  // kernel map len can be 0 when record command is not run in supervisor mode.
  if (len == 0) {
    return;
  }
  Dso* dso = FindKernelDsoOrNew(filename);
  MapEntry* map =
      AllocateMap(MapEntry(start_addr, len, pgoff, time, dso, true));
  FixOverlappedMap(&kernel_maps_, map);
  auto pair = kernel_maps_.maps.insert(map);
  CHECK(pair.second);
}

Dso* ThreadTree::FindKernelDsoOrNew(const std::string& filename) {
  if (filename == DEFAULT_KERNEL_MMAP_NAME ||
      filename == DEFAULT_KERNEL_MMAP_NAME_PERF) {
    return kernel_dso_.get();
  }
  auto it = module_dso_tree_.find(filename);
  if (it == module_dso_tree_.end()) {
    module_dso_tree_[filename] = Dso::CreateDso(DSO_KERNEL_MODULE, filename);
    it = module_dso_tree_.find(filename);
  }
  return it->second.get();
}

void ThreadTree::AddThreadMap(int pid, int tid, uint64_t start_addr,
                              uint64_t len, uint64_t pgoff, uint64_t time,
                              const std::string& filename) {
  ThreadEntry* thread = FindThreadOrNew(pid, tid);
  Dso* dso = FindUserDsoOrNew(filename, start_addr);
  MapEntry* map =
      AllocateMap(MapEntry(start_addr, len, pgoff, time, dso, false));
  FixOverlappedMap(thread->maps, map);
  auto pair = thread->maps->maps.insert(map);
  CHECK(pair.second);
  thread->maps->version++;
}

Dso* ThreadTree::FindUserDsoOrNew(const std::string& filename, uint64_t start_addr) {
  auto it = user_dso_tree_.find(filename);
  if (it == user_dso_tree_.end()) {
    bool force_64bit = start_addr > UINT_MAX;
    user_dso_tree_[filename] = Dso::CreateDso(DSO_ELF_FILE, filename, force_64bit);
    it = user_dso_tree_.find(filename);
  }
  return it->second.get();
}

MapEntry* ThreadTree::AllocateMap(const MapEntry& value) {
  MapEntry* map = new MapEntry(value);
  map_storage_.push_back(std::unique_ptr<MapEntry>(map));
  return map;
}

void ThreadTree::FixOverlappedMap(MapSet* maps, const MapEntry* map) {
  for (auto it = maps->maps.begin(); it != maps->maps.end();) {
    if ((*it)->start_addr >= map->get_end_addr()) {
      // No more overlapped maps.
      break;
    }
    if ((*it)->get_end_addr() <= map->start_addr) {
      ++it;
    } else {
      MapEntry* old = *it;
      if (old->start_addr < map->start_addr) {
        MapEntry* before = AllocateMap(
            MapEntry(old->start_addr, map->start_addr - old->start_addr,
                     old->pgoff, old->time, old->dso, old->in_kernel));
        maps->maps.insert(before);
      }
      if (old->get_end_addr() > map->get_end_addr()) {
        MapEntry* after = AllocateMap(MapEntry(
            map->get_end_addr(), old->get_end_addr() - map->get_end_addr(),
            map->get_end_addr() - old->start_addr + old->pgoff, old->time,
            old->dso, old->in_kernel));
        maps->maps.insert(after);
      }

      it = maps->maps.erase(it);
    }
  }
}

static bool IsAddrInMap(uint64_t addr, const MapEntry* map) {
  return (addr >= map->start_addr && addr < map->get_end_addr());
}

static MapEntry* FindMapByAddr(const MapSet& maps, uint64_t addr) {
  // Construct a map_entry which is strictly after the searched map_entry, based
  // on MapComparator.
  MapEntry find_map(addr, std::numeric_limits<uint64_t>::max(), 0,
                    std::numeric_limits<uint64_t>::max(), nullptr, false);
  auto it = maps.maps.upper_bound(&find_map);
  if (it != maps.maps.begin() && IsAddrInMap(addr, *--it)) {
    return *it;
  }
  return nullptr;
}

const MapEntry* ThreadTree::FindMap(const ThreadEntry* thread, uint64_t ip,
                                    bool in_kernel) {
  MapEntry* result = nullptr;
  if (!in_kernel) {
    result = FindMapByAddr(*thread->maps, ip);
  } else {
    result = FindMapByAddr(kernel_maps_, ip);
  }
  return result != nullptr ? result : &unknown_map_;
}

const MapEntry* ThreadTree::FindMap(const ThreadEntry* thread, uint64_t ip) {
  MapEntry* result = FindMapByAddr(*thread->maps, ip);
  if (result != nullptr) {
    return result;
  }
  result = FindMapByAddr(kernel_maps_, ip);
  return result != nullptr ? result : &unknown_map_;
}

const Symbol* ThreadTree::FindSymbol(const MapEntry* map, uint64_t ip,
                                     uint64_t* pvaddr_in_file, Dso** pdso) {
  uint64_t vaddr_in_file;
  const Symbol* symbol = nullptr;
  Dso* dso = map->dso;
  if (!map->in_kernel) {
    // Find symbol in user space shared libraries.
    vaddr_in_file = ip - map->start_addr + map->dso->MinVirtualAddress();
    symbol = dso->FindSymbol(vaddr_in_file);
  } else {
    if (dso != kernel_dso_.get()) {
      // Find symbol in kernel modules.
      vaddr_in_file = ip - map->start_addr + map->dso->MinVirtualAddress();
      symbol = dso->FindSymbol(vaddr_in_file);
    }
    if (symbol == nullptr) {
      // If the ip address hits the vmlinux, or hits a kernel module, but we can't find its symbol
      // in the kernel module file, then find its symbol in /proc/kallsyms or vmlinux.
      vaddr_in_file = ip;
      dso = kernel_dso_.get();
      symbol = dso->FindSymbol(vaddr_in_file);
    }
  }

  if (symbol == nullptr) {
    if (show_ip_for_unknown_symbol_) {
      std::string name = android::base::StringPrintf(
          "%s%s[+%" PRIx64 "]", (show_mark_for_unknown_symbol_ ? "*" : ""),
          dso->FileName().c_str(), vaddr_in_file);
      dso->AddUnknownSymbol(vaddr_in_file, name);
      symbol = dso->FindSymbol(vaddr_in_file);
      CHECK(symbol != nullptr);
    } else {
      symbol = &unknown_symbol_;
    }
  }
  if (pvaddr_in_file != nullptr) {
    *pvaddr_in_file = vaddr_in_file;
  }
  if (pdso != nullptr) {
    *pdso = dso;
  }
  return symbol;
}

const Symbol* ThreadTree::FindKernelSymbol(uint64_t ip) {
  const MapEntry* map = FindMap(nullptr, ip, true);
  return FindSymbol(map, ip, nullptr);
}

void ThreadTree::ClearThreadAndMap() {
  thread_tree_.clear();
  thread_comm_storage_.clear();
  map_set_storage_.clear();
  kernel_maps_.maps.clear();
  map_storage_.clear();
}

void ThreadTree::AddDsoInfo(const std::string& file_path, uint32_t file_type,
                            uint64_t min_vaddr, std::vector<Symbol>* symbols) {
  DsoType dso_type = static_cast<DsoType>(file_type);
  Dso* dso = nullptr;
  if (dso_type == DSO_KERNEL || dso_type == DSO_KERNEL_MODULE) {
    dso = FindKernelDsoOrNew(file_path);
  } else {
    dso = FindUserDsoOrNew(file_path);
  }
  dso->SetMinVirtualAddress(min_vaddr);
  dso->SetSymbols(symbols);
}

void ThreadTree::Update(const Record& record) {
  if (record.type() == PERF_RECORD_MMAP) {
    const MmapRecord& r = *static_cast<const MmapRecord*>(&record);
    if (r.InKernel()) {
      AddKernelMap(r.data->addr, r.data->len, r.data->pgoff,
                   r.sample_id.time_data.time, r.filename);
    } else {
      AddThreadMap(r.data->pid, r.data->tid, r.data->addr, r.data->len,
                   r.data->pgoff, r.sample_id.time_data.time, r.filename);
    }
  } else if (record.type() == PERF_RECORD_MMAP2) {
    const Mmap2Record& r = *static_cast<const Mmap2Record*>(&record);
    if (r.InKernel()) {
      AddKernelMap(r.data->addr, r.data->len, r.data->pgoff,
                   r.sample_id.time_data.time, r.filename);
    } else {
      std::string filename = (r.filename == DEFAULT_EXECNAME_FOR_THREAD_MMAP)
                                 ? "[unknown]"
                                 : r.filename;
      AddThreadMap(r.data->pid, r.data->tid, r.data->addr, r.data->len,
                   r.data->pgoff, r.sample_id.time_data.time, filename);
    }
  } else if (record.type() == PERF_RECORD_COMM) {
    const CommRecord& r = *static_cast<const CommRecord*>(&record);
    SetThreadName(r.data->pid, r.data->tid, r.comm);
  } else if (record.type() == PERF_RECORD_FORK) {
    const ForkRecord& r = *static_cast<const ForkRecord*>(&record);
    ForkThread(r.data->pid, r.data->tid, r.data->ppid, r.data->ptid);
  } else if (record.type() == SIMPLE_PERF_RECORD_KERNEL_SYMBOL) {
    const auto& r = *static_cast<const KernelSymbolRecord*>(&record);
    Dso::SetKallsyms(std::move(r.kallsyms));
  }
}

std::vector<Dso*> ThreadTree::GetAllDsos() const {
  std::vector<Dso*> result;
  result.push_back(kernel_dso_.get());
  for (auto& p : module_dso_tree_) {
    result.push_back(p.second.get());
  }
  for (auto& p : user_dso_tree_) {
    result.push_back(p.second.get());
  }
  result.push_back(unknown_dso_.get());
  return result;
}

std::vector<const ThreadEntry*> ThreadTree::GetAllThreads() const {
  std::vector<const ThreadEntry*> threads;
  for (auto& pair : thread_tree_) {
    threads.push_back(pair.second.get());
  }
  return threads;
}

}  // namespace simpleperf
