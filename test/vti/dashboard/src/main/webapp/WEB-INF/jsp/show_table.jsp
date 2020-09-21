<%--
  ~ Copyright (c) 2016 Google Inc. All Rights Reserved.
  ~
  ~ Licensed under the Apache License, Version 2.0 (the "License"); you
  ~ may not use this file except in compliance with the License. You may
  ~ obtain a copy of the License at
  ~
  ~     http://www.apache.org/licenses/LICENSE-2.0
  ~
  ~ Unless required by applicable law or agreed to in writing, software
  ~ distributed under the License is distributed on an "AS IS" BASIS,
  ~ WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
  ~ implied. See the License for the specific language governing
  ~ permissions and limitations under the License.
  --%>
<%@ page contentType='text/html;charset=UTF-8' language='java' %>
<%@ taglib prefix='fn' uri='http://java.sun.com/jsp/jstl/functions' %>
<%@ taglib prefix='c' uri='http://java.sun.com/jsp/jstl/core'%>

<html>
  <%@ include file="header.jsp" %>
  <link type='text/css' href='/css/show_table.css' rel='stylesheet'>
  <link type='text/css' href='/css/show_test_runs_common.css' rel='stylesheet'>
  <link type='text/css' href='/css/search_header.css' rel='stylesheet'>
  <script type='text/javascript' src='https://www.gstatic.com/charts/loader.js'></script>
  <script src='https://www.gstatic.com/external_hosted/moment/min/moment-with-locales.min.js'></script>
  <script src='js/search_header.js'></script>
  <script src='js/time.js'></script>
  <script type='text/javascript'>
      google.charts.load('current', {'packages':['table', 'corechart']});
      google.charts.setOnLoadCallback(drawGridTable);
      google.charts.setOnLoadCallback(activateLogLinks);
      google.charts.setOnLoadCallback(drawPieChart);
      google.charts.setOnLoadCallback(function() {
          $('.gradient').removeClass('gradient');
      });

      var search;

      $(document).ready(function() {
          search = $('#filter-bar').createSearchHeader('Module: ', '${testName}', refresh);
          search.addFilter('Branch', 'branch', {
            corpus: ${branches}
          }, ${branch});
          search.addFilter('Device', 'device', {
            corpus: ${devices}
          }, ${device});
          search.addFilter('Device Build ID', 'deviceBuildId', {}, ${deviceBuildId});
          search.addFilter('Test Build ID', 'testBuildId', {}, ${testBuildId});
          search.addFilter('Host', 'hostname', {}, ${hostname});
          search.addFilter('Passing Count', 'passing', {
            validate: 'inequality',
            width: 's2'
          }, ${passing});
          search.addFilter('Non-Passing Count', 'nonpassing', {
            validate: 'inequality',
            width: 's2'
          }, ${nonpassing});
          search.addRunTypeCheckboxes(${showPresubmit}, ${showPostsubmit});
          search.display();

          // disable buttons on load
          if (!${hasNewer}) {
              $('#newer-button').toggleClass('disabled');
          }
          if (!${hasOlder}) {
              $('#older-button').toggleClass('disabled');
          }
          $('#treeLink').click(function() {
              window.open('/show_tree?testName=${testName}&treeDefault=true', '_self');
          });
          $('#newer-button').click(prev);
          $('#older-button').click(next);
      });

      // Actives the log links to display the log info modal when clicked.
      function activateLogLinks() {
          $('.info-btn').click(function(e) {
              showLog(${logInfoMap}[$(this).data('col')]);
          });
      }

      /** Displays a modal window with the specified log entries.
       *
       * @param logEntries Array of string arrays. Each entry in the outer array
       *                   must contain (1) name string, and (2) url string.
       */
      function showLog(logEntries) {
          if (!logEntries || logEntries.length == 0) return;

          var logList = $('<ul class="collection"></ul>');
          var entries = logEntries.reduce(function(acc, entry) {
              if (!entry || entry.length == 0) return acc;
              var link = '<a href="' + entry[1] + '"';
              link += 'class="collection-item">' + entry[0] + '</li>';
              return acc + link;
          }, '');
          logList.html(entries);
          var infoContainer = $('#info-modal>.modal-content>.info-container');
          infoContainer.empty();
          logList.appendTo(infoContainer);
          var modal = $('#info-modal');
          modal.modal();
          modal.modal('open');
      }

      // refresh the page to see the selected test types (pre-/post-submit)
      function refresh() {
          if($(this).hasClass('disabled')) return;
          var link = '${pageContext.request.contextPath}' +
              '/show_table?testName=${testName}' + search.args();
          if (${unfiltered}) {
              link += '&unfiltered=';
          }
          window.open(link,'_self');
      }

      // view older data
      function next() {
          if($(this).hasClass('disabled')) return;
          var endTime = ${startTime};
          var link = '${pageContext.request.contextPath}' +
              '/show_table?testName=${testName}&endTime=' + endTime +
              search.args();
          if (${unfiltered}) {
              link += '&unfiltered=';
          }
          window.open(link,'_self');
      }

      // view newer data
      function prev() {
          if($(this).hasClass('disabled')) return;
          var startTime = ${endTime};
          var link = '${pageContext.request.contextPath}' +
              '/show_table?testName=${testName}&startTime=' + startTime +
              search.args();
          if (${unfiltered}) {
              link += '&unfiltered=';
          }
          window.open(link,'_self');
        }

      // to draw pie chart
      function drawPieChart() {
          var topBuildResultCounts = ${topBuildResultCounts};
          if (topBuildResultCounts.length < 1) {
              return;
          }
          var resultNames = ${resultNamesJson};
          var rows = resultNames.map(function(res, i) {
              nickname = res.replace('TEST_CASE_RESULT_', '').replace('_', ' ')
                         .trim().toLowerCase();
              return [nickname, parseInt(topBuildResultCounts[i])];
          });
          rows.unshift(['Result', 'Count']);

          // Get CSS color definitions (or default to white)
          var colors = resultNames.map(function(res) {
              return $('.' + res).css('background-color') || 'white';
          });

          var data = google.visualization.arrayToDataTable(rows);
          var options = {
              is3D: false,
              colors: colors,
              fontName: 'Roboto',
              fontSize: '14px',
              legend: {position: 'bottom'},
              tooltip: {showColorCode: true, ignoreBounds: false},
              chartArea: {height: '80%', width: '90%'},
              pieHole: 0.4
          };

          var chart = new google.visualization.PieChart(document.getElementById('pie-chart-div'));
          chart.draw(data, options);
      }

      // table for grid data
      function drawGridTable() {
          var data = new google.visualization.DataTable();

          // Add column headers.
          headerRow = ${headerRow};
          headerRow.forEach(function(d, i) {
              var classNames = 'table-header-content';
              if (i == 0) classNames += ' table-header-legend';
              data.addColumn('string', '<span class="' + classNames + '">' +
                             d + '</span>');
          });

          var timeGrid = ${timeGrid};
          var durationGrid = ${durationGrid};
          var summaryGrid = ${summaryGrid};
          var resultsGrid = ${resultsGrid};

          // Format time grid to a formatted date
          timeGrid = timeGrid.map(function(row) {
              return row.map(function(cell, j) {
                  if (j == 0) return cell;
                  return moment().renderTime(cell);
              });
          });

          // Format duration grid to HH:mm:ss.SSS
          durationGrid = durationGrid.map(function(row) {
              return row.map(function(cell, j) {
                  if (j == 0) return cell;
                  return moment().renderDuration(cell);
              });
          });

          // add rows to the data.
          data.addRows(timeGrid);
          data.addRows(durationGrid);
          data.addRows(summaryGrid);
          data.addRows(resultsGrid);

          var table = new google.visualization.Table(document.getElementById('grid-table-div'));
          var classNames = {
              headerRow : 'table-header',
              headerCell : 'table-header-cell'
          };
          var options = {
              showRowNumber: false,
              alternatingRowStyle: true,
              allowHtml: true,
              frozenColumns: 1,
              cssClassNames: classNames,
              sort: 'disable'
          };
          table.draw(data, options);
      }
  </script>

  <body>
    <div class='wide container'>
      <div class='row'>
        <div class='col s12'>
          <div class='card'>
            <ul class='tabs'>
              <li class='tab col s6' id='treeLink'><a>Tree</a></li>
              <li class='tab col s6'><a class='active'>Table</a></li>
            </ul>
          </div>
          <div id='filter-bar'></div>
        </div>
        <div class='col s7'>
          <div class='col s12 card center-align'>
            <div id='legend-wrapper'>
              <c:forEach items='${resultNames}' var='res'>
                <div class='center-align legend-entry'>
                  <c:set var='trimmed' value='${fn:replace(res, "TEST_CASE_RESULT_", "")}'/>
                  <c:set var='nickname' value='${fn:replace(trimmed, "_", " ")}'/>
                  <label for='${res}'>${nickname}</label>
                  <div id='${res}' class='${res} legend-bubble'></div>
                </div>
              </c:forEach>
            </div>
          </div>
          <div id='profiling-container' class='col s12'>
            <c:choose>
              <c:when test='${empty profilingPointNames}'>
                <div id='error-div' class='center-align card'><h5>${error}</h5></div>
              </c:when>
              <c:otherwise>
                <ul id='profiling-body' class='collapsible' data-collapsible='accordion'>
                  <li>
                    <div class='collapsible-header'><i class='material-icons'>timeline</i>Profiling Graphs</div>
                    <div class='collapsible-body'>
                      <ul id='profiling-list' class='collection'>
                        <c:forEach items='${profilingPointNames}' var='pt'>
                          <c:set var='profPointArgs' value='testName=${testName}&profilingPoint=${pt}'/>
                          <c:set var='timeArgs' value='endTime=${endTime}'/>
                          <a href='/show_graph?${profPointArgs}&${timeArgs}'
                             class='collection-item profiling-point-name'>${pt}
                          </a>
                        </c:forEach>
                      </ul>
                    </div>
                  </li>
                  <li>
                    <a class='collapsible-link' href='/show_performance_digest?testName=${testName}'>
                      <div class='collapsible-header'><i class='material-icons'>toc</i>Performance Digest</div>
                    </a>
                  </li>
                </ul>
              </c:otherwise>
            </c:choose>
          </div>
        </div>
        <div class='col s5 valign-wrapper'>
          <!-- pie chart -->
          <div id='pie-chart-wrapper' class='col s12 valign center-align card'>
            <h6 class='pie-chart-title'>Test Status for Device Build ID: ${topBuildId}</h6>
            <div id='pie-chart-div'></div>
          </div>
        </div>
      </div>

      <div class='col s12'>
        <div id='chart-holder' class='col s12 card'>
          <!-- Grid tables-->
          <div id='grid-table-div'></div>
        </div>
      </div>
      <div id='newer-wrapper' class='page-button-wrapper fixed-action-btn'>
        <a id='newer-button' class='btn-floating btn red waves-effect'>
          <i class='large material-icons'>keyboard_arrow_left</i>
        </a>
      </div>
      <div id='older-wrapper' class='page-button-wrapper fixed-action-btn'>
        <a id='older-button' class='btn-floating btn red waves-effect'>
          <i class='large material-icons'>keyboard_arrow_right</i>
        </a>
      </div>
    </div>
    <div id="help-modal" class="modal">
      <div class="modal-content">
        <h4>${searchHelpHeader}</h4>
        <p>${searchHelpBody}</p>
      </div>
      <div class="modal-footer">
        <a href="#!" class="modal-action modal-close waves-effect btn-flat">Close</a>
      </div>
    </div>
    <div id="info-modal" class="modal">
      <div class="modal-content">
        <h4>Logs</h4>
        <div class="info-container"></div>
      </div>
      <div class="modal-footer">
        <a href="#!" class="modal-action modal-close waves-effect btn-flat">Close</a>
      </div>
    </div>
    <%@ include file="footer.jsp" %>
  </body>
</html>
