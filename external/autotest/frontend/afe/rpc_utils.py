# pylint: disable=missing-docstring
"""
Utility functions for rpc_interface.py.  We keep them in a separate file so that
only RPC interface functions go into that file.
"""

__author__ = 'showard@google.com (Steve Howard)'

import collections
import datetime
from functools import wraps
import inspect
import logging
import os
import sys
import django.db.utils
import django.http

from autotest_lib.frontend import thread_local
from autotest_lib.frontend.afe import models, model_logic
from autotest_lib.client.common_lib import control_data, error
from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib import time_utils
from autotest_lib.client.common_lib.cros import dev_server
from autotest_lib.server import utils as server_utils
from autotest_lib.server.cros import provision
from autotest_lib.server.cros.dynamic_suite import frontend_wrappers

NULL_DATETIME = datetime.datetime.max
NULL_DATE = datetime.date.max
DUPLICATE_KEY_MSG = 'Duplicate entry'
RESPECT_STATIC_LABELS = global_config.global_config.get_config_value(
        'SKYLAB', 'respect_static_labels', type=bool, default=False)

def prepare_for_serialization(objects):
    """
    Prepare Python objects to be returned via RPC.
    @param objects: objects to be prepared.
    """
    if (isinstance(objects, list) and len(objects) and
        isinstance(objects[0], dict) and 'id' in objects[0]):
        objects = _gather_unique_dicts(objects)
    return _prepare_data(objects)


def prepare_rows_as_nested_dicts(query, nested_dict_column_names):
    """
    Prepare a Django query to be returned via RPC as a sequence of nested
    dictionaries.

    @param query - A Django model query object with a select_related() method.
    @param nested_dict_column_names - A list of column/attribute names for the
            rows returned by query to expand into nested dictionaries using
            their get_object_dict() method when not None.

    @returns An list suitable to returned in an RPC.
    """
    all_dicts = []
    for row in query.select_related():
        row_dict = row.get_object_dict()
        for column in nested_dict_column_names:
            if row_dict[column] is not None:
                row_dict[column] = getattr(row, column).get_object_dict()
        all_dicts.append(row_dict)
    return prepare_for_serialization(all_dicts)


def _prepare_data(data):
    """
    Recursively process data structures, performing necessary type
    conversions to values in data to allow for RPC serialization:
    -convert datetimes to strings
    -convert tuples and sets to lists
    """
    if isinstance(data, dict):
        new_data = {}
        for key, value in data.iteritems():
            new_data[key] = _prepare_data(value)
        return new_data
    elif (isinstance(data, list) or isinstance(data, tuple) or
          isinstance(data, set)):
        return [_prepare_data(item) for item in data]
    elif isinstance(data, datetime.date):
        if data is NULL_DATETIME or data is NULL_DATE:
            return None
        return str(data)
    else:
        return data


def fetchall_as_list_of_dicts(cursor):
    """
    Converts each row in the cursor to a dictionary so that values can be read
    by using the column name.
    @param cursor: The database cursor to read from.
    @returns: A list of each row in the cursor as a dictionary.
    """
    desc = cursor.description
    return [ dict(zip([col[0] for col in desc], row))
             for row in cursor.fetchall() ]


def raw_http_response(response_data, content_type=None):
    response = django.http.HttpResponse(response_data, mimetype=content_type)
    response['Content-length'] = str(len(response.content))
    return response


def _gather_unique_dicts(dict_iterable):
    """\
    Pick out unique objects (by ID) from an iterable of object dicts.
    """
    objects = collections.OrderedDict()
    for obj in dict_iterable:
        objects.setdefault(obj['id'], obj)
    return objects.values()


def extra_job_status_filters(not_yet_run=False, running=False, finished=False):
    """\
    Generate a SQL WHERE clause for job status filtering, and return it in
    a dict of keyword args to pass to query.extra().
    * not_yet_run: all HQEs are Queued
    * finished: all HQEs are complete
    * running: everything else
    """
    if not (not_yet_run or running or finished):
        return {}
    not_queued = ('(SELECT job_id FROM afe_host_queue_entries '
                  'WHERE status != "%s")'
                  % models.HostQueueEntry.Status.QUEUED)
    not_finished = ('(SELECT job_id FROM afe_host_queue_entries '
                    'WHERE not complete)')

    where = []
    if not_yet_run:
        where.append('id NOT IN ' + not_queued)
    if running:
        where.append('(id IN %s) AND (id IN %s)' % (not_queued, not_finished))
    if finished:
        where.append('id NOT IN ' + not_finished)
    return {'where': [' OR '.join(['(%s)' % x for x in where])]}


