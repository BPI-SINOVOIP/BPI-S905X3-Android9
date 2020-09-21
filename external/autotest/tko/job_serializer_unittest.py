#!/usr/bin/python

"""Unittests for the JobSerializer class.

Mostly test if the serialized object has the expected content.

"""

import datetime
import tempfile
import time
import unittest

import common
from autotest_lib.tko import tko_pb2
from autotest_lib.tko import job_serializer
from autotest_lib.tko import models

NamedTemporaryFile = tempfile.NamedTemporaryFile
datetime = datetime.datetime
mktime = time.mktime

class JobSerializerUnittest(unittest.TestCase):
    """Base class as a job serializer unittest"""

    def setUp(self):
        tko_patches = []
        tko_patches.append(models.patch('New spec!', 'Reference?',
                                        123456))

        tko_kernel = models.kernel('My Computer', tko_patches, '1234567')
        tko_time = datetime.now()

        tko_job = models.job('/tmp/', 'autotest', 'test', 'My Computer',
                             tko_time, tko_time, tko_time, 'root',
                             'www', 'No one', tko_time, {'1+1':2})
        tko_job.afe_parent_job_id = '111'
        tko_job.build_version = 'R1-1.0.0'
        tko_job.suite = 'bvt'
        tko_job.board = 'alex'
        tko_job.index = 2

        tko_iteration = models.iteration(0, {'2+2':4, '3+3':6},
                                   {'4+4':8, '5+5':10, '6+6':12})

        tko_labels = ['unittest', 'dummy test', 'autotest']

        # See the comment about the models.test constructor in
        # job_serializer.py, where the models.test constructor is used.
        tko_test = models.test('/tmp/', 'mocktest', 'Completed', 'N/A',
                               tko_kernel, 'My Computer', tko_time,
                               tko_time, [tko_iteration,
                               tko_iteration, tko_iteration],
                               {'abc':'def'}, [], tko_labels)
        tko_test.test_idx = 3
        self.tko_job = tko_job
        self.tko_job.tests = [tko_test, tko_test, tko_test]

        self.pb_job = tko_pb2.Job()
        self.tag = '1-abc./.'
        self.expected_afe_job_id = '1'

        js = job_serializer.JobSerializer()
        js.set_pb_job(self.tko_job, self.pb_job, self.tag)


    def test_tag(self):
        """Test serializing tag field."""
        self.assertEqual(self.tag, self.pb_job.tag)


    def test_afe_job_id(self):
        """Test serializing afe_job_id field."""
        self.assertEqual(self.expected_afe_job_id,
                         self.pb_job.afe_job_id)


    def test_job_dir(self):
        """Check if the dir field are the same.
        """
        self.assertEqual(self.tko_job.dir, self.pb_job.dir)


    def test_number_of_test(self):
        """Check if the number of test are the same.
        """
        self.assertEqual(len(self.tko_job.tests),
                         len(self.pb_job.tests))


    def test_user(self):
        """Check if the user field are the same.
        """
        self.assertEqual(self.tko_job.user, self.pb_job.user)


    def test_machine(self):
        """Check if the machine fields are the same.
        """
        self.assertEqual(self.tko_job.machine, self.pb_job.machine)


    def test_queued_time(self):
        """Check if queued_time are the same.
        """
        self.check_time(self.tko_job.queued_time,
                        self.pb_job.queued_time)


    def test_started_time(self):
        """Check if the started_time are the same.
        """
        self.check_time(self.tko_job.started_time,
                        self.pb_job.started_time)


    def test_finished_time(self):
        """Check if the finished_time are the same.
        """
        self.check_time(self.tko_job.finished_time,
                        self.pb_job.finished_time)


    def test_machine_owner(self):
        """Check if the machine owners are the same.
        """
        self.assertEqual(self.tko_job.machine_owner,
                         self.pb_job.machine_owner)


    def test_machine_group(self):
        """Check if the machine groups are the same.
        """
        self.assertEqual(self.tko_job.machine_group,
                         self.pb_job.machine_group)

    def test_aborted_by(self):
        """Check if the jobs are aborted by the same person.
        """
        self.assertEqual(self.tko_job.aborted_by,
                         self.pb_job.aborted_by)


    def test_aborted_on(self):
        """Test serializing aborted_on field."""
        self.check_time(self.tko_job.aborted_on,
                        self.pb_job.aborted_on)


    def test_keyval_dict(self):
        """Check if the contents of the dictionary are the same.
        """
        self.assertEqual(len(self.tko_job.keyval_dict),
                         len(self.pb_job.keyval_dict))
        self.check_dict(self.tko_job.keyval_dict,
                        self.convert_keyval_to_dict(self.pb_job,
                        'keyval_dict'))


    def test_job_idx(self):
        """Test serializing job_idx field."""
        self.assertEqual(self.tko_job.index,
                        self.pb_job.job_idx)


    def test_afe_parent_job_id(self):
        """Test serializing afe_parent_job_id field."""
        self.assertEqual(self.tko_job.afe_parent_job_id,
                        self.pb_job.afe_parent_job_id)


    def test_build_version(self):
        """Test serializing build_version field."""
        self.assertEqual(self.tko_job.build_version,
                        self.pb_job.build_version)


    def test_suite(self):
        """Test serializing suite field."""
        self.assertEqual(self.tko_job.suite,
                        self.pb_job.suite)


    def test_board(self):
        """Test serializing board field."""
        self.assertEqual(self.tko_job.board,
                        self.pb_job.board)


    def test_tests(self):
        """Check if all the test are the same.
        """

        for test, newtest in zip(self.tko_job.tests,
                                 self.pb_job.tests):

            self.assertEqual(test.subdir, newtest.subdir)
            self.assertEqual(test.testname, newtest.testname)
            self.assertEqual(test.status, newtest.status)
            self.assertEqual(test.reason, newtest.reason)
            self.assertEqual(test.machine, newtest.machine)
            self.assertEqual(test.labels, newtest.labels)
            self.assertEqual(test.test_idx, newtest.test_idx)

            self.check_time(test.started_time, newtest.started_time)
            self.check_time(test.finished_time, newtest.finished_time)

            self.check_iteration(test.iterations, newtest.iterations)

            self.check_dict(test.attributes,
                            self.convert_keyval_to_dict(newtest,
                            'attributes'))

            self.check_kernel(test.kernel, newtest.kernel)


    def check_time(self, dTime, stime):
        """Check if the datetime object contains the same time value
        in microseconds.

        @param dTime: The datetime.
        @param stime: The original time.
        """
        t = mktime(dTime.timetuple()) + 1e-6 * dTime.microsecond
        self.assertEqual(long(t), stime/1000)


    def check_iteration(self, tko_iterations, pb_iterations):
        """Check if the iteration objects are the same.

        @param tko_iterations: The list of iterations.
        @param pb_iterations: The proto iterations.
        """
        for tko_iteration, pb_iteration in zip(tko_iterations,
                                               pb_iterations):

            self.assertEqual(tko_iteration.index, pb_iteration.index)

            self.check_dict(tko_iteration.attr_keyval,
                            self.convert_keyval_to_dict(pb_iteration,
                                                        'attr_keyval'))

            self.check_dict(tko_iteration.perf_keyval,
                            self.convert_keyval_to_dict(pb_iteration,
                                                        'perf_keyval'))


    def convert_keyval_to_dict(self, var, attr):
        """Convert a protocol buffer repeated keyval object into a
        python dict.

        @param var: The variable name.
        @param attr: The attribute name.
        """

        return dict((keyval.name, keyval.value) for keyval in
                    getattr(var,attr))


    def check_dict(self, dictionary, keyval):
        """Check if the contents of the dictionary are the same as a
        repeated keyval pair.

        @param dictionary: The dict object.
        @param keyval: The keyval object.
        """
        for key, value in dictionary.iteritems():
            self.assertTrue(key in keyval);
            self.assertEqual(str(value), keyval[key])


    def check_kernel(self, kernel, newkernel):
        """Check if the kernels are the same.

        @param kernel: The kernel object.
        @param newkernel: The proto kernel object.
        """
        self.assertEqual(kernel.base, newkernel.base)
        self.assertEqual(kernel.kernel_hash, newkernel.kernel_hash)


