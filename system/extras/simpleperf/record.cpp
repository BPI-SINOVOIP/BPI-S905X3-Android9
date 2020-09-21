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

#include "record.h"

#include <inttypes.h>
#include <algorithm>
#include <unordered_map>

#include <android-base/logging.h>
#include <android-base/stringprintf.h>

#include "dso.h"
#include "OfflineUnwinder.h"
#include "perf_regs.h"
#include "tracing.h"
#include "utils.h"

using namespace simpleperf;

static std::string RecordTypeToString(int record_type) {
  static std::unordered_map<int, std::string> record_type_names = {
      {PERF_RECORD_MMAP, "mmap"},
      {PERF_RECORD_LOST, "lost"},
      {PERF_RECORD_COMM, "comm"},
      {PERF_RECORD_EXIT, "exit"},
      {PERF_RECORD_THROTTLE, "throttle"},
      {PERF_RECORD_UNTHROTTLE, "unthrottle"},
      {PERF_RECORD_FORK, "fork"},
      {PERF_RECORD_READ, "read"},
      {PERF_RECORD_SAMPLE, "sample"},
      {PERF_RECORD_BUILD_ID, "build_id"},
      {PERF_RECORD_MMAP2, "mmap2"},
      {PERF_RECORD_TRACING_DATA, "tracing_data"},
      {SIMPLE_PERF_RECORD_KERNEL_SYMBOL, "kernel_symbol"},
      {SIMPLE_PERF_RECORD_DSO, "dso"},
      {SIMPLE_PERF_RECORD_SYMBOL, "symbol"},
      {SIMPLE_PERF_RECORD_EVENT_ID, "event_id"},
      {SIMPLE_PERF_RECORD_CALLCHAIN, "callchain"},
      {SIMPLE_PERF_RECORD_UNWINDING_RESULT, "unwinding_result"},
  };

  auto it = record_type_names.find(record_type);
  if (it != record_type_names.end()) {
    return it->second;
  }
  return android::base::StringPrintf("unknown(%d)", record_type);
}

template <>
void MoveToBinaryFormat(const RecordHeader& data, char*& p) {
  data.MoveToBinaryFormat(p);
}

SampleId::SampleId() { memset(this, 0, sizeof(SampleId)); }

// Return sample_id size in binary format.
size_t SampleId::CreateContent(const perf_event_attr& attr, uint64_t event_id) {
  sample_id_all = attr.sample_id_all;
  sample_type = attr.sample_type;
  id_data.id = event_id;
  // Other data are not necessary. TODO: Set missing SampleId data.
  return Size();
}

void SampleId::ReadFromBinaryFormat(const perf_event_attr& attr, const char* p,
                                    const char* end) {
  sample_id_all = attr.sample_id_all;
  sample_type = attr.sample_type;
  if (sample_id_all) {
    if (sample_type & PERF_SAMPLE_TID) {
      MoveFromBinaryFormat(tid_data, p);
    }
    if (sample_type & PERF_SAMPLE_TIME) {
      MoveFromBinaryFormat(time_data, p);
    }
    if (sample_type & PERF_SAMPLE_ID) {
      MoveFromBinaryFormat(id_data, p);
    }
    if (sample_type & PERF_SAMPLE_STREAM_ID) {
      MoveFromBinaryFormat(stream_id_data, p);
    }
    if (sample_type & PERF_SAMPLE_CPU) {
      MoveFromBinaryFormat(cpu_data, p);
    }
    if (sample_type & PERF_SAMPLE_IDENTIFIER) {
      MoveFromBinaryFormat(id_data, p);
    }
  }
  CHECK_LE(p, end);
  if (p < end) {
    LOG(DEBUG) << "Record SampleId part has " << end - p << " bytes left\n";
  }
}

void SampleId::WriteToBinaryFormat(char*& p) const {
  if (sample_id_all) {
    if (sample_type & PERF_SAMPLE_TID) {
      MoveToBinaryFormat(tid_data, p);
    }
    if (sample_type & PERF_SAMPLE_TIME) {
      MoveToBinaryFormat(time_data, p);
    }
    if (sample_type & PERF_SAMPLE_ID) {
      MoveToBinaryFormat(id_data, p);
    }
    if (sample_type & PERF_SAMPLE_STREAM_ID) {
      MoveToBinaryFormat(stream_id_data, p);
    }
    if (sample_type & PERF_SAMPLE_CPU) {
      MoveToBinaryFormat(cpu_data, p);
    }
  }
}

void SampleId::Dump(size_t indent) const {
  if (sample_id_all) {
    if (sample_type & PERF_SAMPLE_TID) {
      PrintIndented(indent, "sample_id: pid %u, tid %u\n", tid_data.pid,
                    tid_data.tid);
    }
    if (sample_type & PERF_SAMPLE_TIME) {
      PrintIndented(indent, "sample_id: time %" PRId64 "\n", time_data.time);
    }
    if (sample_type & (PERF_SAMPLE_ID | PERF_SAMPLE_IDENTIFIER)) {
      PrintIndented(indent, "sample_id: id %" PRId64 "\n", id_data.id);
    }
    if (sample_type & PERF_SAMPLE_STREAM_ID) {
      PrintIndented(indent, "sample_id: stream_id %" PRId64 "\n",
                    stream_id_data.stream_id);
    }
    if (sample_type & PERF_SAMPLE_CPU) {
      PrintIndented(indent, "sample_id: cpu %u, res %u\n", cpu_data.cpu,
                    cpu_data.res);
    }
  }
}

size_t SampleId::Size() const {
  size_t size = 0;
  if (sample_id_all) {
    if (sample_type & PERF_SAMPLE_TID) {
      size += sizeof(PerfSampleTidType);
    }
    if (sample_type & PERF_SAMPLE_TIME) {
      size += sizeof(PerfSampleTimeType);
    }
    if (sample_type & PERF_SAMPLE_ID) {
      size += sizeof(PerfSampleIdType);
    }
    if (sample_type & PERF_SAMPLE_STREAM_ID) {
      size += sizeof(PerfSampleStreamIdType);
    }
    if (sample_type & PERF_SAMPLE_CPU) {
      size += sizeof(PerfSampleCpuType);
    }
    if (sample_type & PERF_SAMPLE_IDENTIFIER) {
      size += sizeof(PerfSampleIdType);
    }
  }
  return size;
}

Record::Record(Record&& other) {
  header = other.header;
  sample_id = other.sample_id;
  binary_ = other.binary_;
  own_binary_ = other.own_binary_;
  other.binary_ = nullptr;
  other.own_binary_ = false;
}

void Record::Dump(size_t indent) const {
  PrintIndented(indent, "record %s: type %u, misc %u, size %u\n",
                RecordTypeToString(type()).c_str(), type(), misc(), size());
  DumpData(indent + 1);
  sample_id.Dump(indent + 1);
}

uint64_t Record::Timestamp() const { return sample_id.time_data.time; }
uint32_t Record::Cpu() const { return sample_id.cpu_data.cpu; }
uint64_t Record::Id() const { return sample_id.id_data.id; }

void Record::UpdateBinary(char* new_binary) {
  if (own_binary_) {
    delete[] binary_;
  }
  own_binary_ = true;
  binary_ = new_binary;
}

