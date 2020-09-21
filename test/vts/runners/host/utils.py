#
# Copyright (C) 2016 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import base64
import concurrent.futures
import datetime
import functools
import json
import logging
import os
import random
import re
import signal
import string
import subprocess
import time
import traceback

# File name length is limited to 255 chars on some OS, so we need to make sure
# the file names we output fits within the limit.
MAX_FILENAME_LEN = 255
# Path length is limited to 4096 chars on some OS, so we need to make sure
# the path we output fits within the limit.
MAX_PATH_LEN = 4096


class VTSUtilsError(Exception):
    """Generic error raised for exceptions in ACTS utils."""


class NexusModelNames:
    # TODO(angli): This will be fixed later by angli.
    ONE = 'sprout'
    N5 = 'hammerhead'
    N5v2 = 'bullhead'
    N6 = 'shamu'
    N6v2 = 'angler'


ascii_letters_and_digits = string.ascii_letters + string.digits
valid_filename_chars = "-_." + ascii_letters_and_digits

models = ("sprout", "occam", "hammerhead", "bullhead", "razor", "razorg",
          "shamu", "angler", "volantis", "volantisg", "mantaray", "fugu",
          "ryu")

manufacture_name_to_model = {
    "flo": "razor",
    "flo_lte": "razorg",
    "flounder": "volantis",
    "flounder_lte": "volantisg",
    "dragon": "ryu"
}

GMT_to_olson = {
    "GMT-9": "America/Anchorage",
    "GMT-8": "US/Pacific",
    "GMT-7": "US/Mountain",
    "GMT-6": "US/Central",
    "GMT-5": "US/Eastern",
    "GMT-4": "America/Barbados",
    "GMT-3": "America/Buenos_Aires",
    "GMT-2": "Atlantic/South_Georgia",
    "GMT-1": "Atlantic/Azores",
    "GMT+0": "Africa/Casablanca",
    "GMT+1": "Europe/Amsterdam",
    "GMT+2": "Europe/Athens",
    "GMT+3": "Europe/Moscow",
    "GMT+4": "Asia/Baku",
    "GMT+5": "Asia/Oral",
    "GMT+6": "Asia/Almaty",
    "GMT+7": "Asia/Bangkok",
    "GMT+8": "Asia/Hong_Kong",
    "GMT+9": "Asia/Tokyo",
    "GMT+10": "Pacific/Guam",
    "GMT+11": "Pacific/Noumea",
    "GMT+12": "Pacific/Fiji",
    "GMT+13": "Pacific/Tongatapu",
    "GMT-11": "Pacific/Midway",
    "GMT-10": "Pacific/Honolulu"
}


def abs_path(path):
    """Resolve the '.' and '~' in a path to get the absolute path.

    Args:
        path: The path to expand.

    Returns:
        The absolute path of the input path.
    """
    return os.path.abspath(os.path.expanduser(path))


def create_dir(path):
    """Creates a directory if it does not exist already.

    Args:
        path: The path of the directory to create.
    """
    full_path = abs_path(path)
    if not os.path.exists(full_path):
        os.makedirs(full_path)


def get_current_epoch_time():
    """Current epoch time in milliseconds.

    Returns:
        An integer representing the current epoch time in milliseconds.
    """
    return int(round(time.time() * 1000))


def get_current_human_time():
    """Returns the current time in human readable format.

    Returns:
        The current time stamp in Month-Day-Year Hour:Min:Sec format.
    """
    return time.strftime("%m-%d-%Y %H:%M:%S ")


def epoch_to_human_time(epoch_time):
    """Converts an epoch timestamp to human readable time.

    This essentially converts an output of get_current_epoch_time to an output
    of get_current_human_time

    Args:
        epoch_time: An integer representing an epoch timestamp in milliseconds.

    Returns:
        A time string representing the input time.
        None if input param is invalid.
    """
    if isinstance(epoch_time, int):
        try:
            d = datetime.datetime.fromtimestamp(epoch_time / 1000)
            return d.strftime("%m-%d-%Y %H:%M:%S ")
        except ValueError:
            return None


def get_timezone_olson_id():
    """Return the Olson ID of the local (non-DST) timezone.

    Returns:
        A string representing one of the Olson IDs of the local (non-DST)
        timezone.
    """
    tzoffset = int(time.timezone / 3600)
    gmt = None
    if tzoffset <= 0:
        gmt = "GMT+{}".format(-tzoffset)
    else:
        gmt = "GMT-{}".format(tzoffset)
    return GMT_to_olson[gmt]