class ReadBackTest(JobSerializerUnittest):
    """Check if convert between models.job and pb job is correct even
    after being written to binary and read by manually
    """

    def setUp(self):
        """Setup the test."""
        super(ReadBackTest, self).setUp()

        out_binary = NamedTemporaryFile(mode='wb')
        try:
            out_binary.write(self.pb_job.SerializeToString())
            out_binary.flush()

            binary = open(out_binary.name, 'rb')
            try:
                self.pb_job = tko_pb2.Job()
                self.pb_job.ParseFromString(binary.read())
            finally:
                binary.close()
        finally:
            out_binary.close()


class ReadBackGetterTest(JobSerializerUnittest):
    """Check if convert between models.job and pb job is correct after
    using the getter methods in JobSerializer to read back the
    data.
    """

    def setUp(self):
        super(ReadBackGetterTest, self).setUp()

        temp_binary = NamedTemporaryFile(mode='wb')
        try:
            temp_binary.write(self.pb_job.SerializeToString())
            temp_binary.flush()

            js = job_serializer.JobSerializer()
            self.from_pb_job = js.deserialize_from_binary(temp_binary.name)
        finally:
            temp_binary.close()


    def test_keyval_dict(self):
        """Check if the contents of the dictionary are the same.  """

        self.assertEqual(len(self.tko_job.keyval_dict),
                         len(self.from_pb_job.keyval_dict))

        self.check_dict(self.tko_job.keyval_dict,
                        self.from_pb_job.keyval_dict)


    def test_tests(self):
        """Check if all the test are the same.
        """
        for test, newtest in zip(self.tko_job.tests,
                                 self.from_pb_job.tests):

            self.assertEqual(test.subdir, newtest.subdir)
            self.assertEqual(test.testname, newtest.testname)
            self.assertEqual(test.status, newtest.status)
            self.assertEqual(test.reason, newtest.reason)
            self.assertEqual(test.machine, newtest.machine)
            self.assertEqual(test.labels, newtest.labels)

            self.check_time(test.started_time, newtest.started_time)
            self.check_time(test.finished_time, newtest.finished_time)

            self.check_iteration(test.iterations, newtest.iterations)

            self.check_dict(test.attributes, newtest.attributes)

            self.check_kernel(test.kernel, newtest.kernel)


    def check_time(self, dTime, sTime):
        """Check if the datetime object contains the same time value
        in microseconds.

        If sTime is type int or type long, then only convert dTime to
        microseconds. Else, convert both dTime and sTime to
        microseconds. Then, compare the two after casting them to
        long.
        """

        t = mktime(dTime.timetuple()) + 1e-6 * dTime.microsecond
        if isinstance(sTime, (int, long)):
            self.assertEqual(long(t*1000), sTime)
        else:
            t1 = mktime(sTime.timetuple()) + 1e-6 * sTime.microsecond
            self.assertEqual(long(t*1000), long(t1*1000))


    def check_iteration(self, iterations, newiterations):
        """Check if the iteration objects are the same.
        """
        for iteration, newiteration in zip(iterations, newiterations):
            self.assertEqual(iteration.index, newiteration.index)
            self.check_dict(iteration.attr_keyval,
                            newiteration.attr_keyval)
            self.check_dict(iteration.perf_keyval,
                            newiteration.perf_keyval)


if __name__ == '__main__':
    unittest.main()
