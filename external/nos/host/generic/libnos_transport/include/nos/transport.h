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
 */
#ifndef NOS_TRANSPORT_H
#define NOS_TRANSPORT_H

#include <stdint.h>

#include <nos/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Blocking call using Nugget OS' Transport API */
uint32_t nos_call_application(const struct nos_device *dev,
                              uint8_t app_id, uint16_t params,
                              const uint8_t *args, uint32_t arg_len,
                              uint8_t *reply, uint32_t *reply_len);

#ifdef __cplusplus
}
#endif

#endif /* NOS_TRANSPORT_H */
