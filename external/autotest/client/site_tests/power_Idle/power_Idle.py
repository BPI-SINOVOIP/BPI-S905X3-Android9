# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, time

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros import service_stopper
from autotest_lib.client.cros.bluetooth import bluetooth_device_xmlrpc_server
from autotest_lib.client.cros.power import power_dashboard
from autotest_lib.client.cros.power import power_rapl
from autotest_lib.client.cros.power import power_status
from autotest_lib.client.cros.power import power_utils


class power_Idle(test.test):
    """class for power_Idle test.

    Collects power stats when machine is idle, also compares power stats between
    when bluetooth adapter is on and off.
    """
    version = 1

    def initialize(self):
        """Perform necessary initialization prior to test run.

        Private Attributes:
          _backlight: power_utils.Backlight object
          _services: service_stopper.ServiceStopper object
        """
        super(power_Idle, self).initialize()
        self._backlight = None
        self._services = service_stopper.ServiceStopper(
            service_stopper.ServiceStopper.POWER_DRAW_SERVICES)
        self._services.stop_services()


    def warmup(self, warmup_time=60):
        """Warm up.

        Wait between initialization and run_once for new settings to stabilize.
        """
        time.sleep(warmup_time)


    def run_once(self, idle_time=120, sleep=10, bt_warmup_time=20):
        """Collect power stats when bluetooth adapter is on or off.

        """
        with chrome.Chrome():
            self._backlight = power_utils.Backlight()
            self._backlight.set_default()

            t0 = time.time()
            self._start_time = t0
            self._psr = power_utils.DisplayPanelSelfRefresh(init_time=t0)
            self.status = power_status.get_status()
            self._stats = power_status.StatoMatic()

            measurements = []
            if not self.status.on_ac():
                measurements.append(
                    power_status.SystemPower(self.status.battery_path))
            if power_utils.has_powercap_support():
                measurements += power_rapl.create_powercap()
            elif power_utils.has_rapl_support():
                measurements += power_rapl.create_rapl()
            self._plog = power_status.PowerLogger(measurements,
                                                  seconds_period=sleep)
            self._tlog = power_status.TempLogger([], seconds_period=sleep)
            self._plog.start()
            self._tlog.start()

            for _ in xrange(0, idle_time, sleep):
                time.sleep(sleep)
                self.status.refresh()
            self.status.refresh()
            self._plog.checkpoint('bluetooth_adapter_off', self._start_time)
            self._tlog.checkpoint('', self._start_time)
            self._psr.refresh()

            # Turn on bluetooth adapter.
            bt_device = bluetooth_device_xmlrpc_server \
                    .BluetoothDeviceXmlRpcDelegate()
            # If we cannot start bluetoothd, fail gracefully and still write
            # data with bluetooth adapter off to file, as we are interested in
            # just that data too. start_bluetoothd() already logs the error so
            # not logging any error here.
            if not bt_device.start_bluetoothd():
                return
            if not bt_device.set_powered(True):
                logging.warning("Cannot turn on bluetooth adapter.")
                return
            time.sleep(bt_warmup_time)
            if not bt_device._is_powered_on():
                logging.warning("Bluetooth adapter is off.")
                return
            t1 = time.time()
            time.sleep(idle_time)
            self._plog.checkpoint('bluetooth_adapter_on', t1)
            bt_device.set_powered(False)
            bt_device.stop_bluetoothd()


    def _publish_chromeperf_dashboard(self, measurements):
        """Report to chromeperf dashboard."""
        publish = {key: measurements[key]
                   for key in measurements.keys() if key.endswith('pwr')}

        for key, values in publish.iteritems():
            self.output_perf_value(description=key, value=values,
                units='W', higher_is_better=False, graph='power')


    def postprocess_iteration(self):
        """Write power stats to file.

        """
        keyvals = self._stats.publish()

        # record the current and max backlight levels
        self._backlight = power_utils.Backlight()
        keyvals['level_backlight_max'] = self._backlight.get_max_level()
        keyvals['level_backlight_current'] = self._backlight.get_level()

        # record battery stats if not on AC
        if self.status.on_ac():
            keyvals['b_on_ac'] = 1
        else:
            keyvals['b_on_ac'] = 0
            keyvals['ah_charge_full'] = self.status.battery[0].charge_full
            keyvals['ah_charge_full_design'] = \
                                self.status.battery[0].charge_full_design
            keyvals['ah_charge_now'] = self.status.battery[0].charge_now
            keyvals['a_current_now'] = self.status.battery[0].current_now
            keyvals['wh_energy'] = self.status.battery[0].energy
            keyvals['w_energy_rate'] = self.status.battery[0].energy_rate
            keyvals['h_remaining_time'] = self.status.battery[0].remaining_time
            keyvals['v_voltage_min_design'] = \
                                self.status.battery[0].voltage_min_design
            keyvals['v_voltage_now'] = self.status.battery[0].voltage_now

        plog_keyvals = self._plog.calc()
        self._publish_chromeperf_dashboard(plog_keyvals)
        keyvals.update(plog_keyvals)
        keyvals.update(self._tlog.calc())
        keyvals.update(self._psr.get_keyvals())
        logging.debug("keyvals = %s", keyvals)

        self.write_perf_keyval(keyvals)

        pdash = power_dashboard.PowerLoggerDashboard(
                self._plog, self.tagged_testname, self.resultsdir)
        pdash.upload()


    def cleanup(self):
        """Reverse setting change in initialization.

        """
        if self._backlight:
            self._backlight.restore()
        self._services.restore_services()
        super(power_Idle, self).cleanup()
