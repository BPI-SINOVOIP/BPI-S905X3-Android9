# pylint: disable=missing-docstring

"""\
Functions to expose over the RPC interface.

For all modify* and delete* functions that ask for an 'id' parameter to
identify the object to operate on, the id may be either
 * the database row ID
 * the name of the object (label name, hostname, user login, etc.)
 * a dictionary containing uniquely identifying field (this option should seldom
   be used)

When specifying foreign key fields (i.e. adding hosts to a label, or adding
users to an ACL group), the given value may be either the database row ID or the
name of the object.

All get* functions return lists of dictionaries.  Each dictionary represents one
object and maps field names to values.

Some examples:
modify_host(2, hostname='myhost') # modify hostname of host with database ID 2
modify_host('ipaj2', hostname='myhost') # modify hostname of host 'ipaj2'
modify_test('sleeptest', test_type='Client', params=', seconds=60')
delete_acl_group(1) # delete by ID
delete_acl_group('Everyone') # delete by name
acl_group_add_users('Everyone', ['mbligh', 'showard'])
get_jobs(owner='showard', status='Queued')

See doctests/001_rpc_test.txt for (lots) more examples.
"""

__author__ = 'showard@google.com (Steve Howard)'

import ast
import collections
import contextlib
import datetime
import logging
import os
import sys
import warnings

from django.db import connection as db_connection
from django.db import transaction
from django.db.models import Count
from django.db.utils import DatabaseError

import common
from autotest_lib.client.common_lib import control_data
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib import priorities
from autotest_lib.client.common_lib import time_utils
from autotest_lib.client.common_lib.cros import dev_server
from autotest_lib.frontend.afe import control_file as control_file_lib
from autotest_lib.frontend.afe import model_attributes
from autotest_lib.frontend.afe import model_logic
from autotest_lib.frontend.afe import models
from autotest_lib.frontend.afe import rpc_utils
from autotest_lib.frontend.tko import models as tko_models
from autotest_lib.frontend.tko import rpc_interface as tko_rpc_interface
from autotest_lib.server import frontend
from autotest_lib.server import utils
from autotest_lib.server.cros import provision
from autotest_lib.server.cros.dynamic_suite import constants
from autotest_lib.server.cros.dynamic_suite import control_file_getter
from autotest_lib.server.cros.dynamic_suite import suite as SuiteBase
from autotest_lib.server.cros.dynamic_suite import tools
from autotest_lib.server.cros.dynamic_suite.suite import Suite
from autotest_lib.server.lib import status_history
from autotest_lib.site_utils import job_history
from autotest_lib.site_utils import server_manager_utils
from autotest_lib.site_utils import stable_version_utils


_CONFIG = global_config.global_config

# Definition of LabHealthIndicator
LabHealthIndicator = collections.namedtuple(
        'LabHealthIndicator',
        [
                'if_lab_close',
                'available_duts',
                'devserver_health',
                'upcoming_builds',
        ]
)

RESPECT_STATIC_LABELS = global_config.global_config.get_config_value(
        'SKYLAB', 'respect_static_labels', type=bool, default=False)

RESPECT_STATIC_ATTRIBUTES = global_config.global_config.get_config_value(
        'SKYLAB', 'respect_static_attributes', type=bool, default=False)

# Relevant CrosDynamicSuiteExceptions are defined in client/common_lib/error.py.

# labels

def modify_label(id, **data):
    """Modify a label.

    @param id: id or name of a label. More often a label name.
    @param data: New data for a label.
    """
    label_model = models.Label.smart_get(id)
    if label_model.is_replaced_by_static():
        raise error.UnmodifiableLabelException(
                'Failed to delete label "%s" because it is a static label. '
                'Use go/chromeos-skylab-inventory-tools to modify this '
                'label.' % label_model.name)

    label_model.update_object(data)

    # Master forwards the RPC to shards
    if not utils.is_shard():
        rpc_utils.fanout_rpc(label_model.host_set.all(), 'modify_label', False,
                             id=id, **data)


def delete_label(id):
    """Delete a label.

    @param id: id or name of a label. More often a label name.
    """
    label_model = models.Label.smart_get(id)
    if label_model.is_replaced_by_static():
        raise error.UnmodifiableLabelException(
                'Failed to delete label "%s" because it is a static label. '
                'Use go/chromeos-skylab-inventory-tools to modify this '
                'label.' % label_model.name)

    # Hosts that have the label to be deleted. Save this info before
    # the label is deleted to use it later.
    hosts = []
    for h in label_model.host_set.all():
        hosts.append(models.Host.smart_get(h.id))
    label_model.delete()

    # Master forwards the RPC to shards
    if not utils.is_shard():
        rpc_utils.fanout_rpc(hosts, 'delete_label', False, id=id)


def add_label(name, ignore_exception_if_exists=False, **kwargs):
    """Adds a new label of a given name.

    @param name: label name.
    @param ignore_exception_if_exists: If True and the exception was
        thrown due to the duplicated label name when adding a label,
        then suppress the exception. Default is False.
    @param kwargs: keyword args that store more info about a label
        other than the name.
    @return: int/long id of a new label.
    """
    # models.Label.add_object() throws model_logic.ValidationError
    # when it is given a label name that already exists.
    # However, ValidationError can be thrown with different errors,
    # and those errors should be thrown up to the call chain.
    try:
        label = models.Label.add_object(name=name, **kwargs)
    except:
        exc_info = sys.exc_info()
        if ignore_exception_if_exists:
            label = rpc_utils.get_label(name)
            # If the exception is raised not because of duplicated
            # "name", then raise the original exception.
            if label is None:
                raise exc_info[0], exc_info[1], exc_info[2]
        else:
            raise exc_info[0], exc_info[1], exc_info[2]
    return label.id


def add_label_to_hosts(id, hosts):
    """Adds a label of the given id to the given hosts only in local DB.

    @param id: id or name of a label. More often a label name.
    @param hosts: The hostnames of hosts that need the label.

    @raises models.Label.DoesNotExist: If the label with id doesn't exist.
    """
    label = models.Label.smart_get(id)
    if label.is_replaced_by_static():
        label = models.StaticLabel.smart_get(label.name)

    host_objs = models.Host.smart_get_bulk(hosts)
    if label.platform:
        models.Host.check_no_platform(host_objs)
    # Ensure a host has no more than one board label with it.
    if label.name.startswith('board:'):
        models.Host.check_board_labels_allowed(host_objs, [label.name])
    label.host_set.add(*host_objs)


def _create_label_everywhere(id, hosts):
    """
    Yet another method to create labels.

    ALERT! This method should be run only on master not shards!
    DO NOT RUN THIS ON A SHARD!!!  Deputies will hate you if you do!!!

    This method exists primarily to serve label_add_hosts() and
    host_add_labels().  Basically it pulls out the label check/add logic
    from label_add_hosts() into this nice method that not only creates
    the label but also tells the shards that service the hosts to also
    create the label.

    @param id: id or name of a label. More often a label name.
    @param hosts: A list of hostnames or ids. More often hostnames.
    """
    try:
        label = models.Label.smart_get(id)
    except models.Label.DoesNotExist:
        # This matches the type checks in smart_get, which is a hack
        # in and off itself. The aim here is to create any non-existent
        # label, which we cannot do if the 'id' specified isn't a label name.
        if isinstance(id, basestring):
            label = models.Label.smart_get(add_label(id))
        else:
            raise ValueError('Label id (%s) does not exist. Please specify '
                             'the argument, id, as a string (label name).'
                             % id)

    # Make sure the label exists on the shard with the same id
    # as it is on the master.
    # It is possible that the label is already in a shard because
    # we are adding a new label only to shards of hosts that the label
    # is going to be attached.
    # For example, we add a label L1 to a host in shard S1.
    # Master and S1 will have L1 but other shards won't.
    # Later, when we add the same label L1 to hosts in shards S1 and S2,
    # S1 already has the label but S2 doesn't.
    # S2 should have the new label without any problem.
    # We ignore exception in such a case.
    host_objs = models.Host.smart_get_bulk(hosts)
    rpc_utils.fanout_rpc(
            host_objs, 'add_label', include_hostnames=False,
            name=label.name, ignore_exception_if_exists=True,
            id=label.id, platform=label.platform)


@rpc_utils.route_rpc_to_master
def label_add_hosts(id, hosts):
    """Adds a label with the given id to the given hosts.

    This method should be run only on master not shards.
    The given label will be created if it doesn't exist, provided the `id`
    supplied is a label name not an int/long id.

    @param id: id or name of a label. More often a label name.
    @param hosts: A list of hostnames or ids. More often hostnames.

    @raises ValueError: If the id specified is an int/long (label id)
                        while the label does not exist.
    """
    # Create the label.
    _create_label_everywhere(id, hosts)

    # Add it to the master.
    add_label_to_hosts(id, hosts)

    # Add it to the shards.
    host_objs = models.Host.smart_get_bulk(hosts)
    rpc_utils.fanout_rpc(host_objs, 'add_label_to_hosts', id=id)


def remove_label_from_hosts(id, hosts):
    """Removes a label of the given id from the given hosts only in local DB.

    @param id: id or name of a label.
    @param hosts: The hostnames of hosts that need to remove the label from.
    """
    host_objs = models.Host.smart_get_bulk(hosts)
    label = models.Label.smart_get(id)
    if label.is_replaced_by_static():
        raise error.UnmodifiableLabelException(
                'Failed to remove label "%s" for hosts "%r" because it is a '
                'static label. Use go/chromeos-skylab-inventory-tools to '
                'modify this label.' % (label.name, hosts))

    label.host_set.remove(*host_objs)


@rpc_utils.route_rpc_to_master
def label_remove_hosts(id, hosts):
    """Removes a label of the given id from the given hosts.

    This method should be run only on master not shards.

    @param id: id or name of a label.
    @param hosts: A list of hostnames or ids. More often hostnames.
    """
    host_objs = models.Host.smart_get_bulk(hosts)
    remove_label_from_hosts(id, hosts)

    rpc_utils.fanout_rpc(host_objs, 'remove_label_from_hosts', id=id)


def get_labels(exclude_filters=(), **filter_data):
    """\
    @param exclude_filters: A sequence of dictionaries of filters.

    @returns A sequence of nested dictionaries of label information.
    """
    labels = models.Label.query_objects(filter_data)
    for exclude_filter in exclude_filters:
        labels = labels.exclude(**exclude_filter)

    if not RESPECT_STATIC_LABELS:
        return rpc_utils.prepare_rows_as_nested_dicts(labels, ())

    static_labels = models.StaticLabel.query_objects(filter_data)
    for exclude_filter in exclude_filters:
        static_labels = static_labels.exclude(**exclude_filter)

    non_static_lists = rpc_utils.prepare_rows_as_nested_dicts(labels, ())
    static_lists = rpc_utils.prepare_rows_as_nested_dicts(static_labels, ())

    label_ids = [label.id for label in labels]
    replaced = models.ReplacedLabel.objects.filter(label__id__in=label_ids)
    replaced_ids = {r.label_id for r in replaced}
    replaced_label_names = {l.name for l in labels if l.id in replaced_ids}

    return_lists  = []
    for non_static_label in non_static_lists:
        if non_static_label.get('id') not in replaced_ids:
            return_lists.append(non_static_label)

    for static_label in static_lists:
        if static_label.get('name') in replaced_label_names:
            return_lists.append(static_label)

    return return_lists


