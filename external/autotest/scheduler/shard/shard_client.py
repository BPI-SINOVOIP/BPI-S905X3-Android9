#!/usr/bin/python
#pylint: disable-msg=C0111

# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import httplib
import logging
import os
import random
import signal
import time
import urllib2

import common

from autotest_lib.frontend import setup_django_environment
from autotest_lib.frontend.afe.json_rpc import proxy
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import global_config
from autotest_lib.frontend.afe import models
from autotest_lib.scheduler import email_manager
from autotest_lib.scheduler import scheduler_lib
from autotest_lib.server.cros.dynamic_suite import frontend_wrappers
from autotest_lib.server import utils as server_utils
from chromite.lib import timeout_util
from django.db import transaction

try:
    from chromite.lib import metrics
    from chromite.lib import ts_mon_config
except ImportError:
    metrics = server_utils.metrics_mock
    ts_mon_config = server_utils.metrics_mock


"""
Autotest shard client

The shard client can be run as standalone service. It periodically polls the
master in a heartbeat, retrieves new jobs and hosts and inserts them into the
local database.

A shard is set up (by a human) and pointed to the global AFE (cautotest).
On the shard, this script periodically makes so called heartbeat requests to the
global AFE, which will then complete the following actions:

1. Find the previously created (with atest) record for the shard. Shards are
   identified by their hostnames, specified in the shadow_config.
2. Take the records that were sent in the heartbeat and insert them into the
   global database.
   - This is to set the status of jobs to completed in the master database after
     they were run by a slave. This is necessary so one can just look at the
     master's afe to see the statuses of all jobs. Otherwise one would have to
     check the tko tables or the individual slave AFEs.
3. Find labels that have been assigned to this shard.
4. Assign hosts that:
   - have the specified label
   - aren't leased
   - have an id which is not in the known_host_ids which were sent in the
     heartbeat request.
5. Assign jobs that:
   - depend on the specified label
   - haven't been assigned before
   - aren't started yet
   - aren't completed yet
   - have an id which is not in the jobs_known_ids which were sent in the
     heartbeat request.
6. Serialize the chosen jobs and hosts.
   - Find objects that the Host/Job objects depend on: Labels, AclGroups, Users,
     and many more. Details about this can be found around
     model_logic.serialize()
7. Send these objects to the slave.


On the client side, this will happen:
1. Deserialize the objects sent from the master and persist them to the local
   database.
2. monitor_db on the shard will pick up these jobs and schedule them on the
   available hosts (which were retrieved from a heartbeat).
3. Once a job is finished, it's shard_id is set to NULL
4. The shard_client will pick up all jobs where shard_id=NULL and will
   send them to the master in the request of the next heartbeat.
   - The master will persist them as described earlier.
   - the shard_id will be set back to the shard's id, so the record won't be
     uploaded again.
   The heartbeat request will also contain the ids of incomplete jobs and the
   ids of all hosts. This is used to not send objects repeatedly. For more
   information on this and alternatives considered
   see rpc_interface.shard_heartbeat.
"""


HEARTBEAT_AFE_ENDPOINT = 'shard_heartbeat'
_METRICS_PREFIX  = 'chromeos/autotest/shard_client/heartbeat/'

RPC_TIMEOUT_MIN = 5
RPC_DELAY_SEC = 5

_heartbeat_client = None


