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

#include <ese/ese.h>

#include "ese_operations_interface.h"
#include "ese_operations_wrapper.h"

void EseOperationsWrapper::InitializeEse(
    struct EseInterface *ese, EseOperationsInterface *ops_interface) {
  EseOperationsWrapperData data;
  data.ops_interface = ops_interface;
  ese_init(ese, data.wrapper);
}

static int EseOpen(struct EseInterface *ese, void *data) {
  return EseOperationsWrapperData::ops_interface->EseOpen(ese, data);
}

static uint32_t EseHwReceive(struct EseInterface *ese, uint8_t *data, uint32_t len, int complete) {
  return EseOperationsWrapperData::ops_interface->EseHwReceive(ese, data, len, complete);
}

static uint32_t EseHwTransmit(struct EseInterface *ese, const uint8_t *data, uint32_t len, int complete) {
  return EseOperationsWrapperData::ops_interface->EseHwTransmit(ese, data, len, complete);
}

static int EseReset(struct EseInterface *ese) {
  return EseOperationsWrapperData::ops_interface->EseReset(ese);
}

static uint32_t EseTransceive(struct EseInterface *ese, const struct EseSgBuffer *tx_sg, uint32_t tx_nsg,
                               struct EseSgBuffer *rx_sg, uint32_t rx_nsg) {
  return EseOperationsWrapperData::ops_interface->EseTransceive(ese, tx_sg, tx_nsg, rx_sg, rx_nsg);
}

static int EsePoll(struct EseInterface *ese, uint8_t poll_for, float timeout, int complete) {
  return EseOperationsWrapperData::ops_interface->EsePoll(ese, poll_for, timeout, complete);
}

static void EseClose(struct EseInterface *ese) {
  return EseOperationsWrapperData::ops_interface->EseClose(ese);
}

EseOperationsInterface *EseOperationsWrapperData::ops_interface =
  reinterpret_cast<EseOperationsInterface *>(NULL);

static const char *kErrors[] = {};
const struct EseOperations EseOperationsWrapperData::ops = {
  .name = "EseOperationsWrapper HW",
  .open = &EseOpen,
  .hw_receive = &EseHwReceive,
  .hw_transmit = &EseHwTransmit,
  .hw_reset = &EseReset,
  .poll = &EsePoll,
  .transceive = &EseTransceive,
  .close = &EseClose,
  .opts = NULL,
  .errors = kErrors,
  .errors_count = 0,
};
const struct EseOperations *EseOperationsWrapperData::wrapper_ops =
  &EseOperationsWrapperData::ops;
