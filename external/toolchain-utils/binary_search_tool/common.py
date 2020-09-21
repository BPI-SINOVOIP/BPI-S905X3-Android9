"""Common config and logic for binary search tool

This module serves two main purposes:
  1. Programatically include the utils module in PYTHONPATH
  2. Create the argument parsing shared between binary_search_state.py and
     bisect.py

The argument parsing is handled by populating _ArgsDict with all arguments.
_ArgsDict is required so that binary_search_state.py and bisect.py can share
the argument parsing, but treat them slightly differently. For example,
bisect.py requires that all argument defaults are suppressed so that overriding
can occur properly (i.e. only options that are explicitly entered by the user
end up in the resultant options dictionary).

ArgumentDict inherits OrderedDict in order to preserve the order the args are
created so the help text is made properly.
"""

from __future__ import print_function

import collections
import os
import sys

# Programatically adding utils python path to PYTHONPATH
if os.path.isabs(sys.argv[0]):
  utils_pythonpath = os.path.abspath('{0}/..'.format(
      os.path.dirname(sys.argv[0])))
else:
  wdir = os.getcwd()
  utils_pythonpath = os.path.abspath('{0}/{1}/..'.format(wdir, os.path.dirname(
      sys.argv[0])))
sys.path.append(utils_pythonpath)


class ArgumentDict(collections.OrderedDict):
  """Wrapper around OrderedDict, represents CLI arguments for program.

  AddArgument enforces the following layout:
  {
      ['-n', '--iterations'] : {
          'dest': 'iterations',
          'type': int,
          'help': 'Number of iterations to try in the search.',
          'default': 50
      }
      [arg_name1, arg_name2, ...] : {
          arg_option1 : arg_option_val1,
          ...
      },
      ...
  }
  """
  _POSSIBLE_OPTIONS = ['action', 'nargs', 'const', 'default', 'type', 'choices',
                       'required', 'help', 'metavar', 'dest']

  def AddArgument(self, *args, **kwargs):
    """Add argument to ArgsDict, has same signature as argparse.add_argument

    Emulates the the argparse.add_argument method so the internal OrderedDict
    can be safely and easily populated. Each call to this method will have a 1-1
    corresponding call to argparse.add_argument once BuildArgParser is called.

    Args:
      *args: The names for the argument (-V, --verbose, etc.)
      **kwargs: The options for the argument, corresponds to the args of
                argparse.add_argument

    Returns:
      None

    Raises:
      TypeError: if args is empty or if option in kwargs is not a valid
                 option for argparse.add_argument.
    """
    if len(args) == 0:
      raise TypeError('Argument needs at least one name')

    for key in kwargs:
      if key not in self._POSSIBLE_OPTIONS:
        raise TypeError('Invalid option "%s" for argument %s' % (key, args[0]))

    self[args] = kwargs


_ArgsDict = ArgumentDict()


def GetArgsDict():
  """_ArgsDict singleton method"""
  if not _ArgsDict:
    _BuildArgsDict(_ArgsDict)
  return _ArgsDict


def BuildArgParser(parser, override=False):
  """Add all arguments from singleton ArgsDict to parser.

  Will take argparse parser and add all arguments in ArgsDict. Will ignore
  the default and required options if override is set to True.

  Args:
    parser: type argparse.ArgumentParser, will call add_argument for every item
            in _ArgsDict
    override: True if being called from bisect.py. Used to say that default and
              required options are to be ignored

  Returns:
    None
  """
  ArgsDict = GetArgsDict()

  # Have no defaults when overriding
  for arg_names, arg_options in ArgsDict.iteritems():
    if override:
      arg_options = arg_options.copy()
      arg_options.pop('default', None)
      arg_options.pop('required', None)

    parser.add_argument(*arg_names, **arg_options)


def StrToBool(str_in):
  if str_in.lower() in ['true', 't', '1']:
    return True
  if str_in.lower() in ['false', 'f', '0']:
    return False

  raise AttributeError('%s is not a valid boolean string' % str_in)


def _BuildArgsDict(args):
  """Populate ArgumentDict with all arguments"""
  args.AddArgument(
      '-n',
      '--iterations',
      dest='iterations',
      type=int,
      help='Number of iterations to try in the search.',
      default=50)
  args.AddArgument(
      '-i',
      '--get_initial_items',
      dest='get_initial_items',
      help=('Script to run to get the initial objects. '
            'If your script requires user input '
            'the --verbose option must be used'))
  args.AddArgument(
      '-g',
      '--switch_to_good',
      dest='switch_to_good',
      help=('Script to run to switch to good. '
            'If your switch script requires user input '
            'the --verbose option must be used'))
  args.AddArgument(
      '-b',
      '--switch_to_bad',
      dest='switch_to_bad',
      help=('Script to run to switch to bad. '
            'If your switch script requires user input '
            'the --verbose option must be used'))
  args.AddArgument(
      '-I',
      '--test_setup_script',
      dest='test_setup_script',
      help=('Optional script to perform building, flashing, '
            'and other setup before the test script runs.'))
  args.AddArgument(
      '-t',
      '--test_script',
      dest='test_script',
      help=('Script to run to test the '
            'output after packages are built.'))
  # No input (evals to False),
  # --prune (evals to True),
  # --prune=False,
  # --prune=True
  args.AddArgument(
      '-p',
      '--prune',
      dest='prune',
      nargs='?',
      const=True,
      default=False,
      type=StrToBool,
      metavar='bool',
      help=('If True, continue until all bad items are found. '
            'Defaults to False.'))
  # No input (evals to False),
  # --noincremental (evals to True),
  # --noincremental=False,
  # --noincremental=True
  args.AddArgument(
      '-c',
      '--noincremental',
      dest='noincremental',
      nargs='?',
      const=True,
      default=False,
      type=StrToBool,
      metavar='bool',
      help=('If True, don\'t propagate good/bad changes '
            'incrementally. Defaults to False.'))
  # No input (evals to False),
  # --file_args (evals to True),
  # --file_args=False,
  # --file_args=True
  args.AddArgument(
      '-f',
      '--file_args',
      dest='file_args',
      nargs='?',
      const=True,
      default=False,
      type=StrToBool,
      metavar='bool',
      help=('Whether to use a file to pass arguments to scripts. '
            'Defaults to False.'))
  # No input (evals to True),
  # --verify (evals to True),
  # --verify=False,
  # --verify=True
  args.AddArgument(
      '--verify',
      dest='verify',
      nargs='?',
      const=True,
      default=True,
      type=StrToBool,
      metavar='bool',
      help=('Whether to run verify iterations before searching. '
            'Defaults to True.'))
  args.AddArgument(
      '-N',
      '--prune_iterations',
      dest='prune_iterations',
      type=int,
      help='Number of prune iterations to try in the search.',
      default=100)
  # No input (evals to False),
  # --verbose (evals to True),
  # --verbose=False,
  # --verbose=True
  args.AddArgument(
      '-V',
      '--verbose',
      dest='verbose',
      nargs='?',
      const=True,
      default=False,
      type=StrToBool,
      metavar='bool',
      help='If True, print full output to console.')
  args.AddArgument(
      '-r',
      '--resume',
      dest='resume',
      action='store_true',
      help=('Resume bisection tool execution from state file.'
            'Useful if the last bisection was terminated '
            'before it could properly finish.'))
