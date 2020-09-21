/*
 * Copyright (C) 2017 The Android Open Source Project
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
 *
 */

#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define LOG_TAG "ese-sh"
#include <ese/ese.h>
#include <ese/log.h>

#include "hw.h"

void print_supported_hardware(const struct SupportedHardware *supported) {
  printf("Supported hardware:\n");
  for (size_t i = 0; i < supported->len; ++i) {
    printf("\t%s\t(%s / %s)\n", supported->hw[i].name, supported->hw[i].sym,
           supported->hw[i].lib);
  }
}

int find_supported_hardware(const struct SupportedHardware *supported,
                            const char *name) {
  for (size_t i = 0; i < supported->len; ++i) {
    if (!strcmp(name, supported->hw[i].name)) {
      return (int)i;
    }
  }
  return -1;
}

/* This should be C++. */
void release_hardware(const struct Hardware *hw) {
  void *hw_handle = dlopen(hw->lib, RTLD_NOW);
  /* Close for the above open and the original. */
  dlclose(hw_handle);
  dlclose(hw_handle);
}

bool initialize_hardware(struct EseInterface *ese, const struct Hardware *hw) {
  void *hw_handle = dlopen(hw->lib, RTLD_NOW);
  if (!hw_handle) {
    fprintf(stderr, "Failed to open hardware implementation: %s\n", dlerror());
    return false;
  }
  const struct EseOperations **hw_ops = dlsym(hw_handle, hw->sym);
  if (!hw_ops) {
    fprintf(stderr, "Failed to find hardware implementation: %s\n", dlerror());
    return false;
  }
  /* N.b., ese_init appends _ops to the second argument. */
  ese_init(ese, *hw);
  return true;
}
