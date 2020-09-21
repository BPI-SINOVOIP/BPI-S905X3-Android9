# Copyright 2011 Google Inc. All Rights Reserved.
"""Summarize hottest basic blocks found while doing a ChromeOS FDO build.

Here is an example execution:

  summarize_hot_blocks.py
   --data_dir=~/chromeos/chroot/var/cache/chromeos-chrome/ --cutoff=10000
   --output_dir=/home/x/y

With the cutoff, it will ignore any basic blocks that have a count less
than what is specified (in this example 10000)
The script looks inside the directory (this is typically a directory where
the object files are generated) for files with *.profile and *.optimized
suffixes. To get these, the following flags were added to the compiler
invokation within vanilla_vs_fdo.py in the profile-use phase.

              "-fdump-tree-optimized-blocks-lineno "
              "-fdump-ipa-profile-blocks-lineno "

Here is an example of the *.profile and *.optimized files contents:

# BLOCK 7 freq:3901 count:60342, starting at line 92
# PRED: 6 [39.0%]  count:60342 (true,exec)
  [url_canon_internal.cc : 92:28] MEM[(const char * *)source_6(D) + 16B] =
  D.28080_17;
  [url_canon_internal.cc : 93:41] MEM[(struct Component *)parsed_4(D) + 16B] =
  MEM[(const struct Component &)repl_1(D) + 80];
# SUCC: 8 [100.0%]  count:60342 (fallthru,exec)
# BLOCK 8 freq:10000 count:154667, starting at line 321
# PRED: 7 [100.0%]  count:60342 (fallthru,exec) 6 [61.0%]  count:94325
(false,exec)
  [url_canon_internal.cc : 321:51] # DEBUG D#10 =>
  [googleurl/src/url_canon_internal.cc : 321] &parsed_4(D)->host

this script finds the blocks with highest count and shows the first line
of each block so that it is easy to identify the origin of the basic block.

"""

__author__ = 'llozano@google.com (Luis Lozano)'

import optparse
import os
import re
import shutil
import sys
import tempfile

from cros_utils import command_executer


# Given a line, check if it has a block count and return it.
# Return -1 if there is no match
def GetBlockCount(line):
  match_obj = re.match('.*# BLOCK \d+ .*count:(\d+)', line)
  if match_obj:
    return int(match_obj.group(1))
  else:
    return -1


class Collector(object):

  def __init__(self, data_dir, cutoff, output_dir, tempdir):
    self._data_dir = data_dir
    self._cutoff = cutoff
    self._output_dir = output_dir
    self._tempdir = tempdir
    self._ce = command_executer.GetCommandExecuter()

  def CollectFileList(self, file_exp, list_file):
    command = ("find %s -type f -name '%s' > %s" %
               (self._data_dir, file_exp,
                os.path.join(self._tempdir, list_file)))
    ret = self._ce.RunCommand(command)
    if ret:
      raise RuntimeError('Failed: %s' % command)

  def SummarizeLines(self, data_file):
    sum_lines = []
    search_lno = False
    for line in data_file:
      count = GetBlockCount(line)
      if count != -1:
        if count >= self._cutoff:
          search_lno = True
          sum_line = line.strip()
          sum_count = count
      # look for a line that starts with line number information
      elif search_lno and re.match('^\s*\[.*: \d*:\d*]', line):
        search_lno = False
        sum_lines.append('%d:%s: %s %s' %
                         (sum_count, data_file.name, sum_line, line))
    return sum_lines

  # Look for blocks in the data file that have a count larger than the cutoff
  # and generate a sorted summary file of the hottest blocks.
  def SummarizeFile(self, data_file, sum_file):
    with open(data_file, 'r') as f:
      sum_lines = self.SummarizeLines(f)

    # sort reverse the list in place by the block count number
    sum_lines.sort(key=GetBlockCount, reverse=True)

    with open(sum_file, 'w') as sf:
      sf.write(''.join(sum_lines))

    print 'Generated file Summary: ', sum_file

  # Find hottest blocks in the list of files, generate a sorted summary for
  # each file and then do a sorted merge of all the summaries.
  def SummarizeList(self, list_file, summary_file):
    with open(os.path.join(self._tempdir, list_file)) as f:
      sort_list = []
      for file_name in f:
        file_name = file_name.strip()
        sum_file = '%s.sum' % file_name
        sort_list.append('%s%s' % (sum_file, chr(0)))
        self.SummarizeFile(file_name, sum_file)

    tmp_list_file = os.path.join(self._tempdir, 'file_list.dat')
    with open(tmp_list_file, 'w') as file_list_file:
      for x in sort_list:
        file_list_file.write(x)

    merge_command = ('sort -nr -t: -k1 --merge --files0-from=%s > %s ' %
                     (tmp_list_file, summary_file))

    ret = self._ce.RunCommand(merge_command)
    if ret:
      raise RuntimeError('Failed: %s' % merge_command)
    print 'Generated general summary: ', summary_file

  def SummarizePreOptimized(self, summary_file):
    self.CollectFileList('*.profile', 'chrome.profile.list')
    self.SummarizeList('chrome.profile.list',
                       os.path.join(self._output_dir, summary_file))

  def SummarizeOptimized(self, summary_file):
    self.CollectFileList('*.optimized', 'chrome.optimized.list')
    self.SummarizeList('chrome.optimized.list',
                       os.path.join(self._output_dir, summary_file))


def Main(argv):
  command_executer.InitCommandExecuter()
  usage = ('usage: %prog --data_dir=<dir> --cutoff=<value> '
           '--output_dir=<dir> [--keep_tmp]')
  parser = optparse.OptionParser(usage=usage)
  parser.add_option('--data_dir',
                    dest='data_dir',
                    help=('directory where the FDO (*.profile and '
                          '*.optimized) files are located'))
  parser.add_option('--cutoff',
                    dest='cutoff',
                    help='Minimum count to consider for each basic block')
  parser.add_option('--output_dir',
                    dest='output_dir',
                    help=('directory where summary data will be generated'
                          '(pre_optimized.txt, optimized.txt)'))
  parser.add_option('--keep_tmp',
                    action='store_true',
                    dest='keep_tmp',
                    default=False,
                    help=('Keep directory with temporary files'
                          '(for debugging purposes)'))
  options = parser.parse_args(argv)[0]
  if not all((options.data_dir, options.cutoff, options.output_dir)):
    parser.print_help()
    sys.exit(1)

  tempdir = tempfile.mkdtemp()

  co = Collector(options.data_dir, int(options.cutoff), options.output_dir,
                 tempdir)
  co.SummarizePreOptimized('pre_optimized.txt')
  co.SummarizeOptimized('optimized.txt')

  if not options.keep_tmp:
    shutil.rmtree(tempdir, ignore_errors=True)

  return 0


if __name__ == '__main__':
  retval = Main(sys.argv)
  sys.exit(retval)
