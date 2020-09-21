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

#ifndef ESE_HW_API_H_
#define ESE_HW_API_H_ 1

#include "ese_sg.h"
#include "../../../libese-sysdeps/include/ese/sysdeps.h"

#ifdef __cplusplus
extern "C" {
#endif
/*
 * Pulls the hardware declarations in to scope for the current file
 * to make use of.
 */
#define __ESE_INCLUDE_HW(name) \
  extern const struct EseOperations * name## _ops


struct EseInterface;

/* ese_hw_receive_op_t: receives a buffer from the hardware.
 * Args:
 * - struct EseInterface *: session handle.
 * - uint8_t *: pointer to the buffer to receive data into.
 * - uint32_t: maximum length of the data to receive.
 * - int: 1 or 0 indicating if it is a complete transaction. This allows the underlying
 *       implementation to batch reads if needed by the underlying wire protocol or if
 *       the hardware needs to be signalled explicitly.
 *
 * Returns:
 * - uint32_t: bytes received.
 */
typedef uint32_t (ese_hw_receive_op_t)(struct EseInterface *, uint8_t *, uint32_t, int);
/* ese_hw_transmit_op_t: transmits a buffer over the hardware.
 * Args:
 * - struct EseInterface *: session handle.
 * - uint8_t *: pointer to the buffer to transmit.
 * - uint32_t: length of the data to transmit.
 * - int: 1 or 0 indicating if it is a complete transaction.
 *
 * Returns:
 * - uint32_t: bytes transmitted.
 */
typedef uint32_t (ese_hw_transmit_op_t)(struct EseInterface *, const uint8_t *, uint32_t, int);
/* ese_hw_reset_op_t: resets the hardware in case of communication desynchronization.
 * Args:
 * - struct EseInterface *: session handle.
 *
 * Returns:
 * - int: -1 on error, 0 on success.
 */
typedef int (ese_hw_reset_op_t)(struct EseInterface *);
/* ese_transceive_sg_op_t:  fully contained transmission and receive operation.
 *
 * Must provide an implementation of the wire protocol necessary to transmit
 * and receive an application payload to and from the eSE.  Normally, this
 * implementation is built on the hw_{receive,transmit} operations also
 * provided and often requires the poll operation below.
 *
 * Args:
 * - struct EseInterface *: session handle.
 * - const EseSgBuffer *: array of buffers to transmit
 * - uint32_t: number of buffers to send
 * - const EseSgBuffer *: array of buffers to receive into
 * - uint32_t: number of buffers to receive to
 *
 * Returns:
 * - uint32_t: bytes received.
 */
typedef uint32_t (ese_transceive_op_t)(
  struct EseInterface *, const struct EseSgBuffer *, uint32_t, struct EseSgBuffer *, uint32_t);
/* ese_poll_op_t: waits for the hardware to be ready to send data.
 *
 * Args:
 * - struct EseInterface *: session handle.
 * - uint8_t: byte to wait for. E.g., a start of frame byte.
 * - float: time in seconds to wait.
 * - int: whether to complete a transaction when polling іs complete.
 *
 * Returns:
 * - int: On error or timeout, -1.
 *        On success, 1 or 0 depending on if the found byte was consumed.
 */
typedef int (ese_poll_op_t)(struct EseInterface *, uint8_t, float, int);
/* ese_hw_open_op_t: prepares the hardware for use.
 *
 * This function should prepare the hardware for use and attach
 * any implementation specific state to the EseInterface handle such
 * that it is accessible in the other calls.
 *
 * Args:
 * - struct EseInterface *: freshly initialized session handle.
 * - void *: implementation specific pointer from the libese client.
 *
 * Returns:
 * - int: < 0 on error, 0 on success.
 */
typedef int (ese_open_op_t)(struct EseInterface *, void *);
/* ese_hw_close_op_t: releases the hardware in use.
 *
 * This function should free any dynamic memory and release
 * the claimed hardware.
 *
 * Args:
 * - struct EseInterface *: freshly initialized session handle.
 *
 * Returns:
 * - Nothing.
 */
typedef void (ese_close_op_t)(struct EseInterface *);

#define __ESE_INITIALIZER(TYPE) \
{ \
  .ops = TYPE## _ops, \
  .error = { \
    .is_err = false, \
    .code = 0, \
    .message = NULL, \
  }, \
  .pad =  { 0 }, \
}

#define __ese_init(_ptr, TYPE) {\
  (_ptr)->ops = TYPE## _ops; \
  (_ptr)->pad[0] = 0; \
  (_ptr)->error.is_err = false; \
  (_ptr)->error.code = 0; \
  (_ptr)->error.message = (const char *)NULL; \
}

struct EseOperations {
  const char *name;
  /* Used to prepare any implementation specific internal data and
   * state needed for robust communication.
   */
  ese_open_op_t *open;
  /* Used to receive raw data from the ese. */
  ese_hw_receive_op_t *hw_receive;
  /* Used to transmit raw data to the ese. */
  ese_hw_transmit_op_t *hw_transmit;
  /* Used to perform a power reset on the device. */
  ese_hw_reset_op_t *hw_reset;
  /* Wire-specific protocol polling for readiness. */
  ese_poll_op_t *poll;
  /* Wire-specific protocol for transmitting and receiving
   * application data to the eSE. By default, this may point to
   * a generic implementation, like teq1_transceive, which uses
   * the hw_* ops above.
   */
  ese_transceive_op_t *transceive;
  /* Cleans up any required state: file descriptors or heap allocations. */
  ese_close_op_t *close;

  /* Operational options */
  const void *opts;

  /* Operation error messages. */
  const char **errors;
  uint32_t errors_count;
};

/* Maximum private stack storage on the interface instance. */
#define ESE_INTERFACE_STATE_PAD 16
struct EseInterface {
  const struct EseOperations * ops;
  struct {
    bool is_err;
    int code;
    const char *message;
  } error;
  /* Reserved to avoid heap allocation requirement. */
  uint8_t pad[ESE_INTERFACE_STATE_PAD];
};

/*
 * Provided by libese to manage exposing usable errors up the stack to the
 * libese user.
 */
void ese_set_error(struct EseInterface *ese, int code);

/*
 * Global error enums.
 */
enum EseGlobalError {
  kEseGlobalErrorNoTransceive = -1,
  kEseGlobalErrorPollTimedOut = -2,
};

#define ESE_DEFINE_HW_OPS(name, obj) \
  const struct EseOperations * name##_ops = &obj


#ifdef __cplusplus
}  /* extern "C" */
#endif
#endif  /* ESE_HW_API_H_ */