def find_files(paths, file_predicate):
    """Locate files whose names and extensions match the given predicate in
    the specified directories.

    Args:
        paths: A list of directory paths where to find the files.
        file_predicate: A function that returns True if the file name and
          extension are desired.

    Returns:
        A list of files that match the predicate.
    """
    file_list = []
    for path in paths:
        p = abs_path(path)
        for dirPath, subdirList, fileList in os.walk(p):
            for fname in fileList:
                name, ext = os.path.splitext(fname)
                if file_predicate(name, ext):
                    file_list.append((dirPath, name, ext))
    return file_list


def iterate_files(dir_path):
    """A generator yielding regular files in a directory recursively.

    Args:
        dir_path: A string representing the path to search.

    Yields:
        A tuple of strings (directory, file). The directory containing
        the file and the file name.
    """
    for root_dir, dir_names, file_names in os.walk(dir_path):
        for file_name in file_names:
            yield root_dir, file_name


def load_config(file_full_path):
    """Loads a JSON config file.

    Returns:
        A JSON object.
    """
    if not os.path.isfile(file_full_path):
        logging.warning('cwd: %s', os.getcwd())
        pypath = os.environ['PYTHONPATH']
        if pypath:
            for base_path in pypath.split(':'):
                logging.info('checking %s', base_path)
                new_path = os.path.join(base_path, file_full_path)
                if os.path.isfile(new_path):
                    logging.info('found')
                    file_full_path = new_path
                    break

    with open(file_full_path, 'r') as f:
        conf = json.load(f)
        return conf


def load_file_to_base64_str(f_path):
    """Loads the content of a file into a base64 string.

    Args:
        f_path: full path to the file including the file name.

    Returns:
        A base64 string representing the content of the file in utf-8 encoding.
    """
    path = abs_path(f_path)
    with open(path, 'rb') as f:
        f_bytes = f.read()
        base64_str = base64.b64encode(f_bytes).decode("utf-8")
        return base64_str


def find_field(item_list, cond, comparator, target_field):
    """Finds the value of a field in a dict object that satisfies certain
    conditions.

    Args:
        item_list: A list of dict objects.
        cond: A param that defines the condition.
        comparator: A function that checks if an dict satisfies the condition.
        target_field: Name of the field whose value to be returned if an item
            satisfies the condition.

    Returns:
        Target value or None if no item satisfies the condition.
    """
    for item in item_list:
        if comparator(item, cond) and target_field in item:
            return item[target_field]
    return None


def rand_ascii_str(length):
    """Generates a random string of specified length, composed of ascii letters
    and digits.

    Args:
        length: The number of characters in the string.

    Returns:
        The random string generated.
    """
    letters = [random.choice(ascii_letters_and_digits) for i in range(length)]
    return ''.join(letters)


# Thead/Process related functions.
def concurrent_exec(func, param_list):
    """Executes a function with different parameters pseudo-concurrently.

    This is basically a map function. Each element (should be an iterable) in
    the param_list is unpacked and passed into the function. Due to Python's
    GIL, there's no true concurrency. This is suited for IO-bound tasks.

    Args:
        func: The function that parforms a task.
        param_list: A list of iterables, each being a set of params to be
            passed into the function.

    Returns:
        A list of return values from each function execution. If an execution
        caused an exception, the exception object will be the corresponding
        result.
    """
    with concurrent.futures.ThreadPoolExecutor(max_workers=30) as executor:
        # Start the load operations and mark each future with its params
        future_to_params = {executor.submit(func, *p): p for p in param_list}
        return_vals = []
        for future in concurrent.futures.as_completed(future_to_params):
            params = future_to_params[future]
            try:
                return_vals.append(future.result())
            except Exception as exc:
                print("{} generated an exception: {}".format(
                    params, traceback.format_exc()))
                return_vals.append(exc)
        return return_vals