class ShardClient(object):
    """Performs client side tasks of sharding, i.e. the heartbeat.

    This class contains the logic to do periodic heartbeats to a global AFE,
    to retrieve new jobs from it and to report completed jobs back.
    """

    def __init__(self, global_afe_hostname, shard_hostname, tick_pause_sec):
        self.afe = frontend_wrappers.RetryingAFE(server=global_afe_hostname,
                                                 timeout_min=RPC_TIMEOUT_MIN,
                                                 delay_sec=RPC_DELAY_SEC)
        self.hostname = shard_hostname
        self.tick_pause_sec = tick_pause_sec
        self._shutdown_requested = False
        self._shard = None


    def _deserialize_many(self, serialized_list, djmodel, message):
        """Deserialize data in JSON format to database.

        Deserialize a list of JSON-formatted data to database using Django.

        @param serialized_list: A list of JSON-formatted data.
        @param djmodel: Django model type.
        @param message: A string to be used in a logging message.
        """
        for serialized in serialized_list:
            with transaction.commit_on_success():
                try:
                    djmodel.deserialize(serialized)
                except Exception as e:
                    logging.error('Deserializing a %s fails: %s, Error: %s',
                                  message, serialized, e)
                    metrics.Counter(
                        'chromeos/autotest/shard_client/deserialization_failed'
                        ).increment()


    @metrics.SecondsTimerDecorator(
            'chromeos/autotest/shard_client/heartbeat_response_duration')
    def process_heartbeat_response(self, heartbeat_response):
        """Save objects returned by a heartbeat to the local database.

        This deseralizes hosts and jobs including their dependencies and saves
        them to the local database.

        @param heartbeat_response: A dictionary with keys 'hosts' and 'jobs',
                                   as returned by the `shard_heartbeat` rpc
                                   call.
        """
        hosts_serialized = heartbeat_response['hosts']
        jobs_serialized = heartbeat_response['jobs']
        suite_keyvals_serialized = heartbeat_response['suite_keyvals']
        incorrect_host_ids = heartbeat_response.get('incorrect_host_ids', [])

        metrics.Gauge('chromeos/autotest/shard_client/hosts_received'
                      ).set(len(hosts_serialized))
        metrics.Gauge('chromeos/autotest/shard_client/jobs_received'
                      ).set(len(jobs_serialized))
        metrics.Gauge('chromeos/autotest/shard_client/suite_keyvals_received'
                      ).set(len(suite_keyvals_serialized))

        self._deserialize_many(hosts_serialized, models.Host, 'host')
        self._deserialize_many(jobs_serialized, models.Job, 'job')
        self._deserialize_many(suite_keyvals_serialized, models.JobKeyval,
                               'jobkeyval')

        host_ids = [h['id'] for h in hosts_serialized]
        logging.info('Heartbeat response contains hosts %s', host_ids)
        job_ids = [j['id'] for j in jobs_serialized]
        logging.info('Heartbeat response contains jobs %s', job_ids)
        parent_jobs_with_keyval = set([kv['job_id']
                                       for kv in suite_keyvals_serialized])
        logging.info('Heartbeat response contains suite_keyvals_for jobs %s',
                     list(parent_jobs_with_keyval))
        if incorrect_host_ids:
            logging.info('Heartbeat response contains incorrect_host_ids %s '
                         'which will be deleted.', incorrect_host_ids)
            self._remove_incorrect_hosts(incorrect_host_ids)

        # If the master has just sent any jobs that we think have completed,
        # re-sync them with the master. This is especially useful when a
        # heartbeat or job is silently dropped, as the next heartbeat will
        # have a disagreement. Updating the shard_id to NULL will mark these
        # jobs for upload on the next heartbeat.
        job_models = models.Job.objects.filter(
                id__in=job_ids, hostqueueentry__complete=True)
        if job_models:
            job_models.update(shard=None)
            job_ids_repr = ', '.join([str(job.id) for job in job_models])
            logging.warn('Following completed jobs are reset shard_id to NULL '
                         'to be uploaded to master again: %s', job_ids_repr)


    def _remove_incorrect_hosts(self, incorrect_host_ids=None):
        """Remove from local database any hosts that should not exist.

        Entries of |incorrect_host_ids| that are absent from database will be
        silently ignored.

        @param incorrect_host_ids: a list of ids (as integers) to remove.
        """
        if not incorrect_host_ids:
            return

        models.Host.objects.filter(id__in=incorrect_host_ids).delete()


    @property
    def shard(self):
        """Return this shard's own shard object, fetched from the database.

        A shard's object is fetched from the master with the first jobs. It will
        not exist before that time.

        @returns: The shard object if it already exists, otherwise None
        """
        if self._shard is None:
            try:
                self._shard = models.Shard.smart_get(self.hostname)
            except models.Shard.DoesNotExist:
                # This might happen before any jobs are assigned to this shard.
                # This is okay because then there is nothing to offload anyway.
                pass
        return self._shard


    def _get_jobs_to_upload(self):
        jobs = []
        # The scheduler sets shard to None upon completion of the job.
        # For more information on the shard field's semantic see
        # models.Job.shard. We need to be careful to wait for both the
        # shard_id and the complete bit here, or we will end up syncing
        # the job without ever setting the complete bit.
        job_ids = list(models.Job.objects.filter(
            shard=None,
            hostqueueentry__complete=True).values_list('pk', flat=True))

        for job_to_upload in models.Job.objects.filter(pk__in=job_ids).all():
            jobs.append(job_to_upload)
        return jobs


    def _mark_jobs_as_uploaded(self, job_ids):
        # self.shard might be None if no jobs were downloaded yet.
        # But then job_ids is empty, so this is harmless.
        # Even if there were jobs we'd in the worst case upload them twice.
        models.Job.objects.filter(pk__in=job_ids).update(shard=self.shard)


    def _get_hqes_for_jobs(self, jobs):
        hqes = []
        for job in jobs:
            hqes.extend(job.hostqueueentry_set.all())
        return hqes


    def _get_known_jobs_and_hosts(self):
        """Returns lists of host and job info to send in a heartbeat.

        The host and job ids are ids of objects that are already present on the
        shard and therefore don't need to be sent again.

        For jobs, only incomplete jobs are sent, as the master won't send
        already completed jobs anyway. This helps keeping the list of id's
        considerably small.

        For hosts, host status in addition to host id are sent to master
        to sync the host status.

        @returns: Tuple of three lists. The first one contains job ids, the
                  second one host ids, and the third one host statuses.
        """
        job_ids = list(models.Job.objects.filter(
                hostqueueentry__complete=False).values_list('id', flat=True))
        host_models = models.Host.objects.filter(invalid=0)
        host_ids = []
        host_statuses = []
        for h in host_models:
            host_ids.append(h.id)
            host_statuses.append(h.status)
        return job_ids, host_ids, host_statuses


    def _heartbeat_packet(self):
        """Construct the heartbeat packet.

        See rpc_interface for a more detailed description of the heartbeat.

        @return: A heartbeat packet.
        """
        known_job_ids, known_host_ids, known_host_statuses = (
                self._get_known_jobs_and_hosts())
        logging.info('Known jobs: %s', known_job_ids)

        job_objs = self._get_jobs_to_upload()
        hqes = [hqe.serialize(include_dependencies=False)
                for hqe in self._get_hqes_for_jobs(job_objs)]
        jobs = [job.serialize(include_dependencies=False) for job in job_objs]
        logging.info('Uploading jobs %s', [j['id'] for j in jobs])

        return {'shard_hostname': self.hostname,
                'known_job_ids': known_job_ids,
                'known_host_ids': known_host_ids,
                'known_host_statuses': known_host_statuses,
                'jobs': jobs, 'hqes': hqes}


    def _report_packet_metrics(self, packet):
        """Report stats about outgoing packet to monarch."""
        metrics.Gauge(_METRICS_PREFIX + 'known_job_ids_count').set(
            len(packet['known_job_ids']))
        metrics.Gauge(_METRICS_PREFIX + 'jobs_upload_count').set(
            len(packet['jobs']))
        metrics.Gauge(_METRICS_PREFIX + 'known_host_ids_count').set(
            len(packet['known_host_ids']))


    def _heartbeat_failure(self, log_message, failure_type_str=''):
        logging.error("Heartbeat failed. %s", log_message)
        metrics.Counter('chromeos/autotest/shard_client/heartbeat_failure'
                        ).increment(fields={'failure_type': failure_type_str})


    @metrics.SecondsTimerDecorator(
            'chromeos/autotest/shard_client/do_heatbeat_duration')
    def do_heartbeat(self):
        """Perform a heartbeat: Retreive new jobs.

        This function executes a `shard_heartbeat` RPC. It retrieves the
        response of this call and processes the response by storing the returned
        objects in the local database.

        Returns: True if the heartbeat ran successfully, False otherwise.
        """

        logging.info("Performing heartbeat.")
        packet = self._heartbeat_packet()
        self._report_packet_metrics(packet)
        metrics.Gauge(_METRICS_PREFIX + 'request_size').set(
            len(str(packet)))

        try:
            response = self.afe.run(HEARTBEAT_AFE_ENDPOINT, **packet)
        except urllib2.HTTPError as e:
            self._heartbeat_failure('HTTPError %d: %s' % (e.code, e.reason),
                                    'HTTPError')
            return False
        except urllib2.URLError as e:
            self._heartbeat_failure('URLError: %s' % e.reason,
                                    'URLError')
            return False
        except httplib.HTTPException as e:
            self._heartbeat_failure('HTTPException: %s' % e,
                                    'HTTPException')
            return False
        except timeout_util.TimeoutError as e:
            self._heartbeat_failure('TimeoutError: %s' % e,
                                    'TimeoutError')
            return False
        except proxy.JSONRPCException as e:
            self._heartbeat_failure('JSONRPCException: %s' % e,
                                    'JSONRPCException')
            return False

        metrics.Gauge(_METRICS_PREFIX + 'response_size').set(
            len(str(response)))
        self._mark_jobs_as_uploaded([job['id'] for job in packet['jobs']])
        self.process_heartbeat_response(response)
        logging.info("Heartbeat completed.")
        return True


    def tick(self):
        """Performs all tasks the shard clients needs to do periodically."""
        success = self.do_heartbeat()
        if success:
            metrics.Counter('chromeos/autotest/shard_client/tick').increment()


    def loop(self, lifetime_hours):
        """Calls tick() until shutdown() is called or lifetime expires.

        @param lifetime_hours: (int) hours to loop for.
        """
        loop_start_time = time.time()
        while self._continue_looping(lifetime_hours, loop_start_time):
            self.tick()
            # Sleep with +/- 10% fuzzing to avoid phaselock of shards.
            tick_fuzz = self.tick_pause_sec * 0.2 * (random.random() - 0.5)
            time.sleep(self.tick_pause_sec + tick_fuzz)


    def shutdown(self):
        """Stops the shard client after the current tick."""
        logging.info("Shutdown request received.")
        self._shutdown_requested = True


    def _continue_looping(self, lifetime_hours, loop_start_time):
        """Determines if we should continue with the next mainloop iteration.

        @param lifetime_hours: (float) number of hours to loop for. None
                implies no deadline.
        @param process_start_time: Time when we started looping.
        @returns True if we should continue looping, False otherwise.
        """
        if self._shutdown_requested:
            return False

        if (lifetime_hours is None
            or time.time() - loop_start_time < lifetime_hours * 3600):
            return True
        logging.info('Process lifetime %0.3f hours exceeded. Shutting down.',
                     lifetime_hours)
        return False


