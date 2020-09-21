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
 * Trivial managed buffer structure
 */

#ifndef ESE_REPLAY_BUFFER_H__
#define ESE_REPLAY_BUFFER_H__ 1

#include <ctype.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct Buffer {
  uint32_t len;
  uint32_t size;
  uint8_t *buffer;
};

bool buffer_init(struct Buffer *b, uint32_t len);
void buffer_free(struct Buffer *b);
bool buffer_read_hex(struct Buffer *b, FILE *fp, bool consume_newline);
void buffer_dump(const struct Buffer *b, const char *prefix, const char *name,
                 uint32_t limit, FILE *fp);

#endif  /* ESE_REPLAY_BUFFER_H__ */
