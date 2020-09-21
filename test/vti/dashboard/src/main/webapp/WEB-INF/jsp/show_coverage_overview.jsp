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
  <!-- <link rel='stylesheet' href='/css/dashboard_main.css'> -->
  <%@ include file='header.jsp' %>
  <link type='text/css' href='/css/show_test_runs_common.css' rel='stylesheet'>
  <link type='text/css' href='/css/test_results.css' rel='stylesheet'>
  <link rel='stylesheet' href='/css/search_header.css'>
  <script type='text/javascript' src='https://www.gstatic.com/charts/loader.js'></script>
  <script src='https://www.gstatic.com/external_hosted/moment/min/moment-with-locales.min.js'></script>
  <script src='js/time.js'></script>
  <script src='js/test_results.js'></script>
  <script src='js/search_header.js'></script>
  <script type='text/javascript'>
      google.charts.load('current', {'packages':['table', 'corechart']});
      google.charts.setOnLoadCallback(drawStatsChart);
      google.charts.setOnLoadCallback(drawCoverageCharts);

      var search;

      $(document).ready(function() {
          $('#test-results-container').showTests(${testRuns}, true);
          search = $('#filter-bar').createSearchHeader('Code Coverage', '', refresh);
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
            type: 'number',
            width: 's2'
          }, ${passing});
          search.addFilter('Non-Passing Count', 'nonpassing', {
            type: 'number',
            width: 's2'
          }, ${nonpassing});
          search.addRunTypeCheckboxes(${showPresubmit}, ${showPostsubmit});
          search.display();
      });

      // draw test statistics chart
      function drawStatsChart() {
          var testStats = ${testStats};
          if (testStats.length < 1) {
              return;
          }
          var resultNames = ${resultNamesJson};
          var rows = resultNames.map(function(res, i) {
              nickname = res.replace('TEST_CASE_RESULT_', '').replace('_', ' ')
                         .trim().toLowerCase();
              return [nickname, parseInt(testStats[i])];
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
              legend: {position: 'labeled'},
              tooltip: {showColorCode: true, ignoreBounds: false},
              chartArea: {height: '80%', width: '90%'},
              pieHole: 0.4
          };

          var chart = new google.visualization.PieChart(document.getElementById('pie-chart-stats'));
          chart.draw(data, options);
      }

      // draw the coverage pie charts
      function drawCoverageCharts() {
          var coveredLines = ${coveredLines};
          var uncoveredLines = ${uncoveredLines};
          var rows = [
              ["Result", "Count"],
              ["Covered Lines", coveredLines],
              ["Uncovered Lines", uncoveredLines]
          ];

          // Get CSS color definitions (or default to white)
          var colors = [
              $('.TEST_CASE_RESULT_PASS').css('background-color') || 'white',
              $('.TEST_CASE_RESULT_FAIL').css('background-color') || 'white'
          ]

          var data = google.visualization.arrayToDataTable(rows);


          var optionsRaw = {
              is3D: false,
              colors: colors,
              fontName: 'Roboto',
              fontSize: '14px',
              pieSliceText: 'value',
              legend: {position: 'bottom'},
              chartArea: {height: '80%', width: '90%'},
              tooltip: {showColorCode: true, ignoreBounds: false, text: 'value'},
              pieHole: 0.4
          };

          var optionsNormalized = {
              is3D: false,
              colors: colors,
              fontName: 'Roboto',
              fontSize: '14px',
              legend: {position: 'bottom'},
              tooltip: {showColorCode: true, ignoreBounds: false, text: 'percentage'},
              chartArea: {height: '80%', width: '90%'},
              pieHole: 0.4
          };

          var chart = new google.visualization.PieChart(document.getElementById('pie-chart-coverage-raw'));
          chart.draw(data, optionsRaw);

          chart = new google.visualization.PieChart(document.getElementById('pie-chart-coverage-normalized'));
          chart.draw(data, optionsNormalized);
      }

      // refresh the page to see the runs matching the specified filter
      function refresh() {
        var link = '${pageContext.request.contextPath}' +
            '/show_coverage_overview?' + search.args();
        if (${unfiltered}) {
          link += '&unfiltered=';
        }
        window.open(link,'_self');
      }

  </script>

  <body>
    <div class='wide container'>
      <div id='filter-bar'></div>
      <div class='row'>
        <div class='col s12'>
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
        </div>
        <div class='col s4 valign-wrapper'>
          <!-- pie chart -->
          <div class='pie-chart-wrapper col s12 valign center-align card'>
            <h6 class='pie-chart-title'>Test Statistics</h6>
            <div id='pie-chart-stats' class='pie-chart-div'></div>
          </div>
        </div>
        <div class='col s4 valign-wrapper'>
          <!-- pie chart -->
          <div class='pie-chart-wrapper col s12 valign center-align card'>
            <h6 class='pie-chart-title'>Line Coverage (Raw)</h6>
            <div id='pie-chart-coverage-raw' class='pie-chart-div'></div>
          </div>
        </div>
        <div class='col s4 valign-wrapper'>
          <!-- pie chart -->
          <div class='pie-chart-wrapper col s12 valign center-align card'>
            <h6 class='pie-chart-title'>Line Coverage (Normalized)</h6>
            <div id='pie-chart-coverage-normalized' class='pie-chart-div'></div>
          </div>
        </div>
      </div>
      <div class='col s12' id='test-results-container'>
      </div>
    </div>
    <%@ include file="footer.jsp" %>
  </body>
</html>
