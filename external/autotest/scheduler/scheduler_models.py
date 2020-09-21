# pylint: disable=missing-docstring

"""Database model classes for the scheduler.

Contains model classes abstracting the various DB tables used by the scheduler.
These overlap the Django models in basic functionality, but were written before
the Django models existed and have not yet been phased out.  Some of them
(particularly HostQueueEntry and Job) have considerable scheduler-specific logic
which would probably be ill-suited for inclusion in the general Django model
classes.

Globals:
_notify_email_statuses: list of HQE statuses.  each time a single HQE reaches
        one of these statuses, an email will be sent to the job's email_list.
        comes from global_config.
_base_url: URL to the local AFE server, used to construct URLs for emails.
_db: DatabaseConnection for this module.
_drone_manager: reference to global DroneManager instance.
"""

import base64
import datetime
import errno
import itertools
import logging
import re
import weakref

import google.protobuf.internal.well_known_types as types

from autotest_lib.client.common_lib import global_config, host_protections
from autotest_lib.client.common_lib import time_utils
from autotest_lib.client.common_lib import utils
from autotest_lib.frontend.afe import models, model_attributes
from autotest_lib.scheduler import drone_manager, email_manager
from autotest_lib.scheduler import rdb_lib
from autotest_lib.scheduler import scheduler_config
from autotest_lib.scheduler import scheduler_lib
from autotest_lib.server import afe_urls
from autotest_lib.server.cros import provision

try:
    from chromite.lib import metrics
    from chromite.lib import cloud_trace
except ImportError:
    metrics = utils.metrics_mock
    import mock
    cloud_trace = mock.Mock()


_notify_email_statuses = []
_base_url = None

_db = None
_drone_manager = None

RESPECT_STATIC_LABELS = global_config.global_config.get_config_value(
        'SKYLAB', 'respect_static_labels', type=bool, default=False)


def initialize():
    global _db
    _db = scheduler_lib.ConnectionManager().get_connection()

    notify_statuses_list = global_config.global_config.get_config_value(
            scheduler_config.CONFIG_SECTION, "notify_email_statuses",
            default='')
    global _notify_email_statuses
    _notify_email_statuses = [status for status in
                              re.split(r'[\s,;:]', notify_statuses_list.lower())
                              if status]

    # AUTOTEST_WEB.base_url is still a supported config option as some people
    # may wish to override the entire url.
    global _base_url
    config_base_url = global_config.global_config.get_config_value(
            scheduler_config.CONFIG_SECTION, 'base_url', default='')
    if config_base_url:
        _base_url = config_base_url
    else:
        _base_url = afe_urls.ROOT_URL

    initialize_globals()


def initialize_globals():
    global _drone_manager
    _drone_manager = drone_manager.instance()


def get_job_metadata(job):
    """Get a dictionary of the job information.

    The return value is a dictionary that includes job information like id,
    name and parent job information. The value will be stored in metadata
    database.

    @param job: A Job object.
    @return: A dictionary containing the job id, owner and name.
    """
    if not job:
        logging.error('Job is None, no metadata returned.')
        return {}
    try:
        return {'job_id': job.id,
                'owner': job.owner,
                'job_name': job.name,
                'parent_job_id': job.parent_job_id}
    except AttributeError as e:
        logging.error('Job has missing attribute: %s', e)
        return {}


class DBError(Exception):
    """Raised by the DBObject constructor when its select fails."""


