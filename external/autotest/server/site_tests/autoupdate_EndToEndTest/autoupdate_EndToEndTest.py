# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import collections
import json
import logging
import os
import urlparse

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import dev_server
from autotest_lib.server.cros.update_engine import chromiumos_test_platform
from autotest_lib.server.cros.update_engine import update_engine_test


def snippet(text):
    """Returns the text with start/end snip markers around it.

    @param text: The snippet text.

    @return The text with start/end snip markers around it.
    """
    snip = '---8<---' * 10
    start = '-- START -'
    end = '-- END -'
    return ('%s%s\n%s\n%s%s' %
            (start, snip[len(start):], text, end, snip[len(end):]))


class autoupdate_EndToEndTest(update_engine_test.UpdateEngineTest):
    """Complete update test between two Chrome OS releases.

    Performs an end-to-end test of updating a ChromeOS device from one version
    to another. The test performs the following steps:

      1. Stages the source (full) and target update payload on the central
         devserver.
      2. Installs a source image on the DUT (if provided) and reboots to it.
      3. Then starts the target update by calling cros_au RPC on the devserver.
      4. This call copies the devserver code and all payloads to the DUT.
      5. Starts a devserver on the DUT.
      6. Starts an update pointing to this localhost devserver.
      7. Watches as the DUT applies the update to rootfs and stateful.
      8. Reboots and repeats steps 5-6, ensuring that the next update check
         shows the new image version.
      9. Returns the hostlogs collected during each update check for
         verification against expected update events.

    This class interacts with several others:
    UpdateEngineTest: base class for comparing expected update events against
                      the events listed in the hostlog.
    UpdateEngineEvent: class representing a single expected update engine event.
    ChromiumOSTestPlatform: A class representing the Chrome OS device we are
                            updating. It has functions for things the DUT can
                            do: get logs, reboot, start update etc

    The flow is like this: this class stages the payloads on
    the devserver and then controls the flow of the test. It tells
    ChromiumOSTestPlatform to start the update. When that is done updating, it
    asks UpdateEngineTest to compare the update that just completed with an
    expected update.

    Some notes on naming:
      devserver: Refers to a machine running the Chrome OS Update Devserver.
      autotest_devserver: An autotest wrapper to interact with a devserver.
                          Can be used to stage artifacts to a devserver. We
                          will also class cros_au RPC on this devserver to
                          start the update.
      staged_url's: In this case staged refers to the fact that these items
                     are available to be downloaded statically from these urls
                     e.g. 'localhost:8080/static/my_file.gz'. These are usually
                     given after staging an artifact using a autotest_devserver
                     though they can be re-created given enough assumptions.
    """
    version = 1

    # Timeout periods, given in seconds.
    _WAIT_FOR_INITIAL_UPDATE_CHECK_SECONDS = 12 * 60
    # TODO(sosa): Investigate why this needs to be so long (this used to be
    # 120 and regressed).
    _WAIT_FOR_DOWNLOAD_STARTED_SECONDS = 4 * 60
    # See https://crbug.com/731214 before changing WAIT_FOR_DOWNLOAD
    _WAIT_FOR_DOWNLOAD_COMPLETED_SECONDS = 20 * 60
    _WAIT_FOR_UPDATE_COMPLETED_SECONDS = 4 * 60
    _WAIT_FOR_UPDATE_CHECK_AFTER_REBOOT_SECONDS = 15 * 60

    _DEVSERVER_HOSTLOG_ROOTFS = 'devserver_hostlog_rootfs'
    _DEVSERVER_HOSTLOG_REBOOT = 'devserver_hostlog_reboot'

    StagedURLs = collections.namedtuple('StagedURLs', ['source_url',
                                                       'source_stateful_url',
                                                       'target_url',
                                                       'target_stateful_url'])

    def initialize(self):
        """Sets up variables that will be used by test."""
        super(autoupdate_EndToEndTest, self).initialize()
        self._host = None
        self._autotest_devserver = None

    def _get_least_loaded_devserver(self, test_conf):
        """Find a devserver to use.

        We first try to pick a devserver with the least load. In case all
        devservers' load are higher than threshold, fall back to
        the old behavior by picking a devserver based on the payload URI,
        with which ImageServer.resolve will return a random devserver based on
        the hash of the URI. The picked devserver needs to respect the
        location of the host if 'prefer_local_devserver' is set to True or
        'restricted_subnets' is  set.

        @param test_conf: a dictionary of test settings.
        """
        hostname = self._host.hostname if self._host else None
        least_loaded_devserver = dev_server.get_least_loaded_devserver(
            hostname=hostname)
        if least_loaded_devserver:
            logging.debug('Choosing the least loaded devserver: %s',
                          least_loaded_devserver)
            autotest_devserver = dev_server.ImageServer(least_loaded_devserver)
        else:
            logging.warning('No devserver meets the maximum load requirement. '
                            'Picking a random devserver to use.')
            autotest_devserver = dev_server.ImageServer.resolve(
                test_conf['target_payload_uri'], self._host.hostname)
        devserver_hostname = urlparse.urlparse(
            autotest_devserver.url()).hostname

        logging.info('Devserver chosen for this run: %s', devserver_hostname)
        return autotest_devserver


    def _stage_payloads_onto_devserver(self, test_conf):
        """Stages payloads that will be used by the test onto the devserver.

        @param test_conf: a dictionary containing payload urls to stage.

        @return a StagedURLs tuple containing the staged urls.
        """
        logging.info('Staging images onto autotest devserver (%s)',
                     self._autotest_devserver.url())

        source_uri = test_conf['source_payload_uri']
        staged_src_uri, staged_src_stateful = self._stage_payloads(
            source_uri, test_conf['source_archive_uri'])

        target_uri = test_conf['target_payload_uri']
        staged_target_uri, staged_target_stateful = self._stage_payloads(
            target_uri, test_conf['target_archive_uri'],
            test_conf['update_type'])

        return self.StagedURLs(staged_src_uri, staged_src_stateful,
                               staged_target_uri, staged_target_stateful)


    def _stage_payloads(self, payload_uri, archive_uri, payload_type='full'):
        """Stages a payload and its associated stateful on devserver."""
        if payload_uri:
            staged_uri = self._stage_payload_by_uri(payload_uri)

            # Figure out where to get the matching stateful payload.
            if archive_uri:
                stateful_uri = self._get_stateful_uri(archive_uri)
            else:
                stateful_uri = self._payload_to_stateful_uri(payload_uri)
            staged_stateful = self._stage_payload_by_uri(stateful_uri)

            logging.info('Staged %s payload from %s at %s.', payload_type,
                         payload_uri, staged_uri)
            logging.info('Staged stateful from %s at %s.', stateful_uri,
                         staged_stateful)
            return staged_uri, staged_stateful


    @staticmethod
    def _get_stateful_uri(build_uri):
        """Returns a complete GS URI of a stateful update given a build path."""
        return '/'.join([build_uri.rstrip('/'), 'stateful.tgz'])


    def _payload_to_stateful_uri(self, payload_uri):
        """Given a payload GS URI, returns the corresponding stateful URI."""
        build_uri = payload_uri.rpartition('/')[0]
        if build_uri.endswith('payloads'):
            build_uri = build_uri.rpartition('/')[0]
        return self._get_stateful_uri(build_uri)


    def _get_hostlog_file(self, filename, pid):
        """Return the hostlog file location.

        This hostlog file contains the update engine events that were fired
        during the update. The pid is returned from dev_server.auto_update().

        @param filename: The partial filename to look for.
        @param pid: The pid of the update.

        """
        hosts = [self._host.hostname, self._host.ip]
        for host in hosts:
            hostlog = '%s_%s_%s' % (filename, host, pid)
            file_url = os.path.join(self.job.resultdir,
                                    dev_server.AUTO_UPDATE_LOG_DIR,
                                    hostlog)
            if os.path.exists(file_url):
                logging.info('Hostlog file to be used for checking update '
                             'steps: %s', file_url)
                return file_url
        raise error.TestFail('Could not find %s for pid %s' % (filename, pid))


    def _dump_update_engine_log(self, test_platform):
        """Dumps relevant AU error log."""
        try:
            error_log = test_platform.get_update_log(80)
            logging.error('Dumping snippet of update_engine log:\n%s',
                          snippet(error_log))
        except Exception:
            # Mute any exceptions we get printing debug logs.
            pass


    def _report_perf_data(self, perf_file):
        """Reports performance and resource data.

        Currently, performance attributes are expected to include 'rss_peak'
        (peak memory usage in bytes).

        @param perf_file: A file with performance metrics.
        """
        logging.debug('Reading perf results from %s.', perf_file)
        try:
            with open(perf_file, 'r') as perf_file_handle:
                perf_data = json.load(perf_file_handle)
        except Exception as e:
            logging.warning('Error while reading the perf data file: %s', e)
            return

        rss_peak = perf_data.get('rss_peak')
        if rss_peak:
            rss_peak_kib = rss_peak / 1024
            logging.info('Peak memory (RSS) usage on DUT: %d KiB', rss_peak_kib)
            self.output_perf_value(description='mem_usage_peak',
                                   value=int(rss_peak_kib),
                                   units='KiB',
                                   higher_is_better=False)
        else:
            logging.warning('No rss_peak key in JSON returned by update '
                            'engine perf script.')

        update_time = perf_data.get('update_length')
        if update_time:
            logging.info('Time it took to update: %s seconds', update_time)
            self.output_perf_value(description='update_length',
                                   value=int(update_time), units='sec',
                                   higher_is_better=False)
        else:
            logging.warning('No data about how long it took to update was '
                            'found.')


    def _verify_active_slot_changed(self, source_active_slot,
                                    target_active_slot, source_release,
                                    target_release):
        """Make sure we're using a different slot after the update."""
        if target_active_slot == source_active_slot:
            err_msg = 'The active image slot did not change after the update.'
            if source_release is None:
                err_msg += (
                    ' The DUT likely rebooted into the old image, which '
                    'probably means that the payload we applied was '
                    'corrupt.')
            elif source_release == target_release:
                err_msg += (' Given that the source and target versions are '
                            'identical, either it (1) rebooted into the '
                            'old image due to a bad payload or (2) we retried '
                            'the update after it failed once and the second '
                            'attempt was written to the original slot.')
            else:
                err_msg += (' This is strange since the DUT reported the '
                            'correct target version. This is probably a system '
                            'bug; check the DUT system log.')
            raise error.TestFail(err_msg)

        logging.info('Target active slot changed as expected: %s',
                     target_active_slot)


    def _verify_version(self, expected, actual):
        """Compares actual and expected versions."""
        if expected != actual:
            err_msg = 'Failed to verify OS version. Expected %s, was %s' % (
                expected, actual)
            logging.error(err_msg)
            raise error.TestFail(err_msg)

    def run_update_test(self, cros_device, test_conf):
        """Runs the update test, collects perf stats, checks expected version.

        @param cros_device: The device under test.
        @param test_conf: A dictionary containing test configuration values.

        """
        # Record the active root partition.
        source_active_slot = cros_device.get_active_slot()
        logging.info('Source active slot: %s', source_active_slot)

        source_release = test_conf['source_release']
        target_release = test_conf['target_release']

        cros_device.copy_perf_script_to_device(self.bindir)
        try:
            # Update the DUT to the target image.
            pid = cros_device.install_target_image(test_conf[
                'target_payload_uri'])

            # Verify the hostlog of events was returned from the update.
            file_url = self._get_hostlog_file(self._DEVSERVER_HOSTLOG_ROOTFS,
                                              pid)

            # Call into base class to compare expected events against hostlog.
            self.verify_update_events(source_release, file_url)
        except:
            logging.fatal('ERROR: Failure occurred during the target update.')
            raise

        # Collect perf stats about this update run.
        perf_file = cros_device.get_perf_stats_for_update(self.job.resultdir)
        if perf_file is not None:
            self._report_perf_data(perf_file)

        # Device is updated. Check that we are running the expected version.
        if cros_device.oobe_triggers_update():
            # If DUT automatically checks for update during OOBE (e.g
            # rialto), this update check fires before the test can get the
            # post-reboot update check. So we just check the version from
            # lsb-release.
            logging.info('Skipping post reboot update check.')
            self._verify_version(target_release, cros_device.get_cros_version())
        else:
            # Verify we have a hostlog for the post-reboot update check.
            file_url = self._get_hostlog_file(self._DEVSERVER_HOSTLOG_REBOOT,
                                              pid)

            # Call into base class to compare expected events against hostlog.
            self.verify_update_events(source_release, file_url, target_release)

        self._verify_active_slot_changed(source_active_slot,
                                         cros_device.get_active_slot(),
                                         source_release, target_release)

        logging.info('Update successful, test completed')


    def run_once(self, host, test_conf):
        """Performs a complete auto update test.

        @param host: a host object representing the DUT.
        @param test_conf: a dictionary containing test configuration values.

        @raise error.TestError if anything went wrong with setting up the test;
               error.TestFail if any part of the test has failed.
        """
        self._host = host
        logging.debug('The test configuration supplied: %s', test_conf)

        self._autotest_devserver = self._get_least_loaded_devserver(test_conf)
        self._stage_payloads_onto_devserver(test_conf)

        # Get an object representing the CrOS DUT.
        cros_device = chromiumos_test_platform.ChromiumOSTestPlatform(
            self._host, self._autotest_devserver, self.job.resultdir)

        # Install source image
        cros_device.install_source_image(test_conf['source_payload_uri'])
        cros_device.check_login_after_source_update()

        # Start the update to the target image.
        try:
            self.run_update_test(cros_device, test_conf)
        except update_engine_test.UpdateEngineEventMissing:
            self._dump_update_engine_log(cros_device)
            raise

        # Check we can login after the update.
        cros_device.check_login_after_target_update()
