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

#include <signal.h>

#include "private/sigrtmin.h"
#include "private/SigSetConverter.h"

extern "C" int __rt_sigprocmask(int, const sigset64_t*, sigset64_t*, size_t);

//
// These need to be kept separate from pthread_sigmask, sigblock, sigsetmask,
// sighold, and sigset because libsigchain only intercepts sigprocmask so we
// can't allow clang to decide to inline sigprocmask.
//

int sigprocmask(int how,
                const sigset_t* bionic_new_set,
                sigset_t* bionic_old_set) __attribute__((__noinline__)) {
  SigSetConverter new_set;
  sigset64_t* new_set_ptr = nullptr;
  if (bionic_new_set != nullptr) {
    sigemptyset64(&new_set.sigset64);
    new_set.sigset = *bionic_new_set;
    new_set_ptr = &new_set.sigset64;
  }

  SigSetConverter old_set;
  if (sigprocmask64(how, new_set_ptr, &old_set.sigset64) == -1) {
    return -1;
  }

  if (bionic_old_set != nullptr) {
    *bionic_old_set = old_set.sigset;
  }

  return 0;
}

int sigprocmask64(int how,
                  const sigset64_t* new_set,
                  sigset64_t* old_set) __attribute__((__noinline__)) {
  sigset64_t mutable_new_set;
  sigset64_t* mutable_new_set_ptr = nullptr;
  if (new_set) {
    mutable_new_set = filter_reserved_signals(*new_set);
    mutable_new_set_ptr = &mutable_new_set;
  }
  return __rt_sigprocmask(how, mutable_new_set_ptr, old_set, sizeof(*new_set));
}
