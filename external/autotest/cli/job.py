#
# Copyright 2008 Google Inc. All Rights Reserved.

"""
The job module contains the objects and methods used to
manage jobs in Autotest.

The valid actions are:
list:    lists job(s)
create:  create a job
abort:   abort job(s)
stat:    detailed listing of job(s)

The common options are:

See topic_common.py for a High Level Design and Algorithm.
"""

# pylint: disable=missing-docstring

import getpass, re
from autotest_lib.cli import topic_common, action_common
from autotest_lib.client.common_lib import control_data
from autotest_lib.client.common_lib import priorities


class job(topic_common.atest):
    """Job class
    atest job [create|clone|list|stat|abort] <options>"""
    usage_action = '[create|clone|list|stat|abort]'
    topic = msg_topic = 'job'
    msg_items = '<job_ids>'


    def _convert_status(self, results):
        for result in results:
            total = sum(result['status_counts'].values())
            status = ['%s=%s(%.1f%%)' % (key, val, 100.0*float(val)/total)
                      for key, val in result['status_counts'].iteritems()]
            status.sort()
            result['status_counts'] = ', '.join(status)


    def backward_compatibility(self, action, argv):
        """ 'job create --clone' became 'job clone --id' """
        if action == 'create':
            for option in ['-l', '--clone']:
                if option in argv:
                    argv[argv.index(option)] = '--id'
                    action = 'clone'
        return action


class job_help(job):
    """Just here to get the atest logic working.
    Usage is set by its parent"""
    pass


class job_list_stat(action_common.atest_list, job):
    def __init__(self):
        super(job_list_stat, self).__init__()

        self.topic_parse_info = topic_common.item_parse_info(
            attribute_name='jobs',
            use_leftover=True)


    def __split_jobs_between_ids_names(self):
        job_ids = []
        job_names = []

        # Sort between job IDs and names
        for job_id in self.jobs:
            if job_id.isdigit():
                job_ids.append(job_id)
            else:
                job_names.append(job_id)
        return (job_ids, job_names)


    def execute_on_ids_and_names(self, op, filters={},
                                 check_results={'id__in': 'id',
                                                'name__in': 'id'},
                                 tag_id='id__in', tag_name='name__in'):
        if not self.jobs:
            # Want everything
            return super(job_list_stat, self).execute(op=op, filters=filters)

        all_jobs = []
        (job_ids, job_names) = self.__split_jobs_between_ids_names()

        for items, tag in [(job_ids, tag_id),
                          (job_names, tag_name)]:
            if items:
                new_filters = filters.copy()
                new_filters[tag] = items
                jobs = super(job_list_stat,
                             self).execute(op=op,
                                           filters=new_filters,
                                           check_results=check_results)
                all_jobs.extend(jobs)

        return all_jobs


class job_list(job_list_stat):
    """atest job list [<jobs>] [--all] [--running] [--user <username>]"""
    def __init__(self):
        super(job_list, self).__init__()
        self.parser.add_option('-a', '--all', help='List jobs for all '
                               'users.', action='store_true', default=False)
        self.parser.add_option('-r', '--running', help='List only running '
                               'jobs', action='store_true')
        self.parser.add_option('-u', '--user', help='List jobs for given '
                               'user', type='string')


    def parse(self):
        options, leftover = super(job_list, self).parse()
        self.all = options.all
        self.data['running'] = options.running
        if options.user:
            if options.all:
                self.invalid_syntax('Only specify --all or --user, not both.')
            else:
                self.data['owner'] = options.user
        elif not options.all and not self.jobs:
            self.data['owner'] = getpass.getuser()

        return options, leftover


    def execute(self):
        return self.execute_on_ids_and_names(op='get_jobs_summary',
                                             filters=self.data)


    def output(self, results):
        keys = ['id', 'owner', 'name', 'status_counts']
        if self.verbose:
            keys.extend(['priority', 'control_type', 'created_on'])
        self._convert_status(results)
        super(job_list, self).output(results, keys)