# hosts

def add_host(hostname, status=None, locked=None, lock_reason='', protection=None):
    if locked and not lock_reason:
        raise model_logic.ValidationError(
            {'locked': 'Please provide a reason for locking when adding host.'})

    return models.Host.add_object(hostname=hostname, status=status,
                                  locked=locked, lock_reason=lock_reason,
                                  protection=protection).id


@rpc_utils.route_rpc_to_master
def modify_host(id, **kwargs):
    """Modify local attributes of a host.

    If this is called on the master, but the host is assigned to a shard, this
    will call `modify_host_local` RPC to the responsible shard. This means if
    a host is being locked using this function, this change will also propagate
    to shards.
    When this is called on a shard, the shard just routes the RPC to the master
    and does nothing.

    @param id: id of the host to modify.
    @param kwargs: key=value pairs of values to set on the host.
    """
    rpc_utils.check_modify_host(kwargs)
    host = models.Host.smart_get(id)
    try:
        rpc_utils.check_modify_host_locking(host, kwargs)
    except model_logic.ValidationError as e:
        if not kwargs.get('force_modify_locking', False):
            raise
        logging.exception('The following exception will be ignored and lock '
                          'modification will be enforced. %s', e)

    # This is required to make `lock_time` for a host be exactly same
    # between the master and a shard.
    if kwargs.get('locked', None) and 'lock_time' not in kwargs:
        kwargs['lock_time'] = datetime.datetime.now()

    # force_modifying_locking is not an internal field in database, remove.
    shard_kwargs = dict(kwargs)
    shard_kwargs.pop('force_modify_locking', None)
    rpc_utils.fanout_rpc([host], 'modify_host_local',
                         include_hostnames=False, id=id, **shard_kwargs)

    # Update the local DB **after** RPC fanout is complete.
    # This guarantees that the master state is only updated if the shards were
    # correctly updated.
    # In case the shard update fails mid-flight and the master-shard desync, we
    # always consider the master state to be the source-of-truth, and any
    # (automated) corrective actions will revert the (partial) shard updates.
    host.update_object(kwargs)


def modify_host_local(id, **kwargs):
    """Modify host attributes in local DB.

    @param id: Host id.
    @param kwargs: key=value pairs of values to set on the host.
    """
    models.Host.smart_get(id).update_object(kwargs)


@rpc_utils.route_rpc_to_master
def modify_hosts(host_filter_data, update_data):
    """Modify local attributes of multiple hosts.

    If this is called on the master, but one of the hosts in that match the
    filters is assigned to a shard, this will call `modify_hosts_local` RPC
    to the responsible shard.
    When this is called on a shard, the shard just routes the RPC to the master
    and does nothing.

    The filters are always applied on the master, not on the shards. This means
    if the states of a host differ on the master and a shard, the state on the
    master will be used. I.e. this means:
    A host was synced to Shard 1. On Shard 1 the status of the host was set to
    'Repair Failed'.
    - A call to modify_hosts with host_filter_data={'status': 'Ready'} will
    update the host (both on the shard and on the master), because the state
    of the host as the master knows it is still 'Ready'.
    - A call to modify_hosts with host_filter_data={'status': 'Repair failed'
    will not update the host, because the filter doesn't apply on the master.

    @param host_filter_data: Filters out which hosts to modify.
    @param update_data: A dictionary with the changes to make to the hosts.
    """
    update_data = update_data.copy()
    rpc_utils.check_modify_host(update_data)
    hosts = models.Host.query_objects(host_filter_data)

    affected_shard_hostnames = set()
    affected_host_ids = []

    # Check all hosts before changing data for exception safety.
    for host in hosts:
        try:
            rpc_utils.check_modify_host_locking(host, update_data)
        except model_logic.ValidationError as e:
            if not update_data.get('force_modify_locking', False):
                raise
            logging.exception('The following exception will be ignored and '
                              'lock modification will be enforced. %s', e)

        if host.shard:
            affected_shard_hostnames.add(host.shard.hostname)
            affected_host_ids.append(host.id)

    # This is required to make `lock_time` for a host be exactly same
    # between the master and a shard.
    if update_data.get('locked', None) and 'lock_time' not in update_data:
        update_data['lock_time'] = datetime.datetime.now()
    for host in hosts:
        host.update_object(update_data)

    update_data.pop('force_modify_locking', None)
    # Caution: Changing the filter from the original here. See docstring.
    rpc_utils.run_rpc_on_multiple_hostnames(
            'modify_hosts_local', affected_shard_hostnames,
            host_filter_data={'id__in': affected_host_ids},
            update_data=update_data)


def modify_hosts_local(host_filter_data, update_data):
    """Modify attributes of hosts in local DB.

    @param host_filter_data: Filters out which hosts to modify.
    @param update_data: A dictionary with the changes to make to the hosts.
    """
    for host in models.Host.query_objects(host_filter_data):
        host.update_object(update_data)


def add_labels_to_host(id, labels):
    """Adds labels to a given host only in local DB.

    @param id: id or hostname for a host.
    @param labels: ids or names for labels.
    """
    label_objs = models.Label.smart_get_bulk(labels)
    if not RESPECT_STATIC_LABELS:
        models.Host.smart_get(id).labels.add(*label_objs)
    else:
        static_labels, non_static_labels = models.Host.classify_label_objects(
            label_objs)
        host = models.Host.smart_get(id)
        host.static_labels.add(*static_labels)
        host.labels.add(*non_static_labels)


@rpc_utils.route_rpc_to_master
def host_add_labels(id, labels):
    """Adds labels to a given host.

    @param id: id or hostname for a host.
    @param labels: ids or names for labels.

    @raises ValidationError: If adding more than one platform/board label.
    """
    # Create the labels on the master/shards.
    for label in labels:
        _create_label_everywhere(label, [id])

    label_objs = models.Label.smart_get_bulk(labels)

    platforms = [label.name for label in label_objs if label.platform]
    boards = [label.name for label in label_objs
              if label.name.startswith('board:')]
    if len(platforms) > 1 or not utils.board_labels_allowed(boards):
        raise model_logic.ValidationError(
            {'labels': ('Adding more than one platform label, or a list of '
                        'non-compatible board labels.: %s %s' %
                        (', '.join(platforms), ', '.join(boards)))})

    host_obj = models.Host.smart_get(id)
    if platforms:
        models.Host.check_no_platform([host_obj])
    if boards:
        models.Host.check_board_labels_allowed([host_obj], labels)
    add_labels_to_host(id, labels)

    rpc_utils.fanout_rpc([host_obj], 'add_labels_to_host', False,
                         id=id, labels=labels)


def remove_labels_from_host(id, labels):
    """Removes labels from a given host only in local DB.

    @param id: id or hostname for a host.
    @param labels: ids or names for labels.
    """
    label_objs = models.Label.smart_get_bulk(labels)
    if not RESPECT_STATIC_LABELS:
        models.Host.smart_get(id).labels.remove(*label_objs)
    else:
        static_labels, non_static_labels = models.Host.classify_label_objects(
                label_objs)
        host = models.Host.smart_get(id)
        host.labels.remove(*non_static_labels)
        if static_labels:
            logging.info('Cannot remove labels "%r" for host "%r" due to they '
                         'are static labels. Use '
                         'go/chromeos-skylab-inventory-tools to modify these '
                         'labels.', static_labels, id)


@rpc_utils.route_rpc_to_master
def host_remove_labels(id, labels):
    """Removes labels from a given host.

    @param id: id or hostname for a host.
    @param labels: ids or names for labels.
    """
    remove_labels_from_host(id, labels)

    host_obj = models.Host.smart_get(id)
    rpc_utils.fanout_rpc([host_obj], 'remove_labels_from_host', False,
                         id=id, labels=labels)


def get_host_attribute(attribute, **host_filter_data):
    """
    @param attribute: string name of attribute
    @param host_filter_data: filter data to apply to Hosts to choose hosts to
                             act upon
    """
    hosts = rpc_utils.get_host_query((), False, True, host_filter_data)
    hosts = list(hosts)
    models.Host.objects.populate_relationships(hosts, models.HostAttribute,
                                               'attribute_list')
    host_attr_dicts = []
    host_objs = []
    for host_obj in hosts:
        for attr_obj in host_obj.attribute_list:
            if attr_obj.attribute == attribute:
                host_attr_dicts.append(attr_obj.get_object_dict())
                host_objs.append(host_obj)

    if RESPECT_STATIC_ATTRIBUTES:
        for host_attr, host_obj in zip(host_attr_dicts, host_objs):
            static_attrs = models.StaticHostAttribute.query_objects(
                    {'host_id': host_obj.id, 'attribute': attribute})
            if len(static_attrs) > 0:
                host_attr['value'] = static_attrs[0].value

    return rpc_utils.prepare_for_serialization(host_attr_dicts)


@rpc_utils.route_rpc_to_master
def set_host_attribute(attribute, value, **host_filter_data):
    """Set an attribute on hosts.

    This RPC is a shim that forwards calls to master to be handled there.

    @param attribute: string name of attribute
    @param value: string, or None to delete an attribute
    @param host_filter_data: filter data to apply to Hosts to choose hosts to
                             act upon
    """
    assert not utils.is_shard()
    set_host_attribute_impl(attribute, value, **host_filter_data)


def set_host_attribute_impl(attribute, value, **host_filter_data):
    """Set an attribute on hosts.

    *** DO NOT CALL THIS RPC from client code ***
    This RPC exists for master-shard communication only.
    Call set_host_attribute instead.

    @param attribute: string name of attribute
    @param value: string, or None to delete an attribute
    @param host_filter_data: filter data to apply to Hosts to choose hosts to
                             act upon
    """
    assert host_filter_data # disallow accidental actions on all hosts
    hosts = models.Host.query_objects(host_filter_data)
    models.AclGroup.check_for_acl_violation_hosts(hosts)
    for host in hosts:
        host.set_or_delete_attribute(attribute, value)

    # Master forwards this RPC to shards.
    if not utils.is_shard():
        rpc_utils.fanout_rpc(hosts, 'set_host_attribute_impl', False,
                attribute=attribute, value=value, **host_filter_data)


@rpc_utils.forward_single_host_rpc_to_shard
def delete_host(id):
    models.Host.smart_get(id).delete()


