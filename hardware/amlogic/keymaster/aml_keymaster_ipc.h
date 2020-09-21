/*
 * Copyright (C) 2015 The Android Open Source Project
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

#ifndef AML_KEYMASTER_AML_KEYMASTER_IPC_H_
#define AML_KEYMASTER_AML_KEYMASTER_IPC_H_

extern "C" {
#include <tee_client_api.h>
}

__BEGIN_DECLS

TEEC_Result aml_keymaster_connect(TEEC_Context *c, TEEC_Session *s);
TEEC_Result aml_keymaster_call(TEEC_Session *s, uint32_t cmd, void* in, uint32_t in_size, uint8_t* out,
                          uint32_t* out_size);
TEEC_Result aml_keymaster_disconnect(TEEC_Context *c, TEEC_Session *s);

__END_DECLS

#endif  // AML_KEYMASTER_AML_KEYMASTER_IPC_H_