class job_stat(job_list_stat):
    """atest job stat <job>"""
    usage_action = 'stat'

    def __init__(self):
        super(job_stat, self).__init__()
        self.parser.add_option('-f', '--control-file',
                               help='Display the control file',
                               action='store_true', default=False)
        self.parser.add_option('-N', '--list-hosts',
                               help='Display only a list of hosts',
                               action='store_true')
        self.parser.add_option('-s', '--list-hosts-status',
                               help='Display only the hosts in these statuses '
                               'for a job.', action='store')


    def parse(self):
        status_list = topic_common.item_parse_info(
                attribute_name='status_list',
                inline_option='list_hosts_status')
        options, leftover = super(job_stat, self).parse([status_list],
                                                        req_items='jobs')

        if not self.jobs:
            self.invalid_syntax('Must specify at least one job.')

        self.show_control_file = options.control_file
        self.list_hosts = options.list_hosts

        if self.list_hosts and self.status_list:
            self.invalid_syntax('--list-hosts is implicit when using '
                                '--list-hosts-status.')
        if len(self.jobs) > 1 and (self.list_hosts or self.status_list):
            self.invalid_syntax('--list-hosts and --list-hosts-status should '
                                'only be used on a single job.')

        return options, leftover


    def _merge_results(self, summary, qes):
        hosts_status = {}
        for qe in qes:
            if qe['host']:
                job_id = qe['job']['id']
                hostname = qe['host']['hostname']
                hosts_status.setdefault(job_id,
                                        {}).setdefault(qe['status'],
                                                       []).append(hostname)

        for job in summary:
            job_id = job['id']
            if hosts_status.has_key(job_id):
                this_job = hosts_status[job_id]
                job['hosts'] = ' '.join(' '.join(host) for host in
                                        this_job.itervalues())
                host_per_status = ['%s="%s"' %(status, ' '.join(host))
                                   for status, host in this_job.iteritems()]
                job['hosts_status'] = ', '.join(host_per_status)
                if self.status_list:
                    statuses = set(s.lower() for s in self.status_list)
                    all_hosts = [s for s in host_per_status if s.split('=',
                                 1)[0].lower() in statuses]
                    job['hosts_selected_status'] = '\n'.join(all_hosts)
            else:
                job['hosts_status'] = ''

            if not job.get('hosts'):
                self.generic_error('Job has unassigned meta-hosts, '
                                   'try again shortly.')

        return summary


    def execute(self):
        summary = self.execute_on_ids_and_names(op='get_jobs_summary')

        # Get the real hostnames
        qes = self.execute_on_ids_and_names(op='get_host_queue_entries',
                                            check_results={},
                                            tag_id='job__in',
                                            tag_name='job__name__in')

        self._convert_status(summary)

        return self._merge_results(summary, qes)


    def output(self, results):
        if self.list_hosts:
            keys = ['hosts']
        elif self.status_list:
            keys = ['hosts_selected_status']
        elif not self.verbose:
            keys = ['id', 'name', 'priority', 'status_counts', 'hosts_status']
        else:
            keys = ['id', 'name', 'priority', 'status_counts', 'hosts_status',
                    'owner', 'control_type',  'synch_count', 'created_on',
                    'run_verify', 'reboot_before', 'reboot_after',
                    'parse_failed_repair']

        if self.show_control_file:
            keys.append('control_file')

        super(job_stat, self).output(results, keys)


