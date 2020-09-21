# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import collections
import logging
import numpy
import os
import time

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import arc
from autotest_lib.client.common_lib.cros import arc_common
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.common_lib.cros import power_load_util
from autotest_lib.client.common_lib.cros.network import interface
from autotest_lib.client.common_lib.cros.network import xmlrpc_datatypes
from autotest_lib.client.common_lib.cros.network import xmlrpc_security_types
from autotest_lib.client.cros import backchannel, httpd
from autotest_lib.client.cros import memory_bandwidth_logger
from autotest_lib.client.cros import service_stopper
from autotest_lib.client.cros.audio import audio_helper
from autotest_lib.client.cros.networking import wifi_proxy
from autotest_lib.client.cros.power import power_dashboard
from autotest_lib.client.cros.power import power_rapl
from autotest_lib.client.cros.power import power_status
from autotest_lib.client.cros.power import power_utils
from telemetry.core import exceptions

params_dict = {
    'test_time_ms': '_mseconds',
    'should_scroll': '_should_scroll',
    'should_scroll_up': '_should_scroll_up',
    'scroll_loop': '_scroll_loop',
    'scroll_interval_ms': '_scroll_interval_ms',
    'scroll_by_pixels': '_scroll_by_pixels',
    'tasks': '_tasks',
}

