/**
 * Copyright (c) 2017 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you
 * may not use this file except in compliance with the License. You may
 * obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

(function (moment) {

  /**
   * Renders a timestamp in the user timezone.
   * @param timestamp The long timestamp to render (in microseconds).
   * @param showTimezone True if the timezone should be rendered, false otherwise.
   * @returns the string-formatted version of the provided timestamp.
   */
  moment.prototype.renderTime = function (timestamp, showTimezone) {
    var time = moment(timestamp / 1000);
    var format = 'YYYY-M-DD H:mm:ss';
    if (!!showTimezone) {
        format = format + 'ZZ';
    }
    return time.format(format);
  }

  /**
   * Renders a date in the user timezone.
   * @param timestamp The long timestamp to render (in microseconds).
   * @param showTimezone True if the timezone should be rendered, false otherwise.
   * @returns the string-formatted version of the provided timestamp.
   */
  moment.prototype.renderDate = function (timestamp, showTimezone) {
    var time = moment(timestamp / 1000);
    var format = 'YYYY-M-DD';
    if (!!showTimezone) {
        format = format + 'ZZ';
    }
    return time.format(format);
  }

  /**
   * Renders a duration in the user timezone.
   * @param durationTimestamp The long duration to render (in microseconds).
   * @returns the string-formatted duration of the provided duration timestamp.
   */
  moment.prototype.renderDuration = function (durationTimestamp) {
    var fmt = 's[s]';
    var duration = moment.utc(durationTimestamp / 1000);
    if (duration.hours() > 0) {
      fmt = 'H[h], m[m], ' + fmt;
    } else if (duration.minutes() > 0) {
      fmt = 'm[m], ' + fmt;
    }
    return duration.format(fmt);
  }

})(moment);
