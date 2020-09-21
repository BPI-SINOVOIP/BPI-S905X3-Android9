// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if !defined(__clang__)
#error "Non-clang isn't supported"
#endif

// Clang compile-time and run-time tests for glibc FORTIFY.
//
// This file is compiled in two configurations ways to give us a sane set of
// tests for clang's FORTIFY implementation.
//
// One configuration uses clang's diagnostic consumer
// (https://clang.llvm.org/doxygen/classclang_1_1VerifyDiagnosticConsumer.html#details)
// to check diagnostics (e.g. the expected-* comments everywhere).
//
// The other configuration builds this file such that the resultant object file
// exports a function named test_fortify_1 or test_fortify_2, depending on the
// FORTIFY level we're using. These are called by clang_fortify_driver.cpp.
//
// Please note that this test does things like leaking memory. That's WAI.

// Silence all "from 'diagnose_if'" `note`s from anywhere, including headers;
// they're uninteresting for this test case, and their line numbers may change
// over time.
// expected-note@* 0+{{from 'diagnose_if'}}
//
// Similarly, there are a few overload tricks we have to emit errors. Ignore any
// notes from those.
// expected-note@* 0+{{candidate function}}

// Must come before stdlib.h
#include <limits.h>

#include <err.h>
#include <fcntl.h>
#include <mqueue.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <wchar.h>
#include <vector>

#include "clang-fortify-common.h"

// We're going to use deprecated APIs here (e.g. getwd). That's OK.
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

#ifndef _FORTIFY_SOURCE
#error "_FORTIFY_SOURCE must be defined"
#endif

/////////////////// Test infrastructure; nothing to see here ///////////////////

// GTest doesn't seem to have an EXPECT_NO_DEATH equivalent, and this all seems
// easy enough to hand-roll in a simple environment.

// Failures get stored here.
static std::vector<Failure> *failures;

template <typename Fn>
static void ForkAndExpect(int line, const char *message, Fn &&F,
                          bool expect_death) {
  fprintf(stderr, "Running %s... (expected to %s)\n", message,
          expect_death ? "die" : "not die");

  int pid = fork();
  if (pid == -1)
    err(1, "Failed to fork() a subproc");

  if (pid == 0) {
    F();
    exit(0);
  }

  int status;
  if (waitpid(pid, &status, 0) == -1)
    err(1, "Failed to wait on child (pid %d)", pid);

  bool died = WIFSIGNALED(status) || WEXITSTATUS(status) != 0;
  if (died != expect_death) {
    fprintf(stderr, "Check `%s` (at line %d) %s\n", message, line,
            expect_death ? "failed to die" : "died");
    failures->push_back({line, message, expect_death});
  }
}

#define FORK_AND_EXPECT(x, die) ForkAndExpect(__LINE__, #x, [&] { (x); }, die)

// EXPECT_NO_DEATH forks so that the test remains alive on a bug, and so that
// the environment doesn't get modified on no bug. (Environment modification is
// especially tricky to deal with given the *_STRUCT variants below.)
#define EXPECT_NO_DEATH(x) FORK_AND_EXPECT(x, false)
#define EXPECT_DEATH(x) FORK_AND_EXPECT(x, true)

// Expecting death, but only if we're doing a "strict" struct-checking mode.
#if _FORTIFY_SOURCE > 1
#define EXPECT_DEATH_STRUCT(x) EXPECT_DEATH(x)
#else
#define EXPECT_DEATH_STRUCT(x) EXPECT_NO_DEATH(x)
#endif

//////////////////////////////// FORTIFY tests! ////////////////////////////////

// FIXME(gbiv): glibc shouldn't #define this with FORTIFY on.
#undef mempcpy

const static int kBogusFD = -1;

