# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Convenience functions for use by tests or whomever.
"""

# pylint: disable=missing-docstring

import commands
import fnmatch
import glob
import json
import logging
import math
import multiprocessing
import os
import pickle
import platform
import re
import shutil
import signal
import tempfile
import time
import uuid

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import magic
from autotest_lib.client.common_lib import utils

from autotest_lib.client.common_lib.utils import *


def grep(pattern, file):
    """
    This is mainly to fix the return code inversion from grep
    Also handles compressed files.

    returns 1 if the pattern is present in the file, 0 if not.
    """
    command = 'grep "%s" > /dev/null' % pattern
    ret = cat_file_to_cmd(file, command, ignore_status=True)
    return not ret


def difflist(list1, list2):
    """returns items in list2 that are not in list1"""
    diff = [];
    for x in list2:
        if x not in list1:
            diff.append(x)
    return diff


def cat_file_to_cmd(file, command, ignore_status=0, return_output=False):
    """
    equivalent to 'cat file | command' but knows to use
    zcat or bzcat if appropriate
    """
    if not os.path.isfile(file):
        raise NameError('invalid file %s to cat to command %s'
                % (file, command))

    if return_output:
        run_cmd = utils.system_output
    else:
        run_cmd = utils.system

    if magic.guess_type(file) == 'application/x-bzip2':
        cat = 'bzcat'
    elif magic.guess_type(file) == 'application/x-gzip':
        cat = 'zcat'
    else:
        cat = 'cat'
    return run_cmd('%s %s | %s' % (cat, file, command),
                   ignore_status=ignore_status)


def extract_tarball_to_dir(tarball, dir):
    """
    Extract a tarball to a specified directory name instead of whatever
    the top level of a tarball is - useful for versioned directory names, etc
    """
    if os.path.exists(dir):
        if os.path.isdir(dir):
            shutil.rmtree(dir)
        else:
            os.remove(dir)
    pwd = os.getcwd()
    os.chdir(os.path.dirname(os.path.abspath(dir)))
    newdir = extract_tarball(tarball)
    os.rename(newdir, dir)
    os.chdir(pwd)


def extract_tarball(tarball):
    """Returns the directory extracted by the tarball."""
    extracted = cat_file_to_cmd(tarball, 'tar xvf - 2>/dev/null',
                                    return_output=True).splitlines()

    dir = None

    for line in extracted:
        if line.startswith('./'):
            line = line[2:]
        if not line or line == '.':
            continue
        topdir = line.split('/')[0]
        if os.path.isdir(topdir):
            if dir:
                assert(dir == topdir)
            else:
                dir = topdir
    if dir:
        return dir
    else:
        raise NameError('extracting tarball produced no dir')


def force_copy(src, dest):
    """Replace dest with a new copy of src, even if it exists"""
    if os.path.isfile(dest):
        os.remove(dest)
    if os.path.isdir(dest):
        dest = os.path.join(dest, os.path.basename(src))
    shutil.copyfile(src, dest)
    return dest


def force_link(src, dest):
    """Link src to dest, overwriting it if it exists"""
    return utils.system("ln -sf %s %s" % (src, dest))


def file_contains_pattern(file, pattern):
    """Return true if file contains the specified egrep pattern"""
    if not os.path.isfile(file):
        raise NameError('file %s does not exist' % file)
    return not utils.system('egrep -q "' + pattern + '" ' + file,
                            ignore_status=True)


def list_grep(list, pattern):
    """True if any item in list matches the specified pattern."""
    compiled = re.compile(pattern)
    for line in list:
        match = compiled.search(line)
        if (match):
            return 1
    return 0


def get_os_vendor():
    """Try to guess what's the os vendor
    """
    if os.path.isfile('/etc/SuSE-release'):
        return 'SUSE'

    issue = '/etc/issue'

    if not os.path.isfile(issue):
        return 'Unknown'

    if file_contains_pattern(issue, 'Red Hat'):
        return 'Red Hat'
    elif file_contains_pattern(issue, 'Fedora'):
        return 'Fedora Core'
    elif file_contains_pattern(issue, 'SUSE'):
        return 'SUSE'
    elif file_contains_pattern(issue, 'Ubuntu'):
        return 'Ubuntu'
    elif file_contains_pattern(issue, 'Debian'):
        return 'Debian'
    else:
        return 'Unknown'


def get_cc():
    try:
        return os.environ['CC']
    except KeyError:
        return 'gcc'


def get_vmlinux():
    """Return the full path to vmlinux

    Ahem. This is crap. Pray harder. Bad Martin.
    """
    vmlinux = '/boot/vmlinux-%s' % utils.system_output('uname -r')
    if os.path.isfile(vmlinux):
        return vmlinux
    vmlinux = '/lib/modules/%s/build/vmlinux' % utils.system_output('uname -r')
    if os.path.isfile(vmlinux):
        return vmlinux
    return None


def get_systemmap():
    """Return the full path to System.map

    Ahem. This is crap. Pray harder. Bad Martin.
    """
    map = '/boot/System.map-%s' % utils.system_output('uname -r')
    if os.path.isfile(map):
        return map
    map = '/lib/modules/%s/build/System.map' % utils.system_output('uname -r')
    if os.path.isfile(map):
        return map
    return None


def get_modules_dir():
    """Return the modules dir for the running kernel version"""
    kernel_version = utils.system_output('uname -r')
    return '/lib/modules/%s/kernel' % kernel_version


_CPUINFO_RE = re.compile(r'^(?P<key>[^\t]*)\t*: ?(?P<value>.*)$')


def get_cpuinfo():
    """Read /proc/cpuinfo and convert to a list of dicts."""
    cpuinfo = []
    with open('/proc/cpuinfo', 'r') as f:
        cpu = {}
        for line in f:
            line = line.strip()
            if not line:
                cpuinfo.append(cpu)
                cpu = {}
                continue
            match = _CPUINFO_RE.match(line)
            cpu[match.group('key')] = match.group('value')
        if cpu:
            # cpuinfo usually ends in a blank line, so this shouldn't happen.
            cpuinfo.append(cpu)
    return cpuinfo


def get_cpu_arch():
    """Work out which CPU architecture we're running on"""
    f = open('/proc/cpuinfo', 'r')
    cpuinfo = f.readlines()
    f.close()
    if list_grep(cpuinfo, '^cpu.*(RS64|POWER3|Broadband Engine)'):
        return 'power'
    elif list_grep(cpuinfo, '^cpu.*POWER4'):
        return 'power4'
    elif list_grep(cpuinfo, '^cpu.*POWER5'):
        return 'power5'
    elif list_grep(cpuinfo, '^cpu.*POWER6'):
        return 'power6'
    elif list_grep(cpuinfo, '^cpu.*POWER7'):
        return 'power7'
    elif list_grep(cpuinfo, '^cpu.*PPC970'):
        return 'power970'
    elif list_grep(cpuinfo, 'ARM'):
        return 'arm'
    elif list_grep(cpuinfo, '^flags.*:.* lm .*'):
        return 'x86_64'
    elif list_grep(cpuinfo, 'CPU.*implementer.*0x41'):
        return 'arm'
    else:
        return 'i386'


def get_arm_soc_family_from_devicetree():
    """
    Work out which ARM SoC we're running on based on the 'compatible' property
    of the base node of devicetree, if it exists.
    """
    devicetree_compatible = '/sys/firmware/devicetree/base/compatible'
    if not os.path.isfile(devicetree_compatible):
        return None
    f = open(devicetree_compatible, 'r')
    compatible = f.readlines()
    f.close()
    if list_grep(compatible, 'rk3399'):
        return 'rockchip'
    elif list_grep(compatible, 'mt8173'):
        return 'mediatek'
    return None


def get_arm_soc_family():
    """Work out which ARM SoC we're running on"""
    family = get_arm_soc_family_from_devicetree()
    if family is not None:
        return family

    f = open('/proc/cpuinfo', 'r')
    cpuinfo = f.readlines()
    f.close()
    if list_grep(cpuinfo, 'EXYNOS5'):
        return 'exynos5'
    elif list_grep(cpuinfo, 'Tegra'):
        return 'tegra'
    elif list_grep(cpuinfo, 'Rockchip'):
        return 'rockchip'
    return 'arm'


def get_cpu_soc_family():
    """Like get_cpu_arch, but for ARM, returns the SoC family name"""
    f = open('/proc/cpuinfo', 'r')
    cpuinfo = f.readlines()
    f.close()
    family = get_cpu_arch()
    if family == 'arm':
        family = get_arm_soc_family()
    if list_grep(cpuinfo, '^vendor_id.*:.*AMD'):
        family = 'amd'
    return family


