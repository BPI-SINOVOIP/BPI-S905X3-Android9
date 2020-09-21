/*
 * Copyright (c) 2017 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <sys/timex.h>

/* This program is expected to run under android alt-syscall. */
int main(void) {
  struct timex buf;
  int ret;

  /* Test a read operation. Should succeed (i.e. not returning -1). */
  memset(&buf, 0, sizeof(buf));
  ret = adjtimex(&buf);
  if (ret == -1)
    return 1;

  /* Test with nullptr buffer. Should fail with EFAULT. */
  ret = adjtimex(NULL);
  if (ret != -1 || errno != EFAULT)
    return 2;

  /*
   * Test a write operation. Under android alt-syscall, should fail with
   * EPERM.
   */
  buf.modes = ADJ_MAXERROR;
  ret = adjtimex(&buf);
  if (ret != -1 || errno != EPERM)
    return 3;

  /* Passed successfully */
  return 0;
}