class power_LoadTest(arc.ArcTest):
    """test class"""
    version = 2

    def initialize(self, percent_initial_charge_min=None,
                 check_network=True, loop_time=3600, loop_count=1,
                 should_scroll='true', should_scroll_up='true',
                 scroll_loop='false', scroll_interval_ms='10000',
                 scroll_by_pixels='600', test_low_batt_p=3,
                 verbose=True, force_wifi=False, wifi_ap='', wifi_sec='none',
                 wifi_pw='', wifi_timeout=60, tasks='',
                 volume_level=10, mic_gain=10, low_batt_margin_p=2,
                 ac_ok=False, log_mem_bandwidth=False, gaia_login=True):
        """
        percent_initial_charge_min: min battery charge at start of test
        check_network: check that Ethernet interface is not running
        loop_time: length of time to run the test for in each loop
        loop_count: number of times to loop the test for
        should_scroll: should the extension scroll pages
        should_scroll_up: should scroll in up direction
        scroll_loop: continue scrolling indefinitely
        scroll_interval_ms: how often to scoll
        scroll_by_pixels: number of pixels to scroll each time
        test_low_batt_p: percent battery at which test should stop
        verbose: add more logging information
        force_wifi: should we force to test to run on wifi
        wifi_ap: the name (ssid) of the wifi access point
        wifi_sec: the type of security for the wifi ap
        wifi_pw: password for the wifi ap
        wifi_timeout: The timeout for wifi configuration
        volume_level: percent audio volume level
        mic_gain: percent audio microphone gain level
        low_batt_margin_p: percent low battery margin to be added to
            sys_low_batt_p to guarantee test completes prior to powerd shutdown
        ac_ok: boolean to allow running on AC
        log_mem_bandwidth: boolean to log memory bandwidth during the test
        gaia_login: boolean of whether real GAIA login should be attempted.
        """
        self._backlight = None
        self._services = None
        self._browser = None
        self._loop_time = loop_time
        self._loop_count = loop_count
        self._mseconds = self._loop_time * 1000
        self._verbose = verbose

        self._sys_low_batt_p = 0.
        self._sys_low_batt_s = 0.
        self._test_low_batt_p = test_low_batt_p
        self._should_scroll = should_scroll
        self._should_scroll_up = should_scroll_up
        self._scroll_loop = scroll_loop
        self._scroll_interval_ms = scroll_interval_ms
        self._scroll_by_pixels = scroll_by_pixels
        self._tmp_keyvals = {}
        self._power_status = None
        self._force_wifi = force_wifi
        self._testServer = None
        self._tasks = tasks.replace(' ','')
        self._backchannel = None
        self._shill_proxy = None
        self._volume_level = volume_level
        self._mic_gain = mic_gain
        self._ac_ok = ac_ok
        self._log_mem_bandwidth = log_mem_bandwidth
        self._wait_time = 60
        self._stats = collections.defaultdict(list)
        self._gaia_login = gaia_login

        if not power_utils.has_battery():
            if ac_ok and (power_utils.has_powercap_support() or
                          power_utils.has_rapl_support()):
                logging.info("Device has no battery but has powercap data.")
            else:
                rsp = "Skipping test for device without battery and powercap."
                raise error.TestNAError(rsp)
        self._power_status = power_status.get_status()
        self._tmp_keyvals['b_on_ac'] = self._power_status.on_ac()

        self._username = power_load_util.get_username()
        self._password = power_load_util.get_password()

        if not ac_ok:
            self._power_status.assert_battery_state(percent_initial_charge_min)
        # If force wifi enabled, convert eth0 to backchannel and connect to the
        # specified WiFi AP.
        if self._force_wifi:
            sec_config = None
            # TODO(dbasehore): Fix this when we get a better way of figuring out
            # the wifi security configuration.
            if wifi_sec == 'rsn' or wifi_sec == 'wpa':
                sec_config = xmlrpc_security_types.WPAConfig(
                        psk=wifi_pw,
                        wpa_mode=xmlrpc_security_types.WPAConfig.MODE_PURE_WPA2,
                        wpa2_ciphers=
                                [xmlrpc_security_types.WPAConfig.CIPHER_CCMP])
            wifi_config = xmlrpc_datatypes.AssociationParameters(
                    ssid=wifi_ap, security_config=sec_config,
                    configuration_timeout=wifi_timeout)
            # If backchannel is already running, don't run it again.
            self._backchannel = backchannel.Backchannel()
            if not self._backchannel.setup():
                raise error.TestError('Could not setup Backchannel network.')

            self._shill_proxy = wifi_proxy.WifiProxy()
            self._shill_proxy.remove_all_wifi_entries()
            for i in xrange(1,4):
                raw_output = self._shill_proxy.connect_to_wifi_network(
                        wifi_config.ssid,
                        wifi_config.security,
                        wifi_config.security_parameters,
                        wifi_config.save_credentials,
                        station_type=wifi_config.station_type,
                        hidden_network=wifi_config.is_hidden,
                        discovery_timeout_seconds=
                                wifi_config.discovery_timeout,
                        association_timeout_seconds=
                                wifi_config.association_timeout,
                        configuration_timeout_seconds=
                                wifi_config.configuration_timeout * i)
                result = xmlrpc_datatypes.AssociationResult. \
                        from_dbus_proxy_output(raw_output)
                if result.success:
                    break
                logging.warn('wifi connect: disc:%d assoc:%d config:%d fail:%s',
                             result.discovery_time, result.association_time,
                             result.configuration_time, result.failure_reason)
            else:
                raise error.TestError('Could not connect to WiFi network.')

        else:
            # Find all wired ethernet interfaces.
            ifaces = [ iface for iface in interface.get_interfaces()
                if (not iface.is_wifi_device() and
                    iface.name.find('eth') != -1) ]
            logging.debug(str([iface.name for iface in ifaces]))
            for iface in ifaces:
                if check_network and iface.is_lower_up:
                    raise error.TestError('Ethernet interface is active. ' +
                                          'Please remove Ethernet cable')

        # record the max backlight level
        self._backlight = power_utils.Backlight()
        self._tmp_keyvals['level_backlight_max'] = \
            self._backlight.get_max_level()

        self._services = service_stopper.ServiceStopper(
            service_stopper.ServiceStopper.POWER_DRAW_SERVICES)
        self._services.stop_services()

        # fix up file perms for the power test extension so that chrome
        # can access it
        os.system('chmod -R 755 %s' % self.bindir)

        # setup a HTTP Server to listen for status updates from the power
        # test extension
        self._testServer = httpd.HTTPListener(8001, docroot=self.bindir)
        self._testServer.run()

        # initialize various interesting power related stats
        self._statomatic = power_status.StatoMatic()

        self._power_status.refresh()
        help_output = utils.system_output('check_powerd_config --help')
        if 'low_battery_shutdown' in help_output:
          logging.info('Have low_battery_shutdown option')
          self._sys_low_batt_p = float(utils.system_output(
                  'check_powerd_config --low_battery_shutdown_percent'))
          self._sys_low_batt_s = int(utils.system_output(
                  'check_powerd_config --low_battery_shutdown_time'))
        else:
          # TODO(dchan) Once M57 in stable, remove this option and function.
          logging.info('No low_battery_shutdown option')
          (self._sys_low_batt_p, self._sys_low_batt_s) = \
              self._get_sys_low_batt_values_from_log()

        if self._sys_low_batt_p and self._sys_low_batt_s:
            raise error.TestError(
                    "Low battery percent and seconds are non-zero.")

        min_low_batt_p = min(self._sys_low_batt_p + low_batt_margin_p, 100)
        if self._sys_low_batt_p and (min_low_batt_p > self._test_low_batt_p):
            logging.warning("test low battery threshold is below system " +
                         "low battery requirement.  Setting to %f",
                         min_low_batt_p)
            self._test_low_batt_p = min_low_batt_p

        if self._power_status.battery:
            self._ah_charge_start = self._power_status.battery[0].charge_now
            self._wh_energy_start = self._power_status.battery[0].energy


    def run_once(self):
        """Test main loop."""
        t0 = time.time()

        # record the PSR related info.
        psr = power_utils.DisplayPanelSelfRefresh(init_time=t0)

        try:
            self._keyboard_backlight = power_utils.KbdBacklight()
            self._set_keyboard_backlight_level()
        except power_utils.KbdBacklightException as e:
            logging.info("Assuming no keyboard backlight due to :: %s", str(e))
            self._keyboard_backlight = None

        measurements = []
        if self._power_status.battery:
            measurements += \
                    [power_status.SystemPower(self._power_status.battery_path)]
        if power_utils.has_powercap_support():
            measurements += power_rapl.create_powercap()
        elif power_utils.has_rapl_support():
            measurements += power_rapl.create_rapl()
        self._plog = power_status.PowerLogger(measurements, seconds_period=20)
        self._tlog = power_status.TempLogger([], seconds_period=20)
        self._plog.start()
        self._tlog.start()
        if self._log_mem_bandwidth:
            self._mlog = memory_bandwidth_logger.MemoryBandwidthLogger(
                raw=False, seconds_period=2)
            self._mlog.start()

        ext_path = os.path.join(os.path.dirname(__file__), 'extension')
        self._tmp_keyvals['username'] = self._username

        arc_mode = arc_common.ARC_MODE_DISABLED
        if utils.is_arc_available():
            arc_mode = arc_common.ARC_MODE_ENABLED

        try:
            self._browser = chrome.Chrome(extension_paths=[ext_path],
                                          gaia_login=self._gaia_login,
                                          username=self._username,
                                          password=self._password,
                                          arc_mode=arc_mode)
        except exceptions.LoginException:
            # already failed guest login
            if not self._gaia_login:
                raise
            self._gaia_login = False
            logging.warn("Unable to use GAIA acct %s.  Using GUEST instead.\n",
                         self._username)
            self._browser = chrome.Chrome(extension_paths=[ext_path],
                                          gaia_login=self._gaia_login)
        if not self._gaia_login:
            self._tmp_keyvals['username'] = 'GUEST'

        extension = self._browser.get_extension(ext_path)
        for k in params_dict:
            if getattr(self, params_dict[k]) is not '':
                extension.ExecuteJavaScript('var %s = %s;' %
                                            (k, getattr(self, params_dict[k])))

        # This opens a trap start page to capture tabs opened for first login.
        # It will be closed when startTest is run.
        extension.ExecuteJavaScript('chrome.windows.create(null, null);')

        for i in range(self._loop_count):
            start_time = time.time()
            extension.ExecuteJavaScript('startTest();')
            # the power test extension will report its status here
            latch = self._testServer.add_wait_url('/status')

            # this starts a thread in the server that listens to log
            # information from the script
            script_logging = self._testServer.add_wait_url(url='/log')

            # dump any log entry that comes from the script into
            # the debug log
            self._testServer.add_url_handler(url='/log',\
                handler_func=(lambda handler, forms, loop_counter=i:\
                    _extension_log_handler(handler, forms, loop_counter)))

            pagelt_tracking = self._testServer.add_wait_url(url='/pagelt')

            self._testServer.add_url_handler(url='/pagelt',\
                handler_func=(lambda handler, forms, tracker=self, loop_counter=i:\
                    _extension_page_load_info_handler(handler, forms, loop_counter, self)))

            # reset backlight level since powerd might've modified it
            # based on ambient light
            self._set_backlight_level()
            self._set_lightbar_level()
            if self._keyboard_backlight:
                self._set_keyboard_backlight_level()
            audio_helper.set_volume_levels(self._volume_level,
                                           self._mic_gain)

            low_battery = self._do_wait(self._verbose, self._loop_time,
                                        latch)

            script_logging.set();
            pagelt_tracking.set();
            self._plog.checkpoint('loop%d' % (i), start_time)
            self._tlog.checkpoint('loop%d' % (i), start_time)
            if self._verbose:
                logging.debug('loop %d completed', i)

            if low_battery:
                logging.info('Exiting due to low battery')
                break

        # done with logging from the script, so we can collect that thread
        t1 = time.time()
        psr.refresh()
        self._tmp_keyvals['minutes_battery_life_tested'] = (t1 - t0) / 60
        self._tmp_keyvals.update(psr.get_keyvals())


    def postprocess_iteration(self):
        """Postprocess: write keyvals / log and send data to power dashboard."""
        def _log_stats(prefix, stats):
            if not len(stats):
                return
            np = numpy.array(stats)
            logging.debug("%s samples: %d", prefix, len(np))
            logging.debug("%s mean:    %.2f", prefix, np.mean())
            logging.debug("%s stdev:   %.2f", prefix, np.std())
            logging.debug("%s max:     %.2f", prefix, np.max())
            logging.debug("%s min:     %.2f", prefix, np.min())


        def _log_per_loop_stats():
            samples_per_loop = self._loop_time / self._wait_time + 1
            for kname in self._stats:
                start_idx = 0
                loop = 1
                for end_idx in xrange(samples_per_loop, len(self._stats[kname]),
                                      samples_per_loop):
                    _log_stats("%s loop %d" % (kname, loop),
                               self._stats[kname][start_idx:end_idx])
                    loop += 1
                    start_idx = end_idx


        def _log_all_stats():
            for kname in self._stats:
                _log_stats(kname, self._stats[kname])


        keyvals = self._plog.calc()
        keyvals.update(self._tlog.calc())
        keyvals.update(self._statomatic.publish())

        if self._log_mem_bandwidth:
            self._mlog.stop()
            self._mlog.join()

        _log_all_stats()
        _log_per_loop_stats()

        # record battery stats
        if self._power_status.battery:
            keyvals['a_current_now'] = self._power_status.battery[0].current_now
            keyvals['ah_charge_full'] = \
                    self._power_status.battery[0].charge_full
            keyvals['ah_charge_full_design'] = \
                    self._power_status.battery[0].charge_full_design
            keyvals['ah_charge_start'] = self._ah_charge_start
            keyvals['ah_charge_now'] = self._power_status.battery[0].charge_now
            keyvals['ah_charge_used'] = keyvals['ah_charge_start'] - \
                                        keyvals['ah_charge_now']
            keyvals['wh_energy_start'] = self._wh_energy_start
            keyvals['wh_energy_now'] = self._power_status.battery[0].energy
            keyvals['wh_energy_used'] = keyvals['wh_energy_start'] - \
                                        keyvals['wh_energy_now']
            keyvals['v_voltage_min_design'] = \
                    self._power_status.battery[0].voltage_min_design
            keyvals['wh_energy_full_design'] = \
                    self._power_status.battery[0].energy_full_design
            keyvals['v_voltage_now'] = self._power_status.battery[0].voltage_now

        keyvals.update(self._tmp_keyvals)

        keyvals['percent_sys_low_battery'] = self._sys_low_batt_p
        keyvals['seconds_sys_low_battery'] = self._sys_low_batt_s
        voltage_np = numpy.array(self._stats['v_voltage_now'])
        voltage_mean = voltage_np.mean()
        keyvals['v_voltage_mean'] = voltage_mean

        keyvals['wh_energy_powerlogger'] = \
                             self._energy_use_from_powerlogger(keyvals)

        if not self._power_status.on_ac() and keyvals['ah_charge_used'] > 0:
            # For full runs, we should use charge to scale for battery life,
            # since the voltage swing is accounted for.
            # For short runs, energy will be a better estimate.
            if self._loop_count > 1:
                estimated_reps = (keyvals['ah_charge_full_design'] /
                                  keyvals['ah_charge_used'])
            else:
                estimated_reps = (keyvals['wh_energy_full_design'] /
                                  keyvals['wh_energy_powerlogger'])

            bat_life_scale =  estimated_reps * \
                              ((100 - keyvals['percent_sys_low_battery']) / 100)

            keyvals['minutes_battery_life'] = bat_life_scale * \
                keyvals['minutes_battery_life_tested']
            # In the case where sys_low_batt_s is non-zero subtract those
            # minutes from the final extrapolation.
            if self._sys_low_batt_s:
                keyvals['minutes_battery_life'] -= self._sys_low_batt_s / 60

            keyvals['a_current_rate'] = keyvals['ah_charge_used'] * 60 / \
                                        keyvals['minutes_battery_life_tested']
            keyvals['w_energy_rate'] = keyvals['wh_energy_used'] * 60 / \
                                       keyvals['minutes_battery_life_tested']
            if self._gaia_login:
                self.output_perf_value(description='minutes_battery_life',
                                       value=keyvals['minutes_battery_life'],
                                       units='minutes',
                                       higher_is_better=True)

        if not self._gaia_login:
            keyvals = dict(map(lambda (key, value):
                               ('INVALID_' + str(key), value), keyvals.items()))
        else:
            for key, value in keyvals.iteritems():
                if key.startswith('percent_cpuidle') and \
                   key.endswith('C0_time'):
                    self.output_perf_value(description=key,
                                           value=value,
                                           units='percent',
                                           higher_is_better=False)

        self.write_perf_keyval(keyvals)
        self._plog.save_results(self.resultsdir)
        self._tlog.save_results(self.resultsdir)
        pdash = power_dashboard.PowerLoggerDashboard( \
                self._plog, self.tagged_testname, self.resultsdir)
        pdash.upload()


    def cleanup(self):
        if self._backlight:
            self._backlight.restore()
        if self._services:
            self._services.restore_services()

        # cleanup backchannel interface
        # Prevent wifi congestion in test lab by forcing machines to forget the
        # wifi AP we connected to at the start of the test.
        if self._shill_proxy:
            self._shill_proxy.remove_all_wifi_entries()
        if self._backchannel:
            self._backchannel.teardown()
        if self._browser:
            self._browser.close()
        if self._testServer:
            self._testServer.stop()


    def _do_wait(self, verbose, seconds, latch):
        latched = False
        low_battery = False
        total_time = seconds + self._wait_time
        elapsed_time = 0

        while elapsed_time < total_time:
            time.sleep(self._wait_time)
            elapsed_time += self._wait_time

            self._power_status.refresh()

            if not self._ac_ok and self._power_status.on_ac():
                raise error.TestError('Running on AC power now.')

            if self._power_status.battery:
                charge_now = self._power_status.battery[0].charge_now
                energy_rate = self._power_status.battery[0].energy_rate
                voltage_now = self._power_status.battery[0].voltage_now
                self._stats['w_energy_rate'].append(energy_rate)
                self._stats['v_voltage_now'].append(voltage_now)
                if verbose:
                    logging.debug('ah_charge_now %f', charge_now)
                    logging.debug('w_energy_rate %f', energy_rate)
                    logging.debug('v_voltage_now %f', voltage_now)

                low_battery = (self._power_status.percent_current_charge() <
                               self._test_low_batt_p)

            latched = latch.is_set()

            if latched or low_battery:
                break

        if latched:
            # record chrome power extension stats
            form_data = self._testServer.get_form_entries()
            logging.debug(form_data)
            for e in form_data:
                key = 'ext_' + e
                if key in self._tmp_keyvals:
                    self._tmp_keyvals[key] += "_%s" % form_data[e]
                else:
                    self._tmp_keyvals[key] = form_data[e]
        else:
            logging.debug("Didn't get status back from power extension")

        return low_battery


    def _set_backlight_level(self):
        self._backlight.set_default()
        # record brightness level
        self._tmp_keyvals['level_backlight_current'] = \
            self._backlight.get_level()


    def _set_lightbar_level(self, level='off'):
        """Set lightbar level.

        Args:
          level: string value to set lightbar to.  See ectool for more details.
        """
        rv = utils.system('which ectool', ignore_status=True)
        if rv:
            return
        rv = utils.system('ectool lightbar %s' % level, ignore_status=True)
        if rv:
            logging.info('Assuming no lightbar due to non-zero exit status')
        else:
            logging.info('Setting lightbar to %s', level)
            self._tmp_keyvals['level_lightbar_current'] = level


    def _get_sys_low_batt_values_from_log(self):
        """Determine the low battery values for device and return.

        2012/11/01: power manager (powerd.cc) parses parameters in filesystem
          and outputs a log message like:

           [1101/173837:INFO:powerd.cc(258)] Using low battery time threshold
                     of 0 secs and using low battery percent threshold of 3.5

           It currently checks to make sure that only one of these values is
           defined.

        Returns:
          Tuple of (percent, seconds)
            percent: float of low battery percentage
            seconds: float of low battery seconds

        """
        split_re = 'threshold of'

        powerd_log = '/var/log/power_manager/powerd.LATEST'
        cmd = 'grep "low battery time" %s' % powerd_log
        line = utils.system_output(cmd)
        secs = float(line.split(split_re)[1].split()[0])
        percent = float(line.split(split_re)[2].split()[0])
        return (percent, secs)


    def _has_light_sensor(self):
        """
        Determine if there is a light sensor on the board.

        @returns True if this host has a light sensor or
                 False if it does not.
        """
        # If the command exits with a failure status,
        # we do not have a light sensor
        cmd = 'check_powerd_config --ambient_light_sensor'
        result = utils.run(cmd, ignore_status=True)
        if result.exit_status:
            logging.debug('Ambient light sensor not present')
            return False
        logging.debug('Ambient light sensor present')
        return True


    def _energy_use_from_powerlogger(self, keyval):
        """
        Calculates the energy use, in Wh, used over the course of the run as
        reported by the PowerLogger.

        Args:
          keyval: the dictionary of keyvals containing PowerLogger output

        Returns:
          energy_wh: total energy used over the course of this run

        """
        energy_wh = 0
        loop = 0
        while True:
            duration_key = 'loop%d_system_duration' % loop
            avg_power_key = 'loop%d_system_pwr_avg' % loop
            if duration_key not in keyval or avg_power_key not in keyval:
                break
            energy_wh += keyval[duration_key] * keyval[avg_power_key] / 3600
            loop += 1
        return energy_wh


    def _has_hover_detection(self):
        """
        Checks if hover is detected by the device.

        Returns:
            Returns True if the hover detection support is enabled.
            Else returns false.
        """

        cmd = 'check_powerd_config --hover_detection'
        result = utils.run(cmd, ignore_status=True)
        if result.exit_status:
            logging.debug('Hover not present')
            return False
        logging.debug('Hover present')
        return True


    def _set_keyboard_backlight_level(self):
        """
        Sets keyboard backlight based on light sensor and hover.
        These values are based on UMA as mentioned in
        https://bugs.chromium.org/p/chromium/issues/detail?id=603233#c10

        ALS  | hover | keyboard backlight level
        ---------------------------------------
        No   | No    | default
        ---------------------------------------
        Yes  | No    | 40% of default
        --------------------------------------
        No   | Yes   | System with this configuration does not exist
        --------------------------------------
        Yes  | Yes   | 30% of default
        --------------------------------------

        Here default is no Ambient Light Sensor, no hover,
        default always-on brightness level.
        """

        default_level = self._keyboard_backlight.get_default_level()
        level_to_set = default_level
        has_light_sensor = self._has_light_sensor()
        has_hover = self._has_hover_detection()
        # TODO(ravisadineni):if (crbug: 603233) becomes default
        # change this to reflect it.
        if has_light_sensor and has_hover:
            level_to_set = (30 * default_level) / 100
        elif has_light_sensor:
            level_to_set = (40 * default_level) / 100
        elif has_hover:
            logging.warn('Device has hover but no light sensor')

        logging.info('Setting keyboard backlight to %d', level_to_set)
        self._keyboard_backlight.set_level(level_to_set)
        self._tmp_keyvals['percent_kbd_backlight'] = \
            self._keyboard_backlight.get_percent()

