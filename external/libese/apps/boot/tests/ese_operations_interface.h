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

#ifndef ESE_OPERATIONS_INTERFACE_H_
#define ESE_OPERATIONS_INTERFACE_H_ 1

#include <ese/ese.h>

class EseOperationsInterface {
 public:
  EseOperationsInterface() { }
  virtual ~EseOperationsInterface() { };

  virtual int EseOpen(struct EseInterface *ese, void *data) = 0;
  virtual uint32_t EseHwReceive(struct EseInterface *ese, uint8_t *data, uint32_t len, int complete) = 0;
  virtual uint32_t EseHwTransmit(struct EseInterface *ese, const uint8_t *data, uint32_t len, int complete) = 0;
  virtual int EseReset(struct EseInterface *ese) = 0;
  virtual uint32_t EseTransceive(struct EseInterface *ese, const struct EseSgBuffer *tx_sg, uint32_t tx_nsg,
                                 struct EseSgBuffer *rx_sg, uint32_t rx_nsg) = 0;
  virtual int EsePoll(struct EseInterface *ese, uint8_t poll_for, float timeout, int complete) = 0;
  virtual void EseClose(struct EseInterface *ese) = 0;
};

#endif  // ESE_OPERATIONS_INTERFACE_H_