def extra_job_type_filters(extra_args, suite=False,
                           sub=False, standalone=False):
    """\
    Generate a SQL WHERE clause for job status filtering, and return it in
    a dict of keyword args to pass to query.extra().

    param extra_args: a dict of existing extra_args.

    No more than one of the parameters should be passed as True:
    * suite: job which is parent of other jobs
    * sub: job with a parent job
    * standalone: job with no child or parent jobs
    """
    assert not ((suite and sub) or
                (suite and standalone) or
                (sub and standalone)), ('Cannot specify more than one '
                                        'filter to this function')

    where = extra_args.get('where', [])
    parent_job_id = ('DISTINCT parent_job_id')
    child_job_id = ('id')
    filter_common = ('(SELECT %s FROM afe_jobs '
                     'WHERE parent_job_id IS NOT NULL)')

    if suite:
        where.append('id IN ' + filter_common % parent_job_id)
    elif sub:
        where.append('id IN ' + filter_common % child_job_id)
    elif standalone:
        where.append('NOT EXISTS (SELECT 1 from afe_jobs AS sub_query '
                     'WHERE parent_job_id IS NOT NULL'
                     ' AND (sub_query.parent_job_id=afe_jobs.id'
                     ' OR sub_query.id=afe_jobs.id))')
    else:
        return extra_args

    extra_args['where'] = where
    return extra_args


def get_host_query(multiple_labels, exclude_only_if_needed_labels,
                   valid_only, filter_data):
    """
    @param exclude_only_if_needed_labels: Deprecated. By default it's false.
    """
    if valid_only:
        initial_query = models.Host.valid_objects.all()
    else:
        initial_query = models.Host.objects.all()

    try:
        hosts = models.Host.get_hosts_with_labels(
                multiple_labels, initial_query)
        if not hosts:
            return hosts

        return models.Host.query_objects(filter_data, initial_query=hosts)
    except models.Label.DoesNotExist:
        return models.Host.objects.none()


class InconsistencyException(Exception):
    'Raised when a list of objects does not have a consistent value'


def get_consistent_value(objects, field):
    if not objects:
        # well a list of nothing is consistent
        return None

    value = getattr(objects[0], field)
    for obj in objects:
        this_value = getattr(obj, field)
        if this_value != value:
            raise InconsistencyException(objects[0], obj)
    return value


def afe_test_dict_to_test_object(test_dict):
    if not isinstance(test_dict, dict):
        return test_dict

    numerized_dict = {}
    for key, value in test_dict.iteritems():
        try:
            numerized_dict[key] = int(value)
        except (ValueError, TypeError):
            numerized_dict[key] = value

    return type('TestObject', (object,), numerized_dict)


def _check_is_server_test(test_type):
    """Checks if the test type is a server test.

    @param test_type The test type in enum integer or string.

    @returns A boolean to identify if the test type is server test.
    """
    if test_type is not None:
        if isinstance(test_type, basestring):
            try:
                test_type = control_data.CONTROL_TYPE.get_value(test_type)
            except AttributeError:
                return False
        return (test_type == control_data.CONTROL_TYPE.SERVER)
    return False


def prepare_generate_control_file(tests, profilers, db_tests=True):
    if db_tests:
        test_objects = [models.Test.smart_get(test) for test in tests]
    else:
        test_objects = [afe_test_dict_to_test_object(test) for test in tests]

    profiler_objects = [models.Profiler.smart_get(profiler)
                        for profiler in profilers]
    # ensure tests are all the same type
    try:
        test_type = get_consistent_value(test_objects, 'test_type')
    except InconsistencyException, exc:
        test1, test2 = exc.args
        raise model_logic.ValidationError(
            {'tests' : 'You cannot run both test_suites and server-side '
             'tests together (tests %s and %s differ' % (
            test1.name, test2.name)})

    is_server = _check_is_server_test(test_type)
    if test_objects:
        synch_count = max(test.sync_count for test in test_objects)
    else:
        synch_count = 1

    if db_tests:
        dependencies = set(label.name for label
                           in models.Label.objects.filter(test__in=test_objects))
    else:
        dependencies = reduce(
                set.union, [set(test.dependencies) for test in test_objects])

    cf_info = dict(is_server=is_server, synch_count=synch_count,
                   dependencies=list(dependencies))
    return cf_info, test_objects, profiler_objects


def check_job_dependencies(host_objects, job_dependencies):
    """
    Check that a set of machines satisfies a job's dependencies.
    host_objects: list of models.Host objects
    job_dependencies: list of names of labels
    """
    # check that hosts satisfy dependencies
    host_ids = [host.id for host in host_objects]
    hosts_in_job = models.Host.objects.filter(id__in=host_ids)
    ok_hosts = hosts_in_job
    for index, dependency in enumerate(job_dependencies):
        if not provision.is_for_special_action(dependency):
            try:
              label = models.Label.smart_get(dependency)
            except models.Label.DoesNotExist:
              logging.info('Label %r does not exist, so it cannot '
                           'be replaced by static label.', dependency)
              label = None

            if label is not None and label.is_replaced_by_static():
                ok_hosts = ok_hosts.filter(static_labels__name=dependency)
            else:
                ok_hosts = ok_hosts.filter(labels__name=dependency)

    failing_hosts = (set(host.hostname for host in host_objects) -
                     set(host.hostname for host in ok_hosts))
    if failing_hosts:
        raise model_logic.ValidationError(
            {'hosts' : 'Host(s) failed to meet job dependencies (' +
                       (', '.join(job_dependencies)) + '): ' +
                       (', '.join(failing_hosts))})


