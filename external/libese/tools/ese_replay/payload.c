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

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "payload.h"

bool payload_init(struct Payload *p, uint32_t tx_len, uint32_t exp_len) {
  if (!p)
    return false;
  if (!buffer_init(&p->tx, tx_len))
    return false;
  if (buffer_init(&p->expected, exp_len))
    return true;
  buffer_free(&p->tx);
  return false;
}

void payload_free(struct Payload *p) {
  if (p) {
    buffer_free(&p->tx);
    buffer_free(&p->expected);
  }
}

bool payload_read(struct Payload *p, FILE *fp) {
  if (!p)
    return false;
  p->tx.len = 0;
  p->expected.len = 0;
  while (buffer_read_hex(&p->tx, fp, true) && p->tx.len == 0)
    ;
  if (p->tx.len == 0)
    return false;
  if (!buffer_read_hex(&p->expected, fp, false)) {
    p->expected.buffer[0] = 0x90;
    p->expected.buffer[1] = 0x00;
    p->expected.len = 2;
  }
  return true;
}

void payload_dump(const struct Payload *payload, FILE *fp) {
  fprintf(fp, "Payload {\n");
  buffer_dump(&payload->tx, "  ", "Transmit", 240, fp);
  buffer_dump(&payload->expected, "  ", "Expected", 240, fp);
  fprintf(fp, "}\n");
}