static void TestString() {
  char small_buffer[8] = {};

  {
    char large_buffer[sizeof(small_buffer) + 1] = {};
    // expected-warning@+1{{called with bigger length than the destination}}
    EXPECT_DEATH(memcpy(small_buffer, large_buffer, sizeof(large_buffer)));
    // expected-warning@+1{{called with bigger length than the destination}}
    EXPECT_DEATH(memmove(small_buffer, large_buffer, sizeof(large_buffer)));
    // expected-warning@+1{{called with bigger length than the destination}}
    EXPECT_DEATH(mempcpy(small_buffer, large_buffer, sizeof(large_buffer)));
    // expected-warning@+1{{called with bigger length than the destination}}
    EXPECT_DEATH(memset(small_buffer, 0, sizeof(large_buffer)));
    // expected-warning@+1{{transposed parameters}}
    memset(small_buffer, sizeof(small_buffer), 0);
    // expected-warning@+1{{called with bigger length than the destination}}
    EXPECT_DEATH(bcopy(large_buffer, small_buffer, sizeof(large_buffer)));
    // expected-warning@+1{{called with bigger length than the destination}}
    EXPECT_DEATH(bzero(small_buffer, sizeof(large_buffer)));
  }

  {
    const char large_string[] = "Hello!!!";
    _Static_assert(sizeof(large_string) > sizeof(small_buffer), "");

    // expected-warning@+1{{destination buffer will always be overflown}}
    EXPECT_DEATH(strcpy(small_buffer, large_string));
    // expected-warning@+1{{destination buffer will always be overflown}}
    EXPECT_DEATH(stpcpy(small_buffer, large_string));
    // expected-warning@+1{{called with bigger length than the destination}}
    EXPECT_DEATH(strncpy(small_buffer, large_string, sizeof(large_string)));
    // FIXME(gbiv): Clang (and GCC, for that matter) should diagnose this.
    EXPECT_DEATH(stpncpy(small_buffer, large_string, sizeof(large_string)));
    // expected-warning@+1{{destination buffer will always be overflown}}
    EXPECT_DEATH(strcat(small_buffer, large_string));
    // expected-warning@+1{{called with bigger length than the destination}}
    EXPECT_DEATH(strncat(small_buffer, large_string, sizeof(large_string)));
  }

  {
    struct {
      char tiny_buffer[4];
      char tiny_buffer2[4];
    } split = {};

    EXPECT_NO_DEATH(memcpy(split.tiny_buffer, &split, sizeof(split)));
    EXPECT_NO_DEATH(memcpy(split.tiny_buffer, &split, sizeof(split)));
    EXPECT_NO_DEATH(memmove(split.tiny_buffer, &split, sizeof(split)));
    EXPECT_NO_DEATH(mempcpy(split.tiny_buffer, &split, sizeof(split)));
    EXPECT_NO_DEATH(memset(split.tiny_buffer, 0, sizeof(split)));

    EXPECT_NO_DEATH(bcopy(&split, split.tiny_buffer, sizeof(split)));
    EXPECT_NO_DEATH(bzero(split.tiny_buffer, sizeof(split)));

    const char small_string[] = "Hi!!";
    _Static_assert(sizeof(small_string) > sizeof(split.tiny_buffer), "");

#if _FORTIFY_SOURCE > 1
    // expected-warning@+2{{destination buffer will always be overflown}}
#endif
    EXPECT_DEATH_STRUCT(strcpy(split.tiny_buffer, small_string));

#if _FORTIFY_SOURCE > 1
    // expected-warning@+2{{destination buffer will always be overflown}}
#endif
    EXPECT_DEATH_STRUCT(stpcpy(split.tiny_buffer, small_string));

#if _FORTIFY_SOURCE > 1
    // expected-warning@+2{{called with bigger length than the destination}}
#endif
    EXPECT_DEATH_STRUCT(
        strncpy(split.tiny_buffer, small_string, sizeof(small_string)));

    // FIXME(gbiv): Clang (and GCC, for that matter) should diagnose this.
    EXPECT_DEATH_STRUCT(
        stpncpy(split.tiny_buffer, small_string, sizeof(small_string)));

#if _FORTIFY_SOURCE > 1
    // expected-warning@+2{{destination buffer will always be overflown}}
#endif
    EXPECT_DEATH_STRUCT(strcat(split.tiny_buffer, small_string));

#if _FORTIFY_SOURCE > 1
    // expected-warning@+2{{called with bigger length than the destination}}
#endif
    EXPECT_DEATH_STRUCT(
        strncat(split.tiny_buffer, small_string, sizeof(small_string)));
  }
}