def exe_cmd(*cmds):
    """Executes commands in a new shell.

    Args:
        cmds: A sequence of commands and arguments.

    Returns:
        The output of the command run.

    Raises:
        OSError is raised if an error occurred during the command execution.
    """
    cmd = ' '.join(cmds)
    proc = subprocess.Popen(
        cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
    (out, err) = proc.communicate()
    if not err:
        return out
    raise OSError(err)


def _assert_subprocess_running(proc):
    """Checks if a subprocess has terminated on its own.

    Args:
        proc: A subprocess returned by subprocess.Popen.

    Raises:
        VTSUtilsError is raised if the subprocess has stopped.
    """
    ret = proc.poll()
    if ret is not None:
        out, err = proc.communicate()
        raise VTSUtilsError("Process %d has terminated. ret: %d, stderr: %s,"
                            " stdout: %s" % (proc.pid, ret, err, out))


def is_on_windows():
    """Checks whether the OS is Windows.

    Returns:
        A boolean representing whether the OS is Windows.
    """
    return os.name == "nt"


def kill_process_group(proc, signal_no=signal.SIGTERM):
    """Sends signal to a process group.

    Logs when there is an OSError or PermissionError. The latter one only
    happens on Mac.

    On Windows, SIGABRT, SIGINT, and SIGTERM are replaced with CTRL_BREAK_EVENT
    so as to kill every subprocess in the group.

    Args:
        proc: The Popen object whose pid is the group id.
        signal_no: The signal sent to the subprocess group.
    """
    pid = proc.pid
    try:
        if not is_on_windows():
            os.killpg(pid, signal_no)
        else:
            if signal_no in [signal.SIGABRT,
                             signal.SIGINT,
                             signal.SIGTERM]:
                windows_signal_no = signal.CTRL_BREAK_EVENT
            else:
                windows_signal_no = signal_no
            os.kill(pid, windows_signal_no)
    except (OSError, PermissionError) as e:
        logging.exception("Cannot send signal %s to process group %d: %s",
                          signal_no, pid, str(e))


def start_standing_subprocess(cmd, check_health_delay=0):
    """Starts a long-running subprocess.

    This is not a blocking call and the subprocess started by it should be
    explicitly terminated with stop_standing_subprocess.

    For short-running commands, you should use exe_cmd, which blocks.

    You can specify a health check after the subprocess is started to make sure
    it did not stop prematurely.

    Args:
        cmd: string, the command to start the subprocess with.
        check_health_delay: float, the number of seconds to wait after the
                            subprocess starts to check its health. Default is 0,
                            which means no check.

    Returns:
        The subprocess that got started.
    """
    if not is_on_windows():
        proc = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            shell=True,
            preexec_fn=os.setpgrp)
    else:
        proc = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            shell=True,
            creationflags=subprocess.CREATE_NEW_PROCESS_GROUP)
    logging.debug("Start standing subprocess with cmd: %s", cmd)
    if check_health_delay > 0:
        time.sleep(check_health_delay)
        _assert_subprocess_running(proc)
    return proc


def stop_standing_subprocess(proc, signal_no=signal.SIGTERM):
    """Stops a subprocess started by start_standing_subprocess.

    Before killing the process, we check if the process is running, if it has
    terminated, VTSUtilsError is raised.

    Args:
        proc: Subprocess to terminate.
        signal_no: The signal sent to the subprocess group.
    """
    logging.debug("Stop standing subprocess %d", proc.pid)
    _assert_subprocess_running(proc)
    kill_process_group(proc, signal_no)


def wait_for_standing_subprocess(proc, timeout=None):
    """Waits for a subprocess started by start_standing_subprocess to finish
    or times out.

    Propagates the exception raised by the subprocess.wait(.) function.
    The subprocess.TimeoutExpired exception is raised if the process timed-out
    rather then terminating.

    If no exception is raised: the subprocess terminated on its own. No need
    to call stop_standing_subprocess() to kill it.

    If an exception is raised: the subprocess is still alive - it did not
    terminate. Either call stop_standing_subprocess() to kill it, or call
    wait_for_standing_subprocess() to keep waiting for it to terminate on its
    own.

    Args:
        p: Subprocess to wait for.
        timeout: An integer number of seconds to wait before timing out.
    """
    proc.wait(timeout)


def sync_device_time(ad):
    """Sync the time of an android device with the current system time.

    Both epoch time and the timezone will be synced.

    Args:
        ad: The android device to sync time on.
    """
    droid = ad.droid
    droid.setTimeZone(get_timezone_olson_id())
    droid.setTime(get_current_epoch_time())


# Timeout decorator block
class TimeoutError(Exception):
    """Exception for timeout decorator related errors.
    """
    pass


def _timeout_handler(signum, frame):
    """Handler function used by signal to terminate a timed out function.
    """
    raise TimeoutError()


def timeout(sec):
    """A decorator used to add time out check to a function.

    Args:
        sec: Number of seconds to wait before the function times out.
            No timeout if set to 0

    Returns:
        What the decorated function returns.

    Raises:
        TimeoutError is raised when time out happens.
    """

    def decorator(func):
        @functools.wraps(func)
        def wrapper(*args, **kwargs):
            if sec:
                signal.signal(signal.SIGALRM, _timeout_handler)
                signal.alarm(sec)
            try:
                return func(*args, **kwargs)
            except TimeoutError:
                raise TimeoutError(("Function {} timed out after {} "
                                    "seconds.").format(func.__name__, sec))
            finally:
                signal.alarm(0)

        return wrapper

    return decorator


def trim_model_name(model):
    """Trim any prefix and postfix and return the android designation of the
    model name.

    e.g. "m_shamu" will be trimmed to "shamu".

    Args:
        model: model name to be trimmed.

    Returns
        Trimmed model name if one of the known model names is found.
        None otherwise.
    """
    # Directly look up first.
    if model in models:
        return model
    if model in manufacture_name_to_model:
        return manufacture_name_to_model[model]
    # If not found, try trimming off prefix/postfix and look up again.
    tokens = re.split("_|-", model)
    for t in tokens:
        if t in models:
            return t
        if t in manufacture_name_to_model:
            return manufacture_name_to_model[t]
    return None


