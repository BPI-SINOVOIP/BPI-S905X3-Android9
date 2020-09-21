# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import collections, logging, os, re, shutil, time

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import cros_logging
from autotest_lib.client.cros.power import power_status
from autotest_lib.client.cros.power import power_utils
from autotest_lib.client.cros.power import sys_power
#pylint: disable=W0611
from autotest_lib.client.cros import flimflam_test_path
import flimflam

class Suspender(object):
    """Class for suspend/resume measurements.

    Public attributes:
        disconnect_3G_time: Amount of seconds it took to disable 3G.
        successes[]: List of timing measurement dicts from successful suspends.
        failures[]: List of SuspendFailure exceptions from failed suspends.
        device_times[]: List of individual device suspend/resume time dicts.

    Public methods:
        suspend: Do a suspend/resume cycle. Return timing measurement dict.

    Private attributes:
        _logs: Array of /var/log/messages lines since start of suspend cycle.
        _log_file: Open file descriptor at the end of /var/log/messages.
        _logdir: Directory to store firmware logs in case of errors.
        _suspend: Set to the sys_power suspend function to use.
        _throw: Set to have SuspendFailure exceptions raised to the caller.
        _reset_pm_print_times: Set to deactivate pm_print_times after the test.
        _restart_tlsdated: Set to restart tlsdated after the test.

    Private methods:
        __init__: Shuts off tlsdated for duration of test, disables 3G
        __del__: Restore tlsdated (must run eventually, but GC delay no problem)
        _set_pm_print_times: Enable/disable kernel device suspend timing output.
        _check_failure_log: Check /sys/.../suspend_stats for new failures.
        _ts: Returns a timestamp from /run/power_manager/last_resume_timings
        _hwclock_ts: Read RTC timestamp left on resume in hwclock-on-resume
        _device_resume_time: Read seconds overall device resume took from logs.
        _individual_device_times: Reads individual device suspend/resume times.
        _identify_driver: Return the driver name of a device (or "unknown").
    """

    # board-specific "time to suspend" values determined empirically
    # TODO: migrate to separate file with http://crosbug.com/38148
    _DEFAULT_SUSPEND_DELAY = 5
    _SUSPEND_DELAY = {
        # TODO: Reevaluate this when http://crosbug.com/38460 is fixed
        'daisy': 6,
        'daisy_spring': 6,
        'peach_pit': 6,

        # TODO: Reevaluate these when http://crosbug.com/38225 is fixed
        'x86-mario': 6,
        'x86-alex': 5,

        # Lumpy and Stumpy need high values, because it seems to mitigate their
        # RTC interrupt problem. See http://crosbug.com/36004
        'lumpy': 5,
        'stumpy': 5,

        # RTS5209 card reader has a really bad staging driver, can take ~1 sec
        'butterfly': 4,

        # Hard disk sync and overall just slow
        'parrot': 8,
        'kiev': 9,
    }

    # alarm/not_before value guaranteed to raise SpuriousWakeup in _hwclock_ts
    _ALARM_FORCE_EARLY_WAKEUP = 2147483647

    # File written by send_metrics_on_resume containing timing information about
    # the last resume.
    _TIMINGS_FILE = '/run/power_manager/root/last_resume_timings'

    # Amount of lines to dump from the eventlog on a SpuriousWakeup. Should be
    # enough to include ACPI Wake Reason... 10 should be far on the safe side.
    _RELEVANT_EVENTLOG_LINES = 10

    # Sanity check value to catch overlong resume times (from missed RTC wakes)
    _MAX_RESUME_TIME = 10

    # File written by powerd_suspend containing the hwclock time at resume.
    HWCLOCK_FILE = '/run/power_manager/root/hwclock-on-resume'

    # File read by powerd to decide on the state to suspend (mem or freeze).
    _SUSPEND_STATE_PREF_FILE = 'suspend_to_idle'

    def __init__(self, logdir, method=sys_power.do_suspend,
                 throw=False, device_times=False, suspend_state=''):
        """
        Prepare environment for suspending.
        @param suspend_state: Suspend state to enter into. It can be
                              'mem' or 'freeze' or an empty string. If
                              the suspend state is an empty string,
                              system suspends to the default pref.
        """
        self.disconnect_3G_time = 0
        self.successes = []
        self.failures = []
        self._logdir = logdir
        self._suspend = method
        self._throw = throw
        self._reset_pm_print_times = False
        self._restart_tlsdated = False
        self._log_file = None
        self._suspend_state = suspend_state
        if device_times:
            self.device_times = []

        # stop tlsdated, make sure we/hwclock have /dev/rtc for ourselves
        if utils.system_output('initctl status tlsdated').find('start') != -1:
            utils.system('initctl stop tlsdated')
            self._restart_tlsdated = True
            # give process's file descriptors time to asynchronously tear down
            time.sleep(0.1)

        # prime powerd_suspend RTC timestamp saving and make sure hwclock works
        utils.open_write_close(self.HWCLOCK_FILE, '')
        hwclock_output = utils.system_output('hwclock -r --debug --utc',
                                             ignore_status=True)
        if not re.search('Using.*/dev interface to.*clock', hwclock_output):
            raise error.TestError('hwclock cannot find rtc: ' + hwclock_output)

        # activate device suspend timing debug output
        if hasattr(self, 'device_times'):
            if not int(utils.read_one_line('/sys/power/pm_print_times')):
                self._set_pm_print_times(True)
                self._reset_pm_print_times = True

        # Shut down 3G to remove its variability from suspend time measurements
        flim = flimflam.FlimFlam()
        service = flim.FindCellularService(0)
        if service:
            logging.info('Found 3G interface, disconnecting.')
            start_time = time.time()
            (success, status) = flim.DisconnectService(
                    service=service, wait_timeout=60)
            if success:
                logging.info('3G disconnected successfully.')
                self.disconnect_3G_time = time.time() - start_time
            else:
                logging.error('Could not disconnect: %s.', status)
                self.disconnect_3G_time = -1

        self._configure_suspend_state()


    def _configure_suspend_state(self):
        """Configure the suspend state as requested."""
        if self._suspend_state:
            available_suspend_states = utils.read_one_line('/sys/power/state')
            if self._suspend_state not in available_suspend_states:
                raise error.TestNAError('Invalid suspend state: ' +
                                        self._suspend_state)
            # Check the current state. If it is same as the one requested,
            # we don't want to call PowerPrefChanger(restarts powerd).
            if self._suspend_state == power_utils.get_sleep_state():
                return
            should_freeze = '1' if self._suspend_state == 'freeze' else '0'
            new_prefs = {self._SUSPEND_STATE_PREF_FILE: should_freeze}
            self._power_pref_changer = power_utils.PowerPrefChanger(new_prefs)


    def _set_pm_print_times(self, on):
        """Enable/disable extra suspend timing output from powerd to syslog."""
        if utils.system('echo %s > /sys/power/pm_print_times' % int(bool(on)),
                ignore_status=True):
            logging.warning('Failed to set pm_print_times to %s', bool(on))
            del self.device_times
            self._reset_pm_print_times = False
        else:
            logging.info('Device resume times set to %s', bool(on))


    def _get_board(self):
        """Remove _freon from get_board if found."""
        return (utils.get_board().replace("_freon", ""))


    def _reset_logs(self):
        """Throw away cached log lines and reset log pointer to current end."""
        if self._log_file:
            self._log_file.close()
        self._log_file = open('/var/log/messages')
        self._log_file.seek(0, os.SEEK_END)
        self._logs = []


    def _update_logs(self, retries=11):
        """
        Read all lines logged since last reset into log cache. Block until last
        powerd_suspend resume message was read, raise if it takes too long.
        """
        finished_regex = re.compile(r'powerd_suspend\[\d+\]: Resume finished')
        for retry in xrange(retries + 1):
            lines = self._log_file.readlines()
            if lines:
                if self._logs and self._logs[-1][-1] != '\n':
                    # Reassemble line that was cut in the middle
                    self._logs[-1] += lines.pop(0)
                self._logs += lines
            for line in reversed(self._logs):
                if (finished_regex.search(line)):
                    return
            time.sleep(0.005 * 2**retry)

        raise error.TestError("Sanity check failed: did not try to suspend.")


    def _ts(self, name, retries=11):
        """Searches logs for last timestamp with a given suspend message."""
        # Occasionally need to retry due to races from process wakeup order
        for retry in xrange(retries + 1):
            try:
                f = open(self._TIMINGS_FILE)
                for line in f:
                    words = line.split('=')
                    if name == words[0]:
                        try:
                            timestamp = float(words[1])
                        except ValueError:
                            logging.warning('Invalid timestamp: %s', line)
                            timestamp = 0
                        return timestamp
            except IOError:
                pass
            time.sleep(0.005 * 2**retry)

        raise error.TestError('Could not find %s entry.' % name)


    def _hwclock_ts(self, not_before):
        """Read the RTC resume timestamp saved by powerd_suspend."""
        early_wakeup = False
        if os.path.exists(self.HWCLOCK_FILE):
            # TODO(crbug.com/733773): Still fragile see bug.
            match = re.search(r'(.+)(\.\d+)[+-]\d+:?\d+$',
                              utils.read_file(self.HWCLOCK_FILE), re.DOTALL)
            if match:
                timeval = time.strptime(match.group(1), "%Y-%m-%d %H:%M:%S")
                seconds = time.mktime(timeval)
                seconds += float(match.group(2))
                logging.debug('RTC resume timestamp read: %f', seconds)
                if seconds >= not_before:
                    return seconds
                early_wakeup = True
        if early_wakeup:
            logging.debug('Early wakeup, dumping eventlog if it exists:\n')
            elog = utils.system_output('mosys eventlog list | tail -n %d' %
                    self._RELEVANT_EVENTLOG_LINES, ignore_status=True)
            wake_elog = (['unknown'] + re.findall(r'Wake Source.*', elog))[-1]
            for line in reversed(self._logs):
                match = re.search(r'PM1_STS: WAK.*', line)
                if match:
                    wake_syslog = match.group(0)
                    break
            else:
                wake_syslog = 'unknown'
            for b, e, s in sys_power.SpuriousWakeupError.S3_WHITELIST:
                if (re.search(b, utils.get_board()) and
                        re.search(e, wake_elog) and re.search(s, wake_syslog)):
                    logging.warning('Whitelisted spurious wake in S3: %s | %s',
                                    wake_elog, wake_syslog)
                    return None
            raise sys_power.SpuriousWakeupError('Spurious wake in S3: %s | %s'
                    % (wake_elog, wake_syslog))
        if self._get_board() in ['lumpy', 'stumpy', 'kiev']:
            logging.debug('RTC read failure (crosbug/36004), dumping nvram:\n' +
                    utils.system_output('mosys nvram dump', ignore_status=True))
            return None
        raise error.TestError('Broken RTC timestamp: ' +
                              utils.read_file(self.HWCLOCK_FILE))


    def _firmware_resume_time(self):
        """Calculate seconds for firmware resume from logged TSC. (x86 only)"""
        if utils.get_arch() not in ['i686', 'x86_64']:
            # TODO: support this on ARM somehow
            return 0
        regex_freeze = re.compile(r'PM: resume from suspend-to-idle')
        regex_tsc = re.compile(r'TSC at resume: (\d+)$')
        freq = 1000 * int(utils.read_one_line(
                '/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq'))
        for line in reversed(self._logs):
            match_freeze = regex_freeze.search(line)
            if match_freeze:
                logging.info('fw resume time zero due to suspend-to-idle\n')
                return 0
            match_tsc = regex_tsc.search(line)
            if match_tsc:
                return float(match_tsc.group(1)) / freq

        raise error.TestError('Failed to find TSC resume value in syslog.')


    def _device_resume_time(self):
        """Read amount of seconds for overall device resume from syslog."""
        regex = re.compile(r'PM: resume of devices complete after ([0-9.]+)')
        for line in reversed(self._logs):
            match = regex.search(line)
            if match:
                return float(match.group(1)) / 1000

        raise error.TestError('Failed to find device resume time in syslog.')


    def _get_phase_times(self):
        phase_times = []
        regex = re.compile(r'PM: (\w+ )?(resume|suspend) of devices complete')
        for line in self._logs:
          match = regex.search(line)
          if match:
            ts = cros_logging.extract_kernel_timestamp(line)
            phase = match.group(1)
            if not phase:
              phase = 'REG'
            phase_times.append((phase.upper(), ts))
        return sorted(phase_times, key = lambda entry: entry[1])


    def _get_phase(self, ts, phase_table, dev):
      for entry in phase_table:
        #checking if timestamp was before that phase's cutoff
        if ts < entry[1]:
          return entry[0]
      raise error.TestError('Device %s has a timestamp after all devices %s',
                            dev, 'had already resumed')


    def _individual_device_times(self, start_resume):
        """Return dict of individual device suspend and resume times."""
        self.device_times.append(dict())
        dev_details = collections.defaultdict(dict)
        regex = re.compile(r'call ([^ ]+)\+ returned 0 after ([0-9]+) usecs')
        phase_table = self._get_phase_times()
        for line in self._logs:
          match = regex.search(line)
          if match:
            device = match.group(1).replace(':', '-')
            key = 'seconds_dev_' + device
            secs = float(match.group(2)) / 1e6
            ts = cros_logging.extract_kernel_timestamp(line)
            if ts > start_resume:
              key += '_resume'
            else:
              key += '_suspend'
            #looking if we're in a special phase
            phase = self._get_phase(ts, phase_table, device)
            dev = dev_details[key]
            if phase in dev:
              logging.warning('Duplicate %s entry for device %s, +%f', phase,
                              device, secs)
              dev[phase] += secs
            else:
              dev[phase] = secs

        for dev_key, dev in dev_details.iteritems():
          total_secs = sum(dev.values())
          self.device_times[-1][dev_key] = total_secs
          report = '%s: %f TOT' % (dev_key, total_secs)
          for phase in dev.keys():
            if phase is 'REG':
              continue
            report += ', %f %s' % (dev[phase], phase)
          logging.debug(report)


    def _identify_driver(self, device):
        """Return the driver name of a device (or "unknown")."""
        for path, subdirs, _ in os.walk('/sys/devices'):
            if device in subdirs:
                node = os.path.join(path, device, 'driver')
                if not os.path.exists(node):
                    return "unknown"
                return os.path.basename(os.path.realpath(node))
        else:
            return "unknown"


    def _check_for_errors(self, ignore_kernel_warns):
        """Find and identify suspend errors.

        @param ignore_kernel_warns: Ignore kernel errors.

        @returns: True iff we should retry.

        @raises:
          sys_power.KernelError: for non-whitelisted kernel failures.
          sys_power.SuspendTimeout: took too long to enter suspend.
          sys_power.SpuriousWakeupError: woke too soon from suspend.
          sys_power.SuspendFailure: unidentified failure.
        """
        warning_regex = re.compile(r' kernel: \[.*WARNING:')
        abort_regex = re.compile(r' kernel: \[.*Freezing of tasks abort'
                r'| powerd_suspend\[.*Cancel suspend at kernel'
                r'| kernel: \[.*PM: Wakeup pending, aborting suspend')
        # rsyslogd can put this out of order with dmesg, so track in variable
        fail_regex = re.compile(r'powerd_suspend\[\d+\]: Error')
        failed = False

        # TODO(scottz): warning_monitor crosbug.com/38092
        log_len = len(self._logs)
        for i in xrange(log_len):
            line = self._logs[i]
            if warning_regex.search(line):
                # match the source file from the WARNING line, and the
                # actual error text by peeking one or two lines below that
                src = cros_logging.strip_timestamp(line)
                text = ''
                if i+1 < log_len:
                    text = cros_logging.strip_timestamp(self._logs[i + 1])
                if i+2 < log_len:
                    text += '\n' + cros_logging.strip_timestamp(
                        self._logs[i + 2])
                for p1, p2 in sys_power.KernelError.WHITELIST:
                    if re.search(p1, src) and re.search(p2, text):
                        logging.info('Whitelisted KernelError: %s', src)
                        break
                else:
                    if ignore_kernel_warns:
                        logging.warn('Non-whitelisted KernelError: %s', src)
                    else:
                        raise sys_power.KernelError("%s\n%s" % (src, text))
            if abort_regex.search(line):
                wake_source = 'unknown'
                match = re.search(r'last active wakeup source: (.*)$',
                        '\n'.join(self._logs[i-5:i+3]), re.MULTILINE)
                if match:
                    wake_source = match.group(1)
                driver = self._identify_driver(wake_source)
                for b, w in sys_power.SpuriousWakeupError.S0_WHITELIST:
                    if (re.search(b, utils.get_board()) and
                            re.search(w, wake_source)):
                        logging.warning('Whitelisted spurious wake before '
                                        'S3: %s | %s', wake_source, driver)
                        return True
                if "rtc" in driver:
                    raise sys_power.SuspendTimeout('System took too '
                                                   'long to suspend.')
                raise sys_power.SpuriousWakeupError('Spurious wake '
                        'before S3: %s | %s' % (wake_source, driver))
            if fail_regex.search(line):
                failed = True

        if failed:
            raise sys_power.SuspendFailure('Unidentified problem.')
        return False


    def _arc_resume_ts(self, retries=3):
        """Parse arc logcat for arc resume timestamp.

        @param retries: retry if the expected log cannot be found in logcat.

        @returns: a float representing arc resume timestamp in  CPU seconds
                  starting from the last boot if arc logcat resume log is parsed
                  correctly; otherwise 'unknown'.

        """
        command = 'android-sh -c "logcat -v monotonic -t 300 *:silent' \
                  ' ArcPowerManagerService:D"'
        regex_resume = re.compile(r'^\s*(\d+\.\d+).*ArcPowerManagerService: '
                                  'Suspend is over; resuming all apps$')
        for retry in xrange(retries + 1):
            arc_logcat = utils.system_output(command, ignore_status=False)
            arc_logcat = arc_logcat.splitlines()
            resume_ts = 0
            for line in arc_logcat:
                match_resume = regex_resume.search(line)
                # ARC logcat is cleared before suspend so the first ARC resume
                # timestamp is the one we are looking for.
                if match_resume:
                    return float(match_resume.group(1))
            time.sleep(0.005 * 2**retry)
        else:
            logging.error('ARC did not resume correctly.')
            return 'unknown'


    def suspend(self, duration=10, ignore_kernel_warns=False,
                measure_arc=False):
        """
        Do a single suspend for 'duration' seconds. Estimates the amount of time
        it takes to suspend for a board (see _SUSPEND_DELAY), so the actual RTC
        wakeup delay will be longer. Returns None on errors, or raises the
        exception when _throw is set. Returns a dict of general measurements,
        or a tuple (general_measurements, individual_device_times) when
        _device_times is set.

        @param duration: time in seconds to do a suspend prior to waking.
        @param ignore_kernel_warns: Ignore kernel errors.  Defaults to false.
        """

        if power_utils.get_sleep_state() == 'freeze':
            self._s0ix_residency_stats = power_status.S0ixResidencyStats()

        try:
            iteration = len(self.failures) + len(self.successes) + 1
            # Retry suspend in case we hit a known (whitelisted) bug
            for _ in xrange(10):
                # Clear powerd_suspend RTC timestamp, to avoid stale results.
                utils.open_write_close(self.HWCLOCK_FILE, '')
                self._reset_logs()
                utils.system('sync')
                board_delay = self._SUSPEND_DELAY.get(self._get_board(),
                        self._DEFAULT_SUSPEND_DELAY)
                # Clear the ARC logcat to make parsing easier.
                if measure_arc:
                    command = 'android-sh -c "logcat -c"'
                    utils.system(command, ignore_status=False)
                try:
                    alarm = self._suspend(duration + board_delay)
                except sys_power.SpuriousWakeupError:
                    # might be another error, we check for it ourselves below
                    alarm = self._ALARM_FORCE_EARLY_WAKEUP

                if os.path.exists('/sys/firmware/log'):
                    for msg in re.findall(r'^.*ERROR.*$',
                            utils.read_file('/sys/firmware/log'), re.M):
                        for board, pattern in sys_power.FirmwareError.WHITELIST:
                            if (re.search(board, utils.get_board()) and
                                    re.search(pattern, msg)):
                                logging.info('Whitelisted FW error: ' + msg)
                                break
                        else:
                            firmware_log = os.path.join(self._logdir,
                                    'firmware.log.' + str(iteration))
                            shutil.copy('/sys/firmware/log', firmware_log)
                            logging.info('Saved firmware log: ' + firmware_log)
                            raise sys_power.FirmwareError(msg.strip('\r\n '))

                self._update_logs()
                if not self._check_for_errors(ignore_kernel_warns):
                    hwclock_ts = self._hwclock_ts(alarm)
                    if hwclock_ts:
                        break

            else:
                raise error.TestWarn('Ten tries failed due to whitelisted bug')

            # calculate general measurements
            start_resume = self._ts('start_resume_time')
            kernel_down = (self._ts('end_suspend_time') -
                           self._ts('start_suspend_time'))
            kernel_up = self._ts('end_resume_time') - start_resume
            devices_up = self._device_resume_time()
            total_up = hwclock_ts - alarm
            firmware_up = self._firmware_resume_time()
            board_up = total_up - kernel_up - firmware_up
            try:
                cpu_up = self._ts('cpu_ready_time', 0) - start_resume
            except error.TestError:
                # can be missing on non-SMP machines
                cpu_up = None
            if total_up > self._MAX_RESUME_TIME:
                raise error.TestError('Sanity check failed: missed RTC wakeup.')

            logging.info('Success(%d): %g down, %g up, %g board, %g firmware, '
                         '%g kernel, %g cpu, %g devices',
                         iteration, kernel_down, total_up, board_up,
                         firmware_up, kernel_up, cpu_up, devices_up)

            if hasattr(self, '_s0ix_residency_stats'):
                s0ix_residency_secs = \
                        self._s0ix_residency_stats.\
                                get_accumulated_residency_secs()
                if not s0ix_residency_secs:
                    raise sys_power.S0ixResidencyNotChanged(
                        'S0ix residency did not change.')
                logging.info('S0ix residency : %d secs.', s0ix_residency_secs)

            successful_suspend = {
                'seconds_system_suspend': kernel_down,
                'seconds_system_resume': total_up,
                'seconds_system_resume_firmware': firmware_up + board_up,
                'seconds_system_resume_firmware_cpu': firmware_up,
                'seconds_system_resume_firmware_ec': board_up,
                'seconds_system_resume_kernel': kernel_up,
                'seconds_system_resume_kernel_cpu': cpu_up,
                'seconds_system_resume_kernel_dev': devices_up,
                }

            if measure_arc:
                successful_suspend['seconds_system_resume_arc'] = \
                        self._arc_resume_ts() - start_resume

            self.successes.append(successful_suspend)

            if hasattr(self, 'device_times'):
                self._individual_device_times(start_resume)
                return (self.successes[-1], self.device_times[-1])
            else:
                return self.successes[-1]

        except sys_power.SuspendFailure as ex:
            message = '%s(%d): %s' % (type(ex).__name__, iteration, ex)
            logging.error(message)
            self.failures.append(ex)
            if self._throw:
                if type(ex).__name__ in ['KernelError', 'SuspendTimeout']:
                    raise error.TestWarn(message)
                else:
                    raise error.TestFail(message)
            return None


    def finalize(self):
        """Restore normal environment (not turning 3G back on for now...)"""
        if os.path.exists(self.HWCLOCK_FILE):
            os.remove(self.HWCLOCK_FILE)
            if self._restart_tlsdated:
                utils.system('initctl start tlsdated')
            if self._reset_pm_print_times:
                self._set_pm_print_times(False)
        if hasattr(self, '_power_pref_changer'):
            self._power_pref_changer.finalize()


    def __del__(self):
        self.finalize()
