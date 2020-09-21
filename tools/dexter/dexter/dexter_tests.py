
import re
import os
import glob
import sys
import subprocess
import argparse

# test command line arguments
argParser = argparse.ArgumentParser(description = 'dexter end-to-end test driver')
argParser.add_argument('-cmd', default = 'dexter')
argParser.add_argument('-root', help = 'Root location of the test data files')
argParser.add_argument('-update', action = 'store_true', help = 'Update the expected files')
args = argParser.parse_args()

# the bazel sandbox test data root
data_root = args.root or 'tools/dexter/testdata'

# update expected (golden) output?
if args.update:
  if args.root is None:
    print('ERROR: -update requires -root value')
    exit(1)
  print('\nUpdating expected output (test data root: %s)' % data_root)

# list of test cases
# ( <test_name> : { <test_case_config> } )
test_cases = {
  'map'             : { 'args' : '-m', 'input' : ['*.dex'] },
  'stats'           : { 'args' : '-s', 'input' : ['*.dex'] },
  'asm'             : { 'args' : '-d', 'input' : ['*.dex'] },
  'hello_stats'     : { 'args' : '-s -e Hello', 'input' : ['hello.dex'] },
  'am_stats'        : { 'args' : '-s -e android.app.ActivityManager', 'input' : ['large.dex'] },
  'rewrite'         : { 'args' : '-d -x full_rewrite', 'input' : ['*.dex'] },
  'entry_hook'      : { 'args' : '-d -x stress_entry_hook', 'input' : [
                          'entry_hooks.dex', 'hello.dex', 'medium.dex', 'min.dex' ] },
  'exit_hook'       : { 'args' : '-d -x stress_exit_hook', 'input' : [
                          'exit_hooks.dex', 'medium.dex', 'try_catch.dex' ] },
  'wrap_invoke'     : { 'args' : '-d -x stress_wrap_invoke', 'input' : [
                          'hello.dex', 'hello_nodebug.dex', 'medium.dex' ] },
  'mi'              : { 'args' : '-d -x test_method_instrumenter', 'input' : ['entry_hooks.dex'] },
  'find_method'     : { 'args' : '-x stress_find_method', 'input' : [
                          'hello.dex', 'entry_hooks.dex', 'medium.dex', 'large.dex', 'try_catch.dex' ] },
  'verbose_cfg'     : { 'args' : '-d --cfg=verbose', 'input' : ['*.dex'] },
  'compact_cfg'     : { 'args' : '-d --cfg=compact', 'input' : ['*.dex'] },
  'scratch_regs'    : { 'args' : '-d -x stress_scratch_regs', 'input' : ['*.dex'] },
  'regs_usage'      : { 'args' : '-x regs_histogram', 'input' : ['*.dex'] },
  'code_coverage'   : { 'args' : '-d -x code_coverage', 'input' : ['*.dex'] },
}

# run a shell command and returns the stdout content
def Run(cmd, stdin_content=None):
  return subprocess.Popen(
    args = cmd,
    shell = True,
    stdin = subprocess.PIPE,
    stdout = subprocess.PIPE,
    stderr = subprocess.STDOUT).communicate(input = stdin_content)[0]

tests = 0
failures = 0

# for each test_case, run dexter over the specified input (ex. *.dex)
#
# the expected ('golden') output has the same base name as the input .dex,
# for example (test_name = 'map') :
#
#    'hello.dex' -> 'expected/hello.map'
#
for test_name, test_config in sorted(test_cases.iteritems()):
  for input_pattern in test_config['input']:
    input_files = glob.glob(os.path.join(data_root, input_pattern))

    for input in input_files:
      tests = tests + 1

      # run dexter with the test arguments
      cmd = '%s %s %s' % (args.cmd, test_config['args'], input)
      actual_output = Run(cmd)

      # build the expected filename
      expected_filename = re.sub(r'\.dex', ('.%s' % test_name), os.path.basename(input))
      expected_filename = os.path.join(data_root, 'expected', expected_filename)

      if args.update:
        # update expected output file
        with open(expected_filename, "w") as f:
          f.write(actual_output)
      else:
        # compare the actual output with the expected output
        cmp_output = Run('diff "%s" -' % expected_filename, actual_output)
        if cmp_output:
          print('\nFAILED: expected output mismatch (%s)' % os.path.basename(expected_filename))
          print(cmp_output)
          failures = failures + 1
        else:
          print('ok: output matching (%s)' % os.path.basename(expected_filename))

if args.update:
  print('\nSUMMARY: updated expected output for %d tests\n' % tests)
else:
  print('\nSUMMARY: %d failure(s), %d test cases\n' % (failures, tests))

if failures != 0:
  exit(1)
