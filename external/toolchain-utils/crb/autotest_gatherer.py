from table_formatter import TableFormatter as TableFormatter


class AutotestGatherer(TableFormatter):

  def __init__(self):
    self.runs = []
    TableFormatter.__init__(self)

  def GetFormattedMainTable(self, percents_only, fit_string):
    ret = ''
    table = self.GetTableValues()
    ret += self.GetTableLabels(table)
    ret += self.GetFormattedTable(table,
                                  percents_only=percents_only,
                                  fit_string=fit_string)
    return ret

  def GetFormattedSummaryTable(self, percents_only, fit_string):
    ret = ''
    table = self.GetTableValues()
    summary_table = self.GetSummaryTableValues(table)
    ret += self.GetTableLabels(summary_table)
    ret += self.GetFormattedTable(summary_table,
                                  percents_only=percents_only,
                                  fit_string=fit_string)
    return ret

  def GetBenchmarksString(self):
    ret = 'Benchmarks (in order):'
    ret = '\n'.join(self.GetAllBenchmarks())
    return ret

  def GetAllBenchmarks(self):
    all_benchmarks = []
    for run in self.runs:
      for key in run.results.keys():
        if key not in all_benchmarks:
          all_benchmarks.append(key)
    all_benchmarks.sort()
    return all_benchmarks

  def GetTableValues(self):
    table = []
    row = []

    row.append('Benchmark')
    for i in range(len(self.runs)):
      run = self.runs[i]
      label = run.GetLabel()
      label = self.GetLabelWithIteration(label, run.iteration)
      row.append(label)
    table.append(row)

    all_benchmarks = self.GetAllBenchmarks()
    for benchmark in all_benchmarks:
      row = []
      row.append(benchmark)
      for run in self.runs:
        results = run.results
        if benchmark in results:
          row.append(results[benchmark])
        else:
          row.append('')
      table.append(row)

    return table
