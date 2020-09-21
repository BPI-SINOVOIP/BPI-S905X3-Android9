#!/usr/bin/env python2
# pylint: disable=missing-docstring

import datetime
import mox
import unittest

import common
from autotest_lib.client.common_lib import control_data
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib import priorities
from autotest_lib.client.common_lib.cros import dev_server
from autotest_lib.client.common_lib.test_utils import mock
from autotest_lib.frontend import setup_django_environment
from autotest_lib.frontend.afe import frontend_test_utils
from autotest_lib.frontend.afe import model_logic
from autotest_lib.frontend.afe import models
from autotest_lib.frontend.afe import rpc_interface
from autotest_lib.frontend.afe import rpc_utils
from autotest_lib.server import frontend
from autotest_lib.server import utils as server_utils
from autotest_lib.server.cros import provision
from autotest_lib.server.cros.dynamic_suite import constants
from autotest_lib.server.cros.dynamic_suite import control_file_getter
from autotest_lib.server.cros.dynamic_suite import frontend_wrappers

CLIENT = control_data.CONTROL_TYPE_NAMES.CLIENT
SERVER = control_data.CONTROL_TYPE_NAMES.SERVER

_hqe_status = models.HostQueueEntry.Status


class ShardHeartbeatTest(mox.MoxTestBase, unittest.TestCase):

    _PRIORITY = priorities.Priority.DEFAULT

    def _do_heartbeat_and_assert_response(self, shard_hostname='shard1',
                                          upload_jobs=(), upload_hqes=(),
                                          known_jobs=(), known_hosts=(),
                                          **kwargs):
        known_job_ids = [job.id for job in known_jobs]
        known_host_ids = [host.id for host in known_hosts]
        known_host_statuses = [host.status for host in known_hosts]

        retval = rpc_interface.shard_heartbeat(
            shard_hostname=shard_hostname,
            jobs=upload_jobs, hqes=upload_hqes,
            known_job_ids=known_job_ids, known_host_ids=known_host_ids,
            known_host_statuses=known_host_statuses)

        self._assert_shard_heartbeat_response(shard_hostname, retval,
                                              **kwargs)

        return shard_hostname


    def _assert_shard_heartbeat_response(self, shard_hostname, retval, jobs=[],
                                         hosts=[], hqes=[],
                                         incorrect_host_ids=[]):

        retval_hosts, retval_jobs = retval['hosts'], retval['jobs']
        retval_incorrect_hosts = retval['incorrect_host_ids']

        expected_jobs = [
            (job.id, job.name, shard_hostname) for job in jobs]
        returned_jobs = [(job['id'], job['name'], job['shard']['hostname'])
                         for job in retval_jobs]
        self.assertEqual(returned_jobs, expected_jobs)

        expected_hosts = [(host.id, host.hostname) for host in hosts]
        returned_hosts = [(host['id'], host['hostname'])
                          for host in retval_hosts]
        self.assertEqual(returned_hosts, expected_hosts)

        retval_hqes = []
        for job in retval_jobs:
            retval_hqes += job['hostqueueentry_set']

        expected_hqes = [(hqe.id) for hqe in hqes]
        returned_hqes = [(hqe['id']) for hqe in retval_hqes]
        self.assertEqual(returned_hqes, expected_hqes)

        self.assertEqual(retval_incorrect_hosts, incorrect_host_ids)


    def _createJobForLabel(self, label):
        job_id = rpc_interface.create_job(name='dummy', priority=self._PRIORITY,
                                          control_file='foo',
                                          control_type=CLIENT,
                                          meta_hosts=[label.name],
                                          dependencies=(label.name,))
        return models.Job.objects.get(id=job_id)


    def _testShardHeartbeatFetchHostlessJobHelper(self, host1):
        """Create a hostless job and ensure it's not assigned to a shard."""
        label2 = models.Label.objects.create(name='bluetooth', platform=False)

        job1 = self._create_job(hostless=True)

        # Hostless jobs should be executed by the global scheduler.
        self._do_heartbeat_and_assert_response(hosts=[host1])


    def _testShardHeartbeatIncorrectHostsHelper(self, host1):
        """Ensure that hosts that don't belong to shard are determined."""
        host2 = models.Host.objects.create(hostname='test_host2', leased=False)

        # host2 should not belong to shard1. Ensure that if shard1 thinks host2
        # is a known host, then it is returned as invalid.
        self._do_heartbeat_and_assert_response(known_hosts=[host1, host2],
                                               incorrect_host_ids=[host2.id])


    def _testShardHeartbeatLabelRemovalRaceHelper(self, shard1, host1, label1):
        """Ensure correctness if label removed during heartbeat."""
        host2 = models.Host.objects.create(hostname='test_host2', leased=False)
        host2.labels.add(label1)
        self.assertEqual(host2.shard, None)

        # In the middle of the assign_to_shard call, remove label1 from shard1.
        self.mox.StubOutWithMock(models.Host, '_assign_to_shard_nothing_helper')
        def remove_label():
            rpc_interface.remove_board_from_shard(shard1.hostname, label1.name)

        models.Host._assign_to_shard_nothing_helper().WithSideEffects(
            remove_label)
        self.mox.ReplayAll()

        self._do_heartbeat_and_assert_response(
            known_hosts=[host1], hosts=[], incorrect_host_ids=[host1.id])
        host2 = models.Host.smart_get(host2.id)
        self.assertEqual(host2.shard, None)


    def _testShardRetrieveJobsHelper(self, shard1, host1, label1, shard2,
                                     host2, label2):
        """Create jobs and retrieve them."""
        # should never be returned by heartbeat
        leased_host = models.Host.objects.create(hostname='leased_host',
                                                 leased=True)

        leased_host.labels.add(label1)

        job1 = self._createJobForLabel(label1)

        job2 = self._createJobForLabel(label2)

        job_completed = self._createJobForLabel(label1)
        # Job is already being run, so don't sync it
        job_completed.hostqueueentry_set.update(complete=True)
        job_completed.hostqueueentry_set.create(complete=False)

        job_active = self._createJobForLabel(label1)
        # Job is already started, so don't sync it
        job_active.hostqueueentry_set.update(active=True)
        job_active.hostqueueentry_set.create(complete=False, active=False)

        self._do_heartbeat_and_assert_response(
            jobs=[job1], hosts=[host1], hqes=job1.hostqueueentry_set.all())

        self._do_heartbeat_and_assert_response(
            shard_hostname=shard2.hostname,
            jobs=[job2], hosts=[host2], hqes=job2.hostqueueentry_set.all())

        host3 = models.Host.objects.create(hostname='test_host3', leased=False)
        host3.labels.add(label1)

        self._do_heartbeat_and_assert_response(
            known_jobs=[job1], known_hosts=[host1], hosts=[host3])


    def _testResendJobsAfterFailedHeartbeatHelper(self, shard1, host1, label1):
        """Create jobs, retrieve them, fail on client, fetch them again."""
        job1 = self._createJobForLabel(label1)

        self._do_heartbeat_and_assert_response(
            jobs=[job1],
            hqes=job1.hostqueueentry_set.all(), hosts=[host1])

        # Make sure it's resubmitted by sending last_job=None again
        self._do_heartbeat_and_assert_response(
            known_hosts=[host1],
            jobs=[job1], hqes=job1.hostqueueentry_set.all(), hosts=[])

        # Now it worked, make sure it's not sent again
        self._do_heartbeat_and_assert_response(
            known_jobs=[job1], known_hosts=[host1])

        job1 = models.Job.objects.get(pk=job1.id)
        job1.hostqueueentry_set.all().update(complete=True)

        # Job is completed, make sure it's not sent again
        self._do_heartbeat_and_assert_response(
            known_hosts=[host1])

        job2 = self._createJobForLabel(label1)

        # job2's creation was later, it should be returned now.
        self._do_heartbeat_and_assert_response(
            known_hosts=[host1],
            jobs=[job2], hqes=job2.hostqueueentry_set.all())

        self._do_heartbeat_and_assert_response(
            known_jobs=[job2], known_hosts=[host1])

        job2 = models.Job.objects.get(pk=job2.pk)
        job2.hostqueueentry_set.update(aborted=True)
        # Setting a job to a complete status will set the shard_id to None in
        # scheduler_models. We have to emulate that here, because we use Django
        # models in tests.
        job2.shard = None
        job2.save()

        self._do_heartbeat_and_assert_response(
            known_jobs=[job2], known_hosts=[host1],
            jobs=[job2],
            hqes=job2.hostqueueentry_set.all())

        models.Test.objects.create(name='platform_BootPerfServer:shard',
                                   test_type=1)
        self.mox.StubOutWithMock(server_utils, 'read_file')
        self.mox.ReplayAll()
        rpc_interface.delete_shard(hostname=shard1.hostname)

        self.assertRaises(
            models.Shard.DoesNotExist, models.Shard.objects.get, pk=shard1.id)

        job1 = models.Job.objects.get(pk=job1.id)
        label1 = models.Label.objects.get(pk=label1.id)

        self.assertIsNone(job1.shard)
        self.assertEqual(len(label1.shard_set.all()), 0)


    def _testResendHostsAfterFailedHeartbeatHelper(self, host1):
        """Check that master accepts resending updated records after failure."""
        # Send the host
        self._do_heartbeat_and_assert_response(hosts=[host1])

        # Send it again because previous one didn't persist correctly
        self._do_heartbeat_and_assert_response(hosts=[host1])

        # Now it worked, make sure it isn't sent again
        self._do_heartbeat_and_assert_response(known_hosts=[host1])


