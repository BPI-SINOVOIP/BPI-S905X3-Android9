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
#define ATRACE_TAG ATRACE_TAG_ALWAYS
#include "event_fd.h"

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <atomic>
#include <memory>
#include <cutils/trace.h>
#include <utils/Trace.h>

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/stringprintf.h>

#include "environment.h"
#include "event_attr.h"
#include "event_type.h"
#include "perf_event.h"
#include "utils.h"

static int perf_event_open(const perf_event_attr& attr, pid_t pid, int cpu,
                           int group_fd, unsigned long flags) {  // NOLINT
  return syscall(__NR_perf_event_open, &attr, pid, cpu, group_fd, flags);
}

std::unique_ptr<EventFd> EventFd::OpenEventFile(const perf_event_attr& attr,
                                                pid_t tid, int cpu,
                                                EventFd* group_event_fd,
                                                bool report_error) {
  std::string event_name = GetEventNameByAttr(attr);
  int group_fd = -1;
  if (group_event_fd != nullptr) {
    group_fd = group_event_fd->perf_event_fd_;
  }
  perf_event_attr real_attr = attr;
  if (attr.freq) {
    uint64_t max_sample_freq;
    if (GetMaxSampleFrequency(&max_sample_freq) && max_sample_freq < attr.sample_freq) {
      static bool warned = false;
      if (!warned) {
        warned = true;
        LOG(INFO) << "Adjust sample freq to max allowed sample freq " << max_sample_freq;
      }
      real_attr.sample_freq = max_sample_freq;
    }
  }
  int perf_event_fd = perf_event_open(real_attr, tid, cpu, group_fd, 0);
  if (perf_event_fd == -1) {
    if (report_error) {
      PLOG(ERROR) << "open perf_event_file (event " << event_name << ", tid "
                  << tid << ", cpu " << cpu << ", group_fd " << group_fd
                  << ") failed";
    } else {
      PLOG(DEBUG) << "open perf_event_file (event " << event_name << ", tid "
                  << tid << ", cpu " << cpu << ", group_fd " << group_fd
                  << ") failed";
    }
    return nullptr;
  }
  if (fcntl(perf_event_fd, F_SETFD, FD_CLOEXEC) == -1) {
    if (report_error) {
      PLOG(ERROR) << "fcntl(FD_CLOEXEC) for perf_event_file (event "
                  << event_name << ", tid " << tid << ", cpu " << cpu
                  << ", group_fd " << group_fd << ") failed";
    } else {
      PLOG(DEBUG) << "fcntl(FD_CLOEXEC) for perf_event_file (event "
                  << event_name << ", tid " << tid << ", cpu " << cpu
                  << ", group_fd " << group_fd << ") failed";
    }
    return nullptr;
  }
  return std::unique_ptr<EventFd>(
      new EventFd(real_attr, perf_event_fd, event_name, tid, cpu));
}

EventFd::~EventFd() {
  DestroyMappedBuffer();
  close(perf_event_fd_);
}

std::string EventFd::Name() const {
  return android::base::StringPrintf(
      "perf_event_file(event %s, tid %d, cpu %d)", event_name_.c_str(), tid_,
      cpu_);
}

uint64_t EventFd::Id() const {
  if (id_ == 0) {
    PerfCounter counter;
    if (InnerReadCounter(&counter)) {
      id_ = counter.id;
    }
  }
  return id_;
}

bool EventFd::EnableEvent() {
  int result = ioctl(perf_event_fd_, PERF_EVENT_IOC_ENABLE, 0);
  if (result < 0) {
    PLOG(ERROR) << "ioctl(enable) " << Name() << " failed";
    return false;
  }
  return true;
}

bool EventFd::InnerReadCounter(PerfCounter* counter) const {
  CHECK(counter != nullptr);
  if (!android::base::ReadFully(perf_event_fd_, counter, sizeof(*counter))) {
    PLOG(ERROR) << "ReadCounter from " << Name() << " failed";
    return false;
  }
  return true;
}

bool EventFd::ReadCounter(PerfCounter* counter) {
  if (!InnerReadCounter(counter)) {
    return false;
  }
  // Trace is always available to systrace if enabled
  if (tid_ > 0) {
    ATRACE_INT64(android::base::StringPrintf(
                   "%s_tid%d_cpu%d", event_name_.c_str(), tid_,
                   cpu_).c_str(), counter->value - last_counter_value_);
  } else {
    ATRACE_INT64(android::base::StringPrintf(
                   "%s_cpu%d", event_name_.c_str(),
                   cpu_).c_str(), counter->value - last_counter_value_);
  }
  last_counter_value_ = counter->value;
  return true;
}