class DBObject(object):
    """A miniature object relational model for the database."""

    # Subclasses MUST override these:
    _table_name = ''
    _fields = ()

    # A mapping from (type, id) to the instance of the object for that
    # particular id.  This prevents us from creating new Job() and Host()
    # instances for every HostQueueEntry object that we instantiate as
    # multiple HQEs often share the same Job.
    _instances_by_type_and_id = weakref.WeakValueDictionary()
    _initialized = False


    def __new__(cls, id=None, **kwargs):
        """
        Look to see if we already have an instance for this particular type
        and id.  If so, use it instead of creating a duplicate instance.
        """
        if id is not None:
            instance = cls._instances_by_type_and_id.get((cls, id))
            if instance:
                return instance
        return super(DBObject, cls).__new__(cls, id=id, **kwargs)


    def __init__(self, id=None, row=None, new_record=False, always_query=True):
        assert bool(id) or bool(row)
        if id is not None and row is not None:
            assert id == row[0]
        assert self._table_name, '_table_name must be defined in your class'
        assert self._fields, '_fields must be defined in your class'
        if not new_record:
            if self._initialized and not always_query:
                return  # We've already been initialized.
            if id is None:
                id = row[0]
            # Tell future constructors to use us instead of re-querying while
            # this instance is still around.
            self._instances_by_type_and_id[(type(self), id)] = self

        self.__table = self._table_name

        self.__new_record = new_record

        if row is None:
            row = self._fetch_row_from_db(id)

        if self._initialized:
            differences = self._compare_fields_in_row(row)
            if differences:
                logging.warning(
                    'initialized %s %s instance requery is updating: %s',
                    type(self), self.id, differences)
        self._update_fields_from_row(row)
        self._initialized = True


    @classmethod
    def _clear_instance_cache(cls):
        """Used for testing, clear the internal instance cache."""
        cls._instances_by_type_and_id.clear()


    def _fetch_row_from_db(self, row_id):
        fields = ', '.join(self._fields)
        sql = 'SELECT %s FROM %s WHERE ID=%%s' % (fields, self.__table)
        rows = _db.execute(sql, (row_id,))
        if not rows:
            raise DBError("row not found (table=%s, row id=%s)"
                          % (self.__table, row_id))
        return rows[0]


    def _assert_row_length(self, row):
        assert len(row) == len(self._fields), (
            "table = %s, row = %s/%d, fields = %s/%d" % (
            self.__table, row, len(row), self._fields, len(self._fields)))


    def _compare_fields_in_row(self, row):
        """
        Given a row as returned by a SELECT query, compare it to our existing in
        memory fields.  Fractional seconds are stripped from datetime values
        before comparison.

        @param row - A sequence of values corresponding to fields named in
                The class attribute _fields.

        @returns A dictionary listing the differences keyed by field name
                containing tuples of (current_value, row_value).
        """
        self._assert_row_length(row)
        differences = {}
        for field, row_value in itertools.izip(self._fields, row):
            current_value = getattr(self, field)
            if (isinstance(current_value, datetime.datetime)
                and isinstance(row_value, datetime.datetime)):
                current_value = current_value.strftime(time_utils.TIME_FMT)
                row_value = row_value.strftime(time_utils.TIME_FMT)
            if current_value != row_value:
                differences[field] = (current_value, row_value)
        return differences


    def _update_fields_from_row(self, row):
        """
        Update our field attributes using a single row returned by SELECT.

        @param row - A sequence of values corresponding to fields named in
                the class fields list.
        """
        self._assert_row_length(row)

        self._valid_fields = set()
        for field, value in itertools.izip(self._fields, row):
            setattr(self, field, value)
            self._valid_fields.add(field)

        self._valid_fields.remove('id')


    def update_from_database(self):
        assert self.id is not None
        row = self._fetch_row_from_db(self.id)
        self._update_fields_from_row(row)


    def count(self, where, table = None):
        if not table:
            table = self.__table

        rows = _db.execute("""
                SELECT count(*) FROM %s
                WHERE %s
        """ % (table, where))

        assert len(rows) == 1

        return int(rows[0][0])


    def update_field(self, field, value):
        assert field in self._valid_fields

        if getattr(self, field) == value:
            return

        query = "UPDATE %s SET %s = %%s WHERE id = %%s" % (self.__table, field)
        _db.execute(query, (value, self.id))

        setattr(self, field, value)


    def save(self):
        if self.__new_record:
            keys = self._fields[1:] # avoid id
            columns = ','.join([str(key) for key in keys])
            values = []
            for key in keys:
                value = getattr(self, key)
                if value is None:
                    values.append('NULL')
                else:
                    values.append('"%s"' % value)
            values_str = ','.join(values)
            query = ('INSERT INTO %s (%s) VALUES (%s)' %
                     (self.__table, columns, values_str))
            _db.execute(query)
            # Update our id to the one the database just assigned to us.
            self.id = _db.execute('SELECT LAST_INSERT_ID()')[0][0]


    def delete(self):
        self._instances_by_type_and_id.pop((type(self), id), None)
        self._initialized = False
        self._valid_fields.clear()
        query = 'DELETE FROM %s WHERE id=%%s' % self.__table
        _db.execute(query, (self.id,))


    @staticmethod
    def _prefix_with(string, prefix):
        if string:
            string = prefix + string
        return string


    @classmethod
    def fetch_rows(cls, where='', params=(), joins='', order_by=''):
        """
        Fetch the rows based on the given database query.

        @yields the rows fetched by the given query.
        """
        order_by = cls._prefix_with(order_by, 'ORDER BY ')
        where = cls._prefix_with(where, 'WHERE ')
        fields = []
        for field in cls._fields:
            fields.append('%s.%s' % (cls._table_name, field))

        query = ('SELECT %(fields)s FROM %(table)s %(joins)s '
                 '%(where)s %(order_by)s' % {'fields' : ', '.join(fields),
                                             'table' : cls._table_name,
                                             'joins' : joins,
                                             'where' : where,
                                             'order_by' : order_by})
        rows = _db.execute(query, params)
        return rows

    @classmethod
    def fetch(cls, where='', params=(), joins='', order_by=''):
        """
        Construct instances of our class based on the given database query.

        @yields One class instance for each row fetched.
        """
        rows = cls.fetch_rows(where=where, params=params, joins=joins,
                              order_by=order_by)
        return [cls(id=row[0], row=row) for row in rows]


class IneligibleHostQueue(DBObject):
    _table_name = 'afe_ineligible_host_queues'
    _fields = ('id', 'job_id', 'host_id')


class AtomicGroup(DBObject):
    _table_name = 'afe_atomic_groups'
    _fields = ('id', 'name', 'description', 'max_number_of_machines',
               'invalid')


class Label(DBObject):
    _table_name = 'afe_labels'
    _fields = ('id', 'name', 'kernel_config', 'platform', 'invalid',
               'only_if_needed', 'atomic_group_id')


    def __repr__(self):
        return 'Label(name=%r, id=%d, atomic_group_id=%r)' % (
                self.name, self.id, self.atomic_group_id)


