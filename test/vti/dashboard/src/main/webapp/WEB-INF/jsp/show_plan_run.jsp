<%--
  ~ Copyright (c) 2017 Google Inc. All Rights Reserved.
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
  <link type='text/css' href='/css/show_test_runs_common.css' rel='stylesheet'>
  <link type='text/css' href='/css/test_results.css' rel='stylesheet'>
  <script type='text/javascript' src='https://www.gstatic.com/charts/loader.js'></script>
  <script src='https://www.gstatic.com/external_hosted/moment/min/moment-with-locales.min.js'></script>
  <script src='js/time.js'></script>
  <script src='js/test_results.js'></script>
  <script type='text/javascript'>
      google.charts.load('current', {'packages':['table', 'corechart']});
      google.charts.setOnLoadCallback(drawPieChart);

      $(document).ready(function() {
          $('#test-results-container').showTests(${testRuns}, true);
          drawSummary();
      });

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

      // Draw a test plan run summary box.
      function drawSummary() {
          var testBuildId = ${testBuildId};
          var startTime = ${startTime};
          var endTime = ${endTime};
          var moduleCount = ${moduleCount};
          var passingTestCaseCount = ${passingTestCaseCount};
          var failingTestCaseCount = ${failingTestCaseCount};
          var div = $('<div></div>');
          var details = $('<span></span>').appendTo(div);
          details.append('<b>VTS Build: </b>' + testBuildId + '<br>');
          details.append('<b>Start Time: </b>' + moment().renderTime(startTime, true) + '<br>');
          details.append('<b>End Time: </b>' + moment().renderTime(endTime, true) + '<br>');
          details.append('<b>Duration: </b>' + moment().renderDuration(endTime - startTime) + '<br>');
          details.append('<b>Modules: </b>' + moduleCount + '<br>');
          details.append('<b>Passing Test Cases: </b>' + passingTestCaseCount + '<br>');
          details.append('<b>Non-Passing Test Cases: </b>' + failingTestCaseCount + '<br>');
          div.appendTo($('#summary-container'));
      }
  </script>

  <body>
    <div class='wide container'>
      <div class='row'>
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
          <div id='summary-container' class='col s12 card'>
            <span class='summary-header valign-wrapper'>
              <i class='material-icons'>info_outline</i>Run Details
            </span>
          </div>
        </div>
        <div class='col s5 valign-wrapper'>
          <!-- pie chart -->
          <div id='pie-chart-wrapper' class='col s12 valign center-align card'>
            <div id='pie-chart-div'></div>
          </div>
        </div>
      </div>

      <div class='col s12' id='test-results-container'>
      </div>
    </div>
    <%@ include file="footer.jsp" %>
  </body>
</html>