// Since these emit hard errors, it's sort of hard to run them...
#ifdef COMPILATION_TESTS
namespace compilation_tests {
static void testFcntl() {
  // FIXME(gbiv): Need to fix these; they got dropped.
#if 0
  // expected-error@+1{{either with 2 or 3 arguments, not more}}
#endif
  open("/", 0, 0, 0);
#if 0
  // expected-error@+1{{either with 2 or 3 arguments, not more}}
#endif
  open64("/", 0, 0, 0);
  // expected-error@+1{{either with 3 or 4 arguments, not more}}
  openat(0, "/", 0, 0, 0);
  // expected-error@+1{{either with 3 or 4 arguments, not more}}
  openat64(0, "/", 0, 0, 0);

  // expected-error@+1{{needs 3 arguments}}
  open("/", O_CREAT);
  // expected-error@+1{{needs 3 arguments}}
  open("/", O_TMPFILE);
  // expected-error@+1{{needs 3 arguments}}
  open64("/", O_CREAT);
  // expected-error@+1{{needs 3 arguments}}
  open64("/", O_TMPFILE);
  // expected-error@+1{{needs 4 arguments}}
  openat(0, "/", O_CREAT);
  // expected-error@+1{{needs 4 arguments}}
  openat(0, "/", O_TMPFILE);
  // expected-error@+1{{needs 4 arguments}}
  openat64(0, "/", O_CREAT);
  // expected-error@+1{{needs 4 arguments}}
  openat64(0, "/", O_TMPFILE);

  // Superfluous modes are sometimes bugs, but not often enough to complain
  // about, apparently.
}

static void testMqueue() {
  // FIXME(gbiv): remove mq_open's FORTIFY'ed body from glibc...

  // expected-error@+1{{with 2 or 4 arguments}}
  mq_open("/", 0, 0);
  // expected-error@+1{{with 2 or 4 arguments}}
  mq_open("/", 0, 0, 0, 0);

  // expected-error@+1{{needs 4 arguments}}
  mq_open("/", O_CREAT);
}
} // namespace compilation_tests
#endif

static void TestPoll() {
  struct pollfd invalid_poll_fd = {kBogusFD, 0, 0};
  {
    struct pollfd few_fds[] = {invalid_poll_fd, invalid_poll_fd};
    // expected-warning@+1{{fds buffer too small}}
    EXPECT_DEATH(poll(few_fds, 3, 0));
    // expected-warning@+1{{fds buffer too small}}
    EXPECT_DEATH(ppoll(few_fds, 3, 0, 0));
  }

  {
    struct {
      struct pollfd few[2];
      struct pollfd extra[1];
    } fds = {{invalid_poll_fd, invalid_poll_fd}, {invalid_poll_fd}};
    _Static_assert(sizeof(fds) >= sizeof(struct pollfd) * 3, "");

#if _FORTIFY_SOURCE > 1
    // expected-warning@+2{{fds buffer too small}}
#endif
    EXPECT_DEATH_STRUCT(poll(fds.few, 3, 0));

    struct timespec timeout = {};
#if _FORTIFY_SOURCE > 1
    // expected-warning@+2{{fds buffer too small}}
#endif
    EXPECT_DEATH_STRUCT(ppoll(fds.few, 3, &timeout, 0));
  }
}

static void TestSocket() {
  {
    char small_buffer[8];
    // expected-warning@+1{{bigger length than size of destination buffer}}
    EXPECT_DEATH(recv(kBogusFD, small_buffer, sizeof(small_buffer) + 1, 0));
    // expected-warning@+1{{bigger length than size of destination buffer}}
    EXPECT_DEATH(
        recvfrom(kBogusFD, small_buffer, sizeof(small_buffer) + 1, 0, 0, 0));
  }

  {
    struct {
      char tiny_buffer[4];
      char tiny_buffer2;
    } split = {};

    EXPECT_NO_DEATH(recv(kBogusFD, split.tiny_buffer, sizeof(split), 0));
    EXPECT_NO_DEATH(
        recvfrom(kBogusFD, split.tiny_buffer, sizeof(split), 0, 0, 0));
  }
}

