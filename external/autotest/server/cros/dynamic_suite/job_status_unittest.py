#!/usr/bin/env python2
#
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable-msg=C0111

"""Unit tests for server/cros/dynamic_suite/job_status.py."""

import mox
import shutil
import tempfile
import time
import unittest
import os
import common

from autotest_lib.server import frontend
from autotest_lib.server.cros.dynamic_suite import host_spec
from autotest_lib.server.cros.dynamic_suite import job_status
from autotest_lib.server.cros.dynamic_suite.fakes import FakeJob
from autotest_lib.server.cros.dynamic_suite.fakes import FakeStatus


DEFAULT_WAITTIMEOUT_MINS = 60 * 4


class StatusTest(mox.MoxTestBase):
    """Unit tests for job_status.Status.
    """


    def setUp(self):
        super(StatusTest, self).setUp()
        self.afe = self.mox.CreateMock(frontend.AFE)
        self.tko = self.mox.CreateMock(frontend.TKO)

        self.tmpdir = tempfile.mkdtemp(suffix=type(self).__name__)


    def tearDown(self):
        super(StatusTest, self).tearDown()
        shutil.rmtree(self.tmpdir, ignore_errors=True)


    def expect_yield_job_entries(self, job):
        entries = [s.entry for s in job.statuses]
        self.afe.run('get_host_queue_entries',
                     job=job.id).AndReturn(entries)
        if True not in map(lambda e: 'aborted' in e and e['aborted'], entries):
            self.tko.get_job_test_statuses_from_db(job.id).AndReturn(
                    job.statuses)


    def testJobResultWaiter(self):
        """Should gather status and return records for job summaries."""
        jobs = [FakeJob(0, [FakeStatus('GOOD', 'T0', ''),
                            FakeStatus('GOOD', 'T1', '')]),
                FakeJob(1, [FakeStatus('ERROR', 'T0', 'err', False),
                            FakeStatus('GOOD', 'T1', '')]),
                FakeJob(2, [FakeStatus('TEST_NA', 'T0', 'no')]),
                FakeJob(3, [FakeStatus('FAIL', 'T0', 'broken')]),
                FakeJob(4, [FakeStatus('ERROR', 'SERVER_JOB', 'server error'),
                            FakeStatus('GOOD', 'T0', '')]),]
                # TODO: Write a better test for the case where we yield
                # results for aborts vs cannot yield results because of
                # a premature abort. Currently almost all client aborts
                # have been converted to failures, and when aborts do happen
                # they result in server job failures for which we always
                # want results.
                # FakeJob(5, [FakeStatus('ERROR', 'T0', 'gah', True)]),
                # The next job shouldn't be recorded in the results.
                # FakeJob(6, [FakeStatus('GOOD', 'SERVER_JOB', '')])]
        for status in jobs[4].statuses:
            status.entry['job'] = {'name': 'broken_infra_job'}

        job_id_set = set([job.id for job in jobs])
        yield_values = [
                [jobs[1]],
                [jobs[0], jobs[2]],
                jobs[3:6]
            ]
        self.mox.StubOutWithMock(time, 'sleep')
        for yield_this in yield_values:
            self.afe.get_jobs(id__in=list(job_id_set),
                              finished=True).AndReturn(yield_this)
            for job in yield_this:
                self.expect_yield_job_entries(job)
                job_id_set.remove(job.id)
            time.sleep(mox.IgnoreArg())
        self.mox.ReplayAll()

        waiter = job_status.JobResultWaiter(self.afe, self.tko)
        waiter.add_jobs(jobs)
        results = [result for result in waiter.wait_for_results()]
        for job in jobs[:6]:  # the 'GOOD' SERVER_JOB shouldn't be there.
            for status in job.statuses:
                self.assertTrue(True in map(status.equals_record, results))


    def testYieldSubdir(self):
        """Make sure subdir are properly set for test and non-test status."""
        job_tag = '0-owner/172.33.44.55'
        job_name = 'broken_infra_job'
        job = FakeJob(0, [FakeStatus('ERROR', 'SERVER_JOB', 'server error',
                                     subdir='---', job_tag=job_tag),
                          FakeStatus('GOOD', 'T0', '',
                                     subdir='T0.subdir', job_tag=job_tag)],
                      parent_job_id=54321)
        for status in job.statuses:
            status.entry['job'] = {'name': job_name}
        self.expect_yield_job_entries(job)
        self.mox.ReplayAll()
        results = list(job_status._yield_job_results(self.afe, self.tko, job))
        for i in range(len(results)):
            result = results[i]
            if result.test_name.endswith('SERVER_JOB'):
                expected_name = '%s_%s' % (job_name, job.statuses[i].test_name)
                expected_subdir = job_tag
            else:
                expected_name = job.statuses[i].test_name
                expected_subdir = os.path.join(job_tag, job.statuses[i].subdir)
            self.assertEqual(results[i].test_name, expected_name)
            self.assertEqual(results[i].subdir, expected_subdir)


    def _prepareForReporting(self, results):
        def callable(x):
            pass

        record_entity = self.mox.CreateMock(callable)
        group = self.mox.CreateMock(host_spec.HostGroup)

        statuses = {}
        all_bad = True not in results.itervalues()
        for hostname, result in results.iteritems():
            status = self.mox.CreateMock(job_status.Status)
            status.record_all(record_entity).InAnyOrder('recording')
            status.is_good().InAnyOrder('recording').AndReturn(result)
            if not result:
                status.test_name = 'test'
                if not all_bad:
                    status.override_status('WARN').InAnyOrder('recording')
            else:
                group.mark_host_success(hostname).InAnyOrder('recording')
            statuses[hostname] = status

        return (statuses, group, record_entity)


if __name__ == '__main__':
    unittest.main()