def force_airplane_mode(ad, new_state, timeout_value=60):
    """Force the device to set airplane mode on or off by adb shell command.

    Args:
        ad: android device object.
        new_state: Turn on airplane mode if True.
            Turn off airplane mode if False.
        timeout_value: max wait time for 'adb wait-for-device'

    Returns:
        True if success.
        False if timeout.
    """
    # Using timeout decorator.
    # Wait for device with timeout. If after <timeout_value> seconds, adb
    # is still waiting for device, throw TimeoutError exception.
    @timeout(timeout_value)
    def wait_for_device_with_timeout(ad):
        ad.adb.wait_for_device()

    try:
        wait_for_device_with_timeout(ad)
        ad.adb.shell("settings put global airplane_mode_on {}".format(
            1 if new_state else 0))
    except TimeoutError:
        # adb wait for device timeout
        return False
    return True


def enable_doze(ad):
    """Force the device into doze mode.

    Args:
        ad: android device object.

    Returns:
        True if device is in doze mode.
        False otherwise.
    """
    ad.adb.shell("dumpsys battery unplug")
    ad.adb.shell("dumpsys deviceidle enable")
    if (ad.adb.shell("dumpsys deviceidle force-idle") !=
            b'Now forced in to idle mode\r\n'):
        return False
    ad.droid.goToSleepNow()
    time.sleep(5)
    adb_shell_result = ad.adb.shell("dumpsys deviceidle step")
    if adb_shell_result not in [b'Stepped to: IDLE_MAINTENANCE\r\n',
                                b'Stepped to: IDLE\r\n']:
        info = ("dumpsys deviceidle step: {}dumpsys battery: {}"
                "dumpsys deviceidle: {}".format(
                    adb_shell_result.decode('utf-8'),
                    ad.adb.shell("dumpsys battery").decode('utf-8'),
                    ad.adb.shell("dumpsys deviceidle").decode('utf-8')))
        print(info)
        return False
    return True


def disable_doze(ad):
    """Force the device not in doze mode.

    Args:
        ad: android device object.

    Returns:
        True if device is not in doze mode.
        False otherwise.
    """
    ad.adb.shell("dumpsys deviceidle disable")
    ad.adb.shell("dumpsys battery reset")
    adb_shell_result = ad.adb.shell("dumpsys deviceidle step")
    if (adb_shell_result != b'Stepped to: ACTIVE\r\n'):
        info = ("dumpsys deviceidle step: {}dumpsys battery: {}"
                "dumpsys deviceidle: {}".format(
                    adb_shell_result.decode('utf-8'),
                    ad.adb.shell("dumpsys battery").decode('utf-8'),
                    ad.adb.shell("dumpsys deviceidle").decode('utf-8')))
        print(info)
        return False
    return True


def set_ambient_display(ad, new_state):
    """Set "Ambient Display" in Settings->Display

    Args:
        ad: android device object.
        new_state: new state for "Ambient Display". True or False.
    """
    ad.adb.shell("settings put secure doze_enabled {}".format(1 if new_state
                                                              else 0))


def set_adaptive_brightness(ad, new_state):
    """Set "Adaptive Brightness" in Settings->Display

    Args:
        ad: android device object.
        new_state: new state for "Adaptive Brightness". True or False.
    """
    ad.adb.shell("settings put system screen_brightness_mode {}".format(
        1 if new_state else 0))


def set_auto_rotate(ad, new_state):
    """Set "Auto-rotate" in QuickSetting

    Args:
        ad: android device object.
        new_state: new state for "Auto-rotate". True or False.
    """
    ad.adb.shell("settings put system accelerometer_rotation {}".format(
        1 if new_state else 0))


def set_location_service(ad, new_state):
    """Set Location service on/off in Settings->Location

    Args:
        ad: android device object.
        new_state: new state for "Location service".
            If new_state is False, turn off location service.
            If new_state if True, set location service to "High accuracy".
    """
    if new_state:
        ad.adb.shell("settings put secure location_providers_allowed +gps")
        ad.adb.shell("settings put secure location_providers_allowed +network")
    else:
        ad.adb.shell("settings put secure location_providers_allowed -gps")
        ad.adb.shell("settings put secure location_providers_allowed -network")


def set_mobile_data_always_on(ad, new_state):
    """Set Mobile_Data_Always_On feature bit

    Args:
        ad: android device object.
        new_state: new state for "mobile_data_always_on"
            if new_state is False, set mobile_data_always_on disabled.
            if new_state if True, set mobile_data_always_on enabled.
    """
    ad.adb.shell("settings put global mobile_data_always_on {}".format(
        1 if new_state else 0))
