import datetime
import getpass
import glob
import os
import pickle
import re
import threading
import time
import image_chromeos
import machine_manager_singleton
import table_formatter
from cros_utils import command_executer
from cros_utils import logger

SCRATCH_DIR = '/home/%s/cros_scratch' % getpass.getuser()
PICKLE_FILE = 'pickle.txt'
VERSION = '1'


def ConvertToFilename(text):
  ret = text
  ret = re.sub('/', '__', ret)
  ret = re.sub(' ', '_', ret)
  ret = re.sub('=', '', ret)
  ret = re.sub("\"", '', ret)
  return ret


class AutotestRun(threading.Thread):

  def __init__(self,
               autotest,
               chromeos_root='',
               chromeos_image='',
               board='',
               remote='',
               iteration=0,
               image_checksum='',
               exact_remote=False,
               rerun=False,
               rerun_if_failed=False):
    self.autotest = autotest
    self.chromeos_root = chromeos_root
    self.chromeos_image = chromeos_image
    self.board = board
    self.remote = remote
    self.iteration = iteration
    l = logger.GetLogger()
    l.LogFatalIf(not image_checksum, "Checksum shouldn't be None")
    self.image_checksum = image_checksum
    self.results = {}
    threading.Thread.__init__(self)
    self.terminate = False
    self.retval = None
    self.status = 'PENDING'
    self.run_completed = False
    self.exact_remote = exact_remote
    self.rerun = rerun
    self.rerun_if_failed = rerun_if_failed
    self.results_dir = None
    self.full_name = None

  @staticmethod
  def MeanExcludingSlowest(array):
    mean = sum(array) / len(array)
    array2 = []

    for v in array:
      if mean != 0 and abs(v - mean) / mean < 0.2:
        array2.append(v)

    if array2:
      return sum(array2) / len(array2)
    else:
      return mean

  @staticmethod
  def AddComposite(results_dict):
    composite_keys = []
    composite_dict = {}
    for key in results_dict:
      mo = re.match('(.*){\d+}', key)
      if mo:
        composite_keys.append(mo.group(1))
    for key in results_dict:
      for composite_key in composite_keys:
        if (key.count(composite_key) != 0 and
            table_formatter.IsFloat(results_dict[key])):
          if composite_key not in composite_dict:
            composite_dict[composite_key] = []
          composite_dict[composite_key].append(float(results_dict[key]))
          break

    for composite_key in composite_dict:
      v = composite_dict[composite_key]
      results_dict['%s[c]' % composite_key] = sum(v) / len(v)
      mean_excluding_slowest = AutotestRun.MeanExcludingSlowest(v)
      results_dict['%s[ce]' % composite_key] = mean_excluding_slowest

    return results_dict

  def ParseOutput(self):
    p = re.compile('^-+.*?^-+', re.DOTALL | re.MULTILINE)
    matches = p.findall(self.out)
    for i in range(len(matches)):
      results = matches[i]
      results_dict = {}
      for line in results.splitlines()[1:-1]:
        mo = re.match('(.*\S)\s+\[\s+(PASSED|FAILED)\s+\]', line)
        if mo:
          results_dict[mo.group(1)] = mo.group(2)
          continue
        mo = re.match('(.*\S)\s+(.*)', line)
        if mo:
          results_dict[mo.group(1)] = mo.group(2)

      # Add a composite keyval for tests like startup.
      results_dict = AutotestRun.AddComposite(results_dict)

      self.results = results_dict

      # This causes it to not parse the table again
      # Autotest recently added a secondary table
      # That reports errors and screws up the final pretty output.
      break
    mo = re.search('Results placed in (\S+)', self.out)
    if mo:
      self.results_dir = mo.group(1)
      self.full_name = os.path.basename(self.results_dir)

  def GetCacheHashBase(self):
    ret = ('%s %s %s' %
           (self.image_checksum, self.autotest.name, self.iteration))
    if self.autotest.args:
      ret += ' %s' % self.autotest.args
    ret += '-%s' % VERSION
    return ret

  def GetLabel(self):
    ret = '%s %s remote:%s' % (self.chromeos_image, self.autotest.name,
                               self.remote)
    return ret

  def TryToLoadFromCache(self):
    base = self.GetCacheHashBase()
    if self.exact_remote:
      if not self.remote:
        return False
      cache_dir_glob = '%s_%s' % (ConvertToFilename(base), self.remote)
    else:
      cache_dir_glob = '%s*' % ConvertToFilename(base)
    cache_path_glob = os.path.join(SCRATCH_DIR, cache_dir_glob)
    matching_dirs = glob.glob(cache_path_glob)
    if matching_dirs:
      matching_dir = matching_dirs[0]
      cache_file = os.path.join(matching_dir, PICKLE_FILE)
      assert os.path.isfile(cache_file)
      self._logger.LogOutput('Trying to read from cache file: %s' % cache_file)
      return self.ReadFromCache(cache_file)
    self._logger.LogOutput('Cache miss. AM going to run: %s for: %s' %
                           (self.autotest.name, self.chromeos_image))
    return False

  def ReadFromCache(self, cache_file):
    with open(cache_file, 'rb') as f:
      self.retval = pickle.load(f)
      self.out = pickle.load(f)
      self.err = pickle.load(f)
      self._logger.LogOutput(self.out)
      return True
    return False

  def StoreToCache(self):
    base = self.GetCacheHashBase()
    self.cache_dir = os.path.join(SCRATCH_DIR,
                                  '%s_%s' % (ConvertToFilename(base),
                                             self.remote))
    cache_file = os.path.join(self.cache_dir, PICKLE_FILE)
    command = 'mkdir -p %s' % os.path.dirname(cache_file)
    ret = self._ce.RunCommand(command)
    assert ret == 0, "Couldn't create cache dir"
    with open(cache_file, 'wb') as f:
      pickle.dump(self.retval, f)
      pickle.dump(self.out, f)
      pickle.dump(self.err, f)

  def run(self):
    self._logger = logger.Logger(
        os.path.dirname(__file__), '%s.%s' % (os.path.basename(__file__),
                                              self.name), True)
    self._ce = command_executer.GetCommandExecuter(self._logger)
    self.RunCached()

  def RunCached(self):
    self.status = 'WAITING'
    cache_hit = False
    if not self.rerun:
      cache_hit = self.TryToLoadFromCache()
    else:
      self._logger.LogOutput('--rerun passed. Not using cached results.')
    if self.rerun_if_failed and self.retval:
      self._logger.LogOutput('--rerun_if_failed passed and existing test '
                             'failed. Rerunning...')
      cache_hit = False
    if not cache_hit:
      # Get machine
      while True:
        if self.terminate:
          return 1
        self.machine = (machine_manager_singleton.MachineManagerSingleton(
        ).AcquireMachine(self.image_checksum))
        if self.machine:
          self._logger.LogOutput('%s: Machine %s acquired at %s' %
                                 (self.name, self.machine.name,
                                  datetime.datetime.now()))
          break
        else:
          sleep_duration = 10
          time.sleep(sleep_duration)
      try:
        self.remote = self.machine.name

        if self.machine.checksum != self.image_checksum:
          self.retval = self.ImageTo(self.machine.name)
          if self.retval:
            return self.retval
          self.machine.checksum = self.image_checksum
          self.machine.image = self.chromeos_image
        self.status = 'RUNNING: %s' % self.autotest.name
        [self.retval, self.out, self.err] = self.RunTestOn(self.machine.name)
        self.run_completed = True

      finally:
        self._logger.LogOutput('Releasing machine: %s' % self.machine.name)
        machine_manager_singleton.MachineManagerSingleton().ReleaseMachine(
            self.machine)
        self._logger.LogOutput('Released machine: %s' % self.machine.name)

      self.StoreToCache()

    if not self.retval:
      self.status = 'SUCCEEDED'
    else:
      self.status = 'FAILED'

    self.ParseOutput()
    # Copy results directory to the scratch dir
    if (not cache_hit and not self.retval and self.autotest.args and
        '--profile' in self.autotest.args):
      results_dir = os.path.join(self.chromeos_root, 'chroot',
                                 self.results_dir.lstrip('/'))
      tarball = os.path.join(
          self.cache_dir, os.path.basename(os.path.dirname(self.results_dir)))
      command = ('cd %s && tar cjf %s.tbz2 .' % (results_dir, tarball))
      self._ce.RunCommand(command)
      perf_data_file = os.path.join(self.results_dir, self.full_name,
                                    'profiling/iteration.1/perf.data')

      # Attempt to build a perf report and keep it with the results.
      command = ('cd %s/src/scripts &&'
                 ' cros_sdk -- /usr/sbin/perf report --symfs=/build/%s'
                 ' -i %s --stdio' % (self.chromeos_root, self.board,
                                     perf_data_file))
      ret, out, err = self._ce.RunCommandWOutput(command)
      with open(os.path.join(self.cache_dir, 'perf.report'), 'wb') as f:
        f.write(out)
    return self.retval

  def ImageTo(self, machine_name):
    image_args = [image_chromeos.__file__, '--chromeos_root=%s' %
                  self.chromeos_root, '--image=%s' % self.chromeos_image,
                  '--remote=%s' % machine_name]
    if self.board:
      image_args.append('--board=%s' % self.board)