MmapRecord::MmapRecord(const perf_event_attr& attr, char* p) : Record(p) {
  const char* end = p + size();
  p += header_size();
  data = reinterpret_cast<const MmapRecordDataType*>(p);
  p += sizeof(*data);
  filename = p;
  p += Align(strlen(filename) + 1, 8);
  CHECK_LE(p, end);
  sample_id.ReadFromBinaryFormat(attr, p, end);
}

MmapRecord::MmapRecord(const perf_event_attr& attr, bool in_kernel,
                       uint32_t pid, uint32_t tid, uint64_t addr, uint64_t len,
                       uint64_t pgoff, const std::string& filename,
                       uint64_t event_id, uint64_t time) {
  SetTypeAndMisc(PERF_RECORD_MMAP,
                 in_kernel ? PERF_RECORD_MISC_KERNEL : PERF_RECORD_MISC_USER);
  sample_id.CreateContent(attr, event_id);
  sample_id.time_data.time = time;
  MmapRecordDataType data;
  data.pid = pid;
  data.tid = tid;
  data.addr = addr;
  data.len = len;
  data.pgoff = pgoff;
  SetDataAndFilename(data, filename);
}

void MmapRecord::SetDataAndFilename(const MmapRecordDataType& data,
                                    const std::string& filename) {
  SetSize(header_size() + sizeof(data) + Align(filename.size() + 1, 8) +
          sample_id.Size());
  char* new_binary = new char[size()];
  char* p = new_binary;
  MoveToBinaryFormat(header, p);
  this->data = reinterpret_cast<MmapRecordDataType*>(p);
  MoveToBinaryFormat(data, p);
  this->filename = p;
  strcpy(p, filename.c_str());
  p += Align(filename.size() + 1, 8);
  sample_id.WriteToBinaryFormat(p);
  UpdateBinary(new_binary);
}

void MmapRecord::DumpData(size_t indent) const {
  PrintIndented(indent,
                "pid %u, tid %u, addr 0x%" PRIx64 ", len 0x%" PRIx64 "\n",
                data->pid, data->tid, data->addr, data->len);
  PrintIndented(indent, "pgoff 0x%" PRIx64 ", filename %s\n", data->pgoff,
                filename);
}

Mmap2Record::Mmap2Record(const perf_event_attr& attr, char* p) : Record(p) {
  const char* end = p + size();
  p += header_size();
  data = reinterpret_cast<const Mmap2RecordDataType*>(p);
  p += sizeof(*data);
  filename = p;
  p += Align(strlen(filename) + 1, 8);
  CHECK_LE(p, end);
  sample_id.ReadFromBinaryFormat(attr, p, end);
}

void Mmap2Record::SetDataAndFilename(const Mmap2RecordDataType& data,
                                     const std::string& filename) {
  SetSize(header_size() + sizeof(data) + Align(filename.size() + 1, 8) +
          sample_id.Size());
  char* new_binary = new char[size()];
  char* p = new_binary;
  MoveToBinaryFormat(header, p);
  this->data = reinterpret_cast<Mmap2RecordDataType*>(p);
  MoveToBinaryFormat(data, p);
  this->filename = p;
  strcpy(p, filename.c_str());
  p += Align(filename.size() + 1, 8);
  sample_id.WriteToBinaryFormat(p);
  UpdateBinary(new_binary);
}

void Mmap2Record::DumpData(size_t indent) const {
  PrintIndented(indent,
                "pid %u, tid %u, addr 0x%" PRIx64 ", len 0x%" PRIx64 "\n",
                data->pid, data->tid, data->addr, data->len);
  PrintIndented(indent, "pgoff 0x" PRIx64 ", maj %u, min %u, ino %" PRId64
                        ", ino_generation %" PRIu64 "\n",
                data->pgoff, data->maj, data->min, data->ino,
                data->ino_generation);
  PrintIndented(indent, "prot %u, flags %u, filenames %s\n", data->prot,
                data->flags, filename);
}

CommRecord::CommRecord(const perf_event_attr& attr, char* p) : Record(p) {
  const char* end = p + size();
  p += header_size();
  data = reinterpret_cast<const CommRecordDataType*>(p);
  p += sizeof(*data);
  comm = p;
  p += Align(strlen(p) + 1, 8);
  CHECK_LE(p, end);
  sample_id.ReadFromBinaryFormat(attr, p, end);
}

CommRecord::CommRecord(const perf_event_attr& attr, uint32_t pid, uint32_t tid,
                       const std::string& comm, uint64_t event_id, uint64_t time) {
  SetTypeAndMisc(PERF_RECORD_COMM, 0);
  CommRecordDataType data;
  data.pid = pid;
  data.tid = tid;
  size_t sample_id_size = sample_id.CreateContent(attr, event_id);
  sample_id.time_data.time = time;
  SetSize(header_size() + sizeof(data) + Align(comm.size() + 1, 8) +
          sample_id_size);
  char* new_binary = new char[size()];
  char* p = new_binary;
  MoveToBinaryFormat(header, p);
  this->data = reinterpret_cast<CommRecordDataType*>(p);
  MoveToBinaryFormat(data, p);
  this->comm = p;
  strcpy(p, comm.c_str());
  p += Align(comm.size() + 1, 8);
  sample_id.WriteToBinaryFormat(p);
  UpdateBinary(new_binary);
}

void CommRecord::DumpData(size_t indent) const {
  PrintIndented(indent, "pid %u, tid %u, comm %s\n", data->pid, data->tid,
                comm);
}

ExitOrForkRecord::ExitOrForkRecord(const perf_event_attr& attr, char* p)
    : Record(p) {
  const char* end = p + size();
  p += header_size();
  data = reinterpret_cast<const ExitOrForkRecordDataType*>(p);
  p += sizeof(*data);
  CHECK_LE(p, end);
  sample_id.ReadFromBinaryFormat(attr, p, end);
}

void ExitOrForkRecord::DumpData(size_t indent) const {
  PrintIndented(indent, "pid %u, ppid %u, tid %u, ptid %u\n", data->pid,
                data->ppid, data->tid, data->ptid);
}

ForkRecord::ForkRecord(const perf_event_attr& attr, uint32_t pid, uint32_t tid,
                       uint32_t ppid, uint32_t ptid, uint64_t event_id) {
  SetTypeAndMisc(PERF_RECORD_FORK, 0);
  ExitOrForkRecordDataType data;
  data.pid = pid;
  data.ppid = ppid;
  data.tid = tid;
  data.ptid = ptid;
  data.time = 0;
  size_t sample_id_size = sample_id.CreateContent(attr, event_id);
  SetSize(header_size() + sizeof(data) + sample_id_size);
  char* new_binary = new char[size()];
  char* p = new_binary;
  MoveToBinaryFormat(header, p);
  this->data = reinterpret_cast<ExitOrForkRecordDataType*>(p);
  MoveToBinaryFormat(data, p);
  sample_id.WriteToBinaryFormat(p);
  UpdateBinary(new_binary);
}

