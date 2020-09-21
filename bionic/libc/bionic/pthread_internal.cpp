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

#include "pthread_internal.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include <async_safe/log.h>

#include "private/bionic_futex.h"
#include "private/bionic_sdk_version.h"
#include "private/bionic_tls.h"

static pthread_internal_t* g_thread_list = nullptr;
static pthread_rwlock_t g_thread_list_lock = PTHREAD_RWLOCK_INITIALIZER;

template <bool write> class ScopedRWLock {
 public:
  ScopedRWLock(pthread_rwlock_t* rwlock) : rwlock_(rwlock) {
    (write ? pthread_rwlock_wrlock : pthread_rwlock_rdlock)(rwlock_);
  }

  ~ScopedRWLock() {
    pthread_rwlock_unlock(rwlock_);
  }

 private:
  pthread_rwlock_t* rwlock_;
  DISALLOW_IMPLICIT_CONSTRUCTORS(ScopedRWLock);
};

typedef ScopedRWLock<true> ScopedWriteLock;
typedef ScopedRWLock<false> ScopedReadLock;

pthread_t __pthread_internal_add(pthread_internal_t* thread) {
  ScopedWriteLock locker(&g_thread_list_lock);

  // We insert at the head.
  thread->next = g_thread_list;
  thread->prev = nullptr;
  if (thread->next != nullptr) {
    thread->next->prev = thread;
  }
  g_thread_list = thread;
  return reinterpret_cast<pthread_t>(thread);
}

void __pthread_internal_remove(pthread_internal_t* thread) {
  ScopedWriteLock locker(&g_thread_list_lock);

  if (thread->next != nullptr) {
    thread->next->prev = thread->prev;
  }
  if (thread->prev != nullptr) {
    thread->prev->next = thread->next;
  } else {
    g_thread_list = thread->next;
  }
}

static void __pthread_internal_free(pthread_internal_t* thread) {
  if (thread->mmap_size != 0) {
    // Free mapped space, including thread stack and pthread_internal_t.
    munmap(thread->attr.stack_base, thread->mmap_size);
  }
}

void __pthread_internal_remove_and_free(pthread_internal_t* thread) {
  __pthread_internal_remove(thread);
  __pthread_internal_free(thread);
}

pthread_internal_t* __pthread_internal_find(pthread_t thread_id) {
  pthread_internal_t* thread = reinterpret_cast<pthread_internal_t*>(thread_id);

  // Check if we're looking for ourselves before acquiring the lock.
  if (thread == __get_thread()) return thread;

  {
    // Make sure to release the lock before the abort below. Otherwise,
    // some apps might deadlock in their own crash handlers (see b/6565627).
    ScopedReadLock locker(&g_thread_list_lock);
    for (pthread_internal_t* t = g_thread_list; t != nullptr; t = t->next) {
      if (t == thread) return thread;
    }
  }

  // Historically we'd return null, but
  if (bionic_get_application_target_sdk_version() >= __ANDROID_API_O__) {
    if (thread == nullptr) {
      // This seems to be a common mistake, and it's relatively harmless because
      // there will never be a valid thread at address 0, whereas other invalid
      // addresses might sometimes contain threads or things that look enough like
      // threads for us to do some real damage by continuing.
      // TODO: try getting rid of this when Treble lets us keep vendor blobs on an old API level.
      async_safe_format_log(ANDROID_LOG_WARN, "libc", "invalid pthread_t (0) passed to libc");
    } else {
      async_safe_fatal("invalid pthread_t %p passed to libc", thread);
    }
  }
  return nullptr;
}
