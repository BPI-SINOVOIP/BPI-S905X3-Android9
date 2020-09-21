#!/usr/bin/python2
#
# Copyright 2010 Google Inc. All Rights Reserved.
"""Script to rotate the weekly team sheriff.

This script determines who the next sheriff is, updates the file
appropriately and sends out email notifying the team.
"""

from __future__ import print_function

__author__ = 'asharif@google.com (Ahmad Sharif)'

import argparse
import datetime
import os
import sys

from cros_utils import constants
from cros_utils import email_sender


class SheriffHandler(object):
  """Main class for handling sheriff rotations."""

  SHERIFF_FILE = os.path.join(constants.CROSTC_WORKSPACE, 'sheriffs.txt')
  SUBJECT = 'You (%s) are the sheriff for the week: %s - %s'
  BODY = ('Please see instructions here: '
          'https://sites.google.com/a/google.com/chromeos-toolchain-team-home2'
          '/home/sheriff-s-corner/sheriff-duties')

  def GetWeekInfo(self, day=datetime.datetime.today()):
    """Return week_start, week_end."""

    epoch = datetime.datetime.utcfromtimestamp(0)
    delta_since_epoch = day - epoch

    abs_days = abs(delta_since_epoch.days) - 2  # To get it to start from Sat.
    day_of_week = abs_days % 7

    week_begin = day - datetime.timedelta(days=day_of_week)
    week_end = day + datetime.timedelta(days=(6 - day_of_week))

    strftime_format = '%A, %B %d %Y'

    return (week_begin.strftime(strftime_format),
            week_end.strftime(strftime_format))

  def GetCurrentSheriff(self):
    """Return the current sheriff."""
    return self.ReadSheriffsAsList()[0]

  def ReadSheriffsAsList(self):
    """Return the sheriff file contents."""
    contents = ''
    with open(self.SHERIFF_FILE, 'r') as f:
      contents = f.read()
    return contents.splitlines()

  def WriteSheriffsAsList(self, to_write):
    with open(self.SHERIFF_FILE, 'w') as f:
      f.write('\n'.join(to_write))

  def GetRotatedSheriffs(self, num_rotations=1):
    """Return the sheriff file contents."""
    sheriff_list = self.ReadSheriffsAsList()

    new_sheriff_list = []
    num_rotations = num_rotations % len(sheriff_list)
    new_sheriff_list = (
        sheriff_list[num_rotations:] + sheriff_list[:num_rotations])
    return new_sheriff_list

  def Email(self):
    es = email_sender.EmailSender()
    current_sheriff = self.GetCurrentSheriff()
    week_start, week_end = self.GetWeekInfo()
    subject = self.SUBJECT % (current_sheriff, week_start, week_end)
    es.SendEmail([current_sheriff],
                 subject,
                 self.BODY,
                 email_from=os.path.basename(__file__),
                 email_cc=['c-compiler-chrome'])


def Main(argv):
  parser = argparse.ArgumentParser()
  parser.add_argument('-e',
                      '--email',
                      dest='email',
                      action='store_true',
                      help='Email the sheriff.')
  parser.add_argument('-r',
                      '--rotate',
                      dest='rotate',
                      help='Print sheriffs after n rotations.')
  parser.add_argument('-w',
                      '--write',
                      dest='write',
                      action='store_true',
                      default=False,
                      help='Wrote rotated contents to the sheriff file.')

  options = parser.parse_args(argv)

  sheriff_handler = SheriffHandler()

  current_sheriff = sheriff_handler.GetCurrentSheriff()
  week_start, week_end = sheriff_handler.GetWeekInfo()

  print('Current sheriff: %s (%s - %s)' % (current_sheriff, week_start,
                                           week_end))

  if options.email:
    sheriff_handler.Email()

  if options.rotate:
    rotated_sheriffs = sheriff_handler.GetRotatedSheriffs(int(options.rotate))
    print('Rotated sheriffs (after %s rotations)' % options.rotate)
    print('\n'.join(rotated_sheriffs))
    if options.write:
      sheriff_handler.WriteSheriffsAsList(rotated_sheriffs)
      print('Rotated sheriffs written to file.')

  return 0


if __name__ == '__main__':
  retval = Main(sys.argv[1:])
  sys.exit(retval)
