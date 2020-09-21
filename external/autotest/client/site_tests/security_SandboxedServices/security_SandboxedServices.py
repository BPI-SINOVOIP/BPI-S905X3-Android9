# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import csv
import logging
import os

from collections import namedtuple

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import utils
from autotest_lib.client.cros import asan


PS_FIELDS = (
    'pid',
    'ppid',
    'comm:32',
    'euser:%(usermax)d',
    'ruser:%(usermax)d',
    'egroup:%(groupmax)d',
    'rgroup:%(groupmax)d',
    'ipcns',
    'mntns',
    'netns',
    'pidns',
    'userns',
    'utsns',
    'args',
)
# These fields aren't available via ps, so we have to get them indirectly.
# Note: Case is significant as the fields match the /proc/PID/status file.
STATUS_FIELDS = (
    'CapInh',
    'CapPrm',
    'CapEff',
    'CapBnd',
    'CapAmb',
    'NoNewPrivs',
    'Seccomp',
)
PsOutput = namedtuple("PsOutput",
                      ' '.join([field.split(':')[0].lower()
                                for field in PS_FIELDS + STATUS_FIELDS]))

# Constants that match the values in /proc/PID/status Seccomp field.
# See `man 5 proc` for more details.
SECCOMP_MODE_DISABLED = '0'
SECCOMP_MODE_STRICT = '1'
SECCOMP_MODE_FILTER = '2'
# For human readable strings.
SECCOMP_MAP = {
    SECCOMP_MODE_DISABLED: 'disabled',
    SECCOMP_MODE_STRICT: 'strict',
    SECCOMP_MODE_FILTER: 'filter',
}


def get_properties(service, init_process):
    """Returns a dictionary of the properties of a service.

    @param service: the PsOutput of the service.
    @param init_process: the PsOutput of the init process.
    """

    properties = dict(service._asdict())
    properties['exe'] = service.comm
    properties['pidns'] = yes_or_no(service.pidns != init_process.pidns)
    properties['caps'] = yes_or_no(service.capeff != init_process.capeff)
    properties['nonewprivs'] = yes_or_no(service.nonewprivs == '1')
    properties['filter'] = yes_or_no(service.seccomp == SECCOMP_MODE_FILTER)
    return properties


def yes_or_no(value):
    """Returns 'Yes' or 'No' based on the truthiness of a value.

    @param value: boolean value.
    """

    return 'Yes' if value else 'No'