INTEL_UARCH_TABLE = {
    '06_4C': 'Airmont',
    '06_1C': 'Atom',
    '06_26': 'Atom',
    '06_27': 'Atom',
    '06_35': 'Atom',
    '06_36': 'Atom',
    '06_3D': 'Broadwell',
    '06_47': 'Broadwell',
    '06_4F': 'Broadwell',
    '06_56': 'Broadwell',
    '06_0D': 'Dothan',
    '06_5C': 'Goldmont',
    '06_3C': 'Haswell',
    '06_45': 'Haswell',
    '06_46': 'Haswell',
    '06_3F': 'Haswell-E',
    '06_3A': 'Ivy Bridge',
    '06_3E': 'Ivy Bridge-E',
    '06_8E': 'Kaby Lake',
    '06_9E': 'Kaby Lake',
    '06_0F': 'Merom',
    '06_16': 'Merom',
    '06_17': 'Nehalem',
    '06_1A': 'Nehalem',
    '06_1D': 'Nehalem',
    '06_1E': 'Nehalem',
    '06_1F': 'Nehalem',
    '06_2E': 'Nehalem',
    '0F_03': 'Prescott',
    '0F_04': 'Prescott',
    '0F_06': 'Presler',
    '06_2A': 'Sandy Bridge',
    '06_2D': 'Sandy Bridge',
    '06_37': 'Silvermont',
    '06_4A': 'Silvermont',
    '06_4D': 'Silvermont',
    '06_5A': 'Silvermont',
    '06_5D': 'Silvermont',
    '06_4E': 'Skylake',
    '06_5E': 'Skylake',
    '06_55': 'Skylake',
    '06_25': 'Westmere',
    '06_2C': 'Westmere',
    '06_2F': 'Westmere',
}


def get_intel_cpu_uarch(numeric=False):
    """Return the Intel microarchitecture we're running on, or None.

    Returns None if this is not an Intel CPU. Returns the family and model as
    underscore-separated hex (per Intel manual convention) if the uarch is not
    known, or if numeric is True.
    """
    if not get_current_kernel_arch().startswith('x86'):
        return None
    cpuinfo = get_cpuinfo()[0]
    if cpuinfo['vendor_id'] != 'GenuineIntel':
        return None
    family_model = '%02X_%02X' % (int(cpuinfo['cpu family']),
                                  int(cpuinfo['model']))
    if numeric:
        return family_model
    return INTEL_UARCH_TABLE.get(family_model, family_model)


def get_current_kernel_arch():
    """Get the machine architecture, now just a wrap of 'uname -m'."""
    return os.popen('uname -m').read().rstrip()


def get_file_arch(filename):
    # -L means follow symlinks
    file_data = utils.system_output('file -L ' + filename)
    if file_data.count('80386'):
        return 'i386'
    return None


def count_cpus():
    """number of CPUs in the local machine according to /proc/cpuinfo"""
    try:
       return multiprocessing.cpu_count()
    except Exception:
       logging.exception('can not get cpu count from'
                        ' multiprocessing.cpu_count()')
    cpuinfo = get_cpuinfo()
    # Returns at least one cpu. Check comment #1 in crosbug.com/p/9582.
    return len(cpuinfo) or 1


def cpu_online_map():
    """
    Check out the available cpu online map
    """
    cpuinfo = get_cpuinfo()
    cpus = []
    for cpu in cpuinfo:
        cpus.append(cpu['processor'])  # grab cpu number
    return cpus


def get_cpu_family():
    cpuinfo = get_cpuinfo()[0]
    return int(cpuinfo['cpu_family'])


def get_cpu_vendor():
    cpuinfo = get_cpuinfo()
    vendors = [cpu['vendor_id'] for cpu in cpuinfo]
    for v in vendors[1:]:
        if v != vendors[0]:
            raise error.TestError('multiple cpu vendors found: ' + str(vendors))
    return vendors[0]


def probe_cpus():
    """
    This routine returns a list of cpu devices found under
    /sys/devices/system/cpu.
    """
    cmd = 'find /sys/devices/system/cpu/ -maxdepth 1 -type d -name cpu*'
    return utils.system_output(cmd).splitlines()


# Returns total memory in kb
def read_from_meminfo(key):
    meminfo = utils.system_output('grep %s /proc/meminfo' % key)
    return int(re.search(r'\d+', meminfo).group(0))


def memtotal():
    return read_from_meminfo('MemTotal')


def freememtotal():
    return read_from_meminfo('MemFree')

def usable_memtotal():
    # Reserved 5% for OS use
    return int(read_from_meminfo('MemFree') * 0.95)

def swaptotal():
    return read_from_meminfo('SwapTotal')

def rounded_memtotal():
    # Get total of all physical mem, in kbytes
    usable_kbytes = memtotal()
    # usable_kbytes is system's usable DRAM in kbytes,
    #   as reported by memtotal() from device /proc/meminfo memtotal
    #   after Linux deducts 1.5% to 5.1% for system table overhead
    # Undo the unknown actual deduction by rounding up
    #   to next small multiple of a big power-of-two
    #   eg  12GB - 5.1% gets rounded back up to 12GB
    mindeduct = 0.015  # 1.5 percent
    maxdeduct = 0.055  # 5.5 percent
    # deduction range 1.5% .. 5.5% supports physical mem sizes
    #    6GB .. 12GB in steps of .5GB
    #   12GB .. 24GB in steps of 1 GB
    #   24GB .. 48GB in steps of 2 GB ...
    # Finer granularity in physical mem sizes would require
    #   tighter spread between min and max possible deductions

    # increase mem size by at least min deduction, without rounding
    min_kbytes = int(usable_kbytes / (1.0 - mindeduct))
    # increase mem size further by 2**n rounding, by 0..roundKb or more
    round_kbytes = int(usable_kbytes / (1.0 - maxdeduct)) - min_kbytes
    # find least binary roundup 2**n that covers worst-cast roundKb
    mod2n = 1 << int(math.ceil(math.log(round_kbytes, 2)))
    # have round_kbytes <= mod2n < round_kbytes*2
    # round min_kbytes up to next multiple of mod2n
    phys_kbytes = min_kbytes + mod2n - 1
    phys_kbytes = phys_kbytes - (phys_kbytes % mod2n)  # clear low bits
    return phys_kbytes


def sysctl(key, value=None):
    """Generic implementation of sysctl, to read and write.

    @param key: A location under /proc/sys
    @param value: If not None, a value to write into the sysctl.

    @return The single-line sysctl value as a string.
    """
    path = '/proc/sys/%s' % key
    if value is not None:
        utils.write_one_line(path, str(value))
    return utils.read_one_line(path)


def sysctl_kernel(key, value=None):
    """(Very) partial implementation of sysctl, for kernel params"""
    if value is not None:
        # write
        utils.write_one_line('/proc/sys/kernel/%s' % key, str(value))
    else:
        # read
        out = utils.read_one_line('/proc/sys/kernel/%s' % key)
        return int(re.search(r'\d+', out).group(0))


def _convert_exit_status(sts):
    if os.WIFSIGNALED(sts):
        return -os.WTERMSIG(sts)
    elif os.WIFEXITED(sts):
        return os.WEXITSTATUS(sts)
    else:
        # impossible?
        raise RuntimeError("Unknown exit status %d!" % sts)


def where_art_thy_filehandles():
    """Dump the current list of filehandles"""
    os.system("ls -l /proc/%d/fd >> /dev/tty" % os.getpid())


def get_num_allocated_file_handles():
    """
    Returns the number of currently allocated file handles.

    Gets this information by parsing /proc/sys/fs/file-nr.
    See https://www.kernel.org/doc/Documentation/sysctl/fs.txt
    for details on this file.
    """
    with _open_file('/proc/sys/fs/file-nr') as f:
        line = f.readline()
    allocated_handles = int(line.split()[0])
    return allocated_handles

def print_to_tty(string):
    """Output string straight to the tty"""
    open('/dev/tty', 'w').write(string + '\n')


def dump_object(object):
    """Dump an object's attributes and methods

    kind of like dir()
    """
    for item in object.__dict__.iteritems():
        print item
        try:
            (key, value) = item
            dump_object(value)
        except:
            continue


def environ(env_key):
    """return the requested environment variable, or '' if unset"""
    if (os.environ.has_key(env_key)):
        return os.environ[env_key]
    else:
        return ''


def prepend_path(newpath, oldpath):
    """prepend newpath to oldpath"""
    if (oldpath):
        return newpath + ':' + oldpath
    else:
        return newpath


def append_path(oldpath, newpath):
    """append newpath to oldpath"""
    if (oldpath):
        return oldpath + ':' + newpath
    else:
        return newpath


_TIME_OUTPUT_RE = re.compile(
        r'([\d\.]*)user ([\d\.]*)system '
        r'(\d*):([\d\.]*)elapsed (\d*)%CPU')


def avgtime_print(dir):
    """ Calculate some benchmarking statistics.
        Input is a directory containing a file called 'time'.
        File contains one-per-line results of /usr/bin/time.
        Output is average Elapsed, User, and System time in seconds,
          and average CPU percentage.
    """
    user = system = elapsed = cpu = count = 0
    with open(dir + "/time") as f:
        for line in f:
            try:
                m = _TIME_OUTPUT_RE.match(line);
                user += float(m.group(1))
                system += float(m.group(2))
                elapsed += (float(m.group(3)) * 60) + float(m.group(4))
                cpu += float(m.group(5))
                count += 1
            except:
                raise ValueError("badly formatted times")

    return "Elapsed: %0.2fs User: %0.2fs System: %0.2fs CPU: %0.0f%%" % \
          (elapsed / count, user / count, system / count, cpu / count)


