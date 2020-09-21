# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Services relating to generating a suite timeline and report."""

from __future__ import print_function

import common
import json

from autotest_lib.client.common_lib import time_utils
from autotest_lib.server import frontend
from autotest_lib.server.lib import status_history
from chromite.lib import cros_logging as logging


HostJobHistory = status_history.HostJobHistory

# TODO: Handle other statuses like infra failures.
TKO_STATUS_MAP = {
    'ERROR': 'fail',
    'FAIL': 'fail',
    'GOOD': 'pass',
    'PASS': 'pass',
    'ABORT': 'aborted',
    'Failed': 'fail',
    'Completed': 'pass',
    'Aborted': 'aborted',
}


def parse_tko_status_string(status_string):
    """Parse a status string from TKO or the HQE databases.

    @param status_string: A status string from TKO or HQE databases.

    @return A status string suitable for inclusion within Cloud Datastore.
    """
    return TKO_STATUS_MAP.get(status_string, 'unknown:' + status_string)


def make_entry(entry_id, name, status, start_time,
               finish_time=None, parent=None):
    """Generate an event log entry to be stored in Cloud Datastore.

    @param entry_id: A (Kind, id) tuple representing the key.
    @param name: A string identifying the event
    @param status: A string identifying the status of the event.
    @param start_time: A unix timestamp of the start of the event.
    @param finish_time: A unix timestamp of the finish of the event.
    @param parent: A (Kind, id) tuple representing the parent key.

    @return A dictionary representing the entry suitable for dumping via JSON.
    """
    entry = {
        'id': entry_id,
        'name': name,
        'status': status,
        'start_time': start_time,
    }
    if finish_time is not None:
        entry['finish_time'] = finish_time
    if parent is not None:
        entry['parent'] = parent
    return entry


def find_start_finish_times(statuses):
    """Determines the start and finish times for a list of statuses.

    @param statuses: A list of job test statuses.

    @return (start_tme, finish_time) tuple of seconds past epoch.  If either
            cannot be determined, None for that time.
    """
    starts = {int(time_utils.to_epoch_time(s.test_started_time))
              for s in statuses if s.test_started_time != 'None'}
    finishes = {int(time_utils.to_epoch_time(s.test_finished_time))
                for s in statuses if s.test_finished_time != 'None'}
    start_time = min(starts) if starts else None
    finish_time = max(finishes) if finishes else None
    return start_time, finish_time


def make_job_entry(tko, job, parent=None, suite_job=False, job_entries=None):
    """Generate a Suite or HWTest event log entry.

    @param tko: TKO database handle.
    @param job: A frontend.Job to generate an entry for.
    @param parent: A (Kind, id) tuple representing the parent key.
    @param suite_job: A boolean indicating wheret this represents a suite job.
    @param job_entries: A dictionary mapping job id to earlier job entries.

    @return A dictionary representing the entry suitable for dumping via JSON.
    """
    statuses = tko.get_job_test_statuses_from_db(job.id)
    status = 'pass'
    dut = None
    for s in statuses:
        parsed_status = parse_tko_status_string(s.status)
        # TODO: Improve this generation of status.
        if parsed_status != 'pass':
            status = parsed_status
        if s.hostname:
            dut = s.hostname
        if s.test_started_time == 'None' or s.test_finished_time == 'None':
            logging.warn('TKO entry for %d missing time: %s' % (job.id, str(s)))
    start_time, finish_time = find_start_finish_times(statuses)
    entry = make_entry(('Suite' if suite_job else 'HWTest', int(job.id)),
                       job.name.split('/')[-1], status, start_time,
                       finish_time=finish_time, parent=parent)

    entry['job_id'] = int(job.id)
    if dut:
        entry['dut'] = dut
    if job.shard:
        entry['shard'] = job.shard
    # Determine the try of this job by looking back through what the
    # original job id is.
    if 'retry_original_job_id' in job.keyvals:
        original_job_id = int(job.keyvals['retry_original_job_id'])
        original_job = job_entries.get(original_job_id, None)
        if original_job:
            entry['try'] = original_job['try'] + 1
        else:
            entry['try'] = 0
    else:
        entry['try'] = 1
    entry['gs_url'] = status_history.get_job_gs_url(job)
    return entry


