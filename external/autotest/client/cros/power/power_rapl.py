# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Methods and Classes to support RAPL power access.

Intel processors (Sandybridge and beyond) provide access to a set of registers
via the MSR interface to control and measure energy/power consumption.  These
RAPL ( Running Average Power Limit ) registers can be queried and written to
change and evaluate power consumption on the CPU.

See 'Intel 64 and IA-32 Architectures Software Developer's Manual Volume 3'
(Section 14.9) for complete details.

TODO(tbroch)
1. Investigate exposing access to control Power policy.  Current implementation
just surveys consumption via energy status registers.
"""
import logging
import os
import time

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.power import power_status
from autotest_lib.client.cros.power import power_utils
from numpy import uint32


VALID_DOMAINS = ['pkg', 'pp0', 'gfx', 'dram']


def get_rapl_measurement(tname, exe_function=time.sleep, exe_args=(10,),
                         exe_kwargs={}):
    """Get rapl measurement.

    @param name: String of test name.
    @param exe_function: function that should be executed during measuring
                         rapl readings.
    @param exe_args: tuple of args to be passed into exe_function.
    @param exe_kwargs: dict of args to be passed into exe_function.
    """
    logging.info('Now measuring rapl power consumption.')
    measurement = []
    if power_utils.has_rapl_support():
        measurement += create_rapl()
    power_logger = power_status.PowerLogger(measurement)
    power_logger.start()
    with power_logger.checkblock(tname):
        exe_function(*exe_args, **exe_kwargs)
    keyval = power_logger.calc()
    return keyval


def create_rapl(domains=None):
    """Create a set of Rapl instances.

    Args:
        domains: list of strings, representing desired RAPL domains to
        instantiate.

    Returns:
        list of Rapl objects.

    Raises:
        error.TestFail: If domain is invalid.
    """
    if not domains:
        domains = VALID_DOMAINS
    rapl_list = []
    for domain in set(domains):
        rapl_list.append(Rapl(domain))
    return rapl_list


class Rapl(power_status.PowerMeasurement):
    """Class to expose RAPL functionality.

    Public attibutes:
        domain: string, name of power rail domain.

    Private attributes:
        _joules_per_lsb: float, joules per lsb of energy.
        _joules_start: float, joules measured at the beginning of operation.
        _time_start: float, time in seconds since Epoch.

    Public methods:
        refresh(): Refreshes starting point of RAPL power measurement and
                   returns power in watts.
    """
    _DOMAIN_MSRS = {'pkg': {'power_limit':    0x610,
                            'energy_status':  0x611,
                            'perf_status':    0x613,
                            'power_info':     0x614},
                    'pp0': {'power_limit':    0x638,
                            'energy_status':  0x639,
                            'policy':         0x63a,
                            'perf_status':    0x63b},
                    'gfx': {'power_limit':    0x640,
                            'energy_status':  0x641,
                            'policy':         0x642},
                    'dram': {'power_limit':   0x618,
                             'energy_status': 0x619,
                             'perf_status':   0x61b,
                             'power_info':    0x61c}}

    # Units for Power, Energy & Time
    _POWER_UNIT_MSR = 0x606

    _POWER_UNIT_OFFSET  = 0x0
    _POWER_UNIT_MASK    = 0x0F
    _ENERGY_UNIT_OFFSET = 0x08
    _ENERGY_UNIT_MASK   = 0x1F00
    _TIME_UNIT_OFFSET   = 0x10
    _TIME_UNIT_MASK     = 0xF000

    # Maximum number of seconds allowable between energy status samples.  See
    # docstring in power method for complete details.
    _MAX_MEAS_SECS = 1800


    def __init__(self, domain):
        """Constructor for Rapl class.

        Args:
            domain: string, name of power rail domain

        Raises:
            error.TestError: If domain is invalid
        """
        if domain not in VALID_DOMAINS:
            raise error.TestError("domain %s not in valid domains ( %s )" %
                                  (domain, ", ".join(VALID_DOMAINS)))
        super(Rapl, self).__init__(domain)

        self._joules_per_lsb = self._get_joules_per_lsb()
        logging.debug("RAPL %s joules_per_lsb = %.3e", domain,
                      self._joules_per_lsb)
        self._joules_start = self._get_energy()
        self._time_start = time.time()


    def __del__(self):
        """Deconstructor for Rapl class.

        Raises:
            error.TestError: If the joules per lsb changed during sampling time.
        """
        if self._get_joules_per_lsb() != self._joules_per_lsb:
            raise error.TestError("Results suspect as joules_per_lsb changed "
                                  "during sampling")


    def _rdmsr(self, msr, cpu_id=0):
        """Read MSR ( Model Specific Register )

        Read MSR value for x86 systems.

        Args:
            msr: Integer, address of MSR.
            cpu_id: Integer, number of CPU to read MSR for.  Default 0.
        Returns:
            Integer, representing the requested MSR register.
        """
        return int(utils.system_output('iotools rdmsr %d %d' %
                                       (cpu_id, msr)), 0)


    def _get_joules_per_lsb(self):
        """Calculate and return energy in joules per lsb.

        Value used as a multiplier while reading the RAPL energy status MSR.

        Returns:
            Float, value of joules per lsb.
        """
        msr_val = self._rdmsr(self._POWER_UNIT_MSR)
        return 1.0 / pow(2, (msr_val & self._ENERGY_UNIT_MASK) >>
                         self._ENERGY_UNIT_OFFSET)


    def _get_energy(self):
        """Get energy reading.

        Returns:
            Integer (32-bit), representing total energy consumed since power-on.
        """
        msr = self._DOMAIN_MSRS[self.domain]['energy_status']
        return uint32(self._rdmsr(msr))


    def domain(self):
        """Convenience method to expose Rapl instance domain name.

        Returns:
           string, name of Rapl domain.
        """
        return self.domain


    def refresh(self):
        """Calculate the average power used for RAPL domain.

        Note, Intel doc says ~60secs but in practice it seems much longer on
        laptop class devices.  Using numpy's uint32 correctly calculates single
        wraparound.  Risk is whether wraparound occurs multiple times.  As the
        RAPL facilities don't provide any way to identify multiple wraparounds
        it does present a risk to long samples.  To remedy, method raises an
        exception for long measurements that should be well below the multiple
        wraparound window.  Length of time between measurements must be managed
        by periodic logger instantiating this object to avoid the exception.

        Returns:
            float, average power (in watts) over the last time interval tracked.
        Raises:
            error.TestError:  If time between measurements too great.
        """
        joules_now = self._get_energy()
        time_now = time.time()
        energy_used = (joules_now - self._joules_start) * self._joules_per_lsb
        time_used = time_now - self._time_start
        if time_used > self._MAX_MEAS_SECS:
            raise error.TestError("Time between reads of %s energy status "
                                  "register was > %d seconds" % \
                                      (self.domain, self._MAX_MEAS_SECS))
        average_power = energy_used / time_used
        self._joules_start = joules_now
        self._time_start = time_now
        return average_power


def create_powercap():
    """Create a list of Powercap instances of PowerMeasurement

    Args:
        (none)

    Returns:
        A list of Powercap objects.
    """
    powercap = '/sys/devices/virtual/powercap/intel-rapl/'
    # Failsafe check
    if not os.path.isdir(powercap):
        logging.debug("RAPL: no powercap driver found")
        return []
    rapl_map = {}
    for root, dir, file in os.walk(powercap):
        if os.path.isfile(root + '/energy_uj'):
            with open(root + '/name', 'r') as fn:
                name = fn.read().rstrip()
                rapl_map[name] = root + '/energy_uj'
    return [Powercap(name, path) for name, path in rapl_map.iteritems()]


class Powercap(power_status.PowerMeasurement):
    """Classes to support RAPL power measurement via powercap sysfs

    This class utilizes the subset of Linux powercap driver to report
    energy consumption, in this manner, we do not need microarchitecture
    knowledge in userspace program.

    For more detail of powercap framework, readers could refer to:
    https://www.kernel.org/doc/Documentation/power/powercap/powercap.txt
    https://youtu.be/1Rl8PyuK6yA

    Private attributes:
        _file: sysfs reporting energy of the particular RAPL domain.
        _energy_start: float, micro-joule measured at the beginning.
        _time_start: float, time in seconds since Epoch.
    """
    def __init__(self, name, path):
        """Constructor for Powercap class.
        """
        super(Powercap, self).__init__(name)

        self._file = open(path, 'r')
        self._energy_start = self._get_energy()
        self._time_start = time.time()
        logging.debug("RAPL: monitor domain %s", name)


    def __del__(self):
        """Deconstructor for Powercap class.
        """
        self._file.close()


    def _get_energy(self):
        """Get energy reading in micro-joule unit.
        """
        self._file.seek(0)
        return int(self._file.read().rstrip())


    def refresh(self):
        """Calculate the average power used per RAPL domain.
        """
        energy_now = self._get_energy()
        time_now = time.time()
        energy_used = (energy_now - self._energy_start) / 1000000.0
        time_used = time_now - self._time_start
        average_power = energy_used / time_used
        self._energy_start = energy_now
        self._time_start = time_now
        return average_power