def to_seconds(time_string):
    """Converts a string in M+:SS.SS format to S+.SS"""
    elts = time_string.split(':')
    if len(elts) == 1:
        return time_string
    return str(int(elts[0]) * 60 + float(elts[1]))


_TIME_OUTPUT_RE_2 = re.compile(r'(.*?)user (.*?)system (.*?)elapsed')


def extract_all_time_results(results_string):
    """Extract user, system, and elapsed times into a list of tuples"""
    results = []
    for result in _TIME_OUTPUT_RE_2.findall(results_string):
        results.append(tuple([to_seconds(elt) for elt in result]))
    return results


def running_config():
    """
    Return path of config file of the currently running kernel
    """
    version = utils.system_output('uname -r')
    for config in ('/proc/config.gz', \
                   '/boot/config-%s' % version,
                   '/lib/modules/%s/build/.config' % version):
        if os.path.isfile(config):
            return config
    return None


def check_for_kernel_feature(feature):
    config = running_config()

    if not config:
        raise TypeError("Can't find kernel config file")

    if magic.guess_type(config) == 'application/x-gzip':
        grep = 'zgrep'
    else:
        grep = 'grep'
    grep += ' ^CONFIG_%s= %s' % (feature, config)

    if not utils.system_output(grep, ignore_status=True):
        raise ValueError("Kernel doesn't have a %s feature" % (feature))


def check_glibc_ver(ver):
    glibc_ver = commands.getoutput('ldd --version').splitlines()[0]
    glibc_ver = re.search(r'(\d+\.\d+(\.\d+)?)', glibc_ver).group()
    if utils.compare_versions(glibc_ver, ver) == -1:
        raise error.TestError("Glibc too old (%s). Glibc >= %s is needed." %
                              (glibc_ver, ver))

def check_kernel_ver(ver):
    kernel_ver = utils.system_output('uname -r')
    kv_tmp = re.split(r'[-]', kernel_ver)[0:3]
    # In compare_versions, if v1 < v2, return value == -1
    if utils.compare_versions(kv_tmp[0], ver) == -1:
        raise error.TestError("Kernel too old (%s). Kernel > %s is needed." %
                              (kernel_ver, ver))


def human_format(number):
    # Convert number to kilo / mega / giga format.
    if number < 1024:
        return "%d" % number
    kilo = float(number) / 1024.0
    if kilo < 1024:
        return "%.2fk" % kilo
    meg = kilo / 1024.0
    if meg < 1024:
        return "%.2fM" % meg
    gig = meg / 1024.0
    return "%.2fG" % gig


def numa_nodes():
    node_paths = glob.glob('/sys/devices/system/node/node*')
    nodes = [int(re.sub(r'.*node(\d+)', r'\1', x)) for x in node_paths]
    return (sorted(nodes))


def node_size():
    nodes = max(len(numa_nodes()), 1)
    return ((memtotal() * 1024) / nodes)


def pickle_load(filename):
    return pickle.load(open(filename, 'r'))


# Return the kernel version and build timestamp.
def running_os_release():
    return os.uname()[2:4]


def running_os_ident():
    (version, timestamp) = running_os_release()
    return version + '::' + timestamp


def running_os_full_version():
    (version, timestamp) = running_os_release()
    return version


# much like find . -name 'pattern'
def locate(pattern, root=os.getcwd()):
    for path, dirs, files in os.walk(root):
        for f in files:
            if fnmatch.fnmatch(f, pattern):
                yield os.path.abspath(os.path.join(path, f))


def freespace(path):
    """Return the disk free space, in bytes"""
    s = os.statvfs(path)
    return s.f_bavail * s.f_bsize


def disk_block_size(path):
    """Return the disk block size, in bytes"""
    return os.statvfs(path).f_bsize


_DISK_PARTITION_3_RE = re.compile(r'^(/dev/hd[a-z]+)3', re.M)

def get_disks():
    df_output = utils.system_output('df')
    return _DISK_PARTITION_3_RE.findall(df_output)


def get_disk_size(disk_name):
    """
    Return size of disk in byte. Return 0 in Error Case

    @param disk_name: disk name to find size
    """
    device = os.path.basename(disk_name)
    for line in file('/proc/partitions'):
        try:
            _, _, blocks, name = re.split(r' +', line.strip())
        except ValueError:
            continue
        if name == device:
            return 1024 * int(blocks)
    return 0


def get_disk_size_gb(disk_name):
    """
    Return size of disk in GB (10^9). Return 0 in Error Case

    @param disk_name: disk name to find size
    """
    return int(get_disk_size(disk_name) / (10.0 ** 9) + 0.5)


def get_disk_model(disk_name):
    """
    Return model name for internal storage device

    @param disk_name: disk name to find model
    """
    cmd1 = 'udevadm info --query=property --name=%s' % disk_name
    cmd2 = 'grep -E "ID_(NAME|MODEL)="'
    cmd3 = 'cut -f 2 -d"="'
    cmd = ' | '.join([cmd1, cmd2, cmd3])
    return utils.system_output(cmd)


_DISK_DEV_RE = re.compile(r'/dev/sd[a-z]|'
                          r'/dev/mmcblk[0-9]+|'
                          r'/dev/nvme[0-9]+n[0-9]+')


def get_disk_from_filename(filename):
    """
    Return the disk device the filename is on.
    If the file is on tmpfs or other special file systems,
    return None.

    @param filename: name of file, full path.
    """

    if not os.path.exists(filename):
        raise error.TestError('file %s missing' % filename)

    if filename[0] != '/':
        raise error.TestError('This code works only with full path')

    m = _DISK_DEV_RE.match(filename)
    while not m:
        if filename[0] != '/':
            return None
        if filename == '/dev/root':
            cmd = 'rootdev -d -s'
        elif filename.startswith('/dev/mapper'):
            cmd = 'dmsetup table "%s"' % os.path.basename(filename)
            dmsetup_output = utils.system_output(cmd).split(' ')
            if dmsetup_output[2] == 'verity':
                maj_min = dmsetup_output[4]
            elif dmsetup_output[2] == 'crypt':
                maj_min = dmsetup_output[6]
            cmd = 'realpath "/dev/block/%s"' % maj_min
        elif filename.startswith('/dev/loop'):
            cmd = 'losetup -O BACK-FILE "%s" | tail -1' % filename
        else:
            cmd = 'df "%s" | tail -1 | cut -f 1 -d" "' % filename
        filename = utils.system_output(cmd)
        m = _DISK_DEV_RE.match(filename)
    return m.group(0)


def get_disk_firmware_version(disk_name):
    """
    Return firmware version for internal storage device. (empty string for eMMC)

    @param disk_name: disk name to find model
    """
    cmd1 = 'udevadm info --query=property --name=%s' % disk_name
    cmd2 = 'grep -E "ID_REVISION="'
    cmd3 = 'cut -f 2 -d"="'
    cmd = ' | '.join([cmd1, cmd2, cmd3])
    return utils.system_output(cmd)


def is_disk_scsi(disk_name):
    """
    Return true if disk is a scsi device, return false otherwise

    @param disk_name: disk name check
    """
    return re.match('/dev/sd[a-z]+', disk_name)


def is_disk_harddisk(disk_name):
    """
    Return true if disk is a harddisk, return false otherwise

    @param disk_name: disk name check
    """
    cmd1 = 'udevadm info --query=property --name=%s' % disk_name
    cmd2 = 'grep -E "ID_ATA_ROTATION_RATE_RPM="'
    cmd3 = 'cut -f 2 -d"="'
    cmd = ' | '.join([cmd1, cmd2, cmd3])

    rtt = utils.system_output(cmd)

    # eMMC will not have this field; rtt == ''
    # SSD will have zero rotation rate; rtt == '0'
    # For harddisk rtt > 0
    return rtt and int(rtt) > 0

def concat_partition(disk_name, partition_number):
    """
    Return the name of a partition:
    sda, 3 --> sda3
    mmcblk0, 3 --> mmcblk0p3

    @param disk_name: diskname string
    @param partition_number: integer
    """
    if disk_name.endswith(tuple(str(i) for i in range(0, 10))):
        sep = 'p'
    else:
        sep = ''
    return disk_name + sep + str(partition_number)

def verify_hdparm_feature(disk_name, feature):
    """
    Check for feature support for SCSI disk using hdparm

    @param disk_name: target disk
    @param feature: hdparm output string of the feature
    """
    cmd = 'hdparm -I %s | grep -q "%s"' % (disk_name, feature)
    ret = utils.system(cmd, ignore_status=True)
    if ret == 0:
        return True
    elif ret == 1:
        return False
    else:
        raise error.TestFail('Error running command %s' % cmd)