def make_hqe_entry(hostname, hqe, hqe_statuses, parent=None):
    """Generate a HQE event log entry.

    @param hostname: A string of the hostname.
    @param hqe: A host history to generate an event for.
    @param hqe_statuses: A dictionary mapping HQE ids to job status.
    @param parent: A (Kind, id) tuple representing the parent key.

    @return A dictionary representing the entry suitable for dumping via JSON.
    """
    entry = make_entry(
        ('HQE', int(hqe.id)), hostname,
        hqe_statuses.get(hqe.id, parse_tko_status_string(hqe.job_status)),
        hqe.start_time, finish_time=hqe.end_time, parent=parent)

    entry['task_name'] = hqe.name.split('/')[-1]
    entry['in_suite'] = hqe.id in hqe_statuses
    entry['job_url'] = hqe.job_url
    entry['gs_url'] = hqe.gs_url
    if hqe.job_id is not None:
        entry['job_id'] = hqe.job_id
    entry['is_special'] = hqe.is_special
    return entry


def generate_suite_report(suite_job_id, afe=None, tko=None):
    """Generate a list of events corresonding to a single suite job.

    @param suite_job_id: The AFE id of the suite job.
    @param afe: AFE database handle.
    @param tko: TKO database handle.

    @return A list of entries suitable for dumping via JSON.
    """
    if afe is None:
        afe = frontend.AFE()
    if tko is None:
        tko = frontend.TKO()

    # Retrieve the main suite job.
    suite_job = afe.get_jobs(id=suite_job_id)[0]

    suite_entry = make_job_entry(tko, suite_job, suite_job=True)
    entries = [suite_entry]

    # Retrieve the child jobs and cache all their statuses
    logging.debug('Fetching child jobs...')
    child_jobs = afe.get_jobs(parent_job_id=suite_job_id)
    logging.debug('... fetched %s child jobs.' % len(child_jobs))
    job_statuses = {}
    job_entries = {}
    for j in child_jobs:
        job_entry = make_job_entry(tko, j, suite_entry['id'],
                                   job_entries=job_entries)
        entries.append(job_entry)
        job_statuses[j.id] = job_entry['status']
        job_entries[j.id] = job_entry

    # Retrieve the HQEs from all the child jobs, record statuses from
    # job statuses.
    child_job_ids = {j.id for j in child_jobs}
    logging.debug('Fetching HQEs...')
    hqes = afe.get_host_queue_entries(job_id__in=list(child_job_ids))
    logging.debug('... fetched %s HQEs.' % len(hqes))
    hqe_statuses = {h.id: job_statuses.get(h.job.id, None) for h in hqes}

    # Generate list of hosts.
    hostnames = {h.host.hostname for h in hqes if h.host}
    logging.debug('%s distinct hosts participated in the suite.' %
                  len(hostnames))

    # Retrieve histories for the time of the suite for all associated hosts.
    # TODO: Include all hosts in the pool.
    if suite_entry['start_time'] and suite_entry['finish_time']:
        histories = [HostJobHistory.get_host_history(afe, hostname,
                                                     suite_entry['start_time'],
                                                     suite_entry['finish_time'])
                     for hostname in sorted(hostnames)]

        for history in histories:
            entries.extend(make_hqe_entry(history.hostname, h, hqe_statuses,
                                          suite_entry['id']) for h in history)

    return entries

def dump_entries_as_json(entries, output_file):
    """Dump event log entries as json to a file.

    @param entries: A list of event log entries to dump.
    @param output_file: The file to write to.
    """
    # Write the entries out as JSON.
    logging.debug('Dumping %d entries' % len(entries))
    for e in entries:
        json.dump(e, output_file, sort_keys=True)
        output_file.write('\n')
