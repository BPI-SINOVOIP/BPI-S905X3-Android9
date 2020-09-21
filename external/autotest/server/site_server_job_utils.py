# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Utility classes used by server_job.distribute_across_machines().

test_item: extends the basic test tuple to add include/exclude attributes and
    pre/post actions.
"""


import logging, os, Queue
from autotest_lib.client.common_lib import error, utils
from autotest_lib.server import autotest, hosts, host_attributes


class test_item(object):
    """Adds machine verification logic to the basic test tuple.

    Tests can either be tuples of the existing form ('testName', {args}) or the
    extended form ('testname', {args}, {'include': [], 'exclude': [],
    'attributes': []}) where include and exclude are lists of host attribute
    labels and attributes is a list of strings. A machine must have all the
    labels in include and must not have any of the labels in exclude to be valid
    for the test. Attributes strings can include reboot_before, reboot_after,
    and server_job.
    """

    def __init__(self, test_name, test_args, test_attribs=None):
        """Creates an instance of test_item.

        Args:
            test_name: string, name of test to execute.
            test_args: dictionary, arguments to pass into test.
            test_attribs: Dictionary of test attributes. Valid keys are:
              include - labels a machine must have to run a test.
              exclude - labels preventing a machine from running a test.
              attributes - reboot before/after test, run test as server job.
        """
        self.test_name = test_name
        self.test_args = test_args
        self.tagged_test_name = test_name
        if test_args.get('tag'):
            self.tagged_test_name = test_name + '.' + test_args.get('tag')

        if test_attribs is None:
            test_attribs = {}
        self.inc_set = set(test_attribs.get('include', []))
        self.exc_set = set(test_attribs.get('exclude', []))
        self.attributes = test_attribs.get('attributes', [])

    def __str__(self):
        """Return an info string of this test."""
        params = ['%s=%s' % (k, v) for k, v in self.test_args.items()]
        msg = '%s(%s)' % (self.test_name, params)
        if self.inc_set:
            msg += ' include=%s' % [s for s in self.inc_set]
        if self.exc_set:
            msg += ' exclude=%s' % [s for s in self.exc_set]
        if self.attributes:
            msg += ' attributes=%s' % self.attributes
        return msg

    def validate(self, machine_attributes):
        """Check if this test can run on machine with machine_attributes.

        If the test has include attributes, a candidate machine must have all
        the attributes to be valid.

        If the test has exclude attributes, a candidate machine cannot have any
        of the attributes to be valid.

        Args:
            machine_attributes: set, True attributes of candidate machine.

        Returns:
            True/False if the machine is valid for this test.
        """
        if self.inc_set is not None:
            if not self.inc_set <= machine_attributes:
                return False
        if self.exc_set is not None:
            if self.exc_set & machine_attributes:
                return False
        return True

    def run_test(self, client_at, work_dir='.', server_job=None):
        """Runs the test on the client using autotest.

        Args:
            client_at: Autotest instance for this host.
            work_dir: Directory to use for results and log files.
            server_job: Server_Job instance to use to runs server tests.
        """
        if 'reboot_before' in self.attributes:
            client_at.host.reboot()

        try:
            if 'server_job' in self.attributes:
                if 'host' in self.test_args:
                    self.test_args['host'] = client_at.host
                if server_job is not None:
                    logging.info('Running Server_Job=%s', self.test_name)
                    server_job.run_test(self.test_name, **self.test_args)
                else:
                    logging.error('No Server_Job instance provided for test '
                                  '%s.', self.test_name)
            else:
                client_at.run_test(self.test_name, results_dir=work_dir,
                                   **self.test_args)
        finally:
            if 'reboot_after' in self.attributes:
                client_at.host.reboot()
