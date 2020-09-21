/*
 * Copyright (C) 2012 The Android Open Source Project
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

#pragma once

// clang-format off

#define KEYMASTER_PORT "com.android.trusty.keymaster"
#define KEYMASTER_MAX_BUFFER_LENGTH 4096

/* This UUID is generated with uuidgen
   the ITU-T UUID generator at http://www.itu.int/ITU-T/asn1/uuid.html */
#define TA_KEYMASTER_UUID {0x8efb1e1c, 0x37e5, 0x4326, \
    { 0xa5, 0xd6, 0x8c, 0x33, 0x72, 0x6c, 0x7d, 0x57} }

// Commands
enum keymaster_command : uint32_t {
    KEYMASTER_RESP_BIT              = 1,
    KEYMASTER_STOP_BIT               = 2,
    KEYMASTER_REQ_SHIFT              = 2,

    KM_GENERATE_KEY                 = (0 << KEYMASTER_REQ_SHIFT),
    KM_BEGIN_OPERATION              = (1 << KEYMASTER_REQ_SHIFT),
    KM_UPDATE_OPERATION             = (2 << KEYMASTER_REQ_SHIFT),
    KM_FINISH_OPERATION             = (3 << KEYMASTER_REQ_SHIFT),
    KM_ABORT_OPERATION              = (4 << KEYMASTER_REQ_SHIFT),
    KM_IMPORT_KEY                   = (5 << KEYMASTER_REQ_SHIFT),
    KM_EXPORT_KEY                   = (6 << KEYMASTER_REQ_SHIFT),
    KM_GET_VERSION                  = (7 << KEYMASTER_REQ_SHIFT),
    KM_ADD_RNG_ENTROPY              = (8 << KEYMASTER_REQ_SHIFT),
    KM_GET_SUPPORTED_ALGORITHMS     = (9 << KEYMASTER_REQ_SHIFT),
    KM_GET_SUPPORTED_BLOCK_MODES    = (10 << KEYMASTER_REQ_SHIFT),
    KM_GET_SUPPORTED_PADDING_MODES  = (11 << KEYMASTER_REQ_SHIFT),
    KM_GET_SUPPORTED_DIGESTS        = (12 << KEYMASTER_REQ_SHIFT),
    KM_GET_SUPPORTED_IMPORT_FORMATS = (13 << KEYMASTER_REQ_SHIFT),
    KM_GET_SUPPORTED_EXPORT_FORMATS = (14 << KEYMASTER_REQ_SHIFT),
    KM_GET_KEY_CHARACTERISTICS      = (15 << KEYMASTER_REQ_SHIFT),
    KM_ATTEST_KEY                   = (16 << KEYMASTER_REQ_SHIFT),
    KM_UPGRADE_KEY                  = (17 << KEYMASTER_REQ_SHIFT),
    KM_CONFIGURE                    = (18 << KEYMASTER_REQ_SHIFT),

    KM_SET_BOOT_PARAMS               = (0x1000 << KEYMASTER_REQ_SHIFT),
    KM_SET_ATTESTATION_KEY           = (0x2000 << KEYMASTER_REQ_SHIFT),
    KM_APPEND_ATTESTATION_CERT_CHAIN = (0x3000 << KEYMASTER_REQ_SHIFT),

    KM_TA_INIT                       = (0x10000 << KEYMASTER_REQ_SHIFT),
    KM_TA_TERM                       = (0x10001 << KEYMASTER_REQ_SHIFT),
};

#ifdef __ANDROID__

/**
 * keymaster_message - Serial header for communicating with KM server
 * @cmd: the command, one of keymaster_command.
 * @payload: start of the serialized command specific payload
 */
struct keymaster_message {
    uint32_t cmd;
    uint8_t payload[0];
};

#endif