bool EventFd::CreateMappedBuffer(size_t mmap_pages, bool report_error) {
  CHECK(IsPowerOfTwo(mmap_pages));
  size_t page_size = sysconf(_SC_PAGE_SIZE);
  size_t mmap_len = (mmap_pages + 1) * page_size;
  void* mmap_addr = mmap(nullptr, mmap_len, PROT_READ | PROT_WRITE, MAP_SHARED,
                         perf_event_fd_, 0);
  if (mmap_addr == MAP_FAILED) {
    bool is_perm_error = (errno == EPERM);
    if (report_error) {
      PLOG(ERROR) << "mmap(" << mmap_pages << ") failed for " << Name();
    } else {
      PLOG(DEBUG) << "mmap(" << mmap_pages << ") failed for " << Name();
    }
    if (report_error && is_perm_error) {
      LOG(ERROR)
          << "It seems the kernel doesn't allow allocating enough "
          << "buffer for dumping samples, consider decreasing mmap pages(-m).";
    }
    return false;
  }
  mmap_addr_ = mmap_addr;
  mmap_len_ = mmap_len;
  mmap_metadata_page_ = reinterpret_cast<perf_event_mmap_page*>(mmap_addr_);
  mmap_data_buffer_ = reinterpret_cast<char*>(mmap_addr_) + page_size;
  mmap_data_buffer_size_ = mmap_len_ - page_size;
  return true;
}

bool EventFd::ShareMappedBuffer(const EventFd& event_fd, bool report_error) {
  CHECK(!HasMappedBuffer());
  CHECK(event_fd.HasMappedBuffer());
  int result =
      ioctl(perf_event_fd_, PERF_EVENT_IOC_SET_OUTPUT, event_fd.perf_event_fd_);
  if (result != 0) {
    if (report_error) {
      PLOG(ERROR) << "failed to share mapped buffer of "
                  << event_fd.perf_event_fd_ << " with " << perf_event_fd_;
    }
    return false;
  }
  return true;
}

void EventFd::DestroyMappedBuffer() {
  if (HasMappedBuffer()) {
    munmap(mmap_addr_, mmap_len_);
    mmap_addr_ = nullptr;
    mmap_len_ = 0;
    mmap_metadata_page_ = nullptr;
    mmap_data_buffer_ = nullptr;
    mmap_data_buffer_size_ = 0;
  }
}

size_t EventFd::GetAvailableMmapData(std::vector<char>& buffer, size_t& buffer_pos) {
  if (!HasMappedBuffer()) {
    return 0;
  }
  // The mmap_data_buffer is used as a ring buffer between the kernel and
  // simpleperf. The kernel continuously writes records to the buffer, and
  // simpleperf continuously read records out.
  //         _________________________________________
  // buffer | can write   |   can read   |  can write |
  //                      ^              ^
  //                    read_head       write_head
  //
  // So simpleperf can read records in [read_head, write_head), and the kernel
  // can write records in [write_head, read_head). The kernel is responsible
  // for updating write_head, and simpleperf is responsible for updating
  // read_head.

  size_t buf_mask = mmap_data_buffer_size_ - 1;
  size_t write_head =
      static_cast<size_t>(mmap_metadata_page_->data_head & buf_mask);
  size_t read_head =
      static_cast<size_t>(mmap_metadata_page_->data_tail & buf_mask);

  if (read_head == write_head) {
    // No available data.
    return 0;
  }
  size_t read_bytes;
  if (read_head < write_head) {
    read_bytes = write_head - read_head;
  } else {
    read_bytes = mmap_data_buffer_size_ - read_head + write_head;
  }
  // Extend the buffer if it is not big enough.
  if (buffer.size() < buffer_pos + read_bytes) {
    buffer.resize(buffer_pos + read_bytes);
  }

  // rmb() used to ensure reading data after reading data_head.
  __sync_synchronize();

  // Copy records from mapped buffer. Note that records can be wrapped at the
  // end of the mapped buffer.
  char* to = &buffer[buffer_pos];
  if (read_head < write_head) {
    char* from = mmap_data_buffer_ + read_head;
    size_t n = write_head - read_head;
    memcpy(to, from, n);
  } else {
    char* from = mmap_data_buffer_ + read_head;
    size_t n = mmap_data_buffer_size_ - read_head;
    memcpy(to, from, n);
    to += n;
    from = mmap_data_buffer_;
    n = write_head;
    memcpy(to, from, n);
  }
  buffer_pos += read_bytes;
  DiscardMmapData(read_bytes);
  return read_bytes;
}

void EventFd::DiscardMmapData(size_t discard_size) {
  // mb() used to ensure finish reading data before writing data_tail.
  __sync_synchronize();
  mmap_metadata_page_->data_tail += discard_size;
}

bool EventFd::StartPolling(IOEventLoop& loop,
                           const std::function<bool()>& callback) {
  ioevent_ref_ = loop.AddReadEvent(perf_event_fd_, callback);
  return ioevent_ref_ != nullptr;
}

bool EventFd::StopPolling() { return IOEventLoop::DelEvent(ioevent_ref_); }

bool IsEventAttrSupported(const perf_event_attr& attr) {
  if (attr.type == SIMPLEPERF_TYPE_USER_SPACE_SAMPLERS &&
      attr.config == SIMPLEPERF_CONFIG_INPLACE_SAMPLER) {
    // User space samplers don't need kernel support.
    return true;
  }
  std::unique_ptr<EventFd> event_fd = EventFd::OpenEventFile(attr, getpid(), -1, nullptr, false);
  return event_fd != nullptr;
}
