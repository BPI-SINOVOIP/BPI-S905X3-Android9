#!/usr/bin/env python
# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2017, ARM Limited, Google, and contributors.
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

from __future__ import division
import os
import json
from lxml import etree

from power_average import PowerAverage
from cpu_frequency_power_average import CpuFrequencyPowerAverage


class PowerProfile:
    def __init__(self):
        self.xml = etree.Element('device', name='Android')

    default_comments = {
        'none' : 'Nothing',

        'battery.capacity' : 'This is the battery capacity in mAh',

        'cpu.suspend' : 'Power consumption when CPU is suspended',
        'cpu.idle' : 'Additional power consumption when CPU is in a kernel'
                ' idle loop',
        'cpu.clusters.cores' : 'Number of cores each CPU cluster contains',

        'screen.on' : 'Additional power used when screen is turned on at'
                ' minimum brightness',
        'screen.full' : 'Additional power used when screen is at maximum'
                ' brightness, compared to screen at minimum brightness',

        'camera.flashlight' : 'Average power used by the camera flash module'
                ' when on.',
        'camera.avg' : 'Average power use by the camera subsystem for a typical'
                ' camera application.',

        'gps.on' : 'Additional power used when GPS is acquiring a signal.',

        'bluetooth.controller.idle' : 'Average current draw (mA) of the'
                ' Bluetooth controller when idle.',
        'bluetooth.controller.rx' : 'Average current draw (mA) of the Bluetooth'
                ' controller when receiving.',
        'bluetooth.controller.tx' : 'Average current draw (mA) of the Bluetooth'
                ' controller when transmitting.',
        'bluetooth.controller.voltage' : 'Average operating voltage (mV) of the'
                ' Bluetooth controller.',

        'modem.controller.idle' : 'Average current draw (mA) of the modem'
                ' controller when idle.',
        'modem.controller.rx' : 'Average current draw (mA) of the modem'
                ' controller when receiving.',
        'modem.controller.tx' : 'Average current draw (mA) of the modem'
                ' controller when transmitting.',
        'modem.controller.voltage' : 'Average operating voltage (mV) of the'
                ' modem controller.',

        'wifi.controller.idle' : 'Average current draw (mA) of the Wi-Fi'
                ' controller when idle.',
        'wifi.controller.rx' : 'Average current draw (mA) of the Wi-Fi'
                ' controller when receiving.',
        'wifi.controller.tx' : 'Average current draw (mA) of the Wi-Fi'
                ' controller when transmitting.',
        'wifi.controller.voltage' : 'Average operating voltage (mV) of the'
                ' Wi-Fi controller.',
    }

    def _add_comment(self, item, name, comment):
        if (not comment) and (name in PowerProfile.default_comments):
            comment = PowerProfile.default_comments[name]
        if comment:
            self.xml.append(etree.Comment(comment))

    def add_item(self, name, value, comment=None):
        if self.get_item(name) is not None:
            raise RuntimeWarning('{} already added. Skipping.'.format(name))
            return

        item = etree.Element('item', name=name)
        item.text = str(value)
        self._add_comment(self.xml, name, comment)
        self.xml.append(item)

    def get_item(self, name):
        items = self.xml.findall(".//*[@name='{}']".format(name))
        if len(items) == 0:
            return None
        return float(items[0].text)

    def add_array(self, name, values, comment=None, subcomments=None):
        array = etree.Element('array', name=name)
        for i, value in enumerate(values):
            entry = etree.Element('value')
            entry.text = str(value)
            if subcomments:
                array.append(etree.Comment(subcomments[i]))
            array.append(entry)

        self._add_comment(self.xml, name, comment)
        self.xml.append(array)

    def __str__(self):
        return etree.tostring(self.xml, pretty_print=True)

