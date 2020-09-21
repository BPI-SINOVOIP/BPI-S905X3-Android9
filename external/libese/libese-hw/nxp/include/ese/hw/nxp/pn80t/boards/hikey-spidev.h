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

/* No guard is intentional given the definition below. */
#include <ese/hw/nxp/spi_board.h>

static const struct NxpSpiBoard nxp_boards_hikey_spidev = {
  .dev_path = "/dev/spidev0.0",
  .gpios = {
    488, /* kBoardGpioEseRst = GPIO2_0 */
    490, /* kBoardGpioEseSvddPwrReq = GPIO2_2 */
    -1,  /* kBoardGpioNfcVen = unused */
  },
  .mode = 0,
  .bits = 8,
  .speed = 1000000L,
};
