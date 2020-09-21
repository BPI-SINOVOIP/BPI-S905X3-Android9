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

#include "libc_init_common.h"

#include "private/KernelArgumentBlock.h"
#include "private/bionic_arc4random.h"
#include "private/bionic_auxv.h"
#include "private/bionic_defs.h"
#include "private/bionic_globals.h"
#include "private/bionic_ssp.h"
#include "pthread_internal.h"

extern "C" int __set_tls(void* ptr);
extern "C" int __set_tid_address(int* tid_address);

// Declared in "private/bionic_ssp.h".
uintptr_t __stack_chk_guard = 0;

void __libc_init_global_stack_chk_guard(KernelArgumentBlock& args) {
  __libc_safe_arc4random_buf(&__stack_chk_guard, sizeof(__stack_chk_guard), args);
}

// Setup for the main thread. For dynamic executables, this is called by the
// linker _before_ libc is mapped in memory. This means that all writes to
// globals from this function will apply to linker-private copies and will not
// be visible from libc later on.
//
// Note: this function creates a pthread_internal_t for the initial thread and
// stores the pointer in TLS, but does not add it to pthread's thread list. This
// has to be done later from libc itself (see __libc_init_common).
//
// This is in a file by itself because it needs to be built with
// -fno-stack-protector because it's responsible for setting up the main
// thread's TLS (which stack protector relies on).

__BIONIC_WEAK_FOR_NATIVE_BRIDGE
void __libc_init_main_thread(KernelArgumentBlock& args) {
  __libc_auxv = args.auxv;
#if defined(__i386__)
  __libc_init_sysinfo(args);
#endif

  static pthread_internal_t main_thread;

  // The -fstack-protector implementation uses TLS, so make sure that's
  // set up before we call any function that might get a stack check inserted.
  // TLS also needs to be set up before errno (and therefore syscalls) can be used.
  __set_tls(main_thread.tls);
  if (!__init_tls(&main_thread)) async_safe_fatal("failed to initialize TLS: %s", strerror(errno));

  // Tell the kernel to clear our tid field when we exit, so we're like any other pthread.
  // As a side-effect, this tells us our pid (which is the same as the main thread's tid).
  main_thread.tid = __set_tid_address(&main_thread.tid);
  main_thread.set_cached_pid(main_thread.tid);

  // We don't want to free the main thread's stack even when the main thread exits
  // because things like environment variables with global scope live on it.
  // We also can't free the pthread_internal_t itself, since that lives on the main
  // thread's stack rather than on the heap.
  // The main thread has no mmap allocated space for stack or pthread_internal_t.
  main_thread.mmap_size = 0;

  pthread_attr_init(&main_thread.attr);
  // We don't want to explicitly set the main thread's scheduler attributes (http://b/68328561).
  pthread_attr_setinheritsched(&main_thread.attr, PTHREAD_INHERIT_SCHED);
  // The main thread has no guard page.
  pthread_attr_setguardsize(&main_thread.attr, 0);
  // User code should never see this; we'll compute it when asked.
  pthread_attr_setstacksize(&main_thread.attr, 0);

  // The TLS stack guard is set from the global, so ensure that we've initialized the global
  // before we initialize the TLS. Dynamic executables will initialize their copy of the global
  // stack protector from the one in the main thread's TLS.
  __libc_init_global_stack_chk_guard(args);
  __init_thread_stack_guard(&main_thread);

  __init_thread(&main_thread);

  // Store a pointer to the kernel argument block in a TLS slot to be
  // picked up by the libc constructor.
  main_thread.tls[TLS_SLOT_BIONIC_PREINIT] = &args;

  __init_alternate_signal_stack(&main_thread);
}
