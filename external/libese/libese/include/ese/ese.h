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

#ifndef ESE_H_
#define ESE_H_ 1

#include "ese_sg.h"
#include "ese_hw_api.h"
#include "../../../libese-sysdeps/include/ese/sysdeps.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Public client interface for Embedded Secure Elements.
 *
 * Prior to use in a file, import all necessary variables with:
 *  ESE_INCLUDE_HW(SOME_HAL_IMPL);
 *
 * Instantiate in a function with:
 *   ESE_DECLARE(my_ese, SOME_HAL_IMPL);
 * or
 *   struct EseInterface my_ese = ESE_INITIALIZER(SOME_HAL_IMPL);
 * or
 *   struct EseInterface *my_ese = malloc(sizeof(struct EseInterface));
 *   ...
 *   ese_init(my_ese, SOME_HAL_IMPL);
 *
 * To initialize the hardware abstraction, call:
 *   ese_open(my_ese);
 *
 * To release any claimed resources, call
 *   ese_close(my_ese)
 * when interface use is complete.
 *
 * To perform a transmit-receive cycle, call
 *   ese_transceive(my_ese, ...);
 * with a filled transmit buffer with total data length and
 * an empty receive buffer and a maximum fill length.
 *
 * A negative return value indicates an error and a hardware
 * specific code and string may be collected with calls to
 *   ese_error_code(my_ese);
 *   ese_error_message(my_ese);
 *
 * The EseInterface is not safe for concurrent access.
 * (Patches welcome ;).
 */
struct EseInterface;

#define ese_init(ese_ptr, HW_TYPE)  __ese_init(ese_ptr, HW_TYPE)
#define ESE_DECLARE(name, HW_TYPE, ...) \
  struct EseInterface name = __ESE_INTIALIZER(HW_TYPE)
#define ESE_INITIALIZER __ESE_INITIALIZER
#define ESE_INCLUDE_HW __ESE_INCLUDE_HW

const char *ese_name(const struct EseInterface *ese);
int ese_open(struct EseInterface *ese, void *hw_opts);
void ese_close(struct EseInterface *ese);
int ese_transceive(struct EseInterface *ese, const uint8_t *tx_buf, uint32_t tx_len, uint8_t *rx_buf, uint32_t rx_max);
int ese_transceive_sg(struct EseInterface *ese, const struct EseSgBuffer *tx, uint32_t tx_segs,
                      struct EseSgBuffer *rx, uint32_t rx_segs);

bool ese_error(const struct EseInterface *ese);
const char *ese_error_message(const struct EseInterface *ese);
int ese_error_code(const struct EseInterface *ese);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* ESE_H_ */