class Host(DBObject):
    _table_name = 'afe_hosts'
    # TODO(ayatane): synch_id is not used, remove after fixing DB.
    _fields = ('id', 'hostname', 'locked', 'synch_id', 'status',
               'invalid', 'protection', 'locked_by_id', 'lock_time', 'dirty',
               'leased', 'shard_id', 'lock_reason')


    def set_status(self,status):
        logging.info('%s -> %s', self.hostname, status)
        self.update_field('status',status)


    def _get_labels_with_platform(self, non_static_rows, static_rows):
        """Helper function to fetch labels & platform for a host."""
        if not RESPECT_STATIC_LABELS:
            return non_static_rows

        combined_rows = []
        replaced_labels = _db.execute(
                'SELECT label_id FROM afe_replaced_labels')
        replaced_label_ids = {l[0] for l in replaced_labels}

        # We respect afe_labels more, which means:
        #   * if non-static labels are replaced, we find its replaced static
        #   labels from afe_static_labels by label name.
        #   * if non-static labels are not replaced, we keep it.
        #   * Drop static labels which don't have reference non-static labels.
        static_label_names = []
        for label_id, label_name, is_platform in non_static_rows:
            if label_id not in replaced_label_ids:
                combined_rows.append((label_id, label_name, is_platform))
            else:
                static_label_names.append(label_name)

        # Only keep static labels who have replaced non-static labels.
        for label_id, label_name, is_platform in static_rows:
            if label_name in static_label_names:
                combined_rows.append((label_id, label_name, is_platform))

        return combined_rows


    def platform_and_labels(self):
        """
        Returns a tuple (platform_name, list_of_all_label_names).
        """
        template = ('SELECT %(label_table)s.id, %(label_table)s.name, '
                    '%(label_table)s.platform FROM %(label_table)s INNER '
                    'JOIN %(host_label_table)s '
                    'ON %(label_table)s.id = %(host_label_table)s.%(column)s '
                    'WHERE %(host_label_table)s.host_id = %(host_id)s '
                    'ORDER BY %(label_table)s.name')
        static_query = template % {
                'host_label_table': 'afe_static_hosts_labels',
                'label_table': 'afe_static_labels',
                'column': 'staticlabel_id',
                'host_id': self.id
        }
        non_static_query = template % {
                'host_label_table': 'afe_hosts_labels',
                'label_table': 'afe_labels',
                'column': 'label_id',
                'host_id': self.id
        }
        non_static_rows = _db.execute(non_static_query)
        static_rows = _db.execute(static_query)

        rows = self._get_labels_with_platform(non_static_rows, static_rows)
        platform = None
        all_labels = []
        for _, label_name, is_platform in rows:
            if is_platform:
                platform = label_name
            all_labels.append(label_name)
        return platform, all_labels


    _ALPHANUM_HOST_RE = re.compile(r'^([a-z-]+)(\d+)$', re.IGNORECASE)


    @classmethod
    def cmp_for_sort(cls, a, b):
        """
        A comparison function for sorting Host objects by hostname.

        This strips any trailing numeric digits, ignores leading 0s and
        compares hostnames by the leading name and the trailing digits as a
        number.  If both hostnames do not match this pattern, they are simply
        compared as lower case strings.

        Example of how hostnames will be sorted:

          alice, host1, host2, host09, host010, host10, host11, yolkfolk

        This hopefully satisfy most people's hostname sorting needs regardless
        of their exact naming schemes.  Nobody sane should have both a host10
        and host010 (but the algorithm works regardless).
        """
        lower_a = a.hostname.lower()
        lower_b = b.hostname.lower()
        match_a = cls._ALPHANUM_HOST_RE.match(lower_a)
        match_b = cls._ALPHANUM_HOST_RE.match(lower_b)
        if match_a and match_b:
            name_a, number_a_str = match_a.groups()
            name_b, number_b_str = match_b.groups()
            number_a = int(number_a_str.lstrip('0'))
            number_b = int(number_b_str.lstrip('0'))
            result = cmp((name_a, number_a), (name_b, number_b))
            if result == 0 and lower_a != lower_b:
                # If they compared equal above but the lower case names are
                # indeed different, don't report equality.  abc012 != abc12.
                return cmp(lower_a, lower_b)
            return result
        else:
            return cmp(lower_a, lower_b)


