"""Autotest AFE Cleanup used by the scheduler"""

import contextlib
import logging
import random
import time

from autotest_lib.client.common_lib import utils
from autotest_lib.frontend.afe import models
from autotest_lib.scheduler import scheduler_config
from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib import host_protections

try:
    from chromite.lib import metrics
except ImportError:
    metrics = utils.metrics_mock


_METRICS_PREFIX = 'chromeos/autotest/scheduler/cleanup'


class PeriodicCleanup(object):
    """Base class to schedule periodical cleanup work.
    """

    def __init__(self, db, clean_interval_minutes, run_at_initialize=False):
        self._db = db
        self.clean_interval_minutes = clean_interval_minutes
        self._last_clean_time = time.time()
        self._run_at_initialize = run_at_initialize


    def initialize(self):
        """Method called by scheduler at the startup.
        """
        if self._run_at_initialize:
            self._cleanup()


    def run_cleanup_maybe(self):
        """Test if cleanup method should be called.
        """
        should_cleanup = (self._last_clean_time +
                          self.clean_interval_minutes * 60
                          < time.time())
        if should_cleanup:
            self._cleanup()
            self._last_clean_time = time.time()


    def _cleanup(self):
        """Abrstract cleanup method."""
        raise NotImplementedError


class UserCleanup(PeriodicCleanup):
    """User cleanup that is controlled by the global config variable
       clean_interval_minutes in the SCHEDULER section.
    """

    def __init__(self, db, clean_interval_minutes):
        super(UserCleanup, self).__init__(db, clean_interval_minutes)
        self._last_reverify_time = time.time()


    @metrics.SecondsTimerDecorator(_METRICS_PREFIX + '/user/durations')
    def _cleanup(self):
        logging.info('Running periodic cleanup')
        self._abort_timed_out_jobs()
        self._abort_jobs_past_max_runtime()
        self._clear_inactive_blocks()
        self._check_for_db_inconsistencies()
        self._reverify_dead_hosts()
        self._django_session_cleanup()


    def _abort_timed_out_jobs(self):
        logging.info(
                'Aborting all jobs that have timed out and are not complete')
        query = models.Job.objects.filter(hostqueueentry__complete=False).extra(
            where=['created_on + INTERVAL timeout_mins MINUTE < NOW()'])
        jobs = query.distinct()
        if not jobs:
            return

        with _cleanup_warning_banner('timed out jobs', len(jobs)):
            for job in jobs:
                logging.warning('Aborting job %d due to job timeout', job.id)
                job.abort()
        _report_detected_errors('jobs_timed_out', len(jobs))


    def _abort_jobs_past_max_runtime(self):
        """
        Abort executions that have started and are past the job's max runtime.
        """
        logging.info('Aborting all jobs that have passed maximum runtime')
        rows = self._db.execute("""
            SELECT hqe.id FROM afe_host_queue_entries AS hqe
            WHERE NOT hqe.complete AND NOT hqe.aborted AND EXISTS
            (select * from afe_jobs where hqe.job_id=afe_jobs.id and
             hqe.started_on + INTERVAL afe_jobs.max_runtime_mins MINUTE < NOW())
            """)
        query = models.HostQueueEntry.objects.filter(
            id__in=[row[0] for row in rows])
        hqes = query.distinct()
        if not hqes:
            return

        with _cleanup_warning_banner('hqes past max runtime', len(hqes)):
            for queue_entry in hqes:
                logging.warning('Aborting entry %s due to max runtime',
                                queue_entry)
                queue_entry.abort()
        _report_detected_errors('hqes_past_max_runtime', len(hqes))


    def _check_for_db_inconsistencies(self):
        logging.info('Cleaning db inconsistencies')
        self._check_all_invalid_related_objects()


    def _check_invalid_related_objects_one_way(self, invalid_model,
                                               relation_field, valid_model):
        if 'invalid' not in invalid_model.get_field_dict():
            return

        invalid_objects = list(invalid_model.objects.filter(invalid=True))
        invalid_model.objects.populate_relationships(
                invalid_objects, valid_model, 'related_objects')
        if not invalid_objects:
            return

        num_objects_with_invalid_relations = 0
        errors = []
        for invalid_object in invalid_objects:
            if invalid_object.related_objects:
                related_objects = invalid_object.related_objects
                related_list = ', '.join(str(x) for x in related_objects)
                num_objects_with_invalid_relations += 1
                errors.append('Invalid %s is related to: %s' %
                              (invalid_object, related_list))
                related_manager = getattr(invalid_object, relation_field)
                related_manager.clear()

        # Only log warnings after we're sure we've seen at least one invalid
        # model with some valid relations to avoid empty banners from getting
        # printed.
        if errors:
            invalid_model_name = invalid_model.__name__
            valid_model_name = valid_model.__name__
            banner = 'invalid %s related to valid %s' % (invalid_model_name,
                                                         valid_model_name)
            with _cleanup_warning_banner(banner, len(errors)):
                for error in errors:
                    logging.warning(error)
            _report_detected_errors(
                    'invalid_related_objects',
                    num_objects_with_invalid_relations,
                    fields={'invalid_model': invalid_model_name,
                            'valid_model': valid_model_name})
            _report_detected_errors(
                    'invalid_related_objects_relations',
                    len(errors),
                    fields={'invalid_model': invalid_model_name,
                            'valid_model': valid_model_name})


    def _check_invalid_related_objects(self, first_model, first_field,
                                       second_model, second_field):
        self._check_invalid_related_objects_one_way(
                first_model,
                first_field,
                second_model,
        )
        self._check_invalid_related_objects_one_way(
                second_model,
                second_field,
                first_model,
        )


    def _check_all_invalid_related_objects(self):
        model_pairs = ((models.Host, 'labels', models.Label, 'host_set'),
                       (models.AclGroup, 'hosts', models.Host, 'aclgroup_set'),
                       (models.AclGroup, 'users', models.User, 'aclgroup_set'),
                       (models.Test, 'dependency_labels', models.Label,
                        'test_set'))
        for first_model, first_field, second_model, second_field in model_pairs:
            self._check_invalid_related_objects(
                    first_model,
                    first_field,
                    second_model,
                    second_field,
            )


    def _clear_inactive_blocks(self):
        logging.info('Clear out blocks for all completed jobs.')
        # this would be simpler using NOT IN (subquery), but MySQL
        # treats all IN subqueries as dependent, so this optimizes much
        # better
        self._db.execute("""
                DELETE ihq FROM afe_ineligible_host_queues ihq
                WHERE NOT EXISTS
                    (SELECT job_id FROM afe_host_queue_entries hqe
                     WHERE NOT hqe.complete AND hqe.job_id = ihq.job_id)""")


    def _should_reverify_hosts_now(self):
        reverify_period_sec = (scheduler_config.config.reverify_period_minutes
                               * 60)
        if reverify_period_sec == 0:
            return False
        return (self._last_reverify_time + reverify_period_sec) <= time.time()


    def _choose_subset_of_hosts_to_reverify(self, hosts):
        """Given hosts needing verification, return a subset to reverify."""
        max_at_once = scheduler_config.config.reverify_max_hosts_at_once
        if (max_at_once > 0 and len(hosts) > max_at_once):
            return random.sample(hosts, max_at_once)
        return sorted(hosts)


    def _reverify_dead_hosts(self):
        if not self._should_reverify_hosts_now():
            return

        self._last_reverify_time = time.time()
        logging.info('Checking for dead hosts to reverify')
        hosts = models.Host.objects.filter(
                status=models.Host.Status.REPAIR_FAILED,
                locked=False,
                invalid=False)
        hosts = hosts.exclude(
                protection=host_protections.Protection.DO_NOT_VERIFY)
        if not hosts:
            return

        hosts = list(hosts)
        total_hosts = len(hosts)
        hosts = self._choose_subset_of_hosts_to_reverify(hosts)
        logging.info('Reverifying dead hosts (%d of %d)', len(hosts),
                     total_hosts)
        with _cleanup_warning_banner('reverify dead hosts', len(hosts)):
            for host in hosts:
                logging.warning(host.hostname)
        _report_detected_errors('dead_hosts_triggered_reverify', len(hosts))
        _report_detected_errors('dead_hosts_require_reverify', total_hosts)
        for host in hosts:
            models.SpecialTask.schedule_special_task(
                    host=host, task=models.SpecialTask.Task.VERIFY)


    def _django_session_cleanup(self):
        """Clean up django_session since django doesn't for us.
           http://www.djangoproject.com/documentation/0.96/sessions/
        """
        logging.info('Deleting old sessions from django_session')
        sql = 'TRUNCATE TABLE django_session'
        self._db.execute(sql)


