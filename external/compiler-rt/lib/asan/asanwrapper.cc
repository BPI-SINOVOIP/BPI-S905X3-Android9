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

#include <elf.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <string>

void usage(const char* me) {
  static const char* usage_s = "Usage:\n"
    "  %s /system/bin/app_process <args>\n"
    "or, better:\n"
    "  setprop wrap.<nicename> %s\n";
  fprintf(stderr, usage_s, me, me);
  exit(1);
}

int main(int argc, char** argv) {
  if (argc < 2) {
    usage(argv[0]);
  }
  char** args = new char*[argc];
  // If we are wrapping app_process, replace it with app_process_asan.
  // TODO(eugenis): rewrite to <dirname>/asan/<basename>, if exists?
  if (strcmp(argv[1], "/system/bin/app_process") == 0) {
    args[0] = (char*)"/system/bin/asan/app_process";
  } else if (strcmp(argv[1], "/system/bin/app_process32") == 0) {
    args[0] = (char*)"/system/bin/asan/app_process32";
  } else if (strcmp(argv[1], "/system/bin/app_process64") == 0) {
    args[0] = (char*)"/system/bin/asan/app_process64";
  } else {
    args[0] = argv[1];
  }

  for (int i = 1; i < argc - 1; ++i)
    args[i] = argv[i + 1];
  args[argc - 1] = 0;

  for (int i = 0; i < argc - 1; ++i)
    printf("argv[%d] = %s\n", i, args[i]);

  execv(args[0], args);
}
