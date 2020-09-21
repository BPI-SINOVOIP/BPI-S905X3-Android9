/*
 * Copyright 2016 The Android Open Source Project
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

#include "GcdaFile.h"

#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace android {
namespace vts {

bool GcdaFile::Open() {
  if (filename_.length() < 1) return false;
  if (gcov_var_.file) return false;

  memset(&gcov_var_, 0, sizeof(gcov_var_));
  gcov_var_.overread = -1u;

  gcov_var_.file = fopen(filename_.c_str(), "rb");
  if (!gcov_var_.file) return false;
  gcov_var_.mode = 0;
  setbuf(gcov_var_.file, (char*)0);
  return true;
}

int GcdaFile::Close() {
  if (gcov_var_.file) {
    fclose(gcov_var_.file);
    gcov_var_.file = 0;
    gcov_var_.length = 0;
  }
  free(gcov_var_.buffer);
  gcov_var_.alloc = 0;
  gcov_var_.buffer = 0;
  gcov_var_.mode = 0;
  return gcov_var_.error;
}

void GcdaFile::Sync(unsigned base, unsigned length) {
  if (!gcov_var_.file) return;

  base += length;
  if (base - gcov_var_.start <= gcov_var_.length) {
    gcov_var_.offset = base - gcov_var_.start;
  } else {
    gcov_var_.offset = gcov_var_.length = 0;
    fseek(gcov_var_.file, base << 2, SEEK_SET);
    gcov_var_.start = ftell(gcov_var_.file) >> 2;
  }
}
unsigned GcdaFile::ReadStringArray(char** string_array, unsigned num_strings) {
  unsigned i;
  unsigned j;
  unsigned len = 0;

  for (j = 0; j < num_strings; j++) {
    unsigned string_len = ReadUnsigned();
    string_array[j] = (char*)malloc(string_len * sizeof(unsigned));  // xmalloc
    for (i = 0; i < string_len; i++) {
      ((unsigned*)string_array[j])[i] = ReadUnsigned();
    }
    len += (string_len + 1);
  }
  return len;
}

unsigned GcdaFile::ReadUnsigned() {
  const unsigned* buffer = ReadWords(1);

  if (!buffer) return 0;
  return FromFile(buffer[0]);
}

void GcdaFile::Allocate(unsigned length) {
  size_t new_size = gcov_var_.alloc;

  if (!new_size) new_size = GCOV_BLOCK_SIZE;
  new_size += length;
  new_size *= 2;
  gcov_var_.alloc = new_size;
  gcov_var_.buffer = (unsigned*)realloc(gcov_var_.buffer, new_size << 2);
}

const unsigned* GcdaFile::ReadWords(unsigned words) {
  const unsigned* result;
  unsigned excess = gcov_var_.length - gcov_var_.offset;

  if (!gcov_var_.file) return 0;

  if (excess < words) {
    gcov_var_.start += gcov_var_.offset;
    memmove(gcov_var_.buffer, gcov_var_.buffer + gcov_var_.offset, excess * 4);
    gcov_var_.offset = 0;
    gcov_var_.length = excess;
    if (gcov_var_.length + words > gcov_var_.alloc) {
      Allocate(gcov_var_.length + words);
    }
    excess = gcov_var_.alloc - gcov_var_.length;
    excess = fread(gcov_var_.buffer + gcov_var_.length, 1, excess << 2,
                   gcov_var_.file) >> 2;
    gcov_var_.length += excess;
    if (gcov_var_.length < words) {
      gcov_var_.overread += words - gcov_var_.length;
      gcov_var_.length = 0;
      return 0;
    }
  }
  result = &gcov_var_.buffer[gcov_var_.offset];
  gcov_var_.offset += words;
  return result;
}

int GcdaFile::Magic(unsigned magic, unsigned expected) {
  if (magic == expected) return 1;
  magic = (magic >> 16) | (magic << 16);
  magic = ((magic & 0xff00ff) << 8) | ((magic >> 8) & 0xff00ff);
  if (magic == expected) {
    gcov_var_.endian = 1;
    return -1;
  }
  return 0;
}

gcov_type GcdaFile::ReadCounter() {
  gcov_type value;
  const unsigned* buffer = ReadWords(2);
  if (!buffer) return 0;
  value = FromFile(buffer[0]);
  if (sizeof(value) > sizeof(unsigned)) {
    value |= ((gcov_type)FromFile(buffer[1])) << 32;
  } else if (buffer[1]) {
    gcov_var_.error = -1;
  }
  return value;
}

void GcdaFile::WriteBlock(unsigned size) {
  int num_words = fwrite(gcov_var_.buffer, size << 2, 1, gcov_var_.file);
  if (num_words != 1) {
    gcov_var_.error = 1;
  }
  gcov_var_.start += size;
  gcov_var_.offset -= size;
}

const char* GcdaFile::ReadString() {
  unsigned length = ReadUnsigned();
  if (!length) return 0;
  return (const char*)ReadWords(length);
}

}  // namespace vts
}  // namespace android