class job_create_or_clone(action_common.atest_create, job):
    """Class containing the code common to the job create and clone actions"""
    msg_items = 'job_name'

    def __init__(self):
        super(job_create_or_clone, self).__init__()
        self.hosts = []
        self.data_item_key = 'name'
        self.parser.add_option('-p', '--priority',
                               help='Job priority (int)', type='int',
                               default=priorities.Priority.DEFAULT)
        self.parser.add_option('-b', '--labels',
                               help='Comma separated list of labels '
                               'to get machine list from.', default='')
        self.parser.add_option('-m', '--machine', help='List of machines to '
                               'run on')
        self.parser.add_option('-M', '--mlist',
                               help='File listing machines to use',
                               type='string', metavar='MACHINE_FLIST')
        self.parser.add_option('--one-time-hosts',
                               help='List of one time hosts')
        self.parser.add_option('-e', '--email',
                               help='A comma seperated list of '
                               'email addresses to notify of job completion',
                               default='')


    def _parse_hosts(self, args):
        """ Parses the arguments to generate a list of hosts and meta_hosts
        A host is a regular name, a meta_host is n*label or *label.
        These can be mixed on the CLI, and separated by either commas or
        spaces, e.g.: 5*Machine_Label host0 5*Machine_Label2,host2 """

        hosts = []
        meta_hosts = []

        for arg in args:
            for host in arg.split(','):
                if re.match('^[0-9]+[*]', host):
                    num, host = host.split('*', 1)
                    meta_hosts += int(num) * [host]
                elif re.match('^[*](\w*)', host):
                    meta_hosts += [re.match('^[*](\w*)', host).group(1)]
                elif host != '' and host not in hosts:
                    # Real hostname and not a duplicate
                    hosts.append(host)

        return (hosts, meta_hosts)


    def parse(self, parse_info=[]):
        host_info = topic_common.item_parse_info(attribute_name='hosts',
                                                 inline_option='machine',
                                                 filename_option='mlist')
        job_info = topic_common.item_parse_info(attribute_name='jobname',
                                                use_leftover=True)
        oth_info = topic_common.item_parse_info(attribute_name='one_time_hosts',
                                                inline_option='one_time_hosts')
        label_info = topic_common.item_parse_info(attribute_name='labels',
                                                  inline_option='labels')

        options, leftover = super(job_create_or_clone, self).parse(
                [host_info, job_info, oth_info, label_info] + parse_info,
                req_items='jobname')
        self.data = {
            'priority': options.priority,
        }
        jobname = getattr(self, 'jobname')
        if len(jobname) > 1:
            self.invalid_syntax('Too many arguments specified, only expected '
                                'to receive job name: %s' % jobname)
        self.jobname = jobname[0]

        if self.one_time_hosts:
            self.data['one_time_hosts'] = self.one_time_hosts

        if self.labels:
            label_hosts = self.execute_rpc(op='get_hosts',
                                           multiple_labels=self.labels)
            for host in label_hosts:
                self.hosts.append(host['hostname'])

        self.data['name'] = self.jobname

        (self.data['hosts'],
         self.data['meta_hosts']) = self._parse_hosts(self.hosts)

        self.data['email_list'] = options.email

        return options, leftover


    def create_job(self):
        job_id = self.execute_rpc(op='create_job', **self.data)
        return ['%s (id %s)' % (self.jobname, job_id)]


    def get_items(self):
        return [self.jobname]



