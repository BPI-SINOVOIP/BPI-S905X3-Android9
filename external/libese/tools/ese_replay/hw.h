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

#ifndef ESE_SH_HW_H_
#define ESE_SH_HW_H 1

#include <stdbool.h>
#include <ese/ese.h>

/* TODO(wad): Move this into a libese-hw/hw_loader.{h,cpp} impl. */
struct Hardware {
  const char *name;
  const char *sym;
  const char *lib;
  void *options;
};

struct SupportedHardware {
  size_t len;
  const struct Hardware hw[];
};

bool initialize_hardware(struct EseInterface *ese, const struct Hardware *hw);
void release_hardware(const struct Hardware *hw);
int find_supported_hardware(const struct SupportedHardware *supported,
                            const char *name);
void print_supported_hardware(const struct SupportedHardware *supported);
#endif  /* ESE_SH_HW_H_ */