static void TestStdio() {
  char small_buffer[8] = {};
  {
    // expected-warning@+1{{may overflow the destination buffer}}
    EXPECT_DEATH(snprintf(small_buffer, sizeof(small_buffer) + 1, ""));

    va_list va;
    // expected-warning@+1{{may overflow the destination buffer}}
    EXPECT_DEATH(vsnprintf(small_buffer, sizeof(small_buffer) + 1, "", va));
  }

  // gets is safe here, since stdin is actually /dev/null
  // expected-warning@+1{{ignoring return value}}
  EXPECT_NO_DEATH(gets(small_buffer));

  char *volatile unknown_size_buffer = small_buffer;
  // FIXME(gbiv): This should issue a "don't use me" warning, besides just
  // deprecation (which is suppressed)...
  // Since stdin is /dev/null, gets on a tiny buffer is safe here.
  // expected-warning@+1{{ignoring return value}}
  EXPECT_NO_DEATH(gets(unknown_size_buffer));
}

static void TestUnistd() {
  char small_buffer[8];

  // Return value warnings are (sort of) a part of FORTIFY, so we don't ignore
  // them.
  // expected-warning@+2{{ignoring return value of function}}
  // expected-warning@+1{{bigger length than size of the destination buffer}}
  EXPECT_DEATH(read(kBogusFD, small_buffer, sizeof(small_buffer) + 1));
  // expected-warning@+2{{ignoring return value of function}}
  // expected-warning@+1{{bigger length than size of the destination buffer}}
  EXPECT_DEATH(pread(kBogusFD, small_buffer, sizeof(small_buffer) + 1, 0));
  // expected-warning@+2{{ignoring return value of function}}
  // expected-warning@+1{{bigger length than size of the destination buffer}}
  EXPECT_DEATH(pread64(kBogusFD, small_buffer, sizeof(small_buffer) + 1, 0));
  // expected-warning@+2{{ignoring return value of function}}
  // expected-warning@+1{{bigger length than size of destination buffer}}
  EXPECT_DEATH(readlink("/", small_buffer, sizeof(small_buffer) + 1));
  // expected-warning@+2{{ignoring return value of function}}
  // expected-warning@+1{{bigger length than size of destination buffer}}
  EXPECT_DEATH(getcwd(small_buffer, sizeof(small_buffer) + 1));

  // glibc allocates and returns a buffer if you pass null to getcwd
  // expected-warning@+1{{ignoring return value of function}}
  EXPECT_NO_DEATH(getcwd(NULL, 0));
  // expected-warning@+1{{ignoring return value of function}}
  EXPECT_NO_DEATH(getcwd(NULL, 4096));

  {
    char large_buffer[PATH_MAX * 2];
    // expected-warning@+1{{ignoring return value of function}}
    EXPECT_NO_DEATH(getwd(large_buffer));

    char *volatile unknown_size_buffer = large_buffer;
    // expected-warning@+2{{ignoring return value of function}}
    // FIXME(gbiv): We should emit a "use getcwd" complaint here.
    EXPECT_NO_DEATH(getwd(unknown_size_buffer));
  }

  // expected-warning@+1{{bigger length than size of destination buffer}}
  EXPECT_DEATH(confstr(0, small_buffer, sizeof(small_buffer) + 1));

  {
    gid_t gids[2];
    // expected-warning@+1{{bigger group count than what can fit}}
    EXPECT_DEATH(getgroups(3, gids));
  }

  // expected-warning@+1{{bigger buflen than size of destination buffer}}
  EXPECT_DEATH(ttyname_r(kBogusFD, small_buffer, sizeof(small_buffer) + 1));
  // expected-warning@+1{{bigger buflen than size of destination buffer}}
  EXPECT_DEATH(getlogin_r(small_buffer, sizeof(small_buffer) + 1));
  // expected-warning@+1{{bigger buflen than size of destination buffer}}
  EXPECT_DEATH(gethostname(small_buffer, sizeof(small_buffer) + 1));
  // expected-warning@+1{{bigger buflen than size of destination buffer}}
  EXPECT_DEATH(getdomainname(small_buffer, sizeof(small_buffer) + 1));

  // We've already checked the warn-unused-result warnings; no need to clutter
  // the code with rechecks...
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-value"
  struct {
    char tiny_buffer[4];
    char tiny_buffer2[4];
  } split;

  EXPECT_NO_DEATH(read(kBogusFD, split.tiny_buffer, sizeof(split)));
  EXPECT_NO_DEATH(pread(kBogusFD, split.tiny_buffer, sizeof(split), 0));
  EXPECT_NO_DEATH(pread64(kBogusFD, split.tiny_buffer, sizeof(split), 0));

#if _FORTIFY_SOURCE > 1
  // expected-warning@+2{{bigger length than size of destination buffer}}
#endif
  EXPECT_DEATH_STRUCT(readlink("/", split.tiny_buffer, sizeof(split)));
#if _FORTIFY_SOURCE > 1
  // expected-warning@+2{{bigger length than size of destination buffer}}
#endif
  EXPECT_DEATH_STRUCT(getcwd(split.tiny_buffer, sizeof(split)));

#if _FORTIFY_SOURCE > 1
  // expected-warning@+2{{bigger length than size of destination buffer}}
#endif
  EXPECT_DEATH_STRUCT(confstr(kBogusFD, split.tiny_buffer, sizeof(split)));

  {
    struct {
      gid_t tiny_buffer[2];
      gid_t tiny_buffer2[1];
    } split_gids;
#if _FORTIFY_SOURCE > 1
    // expected-warning@+2{{bigger group count than what can fit}}
#endif
    EXPECT_DEATH_STRUCT(getgroups(3, split_gids.tiny_buffer));
  }

#if _FORTIFY_SOURCE > 1
  // expected-warning@+2{{bigger buflen than size of destination buffer}}
#endif
  EXPECT_DEATH_STRUCT(ttyname_r(kBogusFD, split.tiny_buffer, sizeof(split)));
#if _FORTIFY_SOURCE > 1
  // expected-warning@+2{{bigger buflen than size of destination buffer}}
#endif
  EXPECT_DEATH_STRUCT(getlogin_r(split.tiny_buffer, sizeof(split)));
#if _FORTIFY_SOURCE > 1
  // expected-warning@+2{{bigger buflen than size of destination buffer}}
#endif
  EXPECT_DEATH_STRUCT(gethostname(split.tiny_buffer, sizeof(split)));
#if _FORTIFY_SOURCE > 1
  // expected-warning@+2{{bigger buflen than size of destination buffer}}
#endif
  EXPECT_DEATH_STRUCT(getdomainname(split.tiny_buffer, sizeof(split)));

#pragma clang diagnostic pop // -Wunused-value
}

