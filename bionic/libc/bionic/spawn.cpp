/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include <spawn.h>

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "private/ScopedSignalBlocker.h"
#include "private/SigSetConverter.h"

enum Action {
  kOpen,
  kClose,
  kDup2
};

struct __posix_spawn_file_action {
  __posix_spawn_file_action* next;

  Action what;
  int fd;
  int new_fd;
  char* path;
  int flags;
  mode_t mode;

  void Do() {
    if (what == kOpen) {
      fd = open(path, flags, mode);
      if (fd == -1) _exit(127);
      // If it didn't land where we wanted it, move it.
      if (fd != new_fd) {
        if (dup2(fd, new_fd) == -1) _exit(127);
        close(fd);
      }
    } else if (what == kClose) {
      // Failure to close is ignored.
      close(fd);
    } else {
      if (dup2(fd, new_fd) == -1) _exit(127);
    }
  }
};

struct __posix_spawn_file_actions {
  __posix_spawn_file_action* head;
  __posix_spawn_file_action* last;

  void Do() {
    for (__posix_spawn_file_action* action = head; action != nullptr; action = action->next) {
      action->Do();
    }
  }
};

struct __posix_spawnattr {
  short flags;
  pid_t pgroup;
  sched_param schedparam;
  int schedpolicy;
  SigSetConverter sigmask;
  SigSetConverter sigdefault;
};

static void ApplyAttrs(short flags, const posix_spawnattr_t* attr) {
  // POSIX: "If POSIX_SPAWN_SETSIGDEF is set ... signals in sigdefault ...
  // shall be set to their default actions in the child process."
  // POSIX: "Signals set to be caught by the calling process shall be
  // set to the default action in the child process."
  bool use_sigdefault = ((flags & POSIX_SPAWN_SETSIGDEF) != 0);
  const struct sigaction64 default_sa = { .sa_handler = SIG_DFL };
  for (int s = 1; s < _NSIG; ++s) {
    bool reset = false;
    if (use_sigdefault && sigismember64(&(*attr)->sigdefault.sigset64, s)) {
      reset = true;
    } else {
      struct sigaction64 current;
      if (sigaction64(s, nullptr, &current) == -1) _exit(127);
      reset = (current.sa_handler != SIG_IGN && current.sa_handler != SIG_DFL);
    }
    if (reset && sigaction64(s, &default_sa, nullptr) == -1) _exit(127);
  }

  if ((flags & POSIX_SPAWN_SETPGROUP) != 0 && setpgid(0, (*attr)->pgroup) == -1) _exit(127);
  if ((flags & POSIX_SPAWN_SETSID) != 0 && setsid() == -1) _exit(127);

  // POSIX_SPAWN_SETSCHEDULER overrides POSIX_SPAWN_SETSCHEDPARAM, but it is not an error
  // to set both.
  if ((flags & POSIX_SPAWN_SETSCHEDULER) != 0) {
    if (sched_setscheduler(0, (*attr)->schedpolicy, &(*attr)->schedparam) == -1) _exit(127);
  } else if ((flags & POSIX_SPAWN_SETSCHEDPARAM) != 0) {
    if (sched_setparam(0, &(*attr)->schedparam) == -1) _exit(127);
  }

  if ((flags & POSIX_SPAWN_RESETIDS) != 0) {
    if (seteuid(getuid()) == -1 || setegid(getgid()) == -1) _exit(127);
  }

  if ((flags & POSIX_SPAWN_SETSIGMASK) != 0) {
    if (sigprocmask64(SIG_SETMASK, &(*attr)->sigmask.sigset64, nullptr)) _exit(127);
  }
}

static int posix_spawn(pid_t* pid_ptr,
                       const char* path,
                       const posix_spawn_file_actions_t* actions,
                       const posix_spawnattr_t* attr,
                       char* const argv[],
                       char* const env[],
                       int exec_fn(const char* path, char* const argv[], char* const env[])) {
  // See http://man7.org/linux/man-pages/man3/posix_spawn.3.html
  // and http://pubs.opengroup.org/onlinepubs/9699919799/functions/posix_spawn.html

  ScopedSignalBlocker ssb;

  short flags = attr ? (*attr)->flags : 0;
  bool use_vfork = ((flags & POSIX_SPAWN_USEVFORK) != 0) || (actions == nullptr && flags == 0);

  pid_t pid = use_vfork ? vfork() : fork();
  if (pid == -1) return errno;

  if (pid == 0) {
    // Child.
    ApplyAttrs(flags, attr);
    if (actions) (*actions)->Do();
    if ((flags & POSIX_SPAWN_SETSIGMASK) == 0) ssb.reset();
    exec_fn(path, argv, env ? env : environ);
    _exit(127);
  }

  // Parent.
  if (pid_ptr) *pid_ptr = pid;
  return 0;
}

int posix_spawn(pid_t* pid, const char* path, const posix_spawn_file_actions_t* actions,
                const posix_spawnattr_t* attr, char* const argv[], char* const env[]) {
  return posix_spawn(pid, path, actions, attr, argv, env, execve);
}

int posix_spawnp(pid_t* pid, const char* file, const posix_spawn_file_actions_t* actions,
                 const posix_spawnattr_t* attr, char* const argv[], char* const env[]) {
  return posix_spawn(pid, file, actions, attr, argv, env, execvpe);
}

int posix_spawnattr_init(posix_spawnattr_t* attr) {
  *attr = reinterpret_cast<__posix_spawnattr*>(calloc(1, sizeof(__posix_spawnattr)));
  return (*attr == nullptr) ? errno : 0;
}

int posix_spawnattr_destroy(posix_spawnattr_t* attr) {
  free(*attr);
  *attr = nullptr;
  return 0;
}