def get_hosts(multiple_labels=(), exclude_only_if_needed_labels=False,
              valid_only=True, include_current_job=False, **filter_data):
    """Get a list of dictionaries which contains the information of hosts.

    @param multiple_labels: match hosts in all of the labels given.  Should
            be a list of label names.
    @param exclude_only_if_needed_labels: Deprecated. Raise error if it's True.
    @param include_current_job: Set to True to include ids of currently running
            job and special task.
    """
    if exclude_only_if_needed_labels:
        raise error.RPCException('exclude_only_if_needed_labels is deprecated')

    hosts = rpc_utils.get_host_query(multiple_labels,
                                     exclude_only_if_needed_labels,
                                     valid_only, filter_data)
    hosts = list(hosts)
    models.Host.objects.populate_relationships(hosts, models.Label,
                                               'label_list')
    models.Host.objects.populate_relationships(hosts, models.AclGroup,
                                               'acl_list')
    models.Host.objects.populate_relationships(hosts, models.HostAttribute,
                                               'attribute_list')
    models.Host.objects.populate_relationships(hosts,
                                               models.StaticHostAttribute,
                                               'staticattribute_list')
    host_dicts = []
    for host_obj in hosts:
        host_dict = host_obj.get_object_dict()
        host_dict['acls'] = [acl.name for acl in host_obj.acl_list]
        host_dict['attributes'] = dict((attribute.attribute, attribute.value)
                                       for attribute in host_obj.attribute_list)
        if RESPECT_STATIC_LABELS:
            label_list = []
            # Only keep static labels which has a corresponding entries in
            # afe_labels.
            for label in host_obj.label_list:
                if label.is_replaced_by_static():
                    static_label = models.StaticLabel.smart_get(label.name)
                    label_list.append(static_label)
                else:
                    label_list.append(label)

            host_dict['labels'] = [label.name for label in label_list]
            host_dict['platform'] = rpc_utils.find_platform(
                    host_obj.hostname, label_list)
        else:
            host_dict['labels'] = [label.name for label in host_obj.label_list]
            host_dict['platform'] = rpc_utils.find_platform(
                    host_obj.hostname, host_obj.label_list)

        if RESPECT_STATIC_ATTRIBUTES:
            # Overwrite attribute with values in afe_static_host_attributes.
            for attr in host_obj.staticattribute_list:
                if attr.attribute in host_dict['attributes']:
                    host_dict['attributes'][attr.attribute] = attr.value

        if include_current_job:
            host_dict['current_job'] = None
            host_dict['current_special_task'] = None
            entries = models.HostQueueEntry.objects.filter(
                    host_id=host_dict['id'], active=True, complete=False)
            if entries:
                host_dict['current_job'] = (
                        entries[0].get_object_dict()['job'])
            tasks = models.SpecialTask.objects.filter(
                    host_id=host_dict['id'], is_active=True, is_complete=False)
            if tasks:
                host_dict['current_special_task'] = (
                        '%d-%s' % (tasks[0].get_object_dict()['id'],
                                   tasks[0].get_object_dict()['task'].lower()))
        host_dicts.append(host_dict)

    return rpc_utils.prepare_for_serialization(host_dicts)


def get_num_hosts(multiple_labels=(), exclude_only_if_needed_labels=False,
                  valid_only=True, **filter_data):
    """
    Same parameters as get_hosts().

    @returns The number of matching hosts.
    """
    if exclude_only_if_needed_labels:
        raise error.RPCException('exclude_only_if_needed_labels is deprecated')

    hosts = rpc_utils.get_host_query(multiple_labels,
                                     exclude_only_if_needed_labels,
                                     valid_only, filter_data)
    return len(hosts)


# tests

def get_tests(**filter_data):
    return rpc_utils.prepare_for_serialization(
        models.Test.list_objects(filter_data))


def get_tests_status_counts_by_job_name_label(job_name_prefix, label_name):
    """Gets the counts of all passed and failed tests from the matching jobs.

    @param job_name_prefix: Name prefix of the jobs to get the summary
           from, e.g., 'butterfly-release/r40-6457.21.0/bvt-cq/'. Prefix
           matching is case insensitive.
    @param label_name: Label that must be set in the jobs, e.g.,
            'cros-version:butterfly-release/R40-6457.21.0'.

    @returns A summary of the counts of all the passed and failed tests.
    """
    job_ids = list(models.Job.objects.filter(
            name__istartswith=job_name_prefix,
            dependency_labels__name=label_name).values_list(
                'pk', flat=True))
    summary = {'passed': 0, 'failed': 0}
    if not job_ids:
        return summary

    counts = (tko_models.TestView.objects.filter(
            afe_job_id__in=job_ids).exclude(
                test_name='SERVER_JOB').exclude(
                    test_name__startswith='CLIENT_JOB').values(
                        'status').annotate(
                            count=Count('status')))
    for status in counts:
        if status['status'] == 'GOOD':
            summary['passed'] += status['count']
        else:
            summary['failed'] += status['count']
    return summary


# profilers

def add_profiler(name, description=None):
    return models.Profiler.add_object(name=name, description=description).id


def modify_profiler(id, **data):
    models.Profiler.smart_get(id).update_object(data)


def delete_profiler(id):
    models.Profiler.smart_get(id).delete()


def get_profilers(**filter_data):
    return rpc_utils.prepare_for_serialization(
        models.Profiler.list_objects(filter_data))


# users

def get_users(**filter_data):
    return rpc_utils.prepare_for_serialization(
        models.User.list_objects(filter_data))


# acl groups

def add_acl_group(name, description=None):
    group = models.AclGroup.add_object(name=name, description=description)
    group.users.add(models.User.current_user())
    return group.id


def modify_acl_group(id, **data):
    group = models.AclGroup.smart_get(id)
    group.check_for_acl_violation_acl_group()
    group.update_object(data)
    group.add_current_user_if_empty()


def acl_group_add_users(id, users):
    group = models.AclGroup.smart_get(id)
    group.check_for_acl_violation_acl_group()
    users = models.User.smart_get_bulk(users)
    group.users.add(*users)


def acl_group_remove_users(id, users):
    group = models.AclGroup.smart_get(id)
    group.check_for_acl_violation_acl_group()
    users = models.User.smart_get_bulk(users)
    group.users.remove(*users)
    group.add_current_user_if_empty()


def acl_group_add_hosts(id, hosts):
    group = models.AclGroup.smart_get(id)
    group.check_for_acl_violation_acl_group()
    hosts = models.Host.smart_get_bulk(hosts)
    group.hosts.add(*hosts)
    group.on_host_membership_change()


def acl_group_remove_hosts(id, hosts):
    group = models.AclGroup.smart_get(id)
    group.check_for_acl_violation_acl_group()
    hosts = models.Host.smart_get_bulk(hosts)
    group.hosts.remove(*hosts)
    group.on_host_membership_change()


def delete_acl_group(id):
    models.AclGroup.smart_get(id).delete()


def get_acl_groups(**filter_data):
    acl_groups = models.AclGroup.list_objects(filter_data)
    for acl_group in acl_groups:
        acl_group_obj = models.AclGroup.objects.get(id=acl_group['id'])
        acl_group['users'] = [user.login
                              for user in acl_group_obj.users.all()]
        acl_group['hosts'] = [host.hostname
                              for host in acl_group_obj.hosts.all()]
    return rpc_utils.prepare_for_serialization(acl_groups)


# jobs

def generate_control_file(tests=(), profilers=(),
                          client_control_file='', use_container=False,
                          profile_only=None, db_tests=True,
                          test_source_build=None):
    """
    Generates a client-side control file to run tests.

    @param tests List of tests to run. See db_tests for more information.
    @param profilers List of profilers to activate during the job.
    @param client_control_file The contents of a client-side control file to
        run at the end of all tests.  If this is supplied, all tests must be
        client side.
        TODO: in the future we should support server control files directly
        to wrap with a kernel.  That'll require changing the parameter
        name and adding a boolean to indicate if it is a client or server
        control file.
    @param use_container unused argument today.  TODO: Enable containers
        on the host during a client side test.
    @param profile_only A boolean that indicates what default profile_only
        mode to use in the control file. Passing None will generate a
        control file that does not explcitly set the default mode at all.
    @param db_tests: if True, the test object can be found in the database
                     backing the test model. In this case, tests is a tuple
                     of test IDs which are used to retrieve the test objects
                     from the database. If False, tests is a tuple of test
                     dictionaries stored client-side in the AFE.
    @param test_source_build: Build to be used to retrieve test code. Default
                              to None.

    @returns a dict with the following keys:
        control_file: str, The control file text.
        is_server: bool, is the control file a server-side control file?
        synch_count: How many machines the job uses per autoserv execution.
            synch_count == 1 means the job is asynchronous.
        dependencies: A list of the names of labels on which the job depends.
    """
    if not tests and not client_control_file:
        return dict(control_file='', is_server=False, synch_count=1,
                    dependencies=[])

    cf_info, test_objects, profiler_objects = (
        rpc_utils.prepare_generate_control_file(tests, profilers,
                                                db_tests))
    cf_info['control_file'] = control_file_lib.generate_control(
        tests=test_objects, profilers=profiler_objects,
        is_server=cf_info['is_server'],
        client_control_file=client_control_file, profile_only=profile_only,
        test_source_build=test_source_build)
    return cf_info


def create_job_page_handler(name, priority, control_file, control_type,
                            image=None, hostless=False, firmware_rw_build=None,
                            firmware_ro_build=None, test_source_build=None,
                            is_cloning=False, cheets_build=None, **kwargs):
    """\
    Create and enqueue a job.

    @param name name of this job
    @param priority Integer priority of this job.  Higher is more important.
    @param control_file String contents of the control file.
    @param control_type Type of control file, Client or Server.
    @param image: ChromeOS build to be installed in the dut. Default to None.
    @param firmware_rw_build: Firmware build to update RW firmware. Default to
                              None, i.e., RW firmware will not be updated.
    @param firmware_ro_build: Firmware build to update RO firmware. Default to
                              None, i.e., RO firmware will not be updated.
    @param test_source_build: Build to be used to retrieve test code. Default
                              to None.
    @param is_cloning: True if creating a cloning job.
    @param cheets_build: ChromeOS Android build  to be installed in the dut.
                         Default to None. Cheets build will not be updated.
    @param kwargs extra args that will be required by create_suite_job or
                  create_job.

    @returns The created Job id number.
    """
    test_args = {}
    if kwargs.get('args'):
        # args' format is: ['disable_sysinfo=False', 'fast=True', ...]
        args = kwargs.get('args')
        for arg in args:
            k, v = arg.split('=')[0], arg.split('=')[1]
            test_args[k] = v

    if is_cloning:
        logging.info('Start to clone a new job')
        # When cloning a job, hosts and meta_hosts should not exist together,
        # which would cause host-scheduler to schedule two hqe jobs to one host
        # at the same time, and crash itself. Clear meta_hosts for this case.
        if kwargs.get('hosts') and kwargs.get('meta_hosts'):
            kwargs['meta_hosts'] = []
    else:
        logging.info('Start to create a new job')
    control_file = rpc_utils.encode_ascii(control_file)
    if not control_file:
        raise model_logic.ValidationError({
                'control_file' : "Control file cannot be empty"})

    if image and hostless:
        builds = {}
        builds[provision.CROS_VERSION_PREFIX] = image
        if cheets_build:
            builds[provision.CROS_ANDROID_VERSION_PREFIX] = cheets_build
        if firmware_rw_build:
            builds[provision.FW_RW_VERSION_PREFIX] = firmware_rw_build
        if firmware_ro_build:
            builds[provision.FW_RO_VERSION_PREFIX] = firmware_ro_build
        return create_suite_job(
                name=name, control_file=control_file, priority=priority,
                builds=builds, test_source_build=test_source_build,
                is_cloning=is_cloning, test_args=test_args, **kwargs)

    return create_job(name, priority, control_file, control_type, image=image,
                      hostless=hostless, test_args=test_args, **kwargs)