LostRecord::LostRecord(const perf_event_attr& attr, char* p) : Record(p) {
  const char* end = p + size();
  p += header_size();
  MoveFromBinaryFormat(id, p);
  MoveFromBinaryFormat(lost, p);
  CHECK_LE(p, end);
  sample_id.ReadFromBinaryFormat(attr, p, end);
}

void LostRecord::DumpData(size_t indent) const {
  PrintIndented(indent, "id %" PRIu64 ", lost %" PRIu64 "\n", id, lost);
}

SampleRecord::SampleRecord(const perf_event_attr& attr, char* p) : Record(p) {
  const char* end = p + size();
  p += header_size();
  sample_type = attr.sample_type;

  // Set a default id value to report correctly even if ID is not recorded.
  id_data.id = 0;
  if (sample_type & PERF_SAMPLE_IDENTIFIER) {
    MoveFromBinaryFormat(id_data, p);
  }
  if (sample_type & PERF_SAMPLE_IP) {
    MoveFromBinaryFormat(ip_data, p);
  }
  if (sample_type & PERF_SAMPLE_TID) {
    MoveFromBinaryFormat(tid_data, p);
  }
  if (sample_type & PERF_SAMPLE_TIME) {
    MoveFromBinaryFormat(time_data, p);
  }
  if (sample_type & PERF_SAMPLE_ADDR) {
    MoveFromBinaryFormat(addr_data, p);
  }
  if (sample_type & PERF_SAMPLE_ID) {
    MoveFromBinaryFormat(id_data, p);
  }
  if (sample_type & PERF_SAMPLE_STREAM_ID) {
    MoveFromBinaryFormat(stream_id_data, p);
  }
  if (sample_type & PERF_SAMPLE_CPU) {
    MoveFromBinaryFormat(cpu_data, p);
  }
  if (sample_type & PERF_SAMPLE_PERIOD) {
    MoveFromBinaryFormat(period_data, p);
  }
  if (sample_type & PERF_SAMPLE_CALLCHAIN) {
    MoveFromBinaryFormat(callchain_data.ip_nr, p);
    callchain_data.ips = reinterpret_cast<uint64_t*>(p);
    p += callchain_data.ip_nr * sizeof(uint64_t);
  }
  if (sample_type & PERF_SAMPLE_RAW) {
    MoveFromBinaryFormat(raw_data.size, p);
    raw_data.data = p;
    p += raw_data.size;
  }
  if (sample_type & PERF_SAMPLE_BRANCH_STACK) {
    MoveFromBinaryFormat(branch_stack_data.stack_nr, p);
    branch_stack_data.stack = reinterpret_cast<BranchStackItemType*>(p);
    p += branch_stack_data.stack_nr * sizeof(BranchStackItemType);
  }
  if (sample_type & PERF_SAMPLE_REGS_USER) {
    MoveFromBinaryFormat(regs_user_data.abi, p);
    if (regs_user_data.abi == 0) {
      regs_user_data.reg_mask = 0;
    } else {
      regs_user_data.reg_mask = attr.sample_regs_user;
      size_t bit_nr = 0;
      for (size_t i = 0; i < 64; ++i) {
        if ((regs_user_data.reg_mask >> i) & 1) {
          bit_nr++;
        }
      }
      regs_user_data.reg_nr = bit_nr;
      regs_user_data.regs = reinterpret_cast<uint64_t*>(p);
      p += bit_nr * sizeof(uint64_t);
    }
  }
  if (sample_type & PERF_SAMPLE_STACK_USER) {
    MoveFromBinaryFormat(stack_user_data.size, p);
    if (stack_user_data.size == 0) {
      stack_user_data.dyn_size = 0;
    } else {
      stack_user_data.data = p;
      p += stack_user_data.size;
      MoveFromBinaryFormat(stack_user_data.dyn_size, p);
    }
  }
  // TODO: Add parsing of other PERF_SAMPLE_*.
  CHECK_LE(p, end);
  if (p < end) {
    LOG(DEBUG) << "Record has " << end - p << " bytes left\n";
  }
}

SampleRecord::SampleRecord(const perf_event_attr& attr, uint64_t id,
                           uint64_t ip, uint32_t pid, uint32_t tid,
                           uint64_t time, uint32_t cpu, uint64_t period,
                           const std::vector<uint64_t>& ips) {
  SetTypeAndMisc(PERF_RECORD_SAMPLE, PERF_RECORD_MISC_USER);
  sample_type = attr.sample_type;
  CHECK_EQ(0u, sample_type & ~(PERF_SAMPLE_IP | PERF_SAMPLE_TID
      | PERF_SAMPLE_TIME | PERF_SAMPLE_ID | PERF_SAMPLE_CPU
      | PERF_SAMPLE_PERIOD | PERF_SAMPLE_CALLCHAIN | PERF_SAMPLE_REGS_USER
      | PERF_SAMPLE_STACK_USER));
  ip_data.ip = ip;
  tid_data.pid = pid;
  tid_data.tid = tid;
  time_data.time = time;
  id_data.id = id;
  cpu_data.cpu = cpu;
  cpu_data.res = 0;
  period_data.period = period;
  callchain_data.ip_nr = ips.size();
  raw_data.size = 0;
  branch_stack_data.stack_nr = 0;
  regs_user_data.abi = 0;
  regs_user_data.reg_mask = 0;
  stack_user_data.size = 0;

  uint32_t size = header_size();
  if (sample_type & PERF_SAMPLE_IP) {
    size += sizeof(ip_data);
  }
  if (sample_type & PERF_SAMPLE_TID) {
    size += sizeof(tid_data);
  }
  if (sample_type & PERF_SAMPLE_TIME) {
    size += sizeof(time_data);
  }
  if (sample_type & PERF_SAMPLE_ID) {
    size += sizeof(id_data);
  }
  if (sample_type & PERF_SAMPLE_CPU) {
    size += sizeof(cpu_data);
  }
  if (sample_type & PERF_SAMPLE_PERIOD) {
    size += sizeof(period_data);
  }
  if (sample_type & PERF_SAMPLE_CALLCHAIN) {
    size += sizeof(uint64_t) * (ips.size() + 1);
  }
  if (sample_type & PERF_SAMPLE_REGS_USER) {
    size += sizeof(uint64_t);
  }
  if (sample_type & PERF_SAMPLE_STACK_USER) {
    size += sizeof(uint64_t);
  }

  SetSize(size);
  char* new_binary = new char[size];
  char* p = new_binary;
  MoveToBinaryFormat(header, p);
  if (sample_type & PERF_SAMPLE_IP) {
    MoveToBinaryFormat(ip_data, p);
  }
  if (sample_type & PERF_SAMPLE_TID) {
    MoveToBinaryFormat(tid_data, p);
  }
  if (sample_type & PERF_SAMPLE_TIME) {
    MoveToBinaryFormat(time_data, p);
  }
  if (sample_type & PERF_SAMPLE_ID) {
    MoveToBinaryFormat(id_data, p);
  }
  if (sample_type & PERF_SAMPLE_CPU) {
    MoveToBinaryFormat(cpu_data, p);
  }
  if (sample_type & PERF_SAMPLE_PERIOD) {
    MoveToBinaryFormat(period_data, p);
  }
  if (sample_type & PERF_SAMPLE_CALLCHAIN) {
    MoveToBinaryFormat(callchain_data.ip_nr, p);
    callchain_data.ips = reinterpret_cast<uint64_t*>(p);
    MoveToBinaryFormat(ips.data(), ips.size(), p);
  }
  if (sample_type & PERF_SAMPLE_REGS_USER) {
    MoveToBinaryFormat(regs_user_data.abi, p);
  }
  if (sample_type & PERF_SAMPLE_STACK_USER) {
    MoveToBinaryFormat(stack_user_data.size, p);
  }
  CHECK_EQ(p, new_binary + size);
  UpdateBinary(new_binary);
}