def get_storage_error_msg(disk_name, reason):
    """
    Get Error message for storage test which include disk model.
    and also include the firmware version for the SCSI disk

    @param disk_name: target disk
    @param reason: Reason of the error.
    """

    msg = reason

    model = get_disk_model(disk_name)
    msg += ' Disk model: %s' % model

    if is_disk_scsi(disk_name):
        fw = get_disk_firmware_version(disk_name)
        msg += ' firmware: %s' % fw

    return msg


def load_module(module_name, params=None):
    # Checks if a module has already been loaded
    if module_is_loaded(module_name):
        return False

    cmd = '/sbin/modprobe ' + module_name
    if params:
        cmd += ' ' + params
    utils.system(cmd)
    return True


def unload_module(module_name):
    """
    Removes a module. Handles dependencies. If even then it's not possible
    to remove one of the modules, it will trhow an error.CmdError exception.

    @param module_name: Name of the module we want to remove.
    """
    l_raw = utils.system_output("/bin/lsmod").splitlines()
    lsmod = [x for x in l_raw if x.split()[0] == module_name]
    if len(lsmod) > 0:
        line_parts = lsmod[0].split()
        if len(line_parts) == 4:
            submodules = line_parts[3].split(",")
            for submodule in submodules:
                unload_module(submodule)
        utils.system("/sbin/modprobe -r %s" % module_name)
        logging.info("Module %s unloaded", module_name)
    else:
        logging.info("Module %s is already unloaded", module_name)


def module_is_loaded(module_name):
    module_name = module_name.replace('-', '_')
    modules = utils.system_output('/bin/lsmod').splitlines()
    for module in modules:
        if module.startswith(module_name) and module[len(module_name)] == ' ':
            return True
    return False


def get_loaded_modules():
    lsmod_output = utils.system_output('/bin/lsmod').splitlines()[1:]
    return [line.split(None, 1)[0] for line in lsmod_output]


def get_huge_page_size():
    output = utils.system_output('grep Hugepagesize /proc/meminfo')
    return int(output.split()[1]) # Assumes units always in kB. :(


def get_num_huge_pages():
    raw_hugepages = utils.system_output('/sbin/sysctl vm.nr_hugepages')
    return int(raw_hugepages.split()[2])


def set_num_huge_pages(num):
    utils.system('/sbin/sysctl vm.nr_hugepages=%d' % num)


def ping_default_gateway():
    """Ping the default gateway."""

    network = open('/etc/sysconfig/network')
    m = re.search('GATEWAY=(\S+)', network.read())

    if m:
        gw = m.group(1)
        cmd = 'ping %s -c 5 > /dev/null' % gw
        return utils.system(cmd, ignore_status=True)

    raise error.TestError('Unable to find default gateway')


def drop_caches():
    """Writes back all dirty pages to disk and clears all the caches."""
    utils.system("sync")
    # We ignore failures here as this will fail on 2.6.11 kernels.
    utils.system("echo 3 > /proc/sys/vm/drop_caches", ignore_status=True)


def process_is_alive(name_pattern):
    """
    'pgrep name' misses all python processes and also long process names.
    'pgrep -f name' gets all shell commands with name in args.
    So look only for command whose initial pathname ends with name.
    Name itself is an egrep pattern, so it can use | etc for variations.
    """
    return utils.system("pgrep -f '^([^ /]*/)*(%s)([ ]|$)'" % name_pattern,
                        ignore_status=True) == 0


def get_hwclock_seconds(utc=True):
    """
    Return the hardware clock in seconds as a floating point value.
    Use Coordinated Universal Time if utc is True, local time otherwise.
    Raise a ValueError if unable to read the hardware clock.
    """
    cmd = '/sbin/hwclock --debug'
    if utc:
        cmd += ' --utc'
    hwclock_output = utils.system_output(cmd, ignore_status=True)
    match = re.search(r'= ([0-9]+) seconds since .+ (-?[0-9.]+) seconds$',
                      hwclock_output, re.DOTALL)
    if match:
        seconds = int(match.group(1)) + float(match.group(2))
        logging.debug('hwclock seconds = %f', seconds)
        return seconds

    raise ValueError('Unable to read the hardware clock -- ' +
                     hwclock_output)


def set_wake_alarm(alarm_time):
    """
    Set the hardware RTC-based wake alarm to 'alarm_time'.
    """
    utils.write_one_line('/sys/class/rtc/rtc0/wakealarm', str(alarm_time))


def set_power_state(state):
    """
    Set the system power state to 'state'.
    """
    utils.write_one_line('/sys/power/state', state)


def standby():
    """
    Power-on suspend (S1)
    """
    set_power_state('standby')


def suspend_to_ram():
    """
    Suspend the system to RAM (S3)
    """
    set_power_state('mem')


def suspend_to_disk():
    """
    Suspend the system to disk (S4)
    """
    set_power_state('disk')


_AUTOTEST_CLIENT_PATH = os.path.join(os.path.dirname(__file__), '..')
_AMD_PCI_IDS_FILE_PATH = os.path.join(_AUTOTEST_CLIENT_PATH,
                                      'bin/amd_pci_ids.json')
_INTEL_PCI_IDS_FILE_PATH = os.path.join(_AUTOTEST_CLIENT_PATH,
                                        'bin/intel_pci_ids.json')
_UI_USE_FLAGS_FILE_PATH = '/etc/ui_use_flags.txt'

# Command to check if a package is installed. If the package is not installed
# the command shall fail.
_CHECK_PACKAGE_INSTALLED_COMMAND =(
        "dpkg-query -W -f='${Status}\n' %s | head -n1 | awk '{print $3;}' | "
        "grep -q '^installed$'")

pciid_to_amd_architecture = {}
pciid_to_intel_architecture = {}

class Crossystem(object):
    """A wrapper for the crossystem utility."""

    def __init__(self, client):
        self.cros_system_data = {}
        self._client = client

    def init(self):
        self.cros_system_data = {}
        (_, fname) = tempfile.mkstemp()
        f = open(fname, 'w')
        self._client.run('crossystem', stdout_tee=f)
        f.close()
        text = utils.read_file(fname)
        for line in text.splitlines():
            assignment_string = line.split('#')[0]
            if not assignment_string.count('='):
                continue
            (name, value) = assignment_string.split('=', 1)
            self.cros_system_data[name.strip()] = value.strip()
        os.remove(fname)

    def __getattr__(self, name):
        """
        Retrieve a crosssystem attribute.

        The call crossystemobject.name() will return the crossystem reported
        string.
        """
        return lambda: self.cros_system_data[name]


def get_oldest_pid_by_name(name):
    """
    Return the oldest pid of a process whose name perfectly matches |name|.

    name is an egrep expression, which will be matched against the entire name
    of processes on the system.  For example:

      get_oldest_pid_by_name('chrome')

    on a system running
      8600 ?        00:00:04 chrome
      8601 ?        00:00:00 chrome
      8602 ?        00:00:00 chrome-sandbox

    would return 8600, as that's the oldest process that matches.
    chrome-sandbox would not be matched.

    Arguments:
      name: egrep expression to match.  Will be anchored at the beginning and
            end of the match string.

    Returns:
      pid as an integer, or None if one cannot be found.

    Raises:
      ValueError if pgrep returns something odd.
    """
    str_pid = utils.system_output('pgrep -o ^%s$' % name,
                                  ignore_status=True).rstrip()
    if str_pid:
        return int(str_pid)


def get_oldest_by_name(name):
    """Return pid and command line of oldest process whose name matches |name|.

    @param name: egrep expression to match desired process name.
    @return: A tuple of (pid, command_line) of the oldest process whose name
             matches |name|.

    """
    pid = get_oldest_pid_by_name(name)
    if pid:
        command_line = utils.system_output('ps -p %i -o command=' % pid,
                                           ignore_status=True).rstrip()
        return (pid, command_line)


def get_chrome_remote_debugging_port():
    """Returns remote debugging port for Chrome.

    Parse chrome process's command line argument to get the remote debugging
    port. if it is 0, look at DevToolsActivePort for the ephemeral port.
    """
    _, command = get_oldest_by_name('chrome')
    matches = re.search('--remote-debugging-port=([0-9]+)', command)
    if not matches:
      return 0
    port = int(matches.group(1))
    if port:
      return port
    with open('/home/chronos/DevToolsActivePort') as f:
      return int(f.readline().rstrip())


def get_process_list(name, command_line=None):
    """
    Return the list of pid for matching process |name command_line|.

    on a system running
      31475 ?    0:06 /opt/google/chrome/chrome --allow-webui-compositing -
      31478 ?    0:00 /opt/google/chrome/chrome-sandbox /opt/google/chrome/
      31485 ?    0:00 /opt/google/chrome/chrome --type=zygote --log-level=1
      31532 ?    1:05 /opt/google/chrome/chrome --type=renderer

    get_process_list('chrome')
    would return ['31475', '31485', '31532']

    get_process_list('chrome', '--type=renderer')
    would return ['31532']

    Arguments:
      name: process name to search for. If command_line is provided, name is
            matched against full command line. If command_line is not provided,
            name is only matched against the process name.
      command line: when command line is passed, the full process command line
                    is used for matching.

    Returns:
      list of PIDs of the matching processes.

    """
    # TODO(rohitbm) crbug.com/268861
    flag = '-x' if not command_line else '-f'
    name = '\'%s.*%s\'' % (name, command_line) if command_line else name
    str_pid = utils.system_output('pgrep %s %s' % (flag, name),
                                  ignore_status=True).rstrip()
    return str_pid.split()


