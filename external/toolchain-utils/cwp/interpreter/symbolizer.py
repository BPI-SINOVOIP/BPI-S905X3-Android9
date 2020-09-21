# Copyright 2012 Google Inc. All Rights Reserved.
"""A script that symbolizes perf.data files."""
import optparse
import os
import shutil
from subprocess import call
from subprocess import PIPE
from subprocess import Popen
from cros_utils import misc

GSUTIL_CMD = 'gsutil cp gs://chromeos-image-archive/%s-release/%s/debug.tgz %s'
TAR_CMD = 'tar -zxvf %s -C %s'
PERF_BINARY = '/google/data/ro/projects/perf/perf'
VMLINUX_FLAG = ' --vmlinux=/usr/lib/debug/boot/vmlinux'
PERF_CMD = PERF_BINARY + ' report -i %s -n --symfs=%s' + VMLINUX_FLAG


def main():
  parser = optparse.OptionParser()
  parser.add_option('--in', dest='in_dir')
  parser.add_option('--out', dest='out_dir')
  parser.add_option('--cache', dest='cache')
  (opts, _) = parser.parse_args()
  if not _ValidateOpts(opts):
    return 1
  else:
    for filename in os.listdir(opts.in_dir):
      try:
        _DownloadSymbols(filename, opts.cache)
        _PerfReport(filename, opts.in_dir, opts.out_dir, opts.cache)
      except:
        print 'Exception caught. Continuing...'
  return 0


def _ValidateOpts(opts):
  """Ensures all directories exist, before attempting to populate."""
  if not os.path.exists(opts.in_dir):
    print "Input directory doesn't exist."
    return False
  if not os.path.exists(opts.out_dir):
    print "Output directory doesn't exist. Creating it..."
    os.makedirs(opts.out_dir)
  if not os.path.exists(opts.cache):
    print "Cache directory doesn't exist."
    return False
  return True


def _ParseFilename(filename, canonical=False):
  """Returns a tuple (key, time, board, lsb_version).
     If canonical is True, instead returns (database_key, board, canonical_vers)
     canonical_vers includes the revision string.
  """
  key, time, board, vers = filename.split('~')
  if canonical:
    vers = misc.GetChromeOSVersionFromLSBVersion(vers)
  return (key, time, board, vers)


def _FormReleaseDir(board, version):
  return '%s-release~%s' % (board, version)


def _DownloadSymbols(filename, cache):
  """ Incrementally downloads appropriate symbols.
      We store the downloads in cache, with each set of symbols in a TLD
      named like cache/$board-release~$canonical_vers/usr/lib/debug
  """
  _, _, board, vers = _ParseFilename(filename, canonical=True)
  tmp_suffix = '.tmp'

  tarball_subdir = _FormReleaseDir(board, vers)
  tarball_dir = os.path.join(cache, tarball_subdir)
  tarball_path = os.path.join(tarball_dir, 'debug.tgz')

  symbol_subdir = os.path.join('usr', 'lib')
  symbol_dir = os.path.join(tarball_dir, symbol_subdir)

  if os.path.isdir(symbol_dir):
    print 'Symbol directory %s exists, skipping download.' % symbol_dir
    return
  else:
    # First download using gsutil.
    if not os.path.isfile(tarball_path):
      download_cmd = GSUTIL_CMD % (board, vers, tarball_path + tmp_suffix)
      print 'Downloading symbols for %s' % filename
      print download_cmd
      ret = call(download_cmd.split())
      if ret != 0:
        print 'gsutil returned non-zero error code: %s.' % ret
        # Clean up the empty directory structures.
        os.remove(tarball_path + tmp_suffix)
        raise IOError

      shutil.move(tarball_path + tmp_suffix, tarball_path)

    # Next, untar the tarball.
    os.makedirs(symbol_dir + tmp_suffix)
    extract_cmd = TAR_CMD % (tarball_path, symbol_dir + tmp_suffix)
    print 'Extracting symbols for %s' % filename
    print extract_cmd
    ret = call(extract_cmd.split())
    if ret != 0:
      print 'tar returned non-zero code: %s.' % ret
      raise IOError
    shutil.move(symbol_dir + tmp_suffix, symbol_dir)
    os.remove(tarball_path)


def _PerfReport(filename, in_dir, out_dir, cache):
  """ Call perf report on the file, storing output to the output dir.
      The output is currently stored as $out_dir/$filename
  """
  _, _, board, vers = _ParseFilename(filename, canonical=True)
  symbol_cache_tld = _FormReleaseDir(board, vers)
  input_file = os.path.join(in_dir, filename)
  symfs = os.path.join(cache, symbol_cache_tld)
  report_cmd = PERF_CMD % (input_file, symfs)
  print 'Reporting.'
  print report_cmd
  report_proc = Popen(report_cmd.split(), stdout=PIPE)
  outfile = open(os.path.join(out_dir, filename), 'w')
  outfile.write(report_proc.stdout.read())
  outfile.close()


if __name__ == '__main__':
  exit(main())