class RpcInterfaceTestWithStaticAttribute(
        mox.MoxTestBase, unittest.TestCase,
        frontend_test_utils.FrontendTestMixin):

    def setUp(self):
        super(RpcInterfaceTestWithStaticAttribute, self).setUp()
        self._frontend_common_setup()
        self.god = mock.mock_god()
        self.old_respect_static_config = rpc_interface.RESPECT_STATIC_ATTRIBUTES
        rpc_interface.RESPECT_STATIC_ATTRIBUTES = True
        models.RESPECT_STATIC_ATTRIBUTES = True


    def tearDown(self):
        self.god.unstub_all()
        self._frontend_common_teardown()
        global_config.global_config.reset_config_values()
        rpc_interface.RESPECT_STATIC_ATTRIBUTES = self.old_respect_static_config
        models.RESPECT_STATIC_ATTRIBUTES = self.old_respect_static_config


    def _fake_host_with_static_attributes(self):
        host1 = models.Host.objects.create(hostname='test_host')
        host1.set_attribute('test_attribute1', 'test_value1')
        host1.set_attribute('test_attribute2', 'test_value2')
        self._set_static_attribute(host1, 'test_attribute1', 'static_value1')
        self._set_static_attribute(host1, 'static_attribute1', 'static_value2')
        host1.save()
        return host1


    def test_get_hosts(self):
        host1 = self._fake_host_with_static_attributes()
        hosts = rpc_interface.get_hosts(hostname=host1.hostname)
        host = hosts[0]

        self.assertEquals(host['hostname'], 'test_host')
        self.assertEquals(host['acls'], ['Everyone'])
        # Respect the value of static attributes.
        self.assertEquals(host['attributes'],
                          {'test_attribute1': 'static_value1',
                           'test_attribute2': 'test_value2',
                           'static_attribute1': 'static_value2'})

    def test_get_host_attribute_with_static(self):
        host1 = models.Host.objects.create(hostname='test_host1')
        host1.set_attribute('test_attribute1', 'test_value1')
        self._set_static_attribute(host1, 'test_attribute1', 'static_value1')
        host2 = models.Host.objects.create(hostname='test_host2')
        host2.set_attribute('test_attribute1', 'test_value1')
        host2.set_attribute('test_attribute2', 'test_value2')

        attributes = rpc_interface.get_host_attribute(
                'test_attribute1',
                hostname__in=['test_host1', 'test_host2'])
        hosts = [attr['host'] for attr in attributes]
        values = [attr['value'] for attr in attributes]
        self.assertEquals(set(hosts),
                          set(['test_host1', 'test_host2']))
        self.assertEquals(set(values),
                          set(['test_value1', 'static_value1']))


    def test_get_hosts_by_attribute_without_static(self):
        host1 = models.Host.objects.create(hostname='test_host1')
        host1.set_attribute('test_attribute1', 'test_value1')
        host2 = models.Host.objects.create(hostname='test_host2')
        host2.set_attribute('test_attribute1', 'test_value1')

        hosts = rpc_interface.get_hosts_by_attribute(
                'test_attribute1', 'test_value1')
        self.assertEquals(set(hosts),
                          set(['test_host1', 'test_host2']))


    def test_get_hosts_by_attribute_with_static(self):
        host1 = models.Host.objects.create(hostname='test_host1')
        host1.set_attribute('test_attribute1', 'test_value1')
        self._set_static_attribute(host1, 'test_attribute1', 'test_value1')
        host2 = models.Host.objects.create(hostname='test_host2')
        host2.set_attribute('test_attribute1', 'test_value1')
        self._set_static_attribute(host2, 'test_attribute1', 'static_value1')
        host3 = models.Host.objects.create(hostname='test_host3')
        self._set_static_attribute(host3, 'test_attribute1', 'test_value1')
        host4 = models.Host.objects.create(hostname='test_host4')
        host4.set_attribute('test_attribute1', 'test_value1')
        host5 = models.Host.objects.create(hostname='test_host5')
        host5.set_attribute('test_attribute1', 'temp_value1')
        self._set_static_attribute(host5, 'test_attribute1', 'test_value1')

        hosts = rpc_interface.get_hosts_by_attribute(
                'test_attribute1', 'test_value1')
        # host1: matched, it has the same value for test_attribute1.
        # host2: not matched, it has a new value in
        #        afe_static_host_attributes for test_attribute1.
        # host3: matched, it has a corresponding entry in
        #        afe_host_attributes for test_attribute1.
        # host4: matched, test_attribute1 is not replaced by static
        #        attribute.
        # host5: matched, it has an updated & matched value for
        #        test_attribute1 in afe_static_host_attributes.
        self.assertEquals(set(hosts),
                          set(['test_host1', 'test_host3',
                               'test_host4', 'test_host5']))


