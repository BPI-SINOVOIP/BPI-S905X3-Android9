# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Facade to access the system-related functionality."""

import os

from autotest_lib.client.bin import utils


class SystemFacadeNativeError(Exception):
    """Error in SystemFacadeNative."""
    pass


class SystemFacadeNative(object):
    """Facede to access the system-related functionality.

    The methods inside this class only accept Python native types.

    """
    SCALING_GOVERNOR_MODES = [
            'interactive',
            'performance',
            'ondemand',
            'powersave',
            'sched'
            ]

    def set_scaling_governor_mode(self, index, mode):
        """Set mode of CPU scaling governor on one CPU.

        @param index: CPU index starting from 0.

        @param mode: Mode of scaling governor, accept 'interactive' or
                     'performance'.

        @returns: The original mode.

        """
        if mode not in self.SCALING_GOVERNOR_MODES:
            raise SystemFacadeNativeError('mode %s is invalid' % mode)

        governor_path = os.path.join(
                '/sys/devices/system/cpu/cpu%d' % index,
                'cpufreq/scaling_governor')
        if not os.path.exists(governor_path):
            raise SystemFacadeNativeError(
                    'scaling governor of CPU %d is not available' % index)

        original_mode = utils.read_one_line(governor_path)
        utils.open_write_close(governor_path, mode)

        return original_mode


    def get_cpu_usage(self):
        """Returns machine's CPU usage.

        Returns:
            A dictionary with 'user', 'nice', 'system' and 'idle' values.
            Sample dictionary:
            {
                'user': 254544,
                'nice': 9,
                'system': 254768,
                'idle': 2859878,
            }
        """
        return utils.get_cpu_usage()


    def compute_active_cpu_time(self, cpu_usage_start, cpu_usage_end):
        """Computes the fraction of CPU time spent non-idling.

        This function should be invoked using before/after values from calls to
        get_cpu_usage().
        """
        return utils.compute_active_cpu_time(cpu_usage_start,
                                                  cpu_usage_end)


    def get_mem_total(self):
        """Returns the total memory available in the system in MBytes."""
        return utils.get_mem_total()


    def get_mem_free(self):
        """Returns the currently free memory in the system in MBytes."""
        return utils.get_mem_free()

    def get_mem_free_plus_buffers_and_cached(self):
        """
        Returns the free memory in MBytes, counting buffers and cached as free.

        This is most often the most interesting number since buffers and cached
        memory can be reclaimed on demand. Note however, that there are cases
        where this as misleading as well, for example used tmpfs space
        count as Cached but can not be reclaimed on demand.
        See https://www.kernel.org/doc/Documentation/filesystems/tmpfs.txt.
        """
        return utils.get_mem_free_plus_buffers_and_cached()

    def get_ec_temperatures(self):
        """Uses ectool to return a list of all sensor temperatures in Celsius.
        """
        return utils.get_ec_temperatures()

    def get_current_temperature_max(self):
        """
        Returns the highest reported board temperature (all sensors) in Celsius.
        """
        return utils.get_current_temperature_max()

    def get_current_board(self):
        """Returns the current device board name."""
        return utils.get_current_board()


    def get_chromeos_release_version(self):
        """Returns chromeos version in device under test as string. None on
        fail.
        """
        return utils.get_chromeos_release_version()

    def get_num_allocated_file_handles(self):
        """
        Returns the number of currently allocated file handles.
        """
        return utils.get_num_allocated_file_handles()
