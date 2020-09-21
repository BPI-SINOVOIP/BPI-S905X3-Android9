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
 * Usage:
 *   $0 [impl name] < line-by-line-input-and-expect
 */

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define LOG_TAG "ese-replay"
#include <ese/ese.h>
#include <ese/log.h>

#include "buffer.h"
#include "hw.h"
#include "payload.h"

const struct SupportedHardware kSupportedHardware = {
    .len = 3,
    .hw =
        {
            {
                .name = "nq-nci",
                .sym = "ESE_HW_NXP_PN80T_NQ_NCI_ops",
                .lib = "libese-hw-nxp-pn80t-nq-nci.so",
                .options = NULL,
            },
            {
                .name = "fake",
                .sym = "ESE_HW_FAKE_ops",
                .lib = "libese-hw-fake.so",
                .options = NULL,
            },
            {
                .name = "echo",
                .sym = "ESE_HW_ECHO_ops",
                .lib = "libese-hw-echo.so",
                .options = NULL,
            },
        },
};

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Usage:\n%s [hw_impl] < file_with_apdus\n\n"
           "File format:\n"
           "  hex-apdu-to-send hex-trailing-response-bytes\\n\n"
           "\n"
           "For example,\n"
           "  echo -e '00A4040000 9000\\n80CA9F7F00 9000\\n' | %s nq-nci\n",
           argv[0], argv[0]);
    print_supported_hardware(&kSupportedHardware);
    return 1;
  }
  int hw_id = find_supported_hardware(&kSupportedHardware, argv[1]);
  if (hw_id < 0) {
    fprintf(stderr, "Unknown hardware name: %s\n", argv[1]);
    return 3;
  }
  const struct Hardware *hw = &kSupportedHardware.hw[hw_id];

  struct EseInterface ese;
  printf("[-] Initializing eSE\n");

  if (!initialize_hardware(&ese, hw)) {
    fprintf(stderr, "Could not initialize hardware\n");
    return 2;
  }
  printf("eSE implementation selected: %s\n", ese_name(&ese));
  if (ese_open(&ese, hw->options)) {
    ALOGE("Cannot open hw");
    if (ese_error(&ese)) {
      ALOGE("eSE error (%d): %s", ese_error_code(&ese),
            ese_error_message(&ese));
    }
    return 5;
  }
  printf("eSE is open\n");
  struct Payload payload;
  if (!payload_init(&payload, 10 * 1024 * 1024, 1024 * 4)) {
    ALOGE("Failed to initialize payload.");
    return -1;
  }

  struct Buffer reply;
  buffer_init(&reply, 2048);
  while (!feof(stdin) && payload_read(&payload, stdin)) {
    payload_dump(&payload, stdout);
    reply.len = (uint32_t)ese_transceive(
        &ese, payload.tx.buffer, payload.tx.len, reply.buffer, reply.size);
    if ((int)reply.len < 0 || ese_error(&ese)) {
      printf("Transceive error. See logcat -s ese-replay\n");
      ALOGE("transceived returned failure: %d\n", (int)reply.len);
      if (ese_error(&ese)) {
        ALOGE("An error (%d) occurred: %s", ese_error_code(&ese),
              ese_error_message(&ese));
      }
      break;
    }
    buffer_dump(&reply, "", "Response", 240, stdout);
    if (reply.len < payload.expected.len) {
      printf("Received less data than expected: %u < %u\n", reply.len,
             payload.expected.len);
      break;
    }

    /* Only compare the end. This allows a simple APDU success match. */
    if (memcmp(payload.expected.buffer,
               (reply.buffer + reply.len) - payload.expected.len,
               payload.expected.len)) {
      printf("Response did not match. Aborting!\n");
      break;
    }
  }
  buffer_free(&reply);
  printf("Transmissions complete.\n");
  ese_close(&ese);
  release_hardware(hw);
  return 0;
}
