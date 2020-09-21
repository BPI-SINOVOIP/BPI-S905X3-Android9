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

#include <nos/debug.h>

#include <application.h>

namespace nos {

#define ErrorString_helper(x) \
    case app_status::x: \
      return #x;

std::string StatusCodeString(uint32_t code) {
  switch (code) {
    ErrorString_helper(APP_SUCCESS)
    ErrorString_helper(APP_ERROR_BOGUS_ARGS)
    ErrorString_helper(APP_ERROR_INTERNAL)
    ErrorString_helper(APP_ERROR_TOO_MUCH)
    ErrorString_helper(APP_ERROR_IO)
    ErrorString_helper(APP_ERROR_RPC)
    default:
      if (code >= APP_LINE_NUMBER_BASE && code < MAX_APP_STATUS) {
        return "APP_LINE_NUMBER " + std::to_string(code - APP_LINE_NUMBER_BASE);
      }
      if (code >= APP_SPECIFIC_ERROR && code < APP_LINE_NUMBER_BASE) {
        return "APP_SPECIFIC_ERROR " + std::to_string(APP_LINE_NUMBER_BASE) +
            " + " + std::to_string(code - APP_LINE_NUMBER_BASE);
      }

      return "unknown";
  }
}

}  // namespace nos