class security_SandboxedServices(test.test):
    """Enforces sandboxing restrictions on the processes running
    on the system.
    """

    version = 1


    def get_running_processes(self):
        """Returns a list of running processes as PsOutput objects."""

        usermax = utils.system_output("cut -d: -f1 /etc/passwd | wc -L",
                                      ignore_status=True)
        groupmax = utils.system_output('cut -d: -f1 /etc/group | wc -L',
                                       ignore_status=True)
        # Even if the names are all short, make sure we have enough space
        # to hold numeric 32-bit ids too (can come up with userns).
        usermax = max(int(usermax), 10)
        groupmax = max(int(groupmax), 10)
        fields = {
            'usermax': usermax,
            'groupmax': groupmax,
        }
        ps_cmd = ('ps --no-headers -ww -eo ' +
                  (','.join(PS_FIELDS) % fields))
        ps_fields_len = len(PS_FIELDS)

        output = utils.system_output(ps_cmd)
        logging.debug('output of ps:\n%s', output)

        # Fill in fields that `ps` doesn't support but are in /proc/PID/status.
        # Example line output:
        # Pid:1 CapInh:0000000000000000 CapPrm:0000001fffffffff CapEff:0000001fffffffff CapBnd:0000001fffffffff Seccomp:0
        cmd = (
            "for f in /proc/[1-9]*/status ; do awk '$1 ~ \"^(Pid|%s):\" "
            "{printf \"%%s%%s \", $1, $NF; if ($1 == \"%s:\") printf \"\\n\"}'"
            " $f ; done"
        ) % ('|'.join(STATUS_FIELDS), STATUS_FIELDS[-1])
        # Processes might exit while awk is running, so ignore its exit status.
        status_output = utils.system_output(cmd, ignore_status=True)
        # Turn each line into a dict.
        # [
        #   {'pid': '1', 'CapInh': '0000000000000000', 'Seccomp': '0', ...},
        #   {'pid': '10', ...},
        #   ...,
        # ]
        status_list = list(dict(attr.split(':', 1) for attr in line.split())
                           for line in status_output.splitlines())
        # Create a dict mapping a pid to its extended status data.
        # {
        #   '1': {'pid': '1', 'CapInh': '0000000000000000', ...},
        #   '2': {'pid': '2', ...},
        #   ...,
        # }
        status_data = dict((x['Pid'], x) for x in status_list)
        logging.debug('output of awk:\n%s', status_output)

        # Now merge the two sets of process data.
        running_processes = []
        for line in output.splitlines():
            # crbug.com/422700: Filter out zombie processes.
            if '<defunct>' in line:
                continue

            fields = line.split(None, ps_fields_len - 1)
            pid = fields[0]
            # The process lists might not be exactly the same (since we gathered
            # data with multiple commands), and not all fields might exist (e.g.
            # older kernels might not have all the fields).
            pid_data = status_data.get(pid, {})
            status_fields = [pid_data.get(key) for key in STATUS_FIELDS]
            running_processes.append(PsOutput(*fields + status_fields))

        return running_processes


    def load_baseline(self):
        """The baseline file lists the services we know and
        whether (and how) they are sandboxed.
        """

        def load(path):
            """Load baseline from |path| and return its fields and dictionary.

            @param path: The baseline to load.
            """
            logging.info('Loading baseline %s', path)
            reader = csv.DictReader(open(path))
            return reader.fieldnames, dict((d['exe'], d) for d in reader
                                           if not d['exe'].startswith('#'))

        baseline_path = os.path.join(self.bindir, 'baseline')
        fields, ret = load(baseline_path)

        board = utils.get_current_board()
        baseline_path += '.' + board
        if os.path.exists(baseline_path):
            new_fields, new_entries = load(baseline_path)
            if new_fields != fields:
                raise error.TestError('header mismatch in %s' % baseline_path)
            ret.update(new_entries)

        return fields, ret


    def load_exclusions(self):
        """The exclusions file lists running programs
        that we don't care about (for now).
        """

        exclusions_path = os.path.join(self.bindir, 'exclude')
        return set(line.strip() for line in open(exclusions_path)
                   if not line.startswith('#'))


    def dump_services(self, fieldnames, running_services_properties):
        """Leaves a list of running services in the results dir
        so that we can update the baseline file if necessary.

        @param fieldnames: list of fields to be written.
        @param running_services_properties: list of services to be logged.
        """

        file_path = os.path.join(self.resultsdir, 'running_services')
        with open(file_path, 'w') as output_file:
            writer = csv.DictWriter(output_file, fieldnames=fieldnames,
                                    extrasaction='ignore')
            writer.writeheader()
            for service_properties in running_services_properties:
                writer.writerow(service_properties)


    def run_once(self):
        """Inspects the process list, looking for root and sandboxed processes
        (with some exclusions). If we have a baseline entry for a given process,
        confirms it's an exact match. Warns if we see root or sandboxed
        processes that we have no baseline for, and warns if we have
        baselines for processes not seen running.
        """

        fieldnames, baseline = self.load_baseline()
        exclusions = self.load_exclusions()
        running_processes = self.get_running_processes()
        is_asan = asan.running_on_asan()
        if is_asan:
            logging.info('ASAN image detected -> skipping seccomp checks')

        kthreadd_pid = -1

        init_process = None
        running_services = {}

        # Filter running processes list.
        for process in running_processes:
            exe = process.comm

            if exe == "kthreadd":
                kthreadd_pid = process.pid
                continue
            elif process.pid == "1":
                init_process = process
                continue

            # Don't worry about kernel threads.
            if process.ppid == kthreadd_pid:
                continue

            if exe in exclusions:
                continue

            running_services[exe] = process

        if not init_process:
            raise error.TestFail("Cannot find init process")

        # Find differences between running services and baseline.
        services_set = set(running_services.keys())
        baseline_set = set(baseline.keys())

        new_services = services_set.difference(baseline_set)
        stale_baselines = baseline_set.difference(services_set)

        # Check baseline.
        sandbox_delta = []
        for exe in services_set.intersection(baseline_set):
            process = running_services[exe]

            # If the process is not running as the correct user.
            if process.euser != baseline[exe]["euser"]:
                logging.error('%s: bad user: wanted "%s" but got "%s"',
                              exe, baseline[exe]['euser'], process.euser)

                sandbox_delta.append(exe)
                continue

            # If the process is not running as the correct group.
            if process.egroup != baseline[exe]['egroup']:
                logging.error('%s: bad group: wanted "%s" but got "%s"',
                              exe, baseline[exe]['egroup'], process.egroup)

                sandbox_delta.append(exe)
                continue

            # Check the various sandbox settings.
            if (baseline[exe]['pidns'] == 'Yes' and
                    process.pidns == init_process.pidns):
                logging.error('%s: missing pid ns usage', exe)
                sandbox_delta.append(exe)
            elif (baseline[exe]['caps'] == 'Yes' and
                  process.capeff == init_process.capeff):
                logging.error('%s: missing caps usage', exe)
                sandbox_delta.append(exe)
            elif (baseline[exe]['nonewprivs'] == 'Yes' and
                  process.nonewprivs != '1'):
                logging.error('%s: missing NoNewPrivs', exe)
                sandbox_delta.append(exe)
            elif (baseline[exe]['filter'] == 'Yes' and
                  process.seccomp != SECCOMP_MODE_FILTER and
                  not is_asan):
                # Since Minijail disables seccomp at runtime when ASAN is
                # active, we can't enforce it on ASAN bots.  Just ignore
                # the test entirely.  (Comment applies to "is_asan" above.)
                logging.error('%s: missing seccomp usage: wanted %s (%s) but '
                              'got %s (%s)', exe, SECCOMP_MODE_FILTER,
                              SECCOMP_MAP[SECCOMP_MODE_FILTER], process.seccomp,
                              SECCOMP_MAP.get(process.seccomp, '???'))
                sandbox_delta.append(exe)

        # Save current run to results dir.
        running_services_properties = [get_properties(s, init_process)
                                       for s in running_services.values()]
        self.dump_services(fieldnames, running_services_properties)

        if len(stale_baselines) > 0:
            logging.warn('Stale baselines: %r', stale_baselines)

        if len(new_services) > 0:
            logging.warn('New services: %r', new_services)

            # We won't complain about new non-root services (on the assumption
            # that they've already somewhat sandboxed things), but we'll fail
            # with new root services (on the assumption they haven't done any
            # sandboxing work).  If they really need to run as root, they can
            # update the baseline to whitelist it.
            new_root_services = [x for x in new_services
                                 if running_services[x].euser == 'root']
            if new_root_services:
                logging.error('New services are not allowed to run as root, '
                              'but these are: %r', new_root_services)
                sandbox_delta.extend(new_root_services)

        if len(sandbox_delta) > 0:
            logging.error('Failed sandboxing: %r', sandbox_delta)
            raise error.TestFail('One or more processes failed sandboxing')