def nuke_process_by_name(name, with_prejudice=False):
    """Tell the oldest process specified by name to exit.

    Arguments:
      name: process name specifier, as understood by pgrep.
      with_prejudice: if True, don't allow for graceful exit.

    Raises:
      error.AutoservPidAlreadyDeadError: no existing process matches name.
    """
    try:
        pid = get_oldest_pid_by_name(name)
    except Exception as e:
        logging.error(e)
        return
    if pid is None:
        raise error.AutoservPidAlreadyDeadError('No process matching %s.' %
                                                name)
    if with_prejudice:
        utils.nuke_pid(pid, [signal.SIGKILL])
    else:
        utils.nuke_pid(pid)


def ensure_processes_are_dead_by_name(name, timeout_sec=10):
    """Terminate all processes specified by name and ensure they're gone.

    Arguments:
      name: process name specifier, as understood by pgrep.
      timeout_sec: maximum number of seconds to wait for processes to die.

    Raises:
      error.AutoservPidAlreadyDeadError: no existing process matches name.
      utils.TimeoutError: if processes still exist after timeout_sec.
    """

    def list_and_kill_processes(name):
        process_list = get_process_list(name)
        try:
            for pid in [int(str_pid) for str_pid in process_list]:
                utils.nuke_pid(pid)
        except error.AutoservPidAlreadyDeadError:
            pass
        return process_list

    utils.poll_for_condition(lambda: list_and_kill_processes(name) == [],
                             timeout=timeout_sec)


def is_virtual_machine():
    return 'QEMU' in platform.processor()


def save_vm_state(checkpoint):
    """Saves the current state of the virtual machine.

    This function is a NOOP if the test is not running under a virtual machine
    with the USB serial port redirected.

    Arguments:
      checkpoint - Name used to identify this state

    Returns:
      None
    """
    # The QEMU monitor has been redirected to the guest serial port located at
    # /dev/ttyUSB0. To save the state of the VM, we just send the 'savevm'
    # command to the serial port.
    if is_virtual_machine() and os.path.exists('/dev/ttyUSB0'):
        logging.info('Saving VM state "%s"', checkpoint)
        serial = open('/dev/ttyUSB0', 'w')
        serial.write('savevm %s\r\n' % checkpoint)
        logging.info('Done saving VM state "%s"', checkpoint)


def check_raw_dmesg(dmesg, message_level, whitelist):
    """Checks dmesg for unexpected warnings.

    This function parses dmesg for message with message_level <= message_level
    which do not appear in the whitelist.

    Arguments:
      dmesg - string containing raw dmesg buffer
      message_level - minimum message priority to check
      whitelist - messages to ignore

    Returns:
      List of unexpected warnings
    """
    whitelist_re = re.compile(r'(%s)' % '|'.join(whitelist))
    unexpected = []
    for line in dmesg.splitlines():
        if int(line[1]) <= message_level:
            stripped_line = line.split('] ', 1)[1]
            if whitelist_re.search(stripped_line):
                continue
            unexpected.append(stripped_line)
    return unexpected


def verify_mesg_set(mesg, regex, whitelist):
    """Verifies that the exact set of messages are present in a text.

    This function finds all strings in the text matching a certain regex, and
    then verifies that all expected strings are present in the set, and no
    unexpected strings are there.

    Arguments:
      mesg - the mutiline text to be scanned
      regex - regular expression to match
      whitelist - messages to find in the output, a list of strings
          (potentially regexes) to look for in the filtered output. All these
          strings must be there, and no other strings should be present in the
          filtered output.

    Returns:
      string of inconsistent findings (i.e. an empty string on success).
    """

    rv = []

    missing_strings = []
    present_strings = []
    for line in mesg.splitlines():
        if not re.search(r'%s' % regex, line):
            continue
        present_strings.append(line.split('] ', 1)[1])

    for string in whitelist:
        for present_string in list(present_strings):
            if re.search(r'^%s$' % string, present_string):
                present_strings.remove(present_string)
                break
        else:
            missing_strings.append(string)

    if present_strings:
        rv.append('unexpected strings:')
        rv.extend(present_strings)
    if missing_strings:
        rv.append('missing strings:')
        rv.extend(missing_strings)

    return '\n'.join(rv)


def target_is_pie():
    """Returns whether the toolchain produces a PIE (position independent
    executable) by default.

    Arguments:
      None

    Returns:
      True if the target toolchain produces a PIE by default.
      False otherwise.
    """

    command = 'echo | ${CC} -E -dD -P - | grep -i pie'
    result = utils.system_output(command,
                                 retain_output=True,
                                 ignore_status=True)
    if re.search('#define __PIE__', result):
        return True
    else:
        return False


def target_is_x86():
    """Returns whether the toolchain produces an x86 object

    Arguments:
      None

    Returns:
      True if the target toolchain produces an x86 object
      False otherwise.
    """

    command = 'echo | ${CC} -E -dD -P - | grep -i 86'
    result = utils.system_output(command,
                                 retain_output=True,
                                 ignore_status=True)
    if re.search('__i386__', result) or re.search('__x86_64__', result):
        return True
    else:
        return False


def mounts():
    ret = []
    for line in file('/proc/mounts'):
        m = re.match(
            r'(?P<src>\S+) (?P<dest>\S+) (?P<type>\S+) (?P<opts>\S+).*', line)
        if m:
            ret.append(m.groupdict())
    return ret


def is_mountpoint(path):
    return path in [m['dest'] for m in mounts()]


def require_mountpoint(path):
    """
    Raises an exception if path is not a mountpoint.
    """
    if not is_mountpoint(path):
        raise error.TestFail('Path not mounted: "%s"' % path)


def random_username():
    return str(uuid.uuid4()) + '@example.com'


def get_signin_credentials(filepath):
    """Returns user_id, password tuple from credentials file at filepath.

    File must have one line of the format user_id:password

    @param filepath: path of credentials file.
    @return user_id, password tuple.
    """
    user_id, password = None, None
    if os.path.isfile(filepath):
        with open(filepath) as f:
            user_id, password = f.read().rstrip().split(':')
    return user_id, password


def parse_cmd_output(command, run_method=utils.run):
    """Runs a command on a host object to retrieve host attributes.

    The command should output to stdout in the format of:
    <key> = <value> # <optional_comment>


    @param command: Command to execute on the host.
    @param run_method: Function to use to execute the command. Defaults to
                       utils.run so that the command will be executed locally.
                       Can be replace with a host.run call so that it will
                       execute on a DUT or external machine. Method must accept
                       a command argument, stdout_tee and stderr_tee args and
                       return a result object with a string attribute stdout
                       which will be parsed.

    @returns a dictionary mapping host attributes to their values.
    """
    result = {}
    # Suppresses stdout so that the files are not printed to the logs.
    cmd_result = run_method(command, stdout_tee=None, stderr_tee=None)
    for line in cmd_result.stdout.splitlines():
        # Lines are of the format "<key>     = <value>      # <comment>"
        key_value = re.match(r'^\s*(?P<key>[^ ]+)\s*=\s*(?P<value>[^ '
                             r']+)(?:\s*#.*)?$', line)
        if key_value:
            result[key_value.group('key')] = key_value.group('value')
    return result


def set_from_keyval_output(out, delimiter=' '):
    """Parse delimiter-separated key-val output into a set of tuples.

    Output is expected to be multiline text output from a command.
    Stuffs the key-vals into tuples in a set to be later compared.

    e.g.  deactivated 0
          disableForceClear 0
          ==>  set(('deactivated', '0'), ('disableForceClear', '0'))

    @param out: multiple lines of space-separated key-val pairs.
    @param delimiter: character that separates key from val. Usually a
                      space but may be '=' or something else.
    @return set of key-val tuples.
    """
    results = set()
    kv_match_re = re.compile('([^ ]+)%s(.*)' % delimiter)
    for linecr in out.splitlines():
        match = kv_match_re.match(linecr.strip())
        if match:
            results.add((match.group(1), match.group(2)))
    return results


def get_cpu_usage():
    """Returns machine's CPU usage.

    This function uses /proc/stat to identify CPU usage.
    Returns:
        A dictionary with values for all columns in /proc/stat
        Sample dictionary:
        {
            'user': 254544,
            'nice': 9,
            'system': 254768,
            'idle': 2859878,
            'iowait': 1,
            'irq': 2,
            'softirq': 3,
            'steal': 4,
            'guest': 5,
            'guest_nice': 6
        }
        If a column is missing or malformed in /proc/stat (typically on older
        systems), the value for that column is set to 0.
    """
    with _open_file('/proc/stat') as proc_stat:
        cpu_usage_str = proc_stat.readline().split()
    columns = ('user', 'nice', 'system', 'idle', 'iowait', 'irq', 'softirq',
               'steal', 'guest', 'guest_nice')
    d = {}
    for index, col in enumerate(columns, 1):
        try:
            d[col] = int(cpu_usage_str[index])
        except:
            d[col] = 0
    return d