def handle_signal(signum, frame):
    """Sigint handler so we don't crash mid-tick."""
    _heartbeat_client.shutdown()


def _get_shard_hostname_and_ensure_running_on_shard():
    """Read the hostname the local shard from the global configuration.

    Raise an exception if run from elsewhere than a shard.

    @raises error.HeartbeatOnlyAllowedInShardModeException if run from
            elsewhere than from a shard.
    """
    hostname = global_config.global_config.get_config_value(
        'SHARD', 'shard_hostname', default=None)
    if not hostname:
        raise error.HeartbeatOnlyAllowedInShardModeException(
            'To run the shard client, shard_hostname must neither be None nor '
            'empty.')
    return hostname


def _get_tick_pause_sec():
    """Read pause to make between two ticks from the global configuration."""
    return global_config.global_config.get_config_value(
        'SHARD', 'heartbeat_pause_sec', type=float)


def get_shard_client():
    """Instantiate a shard client instance.

    Configuration values will be read from the global configuration.

    @returns A shard client instance.
    """
    global_afe_hostname = server_utils.get_global_afe_hostname()
    shard_hostname = _get_shard_hostname_and_ensure_running_on_shard()
    tick_pause_sec = _get_tick_pause_sec()
    return ShardClient(global_afe_hostname, shard_hostname, tick_pause_sec)


