# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Text templates used by various parts of results_report."""
from __future__ import print_function

import cgi
from string import Template

_TabMenuTemplate = Template("""
<div class='tab-menu'>
  <a href="javascript:switchTab('$table_name', 'html')">HTML</a>
  <a href="javascript:switchTab('$table_name', 'text')">Text</a>
  <a href="javascript:switchTab('$table_name', 'tsv')">TSV</a>
</div>""")


def _GetTabMenuHTML(table_name):
  # N.B. cgi.escape does some very basic HTML escaping. Nothing more.
  escaped = cgi.escape(table_name, quote=True)
  return _TabMenuTemplate.substitute(table_name=escaped)


_ExperimentFileHTML = """
<div class='results-section'>
  <div class='results-section-title'>Experiment File</div>
  <div class='results-section-content'>
    <pre>%s</pre>
</div>
"""


def _GetExperimentFileHTML(experiment_file_text):
  if not experiment_file_text:
    return ''
  return _ExperimentFileHTML % (cgi.escape(experiment_file_text),)


_ResultsSectionHTML = Template("""
<div class='results-section'>
  <div class='results-section-title'>$sect_name</div>
  <div class='results-section-content'>
    <div id='${short_name}-html'>$html_table</div>
    <div id='${short_name}-text'><pre>$text_table</pre></div>
    <div id='${short_name}-tsv'><pre>$tsv_table</pre></div>
  </div>
  $tab_menu
</div>
""")


def _GetResultsSectionHTML(print_table, table_name, data):
  first_word = table_name.strip().split()[0]
  short_name = first_word.lower()
  return _ResultsSectionHTML.substitute(
      sect_name=table_name,
      html_table=print_table(data, 'HTML'),
      text_table=print_table(data, 'PLAIN'),
      tsv_table=print_table(data, 'TSV'),
      tab_menu=_GetTabMenuHTML(short_name),
      short_name=short_name)


_MainHTML = Template("""
<html>
<head>
  <style type="text/css">
    body {
      font-family: "Lucida Sans Unicode", "Lucida Grande", Sans-Serif;
      font-size: 12px;
    }

    pre {
      margin: 10px;
      color: #039;
      font-size: 14px;
    }

    .chart {
      display: inline;
    }

    .hidden {
      visibility: hidden;
    }

    .results-section {
      border: 1px solid #b9c9fe;
      margin: 10px;
    }

    .results-section-title {
      background-color: #b9c9fe;
      color: #039;
      padding: 7px;
      font-size: 14px;
      width: 200px;
    }

    .results-section-content {
      margin: 10px;
      padding: 10px;
      overflow:auto;
    }

    #box-table-a {
      font-size: 12px;
      width: 480px;
      text-align: left;
      border-collapse: collapse;
    }

    #box-table-a th {
      padding: 6px;
      background: #b9c9fe;
      border-right: 1px solid #fff;
      border-bottom: 1px solid #fff;
      color: #039;
      text-align: center;
    }

    #box-table-a td {
      padding: 4px;
      background: #e8edff;
      border-bottom: 1px solid #fff;
      border-right: 1px solid #fff;
      color: #669;
      border-top: 1px solid transparent;
    }

    #box-table-a tr:hover td {
      background: #d0dafd;
      color: #339;
    }

  </style>
  <script type='text/javascript' src='https://www.google.com/jsapi'></script>
  <script type='text/javascript'>
    google.load('visualization', '1', {packages:['corechart']});
    google.setOnLoadCallback(init);
    function init() {
      switchTab('summary', 'html');
      ${perf_init};
      switchTab('full', 'html');
      drawTable();
    }
    function drawTable() {
      ${chart_js};
    }
    function switchTab(table, tab) {
      document.getElementById(table + '-html').style.display = 'none';
      document.getElementById(table + '-text').style.display = 'none';
      document.getElementById(table + '-tsv').style.display = 'none';
      document.getElementById(table + '-' + tab).style.display = 'block';
    }
  </script>
</head>

<body>
  $summary_table
  $perf_html
  <div class='results-section'>
    <div class='results-section-title'>Charts</div>
    <div class='results-section-content'>$chart_divs</div>
  </div>
  $full_table
  $experiment_file
</body>
</html>
""")


# It's a bit ugly that we take some HTML things, and some non-HTML things, but I
# need to balance prettiness with time spent making things pretty.
def GenerateHTMLPage(perf_table, chart_js, summary_table, print_table,
                     chart_divs, full_table, experiment_file):
  """Generates a crosperf HTML page from the given arguments.

  print_table is a two-arg function called like: print_table(t, f)
    t is one of [summary_table, print_table, full_table]; it's the table we want
      to format.
    f is one of ['TSV', 'HTML', 'PLAIN']; it's the type of format we want.
  """
  summary_table_html = _GetResultsSectionHTML(print_table, 'Summary Table',
                                              summary_table)
  if perf_table:
    perf_html = _GetResultsSectionHTML(print_table, 'Perf Table', perf_table)
    perf_init = "switchTab('perf', 'html')"
  else:
    perf_html = ''
    perf_init = ''

  full_table_html = _GetResultsSectionHTML(print_table, 'Full Table',
                                           full_table)
  experiment_file_html = _GetExperimentFileHTML(experiment_file)
  return _MainHTML.substitute(
      perf_init=perf_init,
      chart_js=chart_js,
      summary_table=summary_table_html,
      perf_html=perf_html,
      chart_divs=chart_divs,
      full_table=full_table_html,
      experiment_file=experiment_file_html)