class HostQueueEntry(DBObject):
    _table_name = 'afe_host_queue_entries'
    _fields = ('id', 'job_id', 'host_id', 'status', 'meta_host',
               'active', 'complete', 'deleted', 'execution_subdir',
               'atomic_group_id', 'aborted', 'started_on', 'finished_on')

    _COMPLETION_COUNT_METRIC = metrics.Counter(
        'chromeos/autotest/scheduler/hqe_completion_count')

    def __init__(self, id=None, row=None, job_row=None, **kwargs):
        """
        @param id: ID field from afe_host_queue_entries table.
                   Either id or row should be specified for initialization.
        @param row: The DB row for a particular HostQueueEntry.
                    Either id or row should be specified for initialization.
        @param job_row: The DB row for the job of this HostQueueEntry.
        """
        assert id or row
        super(HostQueueEntry, self).__init__(id=id, row=row, **kwargs)
        self.job = Job(self.job_id, row=job_row)

        if self.host_id:
            self.host = rdb_lib.get_hosts([self.host_id])[0]
            self.host.dbg_str = self.get_dbg_str()
            self.host.metadata = get_job_metadata(self.job)
        else:
            self.host = None


    @classmethod
    def clone(cls, template):
        """
        Creates a new row using the values from a template instance.

        The new instance will not exist in the database or have a valid
        id attribute until its save() method is called.
        """
        assert isinstance(template, cls)
        new_row = [getattr(template, field) for field in cls._fields]
        clone = cls(row=new_row, new_record=True)
        clone.id = None
        return clone


    @classmethod
    def fetch(cls, where='', params=(), joins='', order_by=''):
        """
        Construct instances of our class based on the given database query.

        @yields One class instance for each row fetched.
        """
        # Override the original fetch method to pre-fetch the jobs from the DB
        # in order to prevent each HQE making separate DB queries.
        rows = cls.fetch_rows(where=where, params=params, joins=joins,
                              order_by=order_by)
        if len(rows) <= 1:
            return [cls(id=row[0], row=row) for row in rows]

        job_params = ', '.join([str(row[1]) for row in rows])
        job_rows = Job.fetch_rows(where='id IN (%s)' % (job_params))
        # Create a Job_id to Job_row match dictionary to match the HQE
        # to its corresponding job.
        job_dict = {job_row[0]: job_row for job_row in job_rows}
        return [cls(id=row[0], row=row, job_row=job_dict.get(row[1]))
                for row in rows]


    def _view_job_url(self):
        return "%s#tab_id=view_job&object_id=%s" % (_base_url, self.job.id)


    def get_labels(self):
        """
        Get all labels associated with this host queue entry (either via the
        meta_host or as a job dependency label).  The labels yielded are not
        guaranteed to be unique.

        @yields Label instances associated with this host_queue_entry.
        """
        if self.meta_host:
            yield Label(id=self.meta_host, always_query=False)
        labels = Label.fetch(
                joins="JOIN afe_jobs_dependency_labels AS deps "
                      "ON (afe_labels.id = deps.label_id)",
                where="deps.job_id = %d" % self.job.id)
        for label in labels:
            yield label


    def set_host(self, host):
        if host:
            logging.info('Assigning host %s to entry %s', host.hostname, self)
            self.update_field('host_id', host.id)
            self.block_host(host.id)
        else:
            logging.info('Releasing host from %s', self)
            self.unblock_host(self.host.id)
            self.update_field('host_id', None)

        self.host = host


    def block_host(self, host_id):
        logging.info("creating block %s/%s", self.job.id, host_id)
        row = [0, self.job.id, host_id]
        block = IneligibleHostQueue(row=row, new_record=True)
        block.save()


    def unblock_host(self, host_id):
        logging.info("removing block %s/%s", self.job.id, host_id)
        blocks = IneligibleHostQueue.fetch(
            'job_id=%d and host_id=%d' % (self.job.id, host_id))
        for block in blocks:
            block.delete()


    def set_execution_subdir(self, subdir=None):
        if subdir is None:
            assert self.host
            subdir = self.host.hostname
        self.update_field('execution_subdir', subdir)


    def _get_hostname(self):
        if self.host:
            return self.host.hostname
        return 'no host'


    def get_dbg_str(self):
        """Get a debug string to identify this host.

        @return: A string containing the hqe and job id.
        """
        try:
            return 'HQE: %s, for job: %s' % (self.id, self.job_id)
        except AttributeError as e:
            return 'HQE has not been initialized yet: %s' % e


    def __str__(self):
        flags = []
        if self.active:
            flags.append('active')
        if self.complete:
            flags.append('complete')
        if self.deleted:
            flags.append('deleted')
        if self.aborted:
            flags.append('aborted')
        flags_str = ','.join(flags)
        if flags_str:
            flags_str = ' [%s]' % flags_str
        return ("%s and host: %s has status:%s%s" %
                (self.get_dbg_str(), self._get_hostname(), self.status,
                 flags_str))


    def set_status(self, status):
        logging.info("%s -> %s", self, status)

        self.update_field('status', status)

        active = (status in models.HostQueueEntry.ACTIVE_STATUSES)
        complete = (status in models.HostQueueEntry.COMPLETE_STATUSES)

        self.update_field('active', active)

        # The ordering of these operations is important. Once we set the
        # complete bit this job will become indistinguishable from all
        # the other complete jobs, unless we first set shard_id to NULL
        # to signal to the shard_client that we need to upload it. However,
        # we can only set both these after we've updated finished_on etc
        # within _on_complete or the job will get synced in an intermediate
        # state. This means that if someone sigkills the scheduler between
        # setting finished_on and complete, we will have inconsistent jobs.
        # This should be fine, because nothing critical checks finished_on,
        # and the scheduler should never be killed mid-tick.
        if complete:
            self._on_complete(status)
            self._email_on_job_complete()

        self.update_field('complete', complete)

        should_email_status = (status.lower() in _notify_email_statuses or
                               'all' in _notify_email_statuses)
        if should_email_status:
            self._email_on_status(status)
        logging.debug('HQE Set Status Complete')


    def _on_complete(self, status):
        metric_fields = {'status': status.lower()}
        if self.host:
            metric_fields['board'] = self.host.board or ''
            if len(self.host.pools) == 1:
                metric_fields['pool'] = self.host.pools[0]
            else:
                metric_fields['pool'] = 'MULTIPLE'
        else:
            metric_fields['board'] = 'NO_HOST'
            metric_fields['pool'] = 'NO_HOST'
        self._COMPLETION_COUNT_METRIC.increment(fields=metric_fields)
        if status is not models.HostQueueEntry.Status.ABORTED:
            self.job.stop_if_necessary()
        if self.started_on:
            self.set_finished_on_now()
            self._log_trace()
        if self.job.shard_id is not None:
            # If shard_id is None, the job will be synced back to the master
            self.job.update_field('shard_id', None)
        if not self.execution_subdir:
            return
        # unregister any possible pidfiles associated with this queue entry
        for pidfile_name in drone_manager.ALL_PIDFILE_NAMES:
            pidfile_id = _drone_manager.get_pidfile_id_from(
                    self.execution_path(), pidfile_name=pidfile_name)
            _drone_manager.unregister_pidfile(pidfile_id)

    def _log_trace(self):
        """Emits a Cloud Trace span for the HQE's duration."""
        if self.started_on and self.finished_on:
            span = cloud_trace.Span('HQE', spanId='0',
                                    traceId=hqe_trace_id(self.id))
            # TODO(phobbs) make a .SetStart() and .SetEnd() helper method
            span.startTime = types.Timestamp()
            span.startTime.FromDatetime(self.started_on)
            span.endTime = types.Timestamp()
            span.endTime.FromDatetime(self.finished_on)
            # TODO(phobbs) any LogSpan calls need to be wrapped in this for
            # safety during tests, so this should be caught within LogSpan.
            try:
                cloud_trace.LogSpan(span)
            except IOError as e:
                if e.errno == errno.ENOENT:
                    logging.warning('Error writing to cloud trace results '
                                    'directory: %s', e)


    def _get_status_email_contents(self, status, summary=None, hostname=None):
        """
        Gather info for the status notification e-mails.

        If needed, we could start using the Django templating engine to create
        the subject and the e-mail body, but that doesn't seem necessary right
        now.

        @param status: Job status text. Mandatory.
        @param summary: Job summary text. Optional.
        @param hostname: A hostname for the job. Optional.

        @return: Tuple (subject, body) for the notification e-mail.
        """
        job_stats = Job(id=self.job.id).get_execution_details()

        subject = ('Autotest | Job ID: %s "%s" | Status: %s ' %
                   (self.job.id, self.job.name, status))

        if hostname is not None:
            subject += '| Hostname: %s ' % hostname

        if status not in ["1 Failed", "Failed"]:
            subject += '| Success Rate: %.2f %%' % job_stats['success_rate']

        body =  "Job ID: %s\n" % self.job.id
        body += "Job name: %s\n" % self.job.name
        if hostname is not None:
            body += "Host: %s\n" % hostname
        if summary is not None:
            body += "Summary: %s\n" % summary
        body += "Status: %s\n" % status
        body += "Results interface URL: %s\n" % self._view_job_url()
        body += "Execution time (HH:MM:SS): %s\n" % job_stats['execution_time']
        if int(job_stats['total_executed']) > 0:
            body += "User tests executed: %s\n" % job_stats['total_executed']
            body += "User tests passed: %s\n" % job_stats['total_passed']
            body += "User tests failed: %s\n" % job_stats['total_failed']
            body += ("User tests success rate: %.2f %%\n" %
                     job_stats['success_rate'])

        if job_stats['failed_rows']:
            body += "Failures:\n"
            body += job_stats['failed_rows']

        return subject, body


    def _email_on_status(self, status):
        hostname = self._get_hostname()
        subject, body = self._get_status_email_contents(status, None, hostname)
        email_manager.manager.send_email(self.job.email_list, subject, body)


    def _email_on_job_complete(self):
        if not self.job.is_finished():
            return

        summary = []
        hosts_queue = HostQueueEntry.fetch('job_id = %s' % self.job.id)
        for queue_entry in hosts_queue:
            summary.append("Host: %s Status: %s" %
                                (queue_entry._get_hostname(),
                                 queue_entry.status))

        summary = "\n".join(summary)
        status_counts = models.Job.objects.get_status_counts(
                [self.job.id])[self.job.id]
        status = ', '.join('%d %s' % (count, status) for status, count
                    in status_counts.iteritems())

        subject, body = self._get_status_email_contents(status, summary, None)
        email_manager.manager.send_email(self.job.email_list, subject, body)


    def schedule_pre_job_tasks(self):
        logging.info("%s/%s/%s (job %s, entry %s) scheduled on %s, status=%s",
                     self.job.name, self.meta_host, self.atomic_group_id,
                     self.job.id, self.id, self.host.hostname, self.status)

        self._do_schedule_pre_job_tasks()


    def _do_schedule_pre_job_tasks(self):
        self.job.schedule_pre_job_tasks(queue_entry=self)


    def requeue(self):
        assert self.host
        self.set_status(models.HostQueueEntry.Status.QUEUED)
        self.update_field('started_on', None)
        self.update_field('finished_on', None)
        # verify/cleanup failure sets the execution subdir, so reset it here
        self.set_execution_subdir('')
        if self.meta_host:
            self.set_host(None)


    @property
    def aborted_by(self):
        self._load_abort_info()
        return self._aborted_by


    @property
    def aborted_on(self):
        self._load_abort_info()
        return self._aborted_on


    def _load_abort_info(self):
        """ Fetch info about who aborted the job. """
        if hasattr(self, "_aborted_by"):
            return
        rows = _db.execute("""
                SELECT afe_users.login,
                        afe_aborted_host_queue_entries.aborted_on
                FROM afe_aborted_host_queue_entries
                INNER JOIN afe_users
                ON afe_users.id = afe_aborted_host_queue_entries.aborted_by_id
                WHERE afe_aborted_host_queue_entries.queue_entry_id = %s
                """, (self.id,))
        if rows:
            self._aborted_by, self._aborted_on = rows[0]
        else:
            self._aborted_by = self._aborted_on = None


    def on_pending(self):
        """
        Called when an entry in a synchronous job has passed verify.  If the
        job is ready to run, sets the entries to STARTING. Otherwise, it leaves
        them in PENDING.
        """
        self.set_status(models.HostQueueEntry.Status.PENDING)
        if not self.host:
            raise scheduler_lib.NoHostIdError(
                    'Failed to recover a job whose host_queue_entry_id=%r due'
                    ' to no host_id.'
                    % self.id)
        self.host.set_status(models.Host.Status.PENDING)

        # Some debug code here: sends an email if an asynchronous job does not
        # immediately enter Starting.
        # TODO: Remove this once we figure out why asynchronous jobs are getting
        # stuck in Pending.
        self.job.run_if_ready(queue_entry=self)
        if (self.job.synch_count == 1 and
                self.status == models.HostQueueEntry.Status.PENDING):
            subject = 'Job %s (id %s)' % (self.job.name, self.job.id)
            message = 'Asynchronous job stuck in Pending'
            email_manager.manager.enqueue_notify_email(subject, message)


    def abort(self, dispatcher):
        assert self.aborted and not self.complete

        Status = models.HostQueueEntry.Status
        if self.status in {Status.GATHERING, Status.PARSING}:
            # do nothing; post-job tasks will finish and then mark this entry
            # with status "Aborted" and take care of the host
            return

        if self.status in {Status.STARTING, Status.PENDING, Status.RUNNING}:
            # If hqe is in any of these status, it should not have any
            # unfinished agent before it can be aborted.
            agents = dispatcher.get_agents_for_entry(self)
            # Agent with finished task can be left behind. This is added to
            # handle the special case of aborting hostless job in STARTING
            # status, in which the agent has only a HostlessQueueTask
            # associated. The finished HostlessQueueTask will be cleaned up in
            # the next tick, so it's safe to leave the agent there. Without
            # filtering out finished agent, HQE abort won't be able to proceed.
            assert all([agent.is_done() for agent in agents])
            # If hqe is still in STARTING status, it may not have assigned a
            # host yet.
            if self.host:
                self.host.set_status(models.Host.Status.READY)
        elif (self.status == Status.VERIFYING or
              self.status == Status.RESETTING):
            models.SpecialTask.objects.create(
                    task=models.SpecialTask.Task.CLEANUP,
                    host=models.Host.objects.get(id=self.host.id),
                    requested_by=self.job.owner_model())
        elif self.status == Status.PROVISIONING:
            models.SpecialTask.objects.create(
                    task=models.SpecialTask.Task.REPAIR,
                    host=models.Host.objects.get(id=self.host.id),
                    requested_by=self.job.owner_model())

        self.set_status(Status.ABORTED)


    def execution_tag(self):
        SQL_SUSPECT_ENTRIES = ('SELECT * FROM afe_host_queue_entries WHERE '
                               'complete!=1 AND execution_subdir="" AND '
                               'status!="Queued";')
        SQL_FIX_SUSPECT_ENTRY = ('UPDATE afe_host_queue_entries SET '
                                 'status="Aborted" WHERE id=%s;')
        try:
            assert self.execution_subdir
        except AssertionError:
            # TODO(scottz): Remove temporary fix/info gathering pathway for
            # crosbug.com/31595 once issue is root caused.
            logging.error('No execution_subdir for host queue id:%s.', self.id)
            logging.error('====DB DEBUG====\n%s', SQL_SUSPECT_ENTRIES)
            for row in _db.execute(SQL_SUSPECT_ENTRIES):
                logging.error(row)
            logging.error('====DB DEBUG====\n')
            fix_query = SQL_FIX_SUSPECT_ENTRY % self.id
            logging.error('EXECUTING: %s', fix_query)
            _db.execute(SQL_FIX_SUSPECT_ENTRY % self.id)
            raise AssertionError(('self.execution_subdir not found. '
                                  'See log for details.'))

        return "%s/%s" % (self.job.tag(), self.execution_subdir)


    def execution_path(self):
        return self.execution_tag()


    def set_started_on_now(self):
        self.update_field('started_on', datetime.datetime.now())


    def set_finished_on_now(self):
        self.update_field('finished_on', datetime.datetime.now())


    def is_hostless(self):
        return (self.host_id is None
                and self.meta_host is None)


