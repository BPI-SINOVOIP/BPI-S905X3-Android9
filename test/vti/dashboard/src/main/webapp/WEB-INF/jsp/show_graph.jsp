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
  <link type='text/css' href='/css/datepicker.css' rel='stylesheet'>
  <link type='text/css' href='/css/show_graph.css' rel='stylesheet'>
  <link rel='stylesheet' href='https://ajax.googleapis.com/ajax/libs/jqueryui/1.12.0/jquery-ui.css'>
  <script type='text/javascript' src='https://www.gstatic.com/charts/loader.js'></script>
  <script type='text/javascript' src='https://ajax.googleapis.com/ajax/libs/jqueryui/1.12.1/jquery-ui.min.js'></script>
  <body>
    <script type='text/javascript'>
        google.charts.load('current', {packages:['corechart', 'table', 'line']});
        google.charts.setOnLoadCallback(drawAllGraphs);

        ONE_DAY = 86400000000;
        MICRO_PER_MILLI = 1000;
        N_BUCKETS = 200;

        var graphs = ${graphs};

        $(function() {
            $('select').material_select();
            var date = $('#date').datepicker({
                showAnim: 'slideDown',
                maxDate: new Date()
            });
            date.datepicker('setDate', new Date(${endTime} / MICRO_PER_MILLI));
            $('#load').click(load);
            $('#outlier-select').change(drawAllGraphs);
        });

        // Draw all graphs.
        function drawAllGraphs() {
            $('#profiling-container').empty();
            var percentileIndex = Number($('#outlier-select').val());

            // Get histogram extrema
            var histMin = null;
            var histMax = null;
            graphs.forEach(function(g) {
                if (g.type != 'HISTOGRAM') return;
                var minVal;
                var maxVal;
                if (percentileIndex == -1) {
                    minVal = g.min;
                    maxVal = g.max;
                } else {
                    minVal = g.percentile_values[percentileIndex];
                    var endIndex = g.percentiles.length - percentileIndex - 1
                    maxVal = g.percentile_values[endIndex];
                }
                if (!histMin || minVal < histMin) histMin = minVal;
                if (!histMax || maxVal > histMax) histMax = maxVal;
            });

            graphs.forEach(function(graph) {
                if (graph.type == 'LINE_GRAPH') drawLineGraph(graph);
                else if (graph.type == 'HISTOGRAM')
                    drawHistogram(graph, histMin, histMax);
            });
        }

       /**
        * Draw a line graph.
        *
        * Args:
        *     lineGraph: a JSON object containing the following fields:
        *                - name: the name of the graph
        *                - values: an array of numbers
        *                - ticks: an array of strings to use as x-axis labels
        *                - ids: an array of string labels for each point (e.g. the
        *                       build info for the run that produced the point)
        *                - x_label: the string label for the x axis
        *                - y_label: the string label for the y axis
        */
        function drawLineGraph(lineGraph) {
            if (!lineGraph.ticks || lineGraph.ticks.length < 1) {
                return;
            }
            var title = 'Performance';
            if (lineGraph.name) title += ' (' + lineGraph.name + ')';
            lineGraph.ticks.forEach(function (label, i) {
                lineGraph.values[i].unshift(label);
            });
            var data = new google.visualization.DataTable();
            data.addColumn('string', lineGraph.x_label);
            lineGraph.ids.forEach(function(id) {
                data.addColumn('number', id);
            });
            data.addRows(lineGraph.values);
            var options = {
              chart: {
                  title: title,
                  subtitle: lineGraph.y_label
              },
              legend: { position: 'none' }
            };
            var container = $('<div class="row card center-align col s12 graph-wrapper"></div>');
            container.appendTo('#profiling-container');
            var chartDiv = $('<div class="col s12 graph"></div>');
            chartDiv.appendTo(container);
            var chart = new google.charts.Line(chartDiv[0]);
            chart.draw(data, options);
        }

       /**
        * Draw a histogram.
        *
        * Args:
        *     hist: a JSON object containing the following fields:
        *           - name: the name of the graph
        *           - values: an array of numbers
        *           - ids: an array of string labels for each point (e.g. the
        *                  build info for the run that produced the point)
        *           - x_label: the string label for the x axis
        *           - y_label: the string label for the y axis
        *     min: the minimum value to display
        *     max: the maximum value to display
        */
        function drawHistogram(hist, min, max) {
            if (!hist.values || hist.values.length == 0) return;
            var title = 'Performance';
            if (hist.name) title += ' (' + hist.name + ')';
            var values = hist.values;
            var histogramData = values.reduce(function(result, d, i) {
                if (d <= max && d >= min) result.push([hist.ids[i], d]);
                return result;
            }, []);

            var data = google.visualization.arrayToDataTable(histogramData, true);
            var bucketSize = (max - min) / N_BUCKETS;

            var options = {
                title: title,
                titleTextStyle: {
                    color: '#757575',
                    fontSize: 16,
                    bold: false
                },
                legend: { position: 'none' },
                colors: ['#4285F4'],
                fontName: 'Roboto',
                vAxis:{
                    title: hist.y_label,
                    titleTextStyle: {
                        color: '#424242',
                        fontSize: 12,
                        italic: false
                    },
                    textStyle: {
                        fontSize: 12,
                        color: '#757575'
                    },
                },
                hAxis: {
                    title: hist.x_label,
                    textStyle: {
                        fontSize: 12,
                        color: '#757575'
                    },
                    titleTextStyle: {
                        color: '#424242',
                        fontSize: 12,
                        italic: false
                    }
                },
                bar: { gap: 0 },
                histogram: {
                    minValue: min,
                    maxValue: max,
                    maxNumBuckets: N_BUCKETS,
                    bucketSize: bucketSize
                },
                chartArea: {
                    width: '100%',
                    top: 40,
                    left: 60,
                    height: 375
                }
            };
            var container = $('<div class="row card col s12 graph-wrapper"></div>');
            container.appendTo('#profiling-container');

            var chartDiv = $('<div class="col s12 graph"></div>');
            chartDiv.appendTo(container);
            var chart = new google.visualization.Histogram(chartDiv[0]);
            chart.draw(data, options);

            var tableDiv = $('<div class="col s12"></div>');
            tableDiv.appendTo(container);

            var tableHtml = '<table class="percentile-table"><thead><tr>';
            hist.percentiles.forEach(function(p) {
                tableHtml += '<th data-field="id">' + p + '%</th>';
            });
            tableHtml += '</tr></thead><tbody><tr>';
            hist.percentile_values.forEach(function(v) {
                tableHtml += '<td>' + v + '</td>';
            });
            tableHtml += '</tbody></table>';
            $(tableHtml).appendTo(tableDiv);
        }

        // Reload the page.
        function load() {
            var endTime = $('#date').datepicker('getDate').getTime();
            endTime = endTime + (ONE_DAY / MICRO_PER_MILLI) - 1;
            var filterVal = $('#outlier-select').val();
            var ctx = '${pageContext.request.contextPath}';
            var link = ctx + '/show_graph?profilingPoint=${profilingPointName}' +
                '&testName=${testName}' +
                '&endTime=' + (endTime * MICRO_PER_MILLI) +
                '&filterVal=' + filterVal;
            if ($('#device-select').prop('selectedIndex') > 1) {
                link += '&device=' + $('#device-select').val();
            }
            window.open(link,'_self');
        }
    </script>
    <div id='download' class='fixed-action-btn'>
      <a id='b' class='btn-floating btn-large red waves-effect waves-light'>
        <i class='large material-icons'>file_download</i>
      </a>
    </div>
    <div class='container wide'>
      <div class='row card'>
        <div id='header-container' class='valign-wrapper col s12'>
          <div class='col s3 valign'>
            <h5>Profiling Point:</h5>
          </div>
          <div class='col s9 right-align valign'>
            <h5 class='profiling-name truncate'>${profilingPointName}</h5>
          </div>
        </div>
        <div id='date-container' class='col s12'>
          <c:set var='offset' value='${showFilterDropdown ? 0 : 2}' />
          <c:if test='${showFilterDropdown}'>
            <div id='outlier-select-wrapper' class='col s2'>
              <select id='outlier-select'>
                <option value='-1' ${filterVal eq -1 ? 'selected' : ''}>Show outliers</option>
                <option value='0' ${filterVal eq 0 ? 'selected' : ''}>Filter outliers (1%)</option>
                <option value='1' ${filterVal eq 1 ? 'selected' : ''}>Filter outliers (2%)</option>
                <option value='2' ${filterVal eq 2 ? 'selected' : ''}>Filter outliers (5%)</option>
              </select>
            </div>
          </c:if>
          <div id='device-select-wrapper' class='input-field col s5 m3 offset-m${offset + 4} offset-s${offset}'>
            <select id='device-select'>
              <option value='' disabled>Select device</option>
              <option value='0' ${empty selectedDevice ? 'selected' : ''}>All Devices</option>
              <c:forEach items='${devices}' var='device' varStatus='loop'>
                <option value=${device} ${selectedDevice eq device ? 'selected' : ''}>${device}</option>
              </c:forEach>
            </select>
          </div>
          <input type='text' id='date' name='date' class='col s4 m2'>
          <a id='load' class='btn-floating btn-medium red right waves-effect waves-light'>
            <i class='medium material-icons'>cached</i>
          </a>
        </div>
      </div>
      <div id='profiling-container'>
      </div>
      <c:if test='${not empty error}'>
        <div id='error-container' class='row card'>
          <div class='col s10 offset-s1 center-align'>
            <!-- Error in case of profiling data is missing -->
            <h5>${error}</h5>
          </div>
        </div>
      </c:if>
    </div>
    <%@ include file="footer.jsp" %>
  </body>
</html>
