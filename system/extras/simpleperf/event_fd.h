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

#ifndef SIMPLE_PERF_EVENT_FD_H_
#define SIMPLE_PERF_EVENT_FD_H_

#include <sys/types.h>

#include <memory>
#include <string>
#include <vector>

#include <android-base/macros.h>

#include "IOEventLoop.h"
#include "perf_event.h"

struct PerfCounter {
  uint64_t value;  // The value of the event specified by the perf_event_file.
  uint64_t time_enabled;  // The enabled time.
  uint64_t time_running;  // The running time.
  uint64_t id;            // The id of the perf_event_file.
};

// EventFd represents an opened perf_event_file.
class EventFd {
 public:
  static std::unique_ptr<EventFd> OpenEventFile(const perf_event_attr& attr,
                                                pid_t tid, int cpu,
                                                EventFd* group_event_fd,
                                                bool report_error = true);

  ~EventFd();

  // Give information about this perf_event_file, like (event_name, tid, cpu).
  std::string Name() const;

  uint64_t Id() const;

  pid_t ThreadId() const { return tid_; }

  int Cpu() const { return cpu_; }

  const perf_event_attr& attr() const { return attr_; }

  // It tells the kernel to start counting and recording events specified by
  // this file.
  bool EnableEvent();

  bool ReadCounter(PerfCounter* counter);

  // Create mapped buffer used to receive records sent by the kernel.
  // mmap_pages should be power of 2.
  bool CreateMappedBuffer(size_t mmap_pages, bool report_error);

  // Share the mapped buffer used by event_fd. The two EventFds should monitor
  // the same event on the same cpu, but have different thread ids.
  bool ShareMappedBuffer(const EventFd& event_fd, bool report_error);

  bool HasMappedBuffer() const { return mmap_data_buffer_size_ != 0; }

  void DestroyMappedBuffer();

  // When the kernel writes new sampled records to the mapped area, we can get
  // them by returning the start address and size of the data.
  size_t GetAvailableMmapData(std::vector<char>& buffer, size_t& buffer_pos);

  // [callback] is called when there is data available in the mapped buffer.
  bool StartPolling(IOEventLoop& loop, const std::function<bool()>& callback);
  bool StopPolling();

 private:
  EventFd(const perf_event_attr& attr, int perf_event_fd,
          const std::string& event_name, pid_t tid, int cpu)
      : attr_(attr),
        perf_event_fd_(perf_event_fd),
        id_(0),
        event_name_(event_name),
        tid_(tid),
        cpu_(cpu),
        mmap_addr_(nullptr),
        mmap_len_(0),
        mmap_metadata_page_(nullptr),
        mmap_data_buffer_(nullptr),
        mmap_data_buffer_size_(0),
        ioevent_ref_(nullptr),
        last_counter_value_(0) {}

  bool InnerReadCounter(PerfCounter* counter) const;
  // Discard how much data we have read, so the kernel can reuse this part of
  // mapped area to store new data.
  void DiscardMmapData(size_t discard_size);

  const perf_event_attr attr_;
  int perf_event_fd_;
  mutable uint64_t id_;
  const std::string event_name_;
  pid_t tid_;
  int cpu_;

  void* mmap_addr_;
  size_t mmap_len_;
  perf_event_mmap_page* mmap_metadata_page_;  // The first page of mmap_area.
  char* mmap_data_buffer_;  // Starting from the second page of mmap_area,
                            // containing records written by then kernel.
  size_t mmap_data_buffer_size_;

  // As mmap_data_buffer is a ring buffer, it is possible that one record is
  // wrapped at the end of the buffer. So we need to copy records from
  // mmap_data_buffer to data_process_buffer before processing them.
  static std::vector<char> data_process_buffer_;

  IOEventRef ioevent_ref_;

  // Used by atrace to generate value difference between two ReadCounter() calls.
  uint64_t last_counter_value_;

  DISALLOW_COPY_AND_ASSIGN(EventFd);
};

bool IsEventAttrSupported(const perf_event_attr& attr);

#endif  // SIMPLE_PERF_EVENT_FD_H_
