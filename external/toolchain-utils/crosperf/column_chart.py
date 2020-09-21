# Copyright 2011 Google Inc. All Rights Reserved.
"""Module to draw column chart."""


class ColumnChart(object):
  """class to draw column chart."""

  def __init__(self, title, width, height):
    self.title = title
    self.chart_div = filter(str.isalnum, title)
    self.width = width
    self.height = height
    self.columns = []
    self.rows = []
    self.series = []

  def AddSeries(self, column_name, series_type, color):
    for i in range(len(self.columns)):
      if column_name == self.columns[i][1]:
        self.series.append((i - 1, series_type, color))
        break

  def AddColumn(self, name, column_type):
    self.columns.append((column_type, name))

  def AddRow(self, row):
    self.rows.append(row)

  def GetJavascript(self):
    res = 'var data = new google.visualization.DataTable();\n'
    for column in self.columns:
      res += "data.addColumn('%s', '%s');\n" % column
    res += 'data.addRows(%s);\n' % len(self.rows)
    for row in range(len(self.rows)):
      for column in range(len(self.columns)):
        val = self.rows[row][column]
        if isinstance(val, str):
          val = "'%s'" % val
        res += 'data.setValue(%s, %s, %s);\n' % (row, column, val)

    series_javascript = ''
    for series in self.series:
      series_javascript += "%s: {type: '%s', color: '%s'}, " % series

    chart_add_javascript = """
var chart_%s = new google.visualization.ComboChart(
  document.getElementById('%s'));
chart_%s.draw(data, {width: %s, height: %s, title: '%s', legend: 'none',
  seriesType: "bars", lineWidth: 0, pointSize: 5, series: {%s},
  vAxis: {minValue: 0}})
"""

    res += chart_add_javascript % (self.chart_div, self.chart_div,
                                   self.chart_div, self.width, self.height,
                                   self.title, series_javascript)
    return res

  def GetDiv(self):
    return "<div id='%s' class='chart'></div>" % self.chart_div