def check_job_metahost_dependencies(metahost_objects, job_dependencies):
    """
    Check that at least one machine within the metahost spec satisfies the job's
    dependencies.

    @param metahost_objects A list of label objects representing the metahosts.
    @param job_dependencies A list of strings of the required label names.
    @raises NoEligibleHostException If a metahost cannot run the job.
    """
    for metahost in metahost_objects:
        if metahost.is_replaced_by_static():
            static_metahost = models.StaticLabel.smart_get(metahost.name)
            hosts = models.Host.objects.filter(static_labels=static_metahost)
        else:
            hosts = models.Host.objects.filter(labels=metahost)

        for label_name in job_dependencies:
            if not provision.is_for_special_action(label_name):
                try:
                    label = models.Label.smart_get(label_name)
                except models.Label.DoesNotExist:
                    logging.info('Label %r does not exist, so it cannot '
                                 'be replaced by static label.', label_name)
                    label = None

                if label is not None and label.is_replaced_by_static():
                    hosts = hosts.filter(static_labels__name=label_name)
                else:
                    hosts = hosts.filter(labels__name=label_name)

        if not any(hosts):
            raise error.NoEligibleHostException("No hosts within %s satisfy %s."
                    % (metahost.name, ', '.join(job_dependencies)))


def _execution_key_for(host_queue_entry):
    return (host_queue_entry.job.id, host_queue_entry.execution_subdir)


def check_abort_synchronous_jobs(host_queue_entries):
    # ensure user isn't aborting part of a synchronous autoserv execution
    count_per_execution = {}
    for queue_entry in host_queue_entries:
        key = _execution_key_for(queue_entry)
        count_per_execution.setdefault(key, 0)
        count_per_execution[key] += 1

    for queue_entry in host_queue_entries:
        if not queue_entry.execution_subdir:
            continue
        execution_count = count_per_execution[_execution_key_for(queue_entry)]
        if execution_count < queue_entry.job.synch_count:
            raise model_logic.ValidationError(
                {'' : 'You cannot abort part of a synchronous job execution '
                      '(%d/%s), %d included, %d expected'
                      % (queue_entry.job.id, queue_entry.execution_subdir,
                         execution_count, queue_entry.job.synch_count)})


def check_modify_host(update_data):
    """
    Sanity check modify_host* requests.

    @param update_data: A dictionary with the changes to make to a host
            or hosts.
    """
    # Only the scheduler (monitor_db) is allowed to modify Host status.
    # Otherwise race conditions happen as a hosts state is changed out from
    # beneath tasks being run on a host.
    if 'status' in update_data:
        raise model_logic.ValidationError({
                'status': 'Host status can not be modified by the frontend.'})


def check_modify_host_locking(host, update_data):
    """
    Checks when locking/unlocking has been requested if the host is already
    locked/unlocked.

    @param host: models.Host object to be modified
    @param update_data: A dictionary with the changes to make to the host.
    """
    locked = update_data.get('locked', None)
    lock_reason = update_data.get('lock_reason', None)
    if locked is not None:
        if locked and host.locked:
            raise model_logic.ValidationError({
                    'locked': 'Host %s already locked by %s on %s.' %
                    (host.hostname, host.locked_by, host.lock_time)})
        if not locked and not host.locked:
            raise model_logic.ValidationError({
                    'locked': 'Host %s already unlocked.' % host.hostname})
        if locked and not lock_reason and not host.locked:
            raise model_logic.ValidationError({
                    'locked': 'Please provide a reason for locking Host %s' %
                    host.hostname})


def get_motd():
    dirname = os.path.dirname(__file__)
    filename = os.path.join(dirname, "..", "..", "motd.txt")
    text = ''
    try:
        fp = open(filename, "r")
        try:
            text = fp.read()
        finally:
            fp.close()
    except:
        pass

    return text


def _get_metahost_counts(metahost_objects):
    metahost_counts = {}
    for metahost in metahost_objects:
        metahost_counts.setdefault(metahost, 0)
        metahost_counts[metahost] += 1
    return metahost_counts


def get_job_info(job, preserve_metahosts=False, queue_entry_filter_data=None):
    hosts = []
    one_time_hosts = []
    meta_hosts = []
    hostless = False

    queue_entries = job.hostqueueentry_set.all()
    if queue_entry_filter_data:
        queue_entries = models.HostQueueEntry.query_objects(
            queue_entry_filter_data, initial_query=queue_entries)

    for queue_entry in queue_entries:
        if (queue_entry.host and (preserve_metahosts or
                                  not queue_entry.meta_host)):
            if queue_entry.deleted:
                continue
            if queue_entry.host.invalid:
                one_time_hosts.append(queue_entry.host)
            else:
                hosts.append(queue_entry.host)
        elif queue_entry.meta_host:
            meta_hosts.append(queue_entry.meta_host)
        else:
            hostless = True

    meta_host_counts = _get_metahost_counts(meta_hosts)

    info = dict(dependencies=[label.name for label
                              in job.dependency_labels.all()],
                hosts=hosts,
                meta_hosts=meta_hosts,
                meta_host_counts=meta_host_counts,
                one_time_hosts=one_time_hosts,
                hostless=hostless)
    return info