class job_create(job_create_or_clone):
    """atest job create [--priority <int>]
    [--synch_count] [--control-file </path/to/cfile>]
    [--on-server] [--test <test1,test2>]
    [--mlist </path/to/machinelist>] [--machine <host1 host2 host3>]
    [--labels <list of labels of machines to run on>]
    [--reboot_before <option>] [--reboot_after <option>]
    [--noverify] [--timeout <timeout>] [--max_runtime <max runtime>]
    [--one-time-hosts <hosts>] [--email <email>]
    [--dependencies <labels this job is dependent on>]
    [--parse-failed-repair <option>]
    [--image <http://path/to/image>] [--require-ssp]
    job_name

    Creating a job is rather different from the other create operations,
    so it only uses the __init__() and output() from its superclass.
    """
    op_action = 'create'

    def __init__(self):
        super(job_create, self).__init__()
        self.ctrl_file_data = {}
        self.parser.add_option('-y', '--synch_count', type=int,
                               help='Number of machines to use per autoserv '
                                    'execution')
        self.parser.add_option('-f', '--control-file',
                               help='use this control file', metavar='FILE')
        self.parser.add_option('-s', '--server',
                               help='This is server-side job',
                               action='store_true', default=False)
        self.parser.add_option('-t', '--test',
                               help='List of tests to run')

        self.parser.add_option('-d', '--dependencies', help='Comma separated '
                               'list of labels this job is dependent on.',
                               default='')

        self.parser.add_option('-B', '--reboot_before',
                               help='Whether or not to reboot the machine '
                                    'before the job (never/if dirty/always)',
                               type='choice',
                               choices=('never', 'if dirty', 'always'))
        self.parser.add_option('-a', '--reboot_after',
                               help='Whether or not to reboot the machine '
                                    'after the job (never/if all tests passed/'
                                    'always)',
                               type='choice',
                               choices=('never', 'if all tests passed',
                                        'always'))

        self.parser.add_option('--parse-failed-repair',
                               help='Whether or not to parse failed repair '
                                    'results as part of the job',
                               type='choice',
                               choices=('true', 'false'))
        self.parser.add_option('-n', '--noverify',
                               help='Do not run verify for job',
                               default=False, action='store_true')
        self.parser.add_option('-o', '--timeout_mins',
                               help='Job timeout in minutes.',
                               metavar='TIMEOUT')
        self.parser.add_option('--max_runtime',
                               help='Job maximum runtime in minutes')

        self.parser.add_option('-i', '--image',
                               help='OS image to install before running the '
                                    'test.')
        self.parser.add_option('--require-ssp',
                               help='Require server-side packaging',
                               default=False, action='store_true')


    def parse(self):
        deps_info = topic_common.item_parse_info(attribute_name='dependencies',
                                                 inline_option='dependencies')
        options, leftover = super(job_create, self).parse(
                parse_info=[deps_info])

        if (len(self.hosts) == 0 and not self.one_time_hosts
            and not options.labels):
            self.invalid_syntax('Must specify at least one machine.'
                                '(-m, -M, -b or --one-time-hosts).')
        if not options.control_file and not options.test:
            self.invalid_syntax('Must specify either --test or --control-file'
                                ' to create a job.')
        if options.control_file and options.test:
            self.invalid_syntax('Can only specify one of --control-file or '
                                '--test, not both.')
        if options.control_file:
            try:
                control_file_f = open(options.control_file)
                try:
                    control_file_data = control_file_f.read()
                finally:
                    control_file_f.close()
            except IOError:
                self.generic_error('Unable to read from specified '
                                   'control-file: %s' % options.control_file)
            self.data['control_file'] = control_file_data
        if options.test:
            if options.server:
                self.invalid_syntax('If you specify tests, then the '
                                    'client/server setting is implicit and '
                                    'cannot be overriden.')
            tests = [t.strip() for t in options.test.split(',') if t.strip()]
            self.ctrl_file_data['tests'] = tests

        if options.image:
            self.data['image'] = options.image

        if options.reboot_before:
            self.data['reboot_before'] = options.reboot_before.capitalize()
        if options.reboot_after:
            self.data['reboot_after'] = options.reboot_after.capitalize()
        if options.parse_failed_repair:
            self.data['parse_failed_repair'] = (
                options.parse_failed_repair == 'true')
        if options.noverify:
            self.data['run_verify'] = False
        if options.timeout_mins:
            self.data['timeout_mins'] = options.timeout_mins
        if options.max_runtime:
            self.data['max_runtime_mins'] = options.max_runtime

        self.data['dependencies'] = self.dependencies

        if options.synch_count:
            self.data['synch_count'] = options.synch_count
        if options.server:
            self.data['control_type'] = control_data.CONTROL_TYPE_NAMES.SERVER
        else:
            self.data['control_type'] = control_data.CONTROL_TYPE_NAMES.CLIENT

        self.data['require_ssp'] = options.require_ssp

        return options, leftover


    def execute(self):
        if self.ctrl_file_data:
            cf_info = self.execute_rpc(op='generate_control_file',
                                       item=self.jobname,
                                       **self.ctrl_file_data)

            self.data['control_file'] = cf_info['control_file']
            if 'synch_count' not in self.data:
                self.data['synch_count'] = cf_info['synch_count']
            if cf_info['is_server']:
                self.data['control_type'] = control_data.CONTROL_TYPE_NAMES.SERVER
            else:
                self.data['control_type'] = control_data.CONTROL_TYPE_NAMES.CLIENT

            # Get the union of the 2 sets of dependencies
            deps = set(self.data['dependencies'])
            deps = sorted(deps.union(cf_info['dependencies']))
            self.data['dependencies'] = list(deps)

        if 'synch_count' not in self.data:
            self.data['synch_count'] = 1

        return self.create_job()