int posix_spawnattr_setflags(posix_spawnattr_t* attr, short flags) {
  if ((flags & ~(POSIX_SPAWN_RESETIDS | POSIX_SPAWN_SETPGROUP | POSIX_SPAWN_SETSIGDEF |
                 POSIX_SPAWN_SETSIGMASK | POSIX_SPAWN_SETSCHEDPARAM | POSIX_SPAWN_SETSCHEDULER |
                 POSIX_SPAWN_USEVFORK | POSIX_SPAWN_SETSID)) != 0) {
    return EINVAL;
  }
  (*attr)->flags = flags;
  return 0;
}

int posix_spawnattr_getflags(const posix_spawnattr_t* attr, short* flags) {
  *flags = (*attr)->flags;
  return 0;
}

int posix_spawnattr_setpgroup(posix_spawnattr_t* attr, pid_t pgroup) {
  (*attr)->pgroup = pgroup;
  return 0;
}

int posix_spawnattr_getpgroup(const posix_spawnattr_t* attr, pid_t* pgroup) {
  *pgroup = (*attr)->pgroup;
  return 0;
}

int posix_spawnattr_setsigmask(posix_spawnattr_t* attr, const sigset_t* mask) {
  (*attr)->sigmask.sigset = *mask;
  return 0;
}

int posix_spawnattr_setsigmask64(posix_spawnattr_t* attr, const sigset64_t* mask) {
  (*attr)->sigmask.sigset64 = *mask;
  return 0;
}

int posix_spawnattr_getsigmask(const posix_spawnattr_t* attr, sigset_t* mask) {
  *mask = (*attr)->sigmask.sigset;
  return 0;
}

int posix_spawnattr_getsigmask64(const posix_spawnattr_t* attr, sigset64_t* mask) {
  *mask = (*attr)->sigmask.sigset64;
  return 0;
}

int posix_spawnattr_setsigdefault(posix_spawnattr_t* attr, const sigset_t* mask) {
  (*attr)->sigdefault.sigset = *mask;
  return 0;
}

int posix_spawnattr_setsigdefault64(posix_spawnattr_t* attr, const sigset64_t* mask) {
  (*attr)->sigdefault.sigset64 = *mask;
  return 0;
}

int posix_spawnattr_getsigdefault(const posix_spawnattr_t* attr, sigset_t* mask) {
  *mask = (*attr)->sigdefault.sigset;
  return 0;
}

int posix_spawnattr_getsigdefault64(const posix_spawnattr_t* attr, sigset64_t* mask) {
  *mask = (*attr)->sigdefault.sigset64;
  return 0;
}

int posix_spawnattr_setschedparam(posix_spawnattr_t* attr, const struct sched_param* param) {
  (*attr)->schedparam = *param;
  return 0;
}

int posix_spawnattr_getschedparam(const posix_spawnattr_t* attr, struct sched_param* param) {
  *param = (*attr)->schedparam;
  return 0;
}

int posix_spawnattr_setschedpolicy(posix_spawnattr_t* attr, int policy) {
  (*attr)->schedpolicy = policy;
  return 0;
}

int posix_spawnattr_getschedpolicy(const posix_spawnattr_t* attr, int* policy) {
  *policy = (*attr)->schedpolicy;
  return 0;
}

int posix_spawn_file_actions_init(posix_spawn_file_actions_t* actions) {
  *actions = reinterpret_cast<__posix_spawn_file_actions*>(calloc(1, sizeof(**actions)));
  return (*actions == nullptr) ? errno : 0;
}

int posix_spawn_file_actions_destroy(posix_spawn_file_actions_t* actions) {
  __posix_spawn_file_action* a = (*actions)->head;
  while (a) {
    __posix_spawn_file_action* last = a;
    a = a->next;
    free(last->path);
    free(last);
  }
  free(*actions);
  *actions = nullptr;
  return 0;
}

static int posix_spawn_add_file_action(posix_spawn_file_actions_t* actions,
                                       Action what,
                                       int fd,
                                       int new_fd,
                                       const char* path,
                                       int flags,
                                       mode_t mode) {
  __posix_spawn_file_action* action =
      reinterpret_cast<__posix_spawn_file_action*>(malloc(sizeof(*action)));
  if (action == nullptr) return errno;

  action->next = nullptr;
  if (path != nullptr) {
    action->path = strdup(path);
    if (action->path == nullptr) {
      free(action);
      return errno;
    }
  } else {
    action->path = nullptr;
  }
  action->what = what;
  action->fd = fd;
  action->new_fd = new_fd;
  action->flags = flags;
  action->mode = mode;

  if ((*actions)->head == nullptr) {
    (*actions)->head = (*actions)->last = action;
  } else {
    (*actions)->last->next = action;
    (*actions)->last = action;
  }

  return 0;
}

int posix_spawn_file_actions_addopen(posix_spawn_file_actions_t* actions,
                                     int fd, const char* path, int flags, mode_t mode) {
  if (fd < 0) return EBADF;
  return posix_spawn_add_file_action(actions, kOpen, -1, fd, path, flags, mode);
}

int posix_spawn_file_actions_addclose(posix_spawn_file_actions_t* actions, int fd) {
  if (fd < 0) return EBADF;
  return posix_spawn_add_file_action(actions, kClose, fd, -1, nullptr, 0, 0);
}

int posix_spawn_file_actions_adddup2(posix_spawn_file_actions_t* actions, int fd, int new_fd) {
  if (fd < 0 || new_fd < 0) return EBADF;
  return posix_spawn_add_file_action(actions, kDup2, fd, new_fd, nullptr, 0, 0);
}
