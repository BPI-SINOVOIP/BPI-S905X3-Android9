#!/usr/bin/python
#
# Copyright 2010 Google Inc. All Rights Reserved.

import datetime
import optparse
import os
import smtplib
import sys
import time
from email.mime.text import MIMEText

from autotest_gatherer import AutotestGatherer as AutotestGatherer
from autotest_run import AutotestRun as AutotestRun
from machine_manager_singleton import MachineManagerSingleton as MachineManagerSingleton
from cros_utils import logger
from cros_utils.file_utils import FileUtils


def CanonicalizeChromeOSRoot(chromeos_root):
  chromeos_root = os.path.expanduser(chromeos_root)
  if os.path.isfile(os.path.join(chromeos_root, 'src/scripts/enter_chroot.sh')):
    return chromeos_root
  else:
    return None


class Autotest(object):

  def __init__(self, autotest_string):
    self.name = None
    self.iterations = None
    self.args = None
    fields = autotest_string.split(',', 1)
    self.name = fields[0]
    if len(fields) > 1:
      autotest_string = fields[1]
      fields = autotest_string.split(',', 1)
    else:
      return
    self.iterations = int(fields[0])
    if len(fields) > 1:
      self.args = fields[1]
    else:
      return

  def __str__(self):
    return '\n'.join([self.name, self.iterations, self.args])


def CreateAutotestListFromString(autotest_strings, default_iterations=None):
  autotest_list = []
  for autotest_string in autotest_strings.split(':'):
    autotest = Autotest(autotest_string)
    if default_iterations and not autotest.iterations:
      autotest.iterations = default_iterations

    autotest_list.append(autotest)
  return autotest_list


def CreateAutotestRuns(images,
                       autotests,
                       remote,
                       board,
                       exact_remote,
                       rerun,
                       rerun_if_failed,
                       main_chromeos_root=None):
  autotest_runs = []
  for image in images:
    logger.GetLogger().LogOutput('Computing md5sum of: %s' % image)
    image_checksum = FileUtils().Md5File(image)
    logger.GetLogger().LogOutput('md5sum %s: %s' % (image, image_checksum))
    ###    image_checksum = "abcdefghi"

    chromeos_root = main_chromeos_root
    if not main_chromeos_root:
      image_chromeos_root = os.path.join(
          os.path.dirname(image), '../../../../..')
      chromeos_root = CanonicalizeChromeOSRoot(image_chromeos_root)
      assert chromeos_root, 'chromeos_root: %s invalid' % image_chromeos_root
    else:
      chromeos_root = CanonicalizeChromeOSRoot(main_chromeos_root)
      assert chromeos_root, 'chromeos_root: %s invalid' % main_chromeos_root

    # We just need a single ChromeOS root in the MachineManagerSingleton. It is
    # needed because we can save re-image time by checking the image checksum at
    # the beginning and assigning autotests to machines appropriately.
    if not MachineManagerSingleton().chromeos_root:
      MachineManagerSingleton().chromeos_root = chromeos_root

    for autotest in autotests:
      for iteration in range(autotest.iterations):
        autotest_run = AutotestRun(autotest,
                                   chromeos_root=chromeos_root,
                                   chromeos_image=image,
                                   board=board,
                                   remote=remote,
                                   iteration=iteration,
                                   image_checksum=image_checksum,
                                   exact_remote=exact_remote,
                                   rerun=rerun,
                                   rerun_if_failed=rerun_if_failed)
        autotest_runs.append(autotest_run)
  return autotest_runs


def GetNamesAndIterations(autotest_runs):
  strings = []
  for autotest_run in autotest_runs:
    strings.append('%s:%s' % (autotest_run.autotest.name,
                              autotest_run.iteration))
  return ' %s (%s)' % (len(strings), ' '.join(strings))