class RpcInterfaceTestWithStaticLabel(ShardHeartbeatTest,
                                      frontend_test_utils.FrontendTestMixin):

    _STATIC_LABELS = ['board:lumpy']

    def setUp(self):
        super(RpcInterfaceTestWithStaticLabel, self).setUp()
        self._frontend_common_setup()
        self.god = mock.mock_god()
        self.old_respect_static_config = rpc_interface.RESPECT_STATIC_LABELS
        rpc_interface.RESPECT_STATIC_LABELS = True
        models.RESPECT_STATIC_LABELS = True


    def tearDown(self):
        self.god.unstub_all()
        self._frontend_common_teardown()
        global_config.global_config.reset_config_values()
        rpc_interface.RESPECT_STATIC_LABELS = self.old_respect_static_config
        models.RESPECT_STATIC_LABELS = self.old_respect_static_config


    def _fake_host_with_static_labels(self):
        host1 = models.Host.objects.create(hostname='test_host')
        label1 = models.Label.objects.create(
                name='non_static_label1', platform=False)
        non_static_platform = models.Label.objects.create(
                name='static_platform', platform=False)
        static_platform = models.StaticLabel.objects.create(
                name='static_platform', platform=True)
        models.ReplacedLabel.objects.create(label_id=non_static_platform.id)
        host1.static_labels.add(static_platform)
        host1.labels.add(non_static_platform)
        host1.labels.add(label1)
        host1.save()
        return host1


    def test_get_hosts(self):
        host1 = self._fake_host_with_static_labels()
        hosts = rpc_interface.get_hosts(hostname=host1.hostname)
        host = hosts[0]

        self.assertEquals(host['hostname'], 'test_host')
        self.assertEquals(host['acls'], ['Everyone'])
        # Respect all labels in afe_hosts_labels.
        self.assertEquals(host['labels'],
                          ['non_static_label1', 'static_platform'])
        # Respect static labels.
        self.assertEquals(host['platform'], 'static_platform')


    def test_get_hosts_multiple_labels(self):
        self._fake_host_with_static_labels()
        hosts = rpc_interface.get_hosts(
                multiple_labels=['non_static_label1', 'static_platform'])
        host = hosts[0]
        self.assertEquals(host['hostname'], 'test_host')


    def test_delete_static_label(self):
        label1 = models.Label.smart_get('static')

        host2 = models.Host.objects.all()[1]
        shard1 = models.Shard.objects.create(hostname='shard1')
        host2.shard = shard1
        host2.labels.add(label1)
        host2.save()

        mock_afe = self.god.create_mock_class_obj(frontend_wrappers.RetryingAFE,
                                                  'MockAFE')
        self.god.stub_with(frontend_wrappers, 'RetryingAFE', mock_afe)

        self.assertRaises(error.UnmodifiableLabelException,
                          rpc_interface.delete_label,
                          label1.id)

        self.god.check_playback()


    def test_modify_static_label(self):
        label1 = models.Label.smart_get('static')
        self.assertEqual(label1.invalid, 0)

        host2 = models.Host.objects.all()[1]
        shard1 = models.Shard.objects.create(hostname='shard1')
        host2.shard = shard1
        host2.labels.add(label1)
        host2.save()

        mock_afe = self.god.create_mock_class_obj(frontend_wrappers.RetryingAFE,
                                                  'MockAFE')
        self.god.stub_with(frontend_wrappers, 'RetryingAFE', mock_afe)

        self.assertRaises(error.UnmodifiableLabelException,
                          rpc_interface.modify_label,
                          label1.id,
                          invalid=1)

        self.assertEqual(models.Label.smart_get('static').invalid, 0)
        self.god.check_playback()


    def test_multiple_platforms_add_non_static_to_static(self):
        """Test non-static platform to a host with static platform."""
        static_platform = models.StaticLabel.objects.create(
                name='static_platform', platform=True)
        non_static_platform = models.Label.objects.create(
                name='static_platform', platform=True)
        models.ReplacedLabel.objects.create(label_id=non_static_platform.id)
        platform2 = models.Label.objects.create(name='platform2', platform=True)
        host1 = models.Host.objects.create(hostname='test_host')
        host1.static_labels.add(static_platform)
        host1.labels.add(non_static_platform)
        host1.save()

        self.assertRaises(model_logic.ValidationError,
                          rpc_interface.label_add_hosts, id='platform2',
                          hosts=['test_host'])
        self.assertRaises(model_logic.ValidationError,
                          rpc_interface.host_add_labels,
                          id='test_host', labels=['platform2'])
        # make sure the platform didn't get added
        platforms = rpc_interface.get_labels(
            host__hostname__in=['test_host'], platform=True)
        self.assertEquals(len(platforms), 1)


    def test_multiple_platforms_add_static_to_non_static(self):
        """Test static platform to a host with non-static platform."""
        platform1 = models.Label.objects.create(
                name='static_platform', platform=True)
        models.ReplacedLabel.objects.create(label_id=platform1.id)
        static_platform = models.StaticLabel.objects.create(
                name='static_platform', platform=True)
        platform2 = models.Label.objects.create(
                name='platform2', platform=True)

        host1 = models.Host.objects.create(hostname='test_host')
        host1.labels.add(platform2)
        host1.save()

        self.assertRaises(model_logic.ValidationError,
                          rpc_interface.label_add_hosts,
                          id='static_platform',
                          hosts=['test_host'])
        self.assertRaises(model_logic.ValidationError,
                          rpc_interface.host_add_labels,
                          id='test_host', labels=['static_platform'])
        # make sure the platform didn't get added
        platforms = rpc_interface.get_labels(
            host__hostname__in=['test_host'], platform=True)
        self.assertEquals(len(platforms), 1)


    def test_label_remove_hosts(self):
        """Test remove a label of hosts."""
        label = models.Label.smart_get('static')
        static_label = models.StaticLabel.objects.create(name='static')

        host1 = models.Host.objects.create(hostname='test_host')
        host1.labels.add(label)
        host1.static_labels.add(static_label)
        host1.save()

        self.assertRaises(error.UnmodifiableLabelException,
                          rpc_interface.label_remove_hosts,
                          id='static', hosts=['test_host'])


    def test_host_remove_labels(self):
        """Test remove labels of a given host."""
        label = models.Label.smart_get('static')
        label1 = models.Label.smart_get('label1')
        label2 = models.Label.smart_get('label2')
        static_label = models.StaticLabel.objects.create(name='static')

        host1 = models.Host.objects.create(hostname='test_host')
        host1.labels.add(label)
        host1.labels.add(label1)
        host1.labels.add(label2)
        host1.static_labels.add(static_label)
        host1.save()

        rpc_interface.host_remove_labels(
                'test_host', ['static', 'label1'])
        labels = rpc_interface.get_labels(host__hostname__in=['test_host'])
        # Only non_static label 'label1' is removed.
        self.assertEquals(len(labels), 2)
        self.assertEquals(labels[0].get('name'), 'label2')


    def test_remove_board_from_shard(self):
        """test remove a board (static label) from shard."""
        label = models.Label.smart_get('static')
        static_label = models.StaticLabel.objects.create(name='static')

        shard = models.Shard.objects.create(hostname='test_shard')
        shard.labels.add(label)

        host = models.Host.objects.create(hostname='test_host',
                                          leased=False,
                                          shard=shard)
        host.static_labels.add(static_label)
        host.save()

        rpc_interface.remove_board_from_shard(shard.hostname, label.name)
        host1 = models.Host.smart_get(host.id)
        shard1 = models.Shard.smart_get(shard.id)
        self.assertEqual(host1.shard, None)
        self.assertItemsEqual(shard1.labels.all(), [])


    def test_check_job_dependencies_success(self):
        """Test check_job_dependencies successfully."""
        static_label = models.StaticLabel.objects.create(name='static')

        host = models.Host.objects.create(hostname='test_host')
        host.static_labels.add(static_label)
        host.save()

        host1 = models.Host.smart_get(host.id)
        rpc_utils.check_job_dependencies([host1], ['static'])


    def test_check_job_dependencies_fail(self):
        """Test check_job_dependencies with raising ValidationError."""
        label = models.Label.smart_get('static')
        static_label = models.StaticLabel.objects.create(name='static')

        host = models.Host.objects.create(hostname='test_host')
        host.labels.add(label)
        host.save()

        host1 = models.Host.smart_get(host.id)
        self.assertRaises(model_logic.ValidationError,
                          rpc_utils.check_job_dependencies,
                          [host1],
                          ['static'])

    def test_check_job_metahost_dependencies_success(self):
        """Test check_job_metahost_dependencies successfully."""
        label1 = models.Label.smart_get('label1')
        label2 = models.Label.smart_get('label2')
        label = models.Label.smart_get('static')
        static_label = models.StaticLabel.objects.create(name='static')

        host = models.Host.objects.create(hostname='test_host')
        host.static_labels.add(static_label)
        host.labels.add(label1)
        host.labels.add(label2)
        host.save()

        rpc_utils.check_job_metahost_dependencies(
                [label1, label], [label2.name])
        rpc_utils.check_job_metahost_dependencies(
                [label1], [label2.name, static_label.name])


    def test_check_job_metahost_dependencies_fail(self):
        """Test check_job_metahost_dependencies with raising errors."""
        label1 = models.Label.smart_get('label1')
        label2 = models.Label.smart_get('label2')
        label = models.Label.smart_get('static')
        static_label = models.StaticLabel.objects.create(name='static')

        host = models.Host.objects.create(hostname='test_host')
        host.labels.add(label1)
        host.labels.add(label2)
        host.save()

        self.assertRaises(error.NoEligibleHostException,
                          rpc_utils.check_job_metahost_dependencies,
                          [label1, label], [label2.name])
        self.assertRaises(error.NoEligibleHostException,
                          rpc_utils.check_job_metahost_dependencies,
                          [label1], [label2.name, static_label.name])


    def _createShardAndHostWithStaticLabel(self,
                                           shard_hostname='shard1',
                                           host_hostname='test_host1',
                                           label_name='board:lumpy'):
        label = models.Label.objects.create(name=label_name)

        shard = models.Shard.objects.create(hostname=shard_hostname)
        shard.labels.add(label)

        host = models.Host.objects.create(hostname=host_hostname, leased=False,
                                          shard=shard)
        host.labels.add(label)
        if label_name in self._STATIC_LABELS:
            models.ReplacedLabel.objects.create(label_id=label.id)
            static_label = models.StaticLabel.objects.create(name=label_name)
            host.static_labels.add(static_label)

        return shard, host, label


    def testShardHeartbeatFetchHostlessJob(self):
        shard1, host1, label1 = self._createShardAndHostWithStaticLabel(
                host_hostname='test_host1')
        self._testShardHeartbeatFetchHostlessJobHelper(host1)


    def testShardHeartbeatIncorrectHosts(self):
        shard1, host1, label1 = self._createShardAndHostWithStaticLabel(
                host_hostname='test_host1')
        self._testShardHeartbeatIncorrectHostsHelper(host1)


    def testShardHeartbeatLabelRemovalRace(self):
        shard1, host1, label1 = self._createShardAndHostWithStaticLabel(
                host_hostname='test_host1')
        self._testShardHeartbeatLabelRemovalRaceHelper(shard1, host1, label1)


    def testShardRetrieveJobs(self):
        shard1, host1, label1 = self._createShardAndHostWithStaticLabel()
        shard2, host2, label2 = self._createShardAndHostWithStaticLabel(
            'shard2', 'test_host2', 'board:grumpy')
        self._testShardRetrieveJobsHelper(shard1, host1, label1,
                                          shard2, host2, label2)


    def testResendJobsAfterFailedHeartbeat(self):
        shard1, host1, label1 = self._createShardAndHostWithStaticLabel()
        self._testResendJobsAfterFailedHeartbeatHelper(shard1, host1, label1)


    def testResendHostsAfterFailedHeartbeat(self):
        shard1, host1, label1 = self._createShardAndHostWithStaticLabel(
                host_hostname='test_host1')
        self._testResendHostsAfterFailedHeartbeatHelper(host1)