###    devserver_port = 8080
###    mo = re.search("\d+", self.name)
###    if mo:
###      to_add = int(mo.group(0))
###      assert to_add < 100, "Too many threads launched!"
###      devserver_port += to_add

###    # I tried --noupdate_stateful, but that still fails when run in parallel.
###    image_args.append("--image_to_live_args=\"--devserver_port=%s"
###                      " --noupdate_stateful\"" % devserver_port)
###    image_args.append("--image_to_live_args=--devserver_port=%s" %
###                      devserver_port)

# Currently can't image two machines at once.
# So have to serialized on this lock.
    self.status = 'WAITING ON IMAGE_LOCK'
    with machine_manager_singleton.MachineManagerSingleton().image_lock:
      self.status = 'IMAGING'
      retval = self._ce.RunCommand(' '.join(['python'] + image_args))
      machine_manager_singleton.MachineManagerSingleton().num_reimages += 1
      if retval:
        self.status = 'ABORTED DUE TO IMAGE FAILURE'
    return retval

  def DoPowerdHack(self):
    command = 'sudo initctl stop powerd'
    self._ce.CrosRunCommand(command,
                            machine=self.machine.name,
                            chromeos_root=self.chromeos_root)

  def RunTestOn(self, machine_name):
    command = 'cd %s/src/scripts' % self.chromeos_root
    options = ''
    if self.board:
      options += ' --board=%s' % self.board
    if self.autotest.args:
      options += " --args='%s'" % self.autotest.args
    if 'tegra2' in self.board:
      self.DoPowerdHack()
    command += ('&& cros_sdk -- /usr/bin/test_that %s %s %s' %
                (options, machine_name, self.autotest.name))
    return self._ce.RunCommand(command, True)
