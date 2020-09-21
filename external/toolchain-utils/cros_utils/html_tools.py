# Copyright 2010 Google Inc. All Rights Reserved.
"""Utilities for generating html."""


def GetPageHeader(page_title):
  return """<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html>
<head>
<style type="text/css">
table
{
border-collapse:collapse;
}
table, td, th
{
border:1px solid black;
}
</style>
<script type="text/javascript">
function displayRow(id){
  var row = document.getElementById("group_"+id);
  if (row.style.display == '')  row.style.display = 'none';
    else row.style.display = '';
  }
</script>
<title>%s</title>
</head>
<body>

""" % page_title


def GetListHeader():
  return '<ul>'


def GetListItem(text):
  return '<li>%s</li>' % text


def GetListFooter():
  return '</ul>'


def GetList(items):
  return '<ul>%s</ul>' % ''.join(['<li>%s</li>' % item for item in items])


def GetParagraph(text):
  return '<p>%s</p>' % text


def GetFooter():
  return '</body>\n</html>'


def GetHeader(text, h=1):
  return '<h%s>%s</h%s>' % (h, text, h)


def GetTableHeader(headers):
  row = ''.join(['<th>%s</th>' % header for header in headers])
  return '<table><tr>%s</tr>' % row


def GetTableFooter():
  return '</table>'


def FormatLineBreaks(text):
  return text.replace('\n', '<br/>')


def GetTableCell(text):
  return '<td>%s</td>' % FormatLineBreaks(str(text))


def GetTableRow(columns):
  return '<tr>%s</tr>' % '\n'.join([GetTableCell(column) for column in columns])


def GetTable(headers, rows):
  table = [GetTableHeader(headers)]
  table.extend([GetTableRow(row) for row in rows])
  table.append(GetTableFooter())
  return '\n'.join(table)


def GetLink(link, text):
  return "<a href='%s'>%s</a>" % (link, text)
