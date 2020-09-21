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

#include "vts_datatype.h"

#include <stdlib.h>
#include <time.h>

namespace android {
namespace vts {

void RandomNumberGeneratorReset() { srand(time(NULL)); }

uint32_t RandomUint32() { return (unsigned int)rand(); }

int32_t RandomInt32() { return rand(); }

uint64_t RandomUint64() {
  uint64_t num = (unsigned int)rand();
  return (num << 32) | (unsigned int)rand();
}

int64_t RandomInt64() {
  int64_t num = rand();
  return (num << 32) | rand();
}

float RandomFloat() { return (float)rand() / (float)(RAND_MAX / 1000000000.0); }

double RandomDouble() {
  return (double)rand() / (double)(RAND_MAX / 1000000000.0);
}

bool RandomBool() { return (abs(rand()) % 2) == 1; }

char* RandomCharPointer() {
  int len = RandomUint32() % MAX_CHAR_POINTER_LENGTH;
  char* buf = (char*)malloc(len);
  buf[len - 1] = '\0';
  return buf;
}

void* RandomVoidPointer() {
  int len = RandomUint32() % MAX_CHAR_POINTER_LENGTH;
  void* buf = malloc(len);
  return buf;
}

}  // namespace vts
}  // namespace android