def check_for_duplicate_hosts(host_objects):
    host_counts = collections.Counter(host_objects)
    duplicate_hostnames = {host.hostname
                           for host, count in host_counts.iteritems()
                           if count > 1}
    if duplicate_hostnames:
        raise model_logic.ValidationError(
                {'hosts' : 'Duplicate hosts: %s'
                 % ', '.join(duplicate_hostnames)})


def create_new_job(owner, options, host_objects, metahost_objects):
    all_host_objects = host_objects + metahost_objects
    dependencies = options.get('dependencies', [])
    synch_count = options.get('synch_count')

    if synch_count is not None and synch_count > len(all_host_objects):
        raise model_logic.ValidationError(
                {'hosts':
                 'only %d hosts provided for job with synch_count = %d' %
                 (len(all_host_objects), synch_count)})

    check_for_duplicate_hosts(host_objects)

    for label_name in dependencies:
        if provision.is_for_special_action(label_name):
            # TODO: We could save a few queries
            # if we had a bulk ensure-label-exists function, which used
            # a bulk .get() call. The win is probably very small.
            _ensure_label_exists(label_name)

    # This only checks targeted hosts, not hosts eligible due to the metahost
    check_job_dependencies(host_objects, dependencies)
    check_job_metahost_dependencies(metahost_objects, dependencies)

    options['dependencies'] = list(
            models.Label.objects.filter(name__in=dependencies))

    job = models.Job.create(owner=owner, options=options,
                            hosts=all_host_objects)
    job.queue(all_host_objects,
              is_template=options.get('is_template', False))
    return job.id


def _ensure_label_exists(name):
    """
    Ensure that a label called |name| exists in the Django models.

    This function is to be called from within afe rpcs only, as an
    alternative to server.cros.provision.ensure_label_exists(...). It works
    by Django model manipulation, rather than by making another create_label
    rpc call.

    @param name: the label to check for/create.
    @raises ValidationError: There was an error in the response that was
                             not because the label already existed.
    @returns True is a label was created, False otherwise.
    """
    # Make sure this function is not called on shards but only on master.
    assert not server_utils.is_shard()
    try:
        models.Label.objects.get(name=name)
    except models.Label.DoesNotExist:
        try:
            new_label = models.Label.objects.create(name=name)
            new_label.save()
            return True
        except django.db.utils.IntegrityError as e:
            # It is possible that another suite/test already
            # created the label between the check and save.
            if DUPLICATE_KEY_MSG in str(e):
                return False
            else:
                raise
    return False


def find_platform(hostname, label_list):
    """
    Figure out the platform name for the given host
    object.  If none, the return value for either will be None.

    @param hostname: The hostname to find platform.
    @param label_list: The label list to find platform.

    @returns platform name for the given host.
    """
    platforms = [label.name for label in label_list if label.platform]
    if not platforms:
        platform = None
    else:
        platform = platforms[0]

    if len(platforms) > 1:
        raise ValueError('Host %s has more than one platform: %s' %
                         (hostname, ', '.join(platforms)))

    return platform


# support for get_host_queue_entries_and_special_tasks()

def _common_entry_to_dict(entry, type, job_dict, exec_path, status, started_on):
    return dict(type=type,
                host=entry['host'],
                job=job_dict,
                execution_path=exec_path,
                status=status,
                started_on=started_on,
                id=str(entry['id']) + type,
                oid=entry['id'])


def _special_task_to_dict(task, queue_entries):
    """Transforms a special task dictionary to another form of dictionary.

    @param task           Special task as a dictionary type
    @param queue_entries  Host queue entries as a list of dictionaries.

    @return Transformed dictionary for a special task.
    """
    job_dict = None
    if task['queue_entry']:
        # Scan queue_entries to get the job detail info.
        for qentry in queue_entries:
            if task['queue_entry']['id'] == qentry['id']:
                job_dict = qentry['job']
                break
        # If not found, get it from DB.
        if job_dict is None:
            job = models.Job.objects.get(id=task['queue_entry']['job'])
            job_dict = job.get_object_dict()

    exec_path = server_utils.get_special_task_exec_path(
            task['host']['hostname'], task['id'], task['task'],
            time_utils.time_string_to_datetime(task['time_requested']))
    status = server_utils.get_special_task_status(
            task['is_complete'], task['success'], task['is_active'])
    return _common_entry_to_dict(task, task['task'], job_dict,
            exec_path, status, task['time_started'])


