#!/usr/bin/env python

import optparse
import os
import shutil
import subprocess
import sys

upstream_git = 'https://github.com/catapult-project/catapult.git'
PACKAGE_DIRS = [
    'common',
    'dependency_manager',
    'devil',
    'systrace',
    'third_party/pyserial',
    'third_party/zipfile',
    'tracing/tracing/trace_data',
]
PACKAGE_FILES = [
    'tracing/tracing/__init__.py',
    'tracing/tracing_project.py',
]
IGNORE_PATTERNS = ['OWNERS'] # doesn't make sense to sync owners files

script_dir = os.path.dirname(os.path.abspath(sys.argv[0]))
catapult_src_dir = os.path.join(script_dir, 'catapult-upstream')
catapult_dst_dir = os.path.join(script_dir, 'catapult')

parser = optparse.OptionParser()
parser.add_option('--local', dest='local_dir', metavar='DIR',
                  help='use a local catapult')
parser.add_option('--no-min', dest='no_min', default=False, action='store_true',
                  help='skip minification')
options, args = parser.parse_args()

## Update the source if needed.
if options.local_dir is None:
  # Remove the old source tree.
  shutil.rmtree(catapult_src_dir, True)

  # Pull the latest source from the upstream git.
  git_args = ['git', 'clone', upstream_git, catapult_src_dir]
  p = subprocess.Popen(git_args, stdout=subprocess.PIPE, cwd=script_dir)
  p.communicate()
  if p.wait() != 0:
    print 'Failed to checkout source from upstream git.'
    sys.exit(1)

  catapult_git_dir = os.path.join(catapult_src_dir, '.git')
  # Update the UPSTREAM_REVISION file
  git_args = ['git', 'rev-parse', 'HEAD']
  p = subprocess.Popen(git_args,
                       stdout=subprocess.PIPE,
                       cwd=catapult_src_dir,
                       env={"GIT_DIR":catapult_git_dir})
  out, err = p.communicate()
  if p.wait() != 0:
    print 'Failed to get revision.'
    sys.exit(1)

  shutil.rmtree(catapult_git_dir, True)

  rev = out.strip()
  with open('UPSTREAM_REVISION', 'wt') as f:
    f.write(rev + '\n')
else:
  catapult_src_dir = options.local_dir


## Update systrace_trace_viewer.html
systrace_dir = os.path.join(catapult_src_dir, 'systrace', 'systrace')
sys.path.append(systrace_dir)
import update_systrace_trace_viewer
update_systrace_trace_viewer.update(no_auto_update=True, no_min=options.no_min)

## Package the result
shutil.rmtree(catapult_dst_dir)

for d in PACKAGE_DIRS:
  src = os.path.join(catapult_src_dir, d)
  dst = os.path.join(catapult_dst_dir, d)

  # make parent dir by creating dst + ancestors, and deleting dst
  if not os.path.isdir(dst):
    os.makedirs(dst)
  shutil.rmtree(dst)

  # copy tree
  shutil.copytree(src, dst, ignore=shutil.ignore_patterns(*IGNORE_PATTERNS))

for f in PACKAGE_FILES:
  src = os.path.join(catapult_src_dir, f)
  dst = os.path.join(catapult_dst_dir, f)
  shutil.copy(src, dst)
