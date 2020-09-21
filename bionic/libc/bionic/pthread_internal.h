/*
 * Copyright (C) 2008 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#pragma once

#include <pthread.h>
#include <stdatomic.h>

#include "private/bionic_lock.h"
#include "private/bionic_tls.h"

// Has the thread been detached by a pthread_join or pthread_detach call?
#define PTHREAD_ATTR_FLAG_DETACHED 0x00000001

// Has the thread been joined by another thread?
#define PTHREAD_ATTR_FLAG_JOINED 0x00000002

// Used for pthread_attr_setinheritsched. We need two flags for this apparent
// boolean because our historical behavior matches neither of the POSIX choices.
#define PTHREAD_ATTR_FLAG_INHERIT 0x00000004
#define PTHREAD_ATTR_FLAG_EXPLICIT 0x00000008

class pthread_key_data_t {
 public:
  uintptr_t seq; // Use uintptr_t just for alignment, as we use pointer below.
  void* data;
};

enum ThreadJoinState {
  THREAD_NOT_JOINED,
  THREAD_EXITED_NOT_JOINED,
  THREAD_JOINED,
  THREAD_DETACHED
};

class thread_local_dtor;

class pthread_internal_t {
 public:
  class pthread_internal_t* next;
  class pthread_internal_t* prev;

  pid_t tid;

 private:
  pid_t cached_pid_;

 public:
  pid_t invalidate_cached_pid() {
    pid_t old_value;
    get_cached_pid(&old_value);
    set_cached_pid(0);
    return old_value;
  }

  void set_cached_pid(pid_t value) {
    cached_pid_ = value;
  }

  bool get_cached_pid(pid_t* cached_pid) {
    *cached_pid = cached_pid_;
    return (*cached_pid != 0);
  }

  pthread_attr_t attr;

  _Atomic(ThreadJoinState) join_state;

  __pthread_cleanup_t* cleanup_stack;

  void* (*start_routine)(void*);
  void* start_routine_arg;
  void* return_value;

  void* alternate_signal_stack;

  Lock startup_handshake_lock;

  size_t mmap_size;

  thread_local_dtor* thread_local_dtors;

  void* tls[BIONIC_TLS_SLOTS];

  pthread_key_data_t key_data[BIONIC_PTHREAD_KEY_COUNT];

  /*
   * The dynamic linker implements dlerror(3), which makes it hard for us to implement this
   * per-thread buffer by simply using malloc(3) and free(3).
   */
#define __BIONIC_DLERROR_BUFFER_SIZE 512
  char dlerror_buffer[__BIONIC_DLERROR_BUFFER_SIZE];

  bionic_tls* bionic_tls;
};

__LIBC_HIDDEN__ int __init_thread(pthread_internal_t* thread);
__LIBC_HIDDEN__ bool __init_tls(pthread_internal_t* thread);
__LIBC_HIDDEN__ void __init_thread_stack_guard(pthread_internal_t* thread);
__LIBC_HIDDEN__ void __init_alternate_signal_stack(pthread_internal_t*);

__LIBC_HIDDEN__ pthread_t           __pthread_internal_add(pthread_internal_t* thread);
__LIBC_HIDDEN__ pthread_internal_t* __pthread_internal_find(pthread_t pthread_id);
__LIBC_HIDDEN__ void                __pthread_internal_remove(pthread_internal_t* thread);
__LIBC_HIDDEN__ void                __pthread_internal_remove_and_free(pthread_internal_t* thread);

// Make __get_thread() inlined for performance reason. See http://b/19825434.
static inline __always_inline pthread_internal_t* __get_thread() {
  void** tls = __get_tls();
  if (__predict_true(tls)) {
    return reinterpret_cast<pthread_internal_t*>(tls[TLS_SLOT_THREAD_ID]);
  }

  // This happens when called during libc initialization before TLS has been initialized.
  return nullptr;
}

static inline __always_inline bionic_tls& __get_bionic_tls() {
  return *__get_thread()->bionic_tls;
}

__LIBC_HIDDEN__ void pthread_key_clean_all(void);

// Address space is precious on LP32, so use the minimum unit: one page.
// On LP64, we could use more but there's no obvious advantage to doing
// so, and the various media processes use RLIMIT_AS as a way to limit
// the amount of allocation they'll do.
#define PTHREAD_GUARD_SIZE PAGE_SIZE

// SIGSTKSZ (8KiB) is not big enough.
// An snprintf to a stack buffer of size PATH_MAX consumes ~7KiB of stack.
// Also, on 64-bit, logging uses more than 8KiB by itself:
// https://code.google.com/p/android/issues/detail?id=187064
#define SIGNAL_STACK_SIZE_WITHOUT_GUARD (16 * 1024)

// Traditionally we gave threads a 1MiB stack. When we started
// allocating per-thread alternate signal stacks to ease debugging of
// stack overflows, we subtracted the same amount we were using there
// from the default thread stack size. This should keep memory usage
// roughly constant.
#define PTHREAD_STACK_SIZE_DEFAULT ((1 * 1024 * 1024) - SIGNAL_STACK_SIZE_WITHOUT_GUARD)

// Leave room for a guard page in the internally created signal stacks.
#define SIGNAL_STACK_SIZE (SIGNAL_STACK_SIZE_WITHOUT_GUARD + PTHREAD_GUARD_SIZE)

// Needed by fork.
__LIBC_HIDDEN__ extern void __bionic_atfork_run_prepare();
__LIBC_HIDDEN__ extern void __bionic_atfork_run_child();
__LIBC_HIDDEN__ extern void __bionic_atfork_run_parent();