def _queue_entry_to_dict(queue_entry):
    job_dict = queue_entry['job']
    tag = server_utils.get_job_tag(job_dict['id'], job_dict['owner'])
    exec_path = server_utils.get_hqe_exec_path(tag,
                                               queue_entry['execution_subdir'])
    return _common_entry_to_dict(queue_entry, 'Job', job_dict, exec_path,
            queue_entry['status'], queue_entry['started_on'])


def prepare_host_queue_entries_and_special_tasks(interleaved_entries,
                                                 queue_entries):
    """
    Prepare for serialization the interleaved entries of host queue entries
    and special tasks.
    Each element in the entries is a dictionary type.
    The special task dictionary has only a job id for a job and lacks
    the detail of the job while the host queue entry dictionary has.
    queue_entries is used to look up the job detail info.

    @param interleaved_entries  Host queue entries and special tasks as a list
                                of dictionaries.
    @param queue_entries        Host queue entries as a list of dictionaries.

    @return A post-processed list of dictionaries that is to be serialized.
    """
    dict_list = []
    for e in interleaved_entries:
        # Distinguish the two mixed entries based on the existence of
        # the key "task". If an entry has the key, the entry is for
        # special task. Otherwise, host queue entry.
        if 'task' in e:
            dict_list.append(_special_task_to_dict(e, queue_entries))
        else:
            dict_list.append(_queue_entry_to_dict(e))
    return prepare_for_serialization(dict_list)


def _compute_next_job_for_tasks(queue_entries, special_tasks):
    """
    For each task, try to figure out the next job that ran after that task.
    This is done using two pieces of information:
    * if the task has a queue entry, we can use that entry's job ID.
    * if the task has a time_started, we can try to compare that against the
      started_on field of queue_entries. this isn't guaranteed to work perfectly
      since queue_entries may also have null started_on values.
    * if the task has neither, or if use of time_started fails, just use the
      last computed job ID.

    @param queue_entries    Host queue entries as a list of dictionaries.
    @param special_tasks    Special tasks as a list of dictionaries.
    """
    next_job_id = None # most recently computed next job
    hqe_index = 0 # index for scanning by started_on times
    for task in special_tasks:
        if task['queue_entry']:
            next_job_id = task['queue_entry']['job']
        elif task['time_started'] is not None:
            for queue_entry in queue_entries[hqe_index:]:
                if queue_entry['started_on'] is None:
                    continue
                t1 = time_utils.time_string_to_datetime(
                        queue_entry['started_on'])
                t2 = time_utils.time_string_to_datetime(task['time_started'])
                if t1 < t2:
                    break
                next_job_id = queue_entry['job']['id']

        task['next_job_id'] = next_job_id

        # advance hqe_index to just after next_job_id
        if next_job_id is not None:
            for queue_entry in queue_entries[hqe_index:]:
                if queue_entry['job']['id'] < next_job_id:
                    break
                hqe_index += 1


def interleave_entries(queue_entries, special_tasks):
    """
    Both lists should be ordered by descending ID.
    """
    _compute_next_job_for_tasks(queue_entries, special_tasks)

    # start with all special tasks that've run since the last job
    interleaved_entries = []
    for task in special_tasks:
        if task['next_job_id'] is not None:
            break
        interleaved_entries.append(task)

    # now interleave queue entries with the remaining special tasks
    special_task_index = len(interleaved_entries)
    for queue_entry in queue_entries:
        interleaved_entries.append(queue_entry)
        # add all tasks that ran between this job and the previous one
        for task in special_tasks[special_task_index:]:
            if task['next_job_id'] < queue_entry['job']['id']:
                break
            interleaved_entries.append(task)
            special_task_index += 1

    return interleaved_entries


def bucket_hosts_by_shard(host_objs):
    """Figure out which hosts are on which shards.

    @param host_objs: A list of host objects.

    @return: A map of shard hostname: list of hosts on the shard.
    """
    shard_host_map = collections.defaultdict(list)
    for host in host_objs:
        if host.shard:
            shard_host_map[host.shard.hostname].append(host.hostname)
    return shard_host_map


