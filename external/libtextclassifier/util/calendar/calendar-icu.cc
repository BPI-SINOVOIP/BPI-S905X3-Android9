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

#include "util/calendar/calendar-icu.h"

#include <memory>

#include "util/base/macros.h"
#include "unicode/gregocal.h"
#include "unicode/timezone.h"
#include "unicode/ucal.h"

namespace libtextclassifier2 {
namespace {
int MapToDayOfWeekOrDefault(int relation_type, int default_value) {
  switch (relation_type) {
    case DateParseData::MONDAY:
      return UCalendarDaysOfWeek::UCAL_MONDAY;
    case DateParseData::TUESDAY:
      return UCalendarDaysOfWeek::UCAL_TUESDAY;
    case DateParseData::WEDNESDAY:
      return UCalendarDaysOfWeek::UCAL_WEDNESDAY;
    case DateParseData::THURSDAY:
      return UCalendarDaysOfWeek::UCAL_THURSDAY;
    case DateParseData::FRIDAY:
      return UCalendarDaysOfWeek::UCAL_FRIDAY;
    case DateParseData::SATURDAY:
      return UCalendarDaysOfWeek::UCAL_SATURDAY;
    case DateParseData::SUNDAY:
      return UCalendarDaysOfWeek::UCAL_SUNDAY;
    default:
      return default_value;
  }
}

bool DispatchToRecedeOrToLastDayOfWeek(icu::Calendar* date, int relation_type,
                                       int distance) {
  UErrorCode status = U_ZERO_ERROR;
  switch (relation_type) {
    case DateParseData::MONDAY:
    case DateParseData::TUESDAY:
    case DateParseData::WEDNESDAY:
    case DateParseData::THURSDAY:
    case DateParseData::FRIDAY:
    case DateParseData::SATURDAY:
    case DateParseData::SUNDAY:
      for (int i = 0; i < distance; i++) {
        do {
          if (U_FAILURE(status)) {
            TC_LOG(ERROR) << "error day of week";
            return false;
          }
          date->add(UCalendarDateFields::UCAL_DAY_OF_MONTH, 1, status);
          if (U_FAILURE(status)) {
            TC_LOG(ERROR) << "error adding a day";
            return false;
          }
        } while (date->get(UCalendarDateFields::UCAL_DAY_OF_WEEK, status) !=
                 MapToDayOfWeekOrDefault(relation_type, 1));
      }
      return true;
    case DateParseData::DAY:
      date->add(UCalendarDateFields::UCAL_DAY_OF_MONTH, -1 * distance, status);
      if (U_FAILURE(status)) {
        TC_LOG(ERROR) << "error adding a day";
        return false;
      }

      return true;
    case DateParseData::WEEK:
      date->set(UCalendarDateFields::UCAL_DAY_OF_WEEK, 1);
      date->add(UCalendarDateFields::UCAL_DAY_OF_MONTH, -7 * (distance - 1),
                status);
      if (U_FAILURE(status)) {
        TC_LOG(ERROR) << "error adding a week";
        return false;
      }

      return true;
    case DateParseData::MONTH:
      date->set(UCalendarDateFields::UCAL_DAY_OF_MONTH, 1);
      date->add(UCalendarDateFields::UCAL_MONTH, -1 * (distance - 1), status);
      if (U_FAILURE(status)) {
        TC_LOG(ERROR) << "error adding a month";
        return false;
      }
      return true;
    case DateParseData::YEAR:
      date->set(UCalendarDateFields::UCAL_DAY_OF_YEAR, 1);
      date->add(UCalendarDateFields::UCAL_YEAR, -1 * (distance - 1), status);
      if (U_FAILURE(status)) {
        TC_LOG(ERROR) << "error adding a year";

        return true;
        default:
          return false;
      }
      return false;
  }
}

bool DispatchToAdvancerOrToNextOrSameDayOfWeek(icu::Calendar* date,
                                               int relation_type) {
  UErrorCode status = U_ZERO_ERROR;
  switch (relation_type) {
    case DateParseData::MONDAY:
    case DateParseData::TUESDAY:
    case DateParseData::WEDNESDAY:
    case DateParseData::THURSDAY:
    case DateParseData::FRIDAY:
    case DateParseData::SATURDAY:
    case DateParseData::SUNDAY:
      while (date->get(UCalendarDateFields::UCAL_DAY_OF_WEEK, status) !=
             MapToDayOfWeekOrDefault(relation_type, 1)) {
        if (U_FAILURE(status)) {
          TC_LOG(ERROR) << "error day of week";
          return false;
        }
        date->add(UCalendarDateFields::UCAL_DAY_OF_MONTH, 1, status);
        if (U_FAILURE(status)) {
          TC_LOG(ERROR) << "error adding a day";
          return false;
        }
      }
      return true;
    case DateParseData::DAY:
      date->add(UCalendarDateFields::UCAL_DAY_OF_MONTH, 1, status);
      if (U_FAILURE(status)) {
        TC_LOG(ERROR) << "error adding a day";
        return false;
      }

      return true;
    case DateParseData::WEEK:
      date->set(UCalendarDateFields::UCAL_DAY_OF_WEEK, 1);
      date->add(UCalendarDateFields::UCAL_DAY_OF_MONTH, 7, status);
      if (U_FAILURE(status)) {
        TC_LOG(ERROR) << "error adding a week";
        return false;
      }

      return true;
    case DateParseData::MONTH:
      date->set(UCalendarDateFields::UCAL_DAY_OF_MONTH, 1);
      date->add(UCalendarDateFields::UCAL_MONTH, 1, status);
      if (U_FAILURE(status)) {
        TC_LOG(ERROR) << "error adding a month";
        return false;
      }
      return true;
    case DateParseData::YEAR:
      date->set(UCalendarDateFields::UCAL_DAY_OF_YEAR, 1);
      date->add(UCalendarDateFields::UCAL_YEAR, 1, status);
      if (U_FAILURE(status)) {
        TC_LOG(ERROR) << "error adding a year";

        return true;
        default:
          return false;
      }
      return false;
  }
}

bool DispatchToAdvancerOrToNextDayOfWeek(icu::Calendar* date, int relation_type,
                                         int distance) {
  UErrorCode status = U_ZERO_ERROR;
  switch (relation_type) {
    case DateParseData::MONDAY:
    case DateParseData::TUESDAY:
    case DateParseData::WEDNESDAY:
    case DateParseData::THURSDAY:
    case DateParseData::FRIDAY:
    case DateParseData::SATURDAY:
    case DateParseData::SUNDAY:
      for (int i = 0; i < distance; i++) {
        do {
          if (U_FAILURE(status)) {
            TC_LOG(ERROR) << "error day of week";
            return false;
          }
          date->add(UCalendarDateFields::UCAL_DAY_OF_MONTH, 1, status);
          if (U_FAILURE(status)) {
            TC_LOG(ERROR) << "error adding a day";
            return false;
          }
        } while (date->get(UCalendarDateFields::UCAL_DAY_OF_WEEK, status) !=
                 MapToDayOfWeekOrDefault(relation_type, 1));
      }
      return true;
    case DateParseData::DAY:
      date->add(UCalendarDateFields::UCAL_DAY_OF_MONTH, distance, status);
      if (U_FAILURE(status)) {
        TC_LOG(ERROR) << "error adding a day";
        return false;
      }

      return true;
    case DateParseData::WEEK:
      date->set(UCalendarDateFields::UCAL_DAY_OF_WEEK, 1);
      date->add(UCalendarDateFields::UCAL_DAY_OF_MONTH, 7 * distance, status);
      if (U_FAILURE(status)) {
        TC_LOG(ERROR) << "error adding a week";
        return false;
      }

      return true;
    case DateParseData::MONTH:
      date->set(UCalendarDateFields::UCAL_DAY_OF_MONTH, 1);
      date->add(UCalendarDateFields::UCAL_MONTH, 1 * distance, status);
      if (U_FAILURE(status)) {
        TC_LOG(ERROR) << "error adding a month";
        return false;
      }
      return true;
    case DateParseData::YEAR:
      date->set(UCalendarDateFields::UCAL_DAY_OF_YEAR, 1);
      date->add(UCalendarDateFields::UCAL_YEAR, 1 * distance, status);
      if (U_FAILURE(status)) {
        TC_LOG(ERROR) << "error adding a year";

        return true;
        default:
          return false;
      }
      return false;
  }
}

bool RoundToGranularity(DatetimeGranularity granularity,
                        icu::Calendar* calendar) {
  // Force recomputation before doing the rounding.
  UErrorCode status = U_ZERO_ERROR;
  calendar->get(UCalendarDateFields::UCAL_DAY_OF_WEEK, status);
  if (U_FAILURE(status)) {
    TC_LOG(ERROR) << "Can't interpret date.";
    return false;
  }

  switch (granularity) {
    case GRANULARITY_YEAR:
      calendar->set(UCalendarDateFields::UCAL_MONTH, 0);
      TC_FALLTHROUGH_INTENDED;
    case GRANULARITY_MONTH:
      calendar->set(UCalendarDateFields::UCAL_DAY_OF_MONTH, 1);
      TC_FALLTHROUGH_INTENDED;
    case GRANULARITY_DAY:
      calendar->set(UCalendarDateFields::UCAL_HOUR, 0);
      TC_FALLTHROUGH_INTENDED;
    case GRANULARITY_HOUR:
      calendar->set(UCalendarDateFields::UCAL_MINUTE, 0);
      TC_FALLTHROUGH_INTENDED;
    case GRANULARITY_MINUTE:
      calendar->set(UCalendarDateFields::UCAL_SECOND, 0);
      break;

    case GRANULARITY_WEEK:
      calendar->set(UCalendarDateFields::UCAL_DAY_OF_WEEK,
                    calendar->getFirstDayOfWeek());
      calendar->set(UCalendarDateFields::UCAL_HOUR, 0);
      calendar->set(UCalendarDateFields::UCAL_MINUTE, 0);
      calendar->set(UCalendarDateFields::UCAL_SECOND, 0);
      break;

    case GRANULARITY_UNKNOWN:
    case GRANULARITY_SECOND:
      break;
  }

  return true;
}

}  // namespace

bool CalendarLib::InterpretParseData(const DateParseData& parse_data,
                                     int64 reference_time_ms_utc,
                                     const std::string& reference_timezone,
                                     const std::string& reference_locale,
                                     DatetimeGranularity granularity,
                                     int64* interpreted_time_ms_utc) const {
  UErrorCode status = U_ZERO_ERROR;

  std::unique_ptr<icu::Calendar> date(icu::Calendar::createInstance(
      icu::Locale::createFromName(reference_locale.c_str()), status));
  if (U_FAILURE(status)) {
    TC_LOG(ERROR) << "error getting calendar instance";
    return false;
  }

  date->adoptTimeZone(icu::TimeZone::createTimeZone(
      icu::UnicodeString::fromUTF8(reference_timezone)));
  date->setTime(reference_time_ms_utc, status);

  // By default, the parsed time is interpreted to be on the reference day. But
  // a parsed date, should have time 0:00:00 unless specified.
  date->set(UCalendarDateFields::UCAL_HOUR_OF_DAY, 0);
  date->set(UCalendarDateFields::UCAL_MINUTE, 0);
  date->set(UCalendarDateFields::UCAL_SECOND, 0);
  date->set(UCalendarDateFields::UCAL_MILLISECOND, 0);

  static const int64 kMillisInHour = 1000 * 60 * 60;
  if (parse_data.field_set_mask & DateParseData::Fields::ZONE_OFFSET_FIELD) {
    date->set(UCalendarDateFields::UCAL_ZONE_OFFSET,
              parse_data.zone_offset * kMillisInHour);
  }
  if (parse_data.field_set_mask & DateParseData::Fields::DST_OFFSET_FIELD) {
    // convert from hours to milliseconds
    date->set(UCalendarDateFields::UCAL_DST_OFFSET,
              parse_data.dst_offset * kMillisInHour);
  }

  if (parse_data.field_set_mask & DateParseData::Fields::RELATION_FIELD) {
    switch (parse_data.relation) {
      case DateParseData::Relation::NEXT:
        if (parse_data.field_set_mask &
            DateParseData::Fields::RELATION_TYPE_FIELD) {
          if (!DispatchToAdvancerOrToNextDayOfWeek(
                  date.get(), parse_data.relation_type, 1)) {
            return false;
          }
        }
        break;
      case DateParseData::Relation::NEXT_OR_SAME:
        if (parse_data.field_set_mask &
            DateParseData::Fields::RELATION_TYPE_FIELD) {
          if (!DispatchToAdvancerOrToNextOrSameDayOfWeek(
                  date.get(), parse_data.relation_type)) {
            return false;
          }
        }
        break;
      case DateParseData::Relation::LAST:
        if (parse_data.field_set_mask &
            DateParseData::Fields::RELATION_TYPE_FIELD) {
          if (!DispatchToRecedeOrToLastDayOfWeek(date.get(),
                                                 parse_data.relation_type, 1)) {
            return false;
          }
        }
        break;
      case DateParseData::Relation::NOW:
        // NOOP
        break;
      case DateParseData::Relation::TOMORROW:
        date->add(UCalendarDateFields::UCAL_DAY_OF_MONTH, 1, status);
        if (U_FAILURE(status)) {
          TC_LOG(ERROR) << "error adding a day";
          return false;
        }
        break;
      case DateParseData::Relation::YESTERDAY:
        date->add(UCalendarDateFields::UCAL_DAY_OF_MONTH, -1, status);
        if (U_FAILURE(status)) {
          TC_LOG(ERROR) << "error subtracting a day";
          return false;
        }
        break;
      case DateParseData::Relation::PAST:
        if (parse_data.field_set_mask &
            DateParseData::Fields::RELATION_TYPE_FIELD) {
          if (parse_data.field_set_mask &
              DateParseData::Fields::RELATION_DISTANCE_FIELD) {
            if (!DispatchToRecedeOrToLastDayOfWeek(
                    date.get(), parse_data.relation_type,
                    parse_data.relation_distance)) {
              return false;
            }
          }
        }
        break;
      case DateParseData::Relation::FUTURE:
        if (parse_data.field_set_mask &
            DateParseData::Fields::RELATION_TYPE_FIELD) {
          if (parse_data.field_set_mask &
              DateParseData::Fields::RELATION_DISTANCE_FIELD) {
            if (!DispatchToAdvancerOrToNextDayOfWeek(
                    date.get(), parse_data.relation_type,
                    parse_data.relation_distance)) {
              return false;
            }
          }
        }
        break;
    }
  }
  if (parse_data.field_set_mask & DateParseData::Fields::YEAR_FIELD) {
    date->set(UCalendarDateFields::UCAL_YEAR, parse_data.year);
  }
  if (parse_data.field_set_mask & DateParseData::Fields::MONTH_FIELD) {
    // NOTE: Java and ICU disagree on month formats
    date->set(UCalendarDateFields::UCAL_MONTH, parse_data.month - 1);
  }
  if (parse_data.field_set_mask & DateParseData::Fields::DAY_FIELD) {
    date->set(UCalendarDateFields::UCAL_DAY_OF_MONTH, parse_data.day_of_month);
  }
  if (parse_data.field_set_mask & DateParseData::Fields::HOUR_FIELD) {
    if (parse_data.field_set_mask & DateParseData::Fields::AMPM_FIELD &&
        parse_data.ampm == 1 && parse_data.hour < 12) {
      date->set(UCalendarDateFields::UCAL_HOUR_OF_DAY, parse_data.hour + 12);
    } else {
      date->set(UCalendarDateFields::UCAL_HOUR_OF_DAY, parse_data.hour);
    }
  }
  if (parse_data.field_set_mask & DateParseData::Fields::MINUTE_FIELD) {
    date->set(UCalendarDateFields::UCAL_MINUTE, parse_data.minute);
  }
  if (parse_data.field_set_mask & DateParseData::Fields::SECOND_FIELD) {
    date->set(UCalendarDateFields::UCAL_SECOND, parse_data.second);
  }

  if (!RoundToGranularity(granularity, date.get())) {
    return false;
  }

  *interpreted_time_ms_utc = date->getTime(status);
  if (U_FAILURE(status)) {
    TC_LOG(ERROR) << "error getting time from instance";
    return false;
  }

  return true;
}
}  // namespace libtextclassifier2
