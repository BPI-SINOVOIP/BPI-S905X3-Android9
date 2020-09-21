/*
 * Copyright (C) 2018 The Android Open Source Project
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

#ifndef KEYSTORE_OPERATION_PROTO_HANDLER_H_
#define KEYSTORE_OPERATION_PROTO_HANDLER_H_

#include "operation_struct.h"

namespace keystore {

using ::android::IBinder;
using keymaster::support::Keymaster;

void uploadOpAsProto(Operation& op, bool wasOpSuccessful);

}  // namespace keystore

#endif  // KEYSTORE_OPERATION_PROTO_HANDLER_H_
