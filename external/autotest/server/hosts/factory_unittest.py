#!/usr/bin/python
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import mock
import unittest

import common
from autotest_lib.client.common_lib import error
from autotest_lib.server.hosts import base_label_unittest, factory
from autotest_lib.server.hosts import host_info


class MockHost(object):
    """Mock host object with no side effects."""
    def __init__(self, hostname, **args):
        self._init_args = args
        self._init_args['hostname'] = hostname


    def job_start(self):
        """Only method called by factory."""
        pass


class MockConnectivity(object):
    """Mock connectivity object with no side effects."""
    def __init__(self, hostname, **args):
        pass


    def close(self):
        """Only method called by factory."""
        pass


def _gen_mock_host(name, check_host=False):
    """Create an identifiable mock host closs.
    """
    return type('mock_host_%s' % name, (MockHost,), {
        '_host_cls_name': name,
        'check_host': staticmethod(lambda host, timeout=None: check_host)
    })


def _gen_mock_conn(name):
    """Create an identifiable mock connectivity class.
    """
    return type('mock_conn_%s' % name, (MockConnectivity,),
                {'_conn_cls_name': name})


def _gen_machine_dict(hostname='localhost', labels=[], attributes={}):
    """Generate a machine dictionary with the specified parameters.

    @param hostname: hostname of machine
    @param labels: list of host labels
    @param attributes: dict of host attributes

    @return: machine dict with mocked AFE Host object and fake AfeStore.
    """
    afe_host = base_label_unittest.MockAFEHost(labels, attributes)
    store = host_info.InMemoryHostInfoStore()
    store.commit(host_info.HostInfo(labels, attributes))
    return {'hostname': hostname,
            'afe_host': afe_host,
            'host_info_store': store}


class CreateHostUnittests(unittest.TestCase):
    """Tests for create_host function."""

    def setUp(self):
        """Prevent use of real Host and connectivity objects due to potential
        side effects.
        """
        self._orig_ssh_engine = factory.SSH_ENGINE
        self._orig_types = factory.host_types
        self._orig_dict = factory.OS_HOST_DICT
        self._orig_cros_host = factory.cros_host.CrosHost
        self._orig_local_host = factory.local_host.LocalHost
        self._orig_ssh_host = factory.ssh_host.SSHHost

        self.host_types = factory.host_types = []
        self.os_host_dict = factory.OS_HOST_DICT = {}
        factory.cros_host.CrosHost = _gen_mock_host('cros_host')
        factory.local_host.LocalHost = _gen_mock_conn('local')
        factory.ssh_host.SSHHost = _gen_mock_conn('ssh')


    def tearDown(self):
        """Clean up mocks."""
        factory.SSH_ENGINE = self._orig_ssh_engine
        factory.host_types = self._orig_types
        factory.OS_HOST_DICT = self._orig_dict
        factory.cros_host.CrosHost = self._orig_cros_host
        factory.local_host.LocalHost = self._orig_local_host
        factory.ssh_host.SSHHost = self._orig_ssh_host


    def test_use_specified(self):
        """Confirm that the specified host and connectivity classes are used."""
        machine = _gen_machine_dict()
        host_obj = factory.create_host(
                machine,
                _gen_mock_host('specified'),
                _gen_mock_conn('specified')
        )
        self.assertEqual(host_obj._host_cls_name, 'specified')
        self.assertEqual(host_obj._conn_cls_name, 'specified')


    def test_detect_host_by_os_label(self):
        """Confirm that the host object is selected by the os label.
        """
        machine = _gen_machine_dict(labels=['os:foo'])
        self.os_host_dict['foo'] = _gen_mock_host('foo')
        host_obj = factory.create_host(machine)
        self.assertEqual(host_obj._host_cls_name, 'foo')


    def test_detect_host_by_os_type_attribute(self):
        """Confirm that the host object is selected by the os_type attribute
        and that the os_type attribute is preferred over the os label.
        """
        machine = _gen_machine_dict(labels=['os:foo'],
                                         attributes={'os_type': 'bar'})
        self.os_host_dict['foo'] = _gen_mock_host('foo')
        self.os_host_dict['bar'] = _gen_mock_host('bar')
        host_obj = factory.create_host(machine)
        self.assertEqual(host_obj._host_cls_name, 'bar')


    def test_detect_host_by_check_host(self):
        """Confirm check_host logic chooses a host object when label/attribute
        detection fails.
        """
        machine = _gen_machine_dict()
        self.host_types.append(_gen_mock_host('first', check_host=False))
        self.host_types.append(_gen_mock_host('second', check_host=True))
        self.host_types.append(_gen_mock_host('third', check_host=False))
        host_obj = factory.create_host(machine)
        self.assertEqual(host_obj._host_cls_name, 'second')


    def test_detect_host_fallback_to_cros_host(self):
        """Confirm fallback to CrosHost when all other detection fails.
        """
        machine = _gen_machine_dict()
        host_obj = factory.create_host(machine)
        self.assertEqual(host_obj._host_cls_name, 'cros_host')


    def test_choose_connectivity_local(self):
        """Confirm local connectivity class used when hostname is localhost.
        """
        machine = _gen_machine_dict(hostname='localhost')
        host_obj = factory.create_host(machine)
        self.assertEqual(host_obj._conn_cls_name, 'local')


    def test_choose_connectivity_ssh(self):
        """Confirm ssh connectivity class used when configured and hostname
        is not localhost.
        """
        factory.SSH_ENGINE = 'raw_ssh'
        machine = _gen_machine_dict(hostname='somehost')
        host_obj = factory.create_host(machine)
        self.assertEqual(host_obj._conn_cls_name, 'ssh')


    def test_choose_connectivity_unsupported(self):
        """Confirm exception when configured for unsupported ssh engine.
        """
        factory.SSH_ENGINE = 'unsupported'
        machine = _gen_machine_dict(hostname='somehost')
        with self.assertRaises(error.AutoservError):
            factory.create_host(machine)


    def test_argument_passthrough(self):
        """Confirm that detected and specified arguments are passed through to
        the host object.
        """
        machine = _gen_machine_dict(hostname='localhost')
        host_obj = factory.create_host(machine, foo='bar')
        self.assertEqual(host_obj._init_args['hostname'], 'localhost')
        self.assertTrue('afe_host' in host_obj._init_args)
        self.assertTrue('host_info_store' in host_obj._init_args)
        self.assertEqual(host_obj._init_args['foo'], 'bar')


    def test_global_ssh_params(self):
        """Confirm passing of ssh parameters set as globals.
        """
        factory.ssh_user = 'foo'
        factory.ssh_pass = 'bar'
        factory.ssh_port = 1
        factory.ssh_verbosity_flag = 'baz'
        factory.ssh_options = 'zip'
        machine = _gen_machine_dict()
        try:
            host_obj = factory.create_host(machine)
            self.assertEqual(host_obj._init_args['user'], 'foo')
            self.assertEqual(host_obj._init_args['password'], 'bar')
            self.assertEqual(host_obj._init_args['port'], 1)
            self.assertEqual(host_obj._init_args['ssh_verbosity_flag'], 'baz')
            self.assertEqual(host_obj._init_args['ssh_options'], 'zip')
        finally:
            del factory.ssh_user
            del factory.ssh_pass
            del factory.ssh_port
            del factory.ssh_verbosity_flag
            del factory.ssh_options


    def test_host_attribute_ssh_params(self):
        """Confirm passing of ssh parameters from host attributes.
        """
        machine = _gen_machine_dict(attributes={'ssh_user': 'somebody',
                                                'ssh_port': 100,
                                                'ssh_verbosity_flag': 'verb',
                                                'ssh_options': 'options'})
        host_obj = factory.create_host(machine)
        self.assertEqual(host_obj._init_args['user'], 'somebody')
        self.assertEqual(host_obj._init_args['port'], 100)
        self.assertEqual(host_obj._init_args['ssh_verbosity_flag'], 'verb')
        self.assertEqual(host_obj._init_args['ssh_options'], 'options')