class RpcInterfaceTest(unittest.TestCase,
                       frontend_test_utils.FrontendTestMixin):
    def setUp(self):
        self._frontend_common_setup()
        self.god = mock.mock_god()


    def tearDown(self):
        self.god.unstub_all()
        self._frontend_common_teardown()
        global_config.global_config.reset_config_values()


    def test_validation(self):
        # omit a required field
        self.assertRaises(model_logic.ValidationError, rpc_interface.add_label,
                          name=None)
        # violate uniqueness constraint
        self.assertRaises(model_logic.ValidationError, rpc_interface.add_host,
                          hostname='host1')


    def test_multiple_platforms(self):
        platform2 = models.Label.objects.create(name='platform2', platform=True)
        self.assertRaises(model_logic.ValidationError,
                          rpc_interface. label_add_hosts, id='platform2',
                          hosts=['host1', 'host2'])
        self.assertRaises(model_logic.ValidationError,
                          rpc_interface.host_add_labels,
                          id='host1', labels=['platform2'])
        # make sure the platform didn't get added
        platforms = rpc_interface.get_labels(
            host__hostname__in=['host1', 'host2'], platform=True)
        self.assertEquals(len(platforms), 1)
        self.assertEquals(platforms[0]['name'], 'myplatform')


    def _check_hostnames(self, hosts, expected_hostnames):
        self.assertEquals(set(host['hostname'] for host in hosts),
                          set(expected_hostnames))


    def test_ping_db(self):
        self.assertEquals(rpc_interface.ping_db(), [True])


    def test_get_hosts_by_attribute(self):
        host1 = models.Host.objects.create(hostname='test_host1')
        host1.set_attribute('test_attribute1', 'test_value1')
        host2 = models.Host.objects.create(hostname='test_host2')
        host2.set_attribute('test_attribute1', 'test_value1')

        hosts = rpc_interface.get_hosts_by_attribute(
                'test_attribute1', 'test_value1')
        self.assertEquals(set(hosts),
                          set(['test_host1', 'test_host2']))


    def test_get_host_attribute(self):
        host1 = models.Host.objects.create(hostname='test_host1')
        host1.set_attribute('test_attribute1', 'test_value1')
        host2 = models.Host.objects.create(hostname='test_host2')
        host2.set_attribute('test_attribute1', 'test_value1')

        attributes = rpc_interface.get_host_attribute(
                'test_attribute1',
                hostname__in=['test_host1', 'test_host2'])
        hosts = [attr['host'] for attr in attributes]
        values = [attr['value'] for attr in attributes]
        self.assertEquals(set(hosts),
                          set(['test_host1', 'test_host2']))
        self.assertEquals(set(values), set(['test_value1']))


    def test_get_hosts(self):
        hosts = rpc_interface.get_hosts()
        self._check_hostnames(hosts, [host.hostname for host in self.hosts])

        hosts = rpc_interface.get_hosts(hostname='host1')
        self._check_hostnames(hosts, ['host1'])
        host = hosts[0]
        self.assertEquals(sorted(host['labels']), ['label1', 'myplatform'])
        self.assertEquals(host['platform'], 'myplatform')
        self.assertEquals(host['acls'], ['my_acl'])
        self.assertEquals(host['attributes'], {})


    def test_get_hosts_multiple_labels(self):
        hosts = rpc_interface.get_hosts(
                multiple_labels=['myplatform', 'label1'])
        self._check_hostnames(hosts, ['host1'])


    def test_job_keyvals(self):
        keyval_dict = {'mykey': 'myvalue'}
        job_id = rpc_interface.create_job(name='test',
                                          priority=priorities.Priority.DEFAULT,
                                          control_file='foo',
                                          control_type=CLIENT,
                                          hosts=['host1'],
                                          keyvals=keyval_dict)
        jobs = rpc_interface.get_jobs(id=job_id)
        self.assertEquals(len(jobs), 1)
        self.assertEquals(jobs[0]['keyvals'], keyval_dict)


    def test_test_retry(self):
        job_id = rpc_interface.create_job(name='flake',
                                          priority=priorities.Priority.DEFAULT,
                                          control_file='foo',
                                          control_type=CLIENT,
                                          hosts=['host1'],
                                          test_retry=10)
        jobs = rpc_interface.get_jobs(id=job_id)
        self.assertEquals(len(jobs), 1)
        self.assertEquals(jobs[0]['test_retry'], 10)


    def test_get_jobs_summary(self):
        job = self._create_job(hosts=xrange(1, 4))
        entries = list(job.hostqueueentry_set.all())
        entries[1].status = _hqe_status.FAILED
        entries[1].save()
        entries[2].status = _hqe_status.FAILED
        entries[2].aborted = True
        entries[2].save()

        # Mock up tko_rpc_interface.get_status_counts.
        self.god.stub_function_to_return(rpc_interface.tko_rpc_interface,
                                         'get_status_counts',
                                         None)

        job_summaries = rpc_interface.get_jobs_summary(id=job.id)
        self.assertEquals(len(job_summaries), 1)
        summary = job_summaries[0]
        self.assertEquals(summary['status_counts'], {'Queued': 1,
                                                     'Failed': 2})


    def _check_job_ids(self, actual_job_dicts, expected_jobs):
        self.assertEquals(
                set(job_dict['id'] for job_dict in actual_job_dicts),
                set(job.id for job in expected_jobs))


    def test_get_jobs_status_filters(self):
        HqeStatus = models.HostQueueEntry.Status
        def create_two_host_job():
            return self._create_job(hosts=[1, 2])
        def set_hqe_statuses(job, first_status, second_status):
            entries = job.hostqueueentry_set.all()
            entries[0].update_object(status=first_status)
            entries[1].update_object(status=second_status)

        queued = create_two_host_job()

        queued_and_running = create_two_host_job()
        set_hqe_statuses(queued_and_running, HqeStatus.QUEUED,
                           HqeStatus.RUNNING)

        running_and_complete = create_two_host_job()
        set_hqe_statuses(running_and_complete, HqeStatus.RUNNING,
                           HqeStatus.COMPLETED)

        complete = create_two_host_job()
        set_hqe_statuses(complete, HqeStatus.COMPLETED, HqeStatus.COMPLETED)

        started_but_inactive = create_two_host_job()
        set_hqe_statuses(started_but_inactive, HqeStatus.QUEUED,
                           HqeStatus.COMPLETED)

        parsing = create_two_host_job()
        set_hqe_statuses(parsing, HqeStatus.PARSING, HqeStatus.PARSING)

        self._check_job_ids(rpc_interface.get_jobs(not_yet_run=True), [queued])
        self._check_job_ids(rpc_interface.get_jobs(running=True),
                      [queued_and_running, running_and_complete,
                       started_but_inactive, parsing])
        self._check_job_ids(rpc_interface.get_jobs(finished=True), [complete])


    def test_get_jobs_type_filters(self):
        self.assertRaises(AssertionError, rpc_interface.get_jobs,
                          suite=True, sub=True)
        self.assertRaises(AssertionError, rpc_interface.get_jobs,
                          suite=True, standalone=True)
        self.assertRaises(AssertionError, rpc_interface.get_jobs,
                          standalone=True, sub=True)

        parent_job = self._create_job(hosts=[1])
        child_jobs = self._create_job(hosts=[1, 2],
                                      parent_job_id=parent_job.id)
        standalone_job = self._create_job(hosts=[1])

        self._check_job_ids(rpc_interface.get_jobs(suite=True), [parent_job])
        self._check_job_ids(rpc_interface.get_jobs(sub=True), [child_jobs])
        self._check_job_ids(rpc_interface.get_jobs(standalone=True),
                            [standalone_job])


    def _create_job_helper(self, **kwargs):
        return rpc_interface.create_job(name='test',
                                        priority=priorities.Priority.DEFAULT,
                                        control_file='control file',
                                        control_type=SERVER, **kwargs)


    def test_one_time_hosts(self):
        job = self._create_job_helper(one_time_hosts=['testhost'])
        host = models.Host.objects.get(hostname='testhost')
        self.assertEquals(host.invalid, True)
        self.assertEquals(host.labels.count(), 0)
        self.assertEquals(host.aclgroup_set.count(), 0)


    def test_create_job_duplicate_hosts(self):
        self.assertRaises(model_logic.ValidationError, self._create_job_helper,
                          hosts=[1, 1])


    def test_create_unrunnable_metahost_job(self):
        self.assertRaises(error.NoEligibleHostException,
                          self._create_job_helper, meta_hosts=['unused'])


    def test_create_hostless_job(self):
        job_id = self._create_job_helper(hostless=True)
        job = models.Job.objects.get(pk=job_id)
        queue_entries = job.hostqueueentry_set.all()
        self.assertEquals(len(queue_entries), 1)
        self.assertEquals(queue_entries[0].host, None)
        self.assertEquals(queue_entries[0].meta_host, None)


    def _setup_special_tasks(self):
        host = self.hosts[0]

        job1 = self._create_job(hosts=[1])
        job2 = self._create_job(hosts=[1])

        entry1 = job1.hostqueueentry_set.all()[0]
        entry1.update_object(started_on=datetime.datetime(2009, 1, 2),
                             execution_subdir='host1')
        entry2 = job2.hostqueueentry_set.all()[0]
        entry2.update_object(started_on=datetime.datetime(2009, 1, 3),
                             execution_subdir='host1')

        self.task1 = models.SpecialTask.objects.create(
                host=host, task=models.SpecialTask.Task.VERIFY,
                time_started=datetime.datetime(2009, 1, 1), # ran before job 1
                is_complete=True, requested_by=models.User.current_user())
        self.task2 = models.SpecialTask.objects.create(
                host=host, task=models.SpecialTask.Task.VERIFY,
                queue_entry=entry2, # ran with job 2
                is_active=True, requested_by=models.User.current_user())
        self.task3 = models.SpecialTask.objects.create(
                host=host, task=models.SpecialTask.Task.VERIFY,
                requested_by=models.User.current_user()) # not yet run


    def test_get_special_tasks(self):
        self._setup_special_tasks()
        tasks = rpc_interface.get_special_tasks(host__hostname='host1',
                                                queue_entry__isnull=True)
        self.assertEquals(len(tasks), 2)
        self.assertEquals(tasks[0]['task'], models.SpecialTask.Task.VERIFY)
        self.assertEquals(tasks[0]['is_active'], False)
        self.assertEquals(tasks[0]['is_complete'], True)


    def test_get_latest_special_task(self):
        # a particular usage of get_special_tasks()
        self._setup_special_tasks()
        self.task2.time_started = datetime.datetime(2009, 1, 2)
        self.task2.save()

        tasks = rpc_interface.get_special_tasks(
                host__hostname='host1', task=models.SpecialTask.Task.VERIFY,
                time_started__isnull=False, sort_by=['-time_started'],
                query_limit=1)
        self.assertEquals(len(tasks), 1)
        self.assertEquals(tasks[0]['id'], 2)


    def _common_entry_check(self, entry_dict):
        self.assertEquals(entry_dict['host']['hostname'], 'host1')
        self.assertEquals(entry_dict['job']['id'], 2)


    def test_get_host_queue_entries_and_special_tasks(self):
        self._setup_special_tasks()

        host = self.hosts[0].id
        entries_and_tasks = (
                rpc_interface.get_host_queue_entries_and_special_tasks(host))

        paths = [entry['execution_path'] for entry in entries_and_tasks]
        self.assertEquals(paths, ['hosts/host1/3-verify',
                                  '2-autotest_system/host1',
                                  'hosts/host1/2-verify',
                                  '1-autotest_system/host1',
                                  'hosts/host1/1-verify'])

        verify2 = entries_and_tasks[2]
        self._common_entry_check(verify2)
        self.assertEquals(verify2['type'], 'Verify')
        self.assertEquals(verify2['status'], 'Running')
        self.assertEquals(verify2['execution_path'], 'hosts/host1/2-verify')

        entry2 = entries_and_tasks[1]
        self._common_entry_check(entry2)
        self.assertEquals(entry2['type'], 'Job')
        self.assertEquals(entry2['status'], 'Queued')
        self.assertEquals(entry2['started_on'], '2009-01-03 00:00:00')


    def _create_hqes_and_start_time_index_entries(self):
        shard = models.Shard.objects.create(hostname='shard')
        job = self._create_job(shard=shard, control_file='foo')
        HqeStatus = models.HostQueueEntry.Status

        models.HostQueueEntry(
            id=1, job=job, started_on='2017-01-01',
            status=HqeStatus.QUEUED).save()
        models.HostQueueEntry(
            id=2, job=job, started_on='2017-01-02',
            status=HqeStatus.QUEUED).save()
        models.HostQueueEntry(
            id=3, job=job, started_on='2017-01-03',
            status=HqeStatus.QUEUED).save()

        models.HostQueueEntryStartTimes(
            insert_time='2017-01-03', highest_hqe_id=3).save()
        models.HostQueueEntryStartTimes(
            insert_time='2017-01-02', highest_hqe_id=2).save()
        models.HostQueueEntryStartTimes(
            insert_time='2017-01-01', highest_hqe_id=1).save()

    def test_get_host_queue_entries_by_insert_time(self):
        """Check the insert_time_after and insert_time_before constraints."""
        self._create_hqes_and_start_time_index_entries()
        hqes = rpc_interface.get_host_queue_entries_by_insert_time(
            insert_time_after='2017-01-01')
        self.assertEquals(len(hqes), 3)

        hqes = rpc_interface.get_host_queue_entries_by_insert_time(
            insert_time_after='2017-01-02')
        self.assertEquals(len(hqes), 2)

        hqes = rpc_interface.get_host_queue_entries_by_insert_time(
            insert_time_after='2017-01-03')
        self.assertEquals(len(hqes), 1)

        hqes = rpc_interface.get_host_queue_entries_by_insert_time(
            insert_time_before='2017-01-01')
        self.assertEquals(len(hqes), 1)

        hqes = rpc_interface.get_host_queue_entries_by_insert_time(
            insert_time_before='2017-01-02')
        self.assertEquals(len(hqes), 2)

        hqes = rpc_interface.get_host_queue_entries_by_insert_time(
            insert_time_before='2017-01-03')
        self.assertEquals(len(hqes), 3)


    def test_get_host_queue_entries_by_insert_time_with_missing_index_row(self):
        """Shows that the constraints are approximate.

        The query may return rows which are actually outside of the bounds
        given, if the index table does not have an entry for the specific time.
        """
        self._create_hqes_and_start_time_index_entries()
        hqes = rpc_interface.get_host_queue_entries_by_insert_time(
            insert_time_before='2016-12-01')
        self.assertEquals(len(hqes), 1)

    def test_get_hqe_by_insert_time_with_before_and_after(self):
        self._create_hqes_and_start_time_index_entries()
        hqes = rpc_interface.get_host_queue_entries_by_insert_time(
            insert_time_before='2017-01-02',
            insert_time_after='2017-01-02')
        self.assertEquals(len(hqes), 1)

    def test_get_hqe_by_insert_time_and_id_constraint(self):
        self._create_hqes_and_start_time_index_entries()
        # The time constraint is looser than the id constraint, so the time
        # constraint should take precedence.
        hqes = rpc_interface.get_host_queue_entries_by_insert_time(
            insert_time_before='2017-01-02',
            id__lte=1)
        self.assertEquals(len(hqes), 1)

        # Now make the time constraint tighter than the id constraint.
        hqes = rpc_interface.get_host_queue_entries_by_insert_time(
            insert_time_before='2017-01-01',
            id__lte=42)
        self.assertEquals(len(hqes), 1)

    def test_view_invalid_host(self):
        # RPCs used by View Host page should work for invalid hosts
        self._create_job_helper(hosts=[1])
        host = self.hosts[0]
        host.delete()

        self.assertEquals(1, rpc_interface.get_num_hosts(hostname='host1',
                                                         valid_only=False))
        data = rpc_interface.get_hosts(hostname='host1', valid_only=False)
        self.assertEquals(1, len(data))

        self.assertEquals(1, rpc_interface.get_num_host_queue_entries(
                host__hostname='host1'))
        data = rpc_interface.get_host_queue_entries(host__hostname='host1')
        self.assertEquals(1, len(data))

        count = rpc_interface.get_num_host_queue_entries_and_special_tasks(
                host=host.id)
        self.assertEquals(1, count)
        data = rpc_interface.get_host_queue_entries_and_special_tasks(
                host=host.id)
        self.assertEquals(1, len(data))


    def test_reverify_hosts(self):
        hostname_list = rpc_interface.reverify_hosts(id__in=[1, 2])
        self.assertEquals(hostname_list, ['host1', 'host2'])
        tasks = rpc_interface.get_special_tasks()
        self.assertEquals(len(tasks), 2)
        self.assertEquals(set(task['host']['id'] for task in tasks),
                          set([1, 2]))

        task = tasks[0]
        self.assertEquals(task['task'], models.SpecialTask.Task.VERIFY)
        self.assertEquals(task['requested_by'], 'autotest_system')


    def test_repair_hosts(self):
        hostname_list = rpc_interface.repair_hosts(id__in=[1, 2])
        self.assertEquals(hostname_list, ['host1', 'host2'])
        tasks = rpc_interface.get_special_tasks()
        self.assertEquals(len(tasks), 2)
        self.assertEquals(set(task['host']['id'] for task in tasks),
                          set([1, 2]))

        task = tasks[0]
        self.assertEquals(task['task'], models.SpecialTask.Task.REPAIR)
        self.assertEquals(task['requested_by'], 'autotest_system')


    def _modify_host_helper(self, on_shard=False, host_on_shard=False):
        shard_hostname = 'shard1'
        if on_shard:
            global_config.global_config.override_config_value(
                'SHARD', 'shard_hostname', shard_hostname)

        host = models.Host.objects.all()[0]
        if host_on_shard:
            shard = models.Shard.objects.create(hostname=shard_hostname)
            host.shard = shard
            host.save()

        self.assertFalse(host.locked)

        self.god.stub_class_method(frontend.AFE, 'run')

        if host_on_shard and not on_shard:
            mock_afe = self.god.create_mock_class_obj(
                    frontend_wrappers.RetryingAFE, 'MockAFE')
            self.god.stub_with(frontend_wrappers, 'RetryingAFE', mock_afe)

            mock_afe2 = frontend_wrappers.RetryingAFE.expect_new(
                    server=shard_hostname, user=None)
            mock_afe2.run.expect_call('modify_host_local', id=host.id,
                    locked=True, lock_reason='_modify_host_helper lock',
                    lock_time=datetime.datetime(2015, 12, 15))
        elif on_shard:
            mock_afe = self.god.create_mock_class_obj(
                    frontend_wrappers.RetryingAFE, 'MockAFE')
            self.god.stub_with(frontend_wrappers, 'RetryingAFE', mock_afe)

            mock_afe2 = frontend_wrappers.RetryingAFE.expect_new(
                    server=server_utils.get_global_afe_hostname(), user=None)
            mock_afe2.run.expect_call('modify_host', id=host.id,
                    locked=True, lock_reason='_modify_host_helper lock',
                    lock_time=datetime.datetime(2015, 12, 15))

        rpc_interface.modify_host(id=host.id, locked=True,
                                  lock_reason='_modify_host_helper lock',
                                  lock_time=datetime.datetime(2015, 12, 15))

        host = models.Host.objects.get(pk=host.id)
        if on_shard:
            # modify_host on shard does nothing but routing the RPC to master.
            self.assertFalse(host.locked)
        else:
            self.assertTrue(host.locked)
        self.god.check_playback()


    def test_modify_host_on_master_host_on_master(self):
        """Call modify_host to master for host in master."""
        self._modify_host_helper()


    def test_modify_host_on_master_host_on_shard(self):
        """Call modify_host to master for host in shard."""
        self._modify_host_helper(host_on_shard=True)


    def test_modify_host_on_shard(self):
        """Call modify_host to shard for host in shard."""
        self._modify_host_helper(on_shard=True, host_on_shard=True)


    def test_modify_hosts_on_master_host_on_shard(self):
        """Ensure calls to modify_hosts are correctly forwarded to shards."""
        host1 = models.Host.objects.all()[0]
        host2 = models.Host.objects.all()[1]

        shard1 = models.Shard.objects.create(hostname='shard1')
        host1.shard = shard1
        host1.save()

        shard2 = models.Shard.objects.create(hostname='shard2')
        host2.shard = shard2
        host2.save()

        self.assertFalse(host1.locked)
        self.assertFalse(host2.locked)

        mock_afe = self.god.create_mock_class_obj(frontend_wrappers.RetryingAFE,
                                                  'MockAFE')
        self.god.stub_with(frontend_wrappers, 'RetryingAFE', mock_afe)

        # The statuses of one host might differ on master and shard.
        # Filters are always applied on the master. So the host on the shard
        # will be affected no matter what his status is.
        filters_to_use = {'status': 'Ready'}

        mock_afe2 = frontend_wrappers.RetryingAFE.expect_new(
                server='shard2', user=None)
        mock_afe2.run.expect_call(
            'modify_hosts_local',
            host_filter_data={'id__in': [shard1.id, shard2.id]},
            update_data={'locked': True,
                         'lock_reason': 'Testing forward to shard',
                         'lock_time' : datetime.datetime(2015, 12, 15) })

        mock_afe1 = frontend_wrappers.RetryingAFE.expect_new(
                server='shard1', user=None)
        mock_afe1.run.expect_call(
            'modify_hosts_local',
            host_filter_data={'id__in': [shard1.id, shard2.id]},
            update_data={'locked': True,
                         'lock_reason': 'Testing forward to shard',
                         'lock_time' : datetime.datetime(2015, 12, 15)})

        rpc_interface.modify_hosts(
                host_filter_data={'status': 'Ready'},
                update_data={'locked': True,
                             'lock_reason': 'Testing forward to shard',
                             'lock_time' : datetime.datetime(2015, 12, 15) })

        host1 = models.Host.objects.get(pk=host1.id)
        self.assertTrue(host1.locked)
        host2 = models.Host.objects.get(pk=host2.id)
        self.assertTrue(host2.locked)
        self.god.check_playback()


    def test_delete_host(self):
        """Ensure an RPC is made on delete a host, if it is on a shard."""
        host1 = models.Host.objects.all()[0]
        shard1 = models.Shard.objects.create(hostname='shard1')
        host1.shard = shard1
        host1.save()
        host1_id = host1.id

        mock_afe = self.god.create_mock_class_obj(frontend_wrappers.RetryingAFE,
                                                 'MockAFE')
        self.god.stub_with(frontend_wrappers, 'RetryingAFE', mock_afe)

        mock_afe1 = frontend_wrappers.RetryingAFE.expect_new(
                server='shard1', user=None)
        mock_afe1.run.expect_call('delete_host', id=host1.id)

        rpc_interface.delete_host(id=host1.id)

        self.assertRaises(models.Host.DoesNotExist,
                          models.Host.smart_get, host1_id)

        self.god.check_playback()


    def test_modify_label(self):
        label1 = models.Label.objects.all()[0]
        self.assertEqual(label1.invalid, 0)

        host2 = models.Host.objects.all()[1]
        shard1 = models.Shard.objects.create(hostname='shard1')
        host2.shard = shard1
        host2.labels.add(label1)
        host2.save()

        mock_afe = self.god.create_mock_class_obj(frontend_wrappers.RetryingAFE,
                                                  'MockAFE')
        self.god.stub_with(frontend_wrappers, 'RetryingAFE', mock_afe)

        mock_afe1 = frontend_wrappers.RetryingAFE.expect_new(
                server='shard1', user=None)
        mock_afe1.run.expect_call('modify_label', id=label1.id, invalid=1)

        rpc_interface.modify_label(label1.id, invalid=1)

        self.assertEqual(models.Label.objects.all()[0].invalid, 1)
        self.god.check_playback()


    def test_delete_label(self):
        label1 = models.Label.objects.all()[0]

        host2 = models.Host.objects.all()[1]
        shard1 = models.Shard.objects.create(hostname='shard1')
        host2.shard = shard1
        host2.labels.add(label1)
        host2.save()

        mock_afe = self.god.create_mock_class_obj(frontend_wrappers.RetryingAFE,
                                                  'MockAFE')
        self.god.stub_with(frontend_wrappers, 'RetryingAFE', mock_afe)

        mock_afe1 = frontend_wrappers.RetryingAFE.expect_new(
                server='shard1', user=None)
        mock_afe1.run.expect_call('delete_label', id=label1.id)

        rpc_interface.delete_label(id=label1.id)

        self.assertRaises(models.Label.DoesNotExist,
                          models.Label.smart_get, label1.id)
        self.god.check_playback()


    def test_get_image_for_job_with_keyval_build(self):
        keyval_dict = {'build': 'cool-image'}
        job_id = rpc_interface.create_job(name='test',
                                          priority=priorities.Priority.DEFAULT,
                                          control_file='foo',
                                          control_type=CLIENT,
                                          hosts=['host1'],
                                          keyvals=keyval_dict)
        job = models.Job.objects.get(id=job_id)
        self.assertIsNotNone(job)
        image = rpc_interface._get_image_for_job(job, True)
        self.assertEquals('cool-image', image)


    def test_get_image_for_job_with_keyval_builds(self):
        keyval_dict = {'builds': {'cros-version': 'cool-image'}}
        job_id = rpc_interface.create_job(name='test',
                                          priority=priorities.Priority.DEFAULT,
                                          control_file='foo',
                                          control_type=CLIENT,
                                          hosts=['host1'],
                                          keyvals=keyval_dict)
        job = models.Job.objects.get(id=job_id)
        self.assertIsNotNone(job)
        image = rpc_interface._get_image_for_job(job, True)
        self.assertEquals('cool-image', image)


    def test_get_image_for_job_with_control_build(self):
        CONTROL_FILE = """build='cool-image'
        """
        job_id = rpc_interface.create_job(name='test',
                                          priority=priorities.Priority.DEFAULT,
                                          control_file='foo',
                                          control_type=CLIENT,
                                          hosts=['host1'])
        job = models.Job.objects.get(id=job_id)
        self.assertIsNotNone(job)
        job.control_file = CONTROL_FILE
        image = rpc_interface._get_image_for_job(job, True)
        self.assertEquals('cool-image', image)


    def test_get_image_for_job_with_control_builds(self):
        CONTROL_FILE = """builds={'cros-version': 'cool-image'}
        """
        job_id = rpc_interface.create_job(name='test',
                                          priority=priorities.Priority.DEFAULT,
                                          control_file='foo',
                                          control_type=CLIENT,
                                          hosts=['host1'])
        job = models.Job.objects.get(id=job_id)
        self.assertIsNotNone(job)
        job.control_file = CONTROL_FILE
        image = rpc_interface._get_image_for_job(job, True)
        self.assertEquals('cool-image', image)