def _extension_log_handler(handler, form, loop_number):
    """
    We use the httpd library to allow us to log whatever we
    want from the extension JS script into the log files.

    This method is provided to the server as a handler for
    all requests that come for the log url in the testServer

    unused parameter, because httpd passes the server itself
    into the handler.
    """
    if form:
        for field in form.keys():
            logging.debug("[extension] @ loop_%d %s", loop_number,
            form[field].value)
            # we don't want to add url information to our keyvals.
            # httpd adds them automatically so we remove them again
            del handler.server._form_entries[field]

def _extension_page_load_info_handler(handler, form, loop_number, tracker):
    stats_ids = ['mean', 'min', 'max', 'std']
    time_measurements = []
    sorted_pagelt = []
    #show up to this number of slow page-loads
    num_slow_page_loads = 5;

    if not form:
        logging.debug("no page load information returned")
        return;

    for field in form.keys():
        url = field[str.find(field, "http"):]
        load_time = int(form[field].value)

        time_measurements.append(load_time)
        sorted_pagelt.append((url, load_time))

        logging.debug("[extension] @ loop_%d url: %s load time: %d ms",
            loop_number, url, load_time)
        # we don't want to add url information to our keyvals.
        # httpd adds them automatically so we remove them again
        del handler.server._form_entries[field]

    time_measurements = numpy.array(time_measurements)
    stats_vals = [time_measurements.mean(), time_measurements.min(),
    time_measurements.max(),time_measurements.std()]

    key_base = 'ext_ms_page_load_time_'
    for i in range(len(stats_ids)):
        key = key_base + stats_ids[i]
        if key in tracker._tmp_keyvals:
            tracker._tmp_keyvals[key] += "_%.2f" % stats_vals[i]
        else:
            tracker._tmp_keyvals[key] = "%.2f" % stats_vals[i]


    sorted_pagelt.sort(key=lambda item: item[1], reverse=True)

    message = "The %d slowest page-load-times are:\n" % (num_slow_page_loads)
    for url, msecs in sorted_pagelt[:num_slow_page_loads]:
        message += "\t%s w/ %d ms" % (url, msecs)

    logging.debug("%s\n", message)
