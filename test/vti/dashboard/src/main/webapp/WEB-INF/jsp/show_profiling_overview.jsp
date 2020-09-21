<%--
  ~ Copyright (C) 2017 The Android Open Source Project
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
  <link type='text/css' href='/css/show_profiling_overview.css' rel='stylesheet'>
  <link rel='stylesheet' href='/css/search_header.css'>
  <script src='https://www.gstatic.com/external_hosted/moment/min/moment-with-locales.min.js'></script>
  <script src='js/search_header.js'></script>
  <script src='js/time.js'></script>
  <script type='text/javascript' src='https://www.gstatic.com/charts/loader.js'></script>
  <body>
    <script type='text/javascript'>
        google.charts.load('current', {'packages':['corechart']});
        google.charts.setOnLoadCallback(drawAllPlots);

        var plots = ${plots};
        var search;

        $(document).ready(function() {
            search = $('#filter-bar').createSearchHeader(
                'Profiling Analysis', '', refresh);
            search.addFilter('Branch', 'branch', {
                corpus: ${branches}
            }, ${branch});
            search.addFilter('Device', 'device', {
                corpus: ${devices}
            }, ${device});
          search.display();
      });

        /**
        * Draw a box plot.
        *
        * Args:
        *     container: the jquery selector in which to draw the graph.
        *     lineGraph: a JSON-serialized BoxPlot.
        */
        function drawBoxPlot(container, plot) {
            var data = new google.visualization.DataTable();
            data.addColumn('string', 'x');
            plot.seriesList.forEach(function(series) {
                data.addColumn('number', series);
                data.addColumn({id:'fill', type:'number', role:'interval'});
                data.addColumn({id:'fill', type:'number', role:'interval'});
                data.addColumn({id:'bar', type:'number', role:'interval'});
                data.addColumn({id:'bar', type:'number', role:'interval'});
                data.addColumn({type:'string', role:'tooltip', p: {'html': true}});
            });
            var rows = plot.values.map(function(day) {
                var dateString = new moment().renderDate(1 * day.label);
                var statArray = day.values.map(function(stat) {
                    var tooltip = (
                        dateString +
                        '</br><b>Mean:</b> ' +
                        stat.mean.toFixed(2) +
                        '</br><b>Std:</b> ' +
                        stat.std.toFixed(2) +
                        '</br><b>Count:</b> ' +
                        stat.count.toFixed(0));
                    return [
                        stat.mean,
                        stat.mean + stat.std,
                        stat.mean - stat.std,
                        stat.mean + stat.std,
                        stat.mean - stat.std,
                        tooltip];
                });
                return [].concat.apply(
                    [dateString], statArray);
            });
            data.addRows(rows);

            var options = {
                title: plot.name,
                curveType: 'function',
                intervals: { 'color' : 'series-color' },
                interval: {
                    'fill': {
                        'style': 'area',
                        'curveType': 'function',
                        'fillOpacity': 0.2
                    },
                    'bar': {
                        'style': 'bars',
                        'barWidth': 0,
                        'lineWidth': 1,
                        'pointSize': 3,
                        'fillOpacity': 1
                    }},
                legend: { position: 'bottom' },
                tooltip: { isHtml: true },
                fontName: 'Roboto',
                titleTextStyle: {
                    color: '#757575',
                    fontSize: 16,
                    bold: false
                },
                pointsVisible: true,
                vAxis:{
                    title: plot.y_label,
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
            };
            var plot = new google.visualization.LineChart(container[0]);
            plot.draw(data, options);
        }

        // Draw all graphs.
        function drawAllPlots() {
            var container = $('#profiling-container');
            container.empty();

            plots.forEach(function(g) {
                var plot = $('<div class="box-plot row card"></div>');
                plot.appendTo(container);
                drawBoxPlot(plot.append('<div></div>'), g);
            });
        }

        // refresh the page to see the runs matching the specified filter
        function refresh() {
            if($(this).hasClass('disabled')) return;
            var link = '${pageContext.request.contextPath}' +
                '/show_profiling_overview?testName=${testName}' + search.args();
            window.open(link,'_self');
        }
    </script>
    <div class='container wide'>
      <div id='filter-bar' class='row'></div>
      <div id='profiling-container'>
      </div>
    </div>
    <%@ include file="footer.jsp" %>
  </body>
</html>
