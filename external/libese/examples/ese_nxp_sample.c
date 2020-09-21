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
 * Test program to select the CardManager and request the card production
 * lifcycle data (CPLC).
 */

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include <ese/ese.h>
ESE_INCLUDE_HW(ESE_HW_NXP_PN80T_NQ_NCI);

/* APDU: CLA INS P1-P2 Lc Data Le */
struct Apdu {
  uint32_t length;
  const uint8_t *bytes;
  const char *desc;
};

struct ApduSession {
  uint32_t count;
  const char *desc;
  const struct Apdu *apdus[];
};

const uint8_t kSelectCardManagerBytes[] = {0x00, 0xA4, 0x04, 0x00, 0x00};
/* Implicitly selects the ISD at A0 00 00 01 51 00 00 */
const struct Apdu kSelectCardManager = {
    .length = sizeof(kSelectCardManagerBytes),
    .bytes = &kSelectCardManagerBytes[0],
    .desc = "SELECT CARD MANAGER",
};

const uint8_t kGetCplcBytes[] = {0x80, 0xCA, 0x9F, 0x7F, 0x00};
const struct Apdu kGetCplc = {
    .length = sizeof(kGetCplcBytes),
    .bytes = &kGetCplcBytes[0],
    .desc = "GET CPLC",
};

const struct ApduSession kGetCplcSession = {
    .count = 2,
    .desc = "GET-CPLC",
    .apdus =
        {
            &kSelectCardManager, &kGetCplc,
        },
};

const struct ApduSession kEmptySession = {
    .count = 0, .desc = "Empty session (hw close only)", .apdus = {},
};

/* Define the loader service sessions here! */
const uint8_t kSelectJcopIdentifyBytes[] = {
    0x00, 0xA4, 0x04, 0x00, 0x09, 0xA0, 0x00,
    0x00, 0x01, 0x67, 0x41, 0x30, 0x00, 0xFF,
};
const struct Apdu kSelectJcopIdentify = {
    .length = sizeof(kSelectJcopIdentifyBytes),
    .bytes = &kSelectJcopIdentifyBytes[0],
    .desc = "SELECT JCOP IDENTIFY",
};

const struct ApduSession *kSessions[] = {
    &kGetCplcSession, &kEmptySession,
};

int main() {
  struct EseInterface ese = ESE_INITIALIZER(ESE_HW_NXP_PN80T_NQ_NCI);
  void *ese_hw_open_data = NULL;
  size_t s = 0;
  for (; s < sizeof(kSessions) / sizeof(kSessions[0]); ++s) {
    int recvd;
    uint32_t apdu_index = 0;
    uint8_t rx_buf[1024];
    if (ese_open(&ese, ese_hw_open_data) < 0) {
      printf("Cannot open hw\n");
      if (ese_error(&ese))
        printf("eSE error (%d): %s\n", ese_error_code(&ese),
               ese_error_message(&ese));
      return 1;
    }
    printf("Running session %s\n", kSessions[s]->desc);
    for (; apdu_index < kSessions[s]->count; ++apdu_index) {
      uint32_t i;
      const struct Apdu *apdu = kSessions[s]->apdus[apdu_index];
      printf("Sending APDU %u: %s\n", apdu_index, apdu->desc);
      printf("Sending %u bytes to card\n", apdu->length);
      printf("TX: ");
      for (i = 0; i < apdu->length; ++i)
        printf("%.2X ", apdu->bytes[i]);
      printf("\n");

      recvd = ese_transceive(&ese, (uint8_t *)apdu->bytes, apdu->length, rx_buf,
                             sizeof(rx_buf));
      if (ese_error(&ese)) {
        printf("An error (%d) occurred: %s", ese_error_code(&ese),
               ese_error_message(&ese));
        return -1;
      }
      if (recvd > 0) {
        int i;
        printf("Read %d bytes from card\n", recvd);
        printf("RX: ");
        for (i = 0; i < recvd; ++i)
          printf("%.2X ", rx_buf[i]);
        printf("\n");
      }
      usleep(1000);
    }
    printf("Session completed\n\n");
    ese_close(&ese);
  }
  return 0;
}