def GetStatusString(autotest_runs):
  status_bins = {}
  for autotest_run in autotest_runs:
    if autotest_run.status not in status_bins:
      status_bins[autotest_run.status] = []
    status_bins[autotest_run.status].append(autotest_run)

  status_strings = []
  for key, val in status_bins.items():
    status_strings.append('%s: %s' % (key, GetNamesAndIterations(val)))
  return 'Thread Status:\n%s' % '\n'.join(status_strings)


def GetProgressBar(num_done, num_total):
  ret = 'Done: %s%%' % int(100.0 * num_done / num_total)
  bar_length = 50
  done_char = '>'
  undone_char = ' '
  num_done_chars = bar_length * num_done / num_total
  num_undone_chars = bar_length - num_done_chars
  ret += ' [%s%s]' % (num_done_chars * done_char,
                      num_undone_chars * undone_char)
  return ret


def GetProgressString(start_time, num_remain, num_total):
  current_time = time.time()
  elapsed_time = current_time - start_time
  try:
    eta_seconds = float(num_remain) * elapsed_time / (num_total - num_remain)
    eta_seconds = int(eta_seconds)
    eta = datetime.timedelta(seconds=eta_seconds)
  except ZeroDivisionError:
    eta = 'Unknown'
  strings = []
  strings.append('Current time: %s Elapsed: %s ETA: %s' %
                 (datetime.datetime.now(),
                  datetime.timedelta(seconds=int(elapsed_time)), eta))
  strings.append(GetProgressBar(num_total - num_remain, num_total))
  return '\n'.join(strings)


def RunAutotestRunsInParallel(autotest_runs):
  start_time = time.time()
  active_threads = []
  for autotest_run in autotest_runs:
    # Set threads to daemon so program exits when ctrl-c is pressed.
    autotest_run.daemon = True
    autotest_run.start()
    active_threads.append(autotest_run)

  print_interval = 30
  last_printed_time = time.time()
  while active_threads:
    try:
      active_threads = [t for t in active_threads
                        if t is not None and t.isAlive()]
      for t in active_threads:
        t.join(1)
      if time.time() - last_printed_time > print_interval:
        border = '=============================='
        logger.GetLogger().LogOutput(border)
        logger.GetLogger().LogOutput(GetProgressString(start_time, len(
            [t for t in autotest_runs if t.status not in ['SUCCEEDED', 'FAILED']
            ]), len(autotest_runs)))
        logger.GetLogger().LogOutput(GetStatusString(autotest_runs))
        logger.GetLogger().LogOutput('%s\n' %
                                     MachineManagerSingleton().AsString())
        logger.GetLogger().LogOutput(border)
        last_printed_time = time.time()
    except KeyboardInterrupt:
      print 'C-c received... cleaning up threads.'
      for t in active_threads:
        t.terminate = True
      return 1
  return 0


def RunAutotestRunsSerially(autotest_runs):
  for autotest_run in autotest_runs:
    retval = autotest_run.Run()
    if retval:
      return retval


def ProduceTables(autotest_runs, full_table, fit_string):
  l = logger.GetLogger()
  ags_dict = {}
  for autotest_run in autotest_runs:
    name = autotest_run.full_name
    if name not in ags_dict:
      ags_dict[name] = AutotestGatherer()
    ags_dict[name].runs.append(autotest_run)
    output = ''
  for b, ag in ags_dict.items():
    output += 'Benchmark: %s\n' % b
    output += ag.GetFormattedMainTable(percents_only=not full_table,
                                       fit_string=fit_string)
    output += '\n'

  summary = ''
  for b, ag in ags_dict.items():
    summary += 'Benchmark Summary Table: %s\n' % b
    summary += ag.GetFormattedSummaryTable(percents_only=not full_table,
                                           fit_string=fit_string)
    summary += '\n'

  output += summary
  output += ('Number of re-images performed: %s' %
             MachineManagerSingleton().num_reimages)
  l.LogOutput(output)

  if autotest_runs:
    board = autotest_runs[0].board
  else:
    board = ''

  subject = '%s: %s' % (board, ', '.join(ags_dict.keys()))

  if any(autotest_run.run_completed for autotest_run in autotest_runs):
    SendEmailToUser(subject, summary)