def main():
    parser = argparse.ArgumentParser(description='Shard client.')
    parser.add_argument(
            '--lifetime-hours',
            type=float,
            default=None,
            help='If provided, number of hours we should run for. '
                 'At the expiry of this time, the process will exit '
                 'gracefully.',
    )
    parser.add_argument(
            '--metrics-file',
            help='If provided, drop metrics to this local file instead of '
                 'reporting to ts_mon',
            type=str,
            default=None,
    )
    options = parser.parse_args()

    with ts_mon_config.SetupTsMonGlobalState(
          'shard_client',
          indirect=True,
          debug_file=options.metrics_file,
    ):
        try:
            metrics.Counter('chromeos/autotest/shard_client/start').increment()
            main_without_exception_handling(options)
        except Exception as e:
            metrics.Counter('chromeos/autotest/shard_client/uncaught_exception'
                            ).increment()
            message = 'Uncaught exception. Terminating shard_client.'
            email_manager.manager.log_stacktrace(message)
            logging.exception(message)
            raise
        finally:
            email_manager.manager.send_queued_emails()


def main_without_exception_handling(options):
    scheduler_lib.setup_logging(
            os.environ.get('AUTOTEST_SCHEDULER_LOG_DIR', None),
            None, timestamped_logfile_prefix='shard_client')

    logging.info("Setting signal handler.")
    signal.signal(signal.SIGINT, handle_signal)
    signal.signal(signal.SIGTERM, handle_signal)

    logging.info("Starting shard client.")
    global _heartbeat_client
    _heartbeat_client = get_shard_client()
    _heartbeat_client.loop(options.lifetime_hours)


if __name__ == '__main__':
    main()
