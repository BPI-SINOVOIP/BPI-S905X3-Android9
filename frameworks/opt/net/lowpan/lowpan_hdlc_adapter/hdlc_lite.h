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

#ifndef HEADER_HDLC_LITE_H_INCLUDED
#define HEADER_HDLC_LITE_H_INCLUDED 1

#include <stdint.h>
#include <stdbool.h>

#if defined(__cplusplus)
extern "C" {
#endif

static const uint8_t kHdlcResetSignal[] = { 0x7E, 0x13, 0x11, 0x7E };
static const uint16_t kHdlcCrcCheckValue = 0xf0b8;
static const uint16_t kHdlcCrcResetValue = 0xffff;

#define HDLC_BYTE_FLAG             0x7E
#define HDLC_BYTE_ESC              0x7D
#define HDLC_BYTE_XON              0x11
#define HDLC_BYTE_XOFF             0x13
#define HDLC_BYTE_SPECIAL          0xF8
#define HDLC_ESCAPE_XFORM          0x20

/** HDLC CRC function */
extern uint16_t hdlc_crc16(uint16_t fcs, uint8_t byte);

/** HDLC CRC Finalize function */
extern uint16_t hdlc_crc16_finalize(uint16_t fcs);

/** Returns true if the byte needs to be escaped, false otherwise */
extern bool hdlc_byte_needs_escape(uint8_t byte);

/**
 * Writes one or two HDLC-encoded bytes to out_buffer,
 * and returns how many bytes were written.
 */
extern int hdlc_write_byte(uint8_t* out_buffer, uint8_t byte);

#if defined(__cplusplus)
}
#endif

#endif // HEADER_HDLC_LITE_H_INCLUDED