def hqe_trace_id(hqe_id):
    """Constructs the canonical trace id based on the HQE's id.

    Encodes 'HQE' in base16 and concatenates with the hex representation
    of the HQE's id.

    @param hqe_id: The HostQueueEntry's id.

    Returns:
        A trace id (in hex format)
    """
    return base64.b16encode('HQE') + hex(hqe_id)[2:]


class Job(DBObject):
    _table_name = 'afe_jobs'
    _fields = ('id', 'owner', 'name', 'priority', 'control_file',
               'control_type', 'created_on', 'synch_count', 'timeout',
               'run_verify', 'email_list', 'reboot_before', 'reboot_after',
               'parse_failed_repair', 'max_runtime_hrs', 'drone_set_id',
               'parameterized_job_id', 'max_runtime_mins', 'parent_job_id',
               'test_retry', 'run_reset', 'timeout_mins', 'shard_id',
               'require_ssp')

    # TODO(gps): On scheduler start/recovery we need to call HQE.on_pending() on
    # all status='Pending' atomic group HQEs incase a delay was running when the
    # scheduler was restarted and no more hosts ever successfully exit Verify.

    def __init__(self, id=None, row=None, **kwargs):
        assert id or row
        super(Job, self).__init__(id=id, row=row, **kwargs)
        self._owner_model = None # caches model instance of owner
        self.update_image_path = None # path of OS image to install


    def model(self):
        return models.Job.objects.get(id=self.id)


    def owner_model(self):
        # work around the fact that the Job owner field is a string, not a
        # foreign key
        if not self._owner_model:
            self._owner_model = models.User.objects.get(login=self.owner)
        return self._owner_model


    def tag(self):
        return "%s-%s" % (self.id, self.owner)


    def get_execution_details(self):
        """
        Get test execution details for this job.

        @return: Dictionary with test execution details
        """
        def _find_test_jobs(rows):
            """
            Here we are looking for tests such as SERVER_JOB and CLIENT_JOB.*
            Those are autotest 'internal job' tests, so they should not be
            counted when evaluating the test stats.

            @param rows: List of rows (matrix) with database results.
            """
            job_test_pattern = re.compile('SERVER|CLIENT\\_JOB\.[\d]')
            n_test_jobs = 0
            for r in rows:
                test_name = r[0]
                if job_test_pattern.match(test_name):
                    n_test_jobs += 1

            return n_test_jobs

        stats = {}

        rows = _db.execute("""
                SELECT t.test, s.word, t.reason
                FROM tko_tests AS t, tko_jobs AS j, tko_status AS s
                WHERE t.job_idx = j.job_idx
                AND s.status_idx = t.status
                AND j.afe_job_id = %s
                ORDER BY t.reason
                """ % self.id)

        failed_rows = [r for r in rows if not r[1] == 'GOOD']

        n_test_jobs = _find_test_jobs(rows)
        n_test_jobs_failed = _find_test_jobs(failed_rows)

        total_executed = len(rows) - n_test_jobs
        total_failed = len(failed_rows) - n_test_jobs_failed

        if total_executed > 0:
            success_rate = 100 - ((total_failed / float(total_executed)) * 100)
        else:
            success_rate = 0

        stats['total_executed'] = total_executed
        stats['total_failed'] = total_failed
        stats['total_passed'] = total_executed - total_failed
        stats['success_rate'] = success_rate

        status_header = ("Test Name", "Status", "Reason")
        if failed_rows:
            stats['failed_rows'] = utils.matrix_to_string(failed_rows,
                                                          status_header)
        else:
            stats['failed_rows'] = ''

        time_row = _db.execute("""
                   SELECT started_time, finished_time
                   FROM tko_jobs
                   WHERE afe_job_id = %s
                   """ % self.id)

        if time_row:
            t_begin, t_end = time_row[0]
            try:
                delta = t_end - t_begin
                minutes, seconds = divmod(delta.seconds, 60)
                hours, minutes = divmod(minutes, 60)
                stats['execution_time'] = ("%02d:%02d:%02d" %
                                           (hours, minutes, seconds))
            # One of t_end or t_begin are None
            except TypeError:
                stats['execution_time'] = '(could not determine)'
        else:
            stats['execution_time'] = '(none)'

        return stats


    def keyval_dict(self):
        return self.model().keyval_dict()


    def _pending_count(self):
        """The number of HostQueueEntries for this job in the Pending state."""
        pending_entries = models.HostQueueEntry.objects.filter(
                job=self.id, status=models.HostQueueEntry.Status.PENDING)
        return pending_entries.count()


    def is_ready(self):
        pending_count = self._pending_count()
        ready = (pending_count >= self.synch_count)

        if not ready:
            logging.info(
                    'Job %s not ready: %s pending, %s required ',
                    self, pending_count, self.synch_count)

        return ready


    def num_machines(self, clause = None):
        sql = "job_id=%s" % self.id
        if clause:
            sql += " AND (%s)" % clause
        return self.count(sql, table='afe_host_queue_entries')


    def num_queued(self):
        return self.num_machines('not complete')


    def num_active(self):
        return self.num_machines('active')


    def num_complete(self):
        return self.num_machines('complete')


    def is_finished(self):
        return self.num_complete() == self.num_machines()


    def _not_yet_run_entries(self, include_active=True):
        if include_active:
          statuses = list(models.HostQueueEntry.PRE_JOB_STATUSES)
        else:
          statuses = list(models.HostQueueEntry.IDLE_PRE_JOB_STATUSES)
        return models.HostQueueEntry.objects.filter(job=self.id,
                                                    status__in=statuses)


    def _stop_all_entries(self):
        """Stops the job's inactive pre-job HQEs."""
        entries_to_stop = self._not_yet_run_entries(
            include_active=False)
        for child_entry in entries_to_stop:
            assert not child_entry.complete, (
                '%s status=%s, active=%s, complete=%s' %
                (child_entry.id, child_entry.status, child_entry.active,
                 child_entry.complete))
            if child_entry.status == models.HostQueueEntry.Status.PENDING:
                child_entry.host.status = models.Host.Status.READY
                child_entry.host.save()
            child_entry.status = models.HostQueueEntry.Status.STOPPED
            child_entry.save()


    def stop_if_necessary(self):
        not_yet_run = self._not_yet_run_entries()
        if not_yet_run.count() < self.synch_count:
            self._stop_all_entries()


    def _next_group_name(self):
        """@returns a directory name to use for the next host group results."""
        group_name = ''
        group_count_re = re.compile(r'%sgroup(\d+)' % re.escape(group_name))
        query = models.HostQueueEntry.objects.filter(
            job=self.id).values('execution_subdir').distinct()
        subdirs = (entry['execution_subdir'] for entry in query)
        group_matches = (group_count_re.match(subdir) for subdir in subdirs)
        ids = [int(match.group(1)) for match in group_matches if match]
        if ids:
            next_id = max(ids) + 1
        else:
            next_id = 0
        return '%sgroup%d' % (group_name, next_id)


    def get_group_entries(self, queue_entry_from_group):
        """
        @param queue_entry_from_group: A HostQueueEntry instance to find other
                group entries on this job for.

        @returns A list of HostQueueEntry objects all executing this job as
                part of the same group as the one supplied (having the same
                execution_subdir).
        """
        execution_subdir = queue_entry_from_group.execution_subdir
        return list(HostQueueEntry.fetch(
            where='job_id=%s AND execution_subdir=%s',
            params=(self.id, execution_subdir)))


    def _should_run_cleanup(self, queue_entry):
        if self.reboot_before == model_attributes.RebootBefore.ALWAYS:
            return True
        elif self.reboot_before == model_attributes.RebootBefore.IF_DIRTY:
            return queue_entry.host.dirty
        return False


    def _should_run_verify(self, queue_entry):
        do_not_verify = (queue_entry.host.protection ==
                         host_protections.Protection.DO_NOT_VERIFY)
        if do_not_verify:
            return False
        # If RebootBefore is set to NEVER, then we won't run reset because
        # we can't cleanup, so we need to weaken a Reset into a Verify.
        weaker_reset = (self.run_reset and
                self.reboot_before == model_attributes.RebootBefore.NEVER)
        return self.run_verify or weaker_reset


    def _should_run_reset(self, queue_entry):
        can_verify = (queue_entry.host.protection !=
                         host_protections.Protection.DO_NOT_VERIFY)
        can_reboot = self.reboot_before != model_attributes.RebootBefore.NEVER
        return (can_reboot and can_verify and (self.run_reset or
                (self._should_run_cleanup(queue_entry) and
                 self._should_run_verify(queue_entry))))


    def _should_run_provision(self, queue_entry):
        """
        Determine if the queue_entry needs to have a provision task run before
        it to provision queue_entry.host.

        @param queue_entry: The host queue entry in question.
        @returns: True if we should schedule a provision task, False otherwise.

        """
        # If we get to this point, it means that the scheduler has already
        # vetted that all the unprovisionable labels match, so we can just
        # find all labels on the job that aren't on the host to get the list
        # of what we need to provision.  (See the scheduling logic in
        # host_scheduler.py:is_host_eligable_for_job() where we discard all
        # actionable labels when assigning jobs to hosts.)
        job_labels = {x.name for x in queue_entry.get_labels()}
        # Skip provision if `skip_provision` is listed in the job labels.
        if provision.SKIP_PROVISION in job_labels:
            return False
        _, host_labels = queue_entry.host.platform_and_labels()
        # If there are any labels on the job that are not on the host and they
        # are labels that provisioning knows how to change, then that means
        # there is provisioning work to do.  If there's no provisioning work to
        # do, then obviously we have no reason to schedule a provision task!
        diff = job_labels - set(host_labels)
        if any([provision.Provision.acts_on(x) for x in diff]):
            return True
        return False


    def _queue_special_task(self, queue_entry, task):
        """
        Create a special task and associate it with a host queue entry.

        @param queue_entry: The queue entry this special task should be
                            associated with.
        @param task: One of the members of the enum models.SpecialTask.Task.
        @returns: None

        """
        models.SpecialTask.objects.create(
                host=models.Host.objects.get(id=queue_entry.host_id),
                queue_entry=queue_entry, task=task)


    def schedule_pre_job_tasks(self, queue_entry):
        """
        Queue all of the special tasks that need to be run before a host
        queue entry may run.

        If no special taskes need to be scheduled, then |on_pending| will be
        called directly.

        @returns None

        """
        task_queued = False
        hqe_model = models.HostQueueEntry.objects.get(id=queue_entry.id)

        if self._should_run_provision(queue_entry):
            self._queue_special_task(hqe_model,
                                     models.SpecialTask.Task.PROVISION)
            task_queued = True
        elif self._should_run_reset(queue_entry):
            self._queue_special_task(hqe_model, models.SpecialTask.Task.RESET)
            task_queued = True
        else:
            if self._should_run_cleanup(queue_entry):
                self._queue_special_task(hqe_model,
                                         models.SpecialTask.Task.CLEANUP)
                task_queued = True
            if self._should_run_verify(queue_entry):
                self._queue_special_task(hqe_model,
                                         models.SpecialTask.Task.VERIFY)
                task_queued = True

        if not task_queued:
            queue_entry.on_pending()


    def _assign_new_group(self, queue_entries):
        if len(queue_entries) == 1:
            group_subdir_name = queue_entries[0].host.hostname
        else:
            group_subdir_name = self._next_group_name()
            logging.info('Running synchronous job %d hosts %s as %s',
                self.id, [entry.host.hostname for entry in queue_entries],
                group_subdir_name)

        for queue_entry in queue_entries:
            queue_entry.set_execution_subdir(group_subdir_name)


    def _choose_group_to_run(self, include_queue_entry):
        """
        @returns A tuple containing a list of HostQueueEntry instances to be
                used to run this Job, a string group name to suggest giving
                to this job in the results database.
        """
        chosen_entries = [include_queue_entry]
        num_entries_wanted = self.synch_count
        num_entries_wanted -= len(chosen_entries)

        if num_entries_wanted > 0:
            where_clause = 'job_id = %s AND status = "Pending" AND id != %s'
            pending_entries = list(HostQueueEntry.fetch(
                     where=where_clause,
                     params=(self.id, include_queue_entry.id)))

            # Sort the chosen hosts by hostname before slicing.
            def cmp_queue_entries_by_hostname(entry_a, entry_b):
                return Host.cmp_for_sort(entry_a.host, entry_b.host)
            pending_entries.sort(cmp=cmp_queue_entries_by_hostname)
            chosen_entries += pending_entries[:num_entries_wanted]

        # Sanity check.  We'll only ever be called if this can be met.
        if len(chosen_entries) < self.synch_count:
            message = ('job %s got less than %s chosen entries: %s' % (
                    self.id, self.synch_count, chosen_entries))
            logging.error(message)
            email_manager.manager.enqueue_notify_email(
                    'Job not started, too few chosen entries', message)
            return []

        self._assign_new_group(chosen_entries)
        return chosen_entries


    def run_if_ready(self, queue_entry):
        """
        Run this job by kicking its HQEs into status='Starting' if enough
        hosts are ready for it to run.

        Cleans up by kicking HQEs into status='Stopped' if this Job is not
        ready to run.
        """
        if not self.is_ready():
            self.stop_if_necessary()
        else:
            self.run(queue_entry)


    def request_abort(self):
        """Request that this Job be aborted on the next scheduler cycle."""
        self.model().abort()


    def run(self, queue_entry):
        """
        @param queue_entry: The HostQueueEntry instance calling this method.
        """
        queue_entries = self._choose_group_to_run(queue_entry)
        if queue_entries:
            self._finish_run(queue_entries)


    def _finish_run(self, queue_entries):
        for queue_entry in queue_entries:
            queue_entry.set_status(models.HostQueueEntry.Status.STARTING)


    def __str__(self):
        return '%s-%s' % (self.id, self.owner)