static void TestWchar() {
  // Sizes here are all expressed in terms of sizeof(wchar_t).
  const int small_buffer_size = 8;
  wchar_t small_buffer[small_buffer_size] = {};
  {
    const int large_buffer_size = small_buffer_size + 1;
    wchar_t large_buffer[large_buffer_size];

    // expected-warning@+1{{length bigger than size of destination buffer}}
    EXPECT_DEATH(wmemcpy(small_buffer, large_buffer, large_buffer_size));
    // expected-warning@+1{{length bigger than size of destination buffer}}
    EXPECT_DEATH(wmemmove(small_buffer, large_buffer, large_buffer_size));
    // expected-warning@+1{{length bigger than size of destination buffer}}
    EXPECT_DEATH(wmempcpy(small_buffer, large_buffer, large_buffer_size));
  }

  {
    const wchar_t large_string[] = L"Hello!!!";
    const int large_string_size = small_buffer_size + 1;
    _Static_assert(sizeof(large_string) == large_string_size * sizeof(wchar_t),
                   "");

    // expected-warning@+1{{length bigger than size of destination buffer}}
    EXPECT_DEATH(wmemset(small_buffer, 0, small_buffer_size + 1));
    // expected-warning@+1{{length bigger than size of destination buffer}}
    EXPECT_DEATH(wcsncpy(small_buffer, large_string, small_buffer_size + 1));
    // expected-warning@+1{{length bigger than size of destination buffer}}
    EXPECT_DEATH(wcpncpy(small_buffer, large_string, small_buffer_size + 1));

    // expected-warning@+2{{ignoring return value of function}}
    // expected-warning@+1{{length bigger than size of destination buffer}}
    EXPECT_DEATH(fgetws(small_buffer, sizeof(small_buffer) + 1, 0));
    // expected-warning@+2{{ignoring return value of function}}
    // expected-warning@+1{{bigger size than length of destination buffer}}
    EXPECT_DEATH(fgetws_unlocked(small_buffer, sizeof(small_buffer) + 1, 0));

    // No diagnostics emitted for either clang or gcc :(
    EXPECT_DEATH(wcscpy(small_buffer, large_string));
    EXPECT_DEATH(wcpcpy(small_buffer, large_string));
    EXPECT_DEATH(wcscat(small_buffer, large_string));
    EXPECT_DEATH(wcsncat(small_buffer, large_string, large_string_size));
  }

  mbstate_t mbs;
  bzero(&mbs, sizeof(mbs));
  {
    const char *src[small_buffer_size * sizeof(wchar_t)];
    // expected-warning@+1{{called with dst buffer smaller than}}
    EXPECT_DEATH(mbsrtowcs(small_buffer, src, sizeof(small_buffer) + 1, &mbs));
  }

  {
    const int array_len = 8;
    char chars[array_len];
    const char *chars_ptr = chars;
    wchar_t wchars[array_len];
    const wchar_t *wchars_ptr = wchars;
    // expected-warning@+1{{called with dst buffer smaller than}}
    EXPECT_DEATH(wcsrtombs(chars, &wchars_ptr, array_len + 1, &mbs));
    // expected-warning@+1{{called with dst buffer smaller than}}
    EXPECT_DEATH(mbsnrtowcs(wchars, &chars_ptr, 0, array_len + 1, &mbs));
    // expected-warning@+1{{called with dst buffer smaller than}}
    EXPECT_DEATH(wcsnrtombs(chars, &wchars_ptr, 0, array_len + 1, &mbs));
  }


  struct {
    wchar_t buf[small_buffer_size - 1];
    wchar_t extra;
  } small_split;
  _Static_assert(sizeof(small_split) == sizeof(small_buffer), "");
  bzero(&small_split, sizeof(small_split));

  EXPECT_NO_DEATH(wmemcpy(small_split.buf, small_buffer, small_buffer_size));
  EXPECT_NO_DEATH(wmemmove(small_split.buf, small_buffer, small_buffer_size));
  EXPECT_NO_DEATH(wmempcpy(small_split.buf, small_buffer, small_buffer_size));

  {
    const wchar_t small_string[] = L"Hello!!";
    _Static_assert(sizeof(small_buffer) == sizeof(small_string), "");

    EXPECT_NO_DEATH(wmemset(small_split.buf, 0, small_buffer_size));
#if _FORTIFY_SOURCE > 1
    // expected-warning@+2{{length bigger than size of destination buffer}}
#endif
    EXPECT_DEATH_STRUCT(
        wcsncpy(small_split.buf, small_string, small_buffer_size));
#if _FORTIFY_SOURCE > 1
    // expected-warning@+2{{length bigger than size of destination buffer}}
#endif
    EXPECT_DEATH_STRUCT(
        wcpncpy(small_split.buf, small_string, small_buffer_size));

    // FIXME(gbiv): FORTIFY doesn't warn about this eagerly enough on
    // _FORTIFY_SOURCE=1.
    // expected-warning@+4{{ignoring return value of function}}
#if _FORTIFY_SOURCE > 1
    // expected-warning@+2{{length bigger than size of destination buffer}}
#endif
    EXPECT_DEATH(fgetws(small_split.buf, small_buffer_size, 0));

    // FIXME(gbiv): FORTIFY doesn't warn about this eagerly enough on
    // _FORTIFY_SOURCE=1.
    // expected-warning@+4{{ignoring return value of function}}
#if _FORTIFY_SOURCE > 1
    // expected-warning@+2{{bigger size than length of destination buffer}}
#endif
    EXPECT_DEATH(fgetws_unlocked(small_split.buf, small_buffer_size, 0));

    // No diagnostics emitted for either clang or gcc :(
    EXPECT_DEATH_STRUCT(wcscpy(small_split.buf, small_string));
    EXPECT_DEATH_STRUCT(wcpcpy(small_split.buf, small_string));
    EXPECT_DEATH_STRUCT(wcscat(small_split.buf, small_string));
    EXPECT_DEATH_STRUCT(
        wcsncat(small_split.buf, small_string, small_buffer_size));
  }

  {
    // NOREVIEW: STRUCT
    const char *src[sizeof(small_buffer)] = {};
    // FIXME(gbiv): _FORTIFY_SOURCE=1 should diagnose this more aggressively
#if _FORTIFY_SOURCE > 1
    // expected-warning@+2{{called with dst buffer smaller than}}
#endif
    EXPECT_DEATH(mbsrtowcs(small_split.buf, src, small_buffer_size, &mbs));
  }

  {
    // NOREVIEW: STRUCT
    const int array_len = 8;
    struct {
      char buf[array_len - 1];
      char extra;
    } split_chars;
    const char *chars_ptr = split_chars.buf;
    struct {
      wchar_t buf[array_len - 1];
      wchar_t extra;
    } split_wchars;
    const wchar_t *wchars_ptr = split_wchars.buf;
#if _FORTIFY_SOURCE > 1
    // expected-warning@+2{{called with dst buffer smaller than}}
#endif
    EXPECT_DEATH_STRUCT(
        wcsrtombs(split_chars.buf, &wchars_ptr, array_len, &mbs));
#if _FORTIFY_SOURCE > 1
    // expected-warning@+2{{called with dst buffer smaller than}}
#endif
    EXPECT_DEATH_STRUCT(
        mbsnrtowcs(split_wchars.buf, &chars_ptr, 0, array_len, &mbs));
#if _FORTIFY_SOURCE > 1
    // expected-warning@+2{{called with dst buffer smaller than}}
#endif
    EXPECT_DEATH_STRUCT(
        wcsnrtombs(split_chars.buf, &wchars_ptr, 0, array_len, &mbs));
  }
}

