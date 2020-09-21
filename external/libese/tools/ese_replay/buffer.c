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

#include "buffer.h"

bool buffer_init(struct Buffer *b, uint32_t len) {
  if (!b)
    return false;
  b->buffer = calloc(len, 1);
  if (!b->buffer)
    return false;
  b->len = 0;
  b->size = len;
  return true;
}

void buffer_free(struct Buffer *b) {
  if (b && b->buffer)
    free(b->buffer);
}

bool buffer_read_hex(struct Buffer *b, FILE *fp, bool consume_newline) {
  int offset;
  uint32_t i;

  if (feof(fp))
    return false;
  /* Consume leading whitespace. */
  while (!feof(fp)) {
    char c = fgetc(fp);
    if (!consume_newline && (c == 0x0d || c == 0x0a)) {
      return false;
    }
    if (!isspace(c)) {
      ungetc(c, fp);
      break;
    }
  }

  for (; !feof(fp) && b->len < b->size; ++b->len) {
    b->buffer[b->len] = fgetc(fp);
    if (!isalnum(b->buffer[b->len])) {
      ungetc(b->buffer[b->len], fp);
      break;
    }
  }
  if (b->len == 0)
    return false;

  for (offset = 0, i = 0; offset < (int)b->len && i < b->size; ++i) {
    int last_offset = offset;
    if (sscanf((char *)b->buffer + offset, "%2hhx%n", &b->buffer[i], &offset) !=
        1)
      break;
    offset += last_offset;
    if (offset == 0)
      break;
  }
  b->len = i;
  return true;
}

void buffer_dump(const struct Buffer *b, const char *prefix, const char *name,
                 uint32_t limit, FILE *fp) {
  fprintf(fp, "%s%s {\n", prefix, name);
  fprintf(fp, "%s  .length = %u\n", prefix, b->len);
  fprintf(fp, "%s  .size = %u\n", prefix, b->size);
  fprintf(fp, "%s  .buffer = {\n", prefix);
  fprintf(fp, "%s    ", prefix);
  for (uint32_t i = 0; i < b->len; ++i) {
    if (i > 15 && i % 16 == 0) {
      fprintf(fp, "\n%s    ", prefix);
    }
    if (i > limit) {
      fprintf(fp, ". . .");
      break;
    }
    fprintf(fp, "%.2x ", b->buffer[i]);
  }
  fprintf(fp, "\n%s}\n", prefix);
}
