#!/usr/bin/python
# pylint: disable=missing-docstring

import unittest

import common
from autotest_lib.frontend import setup_django_environment
from autotest_lib.frontend import setup_test_environment
from autotest_lib.frontend.tko import models


class TKOTestTest(unittest.TestCase):

    def setUp(self):
        setup_test_environment.set_up()
        self.machine1 = models.Machine.objects.create(hostname='myhost')
        self.good_status = models.Status.objects.create(word='GOOD')
        kernel_name = 'mykernel1'
        self.kernel1 = models.Kernel.objects.create(kernel_hash=kernel_name,
                                               base=kernel_name,
                                               printable=kernel_name)
        self.job1 = models.Job.objects.create(
                tag='1-myjobtag1', label='myjob1',
                username='myuser', machine=self.machine1,
                afe_job_id=1)
        self.job1_test1 = models.Test.objects.create(
                job=self.job1, test='mytest1',
                kernel=self.kernel1,
                status=self.good_status,
                machine=self.machine1)


    def tearDown(self):
        setup_test_environment.tear_down()


    def _get_attributes(self, test):
        models.Test.objects.populate_relationships(
                [test], models.TestAttribute, 'attribute_list')
        return dict((attribute.attribute, attribute.value)
                    for attribute in test.attribute_list)

    def test_delete_attribute(self):
        test1 = self.job1_test1
        test1.set_attribute('test_attribute1', 'test_value1')

        attributes = self._get_attributes(test1)
        self.assertEquals(attributes['test_attribute1'], 'test_value1')

        test1.set_or_delete_attribute('test_attribute1', None)
        attributes = self._get_attributes(test1)
        self.assertNotIn('test_attribute1', attributes.keys())


    def test_set_attribute(self):
        # Verify adding static attribute in model_logic doesn't break TKO Test.
        test1 = self.job1_test1
        test1.set_attribute('test_attribute1', 'test_value1')
        test1.set_or_delete_attribute('test_attribute1', 'test_value2')
        attributes = self._get_attributes(test1)
        self.assertEquals(attributes['test_attribute1'], 'test_value2')