void SampleRecord::ReplaceRegAndStackWithCallChain(
    const std::vector<uint64_t>& ips) {
  uint32_t size_added_in_callchain = sizeof(uint64_t) * (ips.size() + 1);
  uint32_t size_reduced_in_reg_stack =
      regs_user_data.reg_nr * sizeof(uint64_t) + stack_user_data.size +
      sizeof(uint64_t);
  CHECK_LE(size_added_in_callchain, size_reduced_in_reg_stack);
  uint32_t size_reduced = size_reduced_in_reg_stack - size_added_in_callchain;
  SetSize(size() - size_reduced);
  char* p = binary_;
  MoveToBinaryFormat(header, p);
  p = (stack_user_data.data + stack_user_data.size + sizeof(uint64_t)) -
      (size_reduced_in_reg_stack - size_added_in_callchain);
  stack_user_data.size = 0;
  regs_user_data.abi = 0;
  p -= sizeof(uint64_t);
  *reinterpret_cast<uint64_t*>(p) = stack_user_data.size;
  p -= sizeof(uint64_t);
  *reinterpret_cast<uint64_t*>(p) = regs_user_data.abi;
  if (sample_type & PERF_SAMPLE_BRANCH_STACK) {
    p -= branch_stack_data.stack_nr * sizeof(BranchStackItemType);
    memmove(p, branch_stack_data.stack,
            branch_stack_data.stack_nr * sizeof(BranchStackItemType));
    p -= sizeof(uint64_t);
    *reinterpret_cast<uint64_t*>(p) = branch_stack_data.stack_nr;
  }
  if (sample_type & PERF_SAMPLE_RAW) {
    p -= raw_data.size;
    memmove(p, raw_data.data, raw_data.size);
    p -= sizeof(uint32_t);
    *reinterpret_cast<uint32_t*>(p) = raw_data.size;
  }
  p -= ips.size() * sizeof(uint64_t);
  memcpy(p, ips.data(), ips.size() * sizeof(uint64_t));
  p -= sizeof(uint64_t);
  *reinterpret_cast<uint64_t*>(p) = PERF_CONTEXT_USER;
  p -= sizeof(uint64_t) * (callchain_data.ip_nr);
  callchain_data.ips = reinterpret_cast<uint64_t*>(p);
  callchain_data.ip_nr += ips.size() + 1;
  p -= sizeof(uint64_t);
  *reinterpret_cast<uint64_t*>(p) = callchain_data.ip_nr;
}

size_t SampleRecord::ExcludeKernelCallChain() {
  size_t user_callchain_length = 0u;
  if (sample_type & PERF_SAMPLE_CALLCHAIN) {
    size_t i;
    for (i = 0; i < callchain_data.ip_nr; ++i) {
      if (callchain_data.ips[i] == PERF_CONTEXT_USER) {
        i++;
        if (i < callchain_data.ip_nr) {
          ip_data.ip = callchain_data.ips[i];
          if (sample_type & PERF_SAMPLE_IP) {
            *reinterpret_cast<uint64_t*>(binary_ + header_size()) = ip_data.ip;
          }
          header.misc = (header.misc & ~PERF_RECORD_MISC_KERNEL) | PERF_RECORD_MISC_USER;
          reinterpret_cast<perf_event_header*>(binary_)->misc = header.misc;
        }
        break;
      } else {
        callchain_data.ips[i] = PERF_CONTEXT_USER;
      }
    }
    user_callchain_length = callchain_data.ip_nr - i;
  }
  return user_callchain_length;
}

bool SampleRecord::HasUserCallChain() const {
  if ((sample_type & PERF_SAMPLE_CALLCHAIN) == 0) {
    return false;
  }
  bool in_user_context = !InKernel();
  for (size_t i = 0; i < callchain_data.ip_nr; ++i) {
    if (in_user_context && callchain_data.ips[i] < PERF_CONTEXT_MAX) {
      return true;
    }
    if (callchain_data.ips[i] == PERF_CONTEXT_USER) {
      in_user_context = true;
    }
  }
  return false;
}

void SampleRecord::UpdateUserCallChain(const std::vector<uint64_t>& user_ips) {
  std::vector<uint64_t> kernel_ips;
  for (size_t i = 0; i < callchain_data.ip_nr; ++i) {
    if (callchain_data.ips[i] == PERF_CONTEXT_USER) {
      break;
    }
    kernel_ips.push_back(callchain_data.ips[i]);
  }
  kernel_ips.push_back(PERF_CONTEXT_USER);
  size_t new_size = size() - callchain_data.ip_nr * sizeof(uint64_t) +
                    (kernel_ips.size() + user_ips.size()) * sizeof(uint64_t);
  if (new_size == size()) {
    return;
  }
  char* new_binary = new char[new_size];
  char* p = new_binary;
  SetSize(new_size);
  MoveToBinaryFormat(header, p);
  if (sample_type & PERF_SAMPLE_IDENTIFIER) {
    MoveToBinaryFormat(id_data, p);
  }
  if (sample_type & PERF_SAMPLE_IP) {
    MoveToBinaryFormat(ip_data, p);
  }
  if (sample_type & PERF_SAMPLE_TID) {
    MoveToBinaryFormat(tid_data, p);
  }
  if (sample_type & PERF_SAMPLE_TIME) {
    MoveToBinaryFormat(time_data, p);
  }
  if (sample_type & PERF_SAMPLE_ADDR) {
    MoveToBinaryFormat(addr_data, p);
  }
  if (sample_type & PERF_SAMPLE_ID) {
    MoveToBinaryFormat(id_data, p);
  }
  if (sample_type & PERF_SAMPLE_STREAM_ID) {
    MoveToBinaryFormat(stream_id_data, p);
  }
  if (sample_type & PERF_SAMPLE_CPU) {
    MoveToBinaryFormat(cpu_data, p);
  }
  if (sample_type & PERF_SAMPLE_PERIOD) {
    MoveToBinaryFormat(period_data, p);
  }
  if (sample_type & PERF_SAMPLE_CALLCHAIN) {
    callchain_data.ip_nr = kernel_ips.size() + user_ips.size();
    MoveToBinaryFormat(callchain_data.ip_nr, p);
    callchain_data.ips = reinterpret_cast<uint64_t*>(p);
    MoveToBinaryFormat(kernel_ips.data(), kernel_ips.size(), p);
    MoveToBinaryFormat(user_ips.data(), user_ips.size(), p);
  }
  if (sample_type & PERF_SAMPLE_RAW) {
    MoveToBinaryFormat(raw_data.size, p);
    MoveToBinaryFormat(raw_data.data, raw_data.size, p);
    raw_data.data = p - raw_data.size;
  }
  if (sample_type & PERF_SAMPLE_BRANCH_STACK) {
    MoveToBinaryFormat(branch_stack_data.stack_nr, p);
    char* old_p = p;
    MoveToBinaryFormat(branch_stack_data.stack, branch_stack_data.stack_nr, p);
    branch_stack_data.stack = reinterpret_cast<BranchStackItemType*>(old_p);
  }
  if (sample_type & PERF_SAMPLE_REGS_USER) {
    MoveToBinaryFormat(regs_user_data.abi, p);
    CHECK_EQ(regs_user_data.abi, 0u);
  }
  if (sample_type & PERF_SAMPLE_STACK_USER) {
    MoveToBinaryFormat(stack_user_data.size, p);
    CHECK_EQ(stack_user_data.size, 0u);
  }
  CHECK_EQ(p, new_binary + new_size) << "sample_type = " << std::hex << sample_type;
  UpdateBinary(new_binary);
}