@rpc_utils.route_rpc_to_master
def create_job(
        name,
        priority,
        control_file,
        control_type,
        hosts=(),
        meta_hosts=(),
        one_time_hosts=(),
        synch_count=None,
        is_template=False,
        timeout=None,
        timeout_mins=None,
        max_runtime_mins=None,
        run_verify=False,
        email_list='',
        dependencies=(),
        reboot_before=None,
        reboot_after=None,
        parse_failed_repair=None,
        hostless=False,
        keyvals=None,
        drone_set=None,
        image=None,
        parent_job_id=None,
        test_retry=0,
        run_reset=True,
        require_ssp=None,
        test_args=None,
        **kwargs):
    """\
    Create and enqueue a job.

    @param name name of this job
    @param priority Integer priority of this job.  Higher is more important.
    @param control_file String contents of the control file.
    @param control_type Type of control file, Client or Server.
    @param synch_count How many machines the job uses per autoserv execution.
        synch_count == 1 means the job is asynchronous.  If an atomic group is
        given this value is treated as a minimum.
    @param is_template If true then create a template job.
    @param timeout Hours after this call returns until the job times out.
    @param timeout_mins Minutes after this call returns until the job times
        out.
    @param max_runtime_mins Minutes from job starting time until job times out
    @param run_verify Should the host be verified before running the test?
    @param email_list String containing emails to mail when the job is done
    @param dependencies List of label names on which this job depends
    @param reboot_before Never, If dirty, or Always
    @param reboot_after Never, If all tests passed, or Always
    @param parse_failed_repair if true, results of failed repairs launched by
        this job will be parsed as part of the job.
    @param hostless if true, create a hostless job
    @param keyvals dict of keyvals to associate with the job
    @param hosts List of hosts to run job on.
    @param meta_hosts List where each entry is a label name, and for each entry
        one host will be chosen from that label to run the job on.
    @param one_time_hosts List of hosts not in the database to run the job on.
    @param drone_set The name of the drone set to run this test on.
    @param image OS image to install before running job.
    @param parent_job_id id of a job considered to be parent of created job.
    @param test_retry Number of times to retry test if the test did not
        complete successfully. (optional, default: 0)
    @param run_reset Should the host be reset before running the test?
    @param require_ssp Set to True to require server-side packaging to run the
                       test. If it's set to None, drone will still try to run
                       the server side with server-side packaging. If the
                       autotest-server package doesn't exist for the build or
                       image is not set, drone will run the test without server-
                       side packaging. Default is None.
    @param test_args A dict of args passed to be injected into control file.
    @param kwargs extra keyword args. NOT USED.

    @returns The created Job id number.
    """
    if test_args:
        control_file = tools.inject_vars(test_args, control_file)
    if image:
        dependencies += (provision.image_version_to_label(image),)
    return rpc_utils.create_job_common(
            name=name,
            priority=priority,
            control_type=control_type,
            control_file=control_file,
            hosts=hosts,
            meta_hosts=meta_hosts,
            one_time_hosts=one_time_hosts,
            synch_count=synch_count,
            is_template=is_template,
            timeout=timeout,
            timeout_mins=timeout_mins,
            max_runtime_mins=max_runtime_mins,
            run_verify=run_verify,
            email_list=email_list,
            dependencies=dependencies,
            reboot_before=reboot_before,
            reboot_after=reboot_after,
            parse_failed_repair=parse_failed_repair,
            hostless=hostless,
            keyvals=keyvals,
            drone_set=drone_set,
            parent_job_id=parent_job_id,
            test_retry=test_retry,
            run_reset=run_reset,
            require_ssp=require_ssp)


def abort_host_queue_entries(**filter_data):
    """\
    Abort a set of host queue entries.

    @return: A list of dictionaries, each contains information
             about an aborted HQE.
    """
    query = models.HostQueueEntry.query_objects(filter_data)

    # Dont allow aborts on:
    #   1. Jobs that have already completed (whether or not they were aborted)
    #   2. Jobs that we have already been aborted (but may not have completed)
    query = query.filter(complete=False).filter(aborted=False)
    models.AclGroup.check_abort_permissions(query)
    host_queue_entries = list(query.select_related())
    rpc_utils.check_abort_synchronous_jobs(host_queue_entries)

    models.HostQueueEntry.abort_host_queue_entries(host_queue_entries)
    hqe_info = [{'HostQueueEntry': hqe.id, 'Job': hqe.job_id,
                 'Job name': hqe.job.name} for hqe in host_queue_entries]
    return hqe_info


def abort_special_tasks(**filter_data):
    """\
    Abort the special task, or tasks, specified in the filter.
    """
    query = models.SpecialTask.query_objects(filter_data)
    special_tasks = query.filter(is_active=True)
    for task in special_tasks:
        task.abort()


def _call_special_tasks_on_hosts(task, hosts):
    """\
    Schedules a set of hosts for a special task.

    @returns A list of hostnames that a special task was created for.
    """
    models.AclGroup.check_for_acl_violation_hosts(hosts)
    shard_host_map = rpc_utils.bucket_hosts_by_shard(hosts)
    if shard_host_map and not utils.is_shard():
        raise ValueError('The following hosts are on shards, please '
                         'follow the link to the shards and create jobs '
                         'there instead. %s.' % shard_host_map)
    for host in hosts:
        models.SpecialTask.schedule_special_task(host, task)
    return list(sorted(host.hostname for host in hosts))


def _forward_special_tasks_on_hosts(task, rpc, **filter_data):
    """Forward special tasks to corresponding shards.

    For master, when special tasks are fired on hosts that are sharded,
    forward the RPC to corresponding shards.

    For shard, create special task records in local DB.

    @param task: Enum value of frontend.afe.models.SpecialTask.Task
    @param rpc: RPC name to forward.
    @param filter_data: Filter keywords to be used for DB query.

    @return: A list of hostnames that a special task was created for.
    """
    hosts = models.Host.query_objects(filter_data)
    shard_host_map = rpc_utils.bucket_hosts_by_shard(hosts)

    # Filter out hosts on a shard from those on the master, forward
    # rpcs to the shard with an additional hostname__in filter, and
    # create a local SpecialTask for each remaining host.
    if shard_host_map and not utils.is_shard():
        hosts = [h for h in hosts if h.shard is None]
        for shard, hostnames in shard_host_map.iteritems():

            # The main client of this module is the frontend website, and
            # it invokes it with an 'id' or an 'id__in' filter. Regardless,
            # the 'hostname' filter should narrow down the list of hosts on
            # each shard even though we supply all the ids in filter_data.
            # This method uses hostname instead of id because it fits better
            # with the overall architecture of redirection functions in
            # rpc_utils.
            shard_filter = filter_data.copy()
            shard_filter['hostname__in'] = hostnames
            rpc_utils.run_rpc_on_multiple_hostnames(
                    rpc, [shard], **shard_filter)

    # There is a race condition here if someone assigns a shard to one of these
    # hosts before we create the task. The host will stay on the master if:
    # 1. The host is not Ready
    # 2. The host is Ready but has a task
    # But if the host is Ready and doesn't have a task yet, it will get sent
    # to the shard as we're creating a task here.

    # Given that we only rarely verify Ready hosts it isn't worth putting this
    # entire method in a transaction. The worst case scenario is that we have
    # a verify running on a Ready host while the shard is using it, if the
    # verify fails no subsequent tasks will be created against the host on the
    # master, and verifies are safe enough that this is OK.
    return _call_special_tasks_on_hosts(task, hosts)


def reverify_hosts(**filter_data):
    """\
    Schedules a set of hosts for verify.

    @returns A list of hostnames that a verify task was created for.
    """
    return _forward_special_tasks_on_hosts(
            models.SpecialTask.Task.VERIFY, 'reverify_hosts', **filter_data)


def repair_hosts(**filter_data):
    """\
    Schedules a set of hosts for repair.

    @returns A list of hostnames that a repair task was created for.
    """
    return _forward_special_tasks_on_hosts(
            models.SpecialTask.Task.REPAIR, 'repair_hosts', **filter_data)


def get_jobs(not_yet_run=False, running=False, finished=False,
             suite=False, sub=False, standalone=False, **filter_data):
    """\
    Extra status filter args for get_jobs:
    -not_yet_run: Include only jobs that have not yet started running.
    -running: Include only jobs that have start running but for which not
    all hosts have completed.
    -finished: Include only jobs for which all hosts have completed (or
    aborted).

    Extra type filter args for get_jobs:
    -suite: Include only jobs with child jobs.
    -sub: Include only jobs with a parent job.
    -standalone: Inlcude only jobs with no child or parent jobs.
    At most one of these three fields should be specified.
    """
    extra_args = rpc_utils.extra_job_status_filters(not_yet_run,
                                                    running,
                                                    finished)
    filter_data['extra_args'] = rpc_utils.extra_job_type_filters(extra_args,
                                                                 suite,
                                                                 sub,
                                                                 standalone)
    job_dicts = []
    jobs = list(models.Job.query_objects(filter_data))
    models.Job.objects.populate_relationships(jobs, models.Label,
                                              'dependencies')
    models.Job.objects.populate_relationships(jobs, models.JobKeyval, 'keyvals')
    for job in jobs:
        job_dict = job.get_object_dict()
        job_dict['dependencies'] = ','.join(label.name
                                            for label in job.dependencies)
        job_dict['keyvals'] = dict((keyval.key, keyval.value)
                                   for keyval in job.keyvals)
        job_dicts.append(job_dict)
    return rpc_utils.prepare_for_serialization(job_dicts)


def get_num_jobs(not_yet_run=False, running=False, finished=False,
                 suite=False, sub=False, standalone=False,
                 **filter_data):
    """\
    See get_jobs() for documentation of extra filter parameters.
    """
    extra_args = rpc_utils.extra_job_status_filters(not_yet_run,
                                                    running,
                                                    finished)
    filter_data['extra_args'] = rpc_utils.extra_job_type_filters(extra_args,
                                                                 suite,
                                                                 sub,
                                                                 standalone)
    return models.Job.query_count(filter_data)


def get_jobs_summary(**filter_data):
    """\
    Like get_jobs(), but adds 'status_counts' and 'result_counts' field.

    'status_counts' filed is a dictionary mapping status strings to the number
    of hosts currently with that status, i.e. {'Queued' : 4, 'Running' : 2}.

    'result_counts' field is piped to tko's rpc_interface and has the return
    format specified under get_group_counts.
    """
    jobs = get_jobs(**filter_data)
    ids = [job['id'] for job in jobs]
    all_status_counts = models.Job.objects.get_status_counts(ids)
    for job in jobs:
        job['status_counts'] = all_status_counts[job['id']]
        job['result_counts'] = tko_rpc_interface.get_status_counts(
                ['afe_job_id', 'afe_job_id'],
                header_groups=[['afe_job_id'], ['afe_job_id']],
                **{'afe_job_id': job['id']})
    return rpc_utils.prepare_for_serialization(jobs)


