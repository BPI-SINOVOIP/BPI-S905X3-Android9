// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <err.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "clang-fortify-common.h"

// It's potentially easy for us to get blocked if a FORTIFY'ed I/O call isn't
// properly verified.
static void SetTimeout(int secs) {
  struct sigaction sigact;
  bzero(&sigact, sizeof(sigact));

  sigact.sa_flags = SA_RESETHAND;
  sigact.sa_handler = [](int) {
    const char complaint[] = "!!! Timeout reached; abort abort abort\n";
    (void)write(STDOUT_FILENO, complaint, strlen(complaint));
    _exit(1);
  };

  if (sigaction(SIGALRM, &sigact, nullptr))
    err(1, "Failed to establish a SIGALRM handler");

  alarm(secs);
}

static void PrintFailures(const std::vector<Failure> &failures) {
  fprintf(stderr, "Failure(s): (%zu total)\n", failures.size());
  for (const Failure &f : failures) {
    const char *why = f.expected_death ? "didn't die" : "died";
    fprintf(stderr, "\t`%s` at line %d %s\n", f.message, f.line, why);
  }
}

int main() {
  // On my dev machine, this test takes around 70ms to run to completion. On
  // samus, it's more like 20 seconds.
  SetTimeout(300);

  // Some functions (e.g. gets()) try to read from stdin. Stop them from doing
  // so, lest they block forever.
  int dev_null = open("/dev/null", O_RDONLY);
  if (dev_null < 0)
    err(1, "Failed opening /dev/null");

  if (dup2(dev_null, STDIN_FILENO) < 0)
    err(1, "Failed making stdin == /dev/null");

  bool failed = false;

  fprintf(stderr, "::: Testing _FORTIFY_SOURCE=1 :::\n");
  std::vector<Failure> failures = test_fortify_1();
  if (!failures.empty()) {
    PrintFailures(failures);
    failed = true;
  }

  fprintf(stderr, "::: Testing _FORTIFY_SOURCE=2 :::\n");
  failures = test_fortify_2();
  if (!failures.empty()) {
    PrintFailures(failures);
    failed = true;
  }

  return failed ? 1 : 0;
}