def create_job_common(
        name,
        priority,
        control_type,
        control_file=None,
        hosts=(),
        meta_hosts=(),
        one_time_hosts=(),
        synch_count=None,
        is_template=False,
        timeout=None,
        timeout_mins=None,
        max_runtime_mins=None,
        run_verify=True,
        email_list='',
        dependencies=(),
        reboot_before=None,
        reboot_after=None,
        parse_failed_repair=None,
        hostless=False,
        keyvals=None,
        drone_set=None,
        parent_job_id=None,
        test_retry=0,
        run_reset=True,
        require_ssp=None):
    #pylint: disable-msg=C0111
    """
    Common code between creating "standard" jobs and creating parameterized jobs
    """
    # input validation
    host_args_passed = any((hosts, meta_hosts, one_time_hosts))
    if hostless:
        if host_args_passed:
            raise model_logic.ValidationError({
                    'hostless': 'Hostless jobs cannot include any hosts!'})
        if control_type != control_data.CONTROL_TYPE_NAMES.SERVER:
            raise model_logic.ValidationError({
                    'control_type': 'Hostless jobs cannot use client-side '
                                    'control files'})
    elif not host_args_passed:
        raise model_logic.ValidationError({
            'arguments' : "For host jobs, you must pass at least one of"
                          " 'hosts', 'meta_hosts', 'one_time_hosts'."
            })
    label_objects = list(models.Label.objects.filter(name__in=meta_hosts))

    # convert hostnames & meta hosts to host/label objects
    host_objects = models.Host.smart_get_bulk(hosts)
    _validate_host_job_sharding(host_objects)
    for host in one_time_hosts:
        this_host = models.Host.create_one_time_host(host)
        host_objects.append(this_host)

    metahost_objects = []
    meta_host_labels_by_name = {label.name: label for label in label_objects}
    for label_name in meta_hosts:
        if label_name in meta_host_labels_by_name:
            metahost_objects.append(meta_host_labels_by_name[label_name])
        else:
            raise model_logic.ValidationError(
                {'meta_hosts' : 'Label "%s" not found' % label_name})

    options = dict(name=name,
                   priority=priority,
                   control_file=control_file,
                   control_type=control_type,
                   is_template=is_template,
                   timeout=timeout,
                   timeout_mins=timeout_mins,
                   max_runtime_mins=max_runtime_mins,
                   synch_count=synch_count,
                   run_verify=run_verify,
                   email_list=email_list,
                   dependencies=dependencies,
                   reboot_before=reboot_before,
                   reboot_after=reboot_after,
                   parse_failed_repair=parse_failed_repair,
                   keyvals=keyvals,
                   drone_set=drone_set,
                   parent_job_id=parent_job_id,
                   test_retry=test_retry,
                   run_reset=run_reset,
                   require_ssp=require_ssp)

    return create_new_job(owner=models.User.current_user().login,
                          options=options,
                          host_objects=host_objects,
                          metahost_objects=metahost_objects)


def _validate_host_job_sharding(host_objects):
    """Check that the hosts obey job sharding rules."""
    if not (server_utils.is_shard()
            or _allowed_hosts_for_master_job(host_objects)):
        shard_host_map = bucket_hosts_by_shard(host_objects)
        raise ValueError(
                'The following hosts are on shard(s), please create '
                'seperate jobs for hosts on each shard: %s ' %
                shard_host_map)


def _allowed_hosts_for_master_job(host_objects):
    """Check that the hosts are allowed for a job on master."""
    # We disallow the following jobs on master:
    #   num_shards > 1: this is a job spanning across multiple shards.
    #   num_shards == 1 but number of hosts on shard is less
    #   than total number of hosts: this is a job that spans across
    #   one shard and the master.
    shard_host_map = bucket_hosts_by_shard(host_objects)
    num_shards = len(shard_host_map)
    if num_shards > 1:
        return False
    if num_shards == 1:
        hosts_on_shard = shard_host_map.values()[0]
        assert len(hosts_on_shard) <= len(host_objects)
        return len(hosts_on_shard) == len(host_objects)
    else:
        return True


def encode_ascii(control_file):
    """Force a control file to only contain ascii characters.

    @param control_file: Control file to encode.

    @returns the control file in an ascii encoding.

    @raises error.ControlFileMalformed: if encoding fails.
    """
    try:
        return control_file.encode('ascii')
    except UnicodeDecodeError as e:
        raise error.ControlFileMalformed(str(e))


def get_wmatrix_url():
    """Get wmatrix url from config file.

    @returns the wmatrix url or an empty string.
    """
    return global_config.global_config.get_config_value('AUTOTEST_WEB',
                                                        'wmatrix_url',
                                                        default='')


def get_stainless_url():
    """Get stainless url from config file.

    @returns the stainless url or an empty string.
    """
    return global_config.global_config.get_config_value('AUTOTEST_WEB',
                                                        'stainless_url',
                                                        default='')


def inject_times_to_filter(start_time_key=None, end_time_key=None,
                         start_time_value=None, end_time_value=None,
                         **filter_data):
    """Inject the key value pairs of start and end time if provided.

    @param start_time_key: A string represents the filter key of start_time.
    @param end_time_key: A string represents the filter key of end_time.
    @param start_time_value: Start_time value.
    @param end_time_value: End_time value.

    @returns the injected filter_data.
    """
    if start_time_value:
        filter_data[start_time_key] = start_time_value
    if end_time_value:
        filter_data[end_time_key] = end_time_value
    return filter_data


def inject_times_to_hqe_special_tasks_filters(filter_data_common,
                                              start_time, end_time):
    """Inject start and end time to hqe and special tasks filters.

    @param filter_data_common: Common filter for hqe and special tasks.
    @param start_time_key: A string represents the filter key of start_time.
    @param end_time_key: A string represents the filter key of end_time.

    @returns a pair of hqe and special tasks filters.
    """
    filter_data_special_tasks = filter_data_common.copy()
    return (inject_times_to_filter('started_on__gte', 'started_on__lte',
                                   start_time, end_time, **filter_data_common),
           inject_times_to_filter('time_started__gte', 'time_started__lte',
                                  start_time, end_time,
                                  **filter_data_special_tasks))


