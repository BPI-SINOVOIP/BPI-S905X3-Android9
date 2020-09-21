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

#ifndef ESE_OPERATIONS_WRAPPER_H_
#define ESE_OPERATIONS_WRAPPER_H_ 1

#include <ese/ese.h>
#include "ese_operations_interface.h"

// Wraps a supplied interface object with static calls.
// This acts as a singleton. If mulitple instances are needed, the
// multion pattern would be more appropriate.
class EseOperationsWrapperData {
 public:
  static EseOperationsInterface *ops_interface;
  static const struct EseOperations ops;
  static const struct EseOperations *wrapper_ops;
};

class EseOperationsWrapper {
 public:
  // Naming convention to be compatible with ese_init();
  EseOperationsWrapper() = default;
  virtual ~EseOperationsWrapper() = default;
  static void InitializeEse(struct EseInterface *ese, EseOperationsInterface *ops_interface);
};

#endif  // ESE_OPERATIONS_WRAPPER_H_