def compute_active_cpu_time(cpu_usage_start, cpu_usage_end):
    """Computes the fraction of CPU time spent non-idling.

    This function should be invoked using before/after values from calls to
    get_cpu_usage().

    See https://stackoverflow.com/a/23376195 and
    https://unix.stackexchange.com/a/303224 for some more context how
    to calculate usage given two /proc/stat snapshots.
    """
    idle_cols = ('idle', 'iowait')  # All other cols are calculated as active.
    time_active_start = sum([x[1] for x in cpu_usage_start.iteritems()
                             if x[0] not in idle_cols])
    time_active_end = sum([x[1] for x in cpu_usage_end.iteritems()
                           if x[0] not in idle_cols])
    total_time_start = sum(cpu_usage_start.values())
    total_time_end = sum(cpu_usage_end.values())
    # Avoid bogus division which has been observed on Tegra.
    if total_time_end <= total_time_start:
        logging.warning('compute_active_cpu_time observed bogus data')
        # We pretend to be busy, this will force a longer wait for idle CPU.
        return 1.0
    return ((float(time_active_end) - time_active_start) /
            (total_time_end - total_time_start))


def is_pgo_mode():
    return 'USE_PGO' in os.environ


def wait_for_idle_cpu(timeout, utilization):
    """Waits for the CPU to become idle (< utilization).

    Args:
        timeout: The longest time in seconds to wait before throwing an error.
        utilization: The CPU usage below which the system should be considered
                idle (between 0 and 1.0 independent of cores/hyperthreads).
    """
    time_passed = 0.0
    fraction_active_time = 1.0
    sleep_time = 1
    logging.info('Starting to wait up to %.1fs for idle CPU...', timeout)
    while fraction_active_time >= utilization:
        cpu_usage_start = get_cpu_usage()
        # Split timeout interval into not too many chunks to limit log spew.
        # Start at 1 second, increase exponentially
        time.sleep(sleep_time)
        time_passed += sleep_time
        sleep_time = min(16.0, 2.0 * sleep_time)
        cpu_usage_end = get_cpu_usage()
        fraction_active_time = compute_active_cpu_time(cpu_usage_start,
                                                       cpu_usage_end)
        logging.info('After waiting %.1fs CPU utilization is %.3f.',
                     time_passed, fraction_active_time)
        if time_passed > timeout:
            logging.warning('CPU did not become idle.')
            log_process_activity()
            # crosbug.com/37389
            if is_pgo_mode():
                logging.info('Still continuing because we are in PGO mode.')
                return True

            return False
    logging.info('Wait for idle CPU took %.1fs (utilization = %.3f).',
                 time_passed, fraction_active_time)
    return True


def log_process_activity():
    """Logs the output of top.

    Useful to debug performance tests and to find runaway processes.
    """
    logging.info('Logging current process activity using top and ps.')
    cmd = 'top -b -n1 -c'
    output = utils.run(cmd)
    logging.info(output)
    output = utils.run('ps axl')
    logging.info(output)


def wait_for_cool_machine():
    """
    A simple heuristic to wait for a machine to cool.
    The code looks a bit 'magic', but we don't know ambient temperature
    nor machine characteristics and still would like to return the caller
    a machine that cooled down as much as reasonably possible.
    """
    temperature = get_current_temperature_max()
    # We got here with a cold machine, return immediately. This should be the
    # most common case.
    if temperature < 50:
        return True
    logging.info('Got a hot machine of %dC. Sleeping 1 minute.', temperature)
    # A modest wait should cool the machine.
    time.sleep(60.0)
    temperature = get_current_temperature_max()
    # Atoms idle below 60 and everyone else should be even lower.
    if temperature < 62:
        return True
    # This should be rare.
    logging.info('Did not cool down (%dC). Sleeping 2 minutes.', temperature)
    time.sleep(120.0)
    temperature = get_current_temperature_max()
    # A temperature over 65'C doesn't give us much headroom to the critical
    # temperatures that start at 85'C (and PerfControl as of today will fail at
    # critical - 10'C).
    if temperature < 65:
        return True
    logging.warning('Did not cool down (%dC), giving up.', temperature)
    log_process_activity()
    return False


# System paths for machine performance state.
_CPUINFO = '/proc/cpuinfo'
_DIRTY_WRITEBACK_CENTISECS = '/proc/sys/vm/dirty_writeback_centisecs'
_KERNEL_MAX = '/sys/devices/system/cpu/kernel_max'
_MEMINFO = '/proc/meminfo'
_TEMP_SENSOR_RE = 'Reading temperature...([0-9]*)'

def _open_file(path):
    """
    Opens a file and returns the file object.

    This method is intended to be mocked by tests.
    @return The open file object.
    """
    return open(path)

def _get_line_from_file(path, line):
    """
    line can be an integer or
    line can be a string that matches the beginning of the line
    """
    with _open_file(path) as f:
        if isinstance(line, int):
            l = f.readline()
            for _ in range(0, line):
                l = f.readline()
            return l
        else:
            for l in f:
                if l.startswith(line):
                    return l
    return None


def _get_match_from_file(path, line, prefix, postfix):
    """
    Matches line in path and returns string between first prefix and postfix.
    """
    match = _get_line_from_file(path, line)
    # Strip everything from front of line including prefix.
    if prefix:
        match = re.split(prefix, match)[1]
    # Strip everything from back of string including first occurence of postfix.
    if postfix:
        match = re.split(postfix, match)[0]
    return match


def _get_float_from_file(path, line, prefix, postfix):
    match = _get_match_from_file(path, line, prefix, postfix)
    return float(match)


def _get_int_from_file(path, line, prefix, postfix):
    match = _get_match_from_file(path, line, prefix, postfix)
    return int(match)


def _get_hex_from_file(path, line, prefix, postfix):
    match = _get_match_from_file(path, line, prefix, postfix)
    return int(match, 16)


# The paths don't change. Avoid running find all the time.
_hwmon_paths = None

def _get_hwmon_paths(file_pattern):
    """
    Returns a list of paths to the temperature sensors.
    """
    # Some systems like daisy_spring only have the virtual hwmon.
    # And other systems like rambi only have coretemp.0. See crbug.com/360249.
    #    /sys/class/hwmon/hwmon*/
    #    /sys/devices/virtual/hwmon/hwmon*/
    #    /sys/devices/platform/coretemp.0/
    if not _hwmon_paths:
        cmd = 'find /sys/ -name "' + file_pattern + '"'
        _hwon_paths = utils.run(cmd, verbose=False).stdout.splitlines()
    return _hwon_paths


def get_temperature_critical():
    """
    Returns temperature at which we will see some throttling in the system.
    """
    min_temperature = 1000.0
    paths = _get_hwmon_paths('temp*_crit')
    for path in paths:
        temperature = _get_float_from_file(path, 0, None, None) * 0.001
        # Today typical for Intel is 98'C to 105'C while ARM is 85'C. Clamp to
        # the lowest known value.
        if (min_temperature < 60.0) or min_temperature > 150.0:
            logging.warning('Critical temperature of %.1fC was reset to 85.0C.',
                            min_temperature)
            min_temperature = 85.0

        min_temperature = min(temperature, min_temperature)
    return min_temperature


def get_temperature_input_max():
    """
    Returns the maximum currently observed temperature.
    """
    max_temperature = -1000.0
    paths = _get_hwmon_paths('temp*_input')
    for path in paths:
        temperature = _get_float_from_file(path, 0, None, None) * 0.001
        max_temperature = max(temperature, max_temperature)
    return max_temperature


def get_thermal_zone_temperatures():
    """
    Returns the maximum currently observered temperature in thermal_zones.
    """
    temperatures = []
    for path in glob.glob('/sys/class/thermal/thermal_zone*/temp'):
        try:
            temperatures.append(
                _get_float_from_file(path, 0, None, None) * 0.001)
        except IOError:
            # Some devices (e.g. Veyron) may have reserved thermal zones that
            # are not active. Trying to read the temperature value would cause a
            # EINVAL IO error.
            continue
    return temperatures


def get_ec_temperatures():
    """
    Uses ectool to return a list of all sensor temperatures in Celsius.

    Output from ectool is either '0: 300' or '0: 300 K' (newer ectool
    includes the unit).
    """
    temperatures = []
    try:
        full_cmd = 'ectool temps all'
        lines = utils.run(full_cmd, verbose=False).stdout.splitlines()
        pattern = re.compile('.*: (\d+)')
        for line in lines:
            matched = pattern.match(line)
            temperature = int(matched.group(1)) - 273
            temperatures.append(temperature)
    except Exception:
        logging.warning('Unable to read temperature sensors using ectool.')
    for temperature in temperatures:
        # Sanity check for real world values.
        assert ((temperature > 10.0) and
                (temperature < 150.0)), ('Unreasonable temperature %.1fC.' %
                                         temperature)

    return temperatures


