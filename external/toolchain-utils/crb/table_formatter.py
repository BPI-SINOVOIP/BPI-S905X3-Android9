import numpy
import re


def IsFloat(text):
  if text is None:
    return False
  try:
    float(text)
    return True
  except ValueError:
    return False


def RemoveTrailingZeros(x):
  ret = x
  ret = re.sub('\.0*$', '', ret)
  ret = re.sub('(\.[1-9]*)0+$', '\\1', ret)
  return ret


def HumanizeFloat(x, n=2):
  if not IsFloat(x):
    return x
  digits = re.findall('[0-9.]', str(x))
  decimal_found = False
  ret = ''
  sig_figs = 0
  for digit in digits:
    if digit == '.':
      decimal_found = True
    elif sig_figs != 0 or digit != '0':
      sig_figs += 1
    if decimal_found and sig_figs >= n:
      break
    ret += digit
  return ret


def GetNSigFigs(x, n=2):
  if not IsFloat(x):
    return x
  my_fmt = '%.' + str(n - 1) + 'e'
  x_string = my_fmt % x
  f = float(x_string)
  return f


def GetFormattedPercent(baseline, other, bad_result='--'):
  result = '%8s' % GetPercent(baseline, other, bad_result)
  return result


def GetPercent(baseline, other, bad_result='--'):
  result = bad_result
  if IsFloat(baseline) and IsFloat(other):
    try:
      pct = (float(other) / float(baseline) - 1) * 100
      result = '%+1.1f' % pct
    except ZeroDivisionError:
      pass
  return result


def FitString(text, length):
  if len(text) == length:
    return text
  elif len(text) > length:
    return text[-length:]
  else:
    fmt = '%%%ds' % length
    return fmt % text


class TableFormatter(object):

  def __init__(self):
    self.d = '\t'
    self.bad_result = 'x'

  def GetTablePercents(self, table):
    # Assumes table is not transposed.
    pct_table = []

    pct_table.append(table[0])
    for i in range(1, len(table)):
      row = []
      row.append(table[i][0])
      for j in range(1, len(table[0])):
        c = table[i][j]
        b = table[i][1]
        p = GetPercent(b, c, self.bad_result)
        row.append(p)
      pct_table.append(row)
    return pct_table

  def FormatFloat(self, c, max_length=8):
    if not IsFloat(c):
      return c
    f = float(c)
    ret = HumanizeFloat(f, 4)
    ret = RemoveTrailingZeros(ret)
    if len(ret) > max_length:
      ret = '%1.1ef' % f
    return ret

  def TransposeTable(self, table):
    transposed_table = []
    for i in range(len(table[0])):
      row = []
      for j in range(len(table)):
        row.append(table[j][i])
      transposed_table.append(row)
    return transposed_table

  def GetTableLabels(self, table):
    ret = ''
    header = table[0]
    for i in range(1, len(header)):
      ret += '%d: %s\n' % (i, header[i])
    return ret

  def GetFormattedTable(self,
                        table,
                        transposed=False,
                        first_column_width=30,
                        column_width=14,
                        percents_only=True,
                        fit_string=True):
    o = ''
    pct_table = self.GetTablePercents(table)
    if transposed == True:
      table = self.TransposeTable(table)
      pct_table = self.TransposeTable(table)

    for i in range(0, len(table)):
      for j in range(len(table[0])):
        if j == 0:
          width = first_column_width
        else:
          width = column_width

        c = table[i][j]
        p = pct_table[i][j]

        # Replace labels with numbers: 0... n
        if IsFloat(c):
          c = self.FormatFloat(c)

        if IsFloat(p) and not percents_only:
          p = '%s%%' % p

        # Print percent values side by side.
        if j != 0:
          if percents_only:
            c = '%s' % p
          else:
            c = '%s (%s)' % (c, p)

        if i == 0 and j != 0:
          c = str(j)

        if fit_string:
          o += FitString(c, width) + self.d
        else:
          o += c + self.d
      o += '\n'
    return o

  def GetGroups(self, table):
    labels = table[0]
    groups = []
    group_dict = {}
    for i in range(1, len(labels)):
      label = labels[i]
      stripped_label = self.GetStrippedLabel(label)
      if stripped_label not in group_dict:
        group_dict[stripped_label] = len(groups)
        groups.append([])
      groups[group_dict[stripped_label]].append(i)
    return groups

  def GetSummaryTableValues(self, table):
    # First get the groups
    groups = self.GetGroups(table)

    summary_table = []

    labels = table[0]

    summary_labels = ['Summary Table']
    for group in groups:
      label = labels[group[0]]
      stripped_label = self.GetStrippedLabel(label)
      group_label = '%s (%d runs)' % (stripped_label, len(group))
      summary_labels.append(group_label)
    summary_table.append(summary_labels)

    for i in range(1, len(table)):
      row = table[i]
      summary_row = [row[0]]
      for group in groups:
        group_runs = []
        for index in group:
          group_runs.append(row[index])
        group_run = self.AggregateResults(group_runs)
        summary_row.append(group_run)
      summary_table.append(summary_row)

    return summary_table

  # Drop N% slowest and M% fastest numbers, and return arithmean of
  # the remaining.
  @staticmethod
  def AverageWithDrops(numbers, slow_percent=20, fast_percent=20):
    sorted_numbers = list(numbers)
    sorted_numbers.sort()
    num_slow = int(slow_percent / 100.0 * len(sorted_numbers))
    num_fast = int(fast_percent / 100.0 * len(sorted_numbers))
    sorted_numbers = sorted_numbers[num_slow:]
    if num_fast:
      sorted_numbers = sorted_numbers[:-num_fast]
    return numpy.average(sorted_numbers)

  @staticmethod
  def AggregateResults(group_results):
    ret = ''
    if not group_results:
      return ret
    all_floats = True
    all_passes = True
    all_fails = True
    for group_result in group_results:
      if not IsFloat(group_result):
        all_floats = False
      if group_result != 'PASSED':
        all_passes = False
      if group_result != 'FAILED':
        all_fails = False
    if all_floats == True:
      float_results = [float(v) for v in group_results]
      ret = '%f' % TableFormatter.AverageWithDrops(float_results)
      # Add this line for standard deviation.
      ###      ret += " %f" % numpy.std(float_results)
    elif all_passes == True:
      ret = 'ALL_PASS'
    elif all_fails == True:
      ret = 'ALL_FAILS'
    return ret

  @staticmethod
  def GetStrippedLabel(label):
    return re.sub('\s*\S+:\S+\s*', '', label)
###    return re.sub("\s*remote:\S*\s*i:\d+$", "", label)

  @staticmethod
  def GetLabelWithIteration(label, iteration):
    return '%s i:%d' % (label, iteration)