class PowerProfileGenerator:

    def __init__(self, emeter, datasheet):
        self.emeter = emeter
        self.datasheet = datasheet
        self.power_profile = PowerProfile()
        self.cpu = None

    def get(self):
        self._compute_measurements()
        self._import_datasheet()

        return self.power_profile

    def _run_experiment(self, filename, duration, out_prefix, args=''):
        os.system('python {} --duration {} --out_prefix {} {} '.format(
                os.path.join(os.environ['LISA_HOME'], 'experiments', filename),
                duration, out_prefix, args))

    def _power_average(self, results_dir, start=None, remove_outliers=False):
        column = self.emeter['power_column']
        sample_rate_hz = self.emeter['sample_rate_hz']

        return PowerAverage.get(os.path.join(os.environ['LISA_HOME'], 'results',
                results_dir, 'samples.csv'), column, sample_rate_hz,
                start=start, remove_outliers=remove_outliers) * 1000

    def _cpu_freq_power_average(self):
        duration = 120

        self._run_experiment(os.path.join('power', 'eas',
                'run_cpu_frequency.py'), duration, 'cpu_freq')
        self.cpu= CpuFrequencyPowerAverage.get(
                os.path.join(os.environ['LISA_HOME'], 'results',
                'CpuFrequency_cpu_freq'), os.path.join(os.environ['LISA_HOME'],
                'results', 'CpuFrequency', 'platform.json'),
                self.emeter['power_column'])

    def _remove_cpu_suspend(self, power):
        cpu_suspend_power = self.power_profile.get_item('cpu.suspend')
        if cpu_suspend_power is None:
            self._measure_cpu_suspend()
            cpu_suspend_power = self.power_profile.get_item('cpu.suspend')

        return power - cpu_suspend_power

    def _remove_cpu_power(self, power, duration, results_dir):
        if self.cpu is None:
            self._cpu_freq_power_average()

        cfile = os.path.join(os.environ['LISA_HOME'], 'results', results_dir,
                'time_in_state.json')
        with open(cfile, 'r') as f:
            time_in_state_json = json.load(f)

        energy = 0.0
        for cl in sorted(time_in_state_json['clusters']):
            time_in_state_cpus = set(int(c) for c in time_in_state_json['clusters'][cl])

            for cluster in self.cpu.get_clusters():
                if time_in_state_cpus == set(self.cpu.get_cores(cluster)):
                    cpu_cnt = len(self.cpu.get_cores(cluster))

                    for freq, time_cs in time_in_state_json['time_delta'][cl].iteritems():
                        time_s = time_cs * 0.01
                        energy += time_s * self.cpu.get_core_cost(cluster, int(freq))

                    # TODO remove cpu cluster cost and addtional base cost
                    # This will require a kernel patch to keep track of cluster
                    # time

        return power - energy / duration * 1000

    def _remove_screen_full(self, power, duration, image):
        out_prefix = image.split('.')[0]
        results_dir = 'DisplayImage_{}'.format(out_prefix)

        self._run_experiment('run_display_image.py', duration, out_prefix,
                args='--collect=energy,time_in_state --brightness 100 --image={}'.format(image))
        display_plus_cpu_power = self._power_average(results_dir)

        display_power = self._remove_cpu_power(display_plus_cpu_power,
                duration, results_dir)

        return power - display_power

    def _measure_cpu_suspend(self):
        duration = 120

        self._run_experiment('run_suspend_resume.py', duration, 'cpu_suspend',
                args='--collect energy')
        power = self._power_average('SuspendResume_cpu_suspend',
                start=duration*0.25, remove_outliers=True)

        self.power_profile.add_item('cpu.suspend', power)

    def _measure_cpu_idle(self):
        duration = 120

        self._run_experiment('run_idle_resume.py', duration, 'cpu_idle',
                args='--collect energy')
        power = self._power_average('IdleResume_cpu_idle', start=duration*0.25,
                remove_outliers=True)

        power = self._remove_cpu_suspend(power)

        self.power_profile.add_item('cpu.idle', power)

    def _measure_screen_on(self):
        duration = 120
        results_dir = 'DisplayImage_screen_on'

        self._run_experiment('run_display_image.py', duration, 'screen_on',
                args='--collect=energy,time_in_state --brightness 0')
        power = self._power_average(results_dir)

        power = self._remove_cpu_power(power, duration, results_dir)

        self.power_profile.add_item('screen.on', power)

    def _measure_screen_full(self):
        duration = 120
        results_dir = 'DisplayImage_screen_full'

        self._run_experiment('run_display_image.py', duration, 'screen_full',
                args='--collect=energy,time_in_state --brightness 100')
        power = self._power_average(results_dir)

        power = self._remove_cpu_power(power, duration, results_dir)

        self.power_profile.add_item('screen.full', power)

    def _measure_cpu_cluster_cores(self):
        if self.cpu is None:
            self._cpu_freq_power_average()

        self.power_profile.add_array('cpu.clusters.cores', self.cpu.get_clusters())

    def _measure_cpu_active_power(self):
        if self.cpu is None:
            self._cpu_freq_power_average()

        comment = 'Additional power used when any cpu core is turned on'\
                ' in any cluster. Does not include the power used by the cpu'\
                ' cluster(s) or core(s).'
        self.power_profile.add_item('cpu.active', self.cpu.get_active_cost()*1000,
                comment)

    def _measure_cpu_cluster_power(self):
        if self.cpu is None:
            self._cpu_freq_power_average()

        clusters = self.cpu.get_clusters()

        for cluster in clusters:
            cluster_power = self.cpu.get_cluster_cost(cluster)

            comment = 'Additional power used when any cpu core is turned on'\
                    ' in cluster{}. Does not include the power used by the cpu'\
                    ' core(s).'.format(cluster)
            self.power_profile.add_item('cpu.cluster_power.cluster{}'.format(cluster),
                    cluster_power*1000, comment)

    def _measure_cpu_core_speeds(self):
        if self.cpu is None:
            self._cpu_freq_power_average()

        clusters = self.cpu.get_clusters()

        for cluster in clusters:
            core_speeds = self.cpu.get_core_freqs(cluster)

            comment = 'Different CPU speeds as reported in /sys/devices/system/'\
                    'cpu/cpuX/cpufreq/scaling_available_frequencies'
            self.power_profile.add_array('cpu.core_speeds.cluster{}'.format(cluster),
                    core_speeds, comment)

    def _measure_cpu_core_power(self):
        if self.cpu is None:
            self._cpu_freq_power_average()

        clusters = self.cpu.get_clusters()

        for cluster in clusters:
            core_speeds = self.cpu.get_core_freqs(cluster)

            core_powers = [ self.cpu.get_core_cost(cluster, core_speed)*1000 for core_speed in core_speeds ]
            comment = 'Additional power used by a CPU from cluster {} when'\
                    ' running at different speeds. Currently this measurement'\
                    ' also includes cluster cost.'.format(cluster)
            subcomments = [ '{} MHz CPU speed'.format(core_speed*0.001) for core_speed in core_speeds ]

            self.power_profile.add_array('cpu.core_power.cluster{}'.format(cluster),
                    core_powers, comment, subcomments)

    def _measure_camera_flashlight(self):
        duration = 120
        results_dir = 'CameraFlashlight_camera_flashlight'

        self._run_experiment(os.path.join('power', 'profile',
                'run_camera_flashlight.py'), duration, 'camera_flashlight',
                args='--collect=energy,time_in_state')
        power = self._power_average(results_dir)

        power = self._remove_screen_full(power, duration,
                'power_profile_camera_flashlight.png')
        power = self._remove_cpu_power(power, duration, results_dir)

        self.power_profile.add_item('camera.flashlight', power)

    def _measure_camera_avg(self):
        duration = 120
        results_dir = 'CameraAvg_camera_avg'

        self._run_experiment(os.path.join('power', 'profile',
                'run_camera_avg.py'), duration, 'camera_avg',
                args='--collect=energy,time_in_state')
        power = self._power_average(results_dir)

        power = self._remove_screen_full(power, duration,
                'power_profile_camera_avg.png')
        power = self._remove_cpu_power(power, duration, results_dir)

        self.power_profile.add_item('camera.avg', power)

    def _measure_gps_on(self):
        duration = 120
        results_dir = 'GpsOn_gps_on'

        self._run_experiment(os.path.join('power', 'profile', 'run_gps_on.py'),
                duration, 'gps_on', args='--collect=energy,time_in_state')
        power = self._power_average(results_dir)

        power = self._remove_screen_full(power, duration,
                'power_profile_gps_on.png')
        power = self._remove_cpu_power(power, duration, results_dir)

        self.power_profile.add_item('gps.on', power)

    def _compute_measurements(self):
        #self._measure_cpu_suspend()
        #self._measure_cpu_idle()
        self._measure_cpu_cluster_cores()
        self._measure_cpu_active_power()
        self._measure_cpu_cluster_power()
        self._measure_cpu_core_speeds()
        self._measure_cpu_core_power()
        self._measure_screen_on()
        self._measure_screen_full()
        self._measure_camera_flashlight()
        self._measure_camera_avg()
        self._measure_gps_on()

    def _import_datasheet(self):
        for item in sorted(self.datasheet.keys()):
            self.power_profile.add_item(item, self.datasheet[item])

my_emeter = {
    'power_column'      : 'output_power',
    'sample_rate_hz'    : 500,
}

my_datasheet = {

# Add datasheet values in the following format:
#
#    'none'  : 0,
#
#    'battery.capacity'  : 0,
#
#    'bluetooth.controller.idle'     : 0,
#    'bluetooth.controller.rx'       : 0,
#    'bluetooth.controller.tx'       : 0,
#    'bluetooth.controller.voltage'  : 0,
#
#    'modem.controller.idle'     : 0,
#    'modem.controller.rx'       : 0,
#    'modem.controller.tx'       : 0,
#    'modem.controller.voltage'  : 0,
#
#    'wifi.controller.idle'      : 0,
#    'wifi.controller.rx'        : 0,
#    'wifi.controller.tx'        : 0,
#    'wifi.controller.voltage'   : 0,
}

power_profile_generator = PowerProfileGenerator(my_emeter, my_datasheet)
print power_profile_generator.get()
