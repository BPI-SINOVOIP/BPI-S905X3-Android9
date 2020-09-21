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

(function ($, moment) {

  /**
   * Display test plan metadata in a vertical popout.
   * @param container The jquery object in which to insert the plan metadata.
   * @param metadataList The list of metadata objects to render on the display.
   */
  function renderCard(container, entry) {
    var card = $('<a class="col s12 m6 l4"></a>');
    var link = (
      '/show_plan_run?plan=' + entry.testPlanRun.testPlanName +
      '&time=' + entry.testPlanRun.startTimestamp);
    card.attr('href', link);
    card.appendTo(container);
    var div = $('<div class="hoverable card release-entry"></div>');
    var startTime = entry.testPlanRun.startTimestamp;
    var endTime = entry.testPlanRun.endTimestamp;
    div.appendTo(card);
    var span = $('<span></span>');
    span.addClass('plan-run-metadata');
    span.appendTo(div);
    $('<b></b>').text(entry.deviceInfo).appendTo(span);
    span.append('<br>');
    $('<b></b>').text('VTS Build: ').appendTo(span);
    span.append(entry.testPlanRun.testBuildId).append('<br>');
    var timeString = (
      moment().renderTime(startTime, false) + ' - ' +
      moment().renderTime(endTime, true) + ' (' +
      moment().renderDuration(endTime - startTime) + ')');
    span.append(timeString);
    var counter = $('<span></span>');
    var color = entry.testPlanRun.failCount > 0 ? 'red' : 'green';
    counter.addClass('counter center ' + color);
    counter.append(
      entry.testPlanRun.passCount + '/' +
      (entry.testPlanRun.passCount + entry.testPlanRun.failCount));
    counter.appendTo(div);
  }

  $.fn.showPlanRuns = function(data) {
    var self = $(this);
    data.forEach(function (entry) {
      renderCard(self, entry);
    })
  }

})(jQuery, moment);
