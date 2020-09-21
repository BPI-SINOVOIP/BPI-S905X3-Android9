#!/usr/bin/env python

import common
import json
import re
import sys

from autotest_lib.client.common_lib import time_utils
from autotest_lib.server import frontend
from autotest_lib.server.lib import status_history
from autotest_lib.server.lib import suite_report
from chromite.lib import cidb
from chromite.lib import commandline
from chromite.lib import cros_logging as logging

HostJobHistory = status_history.HostJobHistory


def GetParser():
    """Creates the argparse parser."""
    parser = commandline.ArgumentParser(description=__doc__)
    parser.add_argument('--input', type=str, action='store',
                        help='Input JSON file')
    parser.add_argument('--output', type=str, action='store',
                        help='Output JSON file')
    parser.add_argument('--name_filter', type=str, action='store',
                        help='Name of task to look for')
    parser.add_argument('--status_filter', type=str, action='store',
                        help='Status fo task to look for')
    parser.add_argument('--afe', type=str, action='store',
                        help='AFE server to connect to')
    parser.add_argument('suite_ids', type=str, nargs='*', action='store',
                        help='Suite ids to resolve')
    return parser


def GetSuiteHQEs(suite_job_id, look_past_seconds, afe=None, tko=None):
    """Get the host queue entries for active DUTs during a suite job.

    @param suite_job_id: Suite's AFE job id.
    @param look_past_seconds: Number of seconds past the end of the suite
                              job to look for next HQEs.
    @param afe: AFE database handle.
    @param tko: TKO database handle.

    @returns A dictionary keyed on hostname to a list of host queue entry
             dictionaries.  HQE dictionary contains the following keys:
             name, hostname, job_status, job_url, gs_url, start_time, end_time
    """
    if afe is None:
        afe = frontend.AFE()
    if tko is None:
        tko = frontend.TKO()

    # Find the suite job and when it ran.
    statuses = tko.get_job_test_statuses_from_db(suite_job_id)
    if len(statuses):
        for s in statuses:
            if s.test_started_time == 'None' or s.test_finished_time == 'None':
                logging.error(
                        'TKO entry missing time: %s %s %s %s %s %s %s %s %s' %
                        (s.id, s.test_name, s.status, s.reason,
                         s.test_started_time, s.test_finished_time,
                         s.job_owner, s.hostname, s.job_tag))
        start_time = min(int(time_utils.to_epoch_time(s.test_started_time))
                         for s in statuses if s.test_started_time != 'None')
        finish_time = max(int(time_utils.to_epoch_time(
                s.test_finished_time)) for s in statuses
                if s.test_finished_time != 'None')
    else:
        start_time = None
        finish_time = None

    # If there is no start time or finish time, won't be able to get HQEs.
    if start_time is None or finish_time is None:
        return {}

    # Find all the HQE entries.
    child_jobs = afe.get_jobs(parent_job_id=suite_job_id)
    child_job_ids = {j.id for j in child_jobs}
    hqes = afe.get_host_queue_entries(job_id__in=list(child_job_ids))
    hostnames = {h.host.hostname for h in hqes if h.host}
    host_hqes = {}
    for hostname in hostnames:
        history = HostJobHistory.get_host_history(afe, hostname,
                                                  start_time,
                                                  finish_time +
                                                  look_past_seconds)
        for h in history:
            gs_url = re.sub(r'http://.*/tko/retrieve_logs.cgi\?job=/results',
                            r'gs://chromeos-autotest-results',
                            h.job_url)
            entry = {
                    'name': h.name,
                    'hostname': history.hostname,
                    'job_status': h.job_status,
                    'job_url': h.job_url,
                    'gs_url': gs_url,
                    'start_time': h.start_time,
                    'end_time': h.end_time,
            }
            host_hqes.setdefault(history.hostname, []).append(entry)

    return host_hqes


def FindSpecialTasks(suite_job_id, look_past_seconds=1800,
                     name_filter=None, status_filter=None, afe=None, tko=None):
    """Find special tasks that happened around a suite job.

    @param suite_job_id: Suite's AFE job id.
    @param look_past_seconds: Number of seconds past the end of the suite
                              job to look for next HQEs.
    @param name_filter: If not None, only return tasks with this name.
    @param status_filter: If not None, only return tasks with this status.
    @param afe: AFE database handle.
    @param tko: TKO database handle.

    @returns A dictionary keyed on hostname to a list of host queue entry
             dictionaries.  HQE dictionary contains the following keys:
             name, hostname, job_status, job_url, gs_url, start_time, end_time,
             next_entry
    """
    host_hqes = GetSuiteHQEs(suite_job_id, look_past_seconds=look_past_seconds,
                             afe=afe, tko=tko)

    task_entries = []
    for hostname in host_hqes:
        host_hqes[hostname] = sorted(host_hqes[hostname],
                                        key=lambda k: k['start_time'])
        current = None
        for e in host_hqes[hostname]:
            # Check if there is an entry to finish off by adding a pointer
            # to this new entry.
            if current:
                logging.debug('    next task: %(name)s %(job_status)s '
                              '%(gs_url)s %(start_time)s %(end_time)s' % e)
                # Only record a pointer to the next entry if filtering some out.
                if name_filter or status_filter:
                    current['next_entry'] = e
                task_entries.append(current)
                current = None

            # Perform matching.
            if ((name_filter and e['name'] != name_filter) or
                (status_filter and e['job_status'] != status_filter)):
                continue

            # Instead of appending right away, wait until the next entry
            # to add a point to it.
            current = e
            logging.debug('Task %(name)s: %(job_status)s %(hostname)s '
                          '%(gs_url)s %(start_time)s %(end_time)s' % e)

        # Add the last one even if a next entry wasn't found.
        if current:
            task_entries.append(current)

    return task_entries

def main(argv):
    parser = GetParser()
    options = parser.parse_args(argv)

    afe = None
    if options.afe:
        afe = frontend.AFE(server=options.afe)
    tko = frontend.TKO()

    special_tasks = []
    builds = []

    # Handle a JSON file being specified.
    if options.input:
        with open(options.input) as f:
            data = json.load(f)
            for build in data.get('builds', []):
                # For each build, amend it to include the list of
                # special tasks for its suite's jobs.
                build.setdefault('special_tasks', {})
                for suite_job_id in build['suite_ids']:
                    suite_tasks = FindSpecialTasks(
                            suite_job_id, name_filter=options.name_filter,
                            status_filter=options.status_filter,
                            afe=afe, tko=tko)
                    special_tasks.extend(suite_tasks)
                    build['special_tasks'][suite_job_id] = suite_tasks
                logging.debug(build)
                builds.append(build)

    # Handle and specifically specified suite IDs.
    for suite_job_id in options.suite_ids:
        special_tasks.extend(FindSpecialTasks(
                suite_job_id, name_filter=options.name_filter,
                status_filter=options.status_filter, afe=afe, tko=tko))

    # Output a resulting JSON file.
    with open(options.output, 'w') if options.output else sys.stdout as f:
        output = {
            'special_tasks': special_tasks,
            'name_filter': options.name_filter,
            'status_filter': options.status_filter,
        }
        if len(builds):
            output['builds'] = builds
        json.dump(output, f)

if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