def get_info_for_clone(id, preserve_metahosts, queue_entry_filter_data=None):
    """\
    Retrieves all the information needed to clone a job.
    """
    job = models.Job.objects.get(id=id)
    job_info = rpc_utils.get_job_info(job,
                                      preserve_metahosts,
                                      queue_entry_filter_data)

    host_dicts = []
    for host in job_info['hosts']:
        host_dict = get_hosts(id=host.id)[0]
        other_labels = host_dict['labels']
        if host_dict['platform']:
            other_labels.remove(host_dict['platform'])
        host_dict['other_labels'] = ', '.join(other_labels)
        host_dicts.append(host_dict)

    for host in job_info['one_time_hosts']:
        host_dict = dict(hostname=host.hostname,
                         id=host.id,
                         platform='(one-time host)',
                         locked_text='')
        host_dicts.append(host_dict)

    # convert keys from Label objects to strings (names of labels)
    meta_host_counts = dict((meta_host.name, count) for meta_host, count
                            in job_info['meta_host_counts'].iteritems())

    info = dict(job=job.get_object_dict(),
                meta_host_counts=meta_host_counts,
                hosts=host_dicts)
    info['job']['dependencies'] = job_info['dependencies']
    info['hostless'] = job_info['hostless']
    info['drone_set'] = job.drone_set and job.drone_set.name

    image = _get_image_for_job(job, job_info['hostless'])
    if image:
        info['job']['image'] = image

    return rpc_utils.prepare_for_serialization(info)


def _get_image_for_job(job, hostless):
    """Gets the image used for a job.

    Gets the image used for an AFE job from the job's keyvals 'build' or
    'builds'. If that fails, and the job is a hostless job, tries to
    get the image from its control file attributes 'build' or 'builds'.

    TODO(ntang): Needs to handle FAFT with two builds for ro/rw.

    @param job      An AFE job object.
    @param hostless Boolean indicating whether the job is hostless.

    @returns The image build used for the job.
    """
    keyvals = job.keyval_dict()
    image = keyvals.get('build')
    if not image:
        value = keyvals.get('builds')
        builds = None
        if isinstance(value, dict):
            builds = value
        elif isinstance(value, basestring):
            builds = ast.literal_eval(value)
        if builds:
            image = builds.get('cros-version')
    if not image and hostless and job.control_file:
        try:
            control_obj = control_data.parse_control_string(
                    job.control_file)
            if hasattr(control_obj, 'build'):
                image = getattr(control_obj, 'build')
            if not image and hasattr(control_obj, 'builds'):
                builds = getattr(control_obj, 'builds')
                image = builds.get('cros-version')
        except:
            logging.warning('Failed to parse control file for job: %s',
                            job.name)
    return image


def get_host_queue_entries_by_insert_time(
    insert_time_after=None, insert_time_before=None, **filter_data):
    """Like get_host_queue_entries, but using the insert index table.

    @param insert_time_after: A lower bound on insert_time
    @param insert_time_before: An upper bound on insert_time
    @returns A sequence of nested dictionaries of host and job information.
    """
    assert insert_time_after is not None or insert_time_before is not None, \
      ('Caller to get_host_queue_entries_by_insert_time must provide either'
       ' insert_time_after or insert_time_before.')
    # Get insert bounds on the index of the host queue entries.
    if insert_time_after:
        query = models.HostQueueEntryStartTimes.objects.filter(
            # Note: '-insert_time' means descending. We want the largest
            # insert time smaller than the insert time.
            insert_time__lte=insert_time_after).order_by('-insert_time')
        try:
            constraint = query[0].highest_hqe_id
            if 'id__gte' in filter_data:
                constraint = max(constraint, filter_data['id__gte'])
            filter_data['id__gte'] = constraint
        except IndexError:
            pass

    # Get end bounds.
    if insert_time_before:
        query = models.HostQueueEntryStartTimes.objects.filter(
            insert_time__gte=insert_time_before).order_by('insert_time')
        try:
            constraint = query[0].highest_hqe_id
            if 'id__lte' in filter_data:
                constraint = min(constraint, filter_data['id__lte'])
            filter_data['id__lte'] = constraint
        except IndexError:
            pass

    return rpc_utils.prepare_rows_as_nested_dicts(
            models.HostQueueEntry.query_objects(filter_data),
            ('host', 'job'))


def get_host_queue_entries(start_time=None, end_time=None, **filter_data):
    """\
    @returns A sequence of nested dictionaries of host and job information.
    """
    filter_data = rpc_utils.inject_times_to_filter('started_on__gte',
                                                   'started_on__lte',
                                                   start_time,
                                                   end_time,
                                                   **filter_data)
    return rpc_utils.prepare_rows_as_nested_dicts(
            models.HostQueueEntry.query_objects(filter_data),
            ('host', 'job'))


def get_num_host_queue_entries(start_time=None, end_time=None, **filter_data):
    """\
    Get the number of host queue entries associated with this job.
    """
    filter_data = rpc_utils.inject_times_to_filter('started_on__gte',
                                                   'started_on__lte',
                                                   start_time,
                                                   end_time,
                                                   **filter_data)
    return models.HostQueueEntry.query_count(filter_data)


def get_hqe_percentage_complete(**filter_data):
    """
    Computes the fraction of host queue entries matching the given filter data
    that are complete.
    """
    query = models.HostQueueEntry.query_objects(filter_data)
    complete_count = query.filter(complete=True).count()
    total_count = query.count()
    if total_count == 0:
        return 1
    return float(complete_count) / total_count


# special tasks

def get_special_tasks(**filter_data):
    """Get special task entries from the local database.

    Query the special tasks table for tasks matching the given
    `filter_data`, and return a list of the results.  No attempt is
    made to forward the call to shards; the buck will stop here.
    The caller is expected to know the target shard for such reasons
    as:
      * The caller is a service (such as gs_offloader) configured
        to operate on behalf of one specific shard, and no other.
      * The caller has a host as a parameter, and knows that this is
        the shard assigned to that host.

    @param filter_data  Filter keywords to pass to the underlying
                        database query.

    """
    return rpc_utils.prepare_rows_as_nested_dicts(
            models.SpecialTask.query_objects(filter_data),
            ('host', 'queue_entry'))


def get_host_special_tasks(host_id, **filter_data):
    """Get special task entries for a given host.

    Query the special tasks table for tasks that ran on the host
    given by `host_id` and matching the given `filter_data`.
    Return a list of the results.  If the host is assigned to a
    shard, forward this call to that shard.

    @param host_id      Id in the database of the target host.
    @param filter_data  Filter keywords to pass to the underlying
                        database query.

    """
    # Retrieve host data even if the host is in an invalid state.
    host = models.Host.smart_get(host_id, False)
    if not host.shard:
        return get_special_tasks(host_id=host_id, **filter_data)
    else:
        # The return values from AFE methods are post-processed
        # objects that aren't JSON-serializable.  So, we have to
        # call AFE.run() to get the raw, serializable output from
        # the shard.
        shard_afe = frontend.AFE(server=host.shard.hostname)
        return shard_afe.run('get_special_tasks',
                             host_id=host_id, **filter_data)


def get_num_special_tasks(**kwargs):
    """Get the number of special task entries from the local database.

    Query the special tasks table for tasks matching the given 'kwargs',
    and return the number of the results. No attempt is made to forward
    the call to shards; the buck will stop here.

    @param kwargs    Filter keywords to pass to the underlying database query.

    """
    return models.SpecialTask.query_count(kwargs)


def get_host_num_special_tasks(host, **kwargs):
    """Get special task entries for a given host.

    Query the special tasks table for tasks that ran on the host
    given by 'host' and matching the given 'kwargs'.
    Return a list of the results.  If the host is assigned to a
    shard, forward this call to that shard.

    @param host      id or name of a host. More often a hostname.
    @param kwargs    Filter keywords to pass to the underlying database query.

    """
    # Retrieve host data even if the host is in an invalid state.
    host_model = models.Host.smart_get(host, False)
    if not host_model.shard:
        return get_num_special_tasks(host=host, **kwargs)
    else:
        shard_afe = frontend.AFE(server=host_model.shard.hostname)
        return shard_afe.run('get_num_special_tasks', host=host, **kwargs)


def get_status_task(host_id, end_time):
    """Get the "status task" for a host from the local shard.

    Returns a single special task representing the given host's
    "status task".  The status task is a completed special task that
    identifies whether the corresponding host was working or broken
    when it completed.  A successful task indicates a working host;
    a failed task indicates broken.

    This call will not be forward to a shard; the receiving server
    must be the shard that owns the host.

    @param host_id      Id in the database of the target host.
    @param end_time     Time reference for the host's status.

    @return A single task; its status (successful or not)
            corresponds to the status of the host (working or
            broken) at the given time.  If no task is found, return
            `None`.

    """
    tasklist = rpc_utils.prepare_rows_as_nested_dicts(
            status_history.get_status_task(host_id, end_time),
            ('host', 'queue_entry'))
    return tasklist[0] if tasklist else None


def get_host_status_task(host_id, end_time):
    """Get the "status task" for a host from its owning shard.

    Finds the given host's owning shard, and forwards to it a call
    to `get_status_task()` (see above).

    @param host_id      Id in the database of the target host.
    @param end_time     Time reference for the host's status.

    @return A single task; its status (successful or not)
            corresponds to the status of the host (working or
            broken) at the given time.  If no task is found, return
            `None`.

    """
    host = models.Host.smart_get(host_id)
    if not host.shard:
        return get_status_task(host_id, end_time)
    else:
        # The return values from AFE methods are post-processed
        # objects that aren't JSON-serializable.  So, we have to
        # call AFE.run() to get the raw, serializable output from
        # the shard.
        shard_afe = frontend.AFE(server=host.shard.hostname)
        return shard_afe.run('get_status_task',
                             host_id=host_id, end_time=end_time)


def get_host_diagnosis_interval(host_id, end_time, success):
    """Find a "diagnosis interval" for a given host.

    A "diagnosis interval" identifies a start and end time where
    the host went from "working" to "broken", or vice versa.  The
    interval's starting time is the starting time of the last status
    task with the old status; the end time is the finish time of the
    first status task with the new status.

    This routine finds the most recent diagnosis interval for the
    given host prior to `end_time`, with a starting status matching
    `success`.  If `success` is true, the interval will start with a
    successful status task; if false the interval will start with a
    failed status task.

    @param host_id      Id in the database of the target host.
    @param end_time     Time reference for the diagnosis interval.
    @param success      Whether the diagnosis interval should start
                        with a successful or failed status task.

    @return A list of two strings.  The first is the timestamp for
            the beginning of the interval; the second is the
            timestamp for the end.  If the host has never changed
            state, the list is empty.

    """
    host = models.Host.smart_get(host_id)
    if not host.shard or utils.is_shard():
        return status_history.get_diagnosis_interval(
                host_id, end_time, success)
    else:
        shard_afe = frontend.AFE(server=host.shard.hostname)
        return shard_afe.get_host_diagnosis_interval(
                host_id, end_time, success)


