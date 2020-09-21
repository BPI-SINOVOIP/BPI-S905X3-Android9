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
#ifndef ESE_REPLAY_PAYLOAD_H__
#define ESE_REPLAY_PAYLOAD_H__ 1

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "buffer.h"

struct Payload {
  struct Buffer tx;
  struct Buffer expected;
};

bool payload_init(struct Payload *p, uint32_t tx_len, uint32_t exp_len);
void payload_free(struct Payload *p);
bool payload_read(struct Payload *p, FILE *fp);
void payload_dump(const struct Payload *payload, FILE *fp);

#endif  /* ESE_REPLAY_PAYLOAD_H__ */
