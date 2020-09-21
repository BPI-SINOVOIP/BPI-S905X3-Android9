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
   * Display the log links in a modal window.
   * @param linkList A list of [name, url] tuples representing log links.
   */
  function showLinks(container, linkList) {
    if (!linkList || linkList.length == 0) return;

    var logCollection = $('<ul class="collection"></ul>');
    var entries = linkList.reduce(function(acc, entry) {
      if (!entry || entry.length == 0) return acc;
      var link = '<a href="' + entry[1] + '"';
      link += 'class="collection-item">' + entry[0] + '</li>';
      return acc + link;
    }, '');
    logCollection.html(entries);

    if (container.find('#info-modal').length == 0) {
      var modal = $('<div id="info-modal" class="modal modal-fixed-footer"></div>');
      var content = $('<div class="modal-content"></div>');
      content.append('<h4>Links</h4>');
      content.append('<div class="info-container"></div>');
      content.appendTo(modal);
      var footer = $('<div class="modal-footer"></div>');
      footer.append('<a class="btn-flat modal-close">Close</a></div>');
      footer.appendTo(modal);
      modal.appendTo(container);
    }
    var infoContainer = $('#info-modal>.modal-content>.info-container');
    infoContainer.empty();
    logCollection.appendTo(infoContainer);
    $('#info-modal').modal({
      dismissible: true
    });
    $('#info-modal').modal('open');
  }

  /**
   * Get the nickname for a test case result.
   *
   * Removes the result prefix and suffix, extracting only the result name.
   *
   * @param testCaseResult The string name of a VtsReportMessage.TestCaseResult.
   * @returns the string nickname of the result.
   */
  function getNickname(testCaseResult) {
    return testCaseResult
      .replace('TEST_CASE_RESULT_', '')
      .replace('_RESULT', '')
      .trim().toLowerCase();
  }

  /**
   * Display test data in the body beneath a test run's metadata.
   * @param container The jquery object in which to insert the test metadata.
   * @param data The json object containing the columns to display.
   * @param lineHeight The height of each list element.
   */
  function displayTestDetails(container, data, lineHeight) {
    var nCol = data.length;
    var width = 's' + (12 / nCol);
    test = container;
    var maxLines = 0;
    data.forEach(function (column, index) {
      if (column.data == undefined || column.name == undefined) {
        return;
      }
      var classes = 'col test-col grey lighten-5 ' + width;
      if (index != nCol -1) {
        classes += ' bordered';
      }
      if (index == 0) {
        classes += ' left-most';
      }
      if (index == nCol - 1) {
        classes += ' right-most';
      }
      var colContainer = $('<div class="' + classes + '"></div>');
      var col = $('<div class="test-case-container"></div>');
      colContainer.appendTo(container);
      var count = column.data.length;
      var head = $('<h5 class="test-result-label white"></h5>')
        .text(getNickname(column.name))
        .appendTo(colContainer)
        .css('text-transform', 'capitalize');
      $('<div class="indicator right center"></div>')
        .text(count)
        .addClass(column.name)
        .appendTo(head);
      col.appendTo(colContainer);
      var list = $('<ul></ul>').appendTo(col);
      column.data.forEach(function (testCase) {
        $('<li></li>')
          .text(testCase)
          .addClass('test-case')
          .css('font-size', lineHeight - 2)
          .css('line-height', lineHeight + 'px')
          .appendTo(list);
      });
      if (count > maxLines) {
        maxLines = count;
      }
    });
    var containers = container.find('.test-case-container');
    containers.height(maxLines * lineHeight);
  }

  /**
   * Click handler for displaying test run details.
   * @param e The click event.
   */
  function testRunClick(e) {
    var header = $(this);
    var icon = header.find('.material-icons.expand-arrow');
    var container = header.parent().find('.test-results');
    var test = header.attr('test');
    var time = header.attr('time');
    var url = '/api/test_run?test=' + test + '&timestamp=' + time;
    if (header.parent().hasClass('active')) {
      header.parent().removeClass('active');
      header.removeClass('active');
      icon.removeClass('rotate');
      header.siblings('.collapsible-body').stop(true, false).slideUp({
        duration: 100,
        easing: "easeOutQuart",
        queue: false,
        complete: function() { header.css('height', ''); }
      });
    } else {
      container.empty();
      header.parent().addClass('active');
      header.addClass('active');
      header.addClass('disabled');
      icon.addClass('rotate');
      $.get(url).done(function(data) {
        displayTestDetails(container, data, 16);
        header.siblings('.collapsible-body').stop(true, false).slideDown({
          duration: 100,
          easing: "easeOutQuart",
          queue: false,
          complete: function() { header.css('height', ''); }
        });
      }).fail(function() {
        icon.removeClass('rotate');
      }).always(function() {
        header.removeClass('disabled');
      });
    }
  }

  /**
   * Append a clickable indicator link to the container.
   * @param container The jquery object to append the indicator to.
   * @param content The text to display in the indicator.
   * @param classes Additional space-delimited classes to add to the indicator.
   * @param click The click handler to assign to the indicator.
   * @returns The jquery object for the indicator.
   */
  function createClickableIndicator(container, content, classes, click) {
    var link = $('<a></a>');
    link.addClass('indicator right center padded hoverable waves-effect');
    link.addClass(classes)
    link.append(content);
    link.appendTo(container);
    link.click(click);
    return link;
  }

  function displayTestMetadata(container, metadataList, showTestNames=false) {
    var popout = $('<ul></ul>');
    popout.attr('data-collapsible', 'expandable');
    popout.addClass('collapsible popout test-runs');
    popout.appendTo(container);
    popout.unbind();
    metadataList.forEach(function (metadata) {
      var li = $('<li class="test-run-container"></li>');
      li.appendTo(popout);
      var div = $('<div></div>');
      var test = metadata.testRun.testName;
      var startTime = metadata.testRun.startTimestamp;
      var endTime = metadata.testRun.endTimestamp;
      div.attr('test', test);
      div.attr('time', startTime);
      div.addClass('collapsible-header test-run');
      div.appendTo(li);
      div.unbind().click(testRunClick);
      var span = $('<span></span>');
      span.addClass('test-run-metadata');
      span.appendTo(div);
      span.click(function() { return false; });
      if (showTestNames) {
          $('<span class="test-run-label"></span>').text(test).appendTo(span);
          span.append('<br>');
      }
      if (metadata.deviceInfo) {
          $('<b></b>').text(metadata.deviceInfo).appendTo(span);
          span.append('<br>');
      }
      if (metadata.abiInfo) {
          $('<b></b>').text('ABI: ')
                .appendTo(span)
          span.append(metadata.abiInfo).append('<br>');
      }
      $('<b></b>').text('VTS Build: ')
            .appendTo(span)
      span.append(metadata.testRun.testBuildId).append('<br>');
      $('<b></b>').text('Host: ')
            .appendTo(span)
      span.append(metadata.testRun.hostName).append('<br>');
      var timeString = (
        moment().renderTime(startTime, false) + ' - ' +
        moment().renderTime(endTime, true) + ' (' +
        moment().renderDuration(endTime - startTime) + ')');
      span.append(timeString);
      var indicator = $('<span></span>');
      var color = metadata.testRun.failCount > 0 ? 'red' : 'green';
      indicator.addClass('indicator right center ' + color);
      indicator.append(
        metadata.testRun.passCount + '/' +
        (metadata.testRun.passCount + metadata.testRun.failCount));
      indicator.appendTo(div);
      if (metadata.testRun.coveredLineCount != undefined &&
        metadata.testRun.totalLineCount != undefined) {
        var url = (
          '/show_coverage?testName=' + test + '&startTime=' + startTime);
        covered = metadata.testRun.coveredLineCount;
        total = metadata.testRun.totalLineCount;
        covPct = Math.round(covered / total * 1000) / 10;
        var color = 'red';
        if (covPct > 20 && covPct < 70) {
          color = 'orange';
        } else if (covPct >= 70) {
          color = 'green';
        }
        var coverage = (
          'Coverage: ' + covered + '/' + total + ' (' + covPct + '%)');
        createClickableIndicator(
          div, coverage, color,
          function () { window.location.href = url; return false; });
      }
      if (metadata.testRun.logLinks != undefined) {
        createClickableIndicator(
          div, 'Links', 'grey lighten-1',
          function () {
            showLinks(popout, metadata.testRun.logLinks);
            return false;
          });
      }
      var expand = $('<i></i>');
      expand.addClass('material-icons expand-arrow')
      expand.text('expand_more');
      expand.appendTo(div);
      var body = $('<div></div>')
        .addClass('collapsible-body test-results row')
        .appendTo(li);
      if (metadata.testDetails != undefined) {
        expand.addClass('rotate');
        li.addClass('active');
        div.addClass('active');
        displayTestDetails(body, metadata.testDetails, 16);
        div.siblings('.collapsible-body').stop(true, false).slideDown({
          duration: 0,
          queue: false,
          complete: function() { div.css('height', ''); }
        });
      }
    });
  }

  /**
   * Display test metadata in a vertical popout.
   * @param container The jquery object in which to insert the test metadata.
   * @param metadataList The list of metadata objects to render on the display.
   * @param showTestNames True to label each entry with the test module name.
   */
  $.fn.showTests = function(metadataList, showTestNames=false) {
    displayTestMetadata($(this), metadataList, showTestNames);
  }

})(jQuery, moment);