def get_current_temperature_max():
    """
    Returns the highest reported board temperature (all sensors) in Celsius.
    """
    temperature = max([get_temperature_input_max()] +
                      get_thermal_zone_temperatures() +
                      get_ec_temperatures())
    # Sanity check for real world values.
    assert ((temperature > 10.0) and
            (temperature < 150.0)), ('Unreasonable temperature %.1fC.' %
                                     temperature)
    return temperature


def get_cpu_cache_size():
    """
    Returns the last level CPU cache size in kBytes.
    """
    cache_size = _get_int_from_file(_CPUINFO, 'cache size', ': ', ' KB')
    # Sanity check.
    assert cache_size >= 64, 'Unreasonably small cache.'
    return cache_size


def get_cpu_model_frequency():
    """
    Returns the model frequency from the CPU model name on Intel only. This
    might be redundant with get_cpu_max_frequency. Unit is Hz.
    """
    frequency = _get_float_from_file(_CPUINFO, 'model name', ' @ ', 'GHz')
    return 1.e9 * frequency


def get_cpu_max_frequency():
    """
    Returns the largest of the max CPU core frequencies. The unit is Hz.
    """
    max_frequency = -1
    paths = _get_cpufreq_paths('cpuinfo_max_freq')
    for path in paths:
        # Convert from kHz to Hz.
        frequency = 1000 * _get_float_from_file(path, 0, None, None)
        max_frequency = max(frequency, max_frequency)
    # Sanity check.
    assert max_frequency > 1e8, 'Unreasonably low CPU frequency.'
    return max_frequency


def get_cpu_min_frequency():
    """
    Returns the smallest of the minimum CPU core frequencies.
    """
    min_frequency = 1e20
    paths = _get_cpufreq_paths('cpuinfo_min_freq')
    for path in paths:
        frequency = _get_float_from_file(path, 0, None, None)
        min_frequency = min(frequency, min_frequency)
    # Sanity check.
    assert min_frequency > 1e8, 'Unreasonably low CPU frequency.'
    return min_frequency


def get_cpu_model():
    """
    Returns the CPU model.
    Only works on Intel.
    """
    cpu_model = _get_int_from_file(_CPUINFO, 'model\t', ': ', None)
    return cpu_model


def get_cpu_family():
    """
    Returns the CPU family.
    Only works on Intel.
    """
    cpu_family = _get_int_from_file(_CPUINFO, 'cpu family\t', ': ', None)
    return cpu_family


def get_board_property(key):
    """
    Get a specific property from /etc/lsb-release.

    @param key: board property to return value for

    @return the value or '' if not present
    """
    with open('/etc/lsb-release') as f:
        pattern = '%s=(.*)' % key
        pat = re.search(pattern, f.read())
        if pat:
            return pat.group(1)
    return ''


def get_board():
    """
    Get the ChromeOS release board name from /etc/lsb-release.
    """
    return get_board_property('BOARD')


def get_board_type():
    """
    Get the ChromeOS board type from /etc/lsb-release.

    @return device type.
    """
    return get_board_property('DEVICETYPE')


def get_ec_version():
    """Get the ec version as strings.

    @returns a string representing this host's ec version.
    """
    command = 'mosys ec info -s fw_version'
    result = utils.run(command, ignore_status=True)
    if result.exit_status != 0:
        return ''
    return result.stdout.strip()


def get_firmware_version():
    """Get the firmware version as strings.

    @returns a string representing this host's firmware version.
    """
    return utils.run('crossystem fwid').stdout.strip()


def get_hardware_revision():
    """Get the hardware revision as strings.

    @returns a string representing this host's hardware revision.
    """
    command = 'mosys platform version'
    result = utils.run(command, ignore_status=True)
    if result.exit_status != 0:
        return ''
    return result.stdout.strip()


def get_kernel_version():
    """Get the kernel version as strings.

    @returns a string representing this host's kernel version.
    """
    return utils.run('uname -r').stdout.strip()


def get_cpu_name():
    """Get the cpu name as strings.

    @returns a string representing this host's cpu name.
    """

    # Try get cpu name from device tree first
    if os.path.exists("/proc/device-tree/compatible"):
        command = "sed -e 's/\\x0/\\n/g' /proc/device-tree/compatible | tail -1"
        return utils.run(command).stdout.strip().replace(',', ' ')


    # Get cpu name from uname -p
    command = "uname -p"
    ret = utils.run(command).stdout.strip()

    # 'uname -p' return variant of unknown or amd64 or x86_64 or i686
    # Try get cpu name from /proc/cpuinfo instead
    if re.match("unknown|amd64|[ix][0-9]?86(_64)?", ret, re.IGNORECASE):
        command = "grep model.name /proc/cpuinfo | cut -f 2 -d: | head -1"
        ret = utils.run(command).stdout.strip()

    # Remove bloat from CPU name, for example
    # 'Intel(R) Core(TM) i5-7Y57 CPU @ 1.20GHz'       -> 'Intel Core i5-7Y57'
    # 'Intel(R) Xeon(R) CPU E5-2690 v4 @ 2.60GHz'     -> 'Intel Xeon E5-2690 v4'
    # 'AMD A10-7850K APU with Radeon(TM) R7 Graphics' -> 'AMD A10-7850K'
    # 'AMD GX-212JC SOC with Radeon(TM) R2E Graphics' -> 'AMD GX-212JC'
    trim_re = " (@|processor|apu|soc|radeon).*|\(.*?\)| cpu"
    return re.sub(trim_re, '', ret, flags=re.IGNORECASE)


def get_screen_resolution():
    """Get the screen(s) resolution as strings.
    In case of more than 1 monitor, return resolution for each monitor separate
    with plus sign.

    @returns a string representing this host's screen(s) resolution.
    """
    command = 'for f in /sys/class/drm/*/*/modes; do head -1 $f; done'
    ret = utils.run(command, ignore_status=True)
    # We might have Chromebox without a screen
    if ret.exit_status != 0:
        return ''
    return ret.stdout.strip().replace('\n', '+')


def get_board_with_frequency_and_memory():
    """
    Returns a board name modified with CPU frequency and memory size to
    differentiate between different board variants. For instance
    link -> link_1.8GHz_4GB.
    """
    board_name = get_board()
    if is_virtual_machine():
        board = '%s_VM' % board_name
    else:
        memory = get_mem_total_gb()
        # Convert frequency to GHz with 1 digit accuracy after the
        # decimal point.
        frequency = int(round(get_cpu_max_frequency() * 1e-8)) * 0.1
        board = '%s_%1.1fGHz_%dGB' % (board_name, frequency, memory)
    return board


def get_mem_total():
    """
    Returns the total memory available in the system in MBytes.
    """
    mem_total = _get_float_from_file(_MEMINFO, 'MemTotal:', 'MemTotal:', ' kB')
    # Sanity check, all Chromebooks have at least 1GB of memory.
    assert mem_total > 256 * 1024, 'Unreasonable amount of memory.'
    return mem_total / 1024


def get_mem_total_gb():
    """
    Returns the total memory available in the system in GBytes.
    """
    return int(round(get_mem_total() / 1024.0))


def get_mem_free():
    """
    Returns the currently free memory in the system in MBytes.
    """
    mem_free = _get_float_from_file(_MEMINFO, 'MemFree:', 'MemFree:', ' kB')
    return mem_free / 1024

def get_mem_free_plus_buffers_and_cached():
    """
    Returns the free memory in MBytes, counting buffers and cached as free.

    This is most often the most interesting number since buffers and cached
    memory can be reclaimed on demand. Note however, that there are cases
    where this as misleading as well, for example used tmpfs space
    count as Cached but can not be reclaimed on demand.
    See https://www.kernel.org/doc/Documentation/filesystems/tmpfs.txt.
    """
    free_mb = get_mem_free()
    cached_mb = (_get_float_from_file(
        _MEMINFO, 'Cached:', 'Cached:', ' kB') / 1024)
    buffers_mb = (_get_float_from_file(
        _MEMINFO, 'Buffers:', 'Buffers:', ' kB') / 1024)
    return free_mb + buffers_mb + cached_mb

def get_kernel_max():
    """
    Returns content of kernel_max.
    """
    kernel_max = _get_int_from_file(_KERNEL_MAX, 0, None, None)
    # Sanity check.
    assert ((kernel_max > 0) and (kernel_max < 257)), 'Unreasonable kernel_max.'
    return kernel_max


def set_high_performance_mode():
    """
    Sets the kernel governor mode to the highest setting.
    Returns previous governor state.
    """
    original_governors = get_scaling_governor_states()
    set_scaling_governors('performance')
    return original_governors


def set_scaling_governors(value):
    """
    Sets all scaling governor to string value.
    Sample values: 'performance', 'interactive', 'ondemand', 'powersave'.
    """
    paths = _get_cpufreq_paths('scaling_governor')
    for path in paths:
        cmd = 'echo %s > %s' % (value, path)
        logging.info('Writing scaling governor mode \'%s\' -> %s', value, path)
        # On Tegra CPUs can be dynamically enabled/disabled. Ignore failures.
        utils.system(cmd, ignore_status=True)