def retrieve_shard(shard_hostname):
    """
    Retrieves the shard with the given hostname from the database.

    @param shard_hostname: Hostname of the shard to retrieve

    @raises models.Shard.DoesNotExist, if no shard with this hostname was found.

    @returns: Shard object
    """
    return models.Shard.smart_get(shard_hostname)


def find_records_for_shard(shard, known_job_ids, known_host_ids):
    """Find records that should be sent to a shard.

    @param shard: Shard to find records for.
    @param known_job_ids: List of ids of jobs the shard already has.
    @param known_host_ids: List of ids of hosts the shard already has.

    @returns: Tuple of lists:
              (hosts, jobs, suite_job_keyvals, invalid_host_ids)
    """
    hosts, invalid_host_ids = models.Host.assign_to_shard(
            shard, known_host_ids)
    jobs = models.Job.assign_to_shard(shard, known_job_ids)
    parent_job_ids = [job.parent_job_id for job in jobs]
    suite_job_keyvals = models.JobKeyval.objects.filter(
            job_id__in=parent_job_ids)
    return hosts, jobs, suite_job_keyvals, invalid_host_ids


def _persist_records_with_type_sent_from_shard(
    shard, records, record_type, *args, **kwargs):
    """
    Handle records of a specified type that were sent to the shard master.

    @param shard: The shard the records were sent from.
    @param records: The records sent in their serialized format.
    @param record_type: Type of the objects represented by records.
    @param args: Additional arguments that will be passed on to the sanity
                 checks.
    @param kwargs: Additional arguments that will be passed on to the sanity
                  checks.

    @raises error.UnallowedRecordsSentToMaster if any of the sanity checks fail.

    @returns: List of primary keys of the processed records.
    """
    pks = []
    for serialized_record in records:
        pk = serialized_record['id']
        try:
            current_record = record_type.objects.get(pk=pk)
        except record_type.DoesNotExist:
            raise error.UnallowedRecordsSentToMaster(
                'Object with pk %s of type %s does not exist on master.' % (
                    pk, record_type))

        try:
            current_record.sanity_check_update_from_shard(
                shard, serialized_record, *args, **kwargs)
        except error.IgnorableUnallowedRecordsSentToMaster:
            # An illegal record change was attempted, but it was of a non-fatal
            # variety. Silently skip this record.
            pass
        else:
            current_record.update_from_serialized(serialized_record)
            pks.append(pk)

    return pks


def persist_records_sent_from_shard(shard, jobs, hqes):
    """
    Sanity checking then saving serialized records sent to master from shard.

    During heartbeats shards upload jobs and hostqueuentries. This performs
    some sanity checks on these and then updates the existing records for those
    entries with the updated ones from the heartbeat.

    The sanity checks include:
    - Checking if the objects sent already exist on the master.
    - Checking if the objects sent were assigned to this shard.
    - hostqueueentries must be sent together with their jobs.

    @param shard: The shard the records were sent from.
    @param jobs: The jobs the shard sent.
    @param hqes: The hostqueuentries the shart sent.

    @raises error.UnallowedRecordsSentToMaster if any of the sanity checks fail.
    """
    job_ids_persisted = _persist_records_with_type_sent_from_shard(
            shard, jobs, models.Job)
    _persist_records_with_type_sent_from_shard(
            shard, hqes, models.HostQueueEntry,
            job_ids_sent=job_ids_persisted)


def forward_single_host_rpc_to_shard(func):
    """This decorator forwards rpc calls that modify a host to a shard.

    If a host is assigned to a shard, rpcs that change his attributes should be
    forwarded to the shard.

    This assumes the first argument of the function represents a host id.

    @param func: The function to decorate

    @returns: The function to replace func with.
    """
    def replacement(**kwargs):
        # Only keyword arguments can be accepted here, as we need the argument
        # names to send the rpc. serviceHandler always provides arguments with
        # their keywords, so this is not a problem.

        # A host record (identified by kwargs['id']) can be deleted in
        # func(). Therefore, we should save the data that can be needed later
        # before func() is called.
        shard_hostname = None
        host = models.Host.smart_get(kwargs['id'])
        if host and host.shard:
            shard_hostname = host.shard.hostname
        ret = func(**kwargs)
        if shard_hostname and not server_utils.is_shard():
            run_rpc_on_multiple_hostnames(func.func_name,
                                          [shard_hostname],
                                          **kwargs)
        return ret

    return replacement