// When simpleperf requests the kernel to dump 64K stack per sample, it will allocate 64K space in
// each sample to store stack data. However, a thread may use less stack than 64K. So not all the
// 64K stack data in a sample is valid. And this function is used to remove invalid stack data in
// a sample, which can save time and disk space when storing samples in file.
void SampleRecord::RemoveInvalidStackData() {
  if (sample_type & PERF_SAMPLE_STACK_USER) {
    uint64_t valid_stack_size = GetValidStackSize();
    if (stack_user_data.size > valid_stack_size) {
      // Shrink stack size to valid_stack_size, and update it in binary.
      stack_user_data.size = valid_stack_size;
      char* p = stack_user_data.data - sizeof(stack_user_data.size);
      MoveToBinaryFormat(stack_user_data.size, p);
      p += valid_stack_size;
      // Update dyn_size in binary.
      if (valid_stack_size != 0u) {
        MoveToBinaryFormat(stack_user_data.dyn_size, p);
      }
      // Update sample size.
      header.size = p - binary_;
      p = binary_;
      header.MoveToBinaryFormat(p);
    }
  }
}

void SampleRecord::DumpData(size_t indent) const {
  PrintIndented(indent, "sample_type: 0x%" PRIx64 "\n", sample_type);
  if (sample_type & PERF_SAMPLE_IP) {
    PrintIndented(indent, "ip %p\n", reinterpret_cast<void*>(ip_data.ip));
  }
  if (sample_type & PERF_SAMPLE_TID) {
    PrintIndented(indent, "pid %u, tid %u\n", tid_data.pid, tid_data.tid);
  }
  if (sample_type & PERF_SAMPLE_TIME) {
    PrintIndented(indent, "time %" PRId64 "\n", time_data.time);
  }
  if (sample_type & PERF_SAMPLE_ADDR) {
    PrintIndented(indent, "addr %p\n", reinterpret_cast<void*>(addr_data.addr));
  }
  if (sample_type & (PERF_SAMPLE_ID | PERF_SAMPLE_IDENTIFIER)) {
    PrintIndented(indent, "id %" PRId64 "\n", id_data.id);
  }
  if (sample_type & PERF_SAMPLE_STREAM_ID) {
    PrintIndented(indent, "stream_id %" PRId64 "\n", stream_id_data.stream_id);
  }
  if (sample_type & PERF_SAMPLE_CPU) {
    PrintIndented(indent, "cpu %u, res %u\n", cpu_data.cpu, cpu_data.res);
  }
  if (sample_type & PERF_SAMPLE_PERIOD) {
    PrintIndented(indent, "period %" PRId64 "\n", period_data.period);
  }
  if (sample_type & PERF_SAMPLE_CALLCHAIN) {
    PrintIndented(indent, "callchain nr=%" PRIu64 "\n", callchain_data.ip_nr);
    for (uint64_t i = 0; i < callchain_data.ip_nr; ++i) {
      PrintIndented(indent + 1, "0x%" PRIx64 "\n", callchain_data.ips[i]);
    }
  }
  if (sample_type & PERF_SAMPLE_RAW) {
    PrintIndented(indent, "raw size=%zu\n", raw_data.size);
    const uint32_t* data = reinterpret_cast<const uint32_t*>(raw_data.data);
    size_t size = raw_data.size / sizeof(uint32_t);
    for (size_t i = 0; i < size; ++i) {
      PrintIndented(indent + 1, "0x%08x (%zu)\n", data[i], data[i]);
    }
  }
  if (sample_type & PERF_SAMPLE_BRANCH_STACK) {
    PrintIndented(indent, "branch_stack nr=%" PRIu64 "\n",
                  branch_stack_data.stack_nr);
    for (uint64_t i = 0; i < branch_stack_data.stack_nr; ++i) {
      auto& item = branch_stack_data.stack[i];
      PrintIndented(indent + 1, "from 0x%" PRIx64 ", to 0x%" PRIx64
                                ", flags 0x%" PRIx64 "\n",
                    item.from, item.to, item.flags);
    }
  }
  if (sample_type & PERF_SAMPLE_REGS_USER) {
    PrintIndented(indent, "user regs: abi=%" PRId64 "\n", regs_user_data.abi);
    for (size_t i = 0, pos = 0; i < 64; ++i) {
      if ((regs_user_data.reg_mask >> i) & 1) {
        PrintIndented(
            indent + 1, "reg (%s) 0x%016" PRIx64 "\n",
            GetRegName(i, ScopedCurrentArch::GetCurrentArch()).c_str(),
            regs_user_data.regs[pos++]);
      }
    }
  }
  if (sample_type & PERF_SAMPLE_STACK_USER) {
    PrintIndented(indent, "user stack: size %zu dyn_size %" PRIu64 "\n",
                  stack_user_data.size, stack_user_data.dyn_size);
    const uint64_t* p = reinterpret_cast<const uint64_t*>(stack_user_data.data);
    const uint64_t* end = p + (stack_user_data.size / sizeof(uint64_t));
    while (p < end) {
      PrintIndented(indent + 1, "");
      for (size_t i = 0; i < 4 && p < end; ++i, ++p) {
        printf(" %016" PRIx64, *p);
      }
      printf("\n");
    }
    printf("\n");
  }
}

uint64_t SampleRecord::Timestamp() const { return time_data.time; }
uint32_t SampleRecord::Cpu() const { return cpu_data.cpu; }
uint64_t SampleRecord::Id() const { return id_data.id; }

void SampleRecord::AdjustCallChainGeneratedByKernel() {
  // The kernel stores return addrs in the callchain, but we want the addrs of call instructions
  // along the callchain.
  uint64_t* ips = callchain_data.ips;
  bool first_frame = true;
  for (uint64_t i = 0; i < callchain_data.ip_nr; ++i) {
    if (ips[i] > 0 && ips[i] < PERF_CONTEXT_MAX) {
      if (first_frame) {
        first_frame = false;
      } else {
        // Here we want to change the return addr to the addr of the previous instruction. We don't
        // need to find the exact start addr of the previous instruction. A location in
        // [start_addr_of_call_inst, start_addr_of_next_inst) is enough.
#if defined(__arm__) || defined(__aarch64__)
        // If we are built for arm/aarch64, this may be a callchain of thumb code. For thumb code,
        // the real instruction addr is (ip & ~1), and ip - 2 can used to hit the address range
        // of the previous instruction. For non thumb code, any addr in [ip - 4, ip - 1] is fine.
        ips[i] -= 2;
#else
        ips[i]--;
#endif
      }
    }
  }
}