# support for host detail view

def get_host_queue_entries_and_special_tasks(host, query_start=None,
                                             query_limit=None, start_time=None,
                                             end_time=None):
    """
    @returns an interleaved list of HostQueueEntries and SpecialTasks,
            in approximate run order.  each dict contains keys for type, host,
            job, status, started_on, execution_path, and ID.
    """
    total_limit = None
    if query_limit is not None:
        total_limit = query_start + query_limit
    filter_data_common = {'host': host,
                          'query_limit': total_limit,
                          'sort_by': ['-id']}

    filter_data_special_tasks = rpc_utils.inject_times_to_filter(
            'time_started__gte', 'time_started__lte', start_time, end_time,
            **filter_data_common)

    queue_entries = get_host_queue_entries(
            start_time, end_time, **filter_data_common)
    special_tasks = get_host_special_tasks(host, **filter_data_special_tasks)

    interleaved_entries = rpc_utils.interleave_entries(queue_entries,
                                                       special_tasks)
    if query_start is not None:
        interleaved_entries = interleaved_entries[query_start:]
    if query_limit is not None:
        interleaved_entries = interleaved_entries[:query_limit]
    return rpc_utils.prepare_host_queue_entries_and_special_tasks(
            interleaved_entries, queue_entries)


def get_num_host_queue_entries_and_special_tasks(host, start_time=None,
                                                 end_time=None):
    filter_data_common = {'host': host}

    filter_data_queue_entries, filter_data_special_tasks = (
            rpc_utils.inject_times_to_hqe_special_tasks_filters(
                    filter_data_common, start_time, end_time))

    return (models.HostQueueEntry.query_count(filter_data_queue_entries)
            + get_host_num_special_tasks(**filter_data_special_tasks))


# other

def echo(data=""):
    """\
    Returns a passed in string. For doing a basic test to see if RPC calls
    can successfully be made.
    """
    return data


def get_motd():
    """\
    Returns the message of the day as a string.
    """
    return rpc_utils.get_motd()


def get_static_data():
    """\
    Returns a dictionary containing a bunch of data that shouldn't change
    often and is otherwise inaccessible.  This includes:

    priorities: List of job priority choices.
    default_priority: Default priority value for new jobs.
    users: Sorted list of all users.
    labels: Sorted list of labels not start with 'cros-version' and
            'fw-version'.
    tests: Sorted list of all tests.
    profilers: Sorted list of all profilers.
    current_user: Logged-in username.
    host_statuses: Sorted list of possible Host statuses.
    job_statuses: Sorted list of possible HostQueueEntry statuses.
    job_timeout_default: The default job timeout length in minutes.
    parse_failed_repair_default: Default value for the parse_failed_repair job
            option.
    reboot_before_options: A list of valid RebootBefore string enums.
    reboot_after_options: A list of valid RebootAfter string enums.
    motd: Server's message of the day.
    status_dictionary: A mapping from one word job status names to a more
            informative description.
    """

    default_drone_set_name = models.DroneSet.default_drone_set_name()
    drone_sets = ([default_drone_set_name] +
                  sorted(drone_set.name for drone_set in
                         models.DroneSet.objects.exclude(
                                 name=default_drone_set_name)))

    result = {}
    result['priorities'] = priorities.Priority.choices()
    result['default_priority'] = 'Default'
    result['max_schedulable_priority'] = priorities.Priority.DEFAULT
    result['users'] = get_users(sort_by=['login'])

    label_exclude_filters = [{'name__startswith': 'cros-version'},
                             {'name__startswith': 'fw-version'},
                             {'name__startswith': 'fwrw-version'},
                             {'name__startswith': 'fwro-version'},
                             {'name__startswith': 'ab-version'},
                             {'name__startswith': 'testbed-version'}]
    result['labels'] = get_labels(
        label_exclude_filters,
        sort_by=['-platform', 'name'])

    result['tests'] = get_tests(sort_by=['name'])
    result['profilers'] = get_profilers(sort_by=['name'])
    result['current_user'] = rpc_utils.prepare_for_serialization(
        models.User.current_user().get_object_dict())
    result['host_statuses'] = sorted(models.Host.Status.names)
    result['job_statuses'] = sorted(models.HostQueueEntry.Status.names)
    result['job_timeout_mins_default'] = models.Job.DEFAULT_TIMEOUT_MINS
    result['job_max_runtime_mins_default'] = (
        models.Job.DEFAULT_MAX_RUNTIME_MINS)
    result['parse_failed_repair_default'] = bool(
        models.Job.DEFAULT_PARSE_FAILED_REPAIR)
    result['reboot_before_options'] = model_attributes.RebootBefore.names
    result['reboot_after_options'] = model_attributes.RebootAfter.names
    result['motd'] = rpc_utils.get_motd()
    result['drone_sets_enabled'] = models.DroneSet.drone_sets_enabled()
    result['drone_sets'] = drone_sets

    result['status_dictionary'] = {"Aborted": "Aborted",
                                   "Verifying": "Verifying Host",
                                   "Provisioning": "Provisioning Host",
                                   "Pending": "Waiting on other hosts",
                                   "Running": "Running autoserv",
                                   "Completed": "Autoserv completed",
                                   "Failed": "Failed to complete",
                                   "Queued": "Queued",
                                   "Starting": "Next in host's queue",
                                   "Stopped": "Other host(s) failed verify",
                                   "Parsing": "Awaiting parse of final results",
                                   "Gathering": "Gathering log files",
                                   "Waiting": "Waiting for scheduler action",
                                   "Archiving": "Archiving results",
                                   "Resetting": "Resetting hosts"}

    result['wmatrix_url'] = rpc_utils.get_wmatrix_url()
    result['stainless_url'] = rpc_utils.get_stainless_url()
    result['is_moblab'] = bool(utils.is_moblab())

    return result


def get_server_time():
    return datetime.datetime.now().strftime("%Y-%m-%d %H:%M")


def ping_db():
    """Simple connection test to db"""
    try:
        db_connection.cursor()
    except DatabaseError:
        return [False]
    return [True]


def get_hosts_by_attribute(attribute, value):
    """
    Get the list of valid hosts that share the same host attribute value.

    @param attribute: String of the host attribute to check.
    @param value: String of the value that is shared between hosts.

    @returns List of hostnames that all have the same host attribute and
             value.
    """
    rows = models.HostAttribute.query_objects({'attribute': attribute,
                                               'value': value})
    if RESPECT_STATIC_ATTRIBUTES:
        returned_hosts = set()
        # Add hosts:
        #     * Non-valid
        #     * Exist in afe_host_attribute with given attribute.
        #     * Don't exist in afe_static_host_attribute OR exist in
        #       afe_static_host_attribute with the same given value.
        for row in rows:
            if row.host.invalid != 0:
                continue

            static_hosts = models.StaticHostAttribute.query_objects(
                {'host_id': row.host.id, 'attribute': attribute})
            values = [static_host.value for static_host in static_hosts]
            if len(values) == 0 or values[0] == value:
                returned_hosts.add(row.host.hostname)

        # Add hosts:
        #     * Non-valid
        #     * Exist in afe_static_host_attribute with given attribute
        #       and value
        #     * No need to check whether each static attribute has its
        #       corresponding entry in afe_host_attribute since it is ensured
        #       in inventory sync.
        static_rows = models.StaticHostAttribute.query_objects(
                {'attribute': attribute, 'value': value})
        for row in static_rows:
            if row.host.invalid != 0:
                continue

            returned_hosts.add(row.host.hostname)

        return list(returned_hosts)
    else:
        return [row.host.hostname for row in rows if row.host.invalid == 0]


def canonicalize_suite_name(suite_name):
    """Canonicalize the suite's name.

    @param suite_name: the name of the suite.
    """
    # Do not change this naming convention without updating
    # site_utils.parse_job_name.
    return 'test_suites/control.%s' % suite_name


def formatted_now():
    """Format the current datetime."""
    return datetime.datetime.now().strftime(time_utils.TIME_FMT)


def _get_control_file_by_build(build, ds, suite_name):
    """Return control file contents for |suite_name|.

    Query the dev server at |ds| for the control file |suite_name|, included
    in |build| for |board|.

    @param build: unique name by which to refer to the image from now on.
    @param ds: a dev_server.DevServer instance to fetch control file with.
    @param suite_name: canonicalized suite name, e.g. test_suites/control.bvt.
    @raises ControlFileNotFound if a unique suite control file doesn't exist.
    @raises NoControlFileList if we can't list the control files at all.
    @raises ControlFileEmpty if the control file exists on the server, but
                             can't be read.

    @return the contents of the desired control file.
    """
    getter = control_file_getter.DevServerGetter.create(build, ds)
    devserver_name = ds.hostname
    # Get the control file for the suite.
    try:
        control_file_in = getter.get_control_file_contents_by_name(suite_name)
    except error.CrosDynamicSuiteException as e:
        raise type(e)('Failed to get control file for %s '
                      '(devserver: %s) (error: %s)' %
                      (build, devserver_name, e))
    if not control_file_in:
        raise error.ControlFileEmpty(
            "Fetching %s returned no data. (devserver: %s)" %
            (suite_name, devserver_name))
    # Force control files to only contain ascii characters.
    try:
        control_file_in.encode('ascii')
    except UnicodeDecodeError as e:
        raise error.ControlFileMalformed(str(e))

    return control_file_in


def _get_control_file_by_suite(suite_name):
    """Get control file contents by suite name.

    @param suite_name: Suite name as string.
    @returns: Control file contents as string.
    """
    getter = control_file_getter.FileSystemGetter(
            [_CONFIG.get_config_value('SCHEDULER',
                                      'drone_installation_directory')])
    return getter.get_control_file_contents_by_name(suite_name)


def _stage_build_artifacts(build, hostname=None):
    """
    Ensure components of |build| necessary for installing images are staged.

    @param build image we want to stage.
    @param hostname hostname of a dut may run test on. This is to help to locate
        a devserver closer to duts if needed. Default is None.

    @raises StageControlFileFailure: if the dev server throws 500 while staging
        suite control files.

    @return: dev_server.ImageServer instance to use with this build.
    @return: timings dictionary containing staging start/end times.
    """
    timings = {}
    # Ensure components of |build| necessary for installing images are staged
    # on the dev server. However set synchronous to False to allow other
    # components to be downloaded in the background.
    ds = dev_server.resolve(build, hostname=hostname)
    ds_name = ds.hostname
    timings[constants.DOWNLOAD_STARTED_TIME] = formatted_now()
    try:
        ds.stage_artifacts(image=build, artifacts=['test_suites'])
    except dev_server.DevServerException as e:
        raise error.StageControlFileFailure(
                "Failed to stage %s on %s: %s" % (build, ds_name, e))
    timings[constants.PAYLOAD_FINISHED_TIME] = formatted_now()
    return (ds, timings)