def SendEmailToUser(subject, text_to_send):
  # Email summary to the current user.
  msg = MIMEText(text_to_send)

  # me == the sender's email address
  # you == the recipient's email address
  me = os.path.basename(__file__)
  you = os.getlogin()
  msg['Subject'] = '[%s] %s' % (os.path.basename(__file__), subject)
  msg['From'] = me
  msg['To'] = you

  # Send the message via our own SMTP server, but don't include the
  # envelope header.
  s = smtplib.SMTP('localhost')
  s.sendmail(me, [you], msg.as_string())
  s.quit()


def Main(argv):
  """The main function."""
  # Common initializations
  ###  command_executer.InitCommandExecuter(True)
  l = logger.GetLogger()

  parser = optparse.OptionParser()
  parser.add_option('-t',
                    '--tests',
                    dest='tests',
                    help=('Tests to compare.'
                          'Optionally specify per-test iterations by:'
                          '<test>,<iter>:<args>'))
  parser.add_option('-c',
                    '--chromeos_root',
                    dest='chromeos_root',
                    help='A *single* chromeos_root where scripts can be found.')
  parser.add_option('-n',
                    '--iterations',
                    dest='iterations',
                    help='Iterations to run per benchmark.',
                    default=1)
  parser.add_option('-r',
                    '--remote',
                    dest='remote',
                    help='The remote chromeos machine.')
  parser.add_option('-b', '--board', dest='board', help='The remote board.')
  parser.add_option('--full_table',
                    dest='full_table',
                    help='Print full tables.',
                    action='store_true',
                    default=True)
  parser.add_option('--exact_remote',
                    dest='exact_remote',
                    help='Run tests on the exact remote.',
                    action='store_true',
                    default=False)
  parser.add_option('--fit_string',
                    dest='fit_string',
                    help='Fit strings to fixed sizes.',
                    action='store_true',
                    default=False)
  parser.add_option('--rerun',
                    dest='rerun',
                    help='Re-run regardless of cache hit.',
                    action='store_true',
                    default=False)
  parser.add_option('--rerun_if_failed',
                    dest='rerun_if_failed',
                    help='Re-run if previous run was a failure.',
                    action='store_true',
                    default=False)
  parser.add_option('--no_lock',
                    dest='no_lock',
                    help='Do not lock the machine before running the tests.',
                    action='store_true',
                    default=False)
  l.LogOutput(' '.join(argv))
  [options, args] = parser.parse_args(argv)

  if options.remote is None:
    l.LogError('No remote machine specified.')
    parser.print_help()
    return 1

  if not options.board:
    l.LogError('No board specified.')
    parser.print_help()
    return 1

  remote = options.remote
  tests = options.tests
  board = options.board
  exact_remote = options.exact_remote
  iterations = int(options.iterations)

  autotests = CreateAutotestListFromString(tests, iterations)

  main_chromeos_root = options.chromeos_root
  images = args[1:]
  fit_string = options.fit_string
  full_table = options.full_table
  rerun = options.rerun
  rerun_if_failed = options.rerun_if_failed

  MachineManagerSingleton().no_lock = options.no_lock

  # Now try creating all the Autotests
  autotest_runs = CreateAutotestRuns(images, autotests, remote, board,
                                     exact_remote, rerun, rerun_if_failed,
                                     main_chromeos_root)

  try:
    # At this point we have all the autotest runs.
    for machine in remote.split(','):
      MachineManagerSingleton().AddMachine(machine)

    retval = RunAutotestRunsInParallel(autotest_runs)
    if retval:
      return retval

    # Now print tables
    ProduceTables(autotest_runs, full_table, fit_string)
  finally:
    # not sure why this isn't called at the end normally...
    MachineManagerSingleton().__del__()

  return 0


if __name__ == '__main__':
  sys.exit(Main(sys.argv))