BuildIdRecord::BuildIdRecord(char* p) : Record(p) {
  const char* end = p + size();
  p += header_size();
  MoveFromBinaryFormat(pid, p);
  build_id = BuildId(p, BUILD_ID_SIZE);
  p += Align(build_id.Size(), 8);
  filename = p;
  p += Align(strlen(filename) + 1, 64);
  CHECK_EQ(p, end);
}

void BuildIdRecord::DumpData(size_t indent) const {
  PrintIndented(indent, "pid %u\n", pid);
  PrintIndented(indent, "build_id %s\n", build_id.ToString().c_str());
  PrintIndented(indent, "filename %s\n", filename);
}

BuildIdRecord::BuildIdRecord(bool in_kernel, pid_t pid, const BuildId& build_id,
                             const std::string& filename) {
  SetTypeAndMisc(PERF_RECORD_BUILD_ID,
                 in_kernel ? PERF_RECORD_MISC_KERNEL : PERF_RECORD_MISC_USER);
  this->pid = pid;
  this->build_id = build_id;
  SetSize(header_size() + sizeof(pid) + Align(build_id.Size(), 8) +
          Align(filename.size() + 1, 64));
  char* new_binary = new char[size()];
  char* p = new_binary;
  MoveToBinaryFormat(header, p);
  MoveToBinaryFormat(pid, p);
  memcpy(p, build_id.Data(), build_id.Size());
  p += Align(build_id.Size(), 8);
  this->filename = p;
  strcpy(p, filename.c_str());
  UpdateBinary(new_binary);
}

KernelSymbolRecord::KernelSymbolRecord(char* p) : Record(p) {
  const char* end = p + size();
  p += header_size();
  MoveFromBinaryFormat(kallsyms_size, p);
  kallsyms = p;
  p += Align(kallsyms_size, 8);
  CHECK_EQ(p, end);
}

void KernelSymbolRecord::DumpData(size_t indent) const {
  PrintIndented(indent, "kallsyms: %s\n",
                std::string(kallsyms, kallsyms + kallsyms_size).c_str());
}

KernelSymbolRecord::KernelSymbolRecord(const std::string& kallsyms) {
  SetTypeAndMisc(SIMPLE_PERF_RECORD_KERNEL_SYMBOL, 0);
  kallsyms_size = kallsyms.size();
  SetSize(header_size() + 4 + Align(kallsyms.size(), 8));
  char* new_binary = new char[size()];
  char* p = new_binary;
  MoveToBinaryFormat(header, p);
  MoveToBinaryFormat(kallsyms_size, p);
  this->kallsyms = p;
  memcpy(p, kallsyms.data(), kallsyms_size);
  UpdateBinary(new_binary);
}

DsoRecord::DsoRecord(char* p) : Record(p) {
  const char* end = p + size();
  p += header_size();
  MoveFromBinaryFormat(dso_type, p);
  MoveFromBinaryFormat(dso_id, p);
  MoveFromBinaryFormat(min_vaddr, p);
  dso_name = p;
  p += Align(strlen(dso_name) + 1, 8);
  CHECK_EQ(p, end);
}

DsoRecord::DsoRecord(uint64_t dso_type, uint64_t dso_id,
                     const std::string& dso_name, uint64_t min_vaddr) {
  SetTypeAndMisc(SIMPLE_PERF_RECORD_DSO, 0);
  this->dso_type = dso_type;
  this->dso_id = dso_id;
  this->min_vaddr = min_vaddr;
  SetSize(header_size() + 3 * sizeof(uint64_t) + Align(dso_name.size() + 1, 8));
  char* new_binary = new char[size()];
  char* p = new_binary;
  MoveToBinaryFormat(header, p);
  MoveToBinaryFormat(dso_type, p);
  MoveToBinaryFormat(dso_id, p);
  MoveToBinaryFormat(min_vaddr, p);
  this->dso_name = p;
  strcpy(p, dso_name.c_str());
  UpdateBinary(new_binary);
}

void DsoRecord::DumpData(size_t indent) const {
  PrintIndented(indent, "dso_type: %s(%" PRIu64 ")\n",
                DsoTypeToString(static_cast<DsoType>(dso_type)), dso_type);
  PrintIndented(indent, "dso_id: %" PRIu64 "\n", dso_id);
  PrintIndented(indent, "min_vaddr: 0x%" PRIx64 "\n", min_vaddr);
  PrintIndented(indent, "dso_name: %s\n", dso_name);
}

SymbolRecord::SymbolRecord(char* p) : Record(p) {
  const char* end = p + size();
  p += header_size();
  MoveFromBinaryFormat(addr, p);
  MoveFromBinaryFormat(len, p);
  MoveFromBinaryFormat(dso_id, p);
  name = p;
  p += Align(strlen(name) + 1, 8);
  CHECK_EQ(p, end);
}

SymbolRecord::SymbolRecord(uint64_t addr, uint64_t len, const std::string& name,
                           uint64_t dso_id) {
  SetTypeAndMisc(SIMPLE_PERF_RECORD_SYMBOL, 0);
  this->addr = addr;
  this->len = len;
  this->dso_id = dso_id;
  SetSize(header_size() + 3 * sizeof(uint64_t) + Align(name.size() + 1, 8));
  char* new_binary = new char[size()];
  char* p = new_binary;
  MoveToBinaryFormat(header, p);
  MoveToBinaryFormat(addr, p);
  MoveToBinaryFormat(len, p);
  MoveToBinaryFormat(dso_id, p);
  this->name = p;
  strcpy(p, name.c_str());
  UpdateBinary(new_binary);
}

void SymbolRecord::DumpData(size_t indent) const {
  PrintIndented(indent, "name: %s\n", name);
  PrintIndented(indent, "addr: 0x%" PRIx64 "\n", addr);
  PrintIndented(indent, "len: 0x%" PRIx64 "\n", len);
  PrintIndented(indent, "dso_id: %" PRIu64 "\n", dso_id);
}

TracingDataRecord::TracingDataRecord(char* p) : Record(p) {
  const char* end = p + size();
  p += header_size();
  MoveFromBinaryFormat(data_size, p);
  data = p;
  p += Align(data_size, 64);
  CHECK_EQ(p, end);
}

TracingDataRecord::TracingDataRecord(const std::vector<char>& tracing_data) {
  SetTypeAndMisc(PERF_RECORD_TRACING_DATA, 0);
  data_size = tracing_data.size();
  SetSize(header_size() + sizeof(uint32_t) + Align(tracing_data.size(), 64));
  char* new_binary = new char[size()];
  char* p = new_binary;
  MoveToBinaryFormat(header, p);
  MoveToBinaryFormat(data_size, p);
  data = p;
  memcpy(p, tracing_data.data(), data_size);
  UpdateBinary(new_binary);
}

