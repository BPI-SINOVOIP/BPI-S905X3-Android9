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

#include "IOEventLoop.h"

#include <event2/event.h>
#include <fcntl.h>

#include <android-base/logging.h>

struct IOEvent {
  IOEventLoop* loop;
  event* e;
  std::function<bool()> callback;
  bool enabled;

  IOEvent(IOEventLoop* loop, const std::function<bool()>& callback)
      : loop(loop), e(nullptr), callback(callback), enabled(false) {}

  ~IOEvent() {
    if (e != nullptr) {
      event_free(e);
    }
  }
};

IOEventLoop::IOEventLoop() : ebase_(nullptr), has_error_(false), use_precise_timer_(false) {}

IOEventLoop::~IOEventLoop() {
  events_.clear();
  if (ebase_ != nullptr) {
    event_base_free(ebase_);
  }
}

bool IOEventLoop::UsePreciseTimer() {
  if (ebase_ != nullptr) {
    return false;  // Too late to set the flag.
  }
  use_precise_timer_ = true;
  return true;
}

bool IOEventLoop::EnsureInit() {
  if (ebase_ == nullptr) {
    event_config* cfg = event_config_new();
    if (cfg != nullptr) {
      if (use_precise_timer_) {
        event_config_set_flag(cfg, EVENT_BASE_FLAG_PRECISE_TIMER);
      }
      ebase_ = event_base_new_with_config(cfg);
      event_config_free(cfg);
    }
    if (ebase_ == nullptr) {
      LOG(ERROR) << "failed to create event_base";
      return false;
    }
  }
  return true;
}

void IOEventLoop::EventCallbackFn(int, short, void* arg) {
  IOEvent* e = static_cast<IOEvent*>(arg);
  if (!e->callback()) {
    e->loop->has_error_ = true;
    e->loop->ExitLoop();
  }
}

static bool MakeFdNonBlocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
    PLOG(ERROR) << "fcntl() failed";
    return false;
  }
  return true;
}

IOEventRef IOEventLoop::AddReadEvent(int fd,
                                     const std::function<bool()>& callback) {
  if (!MakeFdNonBlocking(fd)) {
    return nullptr;
  }
  return AddEvent(fd, EV_READ | EV_PERSIST, nullptr, callback);
}

IOEventRef IOEventLoop::AddWriteEvent(int fd,
                                      const std::function<bool()>& callback) {
  if (!MakeFdNonBlocking(fd)) {
    return nullptr;
  }
  return AddEvent(fd, EV_WRITE | EV_PERSIST, nullptr, callback);
}

bool IOEventLoop::AddSignalEvent(int sig,
                                 const std::function<bool()>& callback) {
  return AddEvent(sig, EV_SIGNAL | EV_PERSIST, nullptr, callback) != nullptr;
}

bool IOEventLoop::AddSignalEvents(std::vector<int> sigs,
                                  const std::function<bool()>& callback) {
  for (auto sig : sigs) {
    if (!AddSignalEvent(sig, callback)) {
      return false;
    }
  }
  return true;
}

bool IOEventLoop::AddPeriodicEvent(timeval duration,
                                   const std::function<bool()>& callback) {
  return AddEvent(-1, EV_PERSIST, &duration, callback) != nullptr;
}

IOEventRef IOEventLoop::AddEvent(int fd_or_sig, short events, timeval* timeout,
                                 const std::function<bool()>& callback) {
  if (!EnsureInit()) {
    return nullptr;
  }
  std::unique_ptr<IOEvent> e(new IOEvent(this, callback));
  e->e = event_new(ebase_, fd_or_sig, events, EventCallbackFn, e.get());
  if (e->e == nullptr) {
    LOG(ERROR) << "event_new() failed";
    return nullptr;
  }
  if (event_add(e->e, timeout) != 0) {
    LOG(ERROR) << "event_add() failed";
    return nullptr;
  }
  e->enabled = true;
  events_.push_back(std::move(e));
  return events_.back().get();
}

bool IOEventLoop::RunLoop() {
  if (event_base_dispatch(ebase_) == -1) {
    LOG(ERROR) << "event_base_dispatch() failed";
    return false;
  }
  if (has_error_) {
    return false;
  }
  return true;
}

bool IOEventLoop::ExitLoop() {
  if (event_base_loopbreak(ebase_) == -1) {
    LOG(ERROR) << "event_base_loopbreak() failed";
    return false;
  }
  return true;
}

bool IOEventLoop::DisableEvent(IOEventRef ref) {
  if (ref->enabled) {
    if (event_del(ref->e) != 0) {
      LOG(ERROR) << "event_del() failed";
      return false;
    }
    ref->enabled = false;
  }
  return true;
}

bool IOEventLoop::EnableEvent(IOEventRef ref) {
  if (!ref->enabled) {
    if (event_add(ref->e, nullptr) != 0) {
      LOG(ERROR) << "event_add() failed";
      return false;
    }
    ref->enabled = true;
  }
  return true;
}

bool IOEventLoop::DelEvent(IOEventRef ref) {
  DisableEvent(ref);
  IOEventLoop* loop = ref->loop;
  for (auto it = loop->events_.begin(); it != loop->events_.end(); ++it) {
    if (it->get() == ref) {
      loop->events_.erase(it);
      break;
    }
  }
  return true;
}
