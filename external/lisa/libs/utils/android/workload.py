# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2015, ARM Limited and contributors.
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may
# not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import logging
import os
import re
import webbrowser
import time
from collections import namedtuple

from gfxinfo import GfxInfo
from surfaceflinger import SurfaceFlinger
from devlib.utils.android_build import Build

from . import System

class Workload(object):
    """
    Base class for Android related workloads
    """

    _packages = None
    _availables = {}

    WorkloadPackage = namedtuple("WorkloadPackage", "package_name apk_path src_path")

    def __init__(self, test_env):
        """
        Initialized workloads available on the specified test environment

        test_env: target test environment
        """
        self._te = test_env
        self._target = test_env.target
        self._log = logging.getLogger('Workload')

        # Set of data reported in output of each run
        self.trace_file = None
        self.nrg_report = None

        # Hooks to run at different points of workload execution
        self.hooks = {}

    def _adb(self, cmd):
        return 'adb -s {} {}'.format(self._target.adb_name, cmd)

    @classmethod
    def _packages_installed(cls, sc, allow_install):
        # If workload does not have packages just return
        if not hasattr(sc, 'packages'):
            return True
        # Require package to be installed unless it can be installed when allowed
        if allow_install:
            required_packages = [package.package_name for package in sc.packages if package.apk_path==None]
        else:
            required_packages = [package.package_name for package in sc.packages]
        return all(p in cls._packages for p in required_packages)

    @classmethod
    def _build_packages(cls, sc, te):
        bld = Build(te)
        for p in sc.packages:
            if p.src_path != None:
                bld.build_module(p.src_path)
        return True

    @classmethod
    def _install_packages(cls, sc, te):
        for p in sc.packages:
            System.install_apk(te.target,
                '{}/{}'.format(te.ANDROID_PRODUCT_OUT, p.apk_path))
        return True;

    @classmethod
    def _subclasses(cls):
        """
        Recursively get all subclasses
        """
        nodes = cls.__subclasses__()
        return nodes + [child for node in nodes for child in node._subclasses()]

    @classmethod
    def _check_availables(cls, test_env):
        """
        List the supported android workloads which are available on the target
        """

        _log = logging.getLogger('Workload')

        # Getting the list of installed packages
        cls._packages = test_env.target.list_packages()
        _log.debug('Packages:\n%s', cls._packages)

        _log.debug('Building list of available workloads...')
        for sc in Workload._subclasses():
            _log.debug('Checking workload [%s]...', sc.__name__)

            # Check if all required packages are installed or can be installed
            if cls._packages_installed(sc, True):
                cls._availables[sc.__name__.lower()] = sc

        _log.info('Supported workloads available on target:')
        _log.info('  %s', ', '.join(cls._availables.keys()))

    @classmethod
    def getInstance(cls, test_env, name, reinstall=False):
        """
        Get a reference to the specified Android workload

        :param test_env: target test environment
        :type test_env: TestEnv

        :param name: workload name
        :type name: str

        :param reinstall: flag to reinstall workload applications
        :type reinstall: boolean

        """

        # Initialize list of available workloads
        if cls._packages is None:
            cls._check_availables(test_env)

        if name.lower() not in cls._availables:
            msg = 'Workload [{}] not available on target'.format(name)
            raise ValueError(msg)

        sc = cls._availables[name.lower()]

        if (reinstall or not cls._packages_installed(sc, False)):
            if (not cls._build_packages(sc, test_env) or
                not cls._install_packages(sc, test_env)):
                msg = 'Unable to install packages required for [{}] workload'.format(name)
                raise RuntimeError(msg)

        ret_cls = sc(test_env)

        # Add generic support for cgroup tracing (detect if cgroup module exists)
        if ('modules' in test_env.conf) and ('cgroups' in test_env.conf['modules']):
                # Enable dumping support (which happens after systrace starts)
                ret_cls._log.info('Enabling CGroup support for dumping schedtune/cpuset events')
                ret_cls.add_hook('post_collect_start', ret_cls.post_collect_start_cgroup)
                # Also update the extra ftrace points needed
                if not 'systrace' in test_env.conf:
                    test_env.conf['systrace'] = { 'extra_events': ['cgroup_attach_task', 'sched_process_fork'] }
                else:
                    if not 'extra_events' in test_env.conf['systrace']:
                        test_env.conf['systrace']['extra_events'] = ['cgroup_attach_task', 'sched_process_fork']
                    else:
                        test_env.conf['systrace']['extra_events'].extend(['cgroup_attach_task', 'sched_process_fork'])

        return ret_cls

    def trace_cgroup(self, controller, cgroup):
        cgroup = self._te.target.cgroups.controllers[controller].cgroup('/' + cgroup)
        cgroup.trace_cgroup_tasks()

    def post_collect_start_cgroup(self):
        # Since systrace starts asynchronously, wait for trace to start
        while True:
            if self._te.target.execute('cat /d/tracing/tracing_on')[0] == "0":
                time.sleep(0.1)
                continue
            break

        self.trace_cgroup('schedtune', '')           # root
        self.trace_cgroup('schedtune', 'top-app')
        self.trace_cgroup('schedtune', 'foreground')
        self.trace_cgroup('schedtune', 'background')
        self.trace_cgroup('schedtune', 'rt')

        self.trace_cgroup('cpuset', '')              # root
        self.trace_cgroup('cpuset', 'top-app')
        self.trace_cgroup('cpuset', 'foreground')
        self.trace_cgroup('cpuset', 'background')
        self.trace_cgroup('cpuset', 'system-background')

    def add_hook(self, hook, hook_fn):
        allowed = ['post_collect_start']
        if hook not in allowed:
            return
        self.hooks[hook] = hook_fn

    def run(self, out_dir, collect='',
            **kwargs):
        raise RuntimeError('Not implemented')

    def tracingStart(self, screen_always_on=True):
        # Keep the screen on during any data collection
        if screen_always_on:
            System.screen_always_on(self._target, enable=True)
        # Reset the dumpsys data for the package
        if 'gfxinfo' in self.collect:
            System.gfxinfo_reset(self._target, self.package)
        if 'surfaceflinger' in self.collect:
            System.surfaceflinger_reset(self._target, self.package)
        if 'logcat' in self.collect:
            System.logcat_reset(self._target)
        # Make sure ftrace and systrace are not both specified to be collected
        if 'ftrace' in self.collect and 'systrace' in self.collect:
            msg = 'ftrace and systrace cannot be used at the same time'
            raise ValueError(msg)
        # Start FTrace
        if 'ftrace' in self.collect:
            self.trace_file = os.path.join(self.out_dir, 'trace.dat')
            self._log.info('FTrace START')
            self._te.ftrace.start()
        # Start Systrace (mutually exclusive with ftrace)
        elif 'systrace' in self.collect:
            self.trace_file = os.path.join(self.out_dir, 'trace.html')
            # Get the systrace time
            match = re.search(r'systrace_([0-9]+)', self.collect)
            self._trace_time = match.group(1) if match else None
            self._log.info('Systrace START')
            self._target.execute('echo 0 > /d/tracing/tracing_on')
            self._systrace_output = System.systrace_start(
                self._te, self.trace_file, self._trace_time, conf=self._te.conf)
            if 'energy' in self.collect:
                # Wait for systrace to start before cutting off USB
                while True:
                    if self._target.execute('cat /d/tracing/tracing_on')[0] == "0":
                        time.sleep(0.1)
                        continue
                    break
        # Initializing frequency times
        if 'time_in_state' in self.collect:
            self._time_in_state_start = self._te.target.cpufreq.get_time_in_state(
                    self._te.topology.get_level('cluster'))
        # Initialize energy meter results
        if 'energy' in self.collect and self._te.emeter:
            self._te.emeter.reset()
            self._log.info('Energy meter STARTED')
        # Run post collect hooks passed added by the user of wload object
        if 'post_collect_start' in self.hooks:
            hookfn = self.hooks['post_collect_start']
            self._log.info("Running post collect startup hook {}".format(hookfn.__name__))
            hookfn()

    def tracingStop(self, screen_always_on=True):
        # Collect energy meter results
        if 'energy' in self.collect and self._te.emeter:
            self.nrg_report = self._te.emeter.report(self.out_dir)
            self._log.info('Energy meter STOPPED')
        # Calculate the delta in frequency times
        if 'time_in_state' in self.collect:
            self._te.target.cpufreq.dump_time_in_state_delta(
                    self._time_in_state_start,
                    self._te.topology.get_level('cluster'),
                    os.path.join(self.out_dir, 'time_in_state.json'))
        # Stop FTrace
        if 'ftrace' in self.collect:
            self._te.ftrace.stop()
            self._log.info('FTrace STOP')
            self._te.ftrace.get_trace(self.trace_file)
        # Stop Systrace (mutually exclusive with ftrace)
        elif 'systrace' in self.collect:
            if not self._systrace_output:
                self._log.warning('Systrace is not running!')
            else:
                self._log.info('Waiting systrace report [%s]...',
                                 self.trace_file)
                if self._trace_time is None:
                    # Systrace expects <enter>
                    self._systrace_output.sendline('')
                self._systrace_output.wait()
        # Parse the data gathered from dumpsys gfxinfo
        if 'gfxinfo' in self.collect:
            dump_file = os.path.join(self.out_dir, 'dumpsys_gfxinfo.txt')
            System.gfxinfo_get(self._target, self.package, dump_file)
            self.gfxinfo = GfxInfo(dump_file)
        # Parse the data gathered from dumpsys SurfaceFlinger
        if 'surfaceflinger' in self.collect:
            dump_file = os.path.join(self.out_dir, 'dumpsys_surfaceflinger.txt')
            System.surfaceflinger_get(self._target, self.package, dump_file)
            self.surfaceflinger = SurfaceFlinger(dump_file)
        if 'logcat' in self.collect:
            dump_file = os.path.join(self.out_dir, 'logcat.txt')
            System.logcat_get(self._target, dump_file)
        # Dump a platform description
        self._te.platform_dump(self.out_dir)
        # Restore automatic screen off
        if screen_always_on:
            System.screen_always_on(self._target, enable=False)

    def traceShow(self):
        """
        Open the collected trace using the most appropriate native viewer.

        The native viewer depends on the specified trace format:
        - ftrace: open using kernelshark
        - systrace: open using a browser

        In both cases the native viewer is assumed to be available in the host
        machine.
        """

        if 'ftrace' in self.collect:
            os.popen("kernelshark {}".format(self.trace_file))
            return

        if 'systrace' in self.collect:
            webbrowser.open(self.trace_file)
            return

        self._log.warning('No trace collected since last run')

# vim :set tabstop=4 shiftwidth=4 expandtab
