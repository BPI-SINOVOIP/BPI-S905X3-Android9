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

#ifndef LIBTEXTCLASSIFIER_UTIL_CALENDAR_CALENDAR_ICU_H_
#define LIBTEXTCLASSIFIER_UTIL_CALENDAR_CALENDAR_ICU_H_

#include <string>

#include "types.h"
#include "util/base/integral_types.h"
#include "util/base/logging.h"

namespace libtextclassifier2 {

class CalendarLib {
 public:
  // Interprets parse_data as milliseconds since_epoch. Relative times are
  // resolved against the current time (reference_time_ms_utc). Returns true if
  // the interpratation was successful, false otherwise.
  bool InterpretParseData(const DateParseData& parse_data,
                          int64 reference_time_ms_utc,
                          const std::string& reference_timezone,
                          const std::string& reference_locale,
                          DatetimeGranularity granularity,
                          int64* interpreted_time_ms_utc) const;
};
}  // namespace libtextclassifier2
#endif  // LIBTEXTCLASSIFIER_UTIL_CALENDAR_CALENDAR_ICU_H_