class TwentyFourHourUpkeep(PeriodicCleanup):
    """Cleanup that runs at the startup of monitor_db and every subsequent
       twenty four hours.
    """


    def __init__(self, db, drone_manager, run_at_initialize=True):
        """Initialize TwentyFourHourUpkeep.

        @param db: Database connection object.
        @param drone_manager: DroneManager to access drones.
        @param run_at_initialize: True to run cleanup when scheduler starts.
                                  Default is set to True.

        """
        self.drone_manager = drone_manager
        clean_interval_minutes = 24 * 60 # 24 hours
        super(TwentyFourHourUpkeep, self).__init__(
            db, clean_interval_minutes, run_at_initialize=run_at_initialize)


    @metrics.SecondsTimerDecorator(_METRICS_PREFIX + '/daily/durations')
    def _cleanup(self):
        logging.info('Running 24 hour clean up')
        self._check_for_uncleanable_db_inconsistencies()
        self._cleanup_orphaned_containers()


    def _check_for_uncleanable_db_inconsistencies(self):
        logging.info('Checking for uncleanable DB inconsistencies')
        self._check_for_active_and_complete_queue_entries()
        self._check_for_multiple_platform_hosts()
        self._check_for_no_platform_hosts()


    def _check_for_active_and_complete_queue_entries(self):
        query = models.HostQueueEntry.objects.filter(active=True, complete=True)
        num_bad_hqes = query.count()
        if num_bad_hqes == 0:
            return

        num_aborted = 0
        logging.warning('%d queue entries found with active=complete=1',
                        num_bad_hqes)
        with _cleanup_warning_banner('active and complete hqes', num_bad_hqes):
            for entry in query:
                if entry.status == 'Aborted':
                    entry.active = False
                    entry.save()
                    recovery_path = 'was also aborted, set active to False'
                    num_aborted += 1
                else:
                    recovery_path = 'can not recover'
                logging.warning('%s (recovery: %s)', entry.get_object_dict(),
                                recovery_path)
        _report_detected_errors('hqes_active_and_complete', num_bad_hqes)
        _report_detected_errors('hqes_aborted_set_to_inactive', num_aborted)


    def _check_for_multiple_platform_hosts(self):
        rows = self._db.execute("""
            SELECT afe_hosts.id, hostname, COUNT(1) AS platform_count,
                   GROUP_CONCAT(afe_labels.name)
            FROM afe_hosts
            INNER JOIN afe_hosts_labels ON
                    afe_hosts.id = afe_hosts_labels.host_id
            INNER JOIN afe_labels ON afe_hosts_labels.label_id = afe_labels.id
            WHERE afe_labels.platform
            GROUP BY afe_hosts.id
            HAVING platform_count > 1
            ORDER BY hostname""")

        if rows:
            logging.warning('Cleanup found hosts with multiple platforms')
            with _cleanup_warning_banner('hosts with multiple platforms',
                                         len(rows)):
                for row in rows:
                    logging.warning(' '.join(str(item) for item in row))
            _report_detected_errors('hosts_with_multiple_platforms', len(rows))


    def _check_for_no_platform_hosts(self):
        rows = self._db.execute("""
            SELECT hostname
            FROM afe_hosts
            LEFT JOIN afe_hosts_labels
              ON afe_hosts.id = afe_hosts_labels.host_id
              AND afe_hosts_labels.label_id IN (SELECT id FROM afe_labels
                                                WHERE platform)
            WHERE NOT afe_hosts.invalid AND afe_hosts_labels.host_id IS NULL""")
        if rows:
            with _cleanup_warning_banner('hosts with no platform', len(rows)):
                for row in rows:
                    logging.warning(row[0])
            _report_detected_errors('hosts_with_no_platform', len(rows))


    def _cleanup_orphaned_containers(self):
        """Cleanup orphaned containers in each drone.

        The function queues a lxc_cleanup call in each drone without waiting for
        the script to finish, as the cleanup procedure could take minutes and the
        script output is logged.

        """
        ssp_enabled = global_config.global_config.get_config_value(
                'AUTOSERV', 'enable_ssp_container')
        if not ssp_enabled:
            logging.info(
                    'Server-side packaging is not enabled, no need to clean '
                    'up orphaned containers.')
            return
        self.drone_manager.cleanup_orphaned_containers()


def _report_detected_errors(metric_name, count, fields={}):
    """Reports a counter metric for recovered errors

    @param metric_name: Name of the metric to report about.
    @param count: How many "errors" were fixed this cycle.
    @param fields: Optional fields to include with the metric.
    """
    m = '%s/errors_recovered/%s' % (_METRICS_PREFIX, metric_name)
    metrics.Counter(m).increment_by(count, fields=fields)


def _report_detected_errors(metric_name, gauge, fields={}):
    """Reports a gauge metric for errors detected

    @param metric_name: Name of the metric to report about.
    @param gauge: Outstanding number of unrecoverable errors of this type.
    @param fields: Optional fields to include with the metric.
    """
    m = '%s/errors_detected/%s' % (_METRICS_PREFIX, metric_name)
    metrics.Gauge(m).set(gauge, fields=fields)


@contextlib.contextmanager
def _cleanup_warning_banner(banner, error_count=None):
    """Put a clear context in the logs around list of errors

    @param: banner: The identifying header to print for context.
    @param: error_count: If not None, the number of errors detected.
    """
    if error_count is not None:
        banner += ' (total: %d)' % error_count
    logging.warning('#### START: %s ####', banner)
    try:
        yield
    finally:
        logging.warning('#### END: %s ####', banner)
