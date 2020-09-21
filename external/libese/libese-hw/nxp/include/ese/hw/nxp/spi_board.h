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

#ifndef ESE_HW_NXP_SPI_BOARD_H_
#define ESE_HW_NXP_SPI_BOARD_H_ 1

typedef enum {
  kBoardGpioEseRst = 0,
  kBoardGpioEseSvddPwrReq,
  kBoardGpioNfcVen,
  kBoardGpioMax,
} BoardGpio;

/* Allow GPIO assignment and configuration to vary without a new device definition. */
struct NxpSpiBoard {
  const char *dev_path;
  int gpios[kBoardGpioMax];
  uint8_t mode;
  uint32_t bits;
  uint32_t speed;
};

#endif  /* ESE_HW_NXP_SPI_BOARD_H_ */


