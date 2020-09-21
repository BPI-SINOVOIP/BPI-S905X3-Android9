/*
 * Copyright 2017 The Android Open Source Project
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

#ifndef LIBATAP_H_
#define LIBATAP_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Library users only need to include this file to integrate with the
 * Android Things Attestation Provisioning reference library.
 */

#include "atap_ops.h"
#include "atap_sysdeps.h"
#include "atap_types.h"
#include "atap_util.h"

/*
 * Parses |operation_start_size| bytes from |operation_start| and
 * constructs a request for the Android Things CA. Number of bytes
 * allocated for the CA Request message is written to
 * |*ca_request_size_p|, and caller takes ownership of |*ca_request_p|,
 * and must free with atap_free(). On success, returns ATAP_RESULT_OK.
 */
AtapResult atap_get_ca_request(AtapOps* ops,
                               const uint8_t* operation_start,
                               uint32_t operation_start_size,
                               uint8_t** ca_request_p,
                               uint32_t* ca_request_size_p);
/*
 * Stores product keys issued/certified by the Android Things CA.
 * |ca_response| contains |ca_response_size| bytes of the CA Response
 * message, which holds the encrypted product certificate chains and
 * (optionally) keys. On success, returns ATAP_RESULT_OK.
 */
AtapResult atap_set_ca_response(AtapOps* ops,
                                const uint8_t* ca_response,
                                uint32_t ca_response_size);

#ifdef __cplusplus
}
#endif

#endif /* LIBATAP_H_ */