class CreateTestbedUnittests(unittest.TestCase):
    """Tests for create_testbed function."""

    def setUp(self):
        """Mock out TestBed class to eliminate side effects.
        """
        self._orig_testbed = factory.testbed.TestBed
        factory.testbed.TestBed = _gen_mock_host('testbed')


    def tearDown(self):
        """Clean up mock.
        """
        factory.testbed.TestBed = self._orig_testbed


    def test_argument_passthrough(self):
        """Confirm that detected and specified arguments are passed through to
        the testbed object.
        """
        machine = _gen_machine_dict(hostname='localhost')
        testbed_obj = factory.create_testbed(machine, foo='bar')
        self.assertEqual(testbed_obj._init_args['hostname'], 'localhost')
        self.assertTrue('afe_host' in testbed_obj._init_args)
        self.assertTrue('host_info_store' in testbed_obj._init_args)
        self.assertEqual(testbed_obj._init_args['foo'], 'bar')


    def test_global_ssh_params(self):
        """Confirm passing of ssh parameters set as globals.
        """
        factory.ssh_user = 'foo'
        factory.ssh_pass = 'bar'
        factory.ssh_port = 1
        factory.ssh_verbosity_flag = 'baz'
        factory.ssh_options = 'zip'
        machine = _gen_machine_dict()
        try:
            testbed_obj = factory.create_testbed(machine)
            self.assertEqual(testbed_obj._init_args['user'], 'foo')
            self.assertEqual(testbed_obj._init_args['password'], 'bar')
            self.assertEqual(testbed_obj._init_args['port'], 1)
            self.assertEqual(testbed_obj._init_args['ssh_verbosity_flag'],
                             'baz')
            self.assertEqual(testbed_obj._init_args['ssh_options'], 'zip')
        finally:
            del factory.ssh_user
            del factory.ssh_pass
            del factory.ssh_port
            del factory.ssh_verbosity_flag
            del factory.ssh_options


    def test_host_attribute_ssh_params(self):
        """Confirm passing of ssh parameters from host attributes.
        """
        machine = _gen_machine_dict(attributes={'ssh_user': 'somebody',
                                                'ssh_port': 100,
                                                'ssh_verbosity_flag': 'verb',
                                                'ssh_options': 'options'})
        testbed_obj = factory.create_testbed(machine)
        self.assertEqual(testbed_obj._init_args['user'], 'somebody')
        self.assertEqual(testbed_obj._init_args['port'], 100)
        self.assertEqual(testbed_obj._init_args['ssh_verbosity_flag'], 'verb')
        self.assertEqual(testbed_obj._init_args['ssh_options'], 'options')


if __name__ == '__main__':
    unittest.main()