def _get_cpufreq_paths(filename):
    """
    Returns a list of paths to the governors.
    """
    cmd = 'ls /sys/devices/system/cpu/cpu*/cpufreq/' + filename
    paths = utils.run(cmd, verbose=False).stdout.splitlines()
    return paths


def get_scaling_governor_states():
    """
    Returns a list of (performance governor path, current state) tuples.
    """
    paths = _get_cpufreq_paths('scaling_governor')
    path_value_list = []
    for path in paths:
        value = _get_line_from_file(path, 0)
        path_value_list.append((path, value))
    return path_value_list


def restore_scaling_governor_states(path_value_list):
    """
    Restores governor states. Inverse operation to get_scaling_governor_states.
    """
    for (path, value) in path_value_list:
        cmd = 'echo %s > %s' % (value.rstrip('\n'), path)
        # On Tegra CPUs can be dynamically enabled/disabled. Ignore failures.
        utils.system(cmd, ignore_status=True)


def get_dirty_writeback_centisecs():
    """
    Reads /proc/sys/vm/dirty_writeback_centisecs.
    """
    time = _get_int_from_file(_DIRTY_WRITEBACK_CENTISECS, 0, None, None)
    return time


def set_dirty_writeback_centisecs(time=60000):
    """
    In hundredths of a second, this is how often pdflush wakes up to write data
    to disk. The default wakes up the two (or more) active threads every five
    seconds. The ChromeOS default is 10 minutes.

    We use this to set as low as 1 second to flush error messages in system
    logs earlier to disk.
    """
    # Flush buffers first to make this function synchronous.
    utils.system('sync')
    if time >= 0:
        cmd = 'echo %d > %s' % (time, _DIRTY_WRITEBACK_CENTISECS)
        utils.system(cmd)


def wflinfo_cmd():
    """
    Returns a wflinfo command appropriate to the current graphics platform/api.
    """
    return 'wflinfo -p %s -a %s' % (graphics_platform(), graphics_api())


def has_mali():
    """ @return: True if system has a Mali GPU enabled."""
    return os.path.exists('/dev/mali0')

def get_gpu_family():
    """Returns the GPU family name."""
    global pciid_to_amd_architecture
    global pciid_to_intel_architecture

    socfamily = get_cpu_soc_family()
    if socfamily == 'exynos5' or socfamily == 'rockchip' or has_mali():
        cmd = wflinfo_cmd()
        wflinfo = utils.system_output(cmd,
                                      retain_output=True,
                                      ignore_status=False)
        version = re.findall(r'OpenGL renderer string: '
                             r'Mali-T([0-9]+)', wflinfo)
        if version:
            return 'mali-t%s' % version[0]
        return 'mali-unrecognized'
    if socfamily == 'tegra':
        return 'tegra'
    if os.path.exists('/sys/kernel/debug/pvr'):
        return 'rogue'

    pci_vga_device = utils.run("lspci | grep VGA").stdout.rstrip('\n')
    bus_device_function = pci_vga_device.partition(' ')[0]
    pci_path = '/sys/bus/pci/devices/0000:' + bus_device_function + '/device'

    if not os.path.exists(pci_path):
        raise error.TestError('PCI device 0000:' + bus_device_function + ' not found')

    device_id = utils.read_one_line(pci_path).lower()

    if "Advanced Micro Devices" in pci_vga_device:
        if not pciid_to_amd_architecture:
            with open(_AMD_PCI_IDS_FILE_PATH, 'r') as in_f:
                pciid_to_amd_architecture = json.load(in_f)

        return pciid_to_amd_architecture[device_id]

    if "Intel Corporation" in pci_vga_device:
        # Only load Intel PCI ID file once and only if necessary.
        if not pciid_to_intel_architecture:
            with open(_INTEL_PCI_IDS_FILE_PATH, 'r') as in_f:
                pciid_to_intel_architecture = json.load(in_f)

        return pciid_to_intel_architecture[device_id]

# TODO(ihf): Consider using /etc/lsb-release DEVICETYPE != CHROMEBOOK/CHROMEBASE
# for sanity check, but usage seems a bit inconsistent. See
# src/third_party/chromiumos-overlay/eclass/appid.eclass
_BOARDS_WITHOUT_MONITOR = [
    'anglar', 'mccloud', 'monroe', 'ninja', 'rikku', 'guado', 'jecht', 'tidus',
    'beltino', 'panther', 'stumpy', 'panther', 'tricky', 'zako', 'veyron_rialto'
]


def has_no_monitor():
    """Returns whether a machine doesn't have a built-in monitor."""
    board_name = get_board()
    if board_name in _BOARDS_WITHOUT_MONITOR:
        return True

    return False


def get_fixed_dst_drive():
    """
    Return device name for internal disk.
    Example: return /dev/sda for falco booted from usb
    """
    cmd = ' '.join(['. /usr/sbin/write_gpt.sh;',
                    '. /usr/share/misc/chromeos-common.sh;',
                    'load_base_vars;',
                    'get_fixed_dst_drive'])
    return utils.system_output(cmd)


def get_root_device():
    """
    Return root device.
    Will return correct disk device even system boot from /dev/dm-0
    Example: return /dev/sdb for falco booted from usb
    """
    return utils.system_output('rootdev -s -d')


def get_root_partition():
    """
    Return current root partition
    Example: return /dev/sdb3 for falco booted from usb
    """
    return utils.system_output('rootdev -s')


def get_free_root_partition(root_part=None):
    """
    Return currently unused root partion
    Example: return /dev/sdb5 for falco booted from usb

    @param root_part: cuurent root partition
    """
    spare_root_map = {'3': '5', '5': '3'}
    if not root_part:
        root_part = get_root_partition()
    return root_part[:-1] + spare_root_map[root_part[-1]]


def get_kernel_partition(root_part=None):
    """
    Return current kernel partition
    Example: return /dev/sda2 for falco booted from usb

    @param root_part: current root partition
    """
    if not root_part:
         root_part = get_root_partition()
    current_kernel_map = {'3': '2', '5': '4'}
    return root_part[:-1] + current_kernel_map[root_part[-1]]


def get_free_kernel_partition(root_part=None):
    """
    return currently unused kernel partition
    Example: return /dev/sda4 for falco booted from usb

    @param root_part: current root partition
    """
    kernel_part = get_kernel_partition(root_part)
    spare_kernel_map = {'2': '4', '4': '2'}
    return kernel_part[:-1] + spare_kernel_map[kernel_part[-1]]


def is_booted_from_internal_disk():
    """Return True if boot from internal disk. False, otherwise."""
    return get_root_device() == get_fixed_dst_drive()


def get_ui_use_flags():
    """Parses the USE flags as listed in /etc/ui_use_flags.txt.

    @return: A list of flag strings found in the ui use flags file.
    """
    flags = []
    for flag in utils.read_file(_UI_USE_FLAGS_FILE_PATH).splitlines():
        # Removes everything after the '#'.
        flag_before_comment = flag.split('#')[0].strip()
        if len(flag_before_comment) != 0:
            flags.append(flag_before_comment)

    return flags


def graphics_platform():
    """
    Return a string identifying the graphics platform,
    e.g. 'glx' or 'x11_egl' or 'gbm'
    """
    return 'null'


def graphics_api():
    """Return a string identifying the graphics api, e.g. gl or gles2."""
    use_flags = get_ui_use_flags()
    if 'opengles' in use_flags:
        return 'gles2'
    return 'gl'


def is_vm():
    """Check if the process is running in a virtual machine.

    @return: True if the process is running in a virtual machine, otherwise
             return False.
    """
    try:
        virt = utils.run('sudo -n virt-what').stdout.strip()
        logging.debug('virt-what output: %s', virt)
        return bool(virt)
    except error.CmdError:
        logging.warn('Package virt-what is not installed, default to assume '
                     'it is not a virtual machine.')
        return False


def is_package_installed(package):
    """Check if a package is installed already.

    @return: True if the package is already installed, otherwise return False.
    """
    try:
        utils.run(_CHECK_PACKAGE_INSTALLED_COMMAND % package)
        return True
    except error.CmdError:
        logging.warn('Package %s is not installed.', package)
        return False


def is_python_package_installed(package):
    """Check if a Python package is installed already.

    @return: True if the package is already installed, otherwise return False.
    """
    try:
        __import__(package)
        return True
    except ImportError:
        logging.warn('Python package %s is not installed.', package)
        return False


def run_sql_cmd(server, user, password, command, database=''):
    """Run the given sql command against the specified database.

    @param server: Hostname or IP address of the MySQL server.
    @param user: User name to log in the MySQL server.
    @param password: Password to log in the MySQL server.
    @param command: SQL command to run.
    @param database: Name of the database to run the command. Default to empty
                     for command that does not require specifying database.

    @return: The stdout of the command line.
    """
    cmd = ('mysql -u%s -p%s --host %s %s -e "%s"' %
           (user, password, server, database, command))
    # Set verbose to False so the command line won't be logged, as it includes
    # database credential.
    return utils.run(cmd, verbose=False).stdout
