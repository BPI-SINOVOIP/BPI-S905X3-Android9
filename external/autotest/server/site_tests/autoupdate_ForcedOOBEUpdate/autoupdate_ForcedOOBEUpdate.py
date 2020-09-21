# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import json
import logging
import os
import random
import time

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import lsbrelease_utils
from autotest_lib.server import autotest
from autotest_lib.server.cros.update_engine import update_engine_test
from chromite.lib import retry_util

class autoupdate_ForcedOOBEUpdate(update_engine_test.UpdateEngineTest):
    """Runs a forced autoupdate during OOBE."""
    version = 1

    # We override the default lsb-release file.
    _CUSTOM_LSB_RELEASE = '/mnt/stateful_partition/etc/lsb-release'

    # Version we tell the DUT it is on before update.
    _CUSTOM_LSB_VERSION = '0.0.0.0'

    # Expected hostlog events during update: 4 during rootfs
    _ROOTFS_HOSTLOG_EVENTS = 4


    def cleanup(self):
        self._host.run('rm %s' % self._CUSTOM_LSB_RELEASE, ignore_status=True)

        # Get the last two update_engine logs: before and after reboot.
        files = self._host.run('ls -t -1 '
                               '/var/log/update_engine/').stdout.splitlines()
        for i in range(2):
            self._host.get_file('/var/log/update_engine/%s' % files[i],
                                self.resultsdir)
        cmd = 'update_engine_client --update_over_cellular=no'
        retry_util.RetryException(error.AutoservRunError, 2, self._host.run,
                                  cmd)
        super(autoupdate_ForcedOOBEUpdate, self).cleanup()

    def _get_chromeos_version(self):
        """Read the ChromeOS version from /etc/lsb-release."""
        lsb = self._host.run('cat /etc/lsb-release').stdout
        return lsbrelease_utils.get_chromeos_release_version(lsb)


    def _create_hostlog_files(self):
        """Create the two hostlog files for the update.

        To ensure the update was succesful we need to compare the update
        events against expected update events. There is a hostlog for the
        rootfs update and for the post reboot update check.
        """
        hostlog = self._omaha_devserver.get_hostlog(self._host.ip,
                                                    wait_for_reboot_events=True)
        logging.info('Hostlog: %s', hostlog)

        # File names to save the hostlog events to.
        rootfs_hostlog = os.path.join(self.resultsdir, 'hostlog_rootfs')
        reboot_hostlog = os.path.join(self.resultsdir, 'hostlog_reboot')

        # Each time we reboot in the middle of an update we ping omaha again
        # for each update event. So parse the list backwards to get the final
        # events.
        with open(reboot_hostlog, 'w') as outfile:
            json.dump(hostlog[-1:], outfile)
        with open(rootfs_hostlog, 'w') as outfile:
            json.dump(hostlog[len(hostlog)-1-self._ROOTFS_HOSTLOG_EVENTS:-1],
                      outfile)

        return rootfs_hostlog, reboot_hostlog


    def _get_update_percentage(self):
        """Returns the current payload downloaded percentage."""
        while True:
            status = self._host.run('update_engine_client --status',
                                    ignore_timeout=True,
                                    timeout=10)
            if not status:
                continue
            status = status.stdout.splitlines()
            logging.debug(status)
            if 'UPDATE_STATUS_IDLE' in status[2]:
                raise error.TestFail('Update status was idle while trying to '
                                     'get download status.')
            # If we call this right after reboot it may not be downloading yet.
            if 'UPDATE_STATUS_DOWNLOADING' not in status[2]:
                time.sleep(1)
                continue
            return float(status[1].partition('=')[2])


    def _update_continued_where_it_left_off(self, percentage):
        """
        Checks that the update did not restart after an interruption.

        @param percentage: The percentage the last time we checked.

        @returns True if update continued. False if update restarted.

        """
        completed = self._get_update_percentage()
        logging.info('New value: %f, old value: %f', completed, percentage)
        return completed >= percentage


    def _wait_for_update_to_complete(self):
        """Wait for the update that started to complete.

        Repeated check status of update. It should move from DOWNLOADING to
        FINALIZING to COMPLETE to IDLE.
        """
        while True:
            status = self._host.run('update_engine_client --status',
                                    ignore_timeout=True,
                                    timeout=10)

            # During reboot, status will be None
            if status is not None:
                status = status.stdout.splitlines()
                logging.debug(status)
                if 'UPDATE_STATUS_IDLE' in status[2]:
                    break
            time.sleep(1)


    def _wait_for_percentage(self, percent):
        """
        Waits until we reach the percentage passed as the input.

        @param percent: The percentage we want to wait for.
        """
        while True:
            completed = self._get_update_percentage()
            logging.debug('Checking if %s is greater than %s', completed,
                          percent)
            if completed > percent:
                break
            time.sleep(1)


    def run_once(self, host, full_payload=True, cellular=False,
                 interrupt=False, max_updates=1, job_repo_url=None):
        """
        Runs a forced autoupdate during ChromeOS OOBE.

        @param host: The DUT that we are running on.
        @param full_payload: True for a full payload. False for delta.
        @param cellular: True to do the update over a cellualar connection.
                         Requires that the DUT have a sim card slot.
        @param interrupt: True to interrupt the update in the middle.
        @param max_updates: Used to tell the test how many times it is
                            expected to ping its omaha server.
        @param job_repo_url: Used for debugging locally. This is used to figure
                             out the current build and the devserver to use.
                             The test will read this from a host argument
                             when run in the lab.

        """
        self._host = host

        # veyron_rialto is a medical device with a different OOBE that auto
        # completes so this test is not valid on that device.
        if 'veyron_rialto' in self._host.get_board():
            raise error.TestNAError('Rialto has a custom OOBE. Skipping test.')

        update_url = self.get_update_url_for_test(job_repo_url,
                                                  full_payload=full_payload,
                                                  critical_update=True,
                                                  cellular=cellular,
                                                  max_updates=max_updates)
        logging.info('Update url: %s', update_url)
        before = self._get_chromeos_version()
        payload_info = None
        if cellular:
            cmd = 'update_engine_client --update_over_cellular=yes'
            retry_util.RetryException(error.AutoservRunError, 2, self._host.run,
                                      cmd)
            # Get the payload's information (size, SHA256 etc) since we will be
            # setting up our own omaha instance on the DUT. We pass this to
            # the client test.
            payload = self._get_payload_url(full_payload=full_payload)
            staged_url = self._stage_payload_by_uri(payload)
            payload_info = self._get_staged_file_info(staged_url)

        # Call client test to start the forced OOBE update.
        client_at = autotest.Autotest(self._host)
        client_at.run_test('autoupdate_StartOOBEUpdate', image_url=update_url,
                           cellular=cellular, payload_info=payload_info,
                           full_payload=full_payload)

        # Don't continue the test if the client failed for any reason.
        client_at._check_client_test_result(self._host,
                                            'autoupdate_StartOOBEUpdate')

        if interrupt:
            # Choose a random downloaded percentage to interrupt the update.
            percent = random.uniform(0.1, 0.8)
            logging.debug('Percent when we will interrupt: %f', percent)
            self._wait_for_percentage(percent)
            logging.info('We will start interrupting the update.')
            completed = self._get_update_percentage()

            # Reboot the DUT during the update.
            self._host.reboot()
            if not self._update_continued_where_it_left_off(completed):
                raise error.TestFail('The update did not continue where it '
                                     'left off before rebooting.')
            completed = self._get_update_percentage()

            # Disconnect and reconnect network.
            reconnect_test_name = 'autoupdate_DisconnectReconnectNetwork'
            client_at.run_test(reconnect_test_name, update_url=update_url)
            client_at._check_client_test_result(self._host, reconnect_test_name)
            if not self._update_continued_where_it_left_off(completed):
                raise error.TestFail('The update did not continue where it '
                                     'left off before disconnecting network.')

            # Suspend / Resume
            boot_id = self._host.get_boot_id()
            self._host.servo.lid_close()
            self._host.test_wait_for_sleep()
            self._host.servo.lid_open()
            self._host.test_wait_for_boot(boot_id)
            if not self._update_continued_where_it_left_off(completed):
                raise error.TestFail('The update did not continue where it '
                                     'left off after suspend/resume.')

        self._wait_for_update_to_complete()

        if cellular:
            # We didn't have a devserver so we cannot check the hostlog to
            # ensure the update completed successfully. Instead we can check
            # that the second-to-last update engine log has the successful
            # update message. Second to last because its the one before OOBE
            # rebooted.
            update_engine_files_cmd = 'ls -t -1 /var/log/update_engine/'
            files = self._host.run(update_engine_files_cmd).stdout.splitlines()
            before_reboot_file = self._host.run('cat /var/log/update_engine/%s'
                                                % files[1]).stdout
            self._check_for_cellular_entries_in_update_log(before_reboot_file)

            success = 'Update successfully applied, waiting to reboot.'
            update_ec = self._host.run('cat /var/log/update_engine/%s | grep '
                                       '"%s"' % (files[1], success)).exit_status
            if update_ec != 0:
                raise error.TestFail('We could not verify that the update '
                                     'completed successfully. Check the logs.')
            return

        # Verify that the update completed successfully by checking hostlog.
        rootfs_hostlog, reboot_hostlog = self._create_hostlog_files()
        self.verify_update_events(self._CUSTOM_LSB_VERSION, rootfs_hostlog)
        self.verify_update_events(self._CUSTOM_LSB_VERSION, reboot_hostlog,
                                  self._CUSTOM_LSB_VERSION)

        after = self._get_chromeos_version()
        logging.info('Successfully force updated from %s to %s.', before, after)
