import datetime

import common
from autotest_lib.frontend import setup_test_environment
from autotest_lib.frontend import thread_local
from autotest_lib.frontend.afe import models, model_attributes
from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib.test_utils import mock

class FrontendTestMixin(object):
    # pylint: disable=missing-docstring
    def _fill_in_test_data(self):
        """Populate the test database with some hosts and labels."""
        if models.DroneSet.drone_sets_enabled():
            models.DroneSet.objects.create(
                    name=models.DroneSet.default_drone_set_name())

        acl_group = models.AclGroup.objects.create(name='my_acl')
        acl_group.users.add(models.User.current_user())

        self.hosts = [models.Host.objects.create(hostname=hostname)
                      for hostname in
                      ('host1', 'host2', 'host3', 'host4', 'host5', 'host6',
                       'host7', 'host8', 'host9')]

        acl_group.hosts = self.hosts
        models.AclGroup.smart_get('Everyone').hosts = []

        self.labels = [models.Label.objects.create(name=name) for name in
                       ('label1', 'label2', 'label3',
                        'label6', 'label7', 'unused')]

        platform = models.Label.objects.create(name='myplatform', platform=True)
        for host in self.hosts:
            host.labels.add(platform)

        self.label1, self.label2, self.label3, self.label6, self.label7, _ \
            = self.labels

        self.labels.append(models.Label.objects.create(name='static'))
        self.replaced_labels = [models.ReplacedLabel.objects.create(
                label_id=self.labels[-1].id)]

        self.label3.only_if_needed = True
        self.label3.save()
        self.hosts[0].labels.add(self.label1)
        self.hosts[1].labels.add(self.label2)
        for hostnum in xrange(4,7):  # host5..host7
            self.hosts[hostnum].labels.add(self.label6)
        self.hosts[6].labels.add(self.label7)
        for hostnum in xrange(7,9):  # host8..host9
            self.hosts[hostnum].labels.add(self.label6)
            self.hosts[hostnum].labels.add(self.label7)


    def _frontend_common_setup(self, fill_data=True, setup_tables=True):
        self.god = mock.mock_god(ut=self)
        if setup_tables:
            setup_test_environment.set_up()
        global_config.global_config.override_config_value(
                'SERVER', 'rpc_logging', 'False')
        if fill_data and setup_tables:
            self._fill_in_test_data()


    def _frontend_common_teardown(self):
        setup_test_environment.tear_down()
        thread_local.set_user(None)
        self.god.unstub_all()


    def _set_static_attribute(self, host, attribute, value):
        """Set static attribute for a host.

        It ensures that all static attributes have a corresponding
        entry in afe_host_attributes.
        """
        # Get or create the reference object in afe_host_attributes.
        model, args = host._get_attribute_model_and_args(attribute)
        model.objects.get_or_create(**args)

        attribute_model, get_args = host._get_static_attribute_model_and_args(
            attribute)
        attribute_object, _ = attribute_model.objects.get_or_create(**get_args)
        attribute_object.value = value
        attribute_object.save()


    def _create_job(self, hosts=[], metahosts=[], priority=0, active=False,
                    synchronous=False, hostless=False,
                    drone_set=None, control_file='control',
                    owner='autotest_system', parent_job_id=None,
                    shard=None):
        """
        Create a job row in the test database.

        @param hosts - A list of explicit host ids for this job to be
                scheduled on.
        @param metahosts - A list of label ids for each host that this job
                should be scheduled on (meta host scheduling).
        @param priority - The job priority (integer).
        @param active - bool, mark this job as running or not in the database?
        @param synchronous - bool, if True use synch_count=2 otherwise use
                synch_count=1.
        @param hostless - if True, this job is intended to be hostless (in that
                case, hosts, and metahosts must all be empty)
        @param owner - The owner of the job. Aclgroups from which a job can
                acquire hosts change with the aclgroups of the owners.
        @param parent_job_id - The id of a parent_job. If a job with the id
                doesn't already exist one will be created.
        @param shard - shard object to assign the job to.

        @raises model.DoesNotExist: If parent_job_id is specified but a job with
            id=parent_job_id does not exist.

        @returns A Django frontend.afe.models.Job instance.
        """
        if not drone_set:
            drone_set = (models.DroneSet.default_drone_set_name()
                         and models.DroneSet.get_default())

        synch_count = synchronous and 2 or 1
        created_on = datetime.datetime(2008, 1, 1)
        status = models.HostQueueEntry.Status.QUEUED
        if active:
            status = models.HostQueueEntry.Status.RUNNING

        parent_job = (models.Job.objects.get(id=parent_job_id)
                      if parent_job_id else None)
        job = models.Job.objects.create(
            name='test', owner=owner, priority=priority,
            synch_count=synch_count, created_on=created_on,
            reboot_before=model_attributes.RebootBefore.NEVER,
            drone_set=drone_set, control_file=control_file,
            parent_job=parent_job, require_ssp=None,
            shard=shard)

        # Update the job's dependencies to include the metahost.
        for metahost_label in metahosts:
            dep = models.Label.objects.get(id=metahost_label)
            job.dependency_labels.add(dep)

        for host_id in hosts:
            models.HostQueueEntry.objects.create(job=job, host_id=host_id,
                                                 status=status)
            models.IneligibleHostQueue.objects.create(job=job, host_id=host_id)
        for label_id in metahosts:
            models.HostQueueEntry.objects.create(job=job, meta_host_id=label_id,
                                                 status=status)

        if hostless:
            assert not (hosts or metahosts)
            models.HostQueueEntry.objects.create(job=job, status=status)
        return job


    def _create_job_simple(self, hosts, use_metahost=False,
                           priority=0, active=False, drone_set=None,
                           parent_job_id=None):
        """An alternative interface to _create_job"""
        args = {'hosts' : [], 'metahosts' : []}
        if use_metahost:
            args['metahosts'] = hosts
        else:
            args['hosts'] = hosts
        return self._create_job(
                priority=priority, active=active, drone_set=drone_set,
                parent_job_id=parent_job_id, **args)