class ExtraRpcInterfaceTest(frontend_test_utils.FrontendTestMixin,
                            ShardHeartbeatTest):
    """Unit tests for functions originally in site_rpc_interface.py.

    @var _NAME: fake suite name.
    @var _BOARD: fake board to reimage.
    @var _BUILD: fake build with which to reimage.
    @var _PRIORITY: fake priority with which to reimage.
    """
    _NAME = 'name'
    _BOARD = 'link'
    _BUILD = 'link-release/R36-5812.0.0'
    _BUILDS = {provision.CROS_VERSION_PREFIX: _BUILD}
    _PRIORITY = priorities.Priority.DEFAULT
    _TIMEOUT = 24


    def setUp(self):
        super(ExtraRpcInterfaceTest, self).setUp()
        self._SUITE_NAME = rpc_interface.canonicalize_suite_name(
            self._NAME)
        self.dev_server = self.mox.CreateMock(dev_server.ImageServer)
        self._frontend_common_setup(fill_data=False)


    def tearDown(self):
        self._frontend_common_teardown()


    def _setupDevserver(self):
        self.mox.StubOutClassWithMocks(dev_server, 'ImageServer')
        dev_server.resolve(self._BUILD).AndReturn(self.dev_server)


    def _mockDevServerGetter(self, get_control_file=True):
        self._setupDevserver()
        if get_control_file:
          self.getter = self.mox.CreateMock(
              control_file_getter.DevServerGetter)
          self.mox.StubOutWithMock(control_file_getter.DevServerGetter,
                                   'create')
          control_file_getter.DevServerGetter.create(
              mox.IgnoreArg(), mox.IgnoreArg()).AndReturn(self.getter)


    def _mockRpcUtils(self, to_return, control_file_substring=''):
        """Fake out the autotest rpc_utils module with a mockable class.

        @param to_return: the value that rpc_utils.create_job_common() should
                          be mocked out to return.
        @param control_file_substring: A substring that is expected to appear
                                       in the control file output string that
                                       is passed to create_job_common.
                                       Default: ''
        """
        download_started_time = constants.DOWNLOAD_STARTED_TIME
        payload_finished_time = constants.PAYLOAD_FINISHED_TIME
        self.mox.StubOutWithMock(rpc_utils, 'create_job_common')
        rpc_utils.create_job_common(mox.And(mox.StrContains(self._NAME),
                                    mox.StrContains(self._BUILD)),
                            priority=self._PRIORITY,
                            timeout_mins=self._TIMEOUT*60,
                            max_runtime_mins=self._TIMEOUT*60,
                            control_type='Server',
                            control_file=mox.And(mox.StrContains(self._BOARD),
                                                 mox.StrContains(self._BUILD),
                                                 mox.StrContains(
                                                     control_file_substring)),
                            hostless=True,
                            keyvals=mox.And(mox.In(download_started_time),
                                            mox.In(payload_finished_time))
                            ).AndReturn(to_return)


    def testStageBuildFail(self):
        """Ensure that a failure to stage the desired build fails the RPC."""
        self._setupDevserver()

        self.dev_server.hostname = 'mox_url'
        self.dev_server.stage_artifacts(
                image=self._BUILD, artifacts=['test_suites']).AndRaise(
                dev_server.DevServerException())
        self.mox.ReplayAll()
        self.assertRaises(error.StageControlFileFailure,
                          rpc_interface.create_suite_job,
                          name=self._NAME,
                          board=self._BOARD,
                          builds=self._BUILDS,
                          pool=None)


    def testGetControlFileFail(self):
        """Ensure that a failure to get needed control file fails the RPC."""
        self._mockDevServerGetter()

        self.dev_server.hostname = 'mox_url'
        self.dev_server.stage_artifacts(
                image=self._BUILD, artifacts=['test_suites']).AndReturn(True)

        self.getter.get_control_file_contents_by_name(
            self._SUITE_NAME).AndReturn(None)
        self.mox.ReplayAll()
        self.assertRaises(error.ControlFileEmpty,
                          rpc_interface.create_suite_job,
                          name=self._NAME,
                          board=self._BOARD,
                          builds=self._BUILDS,
                          pool=None)


    def testGetControlFileListFail(self):
        """Ensure that a failure to get needed control file fails the RPC."""
        self._mockDevServerGetter()

        self.dev_server.hostname = 'mox_url'
        self.dev_server.stage_artifacts(
                image=self._BUILD, artifacts=['test_suites']).AndReturn(True)

        self.getter.get_control_file_contents_by_name(
            self._SUITE_NAME).AndRaise(error.NoControlFileList())
        self.mox.ReplayAll()
        self.assertRaises(error.NoControlFileList,
                          rpc_interface.create_suite_job,
                          name=self._NAME,
                          board=self._BOARD,
                          builds=self._BUILDS,
                          pool=None)


    def testCreateSuiteJobFail(self):
        """Ensure that failure to schedule the suite job fails the RPC."""
        self._mockDevServerGetter()

        self.dev_server.hostname = 'mox_url'
        self.dev_server.stage_artifacts(
                image=self._BUILD, artifacts=['test_suites']).AndReturn(True)

        self.getter.get_control_file_contents_by_name(
            self._SUITE_NAME).AndReturn('f')

        self.dev_server.url().AndReturn('mox_url')
        self._mockRpcUtils(-1)
        self.mox.ReplayAll()
        self.assertEquals(
            rpc_interface.create_suite_job(name=self._NAME,
                                           board=self._BOARD,
                                           builds=self._BUILDS, pool=None),
            -1)


    def testCreateSuiteJobSuccess(self):
        """Ensures that success results in a successful RPC."""
        self._mockDevServerGetter()

        self.dev_server.hostname = 'mox_url'
        self.dev_server.stage_artifacts(
                image=self._BUILD, artifacts=['test_suites']).AndReturn(True)

        self.getter.get_control_file_contents_by_name(
            self._SUITE_NAME).AndReturn('f')

        self.dev_server.url().AndReturn('mox_url')
        job_id = 5
        self._mockRpcUtils(job_id)
        self.mox.ReplayAll()
        self.assertEquals(
            rpc_interface.create_suite_job(name=self._NAME,
                                           board=self._BOARD,
                                           builds=self._BUILDS,
                                           pool=None),
            job_id)


    def testCreateSuiteJobNoHostCheckSuccess(self):
        """Ensures that success results in a successful RPC."""
        self._mockDevServerGetter()

        self.dev_server.hostname = 'mox_url'
        self.dev_server.stage_artifacts(
                image=self._BUILD, artifacts=['test_suites']).AndReturn(True)

        self.getter.get_control_file_contents_by_name(
            self._SUITE_NAME).AndReturn('f')

        self.dev_server.url().AndReturn('mox_url')
        job_id = 5
        self._mockRpcUtils(job_id)
        self.mox.ReplayAll()
        self.assertEquals(
          rpc_interface.create_suite_job(name=self._NAME,
                                         board=self._BOARD,
                                         builds=self._BUILDS,
                                         pool=None, check_hosts=False),
          job_id)


    def testCreateSuiteJobControlFileSupplied(self):
        """Ensure we can supply the control file to create_suite_job."""
        self._mockDevServerGetter(get_control_file=False)

        self.dev_server.hostname = 'mox_url'
        self.dev_server.stage_artifacts(
                image=self._BUILD, artifacts=['test_suites']).AndReturn(True)
        self.dev_server.url().AndReturn('mox_url')
        job_id = 5
        self._mockRpcUtils(job_id)
        self.mox.ReplayAll()
        self.assertEquals(
            rpc_interface.create_suite_job(name='%s/%s' % (self._NAME,
                                                           self._BUILD),
                                           board=None,
                                           builds=self._BUILDS,
                                           pool=None,
                                           control_file='CONTROL FILE'),
            job_id)


    def _get_records_for_sending_to_master(self):
        return [{'control_file': 'foo',
                 'control_type': 1,
                 'created_on': datetime.datetime(2014, 8, 21),
                 'drone_set': None,
                 'email_list': '',
                 'max_runtime_hrs': 72,
                 'max_runtime_mins': 1440,
                 'name': 'dummy',
                 'owner': 'autotest_system',
                 'parse_failed_repair': True,
                 'priority': 40,
                 'reboot_after': 0,
                 'reboot_before': 1,
                 'run_reset': True,
                 'run_verify': False,
                 'synch_count': 0,
                 'test_retry': 10,
                 'timeout': 24,
                 'timeout_mins': 1440,
                 'id': 1
                 }], [{
                    'aborted': False,
                    'active': False,
                    'complete': False,
                    'deleted': False,
                    'execution_subdir': '',
                    'finished_on': None,
                    'started_on': None,
                    'status': 'Queued',
                    'id': 1
                }]


    def _send_records_to_master_helper(
        self, jobs, hqes, shard_hostname='host1',
        exception_to_throw=error.UnallowedRecordsSentToMaster, aborted=False):
        job_id = rpc_interface.create_job(
                name='dummy',
                priority=self._PRIORITY,
                control_file='foo',
                control_type=SERVER,
                test_retry=10, hostless=True)
        job = models.Job.objects.get(pk=job_id)
        shard = models.Shard.objects.create(hostname='host1')
        job.shard = shard
        job.save()

        if aborted:
            job.hostqueueentry_set.update(aborted=True)
            job.shard = None
            job.save()

        hqe = job.hostqueueentry_set.all()[0]
        if not exception_to_throw:
            self._do_heartbeat_and_assert_response(
                shard_hostname=shard_hostname,
                upload_jobs=jobs, upload_hqes=hqes)
        else:
            self.assertRaises(
                exception_to_throw,
                self._do_heartbeat_and_assert_response,
                shard_hostname=shard_hostname,
                upload_jobs=jobs, upload_hqes=hqes)


    def testSendingRecordsToMaster(self):
        """Send records to the master and ensure they are persisted."""
        jobs, hqes = self._get_records_for_sending_to_master()
        hqes[0]['status'] = 'Completed'
        self._send_records_to_master_helper(
            jobs=jobs, hqes=hqes, exception_to_throw=None)

        # Check the entry was actually written to db
        self.assertEqual(models.HostQueueEntry.objects.all()[0].status,
                         'Completed')


    def testSendingRecordsToMasterAbortedOnMaster(self):
        """Send records to the master and ensure they are persisted."""
        jobs, hqes = self._get_records_for_sending_to_master()
        hqes[0]['status'] = 'Completed'
        self._send_records_to_master_helper(
            jobs=jobs, hqes=hqes, exception_to_throw=None, aborted=True)

        # Check the entry was actually written to db
        self.assertEqual(models.HostQueueEntry.objects.all()[0].status,
                         'Completed')


    def testSendingRecordsToMasterJobAssignedToDifferentShard(self):
        """Ensure records belonging to different shard are silently rejected."""
        shard1 = models.Shard.objects.create(hostname='shard1')
        shard2 = models.Shard.objects.create(hostname='shard2')
        job1 = self._create_job(shard=shard1, control_file='foo1')
        job2 = self._create_job(shard=shard2, control_file='foo2')
        job1_id = job1.id
        job2_id = job2.id
        hqe1 = models.HostQueueEntry.objects.create(job=job1)
        hqe2 = models.HostQueueEntry.objects.create(job=job2)
        hqe1_id = hqe1.id
        hqe2_id = hqe2.id
        job1_record = job1.serialize(include_dependencies=False)
        job2_record = job2.serialize(include_dependencies=False)
        hqe1_record = hqe1.serialize(include_dependencies=False)
        hqe2_record = hqe2.serialize(include_dependencies=False)

        # Prepare a bogus job record update from the wrong shard. The update
        # should not throw an exception. Non-bogus jobs in the same update
        # should happily update.
        job1_record.update({'control_file': 'bar1'})
        job2_record.update({'control_file': 'bar2'})
        hqe1_record.update({'status': 'Aborted'})
        hqe2_record.update({'status': 'Aborted'})
        self._do_heartbeat_and_assert_response(
            shard_hostname='shard2', upload_jobs=[job1_record, job2_record],
            upload_hqes=[hqe1_record, hqe2_record])

        # Job and HQE record for wrong job should not be modified, because the
        # rpc came from the wrong shard. Job and HQE record for valid job are
        # modified.
        self.assertEqual(models.Job.objects.get(id=job1_id).control_file,
                         'foo1')
        self.assertEqual(models.Job.objects.get(id=job2_id).control_file,
                         'bar2')
        self.assertEqual(models.HostQueueEntry.objects.get(id=hqe1_id).status,
                         '')
        self.assertEqual(models.HostQueueEntry.objects.get(id=hqe2_id).status,
                         'Aborted')


    def testSendingRecordsToMasterNotExistingJob(self):
        """Ensure update for non existing job gets rejected."""
        jobs, hqes = self._get_records_for_sending_to_master()
        jobs[0]['id'] = 3

        self._send_records_to_master_helper(
            jobs=jobs, hqes=hqes)


    def _createShardAndHostWithLabel(self, shard_hostname='shard1',
                                     host_hostname='host1',
                                     label_name='board:lumpy'):
        """Create a label, host, shard, and assign host to shard."""
        try:
            label = models.Label.objects.create(name=label_name)
        except:
            label = models.Label.smart_get(label_name)

        shard = models.Shard.objects.create(hostname=shard_hostname)
        shard.labels.add(label)

        host = models.Host.objects.create(hostname=host_hostname, leased=False,
                                          shard=shard)
        host.labels.add(label)

        return shard, host, label


    def testShardLabelRemovalInvalid(self):
        """Ensure you cannot remove the wrong label from shard."""
        shard1, host1, lumpy_label = self._createShardAndHostWithLabel()
        stumpy_label = models.Label.objects.create(
                name='board:stumpy', platform=True)
        with self.assertRaises(error.RPCException):
            rpc_interface.remove_board_from_shard(
                    shard1.hostname, stumpy_label.name)


    def testShardHeartbeatLabelRemoval(self):
        """Ensure label removal from shard works."""
        shard1, host1, lumpy_label = self._createShardAndHostWithLabel()

        self.assertEqual(host1.shard, shard1)
        self.assertItemsEqual(shard1.labels.all(), [lumpy_label])
        rpc_interface.remove_board_from_shard(
                shard1.hostname, lumpy_label.name)
        host1 = models.Host.smart_get(host1.id)
        shard1 = models.Shard.smart_get(shard1.id)
        self.assertEqual(host1.shard, None)
        self.assertItemsEqual(shard1.labels.all(), [])


    def testCreateListShard(self):
        """Retrieve a list of all shards."""
        lumpy_label = models.Label.objects.create(name='board:lumpy',
                                                  platform=True)
        stumpy_label = models.Label.objects.create(name='board:stumpy',
                                                  platform=True)
        peppy_label = models.Label.objects.create(name='board:peppy',
                                                  platform=True)

        shard_id = rpc_interface.add_shard(
            hostname='host1', labels='board:lumpy,board:stumpy')
        self.assertRaises(error.RPCException,
                          rpc_interface.add_shard,
                          hostname='host1', labels='board:lumpy,board:stumpy')
        self.assertRaises(model_logic.ValidationError,
                          rpc_interface.add_shard,
                          hostname='host1', labels='board:peppy')
        shard = models.Shard.objects.get(pk=shard_id)
        self.assertEqual(shard.hostname, 'host1')
        self.assertEqual(shard.labels.values_list('pk')[0], (lumpy_label.id,))
        self.assertEqual(shard.labels.values_list('pk')[1], (stumpy_label.id,))

        self.assertEqual(rpc_interface.get_shards(),
                         [{'labels': ['board:lumpy','board:stumpy'],
                           'hostname': 'host1',
                           'id': 1}])


    def testAddBoardsToShard(self):
        """Add boards to a given shard."""
        shard1, host1, lumpy_label = self._createShardAndHostWithLabel()
        stumpy_label = models.Label.objects.create(name='board:stumpy',
                                                   platform=True)
        shard_id = rpc_interface.add_board_to_shard(
            hostname='shard1', labels='board:stumpy')
        # Test whether raise exception when board label does not exist.
        self.assertRaises(models.Label.DoesNotExist,
                          rpc_interface.add_board_to_shard,
                          hostname='shard1', labels='board:test')
        # Test whether raise exception when board already sharded.
        self.assertRaises(error.RPCException,
                          rpc_interface.add_board_to_shard,
                          hostname='shard1', labels='board:lumpy')
        shard = models.Shard.objects.get(pk=shard_id)
        self.assertEqual(shard.hostname, 'shard1')
        self.assertEqual(shard.labels.values_list('pk')[0], (lumpy_label.id,))
        self.assertEqual(shard.labels.values_list('pk')[1], (stumpy_label.id,))

        self.assertEqual(rpc_interface.get_shards(),
                         [{'labels': ['board:lumpy','board:stumpy'],
                           'hostname': 'shard1',
                           'id': 1}])


    def testShardHeartbeatFetchHostlessJob(self):
        shard1, host1, label1 = self._createShardAndHostWithLabel()
        self._testShardHeartbeatFetchHostlessJobHelper(host1)


    def testShardHeartbeatIncorrectHosts(self):
        shard1, host1, label1 = self._createShardAndHostWithLabel()
        self._testShardHeartbeatIncorrectHostsHelper(host1)


    def testShardHeartbeatLabelRemovalRace(self):
        shard1, host1, label1 = self._createShardAndHostWithLabel()
        self._testShardHeartbeatLabelRemovalRaceHelper(shard1, host1, label1)


    def testShardRetrieveJobs(self):
        shard1, host1, label1 = self._createShardAndHostWithLabel()
        shard2, host2, label2 = self._createShardAndHostWithLabel(
                'shard2', 'host2', 'board:grumpy')
        self._testShardRetrieveJobsHelper(shard1, host1, label1,
                                          shard2, host2, label2)


    def testResendJobsAfterFailedHeartbeat(self):
        shard1, host1, label1 = self._createShardAndHostWithLabel()
        self._testResendJobsAfterFailedHeartbeatHelper(shard1, host1, label1)


    def testResendHostsAfterFailedHeartbeat(self):
        shard1, host1, label1 = self._createShardAndHostWithLabel()
        self._testResendHostsAfterFailedHeartbeatHelper(host1)


if __name__ == '__main__':
    unittest.main()