void TracingDataRecord::DumpData(size_t indent) const {
  Tracing tracing(std::vector<char>(data, data + data_size));
  tracing.Dump(indent);
}

EventIdRecord::EventIdRecord(char* p) : Record(p) {
  const char* end = p + size();
  p += header_size();
  MoveFromBinaryFormat(count, p);
  data = reinterpret_cast<const EventIdData*>(p);
  p += sizeof(data[0]) * count;
  CHECK_EQ(p, end);
}

EventIdRecord::EventIdRecord(const std::vector<uint64_t>& data) {
  SetTypeAndMisc(SIMPLE_PERF_RECORD_EVENT_ID, 0);
  SetSize(header_size() + sizeof(uint64_t) * (1 + data.size()));
  char* new_binary = new char[size()];
  char* p = new_binary;
  MoveToBinaryFormat(header, p);
  count = data.size() / 2;
  MoveToBinaryFormat(count, p);
  this->data = reinterpret_cast<EventIdData*>(p);
  memcpy(p, data.data(), sizeof(uint64_t) * data.size());
  UpdateBinary(new_binary);
}

void EventIdRecord::DumpData(size_t indent) const {
  PrintIndented(indent, "count: %" PRIu64 "\n", count);
  for (size_t i = 0; i < count; ++i) {
    PrintIndented(indent, "attr_id[%" PRIu64 "]: %" PRIu64 "\n", i,
                  data[i].attr_id);
    PrintIndented(indent, "event_id[%" PRIu64 "]: %" PRIu64 "\n", i,
                  data[i].event_id);
  }
}

CallChainRecord::CallChainRecord(char* p) : Record(p) {
  const char* end = p + size();
  p += header_size();
  MoveFromBinaryFormat(pid, p);
  MoveFromBinaryFormat(tid, p);
  MoveFromBinaryFormat(chain_type, p);
  MoveFromBinaryFormat(time, p);
  MoveFromBinaryFormat(ip_nr, p);
  ips = reinterpret_cast<uint64_t*>(p);
  p += ip_nr * sizeof(uint64_t);
  sps = reinterpret_cast<uint64_t*>(p);
  p += ip_nr * sizeof(uint64_t);
  CHECK_EQ(p, end);
}

CallChainRecord::CallChainRecord(pid_t pid, pid_t tid, CallChainJoiner::ChainType type,
                                 uint64_t time, const std::vector<uint64_t>& ips,
                                 const std::vector<uint64_t>& sps) {
  CHECK_EQ(ips.size(), sps.size());
  SetTypeAndMisc(SIMPLE_PERF_RECORD_CALLCHAIN, 0);
  this->pid = pid;
  this->tid = tid;
  this->chain_type = static_cast<int>(type);
  this->time = time;
  this->ip_nr = ips.size();
  SetSize(header_size() + (4 + ips.size() * 2) * sizeof(uint64_t));
  char* new_binary = new char[size()];
  char* p = new_binary;
  MoveToBinaryFormat(header, p);
  MoveToBinaryFormat(this->pid, p);
  MoveToBinaryFormat(this->tid, p);
  MoveToBinaryFormat(this->chain_type, p);
  MoveToBinaryFormat(this->time, p);
  MoveToBinaryFormat(this->ip_nr, p);
  this->ips = reinterpret_cast<uint64_t*>(p);
  MoveToBinaryFormat(ips.data(), ips.size(), p);
  this->sps = reinterpret_cast<uint64_t*>(p);
  MoveToBinaryFormat(sps.data(), sps.size(), p);
  UpdateBinary(new_binary);
}

void CallChainRecord::DumpData(size_t indent) const {
  const char* type_name = "";
  switch (chain_type) {
    case CallChainJoiner::ORIGINAL_OFFLINE: type_name = "ORIGINAL_OFFLINE"; break;
    case CallChainJoiner::ORIGINAL_REMOTE: type_name = "ORIGINAL_REMOTE"; break;
    case CallChainJoiner::JOINED_OFFLINE: type_name = "JOINED_OFFLINE"; break;
    case CallChainJoiner::JOINED_REMOTE: type_name = "JOINED_REMOTE"; break;
  }
  PrintIndented(indent, "pid %u\n", pid);
  PrintIndented(indent, "tid %u\n", tid);
  PrintIndented(indent, "chain_type %s\n", type_name);
  PrintIndented(indent, "time %" PRIu64 "\n", time);
  PrintIndented(indent, "ip_nr %" PRIu64 "\n", ip_nr);
  for (size_t i = 0; i < ip_nr; ++i) {
    PrintIndented(indent + 1, "ip 0x%" PRIx64 ", sp 0x%" PRIx64 "\n", ips[i], sps[i]);
  }
}

UnwindingResultRecord::UnwindingResultRecord(char* p) : Record(p) {
  const char* end = p + size();
  p += header_size();
  MoveFromBinaryFormat(time, p);
  MoveFromBinaryFormat(unwinding_result.used_time, p);
  uint64_t stop_reason;
  MoveFromBinaryFormat(stop_reason, p);
  unwinding_result.stop_reason = static_cast<decltype(unwinding_result.stop_reason)>(stop_reason);
  MoveFromBinaryFormat(unwinding_result.stop_info, p);
  MoveFromBinaryFormat(unwinding_result.stack_start, p);
  MoveFromBinaryFormat(unwinding_result.stack_end, p);
  CHECK_EQ(p, end);
}

UnwindingResultRecord::UnwindingResultRecord(uint64_t time,
                                             const UnwindingResult& unwinding_result) {
  SetTypeAndMisc(SIMPLE_PERF_RECORD_UNWINDING_RESULT, 0);
  SetSize(header_size() + 6 * sizeof(uint64_t));
  this->time = time;
  this->unwinding_result = unwinding_result;
  char* new_binary = new char[size()];
  char* p = new_binary;
  MoveToBinaryFormat(header, p);
  MoveToBinaryFormat(this->time, p);
  MoveToBinaryFormat(unwinding_result.used_time, p);
  uint64_t stop_reason = unwinding_result.stop_reason;
  MoveToBinaryFormat(stop_reason, p);
  MoveToBinaryFormat(unwinding_result.stop_info, p);
  MoveToBinaryFormat(unwinding_result.stack_start, p);
  MoveToBinaryFormat(unwinding_result.stack_end, p);
  UpdateBinary(new_binary);
}