@rpc_utils.route_rpc_to_master
def create_suite_job(
        name='',
        board='',
        pool='',
        child_dependencies=(),
        control_file='',
        check_hosts=True,
        num=None,
        file_bugs=False,
        timeout=24,
        timeout_mins=None,
        priority=priorities.Priority.DEFAULT,
        suite_args=None,
        wait_for_results=True,
        job_retry=False,
        max_retries=None,
        max_runtime_mins=None,
        suite_min_duts=0,
        offload_failures_only=False,
        builds=None,
        test_source_build=None,
        run_prod_code=False,
        delay_minutes=0,
        is_cloning=False,
        job_keyvals=None,
        test_args=None,
        **kwargs):
    """
    Create a job to run a test suite on the given device with the given image.

    When the timeout specified in the control file is reached, the
    job is guaranteed to have completed and results will be available.

    @param name: The test name if control_file is supplied, otherwise the name
                 of the test suite to run, e.g. 'bvt'.
    @param board: the kind of device to run the tests on.
    @param builds: the builds to install e.g.
                   {'cros-version:': 'x86-alex-release/R18-1655.0.0',
                    'fwrw-version:':  'x86-alex-firmware/R36-5771.50.0',
                    'fwro-version:':  'x86-alex-firmware/R36-5771.49.0'}
                   If builds is given a value, it overrides argument build.
    @param test_source_build: Build that contains the server-side test code.
    @param pool: Specify the pool of machines to use for scheduling
            purposes.
    @param child_dependencies: (optional) list of additional dependency labels
            (strings) that will be added as dependency labels to child jobs.
    @param control_file: the control file of the job.
    @param check_hosts: require appropriate live hosts to exist in the lab.
    @param num: Specify the number of machines to schedule across (integer).
                Leave unspecified or use None to use default sharding factor.
    @param file_bugs: File a bug on each test failure in this suite.
    @param timeout: The max lifetime of this suite, in hours.
    @param timeout_mins: The max lifetime of this suite, in minutes. Takes
                         priority over timeout.
    @param priority: Integer denoting priority. Higher is more important.
    @param suite_args: Optional arguments which will be parsed by the suite
                       control file. Used by control.test_that_wrapper to
                       determine which tests to run.
    @param wait_for_results: Set to False to run the suite job without waiting
                             for test jobs to finish. Default is True.
    @param job_retry: Set to True to enable job-level retry. Default is False.
    @param max_retries: Integer, maximum job retries allowed at suite level.
                        None for no max.
    @param max_runtime_mins: Maximum amount of time a job can be running in
                             minutes.
    @param suite_min_duts: Integer. Scheduler will prioritize getting the
                           minimum number of machines for the suite when it is
                           competing with another suite that has a higher
                           priority but already got minimum machines it needs.
    @param offload_failures_only: Only enable gs_offloading for failed jobs.
    @param run_prod_code: If True, the suite will run the test code that
                          lives in prod aka the test code currently on the
                          lab servers. If False, the control files and test
                          code for this suite run will be retrieved from the
                          build artifacts.
    @param delay_minutes: Delay the creation of test jobs for a given number of
                          minutes.
    @param is_cloning: True if creating a cloning job.
    @param job_keyvals: A dict of job keyvals to be inject to control file.
    @param test_args: A dict of args passed all the way to each individual test
                      that will be actually run.
    @param kwargs: extra keyword args. NOT USED.

    @raises ControlFileNotFound: if a unique suite control file doesn't exist.
    @raises NoControlFileList: if we can't list the control files at all.
    @raises StageControlFileFailure: If the dev server throws 500 while
                                     staging test_suites.
    @raises ControlFileEmpty: if the control file exists on the server, but
                              can't be read.

    @return: the job ID of the suite; -1 on error.
    """
    if num is not None:
        warnings.warn('num is deprecated for create_suite_job')
    del num

    if builds is None:
        builds = {}

    # Default test source build to CrOS build if it's not specified and
    # run_prod_code is set to False.
    if not run_prod_code:
        test_source_build = Suite.get_test_source_build(
                builds, test_source_build=test_source_build)

    sample_dut = rpc_utils.get_sample_dut(board, pool)

    suite_name = canonicalize_suite_name(name)
    if run_prod_code:
        ds = dev_server.resolve(test_source_build, hostname=sample_dut)
        keyvals = {}
    else:
        (ds, keyvals) = _stage_build_artifacts(
                test_source_build, hostname=sample_dut)
    keyvals[constants.SUITE_MIN_DUTS_KEY] = suite_min_duts

    # Do not change this naming convention without updating
    # site_utils.parse_job_name.
    if run_prod_code:
        # If run_prod_code is True, test_source_build is not set, use the
        # first build in the builds list for the sutie job name.
        name = '%s-%s' % (builds.values()[0], suite_name)
    else:
        name = '%s-%s' % (test_source_build, suite_name)

    timeout_mins = timeout_mins or timeout * 60
    max_runtime_mins = max_runtime_mins or timeout * 60

    if not board:
        board = utils.ParseBuildName(builds[provision.CROS_VERSION_PREFIX])[0]

    if run_prod_code:
        control_file = _get_control_file_by_suite(suite_name)

    if not control_file:
        # No control file was supplied so look it up from the build artifacts.
        control_file = _get_control_file_by_build(
                test_source_build, ds, suite_name)

    # Prepend builds and board to the control file.
    if is_cloning:
        control_file = tools.remove_injection(control_file)

    if suite_args is None:
        suite_args = dict()

    # TODO(crbug.com/758427): suite_args_raw is needed to run old tests.
    # Can be removed after R64.
    if 'tests' in suite_args:
        # TODO(crbug.com/758427): test_that used to have its own
        # snowflake implementation of parsing command line arguments in
        # the test
        suite_args_raw = ' '.join([':lab:'] + suite_args['tests'])
    # TODO(crbug.com/760675): Needed for CTS/GTS as above, but when
    # 'tests' is not passed.  Can be removed after R64.
    elif name.rpartition('/')[-1] in {'control.cts_N',
                                      'control.cts_N_preconditions',
                                      'control.gts'}:
        suite_args_raw = ''
    else:
        # TODO(crbug.com/758427): This is for suite_attr_wrapper.  Can
        # be removed after R64.
        suite_args_raw = repr(suite_args)

    inject_dict = {
        'board': board,
        # `build` is needed for suites like AU to stage image inside suite
        # control file.
        'build': test_source_build,
        'builds': builds,
        'check_hosts': check_hosts,
        'pool': pool,
        'child_dependencies': child_dependencies,
        'file_bugs': file_bugs,
        'timeout': timeout,
        'timeout_mins': timeout_mins,
        'devserver_url': ds.url(),
        'priority': priority,
        # TODO(crbug.com/758427): injecting suite_args is needed to run
        # old tests
        'suite_args' : suite_args_raw,
        'wait_for_results': wait_for_results,
        'job_retry': job_retry,
        'max_retries': max_retries,
        'max_runtime_mins': max_runtime_mins,
        'offload_failures_only': offload_failures_only,
        'test_source_build': test_source_build,
        'run_prod_code': run_prod_code,
        'delay_minutes': delay_minutes,
        'job_keyvals': job_keyvals,
        'test_args': test_args,
    }
    inject_dict.update(suite_args)
    control_file = tools.inject_vars(inject_dict, control_file)

    return rpc_utils.create_job_common(name,
                                       priority=priority,
                                       timeout_mins=timeout_mins,
                                       max_runtime_mins=max_runtime_mins,
                                       control_type='Server',
                                       control_file=control_file,
                                       hostless=True,
                                       keyvals=keyvals)


def get_job_history(**filter_data):
    """Get history of the job, including the special tasks executed for the job

    @param filter_data: filter for the call, should at least include
                        {'job_id': [job id]}
    @returns: JSON string of the job's history, including the information such
              as the hosts run the job and the special tasks executed before
              and after the job.
    """
    job_id = filter_data['job_id']
    job_info = job_history.get_job_info(job_id)
    return rpc_utils.prepare_for_serialization(job_info.get_history())


def get_host_history(start_time, end_time, hosts=None, board=None, pool=None):
    """Deprecated."""
    raise ValueError('get_host_history rpc is deprecated '
                     'and no longer implemented.')


def shard_heartbeat(shard_hostname, jobs=(), hqes=(), known_job_ids=(),
                    known_host_ids=(), known_host_statuses=()):
    """Receive updates for job statuses from shards and assign hosts and jobs.

    @param shard_hostname: Hostname of the calling shard
    @param jobs: Jobs in serialized form that should be updated with newer
                 status from a shard.
    @param hqes: Hostqueueentries in serialized form that should be updated with
                 newer status from a shard. Note that for every hostqueueentry
                 the corresponding job must be in jobs.
    @param known_job_ids: List of ids of jobs the shard already has.
    @param known_host_ids: List of ids of hosts the shard already has.
    @param known_host_statuses: List of statuses of hosts the shard already has.

    @returns: Serialized representations of hosts, jobs, suite job keyvals
              and their dependencies to be inserted into a shard's database.
    """
    # The following alternatives to sending host and job ids in every heartbeat
    # have been considered:
    # 1. Sending the highest known job and host ids. This would work for jobs:
    #    Newer jobs always have larger ids. Also, if a job is not assigned to a
    #    particular shard during a heartbeat, it never will be assigned to this
    #    shard later.
    #    This is not true for hosts though: A host that is leased won't be sent
    #    to the shard now, but might be sent in a future heartbeat. This means
    #    sometimes hosts should be transfered that have a lower id than the
    #    maximum host id the shard knows.
    # 2. Send the number of jobs/hosts the shard knows to the master in each
    #    heartbeat. Compare these to the number of records that already have
    #    the shard_id set to this shard. In the normal case, they should match.
    #    In case they don't, resend all entities of that type.
    #    This would work well for hosts, because there aren't that many.
    #    Resending all jobs is quite a big overhead though.
    #    Also, this approach might run into edge cases when entities are
    #    ever deleted.
    # 3. Mixtures of the above: Use 1 for jobs and 2 for hosts.
    #    Using two different approaches isn't consistent and might cause
    #    confusion. Also the issues with the case of deletions might still
    #    occur.
    #
    # The overhead of sending all job and host ids in every heartbeat is low:
    # At peaks one board has about 1200 created but unfinished jobs.
    # See the numbers here: http://goo.gl/gQCGWH
    # Assuming that job id's have 6 digits and that json serialization takes a
    # comma and a space as overhead, the traffic per id sent is about 8 bytes.
    # If 5000 ids need to be sent, this means 40 kilobytes of traffic.
    # A NOT IN query with 5000 ids took about 30ms in tests made.
    # These numbers seem low enough to outweigh the disadvantages of the
    # solutions described above.
    shard_obj = rpc_utils.retrieve_shard(shard_hostname=shard_hostname)
    rpc_utils.persist_records_sent_from_shard(shard_obj, jobs, hqes)
    assert len(known_host_ids) == len(known_host_statuses)
    for i in range(len(known_host_ids)):
        host_model = models.Host.objects.get(pk=known_host_ids[i])
        if host_model.status != known_host_statuses[i]:
            host_model.status = known_host_statuses[i]
            host_model.save()

    hosts, jobs, suite_keyvals, inc_ids = rpc_utils.find_records_for_shard(
            shard_obj, known_job_ids=known_job_ids,
            known_host_ids=known_host_ids)
    return {
        'hosts': [host.serialize() for host in hosts],
        'jobs': [job.serialize() for job in jobs],
        'suite_keyvals': [kv.serialize() for kv in suite_keyvals],
        'incorrect_host_ids': [int(i) for i in inc_ids],
    }


