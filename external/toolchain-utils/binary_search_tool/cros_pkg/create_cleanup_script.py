#!/usr/bin/env python2
#
#  Copyright 2015 Google Inc. All Rights Reserved
"""The script to generate a cleanup script after setup.sh.

This script takes a set of flags, telling it what setup.sh changed
during the set up process. Based on the values of the input flags, it
generates a cleanup script, named ${BOARD}_cleanup.sh, which will
undo the changes made by setup.sh, returning everything to its
original state.
"""

from __future__ import print_function

import argparse
import sys


def Usage(parser, msg):
  print('ERROR: ' + msg)
  parser.print_help()
  sys.exit(1)


def Main(argv):
  """Generate a script to undo changes done by setup.sh

    The script setup.sh makes a change that needs to be
    undone, namely it creates a soft link making /build/${board} point
    to /build/${board}.work.  To do this, it had to see if
    /build/${board} already existed, and if so, whether it was a real
    tree or a soft link.  If it was soft link, it saved the old value
    of the link, then deleted it and created the new link.  If it was
    a real tree, it renamed the tree to /build/${board}.save, and then
    created the new soft link.  If the /build/${board} did not
    previously exist, then it just created the new soft link.

    This function takes arguments that tell it exactly what setup.sh
    actually did, then generates a script to undo those exact changes.
  """

  parser = argparse.ArgumentParser()
  parser.add_argument(
      '--board',
      dest='board',
      required=True,
      help='Chromeos board for packages/image.')

  parser.add_argument(
      '--old_tree_missing',
      dest='tree_existed',
      action='store_false',
      help='Did /build/${BOARD} exist.',
      default=True)

  parser.add_argument(
      '--renamed_tree',
      dest='renamed_tree',
      action='store_true',
      help='Was /build/${BOARD} saved & renamed.',
      default=False)

  parser.add_argument(
      '--old_link',
      dest='old_link',
      help=('The original build tree soft link.'))

  options = parser.parse_args(argv[1:])

  if options.old_link or options.renamed_tree:
    if not options.tree_existed:
      Usage(parser, 'If --tree_existed is False, cannot have '
            '--renamed_tree or --old_link')

  if options.old_link and options.renamed_tree:
    Usage(parser, '--old_link and --renamed_tree are incompatible options.')

  if options.tree_existed:
    if not options.old_link and not options.renamed_tree:
      Usage(parser, 'If --tree_existed is True, then must have either '
            '--old_link or --renamed_tree')

  out_filename = 'cros_pkg/' + options.board + '_cleanup.sh'

  with open(out_filename, 'w') as out_file:
    out_file.write('#!/bin/bash\n\n')
    # First, remove the 'new' soft link.
    out_file.write('sudo rm /build/%s\n' % options.board)
    if options.tree_existed:
      if options.renamed_tree:
        # Old build tree existed and was a real tree, so it got
        # renamed.  Move the renamed tree back to the original tree.
        out_file.write('sudo mv /build/%s.save /build/%s\n' % (options.board,
                                                               options.board))
      else:
        # Old tree existed and was already a soft link.  Re-create the
        # original soft link.
        original_link = options.old_link
        if original_link[0] == "'":
          original_link = original_link[1:]
        if original_link[-1] == "'":
          original_link = original_link[:-1]
        out_file.write('sudo ln -s %s /build/%s\n' % (original_link,
                                                      options.board))
    out_file.write('\n')
    # Remove common.sh file
    out_file.write('rm common/common.sh\n')

  return 0


if __name__ == '__main__':
  retval = Main(sys.argv)
  sys.exit(retval)
