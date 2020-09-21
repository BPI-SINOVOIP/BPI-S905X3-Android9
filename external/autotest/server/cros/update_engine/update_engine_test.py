# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import logging
import os
import re
import update_engine_event as uee
import urlparse

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import utils
from autotest_lib.client.common_lib.cros import dev_server
from autotest_lib.server import test
from autotest_lib.server.cros.dynamic_suite import tools
from autotest_lib.server.cros.update_engine import omaha_devserver
from datetime import datetime, timedelta
from update_engine_event import UpdateEngineEvent

class UpdateEngineTest(test.test):
    """Class for comparing expected update_engine events against actual ones.

    During a rootfs update, there are several events that are fired (e.g.
    download_started, download_finished, update_started etc). Each event has
    properties associated with it that need to be verified.

    In this class we build a list of expected events (list of
    UpdateEngineEvent objects), and compare that against a "hostlog" returned
    from update_engine from the update. This hostlog is a json list of
    events fired during the update. It is accessed by the api/hostlog URL on the
    devserver during the update.

    We can also verify the hostlog of a one-time update event that is fired
    after rebooting after an update.

    During a typical autoupdate we will check both of these hostlogs.
    """
    version = 1

    # Timeout periods, given in seconds.
    _INITIAL_CHECK_TIMEOUT = 12 * 60
    _DOWNLOAD_STARTED_TIMEOUT = 4 * 60
    # See https://crbug.com/731214 before changing _DOWNLOAD_FINISHED_TIMEOUT
    _DOWNLOAD_FINISHED_TIMEOUT = 20 * 60
    _UPDATE_COMPLETED_TIMEOUT = 4 * 60
    _POST_REBOOT_TIMEOUT = 15 * 60

    # The names of the two hostlog files we will be verifying
    _DEVSERVER_HOSTLOG_ROOTFS = 'devserver_hostlog_rootfs'
    _DEVSERVER_HOSTLOG_REBOOT = 'devserver_hostlog_reboot'

    _CELLULAR_BUCKET = 'gs://chromeos-throw-away-bucket/CrOSPayloads/Cellular/'


    def initialize(self):
        self._hostlog_filename = None
        self._hostlog_events = []
        self._num_consumed_events = 0
        self._current_timestamp = None
        self._expected_events = []
        self._omaha_devserver = None
        self._host = None
        # Some AU tests use multiple DUTs
        self._hosts = None

    def cleanup(self):
        if self._omaha_devserver is not None:
            self._omaha_devserver.stop_devserver()


    def _get_expected_events_for_rootfs_update(self, source_release):
        """Creates a list of expected events fired during a rootfs update.

        There are 4 events fired during a rootfs update. We will create these
        in the correct order with the correct data, timeout, and error
        condition function.
        """
        initial_check = UpdateEngineEvent(
            version=source_release,
            on_error=self._error_initial_check)
        download_started = UpdateEngineEvent(
            event_type=uee.EVENT_TYPE_DOWNLOAD_STARTED,
            event_result=uee.EVENT_RESULT_SUCCESS,
            version=source_release,
            on_error=self._error_incorrect_event)
        download_finished = UpdateEngineEvent(
            event_type=uee.EVENT_TYPE_DOWNLOAD_FINISHED,
            event_result=uee.EVENT_RESULT_SUCCESS,
            version=source_release,
            on_error=self._error_incorrect_event)
        update_complete = UpdateEngineEvent(
            event_type=uee.EVENT_TYPE_UPDATE_COMPLETE,
            event_result=uee.EVENT_RESULT_SUCCESS,
            version=source_release,
            on_error=self._error_incorrect_event)

        # There is an error message if any of them take too long to fire.
        initial_error = self._timeout_error_message('an initial update check',
                                                    self._INITIAL_CHECK_TIMEOUT)
        dls_error = self._timeout_error_message('a download started '
                                                'notification',
                                                self._DOWNLOAD_STARTED_TIMEOUT,
                                                uee.EVENT_TYPE_DOWNLOAD_STARTED)
        dlf_error = self._timeout_error_message('a download finished '
                                                'notification',
                                                self._DOWNLOAD_FINISHED_TIMEOUT,
                                                uee.EVENT_TYPE_DOWNLOAD_FINISHED
                                                )
        uc_error = self._timeout_error_message('an update complete '
                                               'notification',
                                               self._UPDATE_COMPLETED_TIMEOUT,
                                               uee.EVENT_TYPE_UPDATE_COMPLETE)

        # Build an array of tuples (event, timeout, timeout_error_message)
        self._expected_events = [
            (initial_check, self._INITIAL_CHECK_TIMEOUT, initial_error),
            (download_started, self._DOWNLOAD_STARTED_TIMEOUT, dls_error),
            (download_finished, self._DOWNLOAD_FINISHED_TIMEOUT, dlf_error),
            (update_complete, self._UPDATE_COMPLETED_TIMEOUT, uc_error)
        ]


    def _get_expected_event_for_post_reboot_check(self, source_release,
                                                  target_release):
        """Creates the expected event fired during post-reboot update check."""
        post_reboot_check = UpdateEngineEvent(
            event_type=uee.EVENT_TYPE_REBOOTED_AFTER_UPDATE,
            event_result=uee.EVENT_RESULT_SUCCESS,
            version=target_release,
            previous_version=source_release,
            on_error=self._error_reboot_after_update)
        err = self._timeout_error_message('a successful reboot '
                                          'notification',
                                          self._POST_REBOOT_TIMEOUT,
                                          uee.EVENT_TYPE_REBOOTED_AFTER_UPDATE)

        self._expected_events = [
            (post_reboot_check, self._POST_REBOOT_TIMEOUT, err)
        ]


    def _read_hostlog_events(self):
        """Read the list of events from the hostlog json file."""
        if len(self._hostlog_events) <= self._num_consumed_events:
            try:
                with open(self._hostlog_filename, 'r') as out_log:
                    self._hostlog_events = json.loads(out_log.read())
            except Exception as e:
                raise error.TestFail('Error while reading the hostlogs '
                                     'from devserver: %s' % e)


    def _get_next_hostlog_event(self):
        """Returns the next event from the hostlog json file.

        @return The next new event in the host log
                None if no such event was found or an error occurred.
        """
        self._read_hostlog_events()
        # Return next new event, if one is found.
        if len(self._hostlog_events) > self._num_consumed_events:
            new_event = {
                key: str(val) for key, val
                in self._hostlog_events[self._num_consumed_events].iteritems()
            }
            self._num_consumed_events += 1
            logging.info('Consumed new event: %s', new_event)
            return new_event


    def _verify_event_with_timeout(self, expected_event, timeout, on_timeout):
        """Verify an expected event occurs within a given timeout.

        @param expected_event: an expected event
        @param timeout: specified in seconds
        @param on_timeout: A string to return if timeout occurs, or None.

        @return None if event complies, an error string otherwise.
        """
        actual_event = self._get_next_hostlog_event()
        if actual_event:
            # If this is the first event, set it as the current time
            if self._current_timestamp is None:
                self._current_timestamp = datetime.strptime(actual_event[
                                                                'timestamp'],
                                                            '%Y-%m-%d %H:%M:%S')

            # Get the time stamp for the current event and convert to datetime
            timestamp = actual_event['timestamp']
            event_timestamp = datetime.strptime(timestamp, '%Y-%m-%d %H:%M:%S')

            # Add the timeout onto the timestamp to get its expiry
            event_timeout = self._current_timestamp + timedelta(seconds=timeout)

            # If the event happened before the timeout
            if event_timestamp < event_timeout:
                difference = event_timestamp - self._current_timestamp
                logging.info('Event took %s seconds to fire during the '
                             'update', difference.seconds)
                result = expected_event.equals(actual_event)
                self._current_timestamp = event_timestamp
                return result

        logging.error('The expected event was not found in the hostlog: %s',
                      expected_event)
        return on_timeout


    def _error_initial_check(self, expected, actual, mismatched_attrs):
        """Error message for when update fails at initial update check."""
        err_msg = ('The update test appears to have completed successfully but '
                   'we found a problem while verifying the hostlog of events '
                   'returned from the update. Some attributes reported for '
                   'the initial update check event are not what we expected: '
                   '%s. ' % mismatched_attrs)
        if 'version' in mismatched_attrs:
            err_msg += ('The expected version is (%s) but reported version was '
                        '(%s). ' % (expected['version'], actual['version']))
            err_msg += ('If reported version = target version, it is likely '
                        'we retried the update because the test thought the '
                        'first attempt failed but it actually succeeded '
                        '(e.g due to SSH disconnect, DUT not reachable by '
                        'hostname, applying stateful failed after rootfs '
                        'succeeded). This second update attempt is then started'
                        ' from the target version instead of the source '
                        'version, so our hostlog verification is invalid.')
        err_msg += ('Check the full hostlog for this update in the %s file in '
                    'the %s directory.' % (self._DEVSERVER_HOSTLOG_ROOTFS,
                                           dev_server.AUTO_UPDATE_LOG_DIR))
        return err_msg


    def _error_incorrect_event(self, expected, actual, mismatched_attrs):
        """Error message for when an event is not what we expect."""
        return ('The update appears to have completed successfully but '
                'when analysing the update events in the hostlog we have '
                'found that one of the events is incorrect. This should '
                'never happen. The mismatched attributes are: %s. We expected '
                '%s, but got %s.' % (mismatched_attrs, expected, actual))


    def _error_reboot_after_update(self, expected, actual, mismatched_attrs):
        """Error message for problems in the post-reboot update check."""
        err_msg = ('The update completed successfully but there was a problem '
                   'with the post-reboot update check. After a successful '
                   'update, we do a final update check to parse a unique '
                   'omaha request. The mistmatched attributes for this update '
                   'check were %s. ' % mismatched_attrs)
        if 'event_result' in mismatched_attrs:
            err_msg += ('The event_result was expected to be (%s:%s) but '
                        'reported (%s:%s). ' %
                        (expected['event_result'],
                         uee.get_event_result(expected['event_result']),
                         actual.get('event_result'),
                         uee.get_event_result(actual.get('event_result'))))
        if 'event_type' in mismatched_attrs:
            err_msg += ('The event_type was expeted to be (%s:%s) but '
                        'reported (%s:%s). ' %
                        (expected['event_type'],
                         uee.get_event_type(expected['event_type']),
                         actual.get('event_type'),
                         uee.get_event_type(actual.get('event_type'))))
        if 'version' in mismatched_attrs:
            err_msg += ('The version was expected to be (%s) but '
                        'reported (%s). This probably means that the payload '
                        'we applied was incorrect or corrupt. ' %
                        (expected['version'], actual['version']))
        if 'previous_version' in mismatched_attrs:
            err_msg += ('The previous version is expected to be (%s) but '
                        'reported (%s). This can happen if we retried the '
                        'update after rootfs update completed on the first '
                        'attempt then we failed. Or if stateful got wiped and '
                        '/var/lib/update_engine/prefs/previous-version was '
                        'deleted. ' % (expected['previous_version'],
                                       actual['previous_version']))
        err_msg += ('You can see the full hostlog for this update check in '
                    'the %s file within the %s directory. ' %
                    (self._DEVSERVER_HOSTLOG_REBOOT,
                     dev_server.AUTO_UPDATE_LOG_DIR))
        return err_msg


    def _timeout_error_message(self, desc, timeout, event_type=None):
        """Error message for when an event takes too long to fire."""
        if event_type is not None:
            desc += ' (%s)' % uee.get_event_type(event_type)
        return ('The update completed successfully but one of the steps of '
                'the update took longer than we would like. We failed to '
                'receive %s within %d seconds.' % (desc, timeout))


    def _stage_payload_by_uri(self, payload_uri):
        """Stage a payload based on its GS URI.

        This infers the build's label, filename and GS archive from the
        provided GS URI.

        @param payload_uri: The full GS URI of the payload.

        @return URL of the staged payload on the server.

        @raise error.TestError if there's a problem with staging.

        """
        archive_url, _, filename = payload_uri.rpartition('/')
        build_name = urlparse.urlsplit(archive_url).path.strip('/')
        return self._stage_payload(build_name, filename,
                                   archive_url=archive_url)


    def _stage_payload(self, build_name, filename, archive_url=None):
        """Stage the given payload onto the devserver.

        Works for either a stateful or full/delta test payload. Expects the
        gs_path or a combo of build_name + filename.

        @param build_name: The build name e.g. x86-mario-release/<version>.
                           If set, assumes default gs archive bucket and
                           requires filename to be specified.
        @param filename: In conjunction with build_name, this is the file you
                         are downloading.
        @param archive_url: An optional GS archive location, if not using the
                            devserver's default.

        @return URL of the staged payload on the server.

        @raise error.TestError if there's a problem with staging.

        """
        try:
            self._autotest_devserver.stage_artifacts(image=build_name,
                                                     files=[filename],
                                                     archive_url=archive_url)
            return self._autotest_devserver.get_staged_file_url(filename,
                                                                build_name)
        except dev_server.DevServerException, e:
            raise error.TestError('Failed to stage payload: %s' % e)


    def _get_payload_url(self, build=None, full_payload=True):
        """
        Gets the GStorage URL of the full or delta payload for this build.

        @param build: build string e.g samus-release/R65-10225.0.0.
        @param full_payload: True for full payload. False for delta.

        @returns the payload URL.

        """
        if build is None:
            if self._job_repo_url is None:
                self._job_repo_url = self._get_job_repo_url()
            _, build = tools.get_devserver_build_from_package_url(
                self._job_repo_url)

        gs = dev_server._get_image_storage_server()
        if full_payload:
            # Example: chromeos_R65-10225.0.0_samus_full_dev.bin
            regex = 'chromeos_%s*_full_*' % build.rpartition('/')[2]
        else:
            # Example: chromeos_R65-10225.0.0_R65-10225.0.0_samus_delta_dev.bin
            regex = 'chromeos_%s*_delta_*' % build.rpartition('/')[2]
        payload_url_regex = gs + build + '/' + regex
        logging.debug('Trying to find payloads at %s', payload_url_regex)
        payloads = utils.gs_ls(payload_url_regex)
        if not payloads:
            raise error.TestFail('Could not find payload for %s', build)
        logging.debug('Payloads found: %s', payloads)
        return payloads[0]


    def _get_staged_file_info(self, staged_url):
        """
        Gets the staged files info that includes SHA256 and size.

        @param staged_url: the staged file url.

        @returns file info (SHA256 and size).

        """
        split_url = staged_url.rpartition('/static/')
        file_info_url = os.path.join(split_url[0], 'api/fileinfo', split_url[2])
        logging.info('file info url: %s', file_info_url)
        devserver_hostname = urlparse.urlparse(file_info_url).hostname
        cmd = 'ssh %s \'curl "%s"\'' % (devserver_hostname,
                                        utils.sh_escape(file_info_url))
        try:
            result = utils.run(cmd).stdout
            return json.loads(result)
        except error.CmdError as e:
            logging.error('Failed to read file info: %s', e)
            raise error.TestFail('Could not reach fileinfo API on devserver.')


    def _get_job_repo_url(self):
        """Gets the job_repo_url argument supplied to the test by the lab."""
        if self._hosts is not None:
            self._host = self._hosts[0]
        if self._host is None:
            raise error.TestFail('No host specified by AU test.')
        info = self._host.host_info_store.get()
        return info.attributes.get(self._host.job_repo_url_attribute, '')


    def _check_for_cellular_entries_in_update_log(self, update_engine_log):
        """
        Check update_engine.log for log entries about cellular.

        @param update_engine_log: The text of an update_engine.log file.

        """
        logging.info('Making sure we have cellular entries in update_engine '
                     'log.')
        line1 = "Allowing updates over cellular as permission preference is " \
                "set to true."
        line2 = "We are connected via cellular, Updates allowed: Yes"
        for line in [line1, line2]:
            ue = re.compile(line)
            if ue.search(update_engine_log) is None:
                raise error.TestFail('We did not find cellular string "%s" in '
                                     'the update_engine log. Please check the '
                                     'update_engine logs in the results '
                                     'directory.' % line)


    def _copy_payload_to_public_bucket(self, payload_url):
        """
        Copy payload and make link public.

        @param payload_url: Payload URL on Google Storage.

        @returns The payload URL that is now publicly accessible.

        """
        payload_filename = payload_url.rpartition('/')[2]
        utils.run('gsutil cp %s %s' % (payload_url, self._CELLULAR_BUCKET))
        new_gs_url = self._CELLULAR_BUCKET + payload_filename
        utils.run('gsutil acl ch -u AllUsers:R %s' % new_gs_url)
        return new_gs_url.replace('gs://', 'https://storage.googleapis.com/')


    def verify_update_events(self, source_release, hostlog_filename,
                             target_release=None):
        """Compares a hostlog file against a set of expected events.

        This is the main function of this class. It takes in an expected
        source and target version along with a hostlog file location. It will
        then generate the expected events based on the data and compare it
        against the events listed in the hostlog json file.
        """
        self._hostlog_events = []
        self._num_consumed_events = 0
        self._current_timestamp = None
        if target_release is not None:
            self._get_expected_event_for_post_reboot_check(source_release,
                                                           target_release)
        else:
            self._get_expected_events_for_rootfs_update(source_release)

        self._hostlog_filename = hostlog_filename
        logging.info('Checking update steps with hostlog file: %s',
                     self._hostlog_filename)

        for expected_event, timeout, on_timeout in self._expected_events:
            logging.info('Expecting %s within %s seconds', expected_event,
                         timeout)
            err_msg = self._verify_event_with_timeout(
                expected_event, timeout, on_timeout)
            if err_msg is not None:
                logging.error('Failed expected event: %s', err_msg)
                raise UpdateEngineEventMissing(err_msg)


    def get_update_url_for_test(self, job_repo_url, full_payload=True,
                                critical_update=False, max_updates=1,
                                cellular=False):
        """
        Get the correct update URL for autoupdate tests to use.

        There are bunch of different update configurations that are required
        by AU tests. Some tests need a full payload, some need a delta payload.
        Some require the omaha response to be critical or be able to handle
        multiple DUTs etc. This function returns the correct update URL to the
        test based on the inputs parameters.

        Ideally all updates would use an existing lab devserver to handle the
        updates. However the lab devservers default setup does not work for
        all test needs. So we also kick off our own omaha_devserver for the
        test run some times.

        This tests expects the test to set self._host or self._hosts.

        @param job_repo_url: string url containing the current build.
        @param full_payload: bool whether we want a full payload.
        @param critical_update: bool whether we need a critical update.
        @param max_updates: int number of updates the test will perform. This
                            is passed to src/platform/dev/devserver.py if we
                            create our own deverver.
        @param cellular: update will be done over cellular connection.

        @returns an update url string.

        """
        if job_repo_url is None:
            self._job_repo_url = self._get_job_repo_url()
        else:
            self._job_repo_url = job_repo_url
        if not self._job_repo_url:
            raise error.TestFail('There was no job_repo_url so we cannot get '
                                 'a payload to use.')
        ds_url, build = tools.get_devserver_build_from_package_url(
            self._job_repo_url)

        # We always stage the payloads on the existing lab devservers.
        self._autotest_devserver = dev_server.ImageServer(ds_url)

        if cellular:
            # Get the google storage url of the payload. We will be copying
            # the payload to a public google storage bucket (similar location
            # to updates via autest command).
            payload_url = self._get_payload_url(build,
                                                full_payload=full_payload)
            return self._copy_payload_to_public_bucket(payload_url)

        if full_payload:
            self._autotest_devserver.stage_artifacts(build, ['full_payload'])
            if not critical_update:
                # We can use the same lab devserver to handle the update.
                return self._autotest_devserver.get_update_url(build)
            else:
                staged_url = self._autotest_devserver._get_image_url(build)
        else:
            # We need to stage delta ourselves due to crbug.com/793434.
            delta_payload = self._get_payload_url(build, full_payload=False)
            staged_url = self._stage_payload_by_uri(delta_payload)

        # We need to start our own devserver for the rest of the cases.
        self._omaha_devserver = omaha_devserver.OmahaDevserver(
            self._autotest_devserver.hostname, staged_url,
            max_updates=max_updates, critical_update=critical_update)
        self._omaha_devserver.start_devserver()
        return self._omaha_devserver.get_update_url()


class UpdateEngineEventMissing(error.TestFail):
    """Raised if the hostlog is missing an expected event."""
