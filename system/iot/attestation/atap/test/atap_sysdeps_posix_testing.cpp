/*
 * Copyright 2017 The Android Open Source Project
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
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <map>

#include <base/debug/stack_trace.h>

#include <libatap/libatap.h>

void* atap_memcpy(void* dest, const void* src, size_t n) {
  return memcpy(dest, src, n);
}

void* atap_memset(void* dest, const int c, size_t n) {
  return memset(dest, c, n);
}

void atap_abort(void) {
  abort();
}

void atap_print(const char* message) {
  fprintf(stderr, "%s", message);
}

void atap_printv(const char* message, ...) {
  va_list ap;
  const char* m;

  va_start(ap, message);
  for (m = message; m != NULL; m = va_arg(ap, const char*)) {
    fprintf(stderr, "%s", m);
  }
  va_end(ap);
}

size_t atap_strlen(const char* str) {
  return strlen(str);
}

typedef struct {
  size_t size;
  base::debug::StackTrace stack_trace;
} AtapAllocatedBlock;

static std::map<void*, AtapAllocatedBlock> allocated_blocks;

void* atap_malloc(size_t size) {
  void* ptr = malloc(size);
  atap_assert(ptr != nullptr);
  AtapAllocatedBlock block;
  block.size = size;
  allocated_blocks[ptr] = block;
  return ptr;
}

void atap_free(void* ptr) {
  auto block_it = allocated_blocks.find(ptr);
  if (block_it == allocated_blocks.end()) {
    atap_fatal("Tried to free pointer to non-allocated block.\n");
    return;
  }
  allocated_blocks.erase(block_it);
  free(ptr);
}

namespace atap {

void testing_memory_reset() {
  allocated_blocks.clear();
}

bool testing_memory_all_freed() {
  if (allocated_blocks.size() == 0) {
    return true;
  }

  size_t sum = 0;
  for (const auto& block_it : allocated_blocks) {
    sum += block_it.second.size;
  }
  fprintf(stderr,
          "%zd bytes still allocated in %zd blocks:\n",
          sum,
          allocated_blocks.size());
  size_t n = 0;
  for (const auto& block_it : allocated_blocks) {
    fprintf(stderr,
            "--\nAllocation %zd/%zd of %zd bytes:\n",
            1 + n++,
            allocated_blocks.size(),
            block_it.second.size);
    block_it.second.stack_trace.Print();
  }
  return false;
}

// Also check leaks at process exit.
__attribute__((destructor)) static void ensure_all_memory_freed_at_exit() {
  if (!testing_memory_all_freed()) {
    atap_fatal("libatap memory leaks at process exit.\n");
  }
}

}  // namespace atap