void UnwindingResultRecord::DumpData(size_t indent) const {
  PrintIndented(indent, "time %" PRIu64 "\n", time);
  PrintIndented(indent, "used_time %" PRIu64 "\n", unwinding_result.used_time);
  static std::unordered_map<int, std::string> map = {
      {UnwindingResult::UNKNOWN_REASON, "UNKNOWN_REASON"},
      {UnwindingResult::EXCEED_MAX_FRAMES_LIMIT, "EXCEED_MAX_FRAME_LIMIT"},
      {UnwindingResult::ACCESS_REG_FAILED, "ACCESS_REG_FAILED"},
      {UnwindingResult::ACCESS_STACK_FAILED, "ACCESS_STACK_FAILED"},
      {UnwindingResult::ACCESS_MEM_FAILED, "ACCESS_MEM_FAILED"},
      {UnwindingResult::FIND_PROC_INFO_FAILED, "FIND_PROC_INFO_FAILED"},
      {UnwindingResult::EXECUTE_DWARF_INSTRUCTION_FAILED, "EXECUTE_DWARF_INSTRUCTION_FAILED"},
      {UnwindingResult::DIFFERENT_ARCH, "DIFFERENT_ARCH"},
      {UnwindingResult::MAP_MISSING, "MAP_MISSING"},
  };
  PrintIndented(indent, "stop_reason %s\n", map[unwinding_result.stop_reason].c_str());
  if (unwinding_result.stop_reason == UnwindingResult::ACCESS_REG_FAILED) {
    PrintIndented(indent, "regno %" PRIu64 "\n", unwinding_result.stop_info);
  } else if (unwinding_result.stop_reason == UnwindingResult::ACCESS_STACK_FAILED ||
             unwinding_result.stop_reason == UnwindingResult::ACCESS_MEM_FAILED) {
    PrintIndented(indent, "addr 0x%" PRIx64 "\n", unwinding_result.stop_info);
  }
  PrintIndented(indent, "stack_start 0x%" PRIx64 "\n", unwinding_result.stack_start);
  PrintIndented(indent, "stack_end 0x%" PRIx64 "\n", unwinding_result.stack_end);
}

UnknownRecord::UnknownRecord(char* p) : Record(p) {
  p += header_size();
  data = p;
}

void UnknownRecord::DumpData(size_t) const {}

std::unique_ptr<Record> ReadRecordFromBuffer(const perf_event_attr& attr, uint32_t type, char* p) {
  switch (type) {
    case PERF_RECORD_MMAP:
      return std::unique_ptr<Record>(new MmapRecord(attr, p));
    case PERF_RECORD_MMAP2:
      return std::unique_ptr<Record>(new Mmap2Record(attr, p));
    case PERF_RECORD_COMM:
      return std::unique_ptr<Record>(new CommRecord(attr, p));
    case PERF_RECORD_EXIT:
      return std::unique_ptr<Record>(new ExitRecord(attr, p));
    case PERF_RECORD_FORK:
      return std::unique_ptr<Record>(new ForkRecord(attr, p));
    case PERF_RECORD_LOST:
      return std::unique_ptr<Record>(new LostRecord(attr, p));
    case PERF_RECORD_SAMPLE:
      return std::unique_ptr<Record>(new SampleRecord(attr, p));
    case PERF_RECORD_TRACING_DATA:
      return std::unique_ptr<Record>(new TracingDataRecord(p));
    case SIMPLE_PERF_RECORD_KERNEL_SYMBOL:
      return std::unique_ptr<Record>(new KernelSymbolRecord(p));
    case SIMPLE_PERF_RECORD_DSO:
      return std::unique_ptr<Record>(new DsoRecord(p));
    case SIMPLE_PERF_RECORD_SYMBOL:
      return std::unique_ptr<Record>(new SymbolRecord(p));
    case SIMPLE_PERF_RECORD_EVENT_ID:
      return std::unique_ptr<Record>(new EventIdRecord(p));
    case SIMPLE_PERF_RECORD_CALLCHAIN:
      return std::unique_ptr<Record>(new CallChainRecord(p));
    case SIMPLE_PERF_RECORD_UNWINDING_RESULT:
      return std::unique_ptr<Record>(new UnwindingResultRecord(p));
    default:
      return std::unique_ptr<Record>(new UnknownRecord(p));
  }
}

std::unique_ptr<Record> ReadRecordFromOwnedBuffer(const perf_event_attr& attr,
                                                  uint32_t type, char* p) {
  std::unique_ptr<Record> record = ReadRecordFromBuffer(attr, type, p);
  if (record != nullptr) {
    record->OwnBinary();
  } else {
    delete[] p;
  }
  return record;
}

std::vector<std::unique_ptr<Record>> ReadRecordsFromBuffer(
    const perf_event_attr& attr, char* buf, size_t buf_size) {
  std::vector<std::unique_ptr<Record>> result;
  char* p = buf;
  char* end = buf + buf_size;
  while (p < end) {
    RecordHeader header(p);
    CHECK_LE(p + header.size, end);
    CHECK_NE(0u, header.size);
    result.push_back(ReadRecordFromBuffer(attr, header.type, p));
    p += header.size;
  }
  return result;
}

std::unique_ptr<Record> ReadRecordFromBuffer(const perf_event_attr& attr, char* p) {
  auto header = reinterpret_cast<const perf_event_header*>(p);
  return ReadRecordFromBuffer(attr, header->type, p);
}

bool RecordCache::RecordWithSeq::IsHappensBefore(
    const RecordWithSeq& other) const {
  bool is_sample = (record->type() == PERF_RECORD_SAMPLE);
  bool is_other_sample = (other.record->type() == PERF_RECORD_SAMPLE);
  uint64_t time = record->Timestamp();
  uint64_t other_time = other.record->Timestamp();
  // The record with smaller time happens first.
  if (time != other_time) {
    return time < other_time;
  }
  // If happening at the same time, make non-sample records before sample
  // records, because non-sample records may contain useful information to
  // parse sample records.
  if (is_sample != is_other_sample) {
    return is_sample ? false : true;
  }
  // Otherwise, use the same order as they enter the cache.
  return seq < other.seq;
}

bool RecordCache::RecordComparator::operator()(const RecordWithSeq& r1,
                                               const RecordWithSeq& r2) {
  return r2.IsHappensBefore(r1);
}

RecordCache::RecordCache(bool has_timestamp, size_t min_cache_size,
                         uint64_t min_time_diff_in_ns)
    : has_timestamp_(has_timestamp),
      min_cache_size_(min_cache_size),
      min_time_diff_in_ns_(min_time_diff_in_ns),
      last_time_(0),
      cur_seq_(0),
      queue_(RecordComparator()) {}

RecordCache::~RecordCache() { PopAll(); }

void RecordCache::Push(std::unique_ptr<Record> record) {
  if (has_timestamp_) {
    last_time_ = std::max(last_time_, record->Timestamp());
  }
  queue_.push(RecordWithSeq(cur_seq_++, record.release()));
}

void RecordCache::Push(std::vector<std::unique_ptr<Record>> records) {
  for (auto& r : records) {
    Push(std::move(r));
  }
}

std::unique_ptr<Record> RecordCache::Pop() {
  if (queue_.size() < min_cache_size_) {
    return nullptr;
  }
  Record* r = queue_.top().record;
  if (has_timestamp_) {
    if (r->Timestamp() + min_time_diff_in_ns_ > last_time_) {
      return nullptr;
    }
  }
  queue_.pop();
  return std::unique_ptr<Record>(r);
}

std::vector<std::unique_ptr<Record>> RecordCache::PopAll() {
  std::vector<std::unique_ptr<Record>> result;
  while (!queue_.empty()) {
    result.emplace_back(queue_.top().record);
    queue_.pop();
  }
  return result;
}

std::unique_ptr<Record> RecordCache::ForcedPop() {
  if (queue_.empty()) {
    return nullptr;
  }
  Record* r = queue_.top().record;
  queue_.pop();
  return std::unique_ptr<Record>(r);
}