def get_shards(**filter_data):
    """Return a list of all shards.

    @returns A sequence of nested dictionaries of shard information.
    """
    shards = models.Shard.query_objects(filter_data)
    serialized_shards = rpc_utils.prepare_rows_as_nested_dicts(shards, ())
    for serialized, shard in zip(serialized_shards, shards):
        serialized['labels'] = [label.name for label in shard.labels.all()]

    return serialized_shards


def _assign_board_to_shard_precheck(labels):
    """Verify whether board labels are valid to be added to a given shard.

    First check whether board label is in correct format. Second, check whether
    the board label exist. Third, check whether the board has already been
    assigned to shard.

    @param labels: Board labels separated by comma.

    @raises error.RPCException: If label provided doesn't start with `board:`
            or board has been added to shard already.
    @raises models.Label.DoesNotExist: If the label specified doesn't exist.

    @returns: A list of label models that ready to be added to shard.
    """
    if not labels:
      # allow creation of label-less shards (labels='' would otherwise fail the
      # checks below)
      return []
    labels = labels.split(',')
    label_models = []
    for label in labels:
        # Check whether the board label is in correct format.
        if not label.startswith('board:'):
            raise error.RPCException('Sharding only supports `board:.*` label.')
        # Check whether the board label exist. If not, exception will be thrown
        # by smart_get function.
        label = models.Label.smart_get(label)
        # Check whether the board has been sharded already
        try:
            shard = models.Shard.objects.get(labels=label)
            raise error.RPCException(
                    '%s is already on shard %s' % (label, shard.hostname))
        except models.Shard.DoesNotExist:
            # board is not on any shard, so it's valid.
            label_models.append(label)
    return label_models


def add_shard(hostname, labels):
    """Add a shard and start running jobs on it.

    @param hostname: The hostname of the shard to be added; needs to be unique.
    @param labels: Board labels separated by comma. Jobs of one of the labels
                   will be assigned to the shard.

    @raises error.RPCException: If label provided doesn't start with `board:` or
            board has been added to shard already.
    @raises model_logic.ValidationError: If a shard with the given hostname
            already exist.
    @raises models.Label.DoesNotExist: If the label specified doesn't exist.

    @returns: The id of the added shard.
    """
    labels = _assign_board_to_shard_precheck(labels)
    shard = models.Shard.add_object(hostname=hostname)
    for label in labels:
        shard.labels.add(label)
    return shard.id


def add_board_to_shard(hostname, labels):
    """Add boards to a given shard

    @param hostname: The hostname of the shard to be changed.
    @param labels: Board labels separated by comma.

    @raises error.RPCException: If label provided doesn't start with `board:` or
            board has been added to shard already.
    @raises models.Label.DoesNotExist: If the label specified doesn't exist.

    @returns: The id of the changed shard.
    """
    labels = _assign_board_to_shard_precheck(labels)
    shard = models.Shard.objects.get(hostname=hostname)
    for label in labels:
        shard.labels.add(label)
    return shard.id


# Remove board RPCs are rare, so we can afford to make them a bit more
# expensive (by performing in a transaction) in order to guarantee
# atomicity.
# TODO(akeshet): If we ever update to newer version of django, we need to
# migrate to transaction.atomic instead of commit_on_success
@transaction.commit_on_success
def remove_board_from_shard(hostname, label):
    """Remove board from the given shard.
    @param hostname: The hostname of the shard to be changed.
    @param labels: Board label.

    @raises models.Label.DoesNotExist: If the label specified doesn't exist.

    @returns: The id of the changed shard.
    """
    shard = models.Shard.objects.get(hostname=hostname)
    label = models.Label.smart_get(label)
    if label not in shard.labels.all():
        raise error.RPCException(
          'Cannot remove label from shard that does not belong to it.')

    shard.labels.remove(label)
    if label.is_replaced_by_static():
        static_label = models.StaticLabel.smart_get(label.name)
        models.Host.objects.filter(
                static_labels__in=[static_label]).update(shard=None)
    else:
        models.Host.objects.filter(labels__in=[label]).update(shard=None)


def delete_shard(hostname):
    """Delete a shard and reclaim all resources from it.

    This claims back all assigned hosts from the shard.
    """
    shard = rpc_utils.retrieve_shard(shard_hostname=hostname)

    # Remove shard information.
    models.Host.objects.filter(shard=shard).update(shard=None)

    # Note: The original job-cleanup query was performed with django call
    #   models.Job.objects.filter(shard=shard).update(shard=None)
    #
    # But that started becoming unreliable due to the large size of afe_jobs.
    #
    # We don't need atomicity here, so the new cleanup method is iterative, in
    # chunks of 100k jobs.
    QUERY = ('UPDATE afe_jobs SET shard_id = NULL WHERE shard_id = %s '
             'LIMIT 100000')
    try:
        with contextlib.closing(db_connection.cursor()) as cursor:
            clear_jobs = True
            assert shard.id is not None
            while clear_jobs:
                res = cursor.execute(QUERY % shard.id).fetchone()
                clear_jobs = bool(res)
    # Unit tests use sqlite backend instead of MySQL. sqlite does not support
    # UPDATE ... LIMIT, so fall back to the old behavior.
    except DatabaseError as e:
        if 'syntax error' in str(e):
            models.Job.objects.filter(shard=shard).update(shard=None)
        else:
            raise

    shard.labels.clear()
    shard.delete()


def get_servers(hostname=None, role=None, status=None):
    """Get a list of servers with matching role and status.

    @param hostname: FQDN of the server.
    @param role: Name of the server role, e.g., drone, scheduler. Default to
                 None to match any role.
    @param status: Status of the server, e.g., primary, backup, repair_required.
                   Default to None to match any server status.

    @raises error.RPCException: If server database is not used.
    @return: A list of server names for servers with matching role and status.
    """
    if not server_manager_utils.use_server_db():
        raise error.RPCException('Server database is not enabled. Please try '
                                 'retrieve servers from global config.')
    servers = server_manager_utils.get_servers(hostname=hostname, role=role,
                                               status=status)
    return [s.get_details() for s in servers]


@rpc_utils.route_rpc_to_master
def get_stable_version(board=stable_version_utils.DEFAULT, android=False):
    """Get stable version for the given board.

    @param board: Name of the board.
    @param android: If True, the given board is an Android-based device. If
                    False, assume its a Chrome OS-based device.

    @return: Stable version of the given board. Return global configure value
             of CROS.stable_cros_version if stable_versinos table does not have
             entry of board DEFAULT.
    """
    return stable_version_utils.get(board=board, android=android)


@rpc_utils.route_rpc_to_master
def get_all_stable_versions():
    """Get stable versions for all boards.

    @return: A dictionary of board:version.
    """
    return stable_version_utils.get_all()


@rpc_utils.route_rpc_to_master
def set_stable_version(version, board=stable_version_utils.DEFAULT):
    """Modify stable version for the given board.

    @param version: The new value of stable version for given board.
    @param board: Name of the board, default to value `DEFAULT`.
    """
    stable_version_utils.set(version=version, board=board)


@rpc_utils.route_rpc_to_master
def delete_stable_version(board):
    """Modify stable version for the given board.

    Delete a stable version entry in afe_stable_versions table for a given
    board, so default stable version will be used.

    @param board: Name of the board.
    """
    stable_version_utils.delete(board=board)


def get_tests_by_build(build, ignore_invalid_tests=True):
    """Get the tests that are available for the specified build.

    @param build: unique name by which to refer to the image.
    @param ignore_invalid_tests: flag on if unparsable tests are ignored.

    @return: A sorted list of all tests that are in the build specified.
    """
    # Collect the control files specified in this build
    cfile_getter = control_file_lib._initialize_control_file_getter(build)
    if SuiteBase.ENABLE_CONTROLS_IN_BATCH:
        control_file_info_list = cfile_getter.get_suite_info()
        control_file_list = control_file_info_list.keys()
    else:
        control_file_list = cfile_getter.get_control_file_list()

    test_objects = []
    _id = 0
    for control_file_path in control_file_list:
        # Read and parse the control file
        if SuiteBase.ENABLE_CONTROLS_IN_BATCH:
            control_file = control_file_info_list[control_file_path]
        else:
            control_file = cfile_getter.get_control_file_contents(
                    control_file_path)
        try:
            control_obj = control_data.parse_control_string(control_file)
        except:
            logging.info('Failed to parse control file: %s', control_file_path)
            if not ignore_invalid_tests:
                raise

        # Extract the values needed for the AFE from the control_obj.
        # The keys list represents attributes in the control_obj that
        # are required by the AFE
        keys = ['author', 'doc', 'name', 'time', 'test_type', 'experimental',
                'test_category', 'test_class', 'dependencies', 'run_verify',
                'sync_count', 'job_retries', 'retries', 'path']

        test_object = {}
        for key in keys:
            test_object[key] = getattr(control_obj, key) if hasattr(
                    control_obj, key) else ''

        # Unfortunately, the AFE expects different key-names for certain
        # values, these must be corrected to avoid the risk of tests
        # being omitted by the AFE.
        # The 'id' is an additional value used in the AFE.
        # The control_data parsing does not reference 'run_reset', but it
        # is also used in the AFE and defaults to True.
        test_object['id'] = _id
        test_object['run_reset'] = True
        test_object['description'] = test_object.get('doc', '')
        test_object['test_time'] = test_object.get('time', 0)
        test_object['test_retry'] = test_object.get('retries', 0)

        # Fix the test name to be consistent with the current presentation
        # of test names in the AFE.
        testpath, subname = os.path.split(control_file_path)
        testname = os.path.basename(testpath)
        subname = subname.split('.')[1:]
        if subname:
            testname = '%s:%s' % (testname, ':'.join(subname))

        test_object['name'] = testname

        # Correct the test path as parse_control_string sets an empty string.
        test_object['path'] = control_file_path

        _id += 1
        test_objects.append(test_object)

    test_objects = sorted(test_objects, key=lambda x: x.get('name'))
    return rpc_utils.prepare_for_serialization(test_objects)


@rpc_utils.route_rpc_to_master
def get_lab_health_indicators(board=None):
    """Get the healthy indicators for whole lab.

    The indicators now includes:
    1. lab is closed or not.
    2. Available DUTs list for a given board.
    3. Devserver capacity.
    4. When is the next major DUT utilization (e.g. CQ is coming in 3 minutes).

    @param board: if board is specified, a list of available DUTs will be
        returned for it. Otherwise, skip this indicator.

    @returns: A healthy indicator object including health info.
    """
    return LabHealthIndicator(None, None, None, None)