static void TestStdlib() {
  {
    char path_buffer[PATH_MAX - 1];
    // expected-warning@+2{{ignoring return value of function}}
    // expected-warning@+1{{must be either NULL or at least PATH_MAX bytes}}
    EXPECT_DEATH(realpath("/", path_buffer));
    // expected-warning@+1{{ignoring return value of function}}
    realpath("/", NULL);
  }

  char small_buffer[8];
  // expected-warning@+1{{called with buflen bigger than size of buf}}
  EXPECT_DEATH(ptsname_r(kBogusFD, small_buffer, sizeof(small_buffer) + 1));

  {
    const int wchar_buffer_size = 8;
    wchar_t wchar_buffer[wchar_buffer_size];
    // expected-warning@+1{{called with dst buffer smaller than}}
    EXPECT_DEATH(mbstowcs(wchar_buffer, small_buffer, wchar_buffer_size + 1));
    // expected-warning@+1{{called with dst buffer smaller than}}
    EXPECT_DEATH(
        wcstombs(small_buffer, wchar_buffer, sizeof(small_buffer) + 1));
  }

  {
    struct {
      char path_buffer[PATH_MAX - 1];
      char rest[1];
    } split;
    // expected-warning@+4{{ignoring return value of function}}
#if _FORTIFY_SOURCE > 1
    // expected-warning@+2{{must be either NULL or at least PATH_MAX bytes}}
#endif
    EXPECT_DEATH_STRUCT(realpath("/", split.path_buffer));
  }

  struct {
    char tiny_buffer[4];
    char rest[1];
  } split;
#if _FORTIFY_SOURCE > 1
  // expected-warning@+2{{called with buflen bigger than size of buf}}
#endif
  EXPECT_DEATH_STRUCT(ptsname_r(kBogusFD, split.tiny_buffer, sizeof(split)));

  {
    const int tiny_buffer_size = 4;
    struct {
      wchar_t tiny_buffer[tiny_buffer_size];
      wchar_t rest;
    } wsplit;
#if _FORTIFY_SOURCE > 1
    // expected-warning@+2{{called with dst buffer smaller than}}
#endif
    EXPECT_DEATH_STRUCT(
        mbstowcs(wsplit.tiny_buffer, small_buffer, tiny_buffer_size + 1));
#if _FORTIFY_SOURCE > 1
    // expected-warning@+2{{called with dst buffer smaller than}}
#endif
    EXPECT_DEATH_STRUCT(
        wcstombs(split.tiny_buffer, wsplit.tiny_buffer, sizeof(split)));
  }
}

/////////////////// Test infrastructure; nothing to see here ///////////////////

#define CONCAT2(x, y) x ## y
#define CONCAT(x, y) CONCAT2(x, y)

// Exported to the driver so we can run these tests.
std::vector<Failure> CONCAT(test_fortify_, _FORTIFY_SOURCE)() {
  std::vector<Failure> result;
  failures = &result;

  TestPoll();
  TestSocket();
  TestStdio();
  TestStdlib();
  TestString();
  TestUnistd();
  TestWchar();

  failures = nullptr;
  return result;
}