def fanout_rpc(host_objs, rpc_name, include_hostnames=True, **kwargs):
    """Fanout the given rpc to shards of given hosts.

    @param host_objs: Host objects for the rpc.
    @param rpc_name: The name of the rpc.
    @param include_hostnames: If True, include the hostnames in the kwargs.
        Hostnames are not always necessary, this functions is designed to
        send rpcs to the shard a host is on, the rpcs themselves could be
        related to labels, acls etc.
    @param kwargs: The kwargs for the rpc.
    """
    # Figure out which hosts are on which shards.
    shard_host_map = bucket_hosts_by_shard(host_objs)

    # Execute the rpc against the appropriate shards.
    for shard, hostnames in shard_host_map.iteritems():
        if include_hostnames:
            kwargs['hosts'] = hostnames
        try:
            run_rpc_on_multiple_hostnames(rpc_name, [shard], **kwargs)
        except:
            ei = sys.exc_info()
            new_exc = error.RPCException('RPC %s failed on shard %s due to '
                    '%s: %s' % (rpc_name, shard, ei[0].__name__, ei[1]))
            raise new_exc.__class__, new_exc, ei[2]


def run_rpc_on_multiple_hostnames(rpc_call, shard_hostnames, **kwargs):
    """Runs an rpc to multiple AFEs

    This is i.e. used to propagate changes made to hosts after they are assigned
    to a shard.

    @param rpc_call: Name of the rpc endpoint to call.
    @param shard_hostnames: List of hostnames to run the rpcs on.
    @param **kwargs: Keyword arguments to pass in the rpcs.
    """
    # Make sure this function is not called on shards but only on master.
    assert not server_utils.is_shard()
    for shard_hostname in shard_hostnames:
        afe = frontend_wrappers.RetryingAFE(server=shard_hostname,
                                            user=thread_local.get_user())
        afe.run(rpc_call, **kwargs)


def get_label(name):
    """Gets a label object using a given name.

    @param name: Label name.
    @raises model.Label.DoesNotExist: when there is no label matching
                                      the given name.
    @return: a label object matching the given name.
    """
    try:
        label = models.Label.smart_get(name)
    except models.Label.DoesNotExist:
        return None
    return label


# TODO: hide the following rpcs under is_moblab
def moblab_only(func):
    """Ensure moblab specific functions only run on Moblab devices."""
    def verify(*args, **kwargs):
        if not server_utils.is_moblab():
            raise error.RPCException('RPC: %s can only run on Moblab Systems!',
                                     func.__name__)
        return func(*args, **kwargs)
    return verify


def route_rpc_to_master(func):
    """Route RPC to master AFE.

    When a shard receives an RPC decorated by this, the RPC is just
    forwarded to the master.
    When the master gets the RPC, the RPC function is executed.

    @param func: An RPC function to decorate

    @returns: A function replacing the RPC func.
    """
    argspec = inspect.getargspec(func)
    if argspec.varargs is not None:
        raise Exception('RPC function must not have *args.')

    @wraps(func)
    def replacement(*args, **kwargs):
        """We need special handling when decorating an RPC that can be called
        directly using positional arguments.

        One example is rpc_interface.create_job().
        rpc_interface.create_job_page_handler() calls the function using both
        positional and keyword arguments.  Since frontend.RpcClient.run()
        takes only keyword arguments for an RPC, positional arguments of the
        RPC function need to be transformed into keyword arguments.
        """
        kwargs = _convert_to_kwargs_only(func, args, kwargs)
        if server_utils.is_shard():
            afe = frontend_wrappers.RetryingAFE(
                    server=server_utils.get_global_afe_hostname(),
                    user=thread_local.get_user())
            return afe.run(func.func_name, **kwargs)
        return func(**kwargs)

    return replacement


def _convert_to_kwargs_only(func, args, kwargs):
    """Convert a function call's arguments to a kwargs dict.

    This is best illustrated with an example.  Given:

    def foo(a, b, **kwargs):
        pass
    _to_kwargs(foo, (1, 2), {'c': 3})  # corresponding to foo(1, 2, c=3)

        foo(**kwargs)

    @param func: function whose signature to use
    @param args: positional arguments of call
    @param kwargs: keyword arguments of call

    @returns: kwargs dict
    """
    argspec = inspect.getargspec(func)
    # callargs looks like {'a': 1, 'b': 2, 'kwargs': {'c': 3}}
    callargs = inspect.getcallargs(func, *args, **kwargs)
    if argspec.keywords is None:
        kwargs = {}
    else:
        kwargs = callargs.pop(argspec.keywords)
    kwargs.update(callargs)
    return kwargs


def get_sample_dut(board, pool):
    """Get a dut with the given board and pool.

    This method is used to help to locate a dut with the given board and pool.
    The dut then can be used to identify a devserver in the same subnet.

    @param board: Name of the board.
    @param pool: Name of the pool.

    @return: Name of a dut with the given board and pool.
    """
    if not (dev_server.PREFER_LOCAL_DEVSERVER and pool and board):
        return None

    hosts = list(get_host_query(
        multiple_labels=('pool:%s' % pool, 'board:%s' % board),
        exclude_only_if_needed_labels=False,
        valid_only=True,
        filter_data={},
    ))
    if not hosts:
        return None
    else:
        return hosts[0].hostname