class job_clone(job_create_or_clone):
    """atest job clone [--priority <int>]
    [--mlist </path/to/machinelist>] [--machine <host1 host2 host3>]
    [--labels <list of labels of machines to run on>]
    [--one-time-hosts <hosts>] [--email <email>]
    job_name

    Cloning a job is rather different from the other create operations,
    so it only uses the __init__() and output() from its superclass.
    """
    op_action = 'clone'
    usage_action = 'clone'

    def __init__(self):
        super(job_clone, self).__init__()
        self.parser.add_option('-i', '--id', help='Job id to clone',
                               default=False,
                               metavar='JOB_ID')
        self.parser.add_option('-r', '--reuse-hosts',
                               help='Use the exact same hosts as the '
                               'cloned job.',
                               action='store_true', default=False)


    def parse(self):
        options, leftover = super(job_clone, self).parse()

        self.clone_id = options.id
        self.reuse_hosts = options.reuse_hosts

        host_specified = self.hosts or self.one_time_hosts or options.labels
        if self.reuse_hosts and host_specified:
            self.invalid_syntax('Cannot specify hosts and reuse the same '
                                'ones as the cloned job.')

        if not (self.reuse_hosts or host_specified):
            self.invalid_syntax('Must reuse or specify at least one '
                                'machine (-r, -m, -M, -b or '
                                '--one-time-hosts).')

        return options, leftover


    def execute(self):
        clone_info = self.execute_rpc(op='get_info_for_clone',
                                      id=self.clone_id,
                                      preserve_metahosts=self.reuse_hosts)

        # Remove fields from clone data that cannot be reused
        for field in ('name', 'created_on', 'id', 'owner'):
            del clone_info['job'][field]

        # Also remove parameterized_job field, as the feature still is
        # incomplete, this tool does not attempt to support it for now,
        # it uses a different API function and it breaks create_job()
        if clone_info['job'].has_key('parameterized_job'):
            del clone_info['job']['parameterized_job']

        # Keyword args cannot be unicode strings
        self.data.update((str(key), val)
                         for key, val in clone_info['job'].iteritems())

        if self.reuse_hosts:
            # Convert host list from clone info that can be used for job_create
            for label, qty in clone_info['meta_host_counts'].iteritems():
                self.data['meta_hosts'].extend([label]*qty)

            self.data['hosts'].extend(host['hostname']
                                      for host in clone_info['hosts'])

        return self.create_job()


class job_abort(job, action_common.atest_delete):
    """atest job abort <job(s)>"""
    usage_action = op_action = 'abort'
    msg_done = 'Aborted'

    def parse(self):
        job_info = topic_common.item_parse_info(attribute_name='jobids',
                                                use_leftover=True)
        options, leftover = super(job_abort, self).parse([job_info],
                                                         req_items='jobids')


    def execute(self):
        data = {'job__id__in': self.jobids}
        self.execute_rpc(op='abort_host_queue_entries', **data)
        print 'Aborting jobs: %s' % ', '.join(self.jobids)


    def get_items(self):
        return self.jobids
