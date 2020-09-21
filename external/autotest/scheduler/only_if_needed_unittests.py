#!/usr/bin/python

#pylint: disable=missing-docstring

import unittest

import common
from autotest_lib.frontend import setup_django_environment
from autotest_lib.frontend.afe import models
from autotest_lib.scheduler import monitor_db_unittest

_DEBUG = False


class OnlyIfNeededTest(monitor_db_unittest.DispatcherSchedulingTest):

    def _setup_test_only_if_needed_labels(self):
        # apply only_if_needed label3 to host1
        models.Host.smart_get('host1').labels.add(self.label3)
        return self._create_job_simple([1], use_metahost=True)


    def test_only_if_needed_labels_avoids_host(self):
        job = self._setup_test_only_if_needed_labels()
        # if the job doesn't depend on label3, there should be no scheduling
        self._run_scheduler()
        self._check_for_extra_schedulings()


    def test_only_if_needed_labels_schedules(self):
        job = self._setup_test_only_if_needed_labels()
        job.dependency_labels.add(self.label3)
        self._run_scheduler()
        self._assert_job_scheduled_on(1, 1)
        self._check_for_extra_schedulings()


    def test_only_if_needed_labels_via_metahost(self):
        job = self._setup_test_only_if_needed_labels()
        job.dependency_labels.add(self.label3)
        # should also work if the metahost is the only_if_needed label
        self._do_query('DELETE FROM afe_jobs_dependency_labels')
        self._create_job(metahosts=[3])
        self._run_scheduler()
        self._assert_job_scheduled_on(2, 1)
        self._check_for_extra_schedulings()


    def test_metahosts_obey_blocks(self):
        """
        Metahosts can't get scheduled on hosts already scheduled for
        that job.
        """
        self._create_job(metahosts=[1], hosts=[1])
        # make the nonmetahost entry complete, so the metahost can try
        # to get scheduled
        self._update_hqe(set='complete = 1', where='host_id=1')
        self._run_scheduler()
        self._check_for_extra_schedulings()


if __name__ == '__main__':
    unittest.main()

